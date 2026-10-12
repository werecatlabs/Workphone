/**
 * @file workphone_audio_project.c
 * @brief Implementation of the C audio project API.
 */

#include "workphone_audio_project.h"
#include "workphone_audio_sound.h"

#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal structures
 * ====================================================================== */

struct wp_audio_project
{
    wp_c8 name[WP_AUDIO_PROJECT_NAME_MAX];
    wp_c8 path[WP_AUDIO_PROJECT_PATH_MAX];

    wp_audio_sound **sounds;
    wp_s32 sound_count;
    wp_s32 sound_capacity;

    wp_f32 volume;
    wp_s32 mute;
    wp_s32 loaded;

    void *native;
    void *user_data;
};

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static wp_s32 wp_audio_project_grow_sounds( wp_audio_project *project )
{
    wp_s32 new_cap;
    wp_audio_sound **new_sounds;

    new_cap = project->sound_capacity * 2;
    if( new_cap < WP_AUDIO_PROJECT_INITIAL_SOUND_CAPACITY )
    {
        new_cap = WP_AUDIO_PROJECT_INITIAL_SOUND_CAPACITY;
    }

    new_sounds =
        (wp_audio_sound **)realloc( project->sounds, (wp_u32)new_cap * sizeof( wp_audio_sound * ) );
    if( !new_sounds )
    {
        return 0;
    }

    memset( new_sounds + project->sound_capacity, 0,
            (wp_u32)( new_cap - project->sound_capacity ) * sizeof( wp_audio_sound * ) );

    project->sounds = new_sounds;
    project->sound_capacity = new_cap;
    return 1;
}

