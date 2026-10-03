/**
 * @file workphone_game_scene.c
 * @brief Implementation of the C game scene API.
 */

#include "workphone_game_scene.h"

#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static void wp_game_scene_grow( wp_game_scene *scene )
{
    wp_u32 new_cap = scene->actor_capacity + WP_SCENE_ACTOR_GROW_SIZE;
    wp_game_scene_reserve_actors( scene, new_cap );
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

void wp_game_scene_init( wp_game_scene *scene )
{
    memset( scene, 0, sizeof( *scene ) );
    scene->name[0] = '\0';
    scene->actors = NULL;
    scene->num_actors = 0;
    scene->actor_capacity = 0;
    scene->state = WP_SCENE_STATE_NONE;
    scene->loading_state = WP_SCENE_LOADING_NONE;
    scene->user_data = NULL;
}

void wp_game_scene_init_with_capacity( wp_game_scene *scene, wp_u32 capacity )
{
    wp_game_scene_init( scene );
    wp_game_scene_reserve_actors( scene, capacity );
}

void wp_game_scene_destroy( wp_game_scene *scene )
{
    wp_game_scene_clear( scene );
    free( scene->actors );
    scene->actors = NULL;
    scene->actor_capacity = 0;
    scene->state = WP_SCENE_STATE_NONE;
    scene->loading_state = WP_SCENE_LOADING_UNLOADED;
    scene->user_data = NULL;
}

/* =========================================================================
 * Name
 * ====================================================================== */

void wp_game_scene_set_name( wp_game_scene *scene, const wp_c8 *name )
{
    if( name )
    {
        strncpy( scene->name, name, WP_SCENE_MAX_NAME - 1 );
        scene->name[WP_SCENE_MAX_NAME - 1] = '\0';
    }
    else
    {
        scene->name[0] = '\0';
    }
}

const wp_c8 *wp_game_scene_get_name( const wp_game_scene *scene )
{
    return scene->name;
}

/* =========================================================================
 * State
 * ====================================================================== */

enum wp_scene_state wp_game_scene_get_state( const wp_game_scene *scene )
{
    return scene->state;
}

void wp_game_scene_set_state( wp_game_scene *scene, enum wp_scene_state state )
{
    scene->state = state;
}

enum wp_scene_loading_state wp_game_scene_get_loading_state( const wp_game_scene *scene )
{
    return scene->loading_state;
}

void wp_game_scene_set_loading_state( wp_game_scene *scene, enum wp_scene_loading_state state )
{
    scene->loading_state = state;
}

/* =========================================================================
 * Capacity
 * ====================================================================== */

wp_u32 wp_game_scene_get_actor_capacity( const wp_game_scene *scene )
{
    return scene->actor_capacity;
}

void wp_game_scene_reserve_actors( wp_game_scene *scene, wp_u32 capacity )
{
    wp_game_actor **new_actors;
    wp_u32 old_cap;

    if( capacity <= scene->actor_capacity )
        return;

    old_cap = scene->actor_capacity;

    new_actors = (wp_game_actor **)realloc( scene->actors, capacity * sizeof( wp_game_actor * ) );
    if( !new_actors )
        return;

    memset( new_actors + old_cap, 0, ( capacity - old_cap ) * sizeof( wp_game_actor * ) );
    scene->actors = new_actors;
    scene->actor_capacity = capacity;
}

/* =========================================================================
 * Actor management
 * ====================================================================== */

wp_s32 wp_game_scene_add_actor( wp_game_scene *scene, wp_game_actor *actor )
{
    wp_u32 i;

    if( !actor )
        return 0;

    for( i = 0; i < scene->num_actors; ++i )
    {
        if( scene->actors[i] == actor )
            return 0;
    }

    if( scene->num_actors >= scene->actor_capacity )
        wp_game_scene_grow( scene );

    if( scene->num_actors >= scene->actor_capacity )
        return 0;

    scene->actors[scene->num_actors++] = actor;
    wp_game_actor_set_scene( actor, scene );
    return 1;
}

wp_s32 wp_game_scene_remove_actor( wp_game_scene *scene, wp_game_actor *actor )
{
    wp_u32 i;

    if( !actor )
        return 0;

    for( i = 0; i < scene->num_actors; ++i )
    {
        if( scene->actors[i] == actor )
        {
            wp_game_actor_set_scene( actor, NULL );
            scene->num_actors--;
            for( ; i < scene->num_actors; ++i )
                scene->actors[i] = scene->actors[i + 1];
            scene->actors[scene->num_actors] = NULL;
            return 1;
        }
    }
    return 0;
}

void wp_game_scene_remove_all_actors( wp_game_scene *scene )
{
    wp_u32 i;
    for( i = 0; i < scene->num_actors; ++i )
    {
        if( scene->actors[i] )
            wp_game_actor_set_scene( scene->actors[i], NULL );
        scene->actors[i] = NULL;
    }
    scene->num_actors = 0;
}

wp_game_actor *wp_game_scene_get_actor( const wp_game_scene *scene, wp_u32 index )
{
    if( index >= scene->num_actors )
        return NULL;
    return scene->actors[index];
}

wp_u32 wp_game_scene_get_num_actors( const wp_game_scene *scene )
{
    return scene->num_actors;
}

wp_game_actor *wp_game_scene_find_actor_by_name( const wp_game_scene *scene, const wp_c8 *name )
{
    wp_u32 i;
    if( !name )
        return NULL;
    for( i = 0; i < scene->num_actors; ++i )
    {
        if( scene->actors[i] && strcmp( wp_game_actor_get_name( scene->actors[i] ), name ) == 0 )
        {
            return scene->actors[i];
        }
    }
    return NULL;
}

wp_game_actor *wp_game_scene_find_actor_by_id( const wp_game_scene *scene, wp_s32 id )
{
    wp_u32 i;
    for( i = 0; i < scene->num_actors; ++i )
    {
        if( scene->actors[i] && scene->actors[i]->id == id )
            return scene->actors[i];
    }
    return NULL;
}

/* =========================================================================
 * Clear
 * ====================================================================== */

void wp_game_scene_clear( wp_game_scene *scene )
{
    wp_game_scene_remove_all_actors( scene );
}

/* =========================================================================
 * User data
 * ====================================================================== */

void *wp_game_scene_get_user_data( const wp_game_scene *scene )
{
    return scene->user_data;
}

void wp_game_scene_set_user_data( wp_game_scene *scene, void *user_data )
{
    scene->user_data = user_data;
}
