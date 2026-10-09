/**
 * @file wp_rigidbody.c
 * @brief Implementation of the C rigid body API.
 */

#include "workphone_physics_rigidbody.h"
#include "workphone_physics_material.h"
#include "workphone_physics_constraint.h"
#include "workphone_physics_internal.h"
#include "workphone_physics_bounds.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal structure
 * ====================================================================== */

typedef struct wp_rigidbody
{
    wp_rigidbody_type body_type;
    wp_u32 flags;

    /* Transform */
    wp_vec3f position;
    wp_quatf orientation;
    uint64_t bounds_revision;
    uint64_t geometry_revision;
    wp_body_obb_cache obb_cache;

    /* Mass / inertia */
    wp_f32 mass;
    wp_f32 restitution;
    wp_f32 friction;
    wp_c8 material_name[WP_PHYSICS_MATERIAL_MAX_NAME];
    wp_vec3f inertia_tensor;
    wp_vec3f cmass_local_pos;

    /* Velocity */
    wp_vec3f linear_velocity;
    wp_vec3f angular_velocity;

    /* Accumulated forces */
    wp_vec3f force;
    wp_vec3f torque;
    wp_vec3f acceleration;
    wp_vec3f angular_acceleration;
    wp_vec3f gravity_override;
    wp_s32 has_gravity_override;

    /* Damping */
    wp_f32 linear_damping;
    wp_f32 angular_damping;
    wp_f32 max_angular_velocity;

    /* Sleep */
    wp_s32 is_sleeping;
    wp_f32 sleep_threshold;

    /* Shapes */
    wp_collision_shape *shapes[WP_RIGIDBODY_MAX_SHAPES];
    wp_s32 shape_count;

    /* Non-owning references, maintained by wp_constraint_set_body_*(). */
    wp_constraint *constraints[WP_RIGIDBODY_MAX_CONSTRAINTS];
    wp_s32 constraint_count;

    /* Collision filtering */
    wp_u32 collision_type;
    wp_u32 collision_mask;

    /* Opaque pointers */
    void *native;
    void *user_data;
} wp_rigidbody;

