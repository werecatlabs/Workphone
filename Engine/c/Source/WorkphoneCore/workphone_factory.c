/**
 * @file workphone_factory.c
 * @brief Implementation of the engine factory.
 */

#include "workphone_factory.h"
#include "workphone_game_actor.h"
#include "workphone_game_component.h"
#include "workphone_game_component_camera.h"
#include "workphone_game_component_renderer.h"
#include "workphone_game_component_rigidbody.h"
#include "workphone_game_component_ui.h"
#include "workphone_game_component_vehicle.h"
#include "workphone_game_prefab.h"
#include "workphone_resource.h"
#include "workphone_resource_database.h"

#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static wp_s32 s_next_id( wp_factory *f )
{
    return f->next_actor_id++;
}

/**
 * @brief Returns the allocation size for the concrete struct behind @p type.
 */
static wp_u32 s_component_size( enum wp_component_type type )
{
    switch( type )
    {
    case WP_COMPONENT_TYPE_CAMERA:
        return (wp_u32)sizeof( wp_camera_component );
    case WP_COMPONENT_TYPE_RENDERER:
        return (wp_u32)sizeof( wp_renderer_component );
    case WP_COMPONENT_TYPE_PHYSICS:
        return (wp_u32)sizeof( wp_rigidbody_component );
    case WP_COMPONENT_TYPE_UI:
        return (wp_u32)sizeof( wp_ui_component );
    case WP_COMPONENT_TYPE_VEHICLE:
        return (wp_u32)sizeof( wp_vehicle_component );
    default:
        return (wp_u32)sizeof( wp_game_component );
    }
}

/**
 * @brief Calls the typed init function for the concrete component struct.
 */
static void s_component_init( wp_game_component *comp, enum wp_component_type type )
{
    switch( type )
    {
    case WP_COMPONENT_TYPE_CAMERA:
        wp_camera_component_init( (wp_camera_component *)comp );
        break;
    case WP_COMPONENT_TYPE_RENDERER:
        wp_renderer_component_init( (wp_renderer_component *)comp );
        break;
    case WP_COMPONENT_TYPE_PHYSICS:
        wp_rigidbody_component_init( (wp_rigidbody_component *)comp );
        break;
    case WP_COMPONENT_TYPE_UI:
        wp_ui_component_init( (wp_ui_component *)comp );
        break;
    case WP_COMPONENT_TYPE_VEHICLE:
        wp_vehicle_component_init( (wp_vehicle_component *)comp );
        break;
    default:
        wp_game_component_init_with_type( comp, type );
        break;
    }
}

/**
 * @brief Calls the typed destroy function for the concrete component struct.
 */
