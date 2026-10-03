/**
 * @file wp_rigidbody.h
 * @brief C API for 3D rigid bodies.
 *
 * A rigid body is a physics actor with mass, velocity and inertia that
 * participates in a physics simulation. It can be static, dynamic or
 * kinematic. Collision shapes are attached to bodies for narrow-phase
 * collision detection. Forces, torques and velocity changes can be
 * applied in several modes (force, impulse, velocity-change, acceleration).
 */

#ifndef WORKPHONE_PHYSICS_RIGIDBODY_H
#define WORKPHONE_PHYSICS_RIGIDBODY_H

#include <stdint.h>
#include "workphone_vector.h"
#include "workphone_quat.h"
#include "workphone_physics_collisionshape.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wp_rigidbody wp_rigidbody;
typedef struct wp_constraint wp_constraint;

typedef enum wp_rigidbody_type
{
    WORKPHONE_RIGIDBODY_STATIC = 0,
    WORKPHONE_RIGIDBODY_DYNAMIC = 1,
    WORKPHONE_RIGIDBODY_KINEMATIC = 2
} wp_rigidbody_type;

typedef enum wp_force_mode
{
    WORKPHONE_FORCE_MODE_FORCE = 0,
    WORKPHONE_FORCE_MODE_IMPULSE = 1,
    WORKPHONE_FORCE_MODE_VELOCITY_CHANGE = 2,
    WORKPHONE_FORCE_MODE_ACCELERATION = 5
} wp_force_mode;

enum
{
    WORKPHONE_RIGIDBODY_FLAG_ENABLED = ( 1u << 0 ),
    WORKPHONE_RIGIDBODY_FLAG_GRAVITY = ( 1u << 1 ),
    WORKPHONE_RIGIDBODY_FLAG_CCD = ( 1u << 2 ),
    WORKPHONE_RIGIDBODY_FLAG_CCD_FRICTION = ( 1u << 3 ),
    WORKPHONE_RIGIDBODY_FLAG_SLEEP_NOTIFY = ( 1u << 4 )
};

#ifndef WP_RIGIDBODY_MAX_SHAPES
#    define WP_RIGIDBODY_MAX_SHAPES 16
#endif

#ifndef WP_RIGIDBODY_MAX_CONSTRAINTS
#    define WP_RIGIDBODY_MAX_CONSTRAINTS 64
#endif

/* Lifecycle */
wp_rigidbody *wp_rigidbody_create( wp_rigidbody_type type );
void wp_rigidbody_destroy( wp_rigidbody *body );
wp_rigidbody_type wp_rigidbody_get_type( const wp_rigidbody *body );
void wp_rigidbody_set_type( wp_rigidbody *body, wp_rigidbody_type type );

/* Transform */
wp_vec3f wp_rigidbody_get_position( const wp_rigidbody *body );
void wp_rigidbody_set_position( wp_rigidbody *body, wp_vec3f position );
wp_quatf wp_rigidbody_get_orientation( const wp_rigidbody *body );
void wp_rigidbody_set_orientation( wp_rigidbody *body, wp_quatf orientation );

/* Mass and inertia */
wp_f32 wp_rigidbody_get_mass( const wp_rigidbody *body );
void wp_rigidbody_set_mass( wp_rigidbody *body, wp_f32 mass );
wp_f32 wp_rigidbody_get_restitution( const wp_rigidbody *body );
void wp_rigidbody_set_restitution( wp_rigidbody *body, wp_f32 restitution );
wp_f32 wp_rigidbody_get_friction( const wp_rigidbody *body );
void wp_rigidbody_set_friction( wp_rigidbody *body, wp_f32 friction );

/* Data-driven material reference (per-body override resolved by name). */
void wp_rigidbody_set_material_name( wp_rigidbody *body, const wp_c8 *name );
const wp_c8 *wp_rigidbody_get_material_name( const wp_rigidbody *body );
wp_vec3f wp_rigidbody_get_inertia_tensor( const wp_rigidbody *body );
void wp_rigidbody_set_inertia_tensor( wp_rigidbody *body, wp_vec3f inertia );
wp_vec3f wp_rigidbody_get_cmass_local_position( const wp_rigidbody *body );
void wp_rigidbody_set_cmass_local_position( wp_rigidbody *body, wp_vec3f position );

/* Velocity */
wp_vec3f wp_rigidbody_get_linear_velocity( const wp_rigidbody *body );
void wp_rigidbody_set_linear_velocity( wp_rigidbody *body, wp_vec3f velocity );
wp_vec3f wp_rigidbody_get_angular_velocity( const wp_rigidbody *body );
void wp_rigidbody_set_angular_velocity( wp_rigidbody *body, wp_vec3f velocity );

