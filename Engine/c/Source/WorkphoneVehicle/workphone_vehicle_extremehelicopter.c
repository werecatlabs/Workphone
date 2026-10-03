/**
 * @file workphone_vehicle_extremehelicopter.c
 * @brief C99 implementation of the high-fidelity helicopter flight-dynamics
 *        controller.
 */

#include "workphone_vehicle_extremehelicopter.h"
#include "workphone_math.h"
#include "workphone_vector.h"
#include "workphone_quat.h"

/* =========================================================================
 * Internal axis helpers
 * ====================================================================== */

static wp_vec3f wp_xheli_right( const wp_extreme_helicopter *h )
{
    return wp_quatf_rotate_vec3( h->orientation, wp_vec3f_make( 1.0f, 0.0f, 0.0f ) );
}

static wp_vec3f wp_xheli_up( const wp_extreme_helicopter *h )
{
    return wp_quatf_rotate_vec3( h->orientation, wp_vec3f_make( 0.0f, 1.0f, 0.0f ) );
}

static wp_vec3f wp_xheli_forward( const wp_extreme_helicopter *h )
{
    return wp_quatf_rotate_vec3( h->orientation, wp_vec3f_make( 0.0f, 0.0f, 1.0f ) );
}

/* =========================================================================
 * Aerodynamic modifier helpers
 * ====================================================================== */

/* =========================
 * GROUND EFFECT
 * ========================= */
static wp_f32 wp_xheli_ground_effect( const wp_extreme_helicopter *h )
{
    if( h->ground_distance < h->ground_effect_height )
    {
        return 1.0f + ( 1.0f - h->ground_distance / h->ground_effect_height );
    }

    return 1.0f;
}

/* =========================
 * TRANSLATIONAL LIFT
 * ========================= */
static wp_f32 wp_xheli_translational_lift( const wp_extreme_helicopter *h )
{
    wp_f32 speed = wp_vec3f_length( h->velocity );

    if( speed < 10.0f )
        return 1.0f;

    /* Lerp from 1.0 to 1.35 over the 10–30 m/s range, clamped at 1.35. */
    wp_f32 t = wp_clampf( ( speed - 10.0f ) / 20.0f, 0.0f, 1.0f );
    return wp_lerpf( 1.0f, 1.35f, t );
}

/* =========================
 * VORTEX RING STATE
 * ========================= */
static wp_f32 wp_xheli_vortex_ring_state( const wp_extreme_helicopter *h )
{
    /* Downward speed: dot(velocity, world_down) = dot(velocity, (0,-1,0)) */
    static const wp_vec3f world_down = { 0.0f, -1.0f, 0.0f };
    wp_f32 vertical_speed = wp_vec3f_dot( h->velocity, world_down );

    if( vertical_speed > 2.0f )
        return 1.0f;

    if( vertical_speed < h->vortex_descent_rate && h->collective > 0.3f )
    {
        return 0.4f; /* Massive lift loss */
    }

    return 1.0f;
}

/* =========================================================================
 * Internal sub-steps (called by wp_extreme_helicopter_fixed_update)
 * ====================================================================== */

/* =========================
 * ROTOR RPM DYNAMICS
 * ========================= */
static void wp_xheli_update_rotor_rpm( wp_extreme_helicopter *h, wp_f32 dt )
{
    wp_f32 engine_torque = h->throttle * h->max_engine_torque;
    wp_f32 drag_torque = h->rotor_drag_coeff * h->rotor_rpm;
    wp_f32 net_torque = engine_torque - drag_torque;
    wp_f32 angular_accel = net_torque / h->rotor_inertia;

    h->rotor_rpm = wp_clampf( h->rotor_rpm + angular_accel * dt * 60.0f, 0.0f, h->max_rpm );
}

/* =========================
 * INDUCED FLOW CALCULATION
 * ========================= */
static void wp_xheli_calculate_induced_velocity( wp_extreme_helicopter *h )
{
    wp_f32 collective_rad = h->collective * WORKPHONE_DEG2RAD_F * h->max_collective_pitch;
    wp_f32 blade_lift_coeff = h->blade_lift_slope * collective_rad;
    wp_f32 tip_speed = h->rotor_rpm * 2.0f * WORKPHONE_PI_F * h->rotor_radius / 60.0f;
    wp_f32 thrust_guess =
        0.5f * h->air_density * h->rotor_area * tip_speed * tip_speed * blade_lift_coeff;

    h->induced_velocity =
        wp_sqrtf( wp_absf( thrust_guess ) / ( 2.0f * h->air_density * h->rotor_area ) );
}

/* =========================
 * MAIN ROTOR FORCES
 * ========================= */
