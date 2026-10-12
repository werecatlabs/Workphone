/**
 * @file wp_graphics_system.c
 * @brief Implementation of the C graphics system API.
 */

#include "workphone_graphics_system.h"
#include "workphone_graphics_scene.h"
#include "workphone_graphics_scenenode.h"
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Capacity constants
 * ====================================================================== */

#define WORKPHONE_SYSTEM_INITIAL_SCENE_CAPACITY 4

/* =========================================================================
 * Internal structures
 * ====================================================================== */

struct wp_graphics_system
{
    wp_graphics_scene **scenes;
    wp_s32 scene_count;
    wp_s32 scene_capacity;
    wp_renderer *renderer;
    void *native;
    wp_graphics_system_frame_func frame_func;
    void *frame_user_data;
    wp_graphics_system_render_pass passes[WP_GRAPHICS_SYSTEM_MAX_RENDER_PASSES];
    wp_s32 pass_count;
    wp_s32 rendering;
    wp_s32 legacy_callback;
    wp_s32 drawing_scenes;
    wp_f32 delta_time;
};

/* =========================================================================
 * Graphics system — lifecycle
 * ====================================================================== */

wp_graphics_system *wp_graphics_system_create( void )
{
    wp_graphics_system *system = (wp_graphics_system *)malloc( sizeof( wp_graphics_system ) );
    if( !system )
    {
        return NULL;
    }

    memset( system, 0, sizeof( wp_graphics_system ) );

    system->scenes = (wp_graphics_scene **)malloc( WORKPHONE_SYSTEM_INITIAL_SCENE_CAPACITY *
                                                   sizeof( wp_graphics_scene * ) );
    if( !system->scenes )
    {
        free( system );
        return NULL;
    }
    system->scene_capacity = WORKPHONE_SYSTEM_INITIAL_SCENE_CAPACITY;

    return system;
}

void wp_graphics_system_destroy( wp_graphics_system *system )
{
    wp_s32 i;

    if( !system || system->rendering || system->drawing_scenes )
    {
        return;
    }

    for( i = 0; i < system->scene_count; ++i )
    {
        wp_graphics_scene_destroy( system->scenes[i] );
    }
    free( system->scenes );
    free( system );
}

/* =========================================================================
 * Graphics system — scene management
 * ====================================================================== */

wp_graphics_scene *wp_graphics_system_create_scene( wp_graphics_system *system )
{
    wp_graphics_scene *scene;
    wp_graphics_scene **new_scenes;
    wp_s32 new_cap;

    if( !system || system->rendering || system->drawing_scenes )
    {
        return NULL;
    }

    scene = wp_graphics_scene_create();
    if( !scene )
    {
        return NULL;
    }

    if( system->scene_count >= system->scene_capacity )
    {
        new_cap = system->scene_capacity * 2;
        new_scenes = (wp_graphics_scene **)realloc( system->scenes,
                                                    (wp_u32)new_cap * sizeof( wp_graphics_scene * ) );
        if( !new_scenes )
        {
            wp_graphics_scene_destroy( scene );
            return NULL;
        }
        system->scenes = new_scenes;
        system->scene_capacity = new_cap;
    }

    system->scenes[system->scene_count++] = scene;

    return scene;
}

void wp_graphics_system_destroy_scene( wp_graphics_system *system, wp_graphics_scene *scene )
{
    wp_s32 i;

    if( !system || !scene || system->rendering || system->drawing_scenes )
    {
        return;
    }

    for( i = 0; i < system->scene_count; ++i )
    {
        if( system->scenes[i] == scene )
        {
            system->scenes[i] = system->scenes[--system->scene_count];
            wp_graphics_scene_destroy( scene );
            return;
        }
    }
}

wp_graphics_scene *wp_graphics_system_get_scene( const wp_graphics_system *system, wp_s32 index )
{
    if( !system || index < 0 || index >= system->scene_count )
    {
        return NULL;
    }

    return system->scenes[index];
}

wp_s32 wp_graphics_system_get_scene_count( const wp_graphics_system *system )
{
    if( !system )
    {
        return 0;
    }

    return system->scene_count;
}

/* =========================================================================
 * Graphics system — update and render
 * ====================================================================== */

static void wp_system_update_node( wp_scenenode *node )
{
    wp_s32 i;
    wp_s32 child_count;
    wp_scenenode *child_node;

    if( !node )
    {
        return;
    }

    child_count = wp_scenenode_get_child_count( node );
    for( i = 0; i < child_count; ++i )
    {
        child_node = wp_scenenode_get_child( node, i );
        wp_system_update_node( child_node );
    }
}

static void wp_system_update_scenes( wp_graphics_system *system )
{
    wp_s32 i;
    wp_graphics_scene *scene;
    wp_scenenode *root_node;

    if( !system )
    {
        return;
    }

    for( i = 0; i < system->scene_count; ++i )
    {
        scene = system->scenes[i];
        if( !scene )
        {
            continue;
        }

        root_node = scene->root_node;
        wp_system_update_node( root_node );
    }
}

