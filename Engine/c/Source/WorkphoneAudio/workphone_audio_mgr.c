/**
 * @file workphone_audio_mgr.c
 * @brief Implementation of the C audio manager API.
 */

#include "workphone_audio_mgr.h"
#include "workphone_audio_sound.h"
#include "workphone_audio_listener.h"

#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Internal state
 * ---------------------------------------------------------------------- */

static wp_s32 g_platform_initialised = 0;

/* -------------------------------------------------------------------------
 * Platform backend include and dispatch
 * ---------------------------------------------------------------------- */

#if WP_AUDIO_PLATFORM_WINDOWS
static const wp_audio_mgr_backend *g_platform_backend = NULL;
#    include "workphone_audio_mgr_win32.c"
#elif WP_AUDIO_PLATFORM_MACOS
static const wp_audio_mgr_backend *g_platform_backend = NULL;
#    include "workphone_audio_mgr_macos.c"
#elif WP_AUDIO_PLATFORM_IOS
static const wp_audio_mgr_backend *g_platform_backend = NULL;
#    include "workphone_audio_mgr_ios.m"
#elif WP_AUDIO_PLATFORM_ANDROID
static const wp_audio_mgr_backend *g_platform_backend = NULL;
#    include "workphone_audio_mgr_android.c"
#else
static const wp_audio_mgr_backend *g_platform_backend = NULL;
#endif

static void wp_audio_mgr_init_backend( void )
{
    if( !g_platform_backend )
        g_platform_backend = wp_audio_mgr_get_backend();
}

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

/* =========================================================================
 * Internal structures
 * ====================================================================== */

struct wp_audio_mgr
{
    wp_audio_sound **sounds;
    wp_s32 sound_count;
    wp_s32 sound_capacity;

    wp_audio_listener **listeners;
    wp_s32 listener_count;
    wp_s32 listener_capacity;

    wp_f32 volume;
    wp_s32 mute;
    wp_s32 loaded;

    void *native;
    void *user_data;
};

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static wp_s32 wp_audio_mgr_grow_sounds( wp_audio_mgr *mgr )
{
    wp_s32 new_cap;
    wp_audio_sound **new_sounds;

    new_cap = mgr->sound_capacity * 2;
    if( new_cap < WP_AUDIO_MGR_INITIAL_SOUND_CAPACITY )
        new_cap = WP_AUDIO_MGR_INITIAL_SOUND_CAPACITY;

    new_sounds = (wp_audio_sound **)realloc( mgr->sounds, (wp_u32)new_cap * sizeof( wp_audio_sound * ) );
    if( !new_sounds )
        return 0;

    memset( new_sounds + mgr->sound_capacity, 0,
            (wp_u32)( new_cap - mgr->sound_capacity ) * sizeof( wp_audio_sound * ) );

    mgr->sounds = new_sounds;
    mgr->sound_capacity = new_cap;
    return 1;
}

