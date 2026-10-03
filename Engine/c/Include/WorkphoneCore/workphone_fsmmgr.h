/**
 * @file workphone_fsmmgr.h
 * @brief C API for the finite state machine manager.
 */

#ifndef WORKPHONE_FSMMGR_H
#define WORKPHONE_FSMMGR_H

#include "workphone_fsm.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WP_FSM_MGR_GROW_SIZE
#    define WP_FSM_MGR_GROW_SIZE 12
#endif

/**
 * @brief Manager that owns and coordinates a collection of wp_fsm instances.
 *
 * FSMs are stored in a contiguous array addressed by index.  The manager
 * handles creation, destruction and bulk update of all managed FSMs.
 */
typedef struct wp_fsm_manager
{
    wp_fsm *fsms;
    wp_u32 *active;
    wp_u32 num_fsms;
    wp_u32 capacity;
    wp_u32 grow_size;
} wp_fsm_manager;

/* ---- Lifecycle -------------------------------------------------------- */

void wp_fsm_mgr_init( wp_fsm_manager *mgr );
void wp_fsm_mgr_init_with_capacity( wp_fsm_manager *mgr, wp_u32 capacity );
void wp_fsm_mgr_destroy( wp_fsm_manager *mgr );

/* ---- Capacity --------------------------------------------------------- */

wp_u32 wp_fsm_mgr_get_capacity( const wp_fsm_manager *mgr );
void wp_fsm_mgr_reserve( wp_fsm_manager *mgr, wp_u32 capacity );

wp_u32 wp_fsm_mgr_get_grow_size( const wp_fsm_manager *mgr );
void wp_fsm_mgr_set_grow_size( wp_fsm_manager *mgr, wp_u32 grow_size );

/* ---- FSM creation / destruction --------------------------------------- */

wp_s32 wp_fsm_mgr_create_fsm( wp_fsm_manager *mgr );
void wp_fsm_mgr_destroy_fsm( wp_fsm_manager *mgr, wp_u32 id );

/* ---- Access ----------------------------------------------------------- */

wp_fsm *wp_fsm_mgr_get_fsm( const wp_fsm_manager *mgr, wp_u32 id );
wp_u32 wp_fsm_mgr_get_num_fsms( const wp_fsm_manager *mgr );
wp_s32 wp_fsm_mgr_is_active( const wp_fsm_manager *mgr, wp_u32 id );

/* ---- Update ----------------------------------------------------------- */

void wp_fsm_mgr_update( wp_fsm_manager *mgr, wp_f64 dt );

/* ---- Bulk state change ------------------------------------------------ */

void wp_fsm_mgr_change_states( wp_fsm_manager *mgr );
void wp_fsm_mgr_change_state( wp_fsm_manager *mgr, wp_u32 id );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_FSMMGR_H */
