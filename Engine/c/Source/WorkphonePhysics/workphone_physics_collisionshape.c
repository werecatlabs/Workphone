/**
 * @file wp_collisionshape.c
 * @brief Implementation of the C collision shape API.
 */

#include "workphone_physics_collisionshape.h"
#include "workphone_physics_material.h"
#include "workphone_physics_material_registry.h"
#include "workphone_physics_rigidbody.h"
#include "workphone_physics_triangle_mesh.h"
#include "workphone_physics_internal.h"
#include "workphone_physics_geometry.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal structure
 * ====================================================================== */

typedef struct wp_collision_shape
{
    wp_collision_shape_type type;
    wp_u32 flags;

    /* Box */
    wp_vec3f half_extents;

    /* Sphere */
    wp_f32 radius;

    /* Capsule */
    wp_f32 half_height;

    /* Plane: dot(normal, x) + offset = 0 */
    wp_vec3f plane_normal;
    wp_f32 plane_offset;

    /* Mesh (shallow reference owned by caller) */
    wp_collision_mesh_data mesh_data;
    wp_triangle_mesh *triangle_mesh;

    /* Local pose */
    wp_vec3f local_position;
    wp_quatf local_orientation;

    /* Filtering */
    wp_filter_data filter_data;

    /* Parent body */
    wp_rigidbody *body;
    uint64_t revision, lifetime_id;
    wp_prepared_shape prepared;
    wp_physics_material *material;
    wp_c8 material_name[WP_PHYSICS_MATERIAL_MAX_NAME];

    /* Opaque back-end / user pointers */
    void *native;
    void *user_data;
} wp_collision_shape;

static void invalidate_shape_bounds( void *context )
{
    wp_collision_shape *shape = (wp_collision_shape *)context;
    ++shape->revision;
    wp_rigidbody_invalidate_bounds( shape->body );
}

static wp_f32 wp_shape_absf( wp_f32 value )
{
    return value < 0.0f ? -value : value;
}

