/**
 * @file workphone_game_scenemgr.c
 * @brief Implementation of the C game scene manager API.
 */

#include "workphone_game_scenemgr.h"

#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static void wp_game_scenemgr_grow_actors( wp_game_scenemgr *mgr )
{
    wp_u32 new_cap = mgr->actor_capacity + WP_SCENEMGR_ACTOR_GROW_SIZE;
    wp_game_scenemgr_reserve_actors( mgr, new_cap );
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

void wp_game_scenemgr_init( wp_game_scenemgr *mgr )
{
    memset( mgr, 0, sizeof( *mgr ) );
    mgr->actors = NULL;
    mgr->actor_active = NULL;
    mgr->num_actors = 0;
    mgr->actor_capacity = 0;
    mgr->current_scene = NULL;
    mgr->user_data = NULL;

    wp_fsm_mgr_init( &mgr->fsm_mgr );
}

void wp_game_scenemgr_init_with_capacity( wp_game_scenemgr *mgr, wp_u32 actor_capacity )
{
    wp_game_scenemgr_init( mgr );
    wp_game_scenemgr_reserve_actors( mgr, actor_capacity );
}

void wp_game_scenemgr_destroy( wp_game_scenemgr *mgr )
{
    wp_game_scenemgr_destroy_actors( mgr );

    free( mgr->actors );
    free( mgr->actor_active );
    mgr->actors = NULL;
    mgr->actor_active = NULL;
    mgr->actor_capacity = 0;

    if( mgr->current_scene )
    {
        wp_game_scene_destroy( mgr->current_scene );
        mgr->current_scene = NULL;
    }

    wp_fsm_mgr_destroy( &mgr->fsm_mgr );

    mgr->user_data = NULL;
}

/* =========================================================================
 * Actor pool
 * ====================================================================== */

wp_u32 wp_game_scenemgr_get_actor_capacity( const wp_game_scenemgr *mgr )
{
    return mgr->actor_capacity;
}

void wp_game_scenemgr_reserve_actors( wp_game_scenemgr *mgr, wp_u32 capacity )
{
    wp_game_actor **new_actors;
    wp_u32 *new_active;
    wp_u32 old_cap;

    if( capacity <= mgr->actor_capacity )
        return;

    old_cap = mgr->actor_capacity;

    new_actors = (wp_game_actor **)realloc( mgr->actors, capacity * sizeof( wp_game_actor * ) );
    if( !new_actors )
        return;
    memset( new_actors + old_cap, 0, ( capacity - old_cap ) * sizeof( wp_game_actor * ) );
    mgr->actors = new_actors;

    new_active = (wp_u32 *)realloc( mgr->actor_active, capacity * sizeof( wp_u32 ) );
    if( !new_active )
        return;
    memset( new_active + old_cap, 0, ( capacity - old_cap ) * sizeof( wp_u32 ) );
    mgr->actor_active = new_active;

    mgr->actor_capacity = capacity;
}

/* =========================================================================
 * Actor creation / destruction
 * ====================================================================== */

wp_s32 wp_game_scenemgr_create_actor( wp_game_scenemgr *mgr )
{
    wp_u32 i;
    wp_game_actor *actor;

    /* Find a free slot. */
    for( i = 0; i < mgr->actor_capacity; ++i )
    {
        if( !mgr->actor_active[i] )
            break;
    }

    /* Grow if no free slot was found. */
    if( i >= mgr->actor_capacity )
    {
        wp_game_scenemgr_grow_actors( mgr );
        if( i >= mgr->actor_capacity )
            return -1;
    }

    actor = (wp_game_actor *)malloc( sizeof( wp_game_actor ) );
    if( !actor )
        return -1;

    wp_game_actor_init_with_id( actor, (wp_s32)i );

    mgr->actors[i] = actor;
    mgr->actor_active[i] = 1;
    ++mgr->num_actors;

    return (wp_s32)i;
}

void wp_game_scenemgr_destroy_actor( wp_game_scenemgr *mgr, wp_u32 id )
{
    wp_game_actor *actor;

    if( id >= mgr->actor_capacity )
        return;

    if( !mgr->actor_active[id] )
        return;

    actor = mgr->actors[id];

    /* Remove from the current scene if present. */
    if( actor->scene )
        wp_game_scene_remove_actor( actor->scene, actor );

    /* Detach from parent. */
    if( actor->parent )
        wp_game_actor_remove_child( actor->parent, actor );

    /* Recursively destroy children. */
    while( actor->num_children > 0 )
    {
        wp_game_actor *child = actor->children[0];
        if( child )
            wp_game_scenemgr_destroy_actor( mgr, (wp_u32)child->id );
        else
            wp_game_actor_remove_child( actor, child );
    }

    wp_game_actor_destroy( actor );
    free( actor );

    mgr->actors[id] = NULL;
    mgr->actor_active[id] = 0;
    --mgr->num_actors;
}

void wp_game_scenemgr_destroy_actors( wp_game_scenemgr *mgr )
{
    wp_u32 i;
    for( i = 0; i < mgr->actor_capacity; ++i )
    {
        if( mgr->actor_active[i] && mgr->actors[i] )
        {
            wp_game_actor_destroy( mgr->actors[i] );
            free( mgr->actors[i] );
            mgr->actors[i] = NULL;
            mgr->actor_active[i] = 0;
        }
    }
    mgr->num_actors = 0;
}

/* =========================================================================
 * Actor access
 * ====================================================================== */

wp_game_actor *wp_game_scenemgr_get_actor( const wp_game_scenemgr *mgr, wp_u32 id )
{
    if( id >= mgr->actor_capacity )
        return NULL;

    if( !mgr->actor_active[id] )
        return NULL;

    return mgr->actors[id];
}

wp_game_actor *wp_game_scenemgr_find_actor_by_name( const wp_game_scenemgr *mgr, const wp_c8 *name )
{
    wp_u32 i;
    if( !name )
        return NULL;

    for( i = 0; i < mgr->actor_capacity; ++i )
    {
        if( mgr->actor_active[i] && mgr->actors[i] )
        {
            if( strcmp( wp_game_actor_get_name( mgr->actors[i] ), name ) == 0 )
                return mgr->actors[i];
        }
    }
    return NULL;
}

wp_u32 wp_game_scenemgr_get_num_actors( const wp_game_scenemgr *mgr )
{
    return mgr->num_actors;
}

/* =========================================================================
 * Current scene
 * ====================================================================== */

wp_game_scene *wp_game_scenemgr_get_current_scene( const wp_game_scenemgr *mgr )
{
    return mgr->current_scene;
}

void wp_game_scenemgr_set_current_scene( wp_game_scenemgr *mgr, wp_game_scene *scene )
{
    mgr->current_scene = scene;
}

/* =========================================================================
 * FSM manager
 * ====================================================================== */

wp_fsm_manager *wp_game_scenemgr_get_fsm_manager( wp_game_scenemgr *mgr )
{
    return &mgr->fsm_mgr;
}

/* =========================================================================
 * Update
 * ====================================================================== */

void wp_game_scenemgr_update( wp_game_scenemgr *mgr, wp_f64 dt )
{
    wp_fsm_mgr_update( &mgr->fsm_mgr, dt );
}

/* =========================================================================
 * Play / Edit / Stop
 * ====================================================================== */

void wp_game_scenemgr_play( wp_game_scenemgr *mgr )
{
    if( mgr->current_scene )
        wp_game_scene_set_state( mgr->current_scene, WP_SCENE_STATE_PLAY );
}

void wp_game_scenemgr_edit( wp_game_scenemgr *mgr )
{
    if( mgr->current_scene )
        wp_game_scene_set_state( mgr->current_scene, WP_SCENE_STATE_EDIT );
}

void wp_game_scenemgr_stop( wp_game_scenemgr *mgr )
{
    if( mgr->current_scene )
        wp_game_scene_set_state( mgr->current_scene, WP_SCENE_STATE_NONE );
}

/* =========================================================================
 * Clear
 * ====================================================================== */

void wp_game_scenemgr_clear( wp_game_scenemgr *mgr )
{
    if( mgr->current_scene )
        wp_game_scene_clear( mgr->current_scene );
}

/* =========================================================================
 * User data
 * ====================================================================== */

void *wp_game_scenemgr_get_user_data( const wp_game_scenemgr *mgr )
{
    return mgr->user_data;
}

void wp_game_scenemgr_set_user_data( wp_game_scenemgr *mgr, void *user_data )
{
    mgr->user_data = user_data;
}
