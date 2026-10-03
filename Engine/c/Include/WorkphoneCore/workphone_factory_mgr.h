/**
 * @file workphone_factory_mgr.h
 * @brief C API for the factory manager.
 *
 * wp_factory_mgr is the top-level coordinator that owns:
 *
 *   - A wp_factory           -- actor / component / resource allocation.
 *   - A wp_resource_database -- central asset registry.
 *   - A named-creator registry -- maps string keys to actor-creator callbacks
 *                                 so subsystems can spawn typed actors without
 *                                 compile-time knowledge of the concrete type.
 *   - A prefab registry      -- stores wp_game_prefab instances by name for
 *                                 on-demand instantiation.
 */

#ifndef WORKPHONE_FACTORY_MGR_H
#define WORKPHONE_FACTORY_MGR_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_factory.h"
#include "workphone_resource_database.h"
#include "workphone_game_prefab.h"
#include "workphone_game_actor.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Forward declaration
 * ---------------------------------------------------------------------- */

struct wp_factory_mgr;

/* -------------------------------------------------------------------------
 * Named-creator callback
 * ---------------------------------------------------------------------- */

/**
 * @brief Callback that creates and fully initialises an actor of a specific
 *        "kind" (e.g. "Player", "Enemy", "Projectile").
 *
 * @param mgr       The factory manager that triggered the call.
 * @param user_data Opaque context supplied at registration time.
 * @return A ready-to-use actor, or NULL on failure.
 */
typedef wp_game_actor *( *wp_factory_creator_fn )( struct wp_factory_mgr *mgr, void *user_data );

/* -------------------------------------------------------------------------
 * Limits
 * ---------------------------------------------------------------------- */

#ifndef WP_FACTORY_MGR_MAX_NAME
#    define WP_FACTORY_MGR_MAX_NAME 128
#endif

#ifndef WP_FACTORY_MGR_ENTRY_GROW
#    define WP_FACTORY_MGR_ENTRY_GROW 8
#endif

#ifndef WP_FACTORY_MGR_PREFAB_GROW
#    define WP_FACTORY_MGR_PREFAB_GROW 8
#endif

/* -------------------------------------------------------------------------
 * Internal registry entry (opaque to callers)
 * ---------------------------------------------------------------------- */

typedef struct wp_factory_entry
{
    wp_c8 name[WP_FACTORY_MGR_MAX_NAME];
    wp_factory_creator_fn creator;
    void *user_data;
} wp_factory_entry;

/* -------------------------------------------------------------------------
 * Factory manager structure
 * ---------------------------------------------------------------------- */

typedef struct wp_factory_mgr
{
    wp_factory factory;          /**< Owned factory (actor/component allocator). */
    wp_resource_database res_db; /**< Owned resource database.                   */

    wp_factory_entry *entries; /**< Named-creator registry (growable).  */
    wp_u32 num_entries;
    wp_u32 entry_capacity;

    wp_game_prefab **prefabs; /**< Prefab registry (growable).         */
    wp_u32 num_prefabs;
    wp_u32 prefab_capacity;

    void *user_data;
} wp_factory_mgr;

/* ---- Lifecycle -------------------------------------------------------- */

void wp_factory_mgr_init( wp_factory_mgr *mgr );
void wp_factory_mgr_destroy( wp_factory_mgr *mgr );

/* ---- Inner accessors -------------------------------------------------- */

wp_factory *wp_factory_mgr_get_factory( wp_factory_mgr *mgr );
wp_resource_database *wp_factory_mgr_get_resource_db( wp_factory_mgr *mgr );

/* ---- Actor creation (via factory) ------------------------------------ */

wp_game_actor *wp_factory_mgr_create_actor( wp_factory_mgr *mgr );
wp_game_actor *wp_factory_mgr_create_named_actor( wp_factory_mgr *mgr, const wp_c8 *name );
void wp_factory_mgr_destroy_actor( wp_factory_mgr *mgr, wp_game_actor *actor );

/* ---- Component creation (via factory) -------------------------------- */

wp_game_component *wp_factory_mgr_create_component( wp_factory_mgr *mgr, enum wp_component_type type );
void wp_factory_mgr_destroy_component( wp_factory_mgr *mgr, wp_game_component *comp );

/* ---- Resource creation (via factory) --------------------------------- */

wp_resource *wp_factory_mgr_create_resource( wp_factory_mgr *mgr, wp_resource_type type,
                                             const wp_c8 *name, const wp_c8 *path );
void wp_factory_mgr_destroy_resource( wp_factory_mgr *mgr, wp_resource *resource );

/* ---- Named-creator registry ------------------------------------------ */

/**
 * @brief Registers a creator callback under a unique name key.
 *
 * If a creator is already registered under @p name it is replaced.
 *
 * @return Non-zero on success; zero on allocation failure.
 */
wp_s32 wp_factory_mgr_register_creator( wp_factory_mgr *mgr, const wp_c8 *name,
                                        wp_factory_creator_fn creator, void *user_data );

/**
 * @brief Removes the creator registered under @p name.
 */
void wp_factory_mgr_unregister_creator( wp_factory_mgr *mgr, const wp_c8 *name );

/**
 * @brief Looks up and invokes the creator registered under @p name.
 *
 * @return A new actor, or NULL if no creator is registered or it fails.
 */
wp_game_actor *wp_factory_mgr_create( wp_factory_mgr *mgr, const wp_c8 *name );

/**
 * @brief Returns non-zero if a creator is registered under @p name.
 */
wp_s32 wp_factory_mgr_has_creator( const wp_factory_mgr *mgr, const wp_c8 *name );

/* ---- Prefab registry ------------------------------------------------- */

/**
 * @brief Adds @p prefab to the registry.
 *
 * Ownership is NOT transferred; the caller is responsible for the prefab's
 * lifetime.
 *
 * @return Non-zero on success; zero on allocation failure or duplicate name.
 */
wp_s32 wp_factory_mgr_register_prefab( wp_factory_mgr *mgr, wp_game_prefab *prefab );

/**
 * @brief Removes the prefab registered under @p name.
 */
void wp_factory_mgr_unregister_prefab( wp_factory_mgr *mgr, const wp_c8 *name );

/**
 * @brief Finds a registered prefab by name.
 *
 * @return Pointer to the prefab, or NULL if not found.
 */
wp_game_prefab *wp_factory_mgr_find_prefab( const wp_factory_mgr *mgr, const wp_c8 *name );

/**
 * @brief Instantiates the prefab registered under @p name and assigns a
 *        factory-managed actor id to the result.
 *
 * @return A new actor, or NULL if the prefab is not found or instantiation fails.
 */
wp_game_actor *wp_factory_mgr_instantiate( wp_factory_mgr *mgr, const wp_c8 *name );

/**
 * @brief Returns the number of registered prefabs.
 */
wp_u32 wp_factory_mgr_get_prefab_count( const wp_factory_mgr *mgr );

/* ---- User data -------------------------------------------------------- */

void *wp_factory_mgr_get_user_data( const wp_factory_mgr *mgr );
void wp_factory_mgr_set_user_data( wp_factory_mgr *mgr, void *user_data );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_FACTORY_MGR_H */