static wp_quatf wp_shape_normalize_quat( wp_quatf q )
{
    wp_f32 length = sqrtf( q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w );
    if( length <= 1.0e-8f )
    {
        memset( &q, 0, sizeof( q ) );
        q.w = 1.0f;
        return q;
    }

    q.x /= length;
    q.y /= length;
    q.z /= length;
    q.w /= length;
    return q;
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_collision_shape *wp_collision_shape_create( wp_collision_shape_type type )
{
    wp_collision_shape *shape = (wp_collision_shape *)malloc( sizeof( wp_collision_shape ) );
    if( !shape )
    {
        return NULL;
    }

    memset( shape, 0, sizeof( wp_collision_shape ) );

    shape->type = type;
    shape->revision = 1;
    shape->lifetime_id = wp_physics_next_lifetime_id();
    shape->flags = WORKPHONE_COLLISION_SHAPE_FLAG_ENABLED;

    /* Default dimensions */
    shape->half_extents.x = 0.5f;
    shape->half_extents.y = 0.5f;
    shape->half_extents.z = 0.5f;
    shape->radius = 0.5f;
    shape->half_height = 0.5f;

    /* Default plane: Y-up at origin */
    shape->plane_normal.y = 1.0f;
    shape->plane_offset = 0.0f;

    /* Identity local orientation */
    shape->local_orientation.w = 1.0f;

    /* All-pass filter: collide with everything */
    shape->filter_data.word0 = 0xFFFFFFFFu;
    shape->filter_data.word1 = 0xFFFFFFFFu;

    return shape;
}

wp_prepared_shape *wp_collision_shape_get_prepared_storage( const wp_collision_shape *shape )
{
    return shape ? &((wp_collision_shape *)shape)->prepared : NULL;
}
uint64_t wp_collision_shape_get_revision( const wp_collision_shape *shape )
{
    return shape ? shape->revision : 0;
}
uint64_t wp_collision_shape_get_lifetime_id( const wp_collision_shape *shape )
{
    return shape ? shape->lifetime_id : 0;
}

void wp_collision_shape_destroy( wp_collision_shape *shape )
{
    wp_rigidbody *body;
    wp_s32 i;

    if( !shape )
    {
        return;
    }

    body = shape->body;
    if( body )
    {
        for( i = 0; i < wp_rigidbody_get_shape_count( body ); ++i )
        {
            if( wp_rigidbody_get_shape( body, i ) == shape )
            {
                wp_rigidbody_remove_shape( body, i );
                break;
            }
        }
    }
    wp_triangle_mesh_destroy( shape->triangle_mesh );
    shape->triangle_mesh = NULL;
    free( shape );
}

/* =========================================================================
 * Shape type
 * ====================================================================== */

wp_collision_shape_type wp_collision_shape_get_type( const wp_collision_shape *shape )
{
    if( !shape )
    {
        return WORKPHONE_COLLISION_SHAPE_BOX;
    }

    return shape->type;
}

/* =========================================================================
 * Box dimensions
 * ====================================================================== */

void wp_collision_shape_set_box_half_extents( wp_collision_shape *shape, wp_vec3f half_extents )
{
    if( !shape )
    {
        return;
    }

    shape->half_extents.x = wp_shape_absf( half_extents.x );
    shape->half_extents.y = wp_shape_absf( half_extents.y );
    shape->half_extents.z = wp_shape_absf( half_extents.z );
    invalidate_shape_bounds( shape );
}

wp_vec3f wp_collision_shape_get_box_half_extents( const wp_collision_shape *shape )
{
    wp_vec3f zero;
    memset( &zero, 0, sizeof( wp_vec3f ) );

    if( !shape )
    {
        return zero;
    }

    return shape->half_extents;
}

/* =========================================================================
 * Sphere dimensions
 * ====================================================================== */

void wp_collision_shape_set_sphere_radius( wp_collision_shape *shape, wp_f32 radius )
{
    if( !shape )
    {
        return;
    }

    shape->radius = wp_shape_absf( radius );
    invalidate_shape_bounds( shape );
}

wp_f32 wp_collision_shape_get_sphere_radius( const wp_collision_shape *shape )
{
    if( !shape )
    {
        return 0.0f;
    }

    return shape->radius;
}

/* =========================================================================
 * Capsule dimensions
 * ====================================================================== */

void wp_collision_shape_set_capsule( wp_collision_shape *shape, wp_f32 radius, wp_f32 half_height )
{
    if( !shape )
    {
        return;
    }

    shape->radius = wp_shape_absf( radius );
    shape->half_height = wp_shape_absf( half_height );
    invalidate_shape_bounds( shape );
}

wp_f32 wp_collision_shape_get_capsule_radius( const wp_collision_shape *shape )
{
    if( !shape )
    {
        return 0.0f;
    }

    return shape->radius;
}

wp_f32 wp_collision_shape_get_capsule_half_height( const wp_collision_shape *shape )
{
    if( !shape )
    {
        return 0.0f;
    }

    return shape->half_height;
}

/* =========================================================================
 * Plane
 * ====================================================================== */

void wp_collision_shape_set_plane( wp_collision_shape *shape, wp_vec3f normal, wp_f32 offset )
{
    wp_f32 length;

    if( !shape )
    {
        return;
    }

    length = sqrtf( normal.x * normal.x + normal.y * normal.y + normal.z * normal.z );
    if( length <= 1.0e-8f )
    {
        return;
    }

    shape->plane_normal.x = normal.x / length;
    shape->plane_normal.y = normal.y / length;
    shape->plane_normal.z = normal.z / length;
    shape->plane_offset = offset;
    invalidate_shape_bounds( shape );
}

wp_vec3f wp_collision_shape_get_plane_normal( const wp_collision_shape *shape )
{
    wp_vec3f zero;
    memset( &zero, 0, sizeof( wp_vec3f ) );

    if( !shape )
    {
        return zero;
    }

    return shape->plane_normal;
}

wp_f32 wp_collision_shape_get_plane_offset( const wp_collision_shape *shape )
{
    if( !shape )
    {
        return 0.0f;
    }

    return shape->plane_offset;
}

/* =========================================================================
 * Mesh data
 * ====================================================================== */

void wp_collision_shape_set_mesh_data( wp_collision_shape *shape, const wp_collision_mesh_data *data )
{
    if( !shape )
    {
        return;
    }

    invalidate_shape_bounds( shape );
    wp_triangle_mesh_destroy( shape->triangle_mesh );
    shape->triangle_mesh = NULL;

    if( !data )
    {
        memset( &shape->mesh_data, 0, sizeof( wp_collision_mesh_data ) );
        return;
    }

    shape->mesh_data = *data;
    if( data->vertices && data->indices && data->vertex_count > 0u && data->triangle_count > 0u )
    {
        shape->triangle_mesh = wp_triangle_mesh_create( data->vertices, data->vertex_count,
                                                        data->indices, data->triangle_count );
        wp_triangle_mesh_set_refit_callback( shape->triangle_mesh, invalidate_shape_bounds, shape );
    }
}

const wp_collision_mesh_data *wp_collision_shape_get_mesh_data( const wp_collision_shape *shape )
{
    if( !shape || shape->type != WORKPHONE_COLLISION_SHAPE_MESH )
    {
        return NULL;
    }

    return &shape->mesh_data;
}

const wp_triangle_mesh *wp_collision_shape_get_triangle_mesh( const wp_collision_shape *shape )
{
    if( !shape || shape->type != WORKPHONE_COLLISION_SHAPE_MESH )
    {
        return NULL;
    }
    return shape->triangle_mesh;
}

/* =========================================================================
 * Local pose
 * ====================================================================== */

void wp_collision_shape_set_local_position( wp_collision_shape *shape, wp_vec3f position )
{
    if( !shape )
    {
        return;
    }

    shape->local_position = position;
    invalidate_shape_bounds( shape );
}

wp_vec3f wp_collision_shape_get_local_position( const wp_collision_shape *shape )
{
    wp_vec3f zero;
    memset( &zero, 0, sizeof( wp_vec3f ) );

    if( !shape )
    {
        return zero;
    }

    return shape->local_position;
}

void wp_collision_shape_set_local_orientation( wp_collision_shape *shape, wp_quatf orientation )
{
    if( !shape )
    {
        return;
    }

    shape->local_orientation = wp_shape_normalize_quat( orientation );
    invalidate_shape_bounds( shape );
}

wp_quatf wp_collision_shape_get_local_orientation( const wp_collision_shape *shape )
{
    wp_quatf identity;
    memset( &identity, 0, sizeof( wp_quatf ) );
    identity.w = 1.0f;

    if( !shape )
    {
        return identity;
    }

    return shape->local_orientation;
}

/* =========================================================================
 * Enable / trigger
 * ====================================================================== */

void wp_collision_shape_set_enabled( wp_collision_shape *shape, wp_s32 enabled )
{
    if( !shape )
    {
        return;
    }

    if( wp_collision_shape_is_enabled( shape ) != ( enabled != 0 ) )
    {
        invalidate_shape_bounds( shape );
    }
    if( enabled )
    {
        shape->flags |= WORKPHONE_COLLISION_SHAPE_FLAG_ENABLED;
    }
    else
    {
        shape->flags &= ~WORKPHONE_COLLISION_SHAPE_FLAG_ENABLED;
    }
}

wp_s32 wp_collision_shape_is_enabled( const wp_collision_shape *shape )
{
    if( !shape )
    {
        return 0;
    }

    return ( shape->flags & WORKPHONE_COLLISION_SHAPE_FLAG_ENABLED ) != 0;
}

void wp_collision_shape_set_trigger( wp_collision_shape *shape, wp_s32 trigger )
{
    if( !shape )
    {
        return;
    }

    if( trigger )
    {
        shape->flags |= WORKPHONE_COLLISION_SHAPE_FLAG_TRIGGER;
    }
    else
    {
        shape->flags &= ~WORKPHONE_COLLISION_SHAPE_FLAG_TRIGGER;
    }
}

wp_s32 wp_collision_shape_is_trigger( const wp_collision_shape *shape )
{
    if( !shape )
    {
        return 0;
    }

    return ( shape->flags & WORKPHONE_COLLISION_SHAPE_FLAG_TRIGGER ) != 0;
}

/* =========================================================================
 * Collision filtering
 * ====================================================================== */

void wp_collision_shape_set_collision_type( wp_collision_shape *shape, wp_u32 mask )
{
    if( !shape )
    {
        return;
    }

    shape->filter_data.word0 = mask;
}

wp_u32 wp_collision_shape_get_collision_type( const wp_collision_shape *shape )
{
    if( !shape )
    {
        return 0;
    }

    return shape->filter_data.word0;
}

void wp_collision_shape_set_collision_mask( wp_collision_shape *shape, wp_u32 mask )
{
    if( !shape )
    {
        return;
    }

    shape->filter_data.word1 = mask;
}

wp_u32 wp_collision_shape_get_collision_mask( const wp_collision_shape *shape )
{
    if( !shape )
    {
        return 0;
    }

    return shape->filter_data.word1;
}

void wp_collision_shape_set_filter_data( wp_collision_shape *shape, wp_filter_data data )
{
    if( !shape )
    {
        return;
    }

    shape->filter_data = data;
}

wp_filter_data wp_collision_shape_get_filter_data( const wp_collision_shape *shape )
{
    wp_filter_data zero;
    memset( &zero, 0, sizeof( wp_filter_data ) );

    if( !shape )
    {
        return zero;
    }

    return shape->filter_data;
}

/* =========================================================================
 * Body attachment
 * ====================================================================== */

void wp_collision_shape_set_body( wp_collision_shape *shape, wp_rigidbody *body )
{
    if( !shape )
    {
        return;
    }

    shape->body = body;
}

wp_rigidbody *wp_collision_shape_get_body( const wp_collision_shape *shape )
{
    if( !shape )
    {
        return NULL;
    }

    return shape->body;
}

wp_s32 wp_collision_shape_is_attached( const wp_collision_shape *shape )
{
    if( !shape )
    {
        return 0;
    }

    return shape->body != NULL;
}

void wp_collision_shape_set_material( wp_collision_shape *shape, wp_physics_material *material )
{
    if( shape )
    {
        shape->material = material;
    }
}

wp_physics_material *wp_collision_shape_get_material( const wp_collision_shape *shape )
{
    return shape ? shape->material : NULL;
}

void wp_collision_shape_set_material_name( wp_collision_shape *shape, const wp_c8 *name )
{
    if( !shape )
    {
        return;
    }
    if( !name )
    {
        shape->material_name[0] = '\0';
        return;
    }
    {
        wp_u32 i;
        for( i = 0; i < ( wp_u32 )( WP_PHYSICS_MATERIAL_MAX_NAME - 1 ) && name[i] != '\0'; ++i )
        {
            shape->material_name[i] = name[i];
        }
        shape->material_name[i] = '\0';
    }
}

const wp_c8 *wp_collision_shape_get_material_name( const wp_collision_shape *shape )
{
    return shape ? shape->material_name : "";
}

wp_physics_material *wp_collision_shape_resolve_material( const wp_collision_shape *shape,
                                                           wp_physics_material_registry *registry )
{
    if( !shape || !registry )
    {
        return NULL;
    }
    /* An explicit material pointer takes priority (low-level override). */
    if( shape->material )
    {
        return shape->material;
    }
    /* Then the shape's own material name via the registry. */
    if( shape->material_name[0] != '\0' )
    {
        return wp_physics_material_registry_get_material( registry, shape->material_name );
    }
    /* Then fall back to the owning body's material name. */
    if( shape->body )
    {
        const wp_c8 *body_name = wp_rigidbody_get_material_name( shape->body );
        if( body_name && body_name[0] != '\0' )
        {
            return wp_physics_material_registry_get_material( registry, body_name );
        }
    }
    /* Finally the registry default. */
    return wp_physics_material_registry_get_default( registry );
}

/* =========================================================================
 * Native / user data
 * ====================================================================== */

void wp_collision_shape_set_native( wp_collision_shape *shape, void *native )
{
    if( !shape )
    {
        return;
    }

    shape->native = native;
}

void *wp_collision_shape_get_native( const wp_collision_shape *shape )
{
    if( !shape )
    {
        return NULL;
    }

    return shape->native;
}

void wp_collision_shape_set_user_data( wp_collision_shape *shape, void *user_data )
{
    if( !shape )
    {
        return;
    }

    shape->user_data = user_data;
}

void *wp_collision_shape_get_user_data( const wp_collision_shape *shape )
{
    if( !shape )
    {
        return NULL;
    }

    return shape->user_data;
}
