/**
 * @file wp_physics_constraint.c
 * @brief Implementation of the C physics constraint API.
 */

#include "workphone_physics_constraint.h"
#include "workphone_physics_rigidbody.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal structure
 * ====================================================================== */

typedef struct wp_constraint
{
    wp_constraint_type type;
    wp_u32 flags;

    /* Bodies */
    wp_rigidbody *body_a;
    wp_rigidbody *body_b;

    /* Local poses (actor 0 and 1) */
    wp_vec3f local_pos[2];
    wp_quatf local_ori[2];

    /* Break thresholds */
    wp_f32 break_force;
    wp_f32 break_torque;

    /* Projection tolerances */
    wp_f32 proj_linear_tol;
    wp_f32 proj_angular_tol;

    /* D6-specific */
    wp_d6_motion motion[WORKPHONE_D6_AXIS_COUNT];
    wp_constraint_drive_desc drives[WORKPHONE_D6_DRIVE_COUNT];
    wp_vec3f drive_pos;
    wp_quatf drive_ori;
    wp_constraint_linear_limit linear_limit;

    /* Opaque pointers */
    void *native;
    void *user_data;
} wp_constraint;

static wp_vec3f vec3f_zero( void )
{
    wp_vec3f v;
    memset( &v, 0, sizeof( v ) );
    return v;
}

static wp_quatf quatf_identity( void )
{
    wp_quatf q;
    memset( &q, 0, sizeof( q ) );
    q.w = 1.0f;
    return q;
}

static wp_f32 constraint_maxf( wp_f32 a, wp_f32 b )
{
    return a > b ? a : b;
}

static wp_f32 constraint_clampf( wp_f32 value, wp_f32 minimum, wp_f32 maximum )
{
    return value < minimum ? minimum : ( value > maximum ? maximum : value );
}

