/**
 * @file wp_physics_constraint.h
 * @brief C API for physics constraints (joints).
 *
 * A constraint connects two rigid bodies and restricts their relative
 * motion. Supported constraint types are fixed joints and 6-DOF (D6)
 * joints. A D6 joint allows per-axis motion control (locked, limited,
 * or free) and configurable spring drives on each axis.
 */

#ifndef WORKPHONE_PHYSICS_CONSTRAINT_H
#define WORKPHONE_PHYSICS_CONSTRAINT_H

#include <stdint.h>
#include "workphone_vector.h"
#include "workphone_quat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wp_constraint wp_constraint;
typedef struct wp_rigidbody wp_rigidbody;

/* -------------------------------------------------------------------------
 * Constraint type
 * ---------------------------------------------------------------------- */

typedef enum wp_constraint_type
{
    WORKPHONE_CONSTRAINT_FIXED = 0,
    WORKPHONE_CONSTRAINT_D6 = 1
} wp_constraint_type;

/* -------------------------------------------------------------------------
 * Constraint flags
 * ---------------------------------------------------------------------- */

enum
{
    WORKPHONE_CONSTRAINT_FLAG_BROKEN = ( 1u << 0 ),
    WORKPHONE_CONSTRAINT_FLAG_PROJECT_TO_ACTOR0 = ( 1u << 1 ),
    WORKPHONE_CONSTRAINT_FLAG_PROJECT_TO_ACTOR1 = ( 1u << 2 ),
    WORKPHONE_CONSTRAINT_FLAG_PROJECTION = ( 1u << 1 ) | ( 1u << 2 ),
    WORKPHONE_CONSTRAINT_FLAG_COLLISION_ENABLED = ( 1u << 3 ),
    WORKPHONE_CONSTRAINT_FLAG_REPORTING = ( 1u << 4 ),
    WORKPHONE_CONSTRAINT_FLAG_VISUALIZATION = ( 1u << 5 ),
    WORKPHONE_CONSTRAINT_FLAG_DRIVE_LIMITS_FORCE = ( 1u << 6 ),
    WORKPHONE_CONSTRAINT_FLAG_IMPROVED_SLERP = ( 1u << 8 )
};

/* -------------------------------------------------------------------------
 * D6 axis / motion / drive enums
 * ---------------------------------------------------------------------- */

typedef enum wp_d6_axis
{
    WORKPHONE_D6_AXIS_X = 0,
    WORKPHONE_D6_AXIS_Y = 1,
    WORKPHONE_D6_AXIS_Z = 2,
    WORKPHONE_D6_AXIS_TWIST = 3,
    WORKPHONE_D6_AXIS_SWING1 = 4,
    WORKPHONE_D6_AXIS_SWING2 = 5,
    WORKPHONE_D6_AXIS_COUNT = 6
} wp_d6_axis;

typedef enum wp_d6_motion
{
    WORKPHONE_D6_MOTION_LOCKED = 0,
    WORKPHONE_D6_MOTION_LIMITED = 1,
    WORKPHONE_D6_MOTION_FREE = 2
} wp_d6_motion;

typedef enum wp_d6_drive
{
    WORKPHONE_D6_DRIVE_X = 0,
    WORKPHONE_D6_DRIVE_Y = 1,
    WORKPHONE_D6_DRIVE_Z = 2,
    WORKPHONE_D6_DRIVE_SWING = 3,
    WORKPHONE_D6_DRIVE_TWIST = 4,
    WORKPHONE_D6_DRIVE_SLERP = 5,
    WORKPHONE_D6_DRIVE_COUNT = 6
} wp_d6_drive;

/* -------------------------------------------------------------------------
 * Drive descriptor (value type, not heap-allocated)
 * ---------------------------------------------------------------------- */

typedef struct wp_constraint_drive_desc
{
    wp_f32 stiffness;
    wp_f32 damping;
    wp_f32 force_limit;
    wp_s32 is_acceleration;
} wp_constraint_drive_desc;

/* -------------------------------------------------------------------------
 * Linear-limit descriptor (value type)
 * ---------------------------------------------------------------------- */

