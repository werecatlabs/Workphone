/**
 * @file workphone_audio_sound.c
 * @brief Implementation of the C audio sound API.
 */

#include "workphone_audio_sound.h"

#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static wp_c8 *wp_audio_sound_strdup( const wp_c8 *s )
{
    wp_c8 *copy;
    wp_size len;

    if( !s )
        return NULL;

    len = strlen( s ) + 1;
    copy = (wp_c8 *)malloc( len );
    if( copy )
        memcpy( copy, s, len );
    return copy;
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_audio_sound *wp_audio_sound_create( void )
{
    wp_audio_sound *sound = (wp_audio_sound *)malloc( sizeof( wp_audio_sound ) );
    if( !sound )
        return NULL;

    memset( sound, 0, sizeof( wp_audio_sound ) );
    sound->volume = 1.0f;
    sound->state = WP_AUDIO_SOUND_STATE_STOPPED;

    return sound;
}

void wp_audio_sound_destroy( wp_audio_sound *sound )
{
    if( !sound )
        return;

    wp_audio_sound_unload( sound );
    free( sound );
}

/* =========================================================================
 * Loading
 * ====================================================================== */

wp_s32 wp_audio_sound_load( wp_audio_sound *sound, const wp_c8 *filepath, wp_s32 loop )
{
    if( !sound || !filepath )
        return 0;

    /* Unload any previously loaded data. */
    wp_audio_sound_unload( sound );

    sound->filepath = wp_audio_sound_strdup( filepath );
    if( !sound->filepath )
        return 0;

    sound->loop = loop ? 1 : 0;
    sound->loaded = 1;

    /* Platform-specific audio resource loading would go here. */

    return 1;
}

void wp_audio_sound_unload( wp_audio_sound *sound )
{
    if( !sound )
        return;

    if( sound->state == WP_AUDIO_SOUND_STATE_PLAYING )
        wp_audio_sound_stop( sound );

    /* Platform-specific audio resource cleanup would go here. */

    free( sound->filepath );
    sound->filepath = NULL;
    sound->native = NULL;
    sound->loaded = 0;
}

wp_s32 wp_audio_sound_is_loaded( const wp_audio_sound *sound )
{
    if( !sound )
        return 0;

    return sound->loaded;
}

/* =========================================================================
 * Playback
 * ====================================================================== */

void wp_audio_sound_play( wp_audio_sound *sound )
{
    if( !sound || !sound->loaded )
        return;

    /* Platform-specific play would go here. */

    sound->state = WP_AUDIO_SOUND_STATE_PLAYING;
}

void wp_audio_sound_stop( wp_audio_sound *sound )
{
    if( !sound )
        return;

    /* Platform-specific stop would go here. */

    sound->state = WP_AUDIO_SOUND_STATE_STOPPED;
}

wp_s32 wp_audio_sound_is_playing( const wp_audio_sound *sound )
{
    if( !sound )
        return 0;

    return sound->state == WP_AUDIO_SOUND_STATE_PLAYING ? 1 : 0;
}

wp_audio_sound_state wp_audio_sound_get_state( const wp_audio_sound *sound )
{
    if( !sound )
        return WP_AUDIO_SOUND_STATE_STOPPED;

    return sound->state;
}

/* =========================================================================
 * Volume
 * ====================================================================== */

wp_f32 wp_audio_sound_get_volume( const wp_audio_sound *sound )
{
    if( !sound )
        return 0.0f;

    return sound->volume;
}

void wp_audio_sound_set_volume( wp_audio_sound *sound, wp_f32 volume )
{
    if( !sound )
        return;

    sound->volume = volume;

    /* Platform-specific volume change would go here. */
}

/* =========================================================================
 * Loop
 * ====================================================================== */

wp_s32 wp_audio_sound_get_loop( const wp_audio_sound *sound )
{
    if( !sound )
        return 0;

    return sound->loop;
}

void wp_audio_sound_set_loop( wp_audio_sound *sound, wp_s32 loop )
{
    if( !sound )
        return;

    sound->loop = loop ? 1 : 0;

    /* Platform-specific loop state change would go here. */
}

/* =========================================================================
 * File path
 * ====================================================================== */

const wp_c8 *wp_audio_sound_get_filepath( const wp_audio_sound *sound )
{
    if( !sound )
        return NULL;

    return sound->filepath;
}

/* =========================================================================
 * Native access
 * ====================================================================== */

void wp_audio_sound_get_native( const wp_audio_sound *sound, void **pp_object )
{
    if( !pp_object )
        return;

    *pp_object = sound ? sound->native : NULL;
}

void wp_audio_sound_set_native( wp_audio_sound *sound, void *native )
{
    if( !sound )
        return;

    sound->native = native;
}

/* =========================================================================
 * User data
 * ====================================================================== */

void *wp_audio_sound_get_user_data( const wp_audio_sound *sound )
{
    if( !sound )
        return NULL;

    return sound->user_data;
}

void wp_audio_sound_set_user_data( wp_audio_sound *sound, void *user_data )
{
    if( !sound )
        return;

    sound->user_data = user_data;
}