static wp_vec3f wp_vec3f_zero_rb( void )
{
    wp_vec3f v;
    memset( &v, 0, sizeof( v ) );
    return v;
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_rigidbody *wp_rigidbody_create( wp_rigidbody_type type )
{
    wp_rigidbody *body = (wp_rigidbody *)malloc( sizeof( wp_rigidbody ) );
    if( !body )
    {
        return NULL;
    }

    memset( body, 0, sizeof( wp_rigidbody ) );

    body->body_type = type;
    body->bounds_revision = 1;
    body->geometry_revision = 1;
    body->flags = WORKPHONE_RIGIDBODY_FLAG_ENABLED | WORKPHONE_RIGIDBODY_FLAG_GRAVITY;
    body->orientation.w = 1.0f;
    body->mass = 1.0f;
    body->friction = 0.5f;
    body->inertia_tensor.x = 1.0f;
    body->inertia_tensor.y = 1.0f;
    body->inertia_tensor.z = 1.0f;
    body->max_angular_velocity = 100.0f;
    body->sleep_threshold = 0.005f;
    body->collision_type = 0xFFFFFFFFu;
    body->collision_mask = 0xFFFFFFFFu;

    return body;
}

void wp_rigidbody_destroy( wp_rigidbody *body )
{
    wp_s32 i;

    if( !body )
    {
        return;
    }

    for( i = 0; i < body->shape_count; ++i )
    {
        wp_collision_shape_set_body( body->shapes[i], NULL );
        body->shapes[i] = NULL;
    }
    body->shape_count = 0;

    /*
     * Constraints can outlive their actors. Detach through the public
     * setters so the other actor's reference list remains valid too.
     */
    while( body->constraint_count > 0 )
    {
        wp_constraint *constraint = body->constraints[0];
        if( !constraint )
        {
            wp_rigidbody_remove_constraint_reference( body, constraint );
        }
        else if( wp_constraint_get_body_a( constraint ) == body )
        {
            wp_constraint_set_body_a( constraint, NULL );
        }
        else if( wp_constraint_get_body_b( constraint ) == body )
        {
            wp_constraint_set_body_b( constraint, NULL );
        }
        else
        {
            wp_rigidbody_remove_constraint_reference( body, constraint );
        }
    }
    free( body );
}

wp_rigidbody_type wp_rigidbody_get_type( const wp_rigidbody *body )
{
    if( !body )
    {
        return WORKPHONE_RIGIDBODY_STATIC;
    }

    return body->body_type;
}

static wp_f32 wp_rb_maxf( wp_f32 a, wp_f32 b )
{
    return a > b ? a : b;
}

static wp_quatf wp_quatf_normalize_rb( wp_quatf q )
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

void wp_rigidbody_set_type( wp_rigidbody *body, wp_rigidbody_type type )
{
    if( body )
    {
        body->body_type = type;
    }
}

/* =========================================================================
 * Transform
 * ====================================================================== */

wp_vec3f wp_rigidbody_get_position( const wp_rigidbody *body )
{
    if( !body )
    {
        return wp_vec3f_zero_rb();
    }
    return body->position;
}

void wp_rigidbody_set_position( wp_rigidbody *body, wp_vec3f position )
{
    if( !body )
    {
        return;
    }
    if( body->position.x != position.x || body->position.y != position.y ||
        body->position.z != position.z )
    {
        body->position = position;
        ++body->bounds_revision;
    }
}

wp_quatf wp_rigidbody_get_orientation( const wp_rigidbody *body )
{
    wp_quatf identity;
    memset( &identity, 0, sizeof( wp_quatf ) );
    identity.w = 1.0f;
    if( !body )
    {
        return identity;
    }
    return body->orientation;
}

void wp_rigidbody_set_orientation( wp_rigidbody *body, wp_quatf orientation )
{
    if( !body )
    {
        return;
    }
    orientation = wp_quatf_normalize_rb( orientation );
    if( body->orientation.x != orientation.x || body->orientation.y != orientation.y ||
        body->orientation.z != orientation.z || body->orientation.w != orientation.w )
    {
        body->orientation = orientation;
        ++body->bounds_revision;
    }
}

uint64_t wp_rigidbody_get_bounds_revision( const wp_rigidbody *body )
{
    return body ? body->bounds_revision : 0;
}

uint64_t wp_rigidbody_get_geometry_revision( const wp_rigidbody *body )
{
    return body ? body->geometry_revision : 0;
}

wp_body_obb_cache *wp_rigidbody_get_obb_cache( wp_rigidbody *body )
{
    return body ? &body->obb_cache : NULL;
}

void wp_rigidbody_invalidate_bounds( wp_rigidbody *body )
{
    if( body )
    {
        ++body->bounds_revision;
        ++body->geometry_revision;
    }
}

/* =========================================================================
 * Mass and inertia
 * ====================================================================== */

wp_f32 wp_rigidbody_get_mass( const wp_rigidbody *body )
{
    if( !body )
    {
        return 0.0f;
    }
    return body->mass;
}

void wp_rigidbody_set_mass( wp_rigidbody *body, wp_f32 mass )
{
    if( !body )
    {
        return;
    }
    body->mass = wp_rb_maxf( mass, 0.0f );
}

wp_f32 wp_rigidbody_get_restitution( const wp_rigidbody *body )
{
    return body ? body->restitution : 0.0f;
}

void wp_rigidbody_set_restitution( wp_rigidbody *body, wp_f32 restitution )
{
    if( body )
    {
        body->restitution = restitution < 0.0f ? 0.0f : ( restitution > 1.0f ? 1.0f : restitution );
    }
}

wp_f32 wp_rigidbody_get_friction( const wp_rigidbody *body )
{
    return body ? body->friction : 0.0f;
}

void wp_rigidbody_set_friction( wp_rigidbody *body, wp_f32 friction )
{
    if( body )
    {
        body->friction = wp_rb_maxf( friction, 0.0f );
    }
}

void wp_rigidbody_set_material_name( wp_rigidbody *body, const wp_c8 *name )
{
    if( !body )
    {
        return;
    }
    if( !name )
    {
        body->material_name[0] = '\0';
        return;
    }
    {
        wp_u32 i;
        for( i = 0; i < ( wp_u32 )( WP_PHYSICS_MATERIAL_MAX_NAME - 1 ) && name[i] != '\0'; ++i )
        {
            body->material_name[i] = name[i];
        }
        body->material_name[i] = '\0';
    }
}

const wp_c8 *wp_rigidbody_get_material_name( const wp_rigidbody *body )
{
    return body ? body->material_name : "";
}

wp_vec3f wp_rigidbody_get_inertia_tensor( const wp_rigidbody *body )
{
    if( !body )
    {
        return wp_vec3f_zero_rb();
    }
    return body->inertia_tensor;
}

void wp_rigidbody_set_inertia_tensor( wp_rigidbody *body, wp_vec3f inertia )
{
    if( !body )
    {
        return;
    }
    body->inertia_tensor.x = wp_rb_maxf( inertia.x, 0.0f );
    body->inertia_tensor.y = wp_rb_maxf( inertia.y, 0.0f );
    body->inertia_tensor.z = wp_rb_maxf( inertia.z, 0.0f );
}

wp_vec3f wp_rigidbody_get_cmass_local_position( const wp_rigidbody *body )
{
    if( !body )
    {
        return wp_vec3f_zero_rb();
    }
    return body->cmass_local_pos;
}

void wp_rigidbody_set_cmass_local_position( wp_rigidbody *body, wp_vec3f position )
{
    if( !body )
    {
        return;
    }
    body->cmass_local_pos = position;
}

/* =========================================================================
 * Velocity
 * ====================================================================== */

wp_vec3f wp_rigidbody_get_linear_velocity( const wp_rigidbody *body )
{
    if( !body )
    {
        return wp_vec3f_zero_rb();
    }
    return body->linear_velocity;
}

void wp_rigidbody_set_linear_velocity( wp_rigidbody *body, wp_vec3f velocity )
{
    if( !body )
    {
        return;
    }
    body->linear_velocity = velocity;
    body->is_sleeping = 0;
}

wp_vec3f wp_rigidbody_get_angular_velocity( const wp_rigidbody *body )
{
    if( !body )
    {
        return wp_vec3f_zero_rb();
    }
    return body->angular_velocity;
}

void wp_rigidbody_set_angular_velocity( wp_rigidbody *body, wp_vec3f velocity )
{
    if( !body )
    {
        return;
    }
    body->angular_velocity = velocity;
    body->is_sleeping = 0;
}

/* =========================================================================
 * Forces and torques
 * ====================================================================== */

void wp_rigidbody_add_force( wp_rigidbody *body, wp_vec3f force, wp_force_mode mode )
{
    wp_f32 inv_mass;

    if( !body )
    {
        return;
    }

    if( body->body_type != WORKPHONE_RIGIDBODY_DYNAMIC )
    {
        return;
    }

    inv_mass = body->mass > 1.0e-8f ? 1.0f / body->mass : 0.0f;
    switch( mode )
    {
    case WORKPHONE_FORCE_MODE_IMPULSE:
        body->linear_velocity.x += force.x * inv_mass;
        body->linear_velocity.y += force.y * inv_mass;
        body->linear_velocity.z += force.z * inv_mass;
        break;
    case WORKPHONE_FORCE_MODE_VELOCITY_CHANGE:
        body->linear_velocity.x += force.x;
        body->linear_velocity.y += force.y;
        body->linear_velocity.z += force.z;
        break;
    case WORKPHONE_FORCE_MODE_ACCELERATION:
        body->acceleration.x += force.x;
        body->acceleration.y += force.y;
        body->acceleration.z += force.z;
        break;
    case WORKPHONE_FORCE_MODE_FORCE:
    default:
        body->force.x += force.x;
        body->force.y += force.y;
        body->force.z += force.z;
        break;
    }
    body->is_sleeping = 0;
}

void wp_rigidbody_add_torque( wp_rigidbody *body, wp_vec3f torque, wp_force_mode mode )
{
    wp_vec3f inv_inertia;

    if( !body )
    {
        return;
    }

    if( body->body_type != WORKPHONE_RIGIDBODY_DYNAMIC )
    {
        return;
    }

    inv_inertia.x = body->inertia_tensor.x > 1.0e-8f ? 1.0f / body->inertia_tensor.x : 0.0f;
    inv_inertia.y = body->inertia_tensor.y > 1.0e-8f ? 1.0f / body->inertia_tensor.y : 0.0f;
    inv_inertia.z = body->inertia_tensor.z > 1.0e-8f ? 1.0f / body->inertia_tensor.z : 0.0f;
    switch( mode )
    {
    case WORKPHONE_FORCE_MODE_IMPULSE:
        body->angular_velocity.x += torque.x * inv_inertia.x;
        body->angular_velocity.y += torque.y * inv_inertia.y;
        body->angular_velocity.z += torque.z * inv_inertia.z;
        break;
    case WORKPHONE_FORCE_MODE_VELOCITY_CHANGE:
        body->angular_velocity.x += torque.x;
        body->angular_velocity.y += torque.y;
        body->angular_velocity.z += torque.z;
        break;
    case WORKPHONE_FORCE_MODE_ACCELERATION:
        body->angular_acceleration.x += torque.x;
        body->angular_acceleration.y += torque.y;
        body->angular_acceleration.z += torque.z;
        break;
    case WORKPHONE_FORCE_MODE_FORCE:
    default:
        body->torque.x += torque.x;
        body->torque.y += torque.y;
        body->torque.z += torque.z;
        break;
    }
    body->is_sleeping = 0;
}

void wp_rigidbody_clear_force( wp_rigidbody *body )
{
    if( !body )
    {
        return;
    }
    memset( &body->force, 0, sizeof( wp_vec3f ) );
    memset( &body->acceleration, 0, sizeof( wp_vec3f ) );
}

void wp_rigidbody_clear_torque( wp_rigidbody *body )
{
    if( !body )
    {
        return;
    }
    memset( &body->torque, 0, sizeof( wp_vec3f ) );
    memset( &body->angular_acceleration, 0, sizeof( wp_vec3f ) );
}

wp_vec3f wp_rigidbody_get_accumulated_force( const wp_rigidbody *body )
{
    if( !body )
    {
        return wp_vec3f_zero_rb();
    }
    return body->force;
}

wp_vec3f wp_rigidbody_get_accumulated_torque( const wp_rigidbody *body )
{
    if( !body )
    {
        return wp_vec3f_zero_rb();
    }
    return body->torque;
}

wp_vec3f wp_rigidbody_get_accumulated_acceleration( const wp_rigidbody *body )
{
    if( !body )
    {
        return wp_vec3f_zero_rb();
    }
    return body->acceleration;
}

wp_vec3f wp_rigidbody_get_accumulated_angular_acceleration( const wp_rigidbody *body )
{
    if( !body )
    {
        return wp_vec3f_zero_rb();
    }
    return body->angular_acceleration;
}

wp_vec3f wp_rigidbody_get_gravity_override( const wp_rigidbody *body )
{
    return body ? body->gravity_override : wp_vec3f_zero_rb();
}

void wp_rigidbody_set_gravity_override( wp_rigidbody *body, wp_vec3f gravity )
{
    if( body )
    {
        body->gravity_override = gravity;
        body->has_gravity_override = 1;
    }
}

void wp_rigidbody_clear_gravity_override( wp_rigidbody *body )
{
    if( body )
    {
        body->gravity_override = wp_vec3f_zero_rb();
        body->has_gravity_override = 0;
    }
}

wp_s32 wp_rigidbody_has_gravity_override( const wp_rigidbody *body )
{
    return body ? body->has_gravity_override : 0;
}

wp_f32 wp_rigidbody_get_linear_damping( const wp_rigidbody *body )
{
    return body ? body->linear_damping : 0.0f;
}

void wp_rigidbody_set_linear_damping( wp_rigidbody *body, wp_f32 damping )
{
    if( body )
    {
        body->linear_damping = wp_rb_maxf( damping, 0.0f );
    }
}

wp_f32 wp_rigidbody_get_angular_damping( const wp_rigidbody *body )
{
    return body ? body->angular_damping : 0.0f;
}

void wp_rigidbody_set_angular_damping( wp_rigidbody *body, wp_f32 damping )
{
    if( body )
    {
        body->angular_damping = wp_rb_maxf( damping, 0.0f );
    }
}

wp_f32 wp_rigidbody_get_max_angular_velocity( const wp_rigidbody *body )
{
    return body ? body->max_angular_velocity : 0.0f;
}

void wp_rigidbody_set_max_angular_velocity( wp_rigidbody *body, wp_f32 max_vel )
{
    if( body )
    {
        body->max_angular_velocity = wp_rb_maxf( max_vel, 0.0f );
    }
}

wp_s32 wp_rigidbody_is_sleeping( const wp_rigidbody *body )
{
    return body ? body->is_sleeping : 0;
}

void wp_rigidbody_wake_up( wp_rigidbody *body )
{
    if( body )
    {
        body->is_sleeping = 0;
    }
}

void wp_rigidbody_put_to_sleep( wp_rigidbody *body )
{
    if( body )
    {
        body->is_sleeping = 1;
        body->linear_velocity = wp_vec3f_zero_rb();
        body->angular_velocity = wp_vec3f_zero_rb();
        body->force = wp_vec3f_zero_rb();
        body->torque = wp_vec3f_zero_rb();
        body->acceleration = wp_vec3f_zero_rb();
        body->angular_acceleration = wp_vec3f_zero_rb();
    }
}

wp_f32 wp_rigidbody_get_sleep_threshold( const wp_rigidbody *body )
{
    return body ? body->sleep_threshold : 0.0f;
}

void wp_rigidbody_set_sleep_threshold( wp_rigidbody *body, wp_f32 threshold )
{
    if( body )
    {
        body->sleep_threshold = wp_rb_maxf( threshold, 0.0f );
    }
}

wp_s32 wp_rigidbody_add_shape( wp_rigidbody *body, wp_collision_shape *shape )
{
    wp_s32 i;

    if( !body || !shape || body->shape_count >= WP_RIGIDBODY_MAX_SHAPES )
    {
        return -1;
    }

    for( i = 0; i < body->shape_count; ++i )
    {
        if( body->shapes[i] == shape )
        {
            return i;
        }
    }

    if( wp_collision_shape_get_body( shape ) != NULL )
    {
        return -1;
    }

    body->shapes[body->shape_count] = shape;
    wp_collision_shape_set_body( shape, body );
    wp_rigidbody_invalidate_bounds( body );
    return body->shape_count++;
}

void wp_rigidbody_remove_shape( wp_rigidbody *body, wp_s32 index )
{
    wp_s32 i;
    if( !body || index < 0 || index >= body->shape_count )
    {
        return;
    }

    wp_collision_shape_set_body( body->shapes[index], NULL );
    for( i = index; i + 1 < body->shape_count; ++i )
    {
        body->shapes[i] = body->shapes[i + 1];
    }
    body->shapes[--body->shape_count] = NULL;
    wp_rigidbody_invalidate_bounds( body );
}

wp_collision_shape *wp_rigidbody_get_shape( const wp_rigidbody *body, wp_s32 index )
{
    if( !body || index < 0 || index >= body->shape_count )
    {
        return NULL;
    }
    return body->shapes[index];
}

wp_s32 wp_rigidbody_get_shape_count( const wp_rigidbody *body )
{
    return body ? body->shape_count : 0;
}

wp_s32 wp_rigidbody_add_constraint_reference( wp_rigidbody *body, wp_constraint *constraint )
{
    wp_s32 i;

    if( !body || !constraint )
    {
        return 0;
    }

    for( i = 0; i < body->constraint_count; ++i )
    {
        if( body->constraints[i] == constraint )
        {
            return 1;
        }
    }

    if( body->constraint_count >= WP_RIGIDBODY_MAX_CONSTRAINTS )
    {
        return 0;
    }

    body->constraints[body->constraint_count++] = constraint;
    return 1;
}

void wp_rigidbody_remove_constraint_reference( wp_rigidbody *body, wp_constraint *constraint )
{
    wp_s32 i;

    if( !body )
    {
        return;
    }

    for( i = 0; i < body->constraint_count; ++i )
    {
        if( body->constraints[i] == constraint )
        {
            wp_s32 j;
            for( j = i; j < body->constraint_count - 1; ++j )
            {
                body->constraints[j] = body->constraints[j + 1];
            }
            --body->constraint_count;
            body->constraints[body->constraint_count] = NULL;
            return;
        }
    }
}

wp_constraint *wp_rigidbody_get_constraint( const wp_rigidbody *body, wp_s32 index )
{
    if( !body || index < 0 || index >= body->constraint_count )
    {
        return NULL;
    }
    return body->constraints[index];
}

wp_s32 wp_rigidbody_get_constraint_count( const wp_rigidbody *body )
{
    return body ? body->constraint_count : 0;
}

wp_u32 wp_rigidbody_get_flags( const wp_rigidbody *body )
{
    return body ? body->flags : 0u;
}

void wp_rigidbody_set_flags( wp_rigidbody *body, wp_u32 flags )
{
    if( body )
    {
        body->flags = flags;
    }
}

void wp_rigidbody_set_flag( wp_rigidbody *body, wp_u32 flag, wp_s32 enabled )
{
    if( body )
    {
        if( enabled )
        {
            body->flags |= flag;
        }
        else
        {
            body->flags &= ~flag;
        }
    }
}

wp_s32 wp_rigidbody_has_flag( const wp_rigidbody *body, wp_u32 flag )
{
    return body && ( body->flags & flag ) != 0u;
}

wp_u32 wp_rigidbody_get_collision_type( const wp_rigidbody *body )
{
    return body ? body->collision_type : 0u;
}

void wp_rigidbody_set_collision_type( wp_rigidbody *body, wp_u32 type )
{
    if( body )
    {
        body->collision_type = type;
    }
}

wp_u32 wp_rigidbody_get_collision_mask( const wp_rigidbody *body )
{
    return body ? body->collision_mask : 0u;
}

void wp_rigidbody_set_collision_mask( wp_rigidbody *body, wp_u32 mask )
{
    if( body )
    {
        body->collision_mask = mask;
    }
}

void *wp_rigidbody_get_native( const wp_rigidbody *body )
{
    return body ? body->native : NULL;
}

void wp_rigidbody_set_native( wp_rigidbody *body, void *native )
{
    if( body )
    {
        body->native = native;
    }
}

void *wp_rigidbody_get_user_data( const wp_rigidbody *body )
{
    return body ? body->user_data : NULL;
}

void wp_rigidbody_set_user_data( wp_rigidbody *body, void *user_data )
{
    if( body )
    {
        body->user_data = user_data;
    }
}
