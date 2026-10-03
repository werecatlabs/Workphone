/**
 * @file workphone_factory.h
 * @brief C API for the engine factory.
 *
 * wp_factory is a stateful convenience layer that centralises the creation
 * and destruction of game actors, game components, and resources.  It:
 *
 *   - Tracks a monotonically increasing actor-id counter.
 *   - Allocates the correctly-sized concrete struct for each component type
 *     and calls the matching typed init function.
 *   - Dispatches typed destroy calls so callers never need to cast.
 *   - Optionally references a wp_resource_database for resource creation.
 *   - Supports actor cloning (shallow component copy) and prefab instantiation.
 */

#ifndef WORKPHONE_FACTORY_H
#define WORKPHONE_FACTORY_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_game_actor.h"
#include "workphone_game_component.h"
#include "workphone_game_prefab.h"
#include "workphone_resource.h"
#include "workphone_resource_database.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Factory structure
 * ---------------------------------------------------------------------- */

typedef struct wp_factory
{
    wp_s32 next_actor_id;         /**< Next unique actor id to assign.          */
    wp_resource_database *res_db; /**< Optional resource database (may be NULL). */
    void *user_data;
} wp_factory;

/* ---- Lifecycle -------------------------------------------------------- */

void wp_factory_init( wp_factory *f );
void wp_factory_init_with_db( wp_factory *f, wp_resource_database *db );
void wp_factory_destroy( wp_factory *f );

/* ---- Actor ------------------------------------------------------------ */

/**
 * @brief Allocates and initialises an actor with a unique id.
 */
wp_game_actor *wp_factory_create_actor( wp_factory *f );

/**
 * @brief Allocates, initialises, and names an actor with a unique id.
 */
wp_game_actor *wp_factory_create_named_actor( wp_factory *f, const wp_c8 *name );

/**
 * @brief Destroys and frees an actor and all components attached to it.
 */
void wp_factory_destroy_actor( wp_factory *f, wp_game_actor *actor );

/**
 * @brief Shallow-clones an actor: copies identity fields and component
 *        pointers but does NOT deep-copy component data.
 *
 * The returned actor has a new unique id and must be freed with
 * wp_factory_destroy_actor().
 */
wp_game_actor *wp_factory_clone_actor( wp_factory *f, const wp_game_actor *src );

/* ---- Component -------------------------------------------------------- */

/**
 * @brief Allocates the correctly-sized concrete struct for @p type and
 *        calls the matching typed init function.
 *
 * The returned pointer can be cast to the concrete type:
 *   WP_COMPONENT_TYPE_CAMERA    -> wp_camera_component *
 *   WP_COMPONENT_TYPE_RENDERER  -> wp_renderer_component *
 *   WP_COMPONENT_TYPE_PHYSICS   -> wp_rigidbody_component *
 *   WP_COMPONENT_TYPE_UI        -> wp_ui_component *
 *   WP_COMPONENT_TYPE_VEHICLE   -> wp_vehicle_component *
 *   WP_COMPONENT_TYPE_CUSTOM    -> wp_game_component *
 *
 * Returns NULL on allocation failure or unknown type.
 */
wp_game_component *wp_factory_create_component( wp_factory *f, enum wp_component_type type );

/**
 * @brief Calls the typed destroy for @p comp and frees the memory.
 */
void wp_factory_destroy_component( wp_factory *f, wp_game_component *comp );

/* ---- Resource --------------------------------------------------------- */

/**
 * @brief Creates a resource entry and registers it in the factory's
 *        resource database.
 *
 * Returns NULL if no resource database is attached or on failure.
 */
wp_resource *wp_factory_create_resource( wp_factory *f, wp_resource_type type, const wp_c8 *name,
                                         const wp_c8 *path );

/**
 * @brief Removes and destroys a resource from the factory's database.
 */
void wp_factory_destroy_resource( wp_factory *f, wp_resource *resource );

/* ---- Prefab ----------------------------------------------------------- */

/**
 * @brief Instantiates a prefab via its create callback and assigns a unique
 *        actor id to the result.
 *
 * Returns NULL if the prefab has no create callback or returns NULL itself.
 */
wp_game_actor *wp_factory_instantiate_prefab( wp_factory *f, wp_game_prefab *prefab );

/* ---- Accessors -------------------------------------------------------- */

wp_s32 wp_factory_get_next_actor_id( const wp_factory *f );
wp_resource_database *wp_factory_get_resource_db( const wp_factory *f );
void wp_factory_set_resource_db( wp_factory *f, wp_resource_database *db );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_FACTORY_H */