static wp_s32 wp_audio_mgr_grow_listeners( wp_audio_mgr *mgr )
{
    wp_s32 new_cap;
    wp_audio_listener **new_listeners;

    new_cap = mgr->listener_capacity * 2;
    if( new_cap < WP_AUDIO_MGR_INITIAL_LISTENER_CAPACITY )
        new_cap = WP_AUDIO_MGR_INITIAL_LISTENER_CAPACITY;

    new_listeners =
        (wp_audio_listener **)realloc( mgr->listeners, (wp_u32)new_cap * sizeof( wp_audio_listener * ) );
    if( !new_listeners )
        return 0;

    memset( new_listeners + mgr->listener_capacity, 0,
            (wp_u32)( new_cap - mgr->listener_capacity ) * sizeof( wp_audio_listener * ) );

    mgr->listeners = new_listeners;
    mgr->listener_capacity = new_cap;
    return 1;
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_audio_mgr *wp_audio_mgr_create( void )
{
    wp_audio_mgr *mgr = (wp_audio_mgr *)malloc( sizeof( wp_audio_mgr ) );
    if( !mgr )
        return NULL;

    memset( mgr, 0, sizeof( wp_audio_mgr ) );

    mgr->sounds =
        (wp_audio_sound **)malloc( WP_AUDIO_MGR_INITIAL_SOUND_CAPACITY * sizeof( wp_audio_sound * ) );
    if( !mgr->sounds )
    {
        free( mgr );
        return NULL;
    }
    memset( mgr->sounds, 0, WP_AUDIO_MGR_INITIAL_SOUND_CAPACITY * sizeof( wp_audio_sound * ) );
    mgr->sound_capacity = WP_AUDIO_MGR_INITIAL_SOUND_CAPACITY;

    mgr->listeners = (wp_audio_listener **)malloc( WP_AUDIO_MGR_INITIAL_LISTENER_CAPACITY *
                                                   sizeof( wp_audio_listener * ) );
    if( !mgr->listeners )
    {
        free( mgr->sounds );
        free( mgr );
        return NULL;
    }
    memset( mgr->listeners, 0, WP_AUDIO_MGR_INITIAL_LISTENER_CAPACITY * sizeof( wp_audio_listener * ) );
    mgr->listener_capacity = WP_AUDIO_MGR_INITIAL_LISTENER_CAPACITY;

    mgr->volume = 1.0f;
    mgr->mute = 0;
    mgr->loaded = 0;
    mgr->native = NULL;
    mgr->user_data = NULL;

    return mgr;
}

void wp_audio_mgr_destroy( wp_audio_mgr *mgr )
{
    if( !mgr )
        return;

    wp_audio_mgr_unload( mgr );
    wp_audio_mgr_remove_all_sounds( mgr );
    wp_audio_mgr_remove_all_listeners( mgr );

    free( mgr->sounds );
    free( mgr->listeners );
    free( mgr );
}

wp_s32 wp_audio_mgr_load( wp_audio_mgr *mgr )
{
    if( !mgr )
        return 0;

    if( mgr->loaded )
        return 1;

    /* Initialise platform audio backend if not already done */
    if( !g_platform_initialised )
    {
        wp_audio_mgr_init_backend();
        if( g_platform_backend && g_platform_backend->init )
        {
            if( !g_platform_backend->init() )
                return 0;
        }
        g_platform_initialised = 1;
    }

    mgr->loaded = 1;
    return 1;
}

void wp_audio_mgr_unload( wp_audio_mgr *mgr )
{
    if( !mgr )
        return;

    if( !mgr->loaded )
        return;

    wp_audio_mgr_remove_all_sounds( mgr );
    wp_audio_mgr_remove_all_listeners( mgr );

    mgr->loaded = 0;

    /* Shutdown platform audio backend if no managers are loaded */
    /* Note: In a refcount scenario, you'd track manager count here.
     * For simplicity, we leave the backend running. */
}

/* =========================================================================
 * Update
 * ====================================================================== */

void wp_audio_mgr_update( wp_audio_mgr *mgr )
{
    if( !mgr )
        return;

    if( g_platform_backend && g_platform_backend->update )
        g_platform_backend->update( mgr );
}

/* =========================================================================
 * Sound management
 * ====================================================================== */

wp_s32 wp_audio_mgr_add_sound( wp_audio_mgr *mgr, const wp_c8 *filepath, wp_s32 loop )
{
    wp_audio_sound *sound;

    if( !mgr || !filepath )
        return -1;

    if( mgr->sound_count >= mgr->sound_capacity )
    {
        if( !wp_audio_mgr_grow_sounds( mgr ) )
            return -1;
    }

    sound = wp_audio_sound_create();
    if( !sound )
        return -1;

    if( !wp_audio_sound_load( sound, filepath, loop ) )
    {
        wp_audio_sound_destroy( sound );
        return -1;
    }

    mgr->sounds[mgr->sound_count] = sound;
    return mgr->sound_count++;
}

void wp_audio_mgr_remove_sound( wp_audio_mgr *mgr, wp_s32 index )
{
    wp_audio_sound *sound;

    if( !mgr || index < 0 || index >= mgr->sound_count )
        return;

    sound = mgr->sounds[index];
    if( sound )
        wp_audio_sound_destroy( sound );

    /* Swap with last to keep the array compact. */
    mgr->sounds[index] = mgr->sounds[--mgr->sound_count];
    mgr->sounds[mgr->sound_count] = NULL;
}

void wp_audio_mgr_remove_all_sounds( wp_audio_mgr *mgr )
{
    wp_s32 i;

    if( !mgr )
        return;

    for( i = 0; i < mgr->sound_count; ++i )
    {
        if( mgr->sounds[i] )
        {
            wp_audio_sound_destroy( mgr->sounds[i] );
            mgr->sounds[i] = NULL;
        }
    }
    mgr->sound_count = 0;
}

wp_audio_sound *wp_audio_mgr_get_sound( const wp_audio_mgr *mgr, wp_s32 index )
{
    if( !mgr || index < 0 || index >= mgr->sound_count )
        return NULL;

    return mgr->sounds[index];
}

wp_s32 wp_audio_mgr_get_sound_count( const wp_audio_mgr *mgr )
{
    if( !mgr )
        return 0;

    return mgr->sound_count;
}

/* =========================================================================
 * Listener management
 * ====================================================================== */

wp_s32 wp_audio_mgr_add_listener( wp_audio_mgr *mgr, const wp_c8 *name, wp_f32 x, wp_f32 y, wp_f32 z )
{
    wp_audio_listener *listener;

    if( !mgr || !name )
        return -1;

    if( mgr->listener_count >= mgr->listener_capacity )
    {
        if( !wp_audio_mgr_grow_listeners( mgr ) )
            return -1;
    }

    listener = wp_audio_listener_create();
    if( !listener )
        return -1;

    wp_audio_listener_set_name( listener, name );
    wp_audio_listener_set_position( listener, wp_vec3f_make( x, y, z ) );

    mgr->listeners[mgr->listener_count] = listener;
    return mgr->listener_count++;
}

wp_audio_listener *wp_audio_mgr_find_listener( const wp_audio_mgr *mgr, const wp_c8 *name )
{
    wp_s32 i;

    if( !mgr || !name )
        return NULL;

    for( i = 0; i < mgr->listener_count; ++i )
    {
        if( mgr->listeners[i] && strcmp( mgr->listeners[i]->name, name ) == 0 )
            return mgr->listeners[i];
    }

    return NULL;
}

void wp_audio_mgr_remove_listener( wp_audio_mgr *mgr, wp_s32 index )
{
    if( !mgr || index < 0 || index >= mgr->listener_count )
        return;

    wp_audio_listener_destroy( mgr->listeners[index] );

    /* Swap with last to keep the array compact. */
    mgr->listeners[index] = mgr->listeners[--mgr->listener_count];
    mgr->listeners[mgr->listener_count] = NULL;
}

void wp_audio_mgr_remove_all_listeners( wp_audio_mgr *mgr )
{
    wp_s32 i;

    if( !mgr )
        return;

    for( i = 0; i < mgr->listener_count; ++i )
    {
        wp_audio_listener_destroy( mgr->listeners[i] );
        mgr->listeners[i] = NULL;
    }
    mgr->listener_count = 0;
}

wp_audio_listener *wp_audio_mgr_get_listener( const wp_audio_mgr *mgr, wp_s32 index )
{
    if( !mgr || index < 0 || index >= mgr->listener_count )
        return NULL;

    return mgr->listeners[index];
}

wp_s32 wp_audio_mgr_get_listener_count( const wp_audio_mgr *mgr )
{
    if( !mgr )
        return 0;

    return mgr->listener_count;
}

/* =========================================================================
 * Volume
 * ====================================================================== */

wp_f32 wp_audio_mgr_get_volume( const wp_audio_mgr *mgr )
{
    if( !mgr )
        return 0.0f;

    return mgr->volume;
}

void wp_audio_mgr_set_volume( wp_audio_mgr *mgr, wp_f32 volume )
{
    if( !mgr )
        return;

    mgr->volume = volume;
}

/* =========================================================================
 * Mute
 * ====================================================================== */

wp_s32 wp_audio_mgr_is_mute( const wp_audio_mgr *mgr )
{
    if( !mgr )
        return 0;

    return mgr->mute;
}

void wp_audio_mgr_set_mute( wp_audio_mgr *mgr, wp_s32 mute )
{
    if( !mgr )
        return;

    mgr->mute = mute ? 1 : 0;
}

/* =========================================================================
 * Native access
 * ====================================================================== */

void wp_audio_mgr_get_native( const wp_audio_mgr *mgr, void **pp_object )
{
    if( !pp_object )
        return;

    *pp_object = mgr ? mgr->native : NULL;
}

void wp_audio_mgr_set_native( wp_audio_mgr *mgr, void *native )
{
    if( !mgr )
        return;

    mgr->native = native;
}

/* =========================================================================
 * User data
 * ====================================================================== */

void *wp_audio_mgr_get_user_data( const wp_audio_mgr *mgr )
{
    if( !mgr )
        return NULL;

    return mgr->user_data;
}

void wp_audio_mgr_set_user_data( wp_audio_mgr *mgr, void *user_data )
{
    if( !mgr )
        return;

    mgr->user_data = user_data;
}

/* =========================================================================
 * Platform subsystem lifecycle
 * ====================================================================== */

wp_s32 wp_audio_mgr_platform_init( void )
{
    if( g_platform_initialised )
        return 1;

    wp_audio_mgr_init_backend();

    if( g_platform_backend && g_platform_backend->init )
    {
        if( !g_platform_backend->init() )
            return 0;
    }

    g_platform_initialised = 1;
    return 1;
}

void wp_audio_mgr_platform_shutdown( void )
{
    if( !g_platform_initialised )
        return;

    if( g_platform_backend && g_platform_backend->shutdown )
        g_platform_backend->shutdown();

    g_platform_initialised = 0;
}
