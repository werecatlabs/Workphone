/**
 * @file workphone_vehicle_dcs_helicopter.c
 * @brief C99 implementation of the DCS-inspired helicopter flight-dynamics
 *        controller.
 */

#include "workphone_vehicle_dcs_helicopter.h"
#include "workphone_math.h"
#include "workphone_vector.h"
#include "workphone_quat.h"

/* =========================================================================
 * Internal axis helpers
 * ====================================================================== */

static wp_vec3f wp_dcs_heli_right( const wp_dcs_helicopter *h )
{
    return wp_quatf_rotate_vec3( h->orientation, wp_vec3f_make( 1.0f, 0.0f, 0.0f ) );
}

static wp_vec3f wp_dcs_heli_up( const wp_dcs_helicopter *h )
{
    return wp_quatf_rotate_vec3( h->orientation, wp_vec3f_make( 0.0f, 1.0f, 0.0f ) );
}

static wp_vec3f wp_dcs_heli_forward( const wp_dcs_helicopter *h )
{
    return wp_quatf_rotate_vec3( h->orientation, wp_vec3f_make( 0.0f, 0.0f, 1.0f ) );
}

/**
 * @brief Transform a world-space direction into the helicopter's local space.
 *
 * Equivalent to Unity's Transform.InverseTransformDirection: rotates the
 * input vector by the conjugate (inverse) of the orientation quaternion.
 */
static wp_vec3f wp_dcs_heli_to_local( const wp_dcs_helicopter *h, wp_vec3f world_dir )
{
    return wp_quatf_rotate_vec3( wp_quatf_conjugate( h->orientation ), world_dir );
}

/* =========================================================================
 * Aerodynamic modifier helpers
 * ====================================================================== */

/* ======================
 * TRANSLATIONAL LIFT
 * ====================== */
static wp_f32 wp_dcs_heli_translational_lift( const wp_dcs_helicopter *h )
{
    wp_f32 speed = wp_vec3f_length( h->velocity );

    if( speed < 10.0f )
        return 1.0f;

    wp_f32 t = wp_clampf( ( speed - 10.0f ) / 25.0f, 0.0f, 1.0f );
    return wp_lerpf( 1.0f, 1.4f, t );
}

/* ======================
 * RETREATING BLADE STALL
 * ====================== */
static wp_f32 wp_dcs_heli_retreating_blade_stall( const wp_dcs_helicopter *h )
{
    wp_f32 forward_speed = wp_vec3f_dot( h->velocity, wp_dcs_heli_forward( h ) );

    if( forward_speed < 60.0f )
        return 1.0f;

    wp_f32 t = wp_clampf( ( forward_speed - 60.0f ) / 40.0f, 0.0f, 1.0f );
    return wp_lerpf( 1.0f, 0.5f, t );
}

/* =========================================================================
 * Internal sub-steps (called by wp_dcs_helicopter_fixed_update)
 * ====================================================================== */

/* ======================
 * ROTOR RPM PHYSICS
 * ====================== */
static void wp_dcs_heli_update_rotor_rpm( wp_dcs_helicopter *h, wp_f32 dt )
{
    wp_f32 engine_torque = h->throttle * h->max_engine_torque;
    wp_f32 drag_torque = h->rotor_drag * h->rotor_rpm;
    wp_f32 net_torque = engine_torque - drag_torque;
    wp_f32 accel = net_torque / h->rotor_inertia;

    h->rotor_rpm = wp_clampf( h->rotor_rpm + accel * dt * 60.0f, 0.0f, h->max_rpm );
}

/* ======================
 * DYNAMIC INFLOW MODEL
 * ====================== */
static void wp_dcs_heli_update_dynamic_inflow( wp_dcs_helicopter *h, wp_f32 dt )
{
    wp_f32 tip_speed = h->rotor_rpm * 2.0f * WORKPHONE_PI_F * h->rotor_radius / 60.0f;
    wp_f32 collective_rad = h->collective * WORKPHONE_DEG2RAD_F * h->max_collective_pitch;

    wp_f32 thrust_estimate =
        0.5f * h->air_density * h->rotor_area * tip_speed * tip_speed * collective_rad;

    /* Momentum-theory target induced velocity */
    wp_f32 target_induced =
        wp_sqrtf( wp_absf( thrust_estimate ) / ( 2.0f * h->air_density * h->rotor_area ) );

    /* Lagged response — real rotor inflow builds up over time */
    h->induced_velocity = wp_lerpf( h->induced_velocity, target_induced, dt * h->inflow_lag );
}

/* ======================
 * BLADE FLAPPING MODEL
 * ====================== */
static void wp_dcs_heli_update_blade_flapping( wp_dcs_helicopter *h, wp_f32 dt )
{
    /* Convert world velocity to local space: InverseTransformDirection */
    wp_vec3f local_vel = wp_dcs_heli_to_local( h, h->velocity );

    /* Lateral airspeed cross-couples into pitch/roll disc tilt */
    wp_vec3f target_flap = wp_vec3f_make( -local_vel.z * 0.05f, 0.0f, local_vel.x * 0.05f );

    h->flapping_offset = wp_vec3f_lerp( h->flapping_offset, target_flap, dt * h->flapping_stiffness );
}

/* ======================
 * MAIN ROTOR FORCES
 * ====================== */
