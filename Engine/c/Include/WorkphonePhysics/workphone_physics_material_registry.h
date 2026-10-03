/**
 * @file workphone_physics_material_registry.h
 * @brief C API for a data-driven physics material registry and database.
 *
 * The registry owns a set of named wp_physics_material handles and resolves
 * them by name at runtime, mirroring the Esoterica engine's MaterialRegistry.
 * The database is the authoring/serialisation unit (an array of descriptors)
 * that the editor writes and the runtime registers -- mirroring Esoterica's
 * MaterialDatabase resource and PhysicsMaterialLibrary data file.
 *
 * Typical flow:
 *   1. The editor authors a material database (text) and compiles it.
 *   2. At load time the database is parsed into wp_physics_material_desc[].
 *   3. wp_physics_material_database_register() inserts every descriptor into
 *      a wp_physics_material_registry, each becoming an owned material handle.
 *   4. Shapes/rigid bodies reference materials by name; the solver resolves
 *      them through the registry (falling back to the default material).
 *
 * @see workphone_physics_material.h
 */

#ifndef WORKPHONE_PHYSICS_MATERIAL_REGISTRY_H
#define WORKPHONE_PHYSICS_MATERIAL_REGISTRY_H

#include <stdint.h>
#include "workphone_types.h"
#include "workphone_physics_material.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Default material name (ported from Esoterica Material::s_defaultID)
 * ---------------------------------------------------------------------- */

#define WP_PHYSICS_MATERIAL_DEFAULT_NAME "Default"

/* -------------------------------------------------------------------------
 * Material database (authoring / serialisation unit)
 * ---------------------------------------------------------------------- */

#ifndef WP_PHYSICS_MATERIAL_DATABASE_INITIAL_CAPACITY
#    define WP_PHYSICS_MATERIAL_DATABASE_INITIAL_CAPACITY 16
#endif

/**
 * @brief An ordered array of material descriptors.
 *
 * Owned by the caller; free with wp_physics_material_database_free().
 */
typedef struct wp_physics_material_database
{
    wp_physics_material_desc *materials;
    wp_u32 count;
    wp_u32 capacity;
} wp_physics_material_database;

/** @brief Zeros a database structure (does not allocate). */
void wp_physics_material_database_init( wp_physics_material_database *db );

/** @brief Frees storage owned by the database. Safe on a zeroed struct. */
void wp_physics_material_database_free( wp_physics_material_database *db );

/** @brief Appends a descriptor to the database, growing storage as needed. */
wp_s32 wp_physics_material_database_add( wp_physics_material_database *db,
                                         const wp_physics_material_desc *desc );

/**
 * @brief Parses a human-authorable material library text into a database.
 *
 * Grammar (one material per block, keys are case-insensitive):
 * @code
 *   # line comments
 *   material Ice
 *     static_friction 0.10
 *     dynamic_friction 0.05
 *     rolling_friction 0.02
 *     restitution 0.20
 *     friction_combine average      # average|min|max|multiply
 *     restitution_combine max       # average|min|max|multiply
 *   material Rubber
 *     ...
 * @endcode
 *
 * @param text  NUL-terminated material library text.
 * @param db    Destination database (initialised if empty; appended to).
 * @return Number of materials parsed, or -1 on a parse error.
 */
wp_s32 wp_physics_material_database_load_from_text( const wp_c8 *text,
                                                    wp_physics_material_database *db );

/* -------------------------------------------------------------------------
 * Material registry (runtime resolution by name)
 * ---------------------------------------------------------------------- */

typedef struct wp_physics_material_registry wp_physics_material_registry;

/** @brief Creates an empty registry with the default material installed. */
wp_physics_material_registry *wp_physics_material_registry_create( void );

/** @brief Destroys a registry and every material it owns. */
void wp_physics_material_registry_destroy( wp_physics_material_registry *reg );

/** @brief Resets the registry to its initial state (default material only). */
void wp_physics_material_registry_clear( wp_physics_material_registry *reg );

/**
 * @brief Registers a material from a descriptor.
 *
 * The registry creates and owns the resulting handle.  If a material with the
 * same name already exists the call fails (returns NULL) without modifying
 * the registry.  Descriptors with an empty name fail.
 *
 * @return The newly created material handle, or NULL on failure.
 */
wp_physics_material *wp_physics_material_registry_register( wp_physics_material_registry *reg,
                                                             const wp_physics_material_desc *desc );

/** @brief Registers every descriptor in a database (skips duplicates). @return count added. */
wp_u32 wp_physics_material_registry_register_database( wp_physics_material_registry *reg,
                                                        const wp_physics_material_database *db );

/** @brief Unregisters (and destroys) a material by name. Returns 1 if removed. */
wp_s32 wp_physics_material_registry_unregister( wp_physics_material_registry *reg, const wp_c8 *name );

/** @brief Returns the default material (never NULL for a valid registry). */
wp_physics_material *wp_physics_material_registry_get_default( wp_physics_material_registry *reg );

/**
 * @brief Resolves a material by name.
 *
 * Returns the registered material, or the default material if the name is
 * empty/unknown (so callers always get a usable handle).
 */
wp_physics_material *wp_physics_material_registry_get_material( wp_physics_material_registry *reg,
                                                                 const wp_c8 *name );

/** @brief Like get_material but returns NULL when the name is not registered. */
wp_physics_material *wp_physics_material_registry_find_material( wp_physics_material_registry *reg,
                                                                 const wp_c8 *name );

/** @brief Returns non-zero if a material with the given name is registered. */
wp_s32 wp_physics_material_registry_has_material( wp_physics_material_registry *reg, const wp_c8 *name );

/** @brief Number of registered materials (including the default). */
wp_u32 wp_physics_material_registry_get_count( wp_physics_material_registry *reg );

/**
 * @brief Material enumeration for the editor.
 * @param index 0 .. count-1.
 * @param out_name Optional; receives the material name (buffer >= MAX_NAME).
 * @return The material handle at index, or NULL if out of range.
 */
wp_physics_material *wp_physics_material_registry_get_by_index( wp_physics_material_registry *reg,
                                                                 wp_u32 index, wp_c8 *out_name );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_PHYSICS_MATERIAL_REGISTRY_H */
