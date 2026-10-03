/**
 * @file workphone_fsm.c
 * @brief Implementation of the C finite state machine API.
 */

#include "workphone_fsm.h"

#include <string.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static enum wp_fsm_return_type wp_fsm_notify_listeners( wp_fsm *fsm, wp_u32 state,
                                                        enum wp_fsm_event event )
{
    wp_u32 i;
    for( i = 0; i < fsm->num_listeners; ++i )
    {
        wp_fsm_listener *l = &fsm->listeners[i];
        if( l->callback )
        {
            enum wp_fsm_return_type ret = l->callback( l->user_data, state, event );
            if( ret != WP_FSM_RETURN_OK )
                return ret;
        }
    }
    return WP_FSM_RETURN_OK;
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

void wp_fsm_init( wp_fsm *fsm )
{
    memset( fsm, 0, sizeof( *fsm ) );
    fsm->previous_state = 0;
    fsm->current_state = 0;
    fsm->new_state = 0;
    fsm->flags = WP_FSM_FLAG_AUTO_CHANGE_STATE | WP_FSM_FLAG_ALLOW_STATE_CHANGE |
                 WP_FSM_FLAG_STATE_CHANGE_COMPLETE | WP_FSM_FLAG_READY;
    fsm->state_time = 0.0;
    fsm->state_time_elapsed = 0.0;
    fsm->state_ticks = 0;
    fsm->priority = 0;
    fsm->num_listeners = 0;
    fsm->manager = NULL;
}

void wp_fsm_destroy( wp_fsm *fsm )
{
    fsm->num_listeners = 0;
    fsm->manager = NULL;
    fsm->flags = 0;
}

/* =========================================================================
 * Update
 * ====================================================================== */

void wp_fsm_update( wp_fsm *fsm, wp_f64 dt )
{
    fsm->state_time_elapsed += dt;

    if( fsm->flags & WP_FSM_FLAG_AUTO_CHANGE_STATE )
    {
        wp_fsm_update_state( fsm );
    }
}

void wp_fsm_update_state( wp_fsm *fsm )
{
    if( fsm->current_state == fsm->new_state )
        return;

    if( !( fsm->flags & WP_FSM_FLAG_ALLOW_STATE_CHANGE ) )
        return;

    if( fsm->num_listeners > 0 )
    {
        wp_fsm_notify_listeners( fsm, (wp_u32)fsm->current_state, WP_FSM_EVENT_LEAVE );

        fsm->previous_state = fsm->current_state;
        fsm->current_state = fsm->new_state;

        wp_fsm_notify_listeners( fsm, (wp_u32)fsm->current_state, WP_FSM_EVENT_ENTER );
        wp_fsm_notify_listeners( fsm, (wp_u32)fsm->current_state, WP_FSM_EVENT_COMPLETE );
    }
    else
    {
        fsm->previous_state = fsm->current_state;
        fsm->current_state = fsm->new_state;
    }

    fsm->flags &= ~WP_FSM_FLAG_PENDING;
    fsm->flags |= WP_FSM_FLAG_STATE_CHANGE_COMPLETE;
    fsm->state_time_elapsed = 0.0;
    fsm->state_ticks = 0;
}

/* =========================================================================
 * Flags
 * ====================================================================== */

wp_u32 wp_fsm_get_flags( const wp_fsm *fsm )
{
    return fsm->flags;
}

void wp_fsm_set_flags( wp_fsm *fsm, wp_u32 flags )
{
    fsm->flags = flags;
}

wp_s32 wp_fsm_get_flag( const wp_fsm *fsm, wp_u32 flag )
{
    return ( fsm->flags & flag ) != 0;
}

void wp_fsm_set_flag( wp_fsm *fsm, wp_u32 flag, wp_s32 value )
{
    if( value )
        fsm->flags |= flag;
    else
        fsm->flags &= ~flag;
}

/* =========================================================================
 * State access
 * ====================================================================== */

wp_u8 wp_fsm_get_previous_state( const wp_fsm *fsm )
{
    return fsm->previous_state;
}

wp_u8 wp_fsm_get_current_state( const wp_fsm *fsm )
{
    return fsm->current_state;
}

wp_u8 wp_fsm_get_new_state( const wp_fsm *fsm )
{
    return fsm->new_state;
}

void wp_fsm_set_new_state( wp_fsm *fsm, wp_s32 state, wp_s32 change_now )
{
    if( !( fsm->flags & WP_FSM_FLAG_ALLOW_STATE_CHANGE ) )
        return;

    fsm->new_state = (wp_u8)state;
    fsm->flags |= WP_FSM_FLAG_PENDING;
    fsm->flags &= ~WP_FSM_FLAG_STATE_CHANGE_COMPLETE;

    wp_fsm_notify_listeners( fsm, (wp_u32)state, WP_FSM_EVENT_NEW_STATE );

    if( change_now )
    {
        wp_fsm_update_state( fsm );
    }
}

void wp_fsm_state_override( wp_fsm *fsm, wp_s32 state )
{
    fsm->previous_state = fsm->current_state;
    fsm->current_state = (wp_u8)state;
    fsm->new_state = (wp_u8)state;
    fsm->flags &= ~WP_FSM_FLAG_PENDING;
    fsm->flags |= WP_FSM_FLAG_STATE_CHANGE_COMPLETE;
    fsm->state_time_elapsed = 0.0;
    fsm->state_ticks = 0;
}

wp_s32 wp_fsm_is_pending( const wp_fsm *fsm )
{
    return ( fsm->flags & WP_FSM_FLAG_PENDING ) != 0;
}

/* =========================================================================
 * State timing
 * ====================================================================== */

wp_f64 wp_fsm_get_state_time( const wp_fsm *fsm )
{
    return fsm->state_time;
}

void wp_fsm_set_state_time( wp_fsm *fsm, wp_f64 state_time )
{
    fsm->state_time = state_time;
}

wp_f64 wp_fsm_get_state_time_elapsed( const wp_fsm *fsm )
{
    return fsm->state_time_elapsed;
}

/* =========================================================================
 * Ticks
 * ====================================================================== */

wp_s32 wp_fsm_get_state_ticks( const wp_fsm *fsm )
{
    return fsm->state_ticks;
}

void wp_fsm_set_state_ticks( wp_fsm *fsm, wp_s32 ticks )
{
    fsm->state_ticks = ticks;
}

/* =========================================================================
 * Auto change
 * ====================================================================== */

wp_s32 wp_fsm_get_auto_change_state( const wp_fsm *fsm )
{
    return ( fsm->flags & WP_FSM_FLAG_AUTO_CHANGE_STATE ) != 0;
}

void wp_fsm_set_auto_change_state( wp_fsm *fsm, wp_s32 auto_change )
{
    if( auto_change )
        fsm->flags |= WP_FSM_FLAG_AUTO_CHANGE_STATE;
    else
        fsm->flags &= ~WP_FSM_FLAG_AUTO_CHANGE_STATE;
}

/* =========================================================================
 * Allow state change
 * ====================================================================== */

wp_s32 wp_fsm_get_allow_state_change( const wp_fsm *fsm )
{
    return ( fsm->flags & WP_FSM_FLAG_ALLOW_STATE_CHANGE ) != 0;
}

void wp_fsm_set_allow_state_change( wp_fsm *fsm, wp_s32 allow )
{
    if( allow )
        fsm->flags |= WP_FSM_FLAG_ALLOW_STATE_CHANGE;
    else
        fsm->flags &= ~WP_FSM_FLAG_ALLOW_STATE_CHANGE;
}

/* =========================================================================
 * State change complete
 * ====================================================================== */

wp_s32 wp_fsm_is_state_change_complete( const wp_fsm *fsm )
{
    return ( fsm->flags & WP_FSM_FLAG_STATE_CHANGE_COMPLETE ) != 0;
}

void wp_fsm_set_state_change_complete( wp_fsm *fsm, wp_s32 complete )
{
    if( complete )
        fsm->flags |= WP_FSM_FLAG_STATE_CHANGE_COMPLETE;
    else
        fsm->flags &= ~WP_FSM_FLAG_STATE_CHANGE_COMPLETE;
}

/* =========================================================================
 * Priority
 * ====================================================================== */

wp_s32 wp_fsm_get_priority( const wp_fsm *fsm )
{
    return fsm->priority;
}

void wp_fsm_set_priority( wp_fsm *fsm, wp_s32 priority )
{
    fsm->priority = priority;
}

/* =========================================================================
 * Listeners
 * ====================================================================== */

wp_s32 wp_fsm_add_listener( wp_fsm *fsm, wp_fsm_listener_fn callback, void *user_data )
{
    if( fsm->num_listeners >= WP_FSM_MAX_LISTENERS )
        return -1;

    fsm->listeners[fsm->num_listeners].callback = callback;
    fsm->listeners[fsm->num_listeners].user_data = user_data;
    ++fsm->num_listeners;
    return 0;
}

wp_s32 wp_fsm_remove_listener( wp_fsm *fsm, wp_fsm_listener_fn callback, void *user_data )
{
    wp_u32 i;
    for( i = 0; i < fsm->num_listeners; ++i )
    {
        if( fsm->listeners[i].callback == callback && fsm->listeners[i].user_data == user_data )
        {
            wp_u32 last = fsm->num_listeners - 1;
            if( i != last )
                fsm->listeners[i] = fsm->listeners[last];

            fsm->listeners[last].callback = NULL;
            fsm->listeners[last].user_data = NULL;
            --fsm->num_listeners;
            return 0;
        }
    }
    return -1;
}

void wp_fsm_remove_all_listeners( wp_fsm *fsm )
{
    wp_u32 i;
    for( i = 0; i < fsm->num_listeners; ++i )
    {
        fsm->listeners[i].callback = NULL;
        fsm->listeners[i].user_data = NULL;
    }
    fsm->num_listeners = 0;
}

wp_u32 wp_fsm_get_num_listeners( const wp_fsm *fsm )
{
    return fsm->num_listeners;
}

/* =========================================================================
 * Manager
 * ====================================================================== */

struct wp_fsm_manager *wp_fsm_get_manager( const wp_fsm *fsm )
{
    return fsm->manager;
}

void wp_fsm_set_manager( wp_fsm *fsm, struct wp_fsm_manager *manager )
{
    fsm->manager = manager;
}
