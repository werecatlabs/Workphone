/**
 * @file workphone_resource.c
 * @brief Implementation of the C resource API.
 */

#include "workphone_resource.h"
#include "workphone_memory.h"
#include "workphone_util.h"
#include <string.h>

/* =========================================================================
 * Internal state
 * ====================================================================== */

static wp_s32 wp_resource_next_id = 1;

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_resource *wp_resource_create( void )
{
    wp_resource *resource = (wp_resource *)wp_mem_alloc( sizeof( wp_resource ) );
    if( !resource )
        return NULL;

    memset( resource, 0, sizeof( wp_resource ) );
    resource->id = wp_resource_next_id++;
    resource->type = WP_RESOURCE_TYPE_UNKNOWN;
    resource->state = WP_RESOURCE_STATE_NONE;
    resource->flags = 0;
    resource->ref_count = 0;
    resource->path[0] = '\0';
    resource->name[0] = '\0';
    resource->data = NULL;
    resource->data_size = 0;
    resource->load_fn = NULL;
    resource->load_fn_user_data = NULL;
    resource->unload_fn = NULL;
    resource->unload_fn_user_data = NULL;
    resource->user_data = NULL;

    return resource;
}

void wp_resource_destroy( wp_resource *resource )
{
    if( !resource )
        return;

    if( resource->state == WP_RESOURCE_STATE_LOADED )
        wp_resource_unload( resource );

    resource->data = NULL;
    resource->data_size = 0;
    resource->user_data = NULL;
    resource->load_fn = NULL;
    resource->load_fn_user_data = NULL;
    resource->unload_fn = NULL;
    resource->unload_fn_user_data = NULL;

    wp_mem_free( resource );
}

/* =========================================================================
 * Reference counting
 * ====================================================================== */

void wp_resource_add_ref( wp_resource *resource )
{
    if( !resource )
        return;

    resource->ref_count++;
}

void wp_resource_release( wp_resource *resource )
{
    if( !resource )
        return;

    if( resource->ref_count > 0 )
        resource->ref_count--;

    if( resource->ref_count == 0 && !( resource->flags & WP_RESOURCE_FLAG_KEEP_ALIVE ) )
        wp_resource_unload( resource );
}

wp_s32 wp_resource_get_ref_count( const wp_resource *resource )
{
    return resource ? resource->ref_count : 0;
}

/* =========================================================================
 * Load / Unload
 * ====================================================================== */

void wp_resource_load( wp_resource *resource )
{
    if( !resource )
        return;

    if( resource->state == WP_RESOURCE_STATE_LOADED )
        return;

    resource->state = WP_RESOURCE_STATE_LOADING;

    if( resource->load_fn )
    {
        resource->load_fn( resource, resource->load_fn_user_data );
    }
    else
    {
        resource->state = WP_RESOURCE_STATE_LOADED;
    }
}

void wp_resource_unload( wp_resource *resource )
{
    if( !resource )
        return;

    if( resource->state != WP_RESOURCE_STATE_LOADED && resource->state != WP_RESOURCE_STATE_FAILED )
        return;

    resource->state = WP_RESOURCE_STATE_UNLOADING;

    if( resource->unload_fn )
        resource->unload_fn( resource, resource->unload_fn_user_data );

    resource->data = NULL;
    resource->data_size = 0;
    resource->state = WP_RESOURCE_STATE_NONE;
}

/* =========================================================================
 * Callbacks
 * ====================================================================== */

void wp_resource_set_load_fn( wp_resource *resource, wp_resource_load_fn fn, void *user_data )
{
    if( !resource )
        return;

    resource->load_fn = fn;
    resource->load_fn_user_data = user_data;
}

void wp_resource_set_unload_fn( wp_resource *resource, wp_resource_unload_fn fn, void *user_data )
{
    if( !resource )
        return;

    resource->unload_fn = fn;
    resource->unload_fn_user_data = user_data;
}

/* =========================================================================
 * ID
 * ====================================================================== */

wp_s32 wp_resource_get_id( const wp_resource *resource )
{
    return resource ? resource->id : 0;
}