static void s_component_destroy( wp_game_component *comp )
{
    switch( comp->type )
    {
    case WP_COMPONENT_TYPE_CAMERA:
        wp_camera_component_destroy( (wp_camera_component *)comp );
        break;
    case WP_COMPONENT_TYPE_RENDERER:
        wp_renderer_component_destroy( (wp_renderer_component *)comp );
        break;
    case WP_COMPONENT_TYPE_PHYSICS:
        wp_rigidbody_component_destroy( (wp_rigidbody_component *)comp );
        break;
    case WP_COMPONENT_TYPE_UI:
        wp_ui_component_destroy( (wp_ui_component *)comp );
        break;
    case WP_COMPONENT_TYPE_VEHICLE:
        wp_vehicle_component_destroy( (wp_vehicle_component *)comp );
        break;
    default:
        wp_game_component_destroy( comp );
        break;
    }
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

void wp_factory_init( wp_factory *f )
{
    if( !f )
        return;

    memset( f, 0, sizeof( *f ) );
    f->next_actor_id = 1;
    f->res_db = NULL;
    f->user_data = NULL;
}

void wp_factory_init_with_db( wp_factory *f, wp_resource_database *db )
{
    wp_factory_init( f );
    if( f )
        f->res_db = db;
}

void wp_factory_destroy( wp_factory *f )
{
    if( !f )
        return;

    f->res_db = NULL;
    f->user_data = NULL;
}

/* =========================================================================
 * Actor
 * ====================================================================== */

wp_game_actor *wp_factory_create_actor( wp_factory *f )
{
    wp_game_actor *actor;

    if( !f )
        return NULL;

    actor = (wp_game_actor *)malloc( sizeof( wp_game_actor ) );
    if( !actor )
        return NULL;

    wp_game_actor_init_with_id( actor, s_next_id( f ) );
    return actor;
}

wp_game_actor *wp_factory_create_named_actor( wp_factory *f, const wp_c8 *name )
{
    wp_game_actor *actor = wp_factory_create_actor( f );
    if( actor && name )
        wp_game_actor_set_name( actor, name );
    return actor;
}

void wp_factory_destroy_actor( wp_factory *f, wp_game_actor *actor )
{
    wp_u32 i;

    (void)f;

    if( !actor )
        return;

    /* Destroy and free every attached component. */
    for( i = 0; i < actor->num_components; ++i )
    {
        wp_game_component *comp = actor->components[i];
        if( comp )
        {
            s_component_destroy( comp );
            free( comp );
            actor->components[i] = NULL;
        }
    }
    actor->num_components = 0;

    wp_game_actor_destroy( actor );
    free( actor );
}

wp_game_actor *wp_factory_clone_actor( wp_factory *f, const wp_game_actor *src )
{
    wp_game_actor *dst;
    wp_u32 i;

    if( !f || !src )
        return NULL;

    dst = (wp_game_actor *)malloc( sizeof( wp_game_actor ) );
    if( !dst )
        return NULL;

    /* Copy all fields from source. */
    memcpy( dst, src, sizeof( wp_game_actor ) );

    /* Assign a fresh id and clear scene / hierarchy links. */
    dst->id = s_next_id( f );
    dst->parent = NULL;
    dst->num_children = 0;
    dst->sibling_index = 0;
    dst->scene = NULL;

    /* The clone shares the same component pointers (shallow copy). */
    for( i = 0; i < dst->num_components; ++i )
    {
        if( dst->components[i] )
            dst->components[i]->actor = dst;
    }

    return dst;
}

/* =========================================================================
 * Component
 * ====================================================================== */

wp_game_component *wp_factory_create_component( wp_factory *f, enum wp_component_type type )
{
    wp_u32 size;
    wp_game_component *comp;

    (void)f;

    if( type >= WP_COMPONENT_TYPE_COUNT )
        return NULL;

    size = s_component_size( type );
    comp = (wp_game_component *)malloc( size );
    if( !comp )
        return NULL;

    memset( comp, 0, size );
    s_component_init( comp, type );
    return comp;
}

void wp_factory_destroy_component( wp_factory *f, wp_game_component *comp )
{
    (void)f;

    if( !comp )
        return;

    s_component_destroy( comp );
    free( comp );
}

/* =========================================================================
 * Resource
 * ====================================================================== */

wp_resource *wp_factory_create_resource( wp_factory *f, wp_resource_type type, const wp_c8 *name,
                                         const wp_c8 *path )
{
    if( !f || !f->res_db )
        return NULL;

    return wp_resource_db_create( f->res_db, type, name, path );
}

void wp_factory_destroy_resource( wp_factory *f, wp_resource *resource )
{
    if( !f || !f->res_db || !resource )
        return;

    wp_resource_db_remove_by_id( f->res_db, resource->id );
}

/* =========================================================================
 * Prefab
 * ====================================================================== */

wp_game_actor *wp_factory_instantiate_prefab( wp_factory *f, wp_game_prefab *prefab )
{
    wp_game_actor *actor;

    if( !f || !prefab )
        return NULL;

    actor = wp_game_prefab_create_actor( prefab );
    if( !actor )
        return NULL;

    /* Assign a factory-managed id so the actor fits into the normal id space. */
    actor->id = s_next_id( f );
    return actor;
}

/* =========================================================================
 * Accessors
 * ====================================================================== */

wp_s32 wp_factory_get_next_actor_id( const wp_factory *f )
{
    return f ? f->next_actor_id : 0;
}

wp_resource_database *wp_factory_get_resource_db( const wp_factory *f )
{
    return f ? f->res_db : NULL;
}

void wp_factory_set_resource_db( wp_factory *f, wp_resource_database *db )
{
    if( f )
        f->res_db = db;
}
