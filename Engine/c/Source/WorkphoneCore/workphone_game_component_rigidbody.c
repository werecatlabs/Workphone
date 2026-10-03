/**
 * @file workphone_game_component_rigidbody.c
 * @brief Implementation of the rigidbody game component.
 */

#include "workphone_game_component_rigidbody.h"
#include "workphone_game_component.h"
#include "workphone_game_actor.h"
#include "workphone_physics_rigidbody.h"

#include <string.h>

/* =========================================================================
 * Default values
 * ====================================================================== */

#define WP_RB_DEFAULT_MASS ( 1.0f )
#define WP_RB_DEFAULT_LINEAR_DAMPING ( 0.0f )
#define WP_RB_DEFAULT_ANGULAR_DAMPING ( 0.05f )

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static void s_push_actor_transform( wp_rigidbody_component *rc )
{
    wp_transform3f world;

    if( !rc->body || !rc->base.actor )
        return;

    world = rc->base.actor->world_transform;
    wp_rigidbody_set_position( rc->body, world.position );
    wp_rigidbody_set_orientation( rc->body, world.orientation );
}

static void s_pull_body_transform( wp_rigidbody_component *rc )
{
    wp_transform3f *world;

    if( !rc->body || !rc->base.actor )
        return;

    world = &rc->base.actor->world_transform;
    world->position = wp_rigidbody_get_position( rc->body );
    world->orientation = wp_rigidbody_get_orientation( rc->body );

    /* local_transform mirrors world when there is no parent */
    if( !rc->base.actor->parent )
        rc->base.actor->local_transform = *world;
}

static void s_apply_properties( wp_rigidbody_component *rc )
{
    wp_u32 flags;

    if( !rc->body )
        return;

    wp_rigidbody_set_mass( rc->body, rc->mass );
    wp_rigidbody_set_linear_damping( rc->body, rc->linear_damping );
    wp_rigidbody_set_angular_damping( rc->body, rc->angular_damping );
    wp_rigidbody_set_collision_type( rc->body, rc->collision_type );
    wp_rigidbody_set_collision_mask( rc->body, rc->collision_mask );

    flags = wp_rigidbody_get_flags( rc->body );

    if( rc->use_gravity )
        flags |= WORKPHONE_RIGIDBODY_FLAG_GRAVITY;
    else
        flags &= ~WORKPHONE_RIGIDBODY_FLAG_GRAVITY;

    if( wp_game_component_is_enabled( &rc->base ) )
        flags |= WORKPHONE_RIGIDBODY_FLAG_ENABLED;
    else
        flags &= ~WORKPHONE_RIGIDBODY_FLAG_ENABLED;

    wp_rigidbody_set_flags( rc->body, flags );
}

/* =========================================================================
 * Component callbacks
 * ====================================================================== */

static void s_on_create( wp_game_component *comp )
{
    wp_rigidbody_component *rc = (wp_rigidbody_component *)comp;

    if( !rc->body )
    {
        wp_rigidbody_type t = rc->is_kinematic ? WORKPHONE_RIGIDBODY_KINEMATIC : rc->body_type;
        rc->body = wp_rigidbody_create( t );
        if( rc->body )
            wp_rigidbody_set_user_data( rc->body, rc );
    }

    s_apply_properties( rc );
    s_push_actor_transform( rc );
}

static void s_on_destroy( wp_game_component *comp )
{
    wp_rigidbody_component *rc = (wp_rigidbody_component *)comp;

    if( rc->body )
    {
        wp_rigidbody_destroy( rc->body );
        rc->body = NULL;
    }
}

static void s_on_update( wp_game_component *comp, wp_f64 dt )
{
    wp_rigidbody_component *rc = (wp_rigidbody_component *)comp;
    (void)dt;

    if( !rc->body )
        return;

    if( rc->body_type == WORKPHONE_RIGIDBODY_DYNAMIC && !rc->is_kinematic )
        s_pull_body_transform( rc );
}

