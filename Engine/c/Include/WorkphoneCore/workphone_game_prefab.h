/**
 * @file workphone_game_prefab.h
 * @brief C API for game prefabs -- reusable actor blueprints.
 */

#ifndef WORKPHONE_GAME_PREFAB_H
#define WORKPHONE_GAME_PREFAB_H

#include "workphone_prerequisites.h"
#include "workphone_string.h"

#ifdef __cplusplus
extern "C" {
#endif

struct wp_game_actor;

#ifndef WP_PREFAB_MAX_NAME
#    define WP_PREFAB_MAX_NAME 128
#endif

/**
 * @brief Callback used by a prefab to instantiate a new actor.
 *
 * @param prefab  The prefab requesting instantiation.
 * @param user_data Opaque context supplied when the callback was set.
 * @return A pointer to the newly created actor, or NULL on failure.
 */
typedef struct wp_game_actor *( *wp_prefab_create_actor_fn )( struct wp_game_prefab *prefab,
                                                              void *user_data );

/**
 * @brief A reusable actor blueprwp_s32 (prefab).
 *
 * A prefab holds a reference to a template actor and optional opaque data.
 * Calling wp_game_prefab_create_actor() stamps out a copy via the
 * user-supplied create callback.
 */
typedef struct wp_game_prefab
{
    wp_c8 name[WP_PREFAB_MAX_NAME];

    struct wp_game_actor *actor;

    void *data;

    wp_prefab_create_actor_fn create_fn;
    void *create_fn_user_data;
} wp_game_prefab;

/* ---- Lifecycle -------------------------------------------------------- */

void wp_game_prefab_init( wp_game_prefab *prefab );
void wp_game_prefab_destroy( wp_game_prefab *prefab );

/* ---- Name ------------------------------------------------------------- */

void wp_game_prefab_set_name( wp_game_prefab *prefab, const wp_c8 *name );
const wp_c8 *wp_game_prefab_get_name( const wp_game_prefab *prefab );

/* ---- Actor ------------------------------------------------------------ */

struct wp_game_actor *wp_game_prefab_get_actor( const wp_game_prefab *prefab );
void wp_game_prefab_set_actor( wp_game_prefab *prefab, struct wp_game_actor *actor );

/* ---- Data ------------------------------------------------------------- */

void *wp_game_prefab_get_data( const wp_game_prefab *prefab );
void wp_game_prefab_set_data( wp_game_prefab *prefab, void *data );

/* ---- Instantiation ---------------------------------------------------- */

void wp_game_prefab_set_create_fn( wp_game_prefab *prefab, wp_prefab_create_actor_fn fn,
                                   void *user_data );

struct wp_game_actor *wp_game_prefab_create_actor( wp_game_prefab *prefab );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GAME_PREFAB_H */
