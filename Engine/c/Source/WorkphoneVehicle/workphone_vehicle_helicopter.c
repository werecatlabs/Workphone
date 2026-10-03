/**
 * @file workphone_vehicle_helicopter.c
 * @brief C99 implementation of the helicopter flight-dynamics controller.
 */

#include "workphone_vehicle_helicopter.h"
#include "workphone_math.h"
#include "workphone_vector.h"
#include "workphone_quat.h"

/* =========================================================================
 * Internal axis helpers
 * ====================================================================== */

/** Local right axis (1,0,0) rotated into world space by the orientation. */
static wp_vec3f wp_heli_right( const wp_helicopter *h )
{
    return wp_quatf_rotate_vec3( h->orientation, wp_vec3f_make( 1.0f, 0.0f, 0.0f ) );
}

/** Local up axis (0,1,0) rotated into world space by the orientation. */
static wp_vec3f wp_heli_up( const wp_helicopter *h )
{
    return wp_quatf_rotate_vec3( h->orientation, wp_vec3f_make( 0.0f, 1.0f, 0.0f ) );
}

/** Local forward axis (0,0,1) rotated into world space by the orientation. */
static wp_vec3f wp_heli_forward( const wp_helicopter *h )
{
    return wp_quatf_rotate_vec3( h->orientation, wp_vec3f_make( 0.0f, 0.0f, 1.0f ) );
}

/* =========================================================================
 * Internal sub-steps (called by wp_helicopter_fixed_update)
 * ====================================================================== */

/* ==========================
 * GROUND EFFECT
 * ========================== */
static wp_f32 wp_heli_ground_effect( const wp_helicopter *h )
{
    if( h->ground_distance < h->ground_effect_height )
    {
        return 1.0f + ( 1.0f - h->ground_distance / h->ground_effect_height );
    }

    return 1.0f;
}

/* ==========================
 * LIFT
 * ========================== */
static void wp_heli_apply_lift( wp_helicopter *h )
{
    wp_f32 height_factor = wp_heli_ground_effect( h );
    wp_f32 lift = h->engine_throttle * h->lift_power * height_factor;
    wp_vec3f force = wp_vec3f_scale( wp_heli_up( h ), lift );

    h->net_force = wp_vec3f_add( h->net_force, force );
}

/* ==========================
 * TILT (PITCH + ROLL)
 * ========================== */
static void wp_heli_apply_tilt( wp_helicopter *h )
{
    wp_vec3f pitch = wp_vec3f_scale( wp_heli_right( h ), -h->pitch_input * h->pitch_force );

    wp_vec3f roll = wp_vec3f_scale( wp_heli_forward( h ), h->roll_input * h->roll_force );

    h->net_torque = wp_vec3f_add( h->net_torque, wp_vec3f_add( pitch, roll ) );
}

/* ==========================
 * YAW
 * ========================== */
static void wp_heli_apply_yaw( wp_helicopter *h )
{
    wp_vec3f torque = wp_vec3f_scale( wp_heli_up( h ), h->yaw_input * h->yaw_torque );

    h->net_torque = wp_vec3f_add( h->net_torque, torque );
}

/* ==========================
 * STABILISATION
 * ========================== */
static void wp_heli_apply_stabilization( wp_helicopter *h )
{
    static const wp_vec3f world_up = { 0.0f, 1.0f, 0.0f };

    wp_vec3f torque = wp_vec3f_cross( wp_heli_up( h ), world_up );

    h->net_torque = wp_vec3f_add( h->net_torque, wp_vec3f_scale( torque, h->stabilization ) );
}

/* =========================================================================
 * Public API
 * ====================================================================== */

void wp_helicopter_init( wp_helicopter *h )
{
    /* Tunable defaults */
    h->engine_power = 30000.0f;
    h->spool_up_speed = 0.5f;
    h->lift_power = 4000.0f;
    h->ground_effect_height = 8.0f;
    h->pitch_force = 800.0f;
    h->roll_force = 800.0f;
    h->yaw_torque = 600.0f;
    h->stabilization = 3.0f;

    /* Runtime state */
    h->engine_throttle = 0.0f;

    /* Inputs */
    h->collective_input = 0.0f;
    h->pitch_input = 0.0f;
    h->roll_input = 0.0f;
    h->yaw_input = 0.0f;

    /* Transform */
    h->position = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    h->orientation = wp_quatf_identity();

    /* Environment */
    h->ground_distance = WP_HELICOPTER_NO_GROUND;

    /* Outputs */
    h->net_force = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    h->net_torque = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
}

wp_vec3f wp_helicopter_get_com_offset( const wp_helicopter *h )
{
    (void)h;
    return wp_vec3f_make( 0.0f, -0.5f, 0.0f );
}

void wp_helicopter_update( wp_helicopter *h, wp_f32 dt )
{
    /* Spool the engine throttle toward the collective demand */
    h->engine_throttle = wp_lerpf( h->engine_throttle, h->collective_input, dt * h->spool_up_speed );
}

void wp_helicopter_fixed_update( wp_helicopter *h, wp_f32 dt )
{
    (void)dt;

    /* Clear accumulated outputs for this tick */
    h->net_force = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    h->net_torque = wp_vec3f_make( 0.0f, 0.0f, 0.0f );

    wp_heli_apply_lift( h );
    wp_heli_apply_tilt( h );
    wp_heli_apply_yaw( h );
    wp_heli_apply_stabilization( h );
}

void wp_helicopter_set_inputs( wp_helicopter *h, wp_f32 collective, wp_f32 pitch, wp_f32 roll,
                               wp_f32 yaw )
{
    h->collective_input = wp_clampf( collective, 0.0f, 1.0f );
    h->pitch_input = wp_clampf( pitch, -1.0f, 1.0f );
    h->roll_input = wp_clampf( roll, -1.0f, 1.0f );
    h->yaw_input = wp_clampf( yaw, -1.0f, 1.0f );
}

void wp_helicopter_set_transform( wp_helicopter *h, wp_vec3f position, wp_quatf orientation )
{
    h->position = position;
    h->orientation = orientation;
}

void wp_helicopter_set_ground_distance( wp_helicopter *h, wp_f32 distance )
{
    h->ground_distance = distance;
}