void wp_graphics_system_update( wp_graphics_system *system )
{
    if( !system || system->rendering || system->drawing_scenes ) return;
    wp_system_update_scenes( system );
}

wp_s32 wp_graphics_system_set_delta_time( wp_graphics_system *system, wp_f32 delta_time )
{
    if( !system || system->rendering || system->drawing_scenes ) return 0;
    if( !(delta_time >= 0.0f) ) delta_time = 0.0f;
    if( delta_time > 1.0f ) delta_time = 1.0f;
    system->delta_time = delta_time;
    return 1;
}

static void wp_system_render_scenes( wp_graphics_system *system )
{
    wp_s32 i;
    wp_graphics_scene *scene;

    if( !system || !system->renderer || system->drawing_scenes )
    {
        return;
    }

    system->drawing_scenes = 1;
    for( i = 0; i < system->scene_count; ++i )
    {
        scene = system->scenes[i];
        if( !scene )
        {
            continue;
        }

        wp_graphics_scene_render( scene, system->renderer );
    }
    system->drawing_scenes = 0;
}

static void wp_system_execute_passes( wp_graphics_system *system )
{
    wp_s32 i;
    wp_graphics_system_render_pass *pass;
    for( i = 0; i < system->pass_count; ++i )
    {
        pass = &system->passes[i];
        if( pass->begin && !pass->begin( system, system->delta_time, pass->user_data ) )
        {
            continue;
        }
        if( pass->draw ) pass->draw( system, system->delta_time, pass->user_data );
        else wp_system_render_scenes( system );
        if( pass->effects ) pass->effects( system, system->delta_time, pass->user_data );
        if( pass->ui ) pass->ui( system, system->delta_time, pass->user_data );
        if( pass->overlay ) pass->overlay( system, system->delta_time, pass->user_data );
        if( pass->end ) pass->end( system, system->delta_time, pass->user_data );
    }
}

void wp_graphics_system_render( wp_graphics_system *system )
{
    if( !system || system->drawing_scenes ) return;
    if( system->rendering )
    {
        if( system->legacy_callback ) wp_system_render_scenes( system );
        return;
    }
    system->rendering = 1;
    if( system->pass_count ) wp_system_execute_passes( system );
    else wp_system_render_scenes( system );
    system->rendering = 0;
}

wp_s32 wp_graphics_system_set_render_passes( wp_graphics_system *system,
                                            const wp_graphics_system_render_pass *passes,
                                            wp_s32 count )
{
    if( !system || system->rendering || system->drawing_scenes || count < 0 ||
        count > WP_GRAPHICS_SYSTEM_MAX_RENDER_PASSES || (count > 0 && !passes) ) return 0;
    if( count > 0 ) memmove( system->passes, passes, (size_t)count * sizeof( *passes ) );
    system->pass_count = count;
    return 1;
}

void wp_graphics_system_render_frame( wp_graphics_system *system, wp_f32 delta_time )
{
    if( !system || system->rendering || system->drawing_scenes )
    {
        return;
    }

    wp_graphics_system_set_delta_time( system, delta_time );
    delta_time = system->delta_time;
    system->rendering = 1;
    wp_system_update_scenes( system );

    if( system->pass_count )
    {
        wp_system_execute_passes( system );
    }
    else if( system->frame_func )
    {
        system->legacy_callback = 1;
        system->frame_func( system, delta_time, system->frame_user_data );
        system->legacy_callback = 0;
    }
    else
    {
        wp_system_render_scenes( system );
    }
    system->rendering = 0;
}

void wp_graphics_system_set_frame_func( wp_graphics_system *system,
                                        wp_graphics_system_frame_func frame_func, void *user_data )
{
    if( !system || system->rendering || system->drawing_scenes )
    {
        return;
    }

    system->frame_func = frame_func;
    system->frame_user_data = frame_func ? user_data : NULL;
}

/* =========================================================================
 * Graphics system — renderer binding
 * ====================================================================== */

void wp_graphics_system_set_renderer( wp_graphics_system *system, wp_renderer *renderer )
{
    if( !system || system->drawing_scenes )
    {
        return;
    }

    system->renderer = renderer;
}

wp_renderer *wp_graphics_system_get_renderer( const wp_graphics_system *system )
{
    if( !system )
    {
        return NULL;
    }

    return system->renderer;
}

/* =========================================================================
 * Graphics system — native access
 * ====================================================================== */

void wp_graphics_system_get_native( const wp_graphics_system *system, void **pp_object )
{
    if( !pp_object )
    {
        return;
    }

    *pp_object = system ? system->native : NULL;
}

void wp_graphics_system_set_native( wp_graphics_system *system, void *native )
{
    if( !system )
    {
        return;
    }

    system->native = native;
}