static void wp_audio_project_build_filepath( const wp_audio_project *project, const wp_c8 *filepath,
                                             wp_c8 *out, wp_size out_size )
{
    wp_size path_len;
    wp_size file_len;
    wp_size total;

    path_len = strlen( (const char *)project->path );
    file_len = strlen( (const char *)filepath );

    /* Use filepath as-is if there is no base path or it is absolute. */
    if( path_len == 0 || filepath[0] == '/' || filepath[0] == '\\' ||
        ( file_len > 1 && filepath[1] == ':' ) )
    {
        total = file_len < out_size - 1 ? file_len : out_size - 1;
        memcpy( out, filepath, total );
        out[total] = '\0';
        return;
    }

    total = path_len;
    if( total < out_size - 1 )
    {
        memcpy( out, project->path, total );
    }

    /* Append path separator if needed. */
    if( total < out_size - 1 && project->path[total - 1] != '/' && project->path[total - 1] != '\\' )
    {
        out[total++] = '/';
    }

    if( total + file_len < out_size )
    {
        memcpy( out + total, filepath, file_len );
        out[total + file_len] = '\0';
    }
    else
    {
        wp_size remaining = out_size - total - 1;
        memcpy( out + total, filepath, remaining );
        out[out_size - 1] = '\0';
    }
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_audio_project *wp_audio_project_create( void )
{
    wp_audio_project *project = (wp_audio_project *)malloc( sizeof( wp_audio_project ) );
    if( !project )
    {
        return NULL;
    }

    memset( project, 0, sizeof( wp_audio_project ) );

    project->sounds = (wp_audio_sound **)malloc( WP_AUDIO_PROJECT_INITIAL_SOUND_CAPACITY *
                                                 sizeof( wp_audio_sound * ) );
    if( !project->sounds )
    {
        free( project );
        return NULL;
    }
    memset( project->sounds, 0, WP_AUDIO_PROJECT_INITIAL_SOUND_CAPACITY * sizeof( wp_audio_sound * ) );
    project->sound_capacity = WP_AUDIO_PROJECT_INITIAL_SOUND_CAPACITY;

    project->volume = 1.0f;
    project->mute = 0;
    project->loaded = 0;
    project->native = NULL;
    project->user_data = NULL;

    return project;
}

void wp_audio_project_destroy( wp_audio_project *project )
{
    if( !project )
    {
        return;
    }

    wp_audio_project_unload( project );
    wp_audio_project_remove_all_sounds( project );

    free( project->sounds );
    free( project );
}

wp_s32 wp_audio_project_load( wp_audio_project *project )
{
    if( !project )
    {
        return 0;
    }

    if( project->loaded )
    {
        return 1;
    }

    /* Platform-specific initialisation would go here. */

    project->loaded = 1;
    return 1;
}

void wp_audio_project_unload( wp_audio_project *project )
{
    wp_s32 i;

    if( !project )
    {
        return;
    }

    if( !project->loaded )
    {
        return;
    }

    for( i = 0; i < project->sound_count; ++i )
    {
        if( project->sounds[i] )
        {
            wp_audio_sound_unload( project->sounds[i] );
        }
    }

    /* Platform-specific cleanup would go here. */

    project->loaded = 0;
}

/* =========================================================================
 * Update
 * ====================================================================== */

void wp_audio_project_update( wp_audio_project *project )
{
    (void)project;
}

/* =========================================================================
 * Name
 * ====================================================================== */

const wp_c8 *wp_audio_project_get_name( const wp_audio_project *project )
{
    if( !project )
    {
        return NULL;
    }

    return project->name;
}

void wp_audio_project_set_name( wp_audio_project *project, const wp_c8 *name )
{
    wp_size len;

    if( !project || !name )
    {
        return;
    }

    len = strlen( (const char *)name );
    if( len >= WP_AUDIO_PROJECT_NAME_MAX )
    {
        len = WP_AUDIO_PROJECT_NAME_MAX - 1;
    }

    memcpy( project->name, name, len );
    project->name[len] = '\0';
}

/* =========================================================================
 * Path
 * ====================================================================== */

const wp_c8 *wp_audio_project_get_path( const wp_audio_project *project )
{
    if( !project )
    {
        return NULL;
    }

    return project->path;
}

void wp_audio_project_set_path( wp_audio_project *project, const wp_c8 *path )
{
    wp_size len;

    if( !project || !path )
    {
        return;
    }

    len = strlen( (const char *)path );
    if( len >= WP_AUDIO_PROJECT_PATH_MAX )
    {
        len = WP_AUDIO_PROJECT_PATH_MAX - 1;
    }

    memcpy( project->path, path, len );
    project->path[len] = '\0';
}

/* =========================================================================
 * Sound management
 * ====================================================================== */

wp_s32 wp_audio_project_add_sound( wp_audio_project *project, const wp_c8 *filepath, wp_s32 loop )
{
    wp_audio_sound *sound;
    wp_c8 full_path[WP_AUDIO_PROJECT_PATH_MAX * 2];

    if( !project || !filepath )
    {
        return -1;
    }

    if( project->sound_count >= project->sound_capacity )
    {
        if( !wp_audio_project_grow_sounds( project ) )
        {
            return -1;
        }
    }

    sound = wp_audio_sound_create();
    if( !sound )
    {
        return -1;
    }

    wp_audio_project_build_filepath( project, filepath, full_path, sizeof( full_path ) );

    if( !wp_audio_sound_load( sound, full_path, loop ) )
    {
        wp_audio_sound_destroy( sound );
        return -1;
    }

    project->sounds[project->sound_count] = sound;
    return project->sound_count++;
}

void wp_audio_project_remove_sound( wp_audio_project *project, wp_s32 index )
{
    if( !project || index < 0 || index >= project->sound_count )
    {
        return;
    }

    if( project->sounds[index] )
    {
        wp_audio_sound_destroy( project->sounds[index] );
    }

    /* Swap with last to keep the array compact. */
    project->sounds[index] = project->sounds[--project->sound_count];
    project->sounds[project->sound_count] = NULL;
}

void wp_audio_project_remove_all_sounds( wp_audio_project *project )
{
    wp_s32 i;

    if( !project )
    {
        return;
    }

    for( i = 0; i < project->sound_count; ++i )
    {
        if( project->sounds[i] )
        {
            wp_audio_sound_destroy( project->sounds[i] );
            project->sounds[i] = NULL;
        }
    }
    project->sound_count = 0;
}

wp_audio_sound *wp_audio_project_get_sound( const wp_audio_project *project, wp_s32 index )
{
    if( !project || index < 0 || index >= project->sound_count )
    {
        return NULL;
    }

    return project->sounds[index];
}

wp_s32 wp_audio_project_get_sound_count( const wp_audio_project *project )
{
    if( !project )
    {
        return 0;
    }

    return project->sound_count;
}

/* =========================================================================
 * Volume
 * ====================================================================== */

wp_f32 wp_audio_project_get_volume( const wp_audio_project *project )
{
    if( !project )
    {
        return 0.0f;
    }

    return project->volume;
}

void wp_audio_project_set_volume( wp_audio_project *project, wp_f32 volume )
{
    if( !project )
    {
        return;
    }

    project->volume = volume;
}

/* =========================================================================
 * Mute
 * ====================================================================== */

wp_s32 wp_audio_project_is_mute( const wp_audio_project *project )
{
    if( !project )
    {
        return 0;
    }

    return project->mute;
}

void wp_audio_project_set_mute( wp_audio_project *project, wp_s32 mute )
{
    if( !project )
    {
        return;
    }

    project->mute = mute ? 1 : 0;
}

/* =========================================================================
 * Loaded state
 * ====================================================================== */

wp_s32 wp_audio_project_is_loaded( const wp_audio_project *project )
{
    if( !project )
    {
        return 0;
    }

    return project->loaded;
}

/* =========================================================================
 * Native access
 * ====================================================================== */

void wp_audio_project_get_native( const wp_audio_project *project, void **pp_object )
{
    if( !pp_object )
    {
        return;
    }

    *pp_object = project ? project->native : NULL;
}

void wp_audio_project_set_native( wp_audio_project *project, void *native )
{
    if( !project )
    {
        return;
    }

    project->native = native;
}

/* =========================================================================
 * User data
 * ====================================================================== */

void *wp_audio_project_get_user_data( const wp_audio_project *project )
{
    if( !project )
    {
        return NULL;
    }

    return project->user_data;
}

void wp_audio_project_set_user_data( wp_audio_project *project, void *user_data )
{
    if( !project )
    {
        return;
    }

    project->user_data = user_data;
}