static void wp_dcs_heli_apply_rotor_forces( wp_dcs_helicopter *h )
{
    /* Momentum-theory thrust: T = 2 * rho * A * vi * (vi - vy) */
    wp_f32 thrust = 2.0f * h->air_density * h->rotor_area * h->induced_velocity *
                    ( h->induced_velocity - h->velocity.y );

    thrust *= wp_dcs_heli_translational_lift( h );
    thrust *= wp_dcs_heli_retreating_blade_stall( h );

    /* Disc normal: up axis tilted by cyclic input and blade flapping */
    wp_vec3f disc_normal = wp_vec3f_add(
        wp_vec3f_add(
            wp_dcs_heli_up( h ),
            wp_vec3f_add( wp_vec3f_scale( wp_dcs_heli_forward( h ),
                                          h->pitch * WORKPHONE_DEG2RAD_F * h->cyclic_max_tilt ),
                          wp_vec3f_scale( wp_dcs_heli_right( h ),
                                          h->roll * WORKPHONE_DEG2RAD_F * h->cyclic_max_tilt ) ) ),
        h->flapping_offset );

    h->net_force =
        wp_vec3f_add( h->net_force, wp_vec3f_scale( wp_vec3f_normalize( disc_normal ), thrust ) );

    /* Torque reaction opposes the up axis */
    h->net_torque =
        wp_vec3f_add( h->net_torque, wp_vec3f_scale( wp_dcs_heli_up( h ), -thrust * 0.02f ) );
}

/* ======================
 * TAIL ROTOR
 * ====================== */
static void wp_dcs_heli_apply_tail_rotor( wp_dcs_helicopter *h )
{
    wp_f32 anti_torque = h->rotor_rpm * 0.7f;
    wp_f32 pedal = h->yaw * h->tail_rotor_power;
    wp_vec3f torque = wp_vec3f_scale( wp_dcs_heli_up( h ), anti_torque + pedal );

    h->net_torque = wp_vec3f_add( h->net_torque, torque );
}

/* ======================
 * STABILITY AUGMENTATION
 * ====================== */
static void wp_dcs_heli_apply_sas( wp_dcs_helicopter *h )
{
    /* Counter-torque proportional to angular velocity */
    h->net_torque =
        wp_vec3f_add( h->net_torque, wp_vec3f_scale( h->angular_velocity, -h->angular_damping ) );
}

/* =========================================================================
 * Public API
 * ====================================================================== */

void wp_dcs_helicopter_init( wp_dcs_helicopter *h )
{
    /* Rotor system */
    h->rotor_radius = 6.5f;
    h->rotor_inertia = 150.0f;
    h->max_engine_torque = 6500.0f;
    h->rotor_drag = 25.0f;
    h->max_rpm = 450.0f;

    /* Aerodynamics */
    h->air_density = 1.225f;

    /* Blade pitch */
    h->max_collective_pitch = 18.0f;
    h->cyclic_max_tilt = 12.0f;

    /* Dynamic inflow */
    h->inflow_lag = 2.5f;

    /* Blade flapping */
    h->flapping_stiffness = 6.0f;

    /* Tail rotor */
    h->tail_rotor_power = 3500.0f;

    /* SAS */
    h->angular_damping = 2.5f;

    /* Derived fields */
    wp_dcs_helicopter_rebuild_derived( h );

    /* Runtime state */
    h->rotor_rpm = 0.0f;
    h->induced_velocity = 0.0f;
    h->flapping_offset = wp_vec3f_make( 0.0f, 0.0f, 0.0f );

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
    h->angular_velocity = wp_vec3f_make( 0.0f, 0.0f, 0.0f );

    /* Outputs */
    h->net_force = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    h->net_torque = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
}

void wp_dcs_helicopter_rebuild_derived( wp_dcs_helicopter *h )
{
    h->rotor_area = WORKPHONE_PI_F * h->rotor_radius * h->rotor_radius;
}

wp_vec3f wp_dcs_helicopter_get_com_offset( const wp_dcs_helicopter *h )
{
    (void)h;
    return wp_vec3f_make( 0.0f, -0.9f, 0.0f );
}

void wp_dcs_helicopter_fixed_update( wp_dcs_helicopter *h, wp_f32 dt )
{
    /* Clear accumulated outputs for this tick */
    h->net_force = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    h->net_torque = wp_vec3f_make( 0.0f, 0.0f, 0.0f );

    wp_dcs_heli_update_rotor_rpm( h, dt );
    wp_dcs_heli_update_dynamic_inflow( h, dt );
    wp_dcs_heli_update_blade_flapping( h, dt );
    wp_dcs_heli_apply_rotor_forces( h );
    wp_dcs_heli_apply_tail_rotor( h );
    wp_dcs_heli_apply_sas( h );
}

void wp_dcs_helicopter_set_inputs( wp_dcs_helicopter *h, wp_f32 throttle, wp_f32 collective,
                                   wp_f32 pitch, wp_f32 roll, wp_f32 yaw )
{
    h->throttle = wp_clampf( throttle, 0.0f, 1.0f );
    h->collective = wp_clampf( collective, -1.0f, 1.0f );
    h->pitch = wp_clampf( pitch, -1.0f, 1.0f );
    h->roll = wp_clampf( roll, -1.0f, 1.0f );
    h->yaw = wp_clampf( yaw, -1.0f, 1.0f );
}

void wp_dcs_helicopter_set_state( wp_dcs_helicopter *h, wp_vec3f position, wp_quatf orientation,
                                  wp_vec3f linear_velocity, wp_vec3f angular_velocity )
{
    h->position = position;
    h->orientation = orientation;
    h->velocity = linear_velocity;
    h->angular_velocity = angular_velocity;
}

wp_f32 wp_dcs_helicopter_get_rotor_rpm( const wp_dcs_helicopter *h )
{
    return h->rotor_rpm;
}

wp_f32 wp_dcs_helicopter_get_induced_velocity( const wp_dcs_helicopter *h )
{
    return h->induced_velocity;
}

wp_vec3f wp_dcs_helicopter_get_flapping_offset( const wp_dcs_helicopter *h )
{
    return h->flapping_offset;
}
