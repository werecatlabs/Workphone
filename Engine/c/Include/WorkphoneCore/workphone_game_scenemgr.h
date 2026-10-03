/**
 * @file workphone_game_scenemgr.h
 * @brief C API for the game scene manager.
 */

#ifndef WORKPHONE_GAME_SCENEMGR_H
#define WORKPHONE_GAME_SCENEMGR_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_game_scene.h"
#include "workphone_fsmmgr.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WP_SCENEMGR_ACTOR_GROW_SIZE
#    define WP_SCENEMGR_ACTOR_GROW_SIZE 256
#endif

/**
 * @brief Central manager that owns actors, scenes and the FSM manager.
 *
 * Mirrors the C++ GameManager.  Actors are allocated from a contiguous
 * pool addressed by integer id.  A single current scene is tracked.
 */
typedef struct wp_game_scenemgr
{
    wp_game_actor **actors;
    wp_u32 *actor_active;
    wp_u32 num_actors;
    wp_u32 actor_capacity;

    wp_game_scene *current_scene;

    wp_fsm_manager fsm_mgr;

    void *user_data;
} wp_game_scenemgr;

/* ---- Lifecycle -------------------------------------------------------- */

void wp_game_scenemgr_init( wp_game_scenemgr *mgr );
void wp_game_scenemgr_init_with_capacity( wp_game_scenemgr *mgr, wp_u32 actor_capacity );
void wp_game_scenemgr_destroy( wp_game_scenemgr *mgr );

/* ---- Actor pool ------------------------------------------------------- */

wp_u32 wp_game_scenemgr_get_actor_capacity( const wp_game_scenemgr *mgr );
void wp_game_scenemgr_reserve_actors( wp_game_scenemgr *mgr, wp_u32 capacity );

/* ---- Actor creation / destruction ------------------------------------- */

wp_s32 wp_game_scenemgr_create_actor( wp_game_scenemgr *mgr );
void wp_game_scenemgr_destroy_actor( wp_game_scenemgr *mgr, wp_u32 id );
void wp_game_scenemgr_destroy_actors( wp_game_scenemgr *mgr );

/* ---- Actor access ----------------------------------------------------- */

wp_game_actor *wp_game_scenemgr_get_actor( const wp_game_scenemgr *mgr, wp_u32 id );
wp_game_actor *wp_game_scenemgr_find_actor_by_name( const wp_game_scenemgr *mgr, const wp_c8 *name );
wp_u32 wp_game_scenemgr_get_num_actors( const wp_game_scenemgr *mgr );

/* ---- Current scene ---------------------------------------------------- */

wp_game_scene *wp_game_scenemgr_get_current_scene( const wp_game_scenemgr *mgr );
void wp_game_scenemgr_set_current_scene( wp_game_scenemgr *mgr, wp_game_scene *scene );

/* ---- FSM manager ------------------------------------------------------ */

wp_fsm_manager *wp_game_scenemgr_get_fsm_manager( wp_game_scenemgr *mgr );

/* ---- Update ----------------------------------------------------------- */

void wp_game_scenemgr_update( wp_game_scenemgr *mgr, wp_f64 dt );

/* ---- Play / Edit / Stop ----------------------------------------------- */

void wp_game_scenemgr_play( wp_game_scenemgr *mgr );
void wp_game_scenemgr_edit( wp_game_scenemgr *mgr );
void wp_game_scenemgr_stop( wp_game_scenemgr *mgr );

/* ---- Clear ------------------------------------------------------------ */

void wp_game_scenemgr_clear( wp_game_scenemgr *mgr );

/* ---- User data -------------------------------------------------------- */

void *wp_game_scenemgr_get_user_data( const wp_game_scenemgr *mgr );
void wp_game_scenemgr_set_user_data( wp_game_scenemgr *mgr, void *user_data );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GAME_SCENEMGR_H */
