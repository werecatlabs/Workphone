/**
 * @file workphone_game_scene.h
 * @brief C API for game scenes -- containers that manage actors.
 */

#ifndef WORKPHONE_GAME_SCENE_H
#define WORKPHONE_GAME_SCENE_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_string.h"
#include "workphone_game_actor.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Scene runtime states.
 */
enum wp_scene_state
{
    WP_SCENE_STATE_NONE = 0,
    WP_SCENE_STATE_EDIT,
    WP_SCENE_STATE_PLAY,
    WP_SCENE_STATE_RESET,
    WP_SCENE_STATE_COUNT
};

/**
 * @brief Scene loading states.
 */
enum wp_scene_loading_state
{
    WP_SCENE_LOADING_NONE = 0,
    WP_SCENE_LOADING_LOADED,
    WP_SCENE_LOADING_UNLOADED,
    WP_SCENE_LOADING_COUNT
};

#ifndef WP_SCENE_MAX_NAME
#    define WP_SCENE_MAX_NAME 256
#endif

#ifndef WP_SCENE_ACTOR_GROW_SIZE
#    define WP_SCENE_ACTOR_GROW_SIZE 64
#endif

/**
 * @brief A game scene that owns and manages a dynamic set of actors.
 */
typedef struct wp_game_scene
{
    wp_c8 name[WP_SCENE_MAX_NAME];

    wp_game_actor **actors;
    wp_u32 num_actors;
    wp_u32 actor_capacity;

    enum wp_scene_state state;
    enum wp_scene_loading_state loading_state;

    void *user_data;
} wp_game_scene;

/* ---- Lifecycle -------------------------------------------------------- */

void wp_game_scene_init( wp_game_scene *scene );
void wp_game_scene_init_with_capacity( wp_game_scene *scene, wp_u32 capacity );
void wp_game_scene_destroy( wp_game_scene *scene );

/* ---- Name ------------------------------------------------------------- */

void wp_game_scene_set_name( wp_game_scene *scene, const wp_c8 *name );
const wp_c8 *wp_game_scene_get_name( const wp_game_scene *scene );

/* ---- State ------------------------------------------------------------ */

enum wp_scene_state wp_game_scene_get_state( const wp_game_scene *scene );
void wp_game_scene_set_state( wp_game_scene *scene, enum wp_scene_state state );

enum wp_scene_loading_state wp_game_scene_get_loading_state( const wp_game_scene *scene );
void wp_game_scene_set_loading_state( wp_game_scene *scene, enum wp_scene_loading_state state );

/* ---- Capacity --------------------------------------------------------- */

wp_u32 wp_game_scene_get_actor_capacity( const wp_game_scene *scene );
void wp_game_scene_reserve_actors( wp_game_scene *scene, wp_u32 capacity );

/* ---- Actor management ------------------------------------------------- */

wp_s32 wp_game_scene_add_actor( wp_game_scene *scene, wp_game_actor *actor );
wp_s32 wp_game_scene_remove_actor( wp_game_scene *scene, wp_game_actor *actor );
void wp_game_scene_remove_all_actors( wp_game_scene *scene );

wp_game_actor *wp_game_scene_get_actor( const wp_game_scene *scene, wp_u32 index );
wp_u32 wp_game_scene_get_num_actors( const wp_game_scene *scene );

wp_game_actor *wp_game_scene_find_actor_by_name( const wp_game_scene *scene, const wp_c8 *name );
wp_game_actor *wp_game_scene_find_actor_by_id( const wp_game_scene *scene, wp_s32 id );

/* ---- Clear ------------------------------------------------------------ */

void wp_game_scene_clear( wp_game_scene *scene );

/* ---- User data -------------------------------------------------------- */

void *wp_game_scene_get_user_data( const wp_game_scene *scene );
void wp_game_scene_set_user_data( wp_game_scene *scene, void *user_data );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GAME_SCENE_H */
