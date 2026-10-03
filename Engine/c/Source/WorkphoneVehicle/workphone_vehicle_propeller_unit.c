/**
 * @file workphone_vehicle_propeller_unit.c
 * @brief C99 implementation of a propeller unit with blade-element aerodynamics.
 *
 * Reproduces the following computation path:
 *
 *  1. Blade speed at 0.4 R.
 *  2. Effective pitch = geometric_pitch - (axial_flow + propwash) / blade_speed.
 *  3. Cl = cl_slope * effective_pitch, clamped to [cl_min, cl_max].
 *  4. Cd = Cd0 + ratio * Cl^2 (1.4x asymmetry for negative Cl).
 *  5. Thrust = 0.5 rho V_blade^2 Cl * blade_area.
 *  6. P-factor yaw/pitch from lateral disc flow.
 *  7. Drag-imbalance yaw/pitch from in-plane disc flow.
 *  8. Propwash via lagged momentum-theory quadratic solution.
 *  9. Drag power = drag_power_factor * Cd * w^3.
 * 10. Induced power = thrust * propwash.
 * 11. Prop torque = (P_induced + P_drag) / w.
 * 12. Rotor acceleration = (input_torque - prop_torque) / total_inertia.
 * 13. w integration with +/-4000 rad/s^2 acceleration limit.
 */

#include "workphone_vehicle_propeller_unit.h"
#include "workphone_math.h"
#include "workphone_vector.h"

#include <string.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

/**
 * @brief Recompute all fields derived from the four primary geometry inputs.
 *
 * Mirrors CAircraftPropeller::load() derived-field calculations:
 *   disc area, blade area, solidity, geometric pitch angle,
 *   and the profile-drag power factor.
 */