void wp_resource_set_id( wp_resource *resource, wp_s32 id )
{
    if( resource )
        resource->id = id;
}

/* =========================================================================
 * Type
 * ====================================================================== */

wp_resource_type wp_resource_get_type( const wp_resource *resource )
{
    return resource ? resource->type : WP_RESOURCE_TYPE_UNKNOWN;
}

void wp_resource_set_type( wp_resource *resource, wp_resource_type type )
{
    if( resource )
        resource->type = type;
}

/* =========================================================================
 * State
 * ====================================================================== */

wp_resource_state wp_resource_get_state( const wp_resource *resource )
{
    return resource ? resource->state : WP_RESOURCE_STATE_NONE;
}

void wp_resource_set_state( wp_resource *resource, wp_resource_state state )
{
    if( resource )
        resource->state = state;
}

/* =========================================================================
 * Flags
 * ====================================================================== */

wp_u32 wp_resource_get_flags( const wp_resource *resource )
{
    return resource ? resource->flags : 0;
}

void wp_resource_set_flags( wp_resource *resource, wp_u32 flags )
{
    if( resource )
        resource->flags = flags;
}

wp_s32 wp_resource_get_flag( const wp_resource *resource, wp_u32 flag )
{
    return resource ? ( ( resource->flags & flag ) != 0 ) : 0;
}

void wp_resource_set_flag( wp_resource *resource, wp_u32 flag, wp_s32 value )
{
    if( !resource )
        return;

    if( value )
        resource->flags |= flag;
    else
        resource->flags &= ~flag;
}

/* =========================================================================
 * Path
 * ====================================================================== */

const wp_c8 *wp_resource_get_path( const wp_resource *resource )
{
    return resource ? resource->path : NULL;
}

void wp_resource_set_path( wp_resource *resource, const wp_c8 *path )
{
    if( !resource )
        return;

    if( path )
    {
        strncpy( resource->path, path, WP_RESOURCE_MAX_PATH - 1 );
        resource->path[WP_RESOURCE_MAX_PATH - 1] = '\0';
    }
    else
    {
        resource->path[0] = '\0';
    }
}

/* =========================================================================
 * Name
 * ====================================================================== */

const wp_c8 *wp_resource_get_name( const wp_resource *resource )
{
    return resource ? resource->name : NULL;
}

void wp_resource_set_name( wp_resource *resource, const wp_c8 *name )
{
    if( !resource )
        return;

    if( name )
    {
        strncpy( resource->name, name, WP_RESOURCE_MAX_NAME - 1 );
        resource->name[WP_RESOURCE_MAX_NAME - 1] = '\0';
    }
    else
    {
        resource->name[0] = '\0';
    }
}

/* =========================================================================
 * Data
 * ====================================================================== */

void *wp_resource_get_data( const wp_resource *resource )
{
    return resource ? resource->data : NULL;
}

void wp_resource_set_data( wp_resource *resource, void *data, wp_u32 size )
{
    if( !resource )
        return;

    resource->data = data;
    resource->data_size = size;
}

wp_u32 wp_resource_get_data_size( const wp_resource *resource )
{
    return resource ? resource->data_size : 0;
}

/* =========================================================================
 * User data
 * ====================================================================== */

void *wp_resource_get_user_data( const wp_resource *resource )
{
    return resource ? resource->user_data : NULL;
}

void wp_resource_set_user_data( wp_resource *resource, void *user_data )
{
    if( resource )
        resource->user_data = user_data;
}

/* =========================================================================
 * Queries
 * ====================================================================== */

wp_s32 wp_resource_is_loaded( const wp_resource *resource )
{
    return resource ? ( resource->state == WP_RESOURCE_STATE_LOADED ) : 0;
}

wp_s32 wp_resource_is_loading( const wp_resource *resource )
{
    return resource ? ( resource->state == WP_RESOURCE_STATE_LOADING ) : 0;
}

wp_s32 wp_resource_is_failed( const wp_resource *resource )
{
    return resource ? ( resource->state == WP_RESOURCE_STATE_FAILED ) : 0;
}