typedef struct wp_constraint_linear_limit
{
    wp_f32 value;
    wp_f32 restitution;
    wp_f32 bounce_threshold;
    wp_f32 stiffness;
    wp_f32 damping;
    wp_f32 contact_distance;
} wp_constraint_linear_limit;

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_constraint *wp_constraint_create( wp_constraint_type type );
void wp_constraint_destroy( wp_constraint *con );
wp_constraint_type wp_constraint_get_type( const wp_constraint *con );

/* =========================================================================
 * Body references
 * ====================================================================== */

wp_rigidbody *wp_constraint_get_body_a( const wp_constraint *con );
void wp_constraint_set_body_a( wp_constraint *con, wp_rigidbody *body );
wp_rigidbody *wp_constraint_get_body_b( const wp_constraint *con );
void wp_constraint_set_body_b( wp_constraint *con, wp_rigidbody *body );

/* =========================================================================
 * Local poses (actor 0 / 1)
 * ====================================================================== */

wp_vec3f wp_constraint_get_local_position( const wp_constraint *con, wp_s32 actor );
void wp_constraint_set_local_position( wp_constraint *con, wp_s32 actor, wp_vec3f pos );
wp_quatf wp_constraint_get_local_orientation( const wp_constraint *con, wp_s32 actor );
void wp_constraint_set_local_orientation( wp_constraint *con, wp_s32 actor, wp_quatf ori );

/* =========================================================================
 * Flags
 * ====================================================================== */

wp_u32 wp_constraint_get_flags( const wp_constraint *con );
void wp_constraint_set_flags( wp_constraint *con, wp_u32 flags );
void wp_constraint_set_flag( wp_constraint *con, wp_u32 flag, wp_s32 enabled );
wp_s32 wp_constraint_has_flag( const wp_constraint *con, wp_u32 flag );

/* =========================================================================
 * Break force / torque
 * ====================================================================== */

wp_f32 wp_constraint_get_break_force( const wp_constraint *con );
wp_f32 wp_constraint_get_break_torque( const wp_constraint *con );
void wp_constraint_set_break_force( wp_constraint *con, wp_f32 force, wp_f32 torque );

/* =========================================================================
 * Projection tolerances
 * ====================================================================== */

wp_f32 wp_constraint_get_projection_linear_tolerance( const wp_constraint *con );
void wp_constraint_set_projection_linear_tolerance( wp_constraint *con, wp_f32 tol );
wp_f32 wp_constraint_get_projection_angular_tolerance( const wp_constraint *con );
void wp_constraint_set_projection_angular_tolerance( wp_constraint *con, wp_f32 tol );

/* =========================================================================
 * D6 - Per-axis motion type
 * ====================================================================== */

wp_d6_motion wp_constraint_get_motion( const wp_constraint *con, wp_d6_axis axis );
void wp_constraint_set_motion( wp_constraint *con, wp_d6_axis axis, wp_d6_motion type );

/* =========================================================================
 * D6 - Drive
 * ====================================================================== */

wp_constraint_drive_desc wp_constraint_get_drive( const wp_constraint *con, wp_d6_drive index );
void wp_constraint_set_drive( wp_constraint *con, wp_d6_drive index, wp_constraint_drive_desc desc );

wp_vec3f wp_constraint_get_drive_position( const wp_constraint *con );
void wp_constraint_set_drive_position( wp_constraint *con, wp_vec3f pos );
wp_quatf wp_constraint_get_drive_orientation( const wp_constraint *con );
void wp_constraint_set_drive_orientation( wp_constraint *con, wp_quatf ori );

/* =========================================================================
 * D6 - Linear limit
 * ====================================================================== */

wp_constraint_linear_limit wp_constraint_get_linear_limit( const wp_constraint *con );
void wp_constraint_set_linear_limit( wp_constraint *con, wp_constraint_linear_limit limit );

/* =========================================================================
 * Native / user data
 * ====================================================================== */

void *wp_constraint_get_native( const wp_constraint *con );
void wp_constraint_set_native( wp_constraint *con, void *native );
void *wp_constraint_get_user_data( const wp_constraint *con );
void wp_constraint_set_user_data( wp_constraint *con, void *user_data );

#ifdef __cplusplus
}
#endif

#endif
