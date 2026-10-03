/**
 * @file workphone_fsmmgr.c
 * @brief Implementation of the C finite state machine manager API.
 */

#include "workphone_fsmmgr.h"

#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static void wp_fsm_mgr_grow( wp_fsm_manager *mgr )
{
    wp_u32 new_cap = mgr->capacity + mgr->grow_size;
    wp_fsm_mgr_reserve( mgr, new_cap );
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

void wp_fsm_mgr_init( wp_fsm_manager *mgr )
{
    memset( mgr, 0, sizeof( *mgr ) );
    mgr->fsms = NULL;
    mgr->active = NULL;
    mgr->num_fsms = 0;
    mgr->capacity = 0;
    mgr->grow_size = WP_FSM_MGR_GROW_SIZE;
}

void wp_fsm_mgr_init_with_capacity( wp_fsm_manager *mgr, wp_u32 capacity )
{
    wp_fsm_mgr_init( mgr );
    wp_fsm_mgr_reserve( mgr, capacity );
}

void wp_fsm_mgr_destroy( wp_fsm_manager *mgr )
{
    wp_u32 i;
    for( i = 0; i < mgr->capacity; ++i )
    {
        if( mgr->active[i] )
        {
            wp_fsm_destroy( &mgr->fsms[i] );
            mgr->active[i] = 0;
        }
    }
    free( mgr->fsms );
    free( mgr->active );
    mgr->fsms = NULL;
    mgr->active = NULL;
    mgr->num_fsms = 0;
    mgr->capacity = 0;
}

/* =========================================================================
 * Capacity
 * ====================================================================== */

wp_u32 wp_fsm_mgr_get_capacity( const wp_fsm_manager *mgr )
{
    return mgr->capacity;
}

void wp_fsm_mgr_reserve( wp_fsm_manager *mgr, wp_u32 capacity )
{
    wp_fsm *new_fsms;
    wp_u32 *new_active;
    wp_u32 old_cap;

    if( capacity <= mgr->capacity )
        return;

    old_cap = mgr->capacity;

    new_fsms = (wp_fsm *)realloc( mgr->fsms, capacity * sizeof( wp_fsm ) );
    if( !new_fsms )
        return;
    memset( new_fsms + old_cap, 0, ( capacity - old_cap ) * sizeof( wp_fsm ) );
    mgr->fsms = new_fsms;

    new_active = (wp_u32 *)realloc( mgr->active, capacity * sizeof( wp_u32 ) );
    if( !new_active )
        return;
    memset( new_active + old_cap, 0, ( capacity - old_cap ) * sizeof( wp_u32 ) );
    mgr->active = new_active;

    mgr->capacity = capacity;
}

wp_u32 wp_fsm_mgr_get_grow_size( const wp_fsm_manager *mgr )
{
    return mgr->grow_size;
}

void wp_fsm_mgr_set_grow_size( wp_fsm_manager *mgr, wp_u32 grow_size )
{
    mgr->grow_size = grow_size;
}

/* =========================================================================
 * FSM creation / destruction
 * ====================================================================== */

wp_s32 wp_fsm_mgr_create_fsm( wp_fsm_manager *mgr )
{
    wp_u32 i;

    /* Find a free slot. */
    for( i = 0; i < mgr->capacity; ++i )
    {
        if( !mgr->active[i] )
        {
            wp_fsm_init( &mgr->fsms[i] );
            wp_fsm_set_manager( &mgr->fsms[i], mgr );
            mgr->active[i] = 1;
            ++mgr->num_fsms;
            return (wp_s32)i;
        }
    }

    /* No free slot -- grow and use the first new slot. */
    wp_fsm_mgr_grow( mgr );

    if( i < mgr->capacity )
    {
        wp_fsm_init( &mgr->fsms[i] );
        wp_fsm_set_manager( &mgr->fsms[i], mgr );
        mgr->active[i] = 1;
        ++mgr->num_fsms;
        return (wp_s32)i;
    }

    return -1;
}

void wp_fsm_mgr_destroy_fsm( wp_fsm_manager *mgr, wp_u32 id )
{
    if( id >= mgr->capacity )
        return;

    if( !mgr->active[id] )
        return;

    wp_fsm_destroy( &mgr->fsms[id] );
    mgr->active[id] = 0;
    --mgr->num_fsms;
}

/* =========================================================================
 * Access
 * ====================================================================== */

wp_fsm *wp_fsm_mgr_get_fsm( const wp_fsm_manager *mgr, wp_u32 id )
{
    if( id >= mgr->capacity )
        return NULL;

    if( !mgr->active[id] )
        return NULL;

    return &mgr->fsms[id];
}

wp_u32 wp_fsm_mgr_get_num_fsms( const wp_fsm_manager *mgr )
{
    return mgr->num_fsms;
}

wp_s32 wp_fsm_mgr_is_active( const wp_fsm_manager *mgr, wp_u32 id )
{
    if( id >= mgr->capacity )
        return 0;

    return mgr->active[id] != 0;
}

/* =========================================================================
 * Update
 * ====================================================================== */

void wp_fsm_mgr_update( wp_fsm_manager *mgr, wp_f64 dt )
{
    wp_u32 i;
    for( i = 0; i < mgr->capacity; ++i )
    {
        if( mgr->active[i] )
        {
            wp_fsm_update( &mgr->fsms[i], dt );
        }
    }
}

/* =========================================================================
 * Bulk state change
 * ====================================================================== */

void wp_fsm_mgr_change_states( wp_fsm_manager *mgr )
{
    wp_u32 i;
    for( i = 0; i < mgr->capacity; ++i )
    {
        if( mgr->active[i] )
        {
            wp_fsm_update_state( &mgr->fsms[i] );
        }
    }
}

void wp_fsm_mgr_change_state( wp_fsm_manager *mgr, wp_u32 id )
{
    if( id >= mgr->capacity )
        return;

    if( !mgr->active[id] )
        return;

    wp_fsm_update_state( &mgr->fsms[id] );
}
