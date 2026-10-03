/**
 * @file workphone_vechicle_advanced_helicopter.c
 * @brief C99 implementation of the advanced helicopter flight-dynamics
 *        controller.
 */

#include "workphone_vechicle_advanced_helicopter.h"
#include "workphone_math.h"
#include "workphone_vector.h"
#include "workphone_quat.h"

/* =========================================================================
 * Internal axis helpers
 * ====================================================================== */

static wp_vec3f wp_aheli_right( const wp_advanced_helicopter *h )
{
    return wp_quatf_rotate_vec3( h->orientation, wp_vec3f_make( 1.0f, 0.0f, 0.0f ) );
}

static wp_vec3f wp_aheli_up( const wp_advanced_helicopter *h )
{
    return wp_quatf_rotate_vec3( h->orientation, wp_vec3f_make( 0.0f, 1.0f, 0.0f ) );
}

static wp_vec3f wp_aheli_forward( const wp_advanced_helicopter *h )
{
    return wp_quatf_rotate_vec3( h->orientation, wp_vec3f_make( 0.0f, 0.0f, 1.0f ) );
}

/* =========================================================================
 * Aerodynamic modifier helpers
 * ====================================================================== */

/* =============================
 * GROUND EFFECT
 * ============================= */
static wp_f32 wp_aheli_ground_effect( const wp_advanced_helicopter *h )
{
    if( h->ground_distance < h->ground_effect_height )
    {
        return 1.0f + ( 1.0f - h->ground_distance / h->ground_effect_height );
    }

    return 1.0f;
}

/* =============================
 * TRANSLATIONAL LIFT (ETL)
 * ============================= */
static wp_f32 wp_aheli_translational_lift( const wp_advanced_helicopter *h )
{
    wp_f32 speed = wp_vec3f_length( h->velocity );

    if( speed < h->etl_start_speed )
        return 1.0f;

    /* Mathf.InverseLerp(ETLStartSpeed, ETLStartSpeed * 3, speed)
     * = clamp((speed - start) / (start*3 - start), 0, 1)
     * = clamp((speed - start) / (start * 2), 0, 1)           */
    wp_f32 t = wp_clampf( ( speed - h->etl_start_speed ) / ( h->etl_start_speed * 2.0f ), 0.0f, 1.0f );

    return wp_lerpf( 1.0f, h->etl_max_boost, t );
}

/* =========================================================================
 * Internal sub-steps (called by wp_advanced_helicopter_fixed_update)
 * ====================================================================== */

/* =============================
 * ROTOR RPM PHYSICS
 * ============================= */
static void wp_aheli_update_rotor_physics( wp_advanced_helicopter *h, wp_f32 dt )
{
    h->engine_torque = h->throttle * h->max_engine_torque;

    wp_f32 drag_torque = h->rotor_drag * h->rotor_rpm;
    wp_f32 net_torque = h->engine_torque - drag_torque;
    wp_f32 angular_accel = net_torque / h->rotor_inertia;

    h->rotor_rpm = wp_clampf( h->rotor_rpm + angular_accel * dt * 60.0f, 0.0f, h->max_rotor_rpm );
}

/* =============================
 * LIFT + CYCLIC DISC TILT
 * ============================= */
static void wp_aheli_apply_lift( wp_advanced_helicopter *h )
{
    wp_f32 pitch_angle = h->collective * h->max_collective_pitch;

    /* rps = rotor_rpm / 60; lift proportional to rps^2 = Mathf.Pow(rps, 2) */
    wp_f32 rps = h->rotor_rpm / 60.0f;

    wp_f32 lift = 0.5f * h->air_density * h->rotor_area * ( rps * rps ) * h->lift_coefficient *
                  WORKPHONE_DEG2RAD_F * pitch_angle;

    lift *= wp_aheli_ground_effect( h );
    lift *= wp_aheli_translational_lift( h );

    /* Disc normal: up axis tilted by cyclic input */
    wp_vec3f disc_tilt = wp_vec3f_add(
        wp_vec3f_add( wp_aheli_up( h ),
                      wp_vec3f_scale( wp_aheli_forward( h ),
                                      h->pitch * WORKPHONE_DEG2RAD_F * h->cyclic_tilt_angle ) ),
        wp_vec3f_scale( wp_aheli_right( h ), h->roll * WORKPHONE_DEG2RAD_F * h->cyclic_tilt_angle ) );

    h->net_force = wp_vec3f_add( h->net_force, wp_vec3f_scale( wp_vec3f_normalize( disc_tilt ), lift ) );

    /* Torque reaction from main rotor */
    h->net_torque = wp_vec3f_add( h->net_torque, wp_vec3f_scale( wp_aheli_up( h ), -lift * 0.02f ) );
}