static void wp_pu_rebuild_derived( wp_propeller_unit *pu )
{
    wp_f32 D = pu->diameter;
    wp_f32 D2 = D * D;
    wp_f32 D4 = D2 * D2;

    /* Disc area: pi D^2 / 4 */
    pu->area = WORKPHONE_PI_F * 0.25f * D2;

    /* Blade area: blades * chord * 0.4 * D (effective blade span ~ 0.4 D) */
    pu->blade_area = (wp_f32)pu->blades * pu->chord * 0.4f * D;

    /* Solidity ratio */
    pu->solidity = pu->area > 1e-9f ? pu->blade_area / pu->area : 0.0f;

    /* Geometric pitch angle: pitch / (pi * 0.7 * D) */
    {
        wp_f32 denom = WORKPHONE_PI_F * 0.7f * D;
        pu->geometric_pitch = denom > 1e-9f ? pu->pitch / denom : 0.0f;
    }

    /* Profile-drag power factor (from CAircraftPropeller::load):
     *   blades * rho * chord * Cd_profile(0.5) * D^4 / 128              */
    {
        const wp_f32 cd_profile = 0.5f;
        pu->drag_power_factor =
            (wp_f32)pu->blades * pu->air_density * pu->chord * cd_profile * D4 / 128.0f;
    }
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

void wp_propeller_unit_init( wp_propeller_unit *pu )
{
    memset( pu, 0, sizeof( *pu ) );

    /* Geometry */
    pu->diameter = 0.28f; /* ~11 in */
    pu->pitch = 0.23f;    /* ~9 in  */
    pu->blades = 2;
    pu->chord = 0.015f;

    /* Aero coefficients */
    pu->cl_slope = 2.0f * WORKPHONE_PI_F;
    pu->cd0 = 0.01f;
    pu->dcl_dcd_ratio = 0.02f;
    pu->cl_max = 1.2f;
    pu->cl_min = -0.8f;

    /* Inertia */
    pu->rotor_inertia = 0.001f;
    pu->total_inertia = 0.002f;

    /* Propwash dynamics */
    pu->flow_settling_time = 0.1f;

    /* Environment */
    pu->air_density = 1.225f;

    /* Config */
    pu->reversed = 0;
    pu->ducted = 0;

    wp_pu_rebuild_derived( pu );
}

/* =========================================================================
 * Configuration
 * ====================================================================== */

void wp_propeller_unit_set_geometry( wp_propeller_unit *pu, wp_f32 diameter, wp_f32 pitch, wp_s32 blades,
                                     wp_f32 chord )
{
    pu->diameter = diameter;
    pu->pitch = pitch;
    pu->blades = blades;
    pu->chord = chord;

    wp_pu_rebuild_derived( pu );
}

void wp_propeller_unit_set_aerodynamics( wp_propeller_unit *pu, wp_f32 cl_slope, wp_f32 cd0,
                                         wp_f32 dcl_dcd_ratio, wp_f32 cl_max, wp_f32 cl_min )
{
    pu->cl_slope = cl_slope;
    pu->cd0 = cd0;
    pu->dcl_dcd_ratio = dcl_dcd_ratio;
    pu->cl_max = cl_max;
    pu->cl_min = cl_min;
}

void wp_propeller_unit_set_inertia( wp_propeller_unit *pu, wp_f32 rotor_inertia, wp_f32 total_inertia )
{
    pu->rotor_inertia = rotor_inertia;
    pu->total_inertia = total_inertia;
}

void wp_propeller_unit_set_environment( wp_propeller_unit *pu, wp_f32 air_density, wp_f32 settling_tc )
{
    pu->air_density = air_density;
    pu->flow_settling_time = settling_tc > 1e-6f ? settling_tc : 0.1f;

    /* Drag-power factor depends on rho, so rebuild */
    wp_pu_rebuild_derived( pu );
}

void wp_propeller_unit_set_config( wp_propeller_unit *pu, wp_s32 reversed, wp_s32 ducted )
{
    pu->reversed = reversed;
    pu->ducted = ducted;
}

/* =========================================================================
 * Per-tick inputs
 * ====================================================================== */

void wp_propeller_unit_set_rotor_state( wp_propeller_unit *pu, wp_f32 rotor_rpm, wp_f32 input_torque )
{
    pu->rotor_rpm = rotor_rpm;
    /* RPM -> rad/s : w = RPM * 2pi / 60 */
    pu->rotor_rps = rotor_rpm * WORKPHONE_TWO_PI_F / 60.0f;
    pu->input_torque = input_torque;
}

void wp_propeller_unit_set_flow( wp_propeller_unit *pu, wp_f32 flow_x, wp_f32 flow_y, wp_f32 flow_z )
{
    pu->disc_flow = wp_vec3f_make( flow_x, flow_y, flow_z );
}

/* =========================================================================
 * Core simulation
 * ====================================================================== */

void wp_propeller_unit_fixed_update( wp_propeller_unit *pu, wp_f32 dt )
{
    wp_f32 rho = pu->air_density;
    wp_f32 D = pu->diameter;
    wp_f32 blade_area = pu->blade_area;

    /* ---------------------------------------------------------------
     * 1. Effective blade speed at 40 % radius
     * --------------------------------------------------------------- */
    wp_f32 blade_speed = 0.4f * D * pu->rotor_rps;
    pu->blade_speed = blade_speed;

    /* ---------------------------------------------------------------
     * 2. Effective pitch angle
     *    = geometric_pitch - (axial_flow + propwash) / blade_speed
     * --------------------------------------------------------------- */
    wp_f32 eff_pitch = pu->geometric_pitch;
    if( blade_speed > 1e-6f )
    {
        eff_pitch = pu->geometric_pitch - ( pu->disc_flow.x + pu->propwash ) / blade_speed;
    }
    pu->effective_pitch = eff_pitch;

    /* ---------------------------------------------------------------
     * 3. Lift coefficient Cl = cl_slope * alpha, clamped
     * --------------------------------------------------------------- */
    wp_f32 cl = pu->cl_slope * eff_pitch;
    if( cl > pu->cl_max )
        cl = pu->cl_max;
    if( cl < pu->cl_min )
        cl = pu->cl_min;
    pu->cl = cl;

    /* ---------------------------------------------------------------
     * 4. Drag coefficient Cd = Cd0 + ratio * Cl^2
     *    (asymmetric: 1.4x Cl when Cl < 0 -- blade operating against
     *     camber generates higher drag)
     * --------------------------------------------------------------- */
    wp_f32 cd;
    if( cl >= 0.0f )
        cd = pu->cd0 + pu->dcl_dcd_ratio * cl * cl;
    else
    {
        wp_f32 cl_neg = 1.4f * cl;
        cd = pu->cd0 + pu->dcl_dcd_ratio * cl_neg * cl_neg;
    }
    pu->cd = cd;

    /* ---------------------------------------------------------------
     * 5. Thrust  T = 0.5 * rho * V_blade^2 * Cl * blade_area
     * --------------------------------------------------------------- */
    wp_f32 blade_speed_sq = blade_speed * blade_speed;
    wp_f32 thrust = 0.5f * rho * blade_speed_sq * cl * blade_area;
    pu->thrust_value = thrust;

    /* ---------------------------------------------------------------
     * 6. P-factor yaw and pitch  (lateral flow * Cl contribution)
     *    factor = -0.20 * D * rho * Cl * blade_area * blade_speed
     *    yaw   = factor * disc_flow.z
     *    pitch = factor * disc_flow.y
     * --------------------------------------------------------------- */
    wp_f32 pfac = -0.20f * D * rho * cl * blade_area * blade_speed;
    pu->yaw_pfac = pfac * pu->disc_flow.z;
    pu->pitch_pfac = pfac * pu->disc_flow.y;

    if( pu->reversed )
    {
        pu->yaw_pfac = -pu->yaw_pfac;
        pu->pitch_pfac = -pu->pitch_pfac;
    }

    /* ---------------------------------------------------------------
     * 7. Drag-imbalance yaw and pitch
     *    factor = -0.5 * rho * Cd * blade_area * blade_speed
     * --------------------------------------------------------------- */
    wp_f32 drag_fac = -0.5f * rho * cd * blade_area * blade_speed;
    pu->yaw_drag = drag_fac * pu->disc_flow.y;
    pu->pitch_drag = drag_fac * pu->disc_flow.z;

    /* ---------------------------------------------------------------
     * Thrust vector (prop-frame: X = axial, Y/Z = drag offsets)
     * --------------------------------------------------------------- */
    pu->thrust = wp_vec3f_make( thrust, pu->yaw_drag, pu->pitch_drag );

    /* ---------------------------------------------------------------
     * 8. Propwash (induced velocity) -- lagged momentum-theory
     *    k = dt / flow_settling_time
     *    temp = flow_x^2 + 4T / (2 rho A)
     *    if temp >= 0: propwash <- (1-k)*propwash + k*0.5*(sqrt(temp) - flow_x)
     * --------------------------------------------------------------- */
    {
        wp_f32 k = dt / pu->flow_settling_time;
        if( k > 1.0f )
            k = 1.0f;

        wp_f32 flow_x = pu->disc_flow.x;
        wp_f32 temp = flow_x * flow_x + 4.0f * thrust / ( 2.0f * rho * pu->area );

        if( temp >= 0.0f )
        {
            wp_f32 target = 0.5f * ( wp_sqrtf( temp ) - flow_x );
            pu->propwash = ( 1.0f - k ) * pu->propwash + k * target;
        }
    }

    /* ---------------------------------------------------------------
     * 9. Profile-drag power  P_drag = drag_power_factor * Cd * w^3
     * --------------------------------------------------------------- */
    wp_f32 w3 = pu->rotor_rps * pu->rotor_rps * pu->rotor_rps;
    pu->drag_power = pu->drag_power_factor * cd * w3;

    /* ---------------------------------------------------------------
     * 10. Induced power  P_ind = thrust * propwash
     * --------------------------------------------------------------- */
    pu->induced_power = thrust * pu->propwash;

    /* ---------------------------------------------------------------
     * 11. Propeller torque = (P_ind + P_drag) / w
     * --------------------------------------------------------------- */
    if( wp_absf( pu->rotor_rps ) > 1e-6f )
        pu->prop_torque = ( pu->induced_power + pu->drag_power ) / pu->rotor_rps;
    else
        pu->prop_torque = 0.0f;

    /* ---------------------------------------------------------------
     * 12-13. Rotor acceleration and integration
     *        dw = dt * (input_torque - prop_torque) / total_inertia
     *        limited to +/-4000 rad/s^2 then integrated
     * --------------------------------------------------------------- */
    wp_f32 delta_w = 0.0f;
    if( pu->total_inertia > 1e-9f )
    {
        delta_w = dt * ( pu->input_torque - pu->prop_torque ) / pu->total_inertia;
    }

    /* Clamp angular acceleration */
    wp_f32 max_dw = 4000.0f * dt;
    if( delta_w > max_dw )
        delta_w = max_dw;
    if( delta_w < -max_dw )
        delta_w = -max_dw;

    /* Integrate (prevent running backwards) */
    pu->rotor_rps = pu->rotor_rps + delta_w;
    if( pu->rotor_rps < 0.0f )
        pu->rotor_rps = 0.0f;

    /* Sync RPM from rad/s */
    pu->rotor_rpm = pu->rotor_rps * 60.0f / WORKPHONE_TWO_PI_F;

    /* ---------------------------------------------------------------
     * Reaction torque vector (prop-frame):
     *   X = -prop_torque  (opposes rotation)
     *   Y = pitch P-factor
     *   Z = yaw P-factor
     * --------------------------------------------------------------- */
    pu->torque = wp_vec3f_make( -pu->prop_torque, pu->pitch_pfac, pu->yaw_pfac );
}

/* =========================================================================
 * Outputs
 * ====================================================================== */

wp_vec3f wp_propeller_unit_get_thrust( const wp_propeller_unit *pu )
{
    return pu->thrust;
}

wp_vec3f wp_propeller_unit_get_torque( const wp_propeller_unit *pu )
{
    return pu->torque;
}

wp_f32 wp_propeller_unit_get_thrust_value( const wp_propeller_unit *pu )
{
    return pu->thrust_value;
}

wp_f32 wp_propeller_unit_get_propwash( const wp_propeller_unit *pu )
{
    return pu->propwash;
}

wp_f32 wp_propeller_unit_get_rotor_rpm( const wp_propeller_unit *pu )
{
    return pu->rotor_rpm;
}
