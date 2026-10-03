/**
 * @file workphone_resource_database.h
 * @brief C API for the engine resource database -- central registry for all managed assets.
 */

#ifndef WORKPHONE_RESOURCE_DATABASE_H
#define WORKPHONE_RESOURCE_DATABASE_H

#include "workphone_prerequisites.h"
#include "workphone_resource.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Constants
 * ====================================================================== */

#ifndef WP_RESOURCE_DB_GROW_SIZE
#    define WP_RESOURCE_DB_GROW_SIZE 16
#endif

/* =========================================================================
 * Structure
 * ====================================================================== */

/**
 * @brief Central registry that owns and manages all engine resources.
 *
 * Resources are stored as pointers in a growable slot-based array.  Slots
 * are reused when resources are removed.  Look-ups by ID, path, or name
 * perform a linear scan which is sufficient for typical asset counts.
 */
typedef struct wp_resource_database
{
    wp_resource **resources; /**< Slot array of resource pointers. */
    wp_u32 *active;          /**< Per-slot active flag.            */
    wp_u32 count;            /**< Number of active resources.      */
    wp_u32 capacity;         /**< Total number of allocated slots. */
    wp_u32 grow_size;        /**< Slots to add when growing.       */
} wp_resource_database;

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

void wp_resource_db_init( wp_resource_database *db );
void wp_resource_db_init_with_capacity( wp_resource_database *db, wp_u32 capacity );
void wp_resource_db_destroy( wp_resource_database *db );

/* =========================================================================
 * Capacity
 * ====================================================================== */

wp_u32 wp_resource_db_get_capacity( const wp_resource_database *db );
void wp_resource_db_reserve( wp_resource_database *db, wp_u32 capacity );
wp_u32 wp_resource_db_get_grow_size( const wp_resource_database *db );
void wp_resource_db_set_grow_size( wp_resource_database *db, wp_u32 grow_size );

/* =========================================================================
 * Resource management
 * ====================================================================== */

wp_s32 wp_resource_db_add( wp_resource_database *db, wp_resource *resource );
wp_resource *wp_resource_db_create( wp_resource_database *db, wp_resource_type type, const wp_c8 *name,
                                    const wp_c8 *path );
void wp_resource_db_remove( wp_resource_database *db, wp_u32 index );
void wp_resource_db_remove_by_id( wp_resource_database *db, wp_s32 id );

/* =========================================================================
 * Lookup
 * ====================================================================== */

wp_resource *wp_resource_db_get( const wp_resource_database *db, wp_u32 index );
wp_resource *wp_resource_db_find_by_id( const wp_resource_database *db, wp_s32 id );
wp_resource *wp_resource_db_find_by_path( const wp_resource_database *db, const wp_c8 *path );
wp_resource *wp_resource_db_find_by_name( const wp_resource_database *db, const wp_c8 *name );

/* =========================================================================
 * Queries
 * ====================================================================== */

wp_u32 wp_resource_db_get_count( const wp_resource_database *db );
wp_s32 wp_resource_db_is_active( const wp_resource_database *db, wp_u32 index );

/* =========================================================================
 * Bulk operations
 * ====================================================================== */

void wp_resource_db_load_all( wp_resource_database *db );
void wp_resource_db_unload_all( wp_resource_database *db );
void wp_resource_db_purge_by_type( wp_resource_database *db, wp_resource_type type );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_RESOURCE_DATABASE_H */