static wp_quatf quatf_normalize_constraint( wp_quatf q )
{
    wp_f32 length = sqrtf( q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w );
    if( length <= 1.0e-8f )
    {
        return quatf_identity();
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

wp_constraint *wp_constraint_create( wp_constraint_type type )
{
    wp_s32 i;
    wp_constraint *con = (wp_constraint *)malloc( sizeof( wp_constraint ) );
    if( !con )
    {
        return NULL;
    }

    if( type != WORKPHONE_CONSTRAINT_FIXED && type != WORKPHONE_CONSTRAINT_D6 )
    {
        free( con );
        return NULL;
    }

    memset( con, 0, sizeof( wp_constraint ) );
    con->type = type;

    /* Identity orientations for both actor local poses */
    con->local_ori[0].w = 1.0f;
    con->local_ori[1].w = 1.0f;

    /* Default break thresholds: unbreakable */
    con->break_force = 3.402823466e+38f;
    con->break_torque = 3.402823466e+38f;

    /* Default projection tolerances */
    con->proj_linear_tol = 1.0e10f;
    con->proj_angular_tol = 3.14159265f;

    /* D6 defaults: all axes locked */
    for( i = 0; i < WORKPHONE_D6_AXIS_COUNT; ++i )
    {
        con->motion[i] = WORKPHONE_D6_MOTION_LOCKED;
    }

    /* D6 drive defaults */
    for( i = 0; i < WORKPHONE_D6_DRIVE_COUNT; ++i )
    {
        con->drives[i].force_limit = 3.402823466e+38f;
    }

    /* Drive orientation identity */
    con->drive_ori.w = 1.0f;

    return con;
}

void wp_constraint_destroy( wp_constraint *con )
{
    if( !con )
    {
        return;
    }

    wp_constraint_set_body_a( con, NULL );
    wp_constraint_set_body_b( con, NULL );
    free( con );
}

wp_constraint_type wp_constraint_get_type( const wp_constraint *con )
{
    if( !con )
    {
        return WORKPHONE_CONSTRAINT_FIXED;
    }
    return con->type;
}

/* =========================================================================
 * Body references
 * ====================================================================== */

wp_rigidbody *wp_constraint_get_body_a( const wp_constraint *con )
{
    if( !con )
    {
        return NULL;
    }
    return con->body_a;
}

void wp_constraint_set_body_a( wp_constraint *con, wp_rigidbody *body )
{
    if( !con )
    {
        return;
    }
    if( body == con->body_b )
    {
        body = NULL;
    }
    if( con->body_a == body )
    {
        return;
    }

    wp_rigidbody_remove_constraint_reference( con->body_a, con );
    con->body_a = body;
    if( body && !wp_rigidbody_add_constraint_reference( body, con ) )
    {
        con->body_a = NULL;
    }
}

wp_rigidbody *wp_constraint_get_body_b( const wp_constraint *con )
{
    if( !con )
    {
        return NULL;
    }
    return con->body_b;
}

void wp_constraint_set_body_b( wp_constraint *con, wp_rigidbody *body )
{
    if( !con )
    {
        return;
    }
    if( body == con->body_a )
    {
        body = NULL;
    }
    if( con->body_b == body )
    {
        return;
    }

    wp_rigidbody_remove_constraint_reference( con->body_b, con );
    con->body_b = body;
    if( body && !wp_rigidbody_add_constraint_reference( body, con ) )
    {
        con->body_b = NULL;
    }
}

/* =========================================================================
 * Local poses
 * ====================================================================== */

wp_vec3f wp_constraint_get_local_position( const wp_constraint *con, wp_s32 actor )
{
    if( !con || actor < 0 || actor > 1 )
    {
        return vec3f_zero();
    }
    return con->local_pos[actor];
}

void wp_constraint_set_local_position( wp_constraint *con, wp_s32 actor, wp_vec3f pos )
{
    if( !con || actor < 0 || actor > 1 )
    {
        return;
    }
    con->local_pos[actor] = pos;
}

wp_quatf wp_constraint_get_local_orientation( const wp_constraint *con, wp_s32 actor )
{
    if( !con || actor < 0 || actor > 1 )
    {
        return quatf_identity();
    }
    return con->local_ori[actor];
}

void wp_constraint_set_local_orientation( wp_constraint *con, wp_s32 actor, wp_quatf ori )
{
    if( !con || actor < 0 || actor > 1 )
    {
        return;
    }
    con->local_ori[actor] = quatf_normalize_constraint( ori );
}

/* =========================================================================
 * Flags
 * ====================================================================== */

wp_u32 wp_constraint_get_flags( const wp_constraint *con )
{
    if( !con )
    {
        return 0;
    }
    return con->flags;
}

void wp_constraint_set_flags( wp_constraint *con, wp_u32 flags )
{
    if( !con )
    {
        return;
    }
    con->flags = flags;
}

void wp_constraint_set_flag( wp_constraint *con, wp_u32 flag, wp_s32 enabled )
{
    if( !con )
    {
        return;
    }
    if( enabled )
    {
        con->flags |= flag;
    }
    else
    {
        con->flags &= ~flag;
    }
}

wp_s32 wp_constraint_has_flag( const wp_constraint *con, wp_u32 flag )
{
    if( !con )
    {
        return 0;
    }
    return ( con->flags & flag ) != 0;
}

/* =========================================================================
 * Break force / torque
 * ====================================================================== */

wp_f32 wp_constraint_get_break_force( const wp_constraint *con )
{
    if( !con )
    {
        return 0.0f;
    }
    return con->break_force;
}

wp_f32 wp_constraint_get_break_torque( const wp_constraint *con )
{
    if( !con )
    {
        return 0.0f;
    }
    return con->break_torque;
}

void wp_constraint_set_break_force( wp_constraint *con, wp_f32 force, wp_f32 torque )
{
    if( !con )
    {
        return;
    }
    con->break_force = isfinite( force ) ? constraint_maxf( force, 0.0f ) : 3.402823466e+38f;
    con->break_torque = isfinite( torque ) ? constraint_maxf( torque, 0.0f ) : 3.402823466e+38f;
}

/* =========================================================================
 * Projection tolerances
 * ====================================================================== */

wp_f32 wp_constraint_get_projection_linear_tolerance( const wp_constraint *con )
{
    if( !con )
    {
        return 0.0f;
    }
    return con->proj_linear_tol;
}

void wp_constraint_set_projection_linear_tolerance( wp_constraint *con, wp_f32 tol )
{
    if( !con )
    {
        return;
    }
    con->proj_linear_tol = isfinite( tol ) ? constraint_maxf( tol, 0.0f ) : 1.0e10f;
}

wp_f32 wp_constraint_get_projection_angular_tolerance( const wp_constraint *con )
{
    if( !con )
    {
        return 0.0f;
    }
    return con->proj_angular_tol;
}

void wp_constraint_set_projection_angular_tolerance( wp_constraint *con, wp_f32 tol )
{
    if( !con )
    {
        return;
    }
    con->proj_angular_tol = isfinite( tol ) ? constraint_clampf( tol, 0.0f, 3.14159265f ) : 3.14159265f;
}

/* =========================================================================
 * D6 - Per-axis motion type
 * ====================================================================== */

wp_d6_motion wp_constraint_get_motion( const wp_constraint *con, wp_d6_axis axis )
{
    if( !con || axis < 0 || axis >= WORKPHONE_D6_AXIS_COUNT )
    {
        return WORKPHONE_D6_MOTION_LOCKED;
    }
    return con->motion[axis];
}

void wp_constraint_set_motion( wp_constraint *con, wp_d6_axis axis, wp_d6_motion type )
{
    if( !con || axis < 0 || axis >= WORKPHONE_D6_AXIS_COUNT )
    {
        return;
    }
    if( type < WORKPHONE_D6_MOTION_LOCKED || type > WORKPHONE_D6_MOTION_FREE )
    {
        type = WORKPHONE_D6_MOTION_LOCKED;
    }
    con->motion[axis] = type;
}

/* =========================================================================
 * D6 - Drive
 * ====================================================================== */

wp_constraint_drive_desc wp_constraint_get_drive( const wp_constraint *con, wp_d6_drive index )
{
    wp_constraint_drive_desc zero;
    memset( &zero, 0, sizeof( zero ) );
    if( !con || index < 0 || index >= WORKPHONE_D6_DRIVE_COUNT )
    {
        return zero;
    }
    return con->drives[index];
}

void wp_constraint_set_drive( wp_constraint *con, wp_d6_drive index, wp_constraint_drive_desc desc )
{
    if( !con || index < 0 || index >= WORKPHONE_D6_DRIVE_COUNT )
    {
        return;
    }
    desc.stiffness = isfinite( desc.stiffness ) ? constraint_maxf( desc.stiffness, 0.0f ) : 0.0f;
    desc.damping = isfinite( desc.damping ) ? constraint_maxf( desc.damping, 0.0f ) : 0.0f;
    desc.force_limit =
        isfinite( desc.force_limit ) ? constraint_maxf( desc.force_limit, 0.0f ) : 3.402823466e+38f;
    desc.is_acceleration = desc.is_acceleration ? 1 : 0;
    con->drives[index] = desc;
}

wp_vec3f wp_constraint_get_drive_position( const wp_constraint *con )
{
    if( !con )
    {
        return vec3f_zero();
    }
    return con->drive_pos;
}

void wp_constraint_set_drive_position( wp_constraint *con, wp_vec3f pos )
{
    if( !con )
    {
        return;
    }
    con->drive_pos = pos;
}

wp_quatf wp_constraint_get_drive_orientation( const wp_constraint *con )
{
    if( !con )
    {
        return quatf_identity();
    }
    return con->drive_ori;
}

void wp_constraint_set_drive_orientation( wp_constraint *con, wp_quatf ori )
{
    if( !con )
    {
        return;
    }
    con->drive_ori = quatf_normalize_constraint( ori );
}

/* =========================================================================
 * D6 - Linear limit
 * ====================================================================== */

wp_constraint_linear_limit wp_constraint_get_linear_limit( const wp_constraint *con )
{
    wp_constraint_linear_limit zero;
    memset( &zero, 0, sizeof( zero ) );
    if( !con )
    {
        return zero;
    }
    return con->linear_limit;
}

void wp_constraint_set_linear_limit( wp_constraint *con, wp_constraint_linear_limit limit )
{
    if( !con )
    {
        return;
    }
    limit.value = isfinite( limit.value ) ? constraint_maxf( limit.value, 0.0f ) : 0.0f;
    limit.restitution =
        isfinite( limit.restitution ) ? constraint_clampf( limit.restitution, 0.0f, 1.0f ) : 0.0f;
    limit.bounce_threshold =
        isfinite( limit.bounce_threshold ) ? constraint_maxf( limit.bounce_threshold, 0.0f ) : 0.0f;
    limit.stiffness = isfinite( limit.stiffness ) ? constraint_maxf( limit.stiffness, 0.0f ) : 0.0f;
    limit.damping = isfinite( limit.damping ) ? constraint_maxf( limit.damping, 0.0f ) : 0.0f;
    limit.contact_distance =
        isfinite( limit.contact_distance ) ? constraint_maxf( limit.contact_distance, 0.0f ) : 0.0f;
    con->linear_limit = limit;
}

/* =========================================================================
 * Native / user data
 * ====================================================================== */

void *wp_constraint_get_native( const wp_constraint *con )
{
    if( !con )
    {
        return NULL;
    }
    return con->native;
}

void wp_constraint_set_native( wp_constraint *con, void *native )
{
    if( !con )
    {
        return;
    }
    con->native = native;
}

void *wp_constraint_get_user_data( const wp_constraint *con )
{
    if( !con )
    {
        return NULL;
    }
    return con->user_data;
}

void wp_constraint_set_user_data( wp_constraint *con, void *user_data )
{
    if( !con )
    {
        return;
    }
    con->user_data = user_data;
}
