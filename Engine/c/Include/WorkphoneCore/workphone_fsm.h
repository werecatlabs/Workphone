/**
 * @file workphone_fsm.h
 * @brief C API for a lightweight finite state machine (FSM).
 */

#ifndef WORKPHONE_FSM_H
#define WORKPHONE_FSM_H

#include "workphone_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct wp_fsm_manager;

/**
 * @brief Event IDs for FSM state transitions.
 */
enum wp_fsm_event
{
    WP_FSM_EVENT_CHANGE = 0,
    WP_FSM_EVENT_ENTER,
    WP_FSM_EVENT_LEAVE,
    WP_FSM_EVENT_PENDING,
    WP_FSM_EVENT_COMPLETE,
    WP_FSM_EVENT_NEW_STATE,
    WP_FSM_EVENT_WAIT_FOR_CHANGE,
    WP_FSM_EVENT_COUNT
};

/**
 * @brief Return types for FSM listener callbacks.
 */
enum wp_fsm_return_type
{
    WP_FSM_RETURN_FAILED = -1,
    WP_FSM_RETURN_OK = 0,
    WP_FSM_RETURN_ACCEPT,
    WP_FSM_RETURN_CANCEL,
    WP_FSM_RETURN_IGNORE,
    WP_FSM_RETURN_WAIT_FOR_CHANGE,
    WP_FSM_RETURN_NOT_LOADED,
    WP_FSM_RETURN_NOT_HANDLED,
    WP_FSM_RETURN_COUNT
};

/**
 * @brief FSM flag bits.
 */
enum
{
    WP_FSM_FLAG_STATE_CHANGE_COMPLETE = ( 1u << 0 ),
    WP_FSM_FLAG_AUTO_CHANGE_STATE = ( 1u << 1 ),
    WP_FSM_FLAG_PENDING = ( 1u << 2 ),
    WP_FSM_FLAG_READY = ( 1u << 3 ),
    WP_FSM_FLAG_LOCKED = ( 1u << 4 ),
    WP_FSM_FLAG_ALLOW_STATE_CHANGE = ( 1u << 5 )
};

#ifndef WP_FSM_MAX_LISTENERS
#    define WP_FSM_MAX_LISTENERS 8
#endif

/**
 * @brief Callback invoked when an FSM event occurs.
 *
 * @param user_data Opaque pointer supplied when the listener was added.
 * @param state     Current state id at the time of the event.
 * @param event     The event that occurred.
 * @return A wp_fsm_return_type value indicating how the event was handled.
 */
typedef enum wp_fsm_return_type ( *wp_fsm_listener_fn )( void *user_data, wp_u32 state,
                                                         enum wp_fsm_event event );

/**
 * @brief A single listener registration (callback + user data).
 */
typedef struct
{
    wp_fsm_listener_fn callback;
    void *user_data;
} wp_fsm_listener;

/**
 * @brief Lightweight finite state machine.
 */
typedef struct wp_fsm
{
    wp_u8 previous_state;
    wp_u8 current_state;
    wp_u8 new_state;

    wp_u32 flags;

    wp_f64 state_time;
    wp_f64 state_time_elapsed;

    wp_s32 state_ticks;
    wp_s32 priority;

    wp_fsm_listener listeners[WP_FSM_MAX_LISTENERS];
    wp_u32 num_listeners;

    struct wp_fsm_manager *manager;
} wp_fsm;

/* ---- Lifecycle -------------------------------------------------------- */

void wp_fsm_init( wp_fsm *fsm );
void wp_fsm_destroy( wp_fsm *fsm );

/* ---- Update ----------------------------------------------------------- */

void wp_fsm_update( wp_fsm *fsm, wp_f64 dt );
void wp_fsm_update_state( wp_fsm *fsm );

/* ---- Flags ------------------------------------------------------------ */

wp_u32 wp_fsm_get_flags( const wp_fsm *fsm );
void wp_fsm_set_flags( wp_fsm *fsm, wp_u32 flags );
wp_s32 wp_fsm_get_flag( const wp_fsm *fsm, wp_u32 flag );
void wp_fsm_set_flag( wp_fsm *fsm, wp_u32 flag, wp_s32 value );

/* ---- State access ----------------------------------------------------- */

wp_u8 wp_fsm_get_previous_state( const wp_fsm *fsm );
wp_u8 wp_fsm_get_current_state( const wp_fsm *fsm );
wp_u8 wp_fsm_get_new_state( const wp_fsm *fsm );

void wp_fsm_set_new_state( wp_fsm *fsm, wp_s32 state, wp_s32 change_now );
void wp_fsm_state_override( wp_fsm *fsm, wp_s32 state );

wp_s32 wp_fsm_is_pending( const wp_fsm *fsm );

/* ---- State timing ----------------------------------------------------- */

wp_f64 wp_fsm_get_state_time( const wp_fsm *fsm );
void wp_fsm_set_state_time( wp_fsm *fsm, wp_f64 state_time );

wp_f64 wp_fsm_get_state_time_elapsed( const wp_fsm *fsm );

/* ---- Ticks ------------------------------------------------------------ */

wp_s32 wp_fsm_get_state_ticks( const wp_fsm *fsm );
void wp_fsm_set_state_ticks( wp_fsm *fsm, wp_s32 ticks );

/* ---- Auto change ------------------------------------------------------ */

wp_s32 wp_fsm_get_auto_change_state( const wp_fsm *fsm );
void wp_fsm_set_auto_change_state( wp_fsm *fsm, wp_s32 auto_change );

/* ---- Allow state change ----------------------------------------------- */

wp_s32 wp_fsm_get_allow_state_change( const wp_fsm *fsm );
void wp_fsm_set_allow_state_change( wp_fsm *fsm, wp_s32 allow );

/* ---- State change complete -------------------------------------------- */

wp_s32 wp_fsm_is_state_change_complete( const wp_fsm *fsm );
void wp_fsm_set_state_change_complete( wp_fsm *fsm, wp_s32 complete );

/* ---- Priority --------------------------------------------------------- */

wp_s32 wp_fsm_get_priority( const wp_fsm *fsm );
void wp_fsm_set_priority( wp_fsm *fsm, wp_s32 priority );

/* ---- Listeners -------------------------------------------------------- */

wp_s32 wp_fsm_add_listener( wp_fsm *fsm, wp_fsm_listener_fn callback, void *user_data );
wp_s32 wp_fsm_remove_listener( wp_fsm *fsm, wp_fsm_listener_fn callback, void *user_data );
void wp_fsm_remove_all_listeners( wp_fsm *fsm );
wp_u32 wp_fsm_get_num_listeners( const wp_fsm *fsm );

/* ---- Manager ---------------------------------------------------------- */

struct wp_fsm_manager *wp_fsm_get_manager( const wp_fsm *fsm );
void wp_fsm_set_manager( wp_fsm *fsm, struct wp_fsm_manager *manager );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_FSM_H */
