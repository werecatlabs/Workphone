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

typedef struct wp_graphics_system
{
    wp_graphics_scene **scenes;
    wp_s32 scene_count;
    wp_s32 scene_capacity;
    wp_renderer *renderer;
    void *native;
    wp_graphics_system_frame_func frame_func;
    void *frame_user_data;
} wp_graphics_system;

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

    if( !system )
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

    if( !system )
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

    if( !system || !scene )
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

    if( !node )
    {
        return;
    }

    wp_s32 child_count = wp_scenenode_get_child_count( node );
    for( i = 0; i < child_count; ++i )
    {
        wp_scenenode *child_node = wp_scenenode_get_child( node, i );
        wp_system_update_node( child_node );
    }
}

void wp_graphics_system_update( wp_graphics_system *system )
{
    wp_s32 i;
    wp_graphics_scene *scene;

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

        wp_scenenode *root_node = scene->root_node;
        wp_system_update_node( root_node );
    }
}

void wp_graphics_system_render( wp_graphics_system *system )
{
    wp_s32 i;
    wp_graphics_scene *scene;

    if( !system || !system->renderer )
    {
        return;
    }

    wp_graphics_scene **scenes = system->scenes;
    wp_s32 scene_count = system->scene_count;

    for( i = 0; i < scene_count; ++i )
    {
        scene = scenes[i];
        if( !scene )
        {
            continue;
        }

        wp_renderer *renderer = system->renderer;
        wp_graphics_scene_render( scene, renderer );
    }
}

void wp_graphics_system_render_frame( wp_graphics_system *system, wp_f32 delta_time )
{
    if( !system )
    {
        return;
    }

    wp_graphics_system_update( system );

    if( system->frame_func )
    {
        system->frame_func( system, delta_time, system->frame_user_data );
    }
    else
    {
        wp_graphics_system_render( system );
    }
}

void wp_graphics_system_set_frame_func( wp_graphics_system *system,
                                        wp_graphics_system_frame_func frame_func, void *user_data )
{
    if( !system )
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
    if( !system )
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