/* =============================
 * TAIL ROTOR ANTI-TORQUE
 * ============================= */
static void wp_aheli_apply_tail_rotor( wp_advanced_helicopter *h )
{
    wp_f32 anti_torque = h->rotor_rpm * 0.5f;
    wp_f32 yaw_control = h->yaw * h->tail_rotor_force;
    wp_vec3f torque = wp_vec3f_scale( wp_aheli_up( h ), yaw_control + anti_torque );

    h->net_torque = wp_vec3f_add( h->net_torque, torque );
}

/* =========================================================================
 * Public API
 * ====================================================================== */

void wp_advanced_helicopter_init( wp_advanced_helicopter *h )
{
    /* Rotor physics */
    h->rotor_inertia = 120.0f;
    h->max_engine_torque = 5000.0f;
    h->rotor_drag = 15.0f;
    h->max_rotor_rpm = 450.0f;

    /* Lift */
    h->rotor_area = 120.0f;
    h->air_density = 1.225f;
    h->lift_coefficient = 0.6f;

    /* Blade pitch */
    h->max_collective_pitch = 15.0f;
    h->cyclic_tilt_angle = 10.0f;

    /* Tail rotor */
    h->tail_rotor_force = 2000.0f;

    /* Ground effect */
    h->ground_effect_height = 10.0f;

    /* ETL */
    h->etl_start_speed = 8.0f;
    h->etl_max_boost = 1.35f;

    /* Runtime state */
    h->rotor_rpm = 0.0f;
    h->engine_torque = 0.0f;

    /* Inputs */
    h->throttle = 0.0f;
    h->collective = 0.0f;
    h->pitch = 0.0f;
    h->roll = 0.0f;
    h->yaw = 0.0f;

    /* Rigid-body state */
    h->position = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    h->orientation = wp_quatf_identity();
    h->velocity = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    h->ground_distance = WP_ADVANCED_HELI_NO_GROUND;

    /* Outputs */
    h->net_force = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    h->net_torque = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
}

wp_vec3f wp_advanced_helicopter_get_com_offset( const wp_advanced_helicopter *h )
{
    (void)h;
    return wp_vec3f_make( 0.0f, -0.7f, 0.0f );
}

void wp_advanced_helicopter_fixed_update( wp_advanced_helicopter *h, wp_f32 dt )
{
    /* Clear accumulated outputs for this tick */
    h->net_force = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    h->net_torque = wp_vec3f_make( 0.0f, 0.0f, 0.0f );

    wp_aheli_update_rotor_physics( h, dt );
    wp_aheli_apply_lift( h );
    wp_aheli_apply_tail_rotor( h );
}

void wp_advanced_helicopter_set_inputs( wp_advanced_helicopter *h, wp_f32 throttle, wp_f32 collective,
                                        wp_f32 pitch, wp_f32 roll, wp_f32 yaw )
{
    h->throttle = wp_clampf( throttle, 0.0f, 1.0f );
    h->collective = wp_clampf( collective, -1.0f, 1.0f );
    h->pitch = wp_clampf( pitch, -1.0f, 1.0f );
    h->roll = wp_clampf( roll, -1.0f, 1.0f );
    h->yaw = wp_clampf( yaw, -1.0f, 1.0f );
}

void wp_advanced_helicopter_set_state( wp_advanced_helicopter *h, wp_vec3f position,
                                       wp_quatf orientation, wp_vec3f velocity, wp_f32 ground_dist )
{
    h->position = position;
    h->orientation = orientation;
    h->velocity = velocity;
    h->ground_distance = ground_dist;
}

wp_f32 wp_advanced_helicopter_get_rotor_rpm( const wp_advanced_helicopter *h )
{
    return h->rotor_rpm;
}

wp_f32 wp_advanced_helicopter_get_engine_torque( const wp_advanced_helicopter *h )
{
    return h->engine_torque;
}