/* Forces and torques */
void wp_rigidbody_add_force( wp_rigidbody *body, wp_vec3f force, wp_force_mode mode );
void wp_rigidbody_add_torque( wp_rigidbody *body, wp_vec3f torque, wp_force_mode mode );
void wp_rigidbody_clear_force( wp_rigidbody *body );
void wp_rigidbody_clear_torque( wp_rigidbody *body );
wp_vec3f wp_rigidbody_get_accumulated_force( const wp_rigidbody *body );
wp_vec3f wp_rigidbody_get_accumulated_torque( const wp_rigidbody *body );
wp_vec3f wp_rigidbody_get_accumulated_acceleration( const wp_rigidbody *body );
wp_vec3f wp_rigidbody_get_accumulated_angular_acceleration( const wp_rigidbody *body );

/* Optional per-body gravity override */
wp_vec3f wp_rigidbody_get_gravity_override( const wp_rigidbody *body );
void wp_rigidbody_set_gravity_override( wp_rigidbody *body, wp_vec3f gravity );
void wp_rigidbody_clear_gravity_override( wp_rigidbody *body );
wp_s32 wp_rigidbody_has_gravity_override( const wp_rigidbody *body );

/* Damping */
wp_f32 wp_rigidbody_get_linear_damping( const wp_rigidbody *body );
void wp_rigidbody_set_linear_damping( wp_rigidbody *body, wp_f32 damping );
wp_f32 wp_rigidbody_get_angular_damping( const wp_rigidbody *body );
void wp_rigidbody_set_angular_damping( wp_rigidbody *body, wp_f32 damping );
wp_f32 wp_rigidbody_get_max_angular_velocity( const wp_rigidbody *body );
void wp_rigidbody_set_max_angular_velocity( wp_rigidbody *body, wp_f32 max_vel );

/* Sleep */
wp_s32 wp_rigidbody_is_sleeping( const wp_rigidbody *body );
void wp_rigidbody_wake_up( wp_rigidbody *body );
void wp_rigidbody_put_to_sleep( wp_rigidbody *body );
wp_f32 wp_rigidbody_get_sleep_threshold( const wp_rigidbody *body );
void wp_rigidbody_set_sleep_threshold( wp_rigidbody *body, wp_f32 threshold );

/* Shape management */
wp_s32 wp_rigidbody_add_shape( wp_rigidbody *body, wp_collision_shape *shape );
void wp_rigidbody_remove_shape( wp_rigidbody *body, wp_s32 index );
wp_collision_shape *wp_rigidbody_get_shape( const wp_rigidbody *body, wp_s32 index );
wp_s32 wp_rigidbody_get_shape_count( const wp_rigidbody *body );

/*
 * Constraint references are maintained automatically by wp_constraint.
 * These functions are public so custom scene implementations can enumerate
 * joints without depending on the rigid body's private representation.
 */
wp_s32 wp_rigidbody_add_constraint_reference( wp_rigidbody *body, wp_constraint *constraint );
void wp_rigidbody_remove_constraint_reference( wp_rigidbody *body, wp_constraint *constraint );
wp_constraint *wp_rigidbody_get_constraint( const wp_rigidbody *body, wp_s32 index );
wp_s32 wp_rigidbody_get_constraint_count( const wp_rigidbody *body );

/* Flags */
wp_u32 wp_rigidbody_get_flags( const wp_rigidbody *body );
void wp_rigidbody_set_flags( wp_rigidbody *body, wp_u32 flags );
void wp_rigidbody_set_flag( wp_rigidbody *body, wp_u32 flag, wp_s32 enabled );
wp_s32 wp_rigidbody_has_flag( const wp_rigidbody *body, wp_u32 flag );

/* Collision filtering */
wp_u32 wp_rigidbody_get_collision_type( const wp_rigidbody *body );
void wp_rigidbody_set_collision_type( wp_rigidbody *body, wp_u32 type );
wp_u32 wp_rigidbody_get_collision_mask( const wp_rigidbody *body );
void wp_rigidbody_set_collision_mask( wp_rigidbody *body, wp_u32 mask );

/* Native / user data */
void *wp_rigidbody_get_native( const wp_rigidbody *body );
void wp_rigidbody_set_native( wp_rigidbody *body, void *native );
void *wp_rigidbody_get_user_data( const wp_rigidbody *body );
void wp_rigidbody_set_user_data( wp_rigidbody *body, void *user_data );

#ifdef __cplusplus
}
#endif

#endif