static void wp_xheli_apply_main_rotor( wp_extreme_helicopter *h )
{
    wp_f32 tip_speed = h->rotor_rpm * 2.0f * WORKPHONE_PI_F * h->rotor_radius / 60.0f;
    wp_f32 collective_rad = h->collective * WORKPHONE_DEG2RAD_F * h->max_collective_pitch;
    wp_f32 lift_coeff = h->blade_lift_slope * collective_rad;

    wp_f32 thrust = 0.5f * h->air_density * h->rotor_area * tip_speed * tip_speed * lift_coeff;

    thrust *= wp_xheli_ground_effect( h );
    thrust *= wp_xheli_translational_lift( h );
    thrust *= wp_xheli_vortex_ring_state( h );

    /* Cyclic input tilts the rotor disc normal */
    wp_f32 cyclic_pitch_rad = h->pitch * WORKPHONE_DEG2RAD_F * h->cyclic_tilt_max;
    wp_f32 cyclic_roll_rad = h->roll * WORKPHONE_DEG2RAD_F * h->cyclic_tilt_max;

    wp_vec3f disc_normal = wp_vec3f_add(
        wp_xheli_up( h ), wp_vec3f_add( wp_vec3f_scale( wp_xheli_forward( h ), cyclic_pitch_rad ),
                                        wp_vec3f_scale( wp_xheli_right( h ), cyclic_roll_rad ) ) );

    /* Thrust acts along the normalised disc normal */
    h->net_force =
        wp_vec3f_add( h->net_force, wp_vec3f_scale( wp_vec3f_normalize( disc_normal ), thrust ) );

    /* Rotor torque reaction opposes the up axis */
    h->net_torque = wp_vec3f_add( h->net_torque, wp_vec3f_scale( wp_xheli_up( h ), -thrust * 0.015f ) );
}

/* =========================
 * TAIL ROTOR ANTI-TORQUE
 * ========================= */
static void wp_xheli_apply_tail_rotor( wp_extreme_helicopter *h )
{
    wp_f32 anti_torque = h->rotor_rpm * 0.6f;
    wp_f32 pedal_force = h->yaw * h->tail_rotor_power;
    wp_vec3f torque = wp_vec3f_scale( wp_xheli_up( h ), anti_torque + pedal_force );

    h->net_torque = wp_vec3f_add( h->net_torque, torque );
}

/* =========================================================================
 * Public API
 * ====================================================================== */

void wp_extreme_helicopter_init( wp_extreme_helicopter *h )
{
    /* Rotor system */
    h->rotor_radius = 6.5f;
    h->rotor_inertia = 140.0f;
    h->max_engine_torque = 6000.0f;
    h->rotor_drag_coeff = 20.0f;
    h->max_rpm = 450.0f;

    /* Blade pitch */
    h->max_collective_pitch = 18.0f;
    h->cyclic_tilt_max = 12.0f;

    /* Aerodynamics */
    h->air_density = 1.225f;
    h->blade_lift_slope = 5.7f;

    /* Tail rotor */
    h->tail_rotor_power = 3000.0f;

    /* Ground effect */
    h->ground_effect_height = 10.0f;

    /* Vortex ring state */
    h->vortex_descent_rate = -5.0f;

    /* Derived fields */
    wp_extreme_helicopter_rebuild_derived( h );

    /* Runtime state */
    h->rotor_rpm = 0.0f;
    h->induced_velocity = 0.0f;

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
    h->ground_distance = WP_EXTREME_HELI_NO_GROUND;

    /* Outputs */
    h->net_force = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    h->net_torque = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
}

void wp_extreme_helicopter_rebuild_derived( wp_extreme_helicopter *h )
{
    h->rotor_area = WORKPHONE_PI_F * h->rotor_radius * h->rotor_radius;
}

wp_vec3f wp_extreme_helicopter_get_com_offset( const wp_extreme_helicopter *h )
{
    (void)h;
    return wp_vec3f_make( 0.0f, -0.8f, 0.0f );
}

void wp_extreme_helicopter_fixed_update( wp_extreme_helicopter *h, wp_f32 dt )
{
    /* Clear accumulated outputs for this tick */
    h->net_force = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    h->net_torque = wp_vec3f_make( 0.0f, 0.0f, 0.0f );

    wp_xheli_update_rotor_rpm( h, dt );
    wp_xheli_calculate_induced_velocity( h );
    wp_xheli_apply_main_rotor( h );
    wp_xheli_apply_tail_rotor( h );
}

void wp_extreme_helicopter_set_inputs( wp_extreme_helicopter *h, wp_f32 throttle, wp_f32 collective,
                                       wp_f32 pitch, wp_f32 roll, wp_f32 yaw )
{
    h->throttle = wp_clampf( throttle, 0.0f, 1.0f );
    h->collective = wp_clampf( collective, -1.0f, 1.0f );
    h->pitch = wp_clampf( pitch, -1.0f, 1.0f );
    h->roll = wp_clampf( roll, -1.0f, 1.0f );
    h->yaw = wp_clampf( yaw, -1.0f, 1.0f );
}

void wp_extreme_helicopter_set_state( wp_extreme_helicopter *h, wp_vec3f position, wp_quatf orientation,
                                      wp_vec3f velocity, wp_f32 ground_dist )
{
    h->position = position;
    h->orientation = orientation;
    h->velocity = velocity;
    h->ground_distance = ground_dist;
}

wp_f32 wp_extreme_helicopter_get_rotor_rpm( const wp_extreme_helicopter *h )
{
    return h->rotor_rpm;
}

wp_f32 wp_extreme_helicopter_get_induced_velocity( const wp_extreme_helicopter *h )
{
    return h->induced_velocity;
}
