/**
 * @file workphone_resource_database.c
 * @brief Implementation of the C resource database API.
 */

#include "workphone_resource_database.h"

#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static void wp_resource_db_grow( wp_resource_database *db )
{
    wp_u32 new_cap = db->capacity + db->grow_size;
    wp_resource_db_reserve( db, new_cap );
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

void wp_resource_db_init( wp_resource_database *db )
{
    memset( db, 0, sizeof( *db ) );
    db->resources = NULL;
    db->active = NULL;
    db->count = 0;
    db->capacity = 0;
    db->grow_size = WP_RESOURCE_DB_GROW_SIZE;
}

void wp_resource_db_init_with_capacity( wp_resource_database *db, wp_u32 capacity )
{
    wp_resource_db_init( db );
    wp_resource_db_reserve( db, capacity );
}

void wp_resource_db_destroy( wp_resource_database *db )
{
    wp_u32 i;
    for( i = 0; i < db->capacity; ++i )
    {
        if( db->active[i] )
        {
            wp_resource_destroy( db->resources[i] );
            db->resources[i] = NULL;
            db->active[i] = 0;
        }
    }
    free( db->resources );
    free( db->active );
    db->resources = NULL;
    db->active = NULL;
    db->count = 0;
    db->capacity = 0;
}

/* =========================================================================
 * Capacity
 * ====================================================================== */

wp_u32 wp_resource_db_get_capacity( const wp_resource_database *db )
{
    return db->capacity;
}

void wp_resource_db_reserve( wp_resource_database *db, wp_u32 capacity )
{
    wp_resource **new_resources;
    wp_u32 *new_active;
    wp_u32 old_cap;

    if( capacity <= db->capacity )
        return;

    old_cap = db->capacity;

    new_resources = (wp_resource **)realloc( db->resources, capacity * sizeof( wp_resource * ) );
    if( !new_resources )
        return;
    memset( new_resources + old_cap, 0, ( capacity - old_cap ) * sizeof( wp_resource * ) );
    db->resources = new_resources;

    new_active = (wp_u32 *)realloc( db->active, capacity * sizeof( wp_u32 ) );
    if( !new_active )
        return;
    memset( new_active + old_cap, 0, ( capacity - old_cap ) * sizeof( wp_u32 ) );
    db->active = new_active;

    db->capacity = capacity;
}

wp_u32 wp_resource_db_get_grow_size( const wp_resource_database *db )
{
    return db->grow_size;
}

void wp_resource_db_set_grow_size( wp_resource_database *db, wp_u32 grow_size )
{
    db->grow_size = grow_size;
}

/* =========================================================================
 * Resource management
 * ====================================================================== */

wp_s32 wp_resource_db_add( wp_resource_database *db, wp_resource *resource )
{
    wp_u32 i;
    wp_u32 first_new;

    if( !resource )
        return -1;

    for( i = 0; i < db->capacity; ++i )
    {
        if( !db->active[i] )
        {
            db->resources[i] = resource;
            db->active[i] = 1;
            db->count++;
            return (wp_s32)i;
        }
    }

    first_new = db->capacity;
    wp_resource_db_grow( db );
    if( db->capacity <= first_new )
        return -1;

    db->resources[first_new] = resource;
    db->active[first_new] = 1;
    db->count++;
    return (wp_s32)first_new;
}

wp_resource *wp_resource_db_create( wp_resource_database *db, wp_resource_type type, const wp_c8 *name,
                                    const wp_c8 *path )
{
    wp_resource *resource;
    wp_s32 index;

    resource = wp_resource_create();
    if( !resource )
        return NULL;

    wp_resource_set_type( resource, type );
    if( name )
        wp_resource_set_name( resource, name );
    if( path )
        wp_resource_set_path( resource, path );

    index = wp_resource_db_add( db, resource );
    if( index < 0 )
    {
        wp_resource_destroy( resource );
        return NULL;
    }

    return resource;
}

void wp_resource_db_remove( wp_resource_database *db, wp_u32 index )
{
    if( index >= db->capacity )
        return;
    if( !db->active[index] )
        return;

    wp_resource_destroy( db->resources[index] );
    db->resources[index] = NULL;
    db->active[index] = 0;
    db->count--;
}

void wp_resource_db_remove_by_id( wp_resource_database *db, wp_s32 id )
{
    wp_u32 i;
    for( i = 0; i < db->capacity; ++i )
    {
        if( db->active[i] && db->resources[i]->id == id )
        {
            wp_resource_db_remove( db, i );
            return;
        }
    }
}

/* =========================================================================
 * Lookup
 * ====================================================================== */

wp_resource *wp_resource_db_get( const wp_resource_database *db, wp_u32 index )
{
    if( index >= db->capacity )
        return NULL;
    return db->active[index] ? db->resources[index] : NULL;
}

wp_resource *wp_resource_db_find_by_id( const wp_resource_database *db, wp_s32 id )
{
    wp_u32 i;
    for( i = 0; i < db->capacity; ++i )
    {
        if( db->active[i] && db->resources[i]->id == id )
            return db->resources[i];
    }
    return NULL;
}

wp_resource *wp_resource_db_find_by_path( const wp_resource_database *db, const wp_c8 *path )
{
    wp_u32 i;
    if( !path )
        return NULL;
    for( i = 0; i < db->capacity; ++i )
    {
        if( db->active[i] && strcmp( db->resources[i]->path, path ) == 0 )
            return db->resources[i];
    }
    return NULL;
}

wp_resource *wp_resource_db_find_by_name( const wp_resource_database *db, const wp_c8 *name )
{
    wp_u32 i;
    if( !name )
        return NULL;
    for( i = 0; i < db->capacity; ++i )
    {
        if( db->active[i] && strcmp( db->resources[i]->name, name ) == 0 )
            return db->resources[i];
    }
    return NULL;
}

/* =========================================================================
 * Queries
 * ====================================================================== */

wp_u32 wp_resource_db_get_count( const wp_resource_database *db )
{
    return db->count;
}

wp_s32 wp_resource_db_is_active( const wp_resource_database *db, wp_u32 index )
{
    return ( index < db->capacity ) ? (wp_s32)db->active[index] : 0;
}

/* =========================================================================
 * Bulk operations
 * ====================================================================== */

void wp_resource_db_load_all( wp_resource_database *db )
{
    wp_u32 i;
    for( i = 0; i < db->capacity; ++i )
    {
        if( db->active[i] )
            wp_resource_load( db->resources[i] );
    }
}

void wp_resource_db_unload_all( wp_resource_database *db )
{
    wp_u32 i;
    for( i = 0; i < db->capacity; ++i )
    {
        if( db->active[i] )
            wp_resource_unload( db->resources[i] );
    }
}

void wp_resource_db_purge_by_type( wp_resource_database *db, wp_resource_type type )
{
    wp_u32 i;
    for( i = 0; i < db->capacity; ++i )
    {
        if( db->active[i] && db->resources[i]->type == type )
            wp_resource_db_remove( db, i );
    }
}