static void s_on_enable( wp_game_component *comp )
{
    wp_rigidbody_component *rc = (wp_rigidbody_component *)comp;

    if( rc->body )
        wp_rigidbody_set_flag( rc->body, WORKPHONE_RIGIDBODY_FLAG_ENABLED, 1 );
}

static void s_on_disable( wp_game_component *comp )
{
    wp_rigidbody_component *rc = (wp_rigidbody_component *)comp;

    if( rc->body )
        wp_rigidbody_set_flag( rc->body, WORKPHONE_RIGIDBODY_FLAG_ENABLED, 0 );
}

static void s_on_transform_updated( wp_game_component *comp )
{
    wp_rigidbody_component *rc = (wp_rigidbody_component *)comp;

    /* Always push for static and kinematic; also allow teleport of dynamic. */
    s_push_actor_transform( rc );
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

static void s_init_common( wp_rigidbody_component *rc, wp_rigidbody_type body_type )
{
    wp_component_callbacks cb;

    memset( rc, 0, sizeof( *rc ) );

    wp_game_component_init_with_type( &rc->base, WP_COMPONENT_TYPE_PHYSICS );
    wp_game_component_set_name( &rc->base, "Rigidbody" );

    rc->body = NULL;
    rc->body_type = body_type;
    rc->mass = WP_RB_DEFAULT_MASS;
    rc->linear_damping = WP_RB_DEFAULT_LINEAR_DAMPING;
    rc->angular_damping = WP_RB_DEFAULT_ANGULAR_DAMPING;
    rc->use_gravity = 1;
    rc->is_kinematic = ( body_type == WORKPHONE_RIGIDBODY_KINEMATIC ) ? 1 : 0;
    rc->collision_type = 0xFFFFFFFFu;
    rc->collision_mask = 0xFFFFFFFFu;

    memset( &rc->rb_callbacks, 0, sizeof( rc->rb_callbacks ) );
    rc->rb_user_data = NULL;

    memset( &cb, 0, sizeof( cb ) );
    cb.on_create = s_on_create;
    cb.on_destroy = s_on_destroy;
    cb.on_update = s_on_update;
    cb.on_enable = s_on_enable;
    cb.on_disable = s_on_disable;
    cb.on_transform_updated = s_on_transform_updated;
    wp_game_component_set_callbacks( &rc->base, cb );
}

void wp_rigidbody_component_init( wp_rigidbody_component *rc )
{
    if( !rc )
        return;

    s_init_common( rc, WORKPHONE_RIGIDBODY_DYNAMIC );
}

void wp_rigidbody_component_init_with_type( wp_rigidbody_component *rc, wp_rigidbody_type body_type )
{
    if( !rc )
        return;

    s_init_common( rc, body_type );
}

void wp_rigidbody_component_destroy( wp_rigidbody_component *rc )
{
    if( !rc )
        return;

    wp_game_component_destroy( &rc->base );
}

/* =========================================================================
 * Update and sync
 * ====================================================================== */

void wp_rigidbody_component_update( wp_rigidbody_component *rc, wp_f64 dt )
{
    if( !rc )
        return;

    wp_game_component_update( &rc->base, dt );
}

void wp_rigidbody_component_sync_to_physics( wp_rigidbody_component *rc )
{
    if( !rc )
        return;

    s_push_actor_transform( rc );
}

void wp_rigidbody_component_sync_from_physics( wp_rigidbody_component *rc )
{
    if( !rc )
        return;

    s_pull_body_transform( rc );
}

/* =========================================================================
 * Physics body access
 * ====================================================================== */

wp_rigidbody *wp_rigidbody_component_get_body( const wp_rigidbody_component *rc )
{
    return rc ? rc->body : NULL;
}

wp_rigidbody_type wp_rigidbody_component_get_body_type( const wp_rigidbody_component *rc )
{
    return rc ? rc->body_type : WORKPHONE_RIGIDBODY_STATIC;
}

void wp_rigidbody_component_set_body_type( wp_rigidbody_component *rc, wp_rigidbody_type type )
{
    if( !rc )
        return;

    rc->body_type = type;
    rc->is_kinematic = ( type == WORKPHONE_RIGIDBODY_KINEMATIC ) ? 1 : 0;

    if( rc->body )
    {
        /* Recreate with new type to honour the change. */
        wp_rigidbody_destroy( rc->body );
        rc->body = wp_rigidbody_create( type );
        if( rc->body )
        {
            wp_rigidbody_set_user_data( rc->body, rc );
            s_apply_properties( rc );
            s_push_actor_transform( rc );
        }
    }
}

/* =========================================================================
 * Mass
 * ====================================================================== */

void wp_rigidbody_component_set_mass( wp_rigidbody_component *rc, wp_f32 mass )
{
    if( !rc )
        return;

    rc->mass = mass;
    if( rc->body )
        wp_rigidbody_set_mass( rc->body, mass );
}

wp_f32 wp_rigidbody_component_get_mass( const wp_rigidbody_component *rc )
{
    return rc ? rc->mass : WP_RB_DEFAULT_MASS;
}

/* =========================================================================
 * Damping
 * ====================================================================== */

void wp_rigidbody_component_set_linear_damping( wp_rigidbody_component *rc, wp_f32 damping )
{
    if( !rc )
        return;

    rc->linear_damping = damping;
    if( rc->body )
        wp_rigidbody_set_linear_damping( rc->body, damping );
}

wp_f32 wp_rigidbody_component_get_linear_damping( const wp_rigidbody_component *rc )
{
    return rc ? rc->linear_damping : WP_RB_DEFAULT_LINEAR_DAMPING;
}

void wp_rigidbody_component_set_angular_damping( wp_rigidbody_component *rc, wp_f32 damping )
{
    if( !rc )
        return;

    rc->angular_damping = damping;
    if( rc->body )
        wp_rigidbody_set_angular_damping( rc->body, damping );
}

wp_f32 wp_rigidbody_component_get_angular_damping( const wp_rigidbody_component *rc )
{
    return rc ? rc->angular_damping : WP_RB_DEFAULT_ANGULAR_DAMPING;
}

/* =========================================================================
 * Gravity / kinematic
 * ====================================================================== */

void wp_rigidbody_component_set_use_gravity( wp_rigidbody_component *rc, wp_s32 enabled )
{
    if( !rc )
        return;

    rc->use_gravity = enabled;
    if( rc->body )
        wp_rigidbody_set_flag( rc->body, WORKPHONE_RIGIDBODY_FLAG_GRAVITY, enabled );
}

wp_s32 wp_rigidbody_component_get_use_gravity( const wp_rigidbody_component *rc )
{
    return rc ? rc->use_gravity : 0;
}

void wp_rigidbody_component_set_kinematic( wp_rigidbody_component *rc, wp_s32 kinematic )
{
    wp_rigidbody_type new_type;

    if( !rc )
        return;

    if( rc->is_kinematic == kinematic )
        return;

    rc->is_kinematic = kinematic;
    new_type = kinematic ? WORKPHONE_RIGIDBODY_KINEMATIC : WORKPHONE_RIGIDBODY_DYNAMIC;
    wp_rigidbody_component_set_body_type( rc, new_type );
}

wp_s32 wp_rigidbody_component_is_kinematic( const wp_rigidbody_component *rc )
{
    return rc ? rc->is_kinematic : 0;
}

/* =========================================================================
 * Velocity
 * ====================================================================== */

void wp_rigidbody_component_set_linear_velocity( wp_rigidbody_component *rc, wp_vec3f v )
{
    if( rc && rc->body )
        wp_rigidbody_set_linear_velocity( rc->body, v );
}

wp_vec3f wp_rigidbody_component_get_linear_velocity( const wp_rigidbody_component *rc )
{
    wp_vec3f zero;
    zero.x = 0.0f;
    zero.y = 0.0f;
    zero.z = 0.0f;

    if( !rc || !rc->body )
        return zero;

    return wp_rigidbody_get_linear_velocity( rc->body );
}

void wp_rigidbody_component_set_angular_velocity( wp_rigidbody_component *rc, wp_vec3f v )
{
    if( rc && rc->body )
        wp_rigidbody_set_angular_velocity( rc->body, v );
}

wp_vec3f wp_rigidbody_component_get_angular_velocity( const wp_rigidbody_component *rc )
{
    wp_vec3f zero;
    zero.x = 0.0f;
    zero.y = 0.0f;
    zero.z = 0.0f;

    if( !rc || !rc->body )
        return zero;

    return wp_rigidbody_get_angular_velocity( rc->body );
}

/* =========================================================================
 * Forces and torques
 * ====================================================================== */

void wp_rigidbody_component_add_force( wp_rigidbody_component *rc, wp_vec3f force, wp_force_mode mode )
{
    if( rc && rc->body )
        wp_rigidbody_add_force( rc->body, force, mode );
}

void wp_rigidbody_component_add_torque( wp_rigidbody_component *rc, wp_vec3f torque, wp_force_mode mode )
{
    if( rc && rc->body )
        wp_rigidbody_add_torque( rc->body, torque, mode );
}

void wp_rigidbody_component_clear_forces( wp_rigidbody_component *rc )
{
    if( !rc || !rc->body )
        return;

    wp_rigidbody_clear_force( rc->body );
    wp_rigidbody_clear_torque( rc->body );
}

/* =========================================================================
 * Sleep
 * ====================================================================== */

wp_s32 wp_rigidbody_component_is_sleeping( const wp_rigidbody_component *rc )
{
    return ( rc && rc->body ) ? wp_rigidbody_is_sleeping( rc->body ) : 0;
}

void wp_rigidbody_component_wake_up( wp_rigidbody_component *rc )
{
    if( rc && rc->body )
        wp_rigidbody_wake_up( rc->body );
}

void wp_rigidbody_component_put_to_sleep( wp_rigidbody_component *rc )
{
    if( rc && rc->body )
        wp_rigidbody_put_to_sleep( rc->body );
}

/* =========================================================================
 * Collision filtering
 * ====================================================================== */

void wp_rigidbody_component_set_collision_type( wp_rigidbody_component *rc, wp_u32 type )
{
    if( !rc )
        return;

    rc->collision_type = type;
    if( rc->body )
        wp_rigidbody_set_collision_type( rc->body, type );
}

wp_u32 wp_rigidbody_component_get_collision_type( const wp_rigidbody_component *rc )
{
    return rc ? rc->collision_type : 0u;
}

void wp_rigidbody_component_set_collision_mask( wp_rigidbody_component *rc, wp_u32 mask )
{
    if( !rc )
        return;

    rc->collision_mask = mask;
    if( rc->body )
        wp_rigidbody_set_collision_mask( rc->body, mask );
}

wp_u32 wp_rigidbody_component_get_collision_mask( const wp_rigidbody_component *rc )
{
    return rc ? rc->collision_mask : 0u;
}

/* =========================================================================
 * Callbacks
 * ====================================================================== */

void wp_rigidbody_component_set_callbacks( wp_rigidbody_component *rc,
                                           wp_rigidbody_component_callbacks cbs )
{
    if( rc )
        rc->rb_callbacks = cbs;
}

/* =========================================================================
 * User data
 * ====================================================================== */

void *wp_rigidbody_component_get_user_data( const wp_rigidbody_component *rc )
{
    return rc ? rc->rb_user_data : NULL;
}

void wp_rigidbody_component_set_user_data( wp_rigidbody_component *rc, void *user_data )
{
    if( rc )
        rc->rb_user_data = user_data;
}
