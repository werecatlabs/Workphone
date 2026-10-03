/**
 * @file workphone_vehicle_propeller_unit.h
 * @brief C99 API for a propeller unit controller with blade-element aerodynamics.
 *
 * This module models a propeller system with:
 *   - Blade element theory for thrust, torque, and induced velocity.
 *   - Pitch and yaw forces (P-factor, drag imbalance).
 *   - Engine/motor torque integration with rotor inertia.
 *   - Propwash dynamics (lagged response via momentum theory).
 *   - Configurable blade geometry (diameter, pitch, chord, solidity).
 *
 * Physics-engine agnostic: computes forces/torques; caller applies them to the body.
 *
 * Typical usage:
 * @code
 *   wp_propeller_unit pu;
 *   wp_propeller_unit_init(&pu);
 *   wp_propeller_unit_set_geometry(&pu, diameter, pitch, blades, chord);
 *   wp_propeller_unit_set_inertia(&pu, rotor_moi, total_moi);
 *   wp_propeller_unit_set_environment(&pu, air_density);
 *
 *   // Each tick:
 *   wp_propeller_unit_set_state(&pu, rotor_rpm, input_torque);
 *   wp_propeller_unit_set_flow(&pu, disc_flow_axial, disc_flow_lateral_y, disc_flow_lateral_z);
 *   wp_propeller_unit_fixed_update(&pu, dt);
 *
 *   wp_vec3f thrust = wp_propeller_unit_get_thrust(&pu);
 *   wp_vec3f torque = wp_propeller_unit_get_torque(&pu);
 * @endcode
 */

#ifndef WORKPHONE_VEHICLE_PROPELLER_UNIT_H
#define WORKPHONE_VEHICLE_PROPELLER_UNIT_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Propeller unit state
 * ---------------------------------------------------------------------- */

/**
 * @brief Complete state for one propeller unit (rotor + aerodynamics).
 *
 * Manages blade-element force/torque computation, propwash dynamics,
 * and rotor inertia integration.
 */
typedef struct wp_propeller_unit
{
    /* -- rotor geometry -- */
    wp_f32 diameter; /* metres */
    wp_f32 pitch;    /* metres (geometric pitch) */
    wp_s32 blades;   /* number of blades */
    wp_f32 chord;    /* metres (mean chord) */

    /* -- aerodynamic coefficients -- */
    wp_f32 cl_slope;      /* dCl/dα per radian (default 2π) */
    wp_f32 cd0;           /* profile drag coefficient at zero lift */
    wp_f32 dcl_dcd_ratio; /* ratio for polar: Cd = Cd0 + ratio * Cl^2 */
    wp_f32 cl_max;        /* maximum lift coefficient */
    wp_f32 cl_min;        /* minimum (most negative) lift coefficient */

    /* -- derived geometry -- */
    wp_f32 area;            /* rotor disc area (πD²/4) */
    wp_f32 blade_area;      /* total blade planform area */
    wp_f32 solidity;        /* blade_area / disc_area */
    wp_f32 geometric_pitch; /* geometric pitch angle (rad) */

    /* -- rotor dynamics -- */
    wp_f32 rotor_inertia; /* kg·m² of rotor alone */
    wp_f32 total_inertia; /* kg·m² including wake effects */
    wp_f32 rotor_rpm;     /* current rotor speed in RPM */
    wp_f32 rotor_rps;     /* rotor speed in rad/s */

    /* -- aerodynamic state -- */
    wp_vec3f disc_flow; /* local flow through disc (X=axial, Y/Z=lateral) */
    wp_f32 propwash;    /* induced axial velocity (m/s) */
    wp_f32 blade_speed; /* effective blade speed at 0.4R (m/s) */

    /* -- blade coefficients -- */
    wp_f32 cl;              /* lift coefficient of blade */
    wp_f32 cd;              /* drag coefficient of blade */
    wp_f32 effective_pitch; /* effective geometric pitch after flow correction */

    /* -- engine inputs -- */
    wp_f32 input_torque; /* input motor/engine torque (N·m) */

    /* -- computed forces/torques -- */
    wp_f32 thrust_value; /* magnitude of thrust (N) */
    wp_f32 yaw_pfac;     /* P-factor yaw component */
    wp_f32 pitch_pfac;   /* P-factor pitch component */
    wp_f32 yaw_drag;     /* drag-imbalance yaw force */
    wp_f32 pitch_drag;   /* drag-imbalance pitch force */

    wp_vec3f thrust; /* world-space thrust vector (N) */
    wp_vec3f torque; /* world-space reaction torque (N·m) */

    /* -- propwash dynamics -- */
    wp_f32 flow_settling_time; /* time constant for propwash lag (default 0.1s) */

    /* -- environment -- */
    wp_f32 air_density; /* kg/m³ */

    /* -- configuration -- */
    wp_s32 reversed; /* 1 if prop rotates reverse, 0 otherwise */
    wp_s32 ducted;   /* 1 if prop is ducted, 0 otherwise */

    /* -- internal scratch -- */
    wp_f32 drag_power;        /* profile drag power (W) */
    wp_f32 induced_power;     /* induced drag power (W) */
    wp_f32 prop_torque;       /* computed propeller torque (N·m) */
    wp_f32 drag_power_factor; /* blades × ρ × chord × 0.5 × D⁴ / 128 */
} wp_propeller_unit;

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Initialise a propeller unit with default parameters.
 *
 * @param pu Pointer to the unit instance.
 */
void wp_propeller_unit_init( wp_propeller_unit *pu );

/* -------------------------------------------------------------------------
 * Configuration
 * ---------------------------------------------------------------------- */

/**
 * @brief Set rotor geometry (diameter, pitch, blade count, mean chord).
 *
 * @param pu       Pointer to the unit.
 * @param diameter Rotor disc diameter in metres.
 * @param pitch    Geometric pitch in metres.
 * @param blades   Number of blades.
 * @param chord    Mean blade chord in metres.
 */
void wp_propeller_unit_set_geometry( wp_propeller_unit *pu, wp_f32 diameter, wp_f32 pitch, wp_s32 blades,
                                     wp_f32 chord );

/**
 * @brief Set aerodynamic coefficients.
 *
 * @param pu                 Pointer to the unit.
 * @param cl_slope           dCl/dα in radians (default 2π).
 * @param cd0                Cd at zero lift (default 0.01).
 * @param dcl_dcd_ratio      Cd = Cd0 + ratio * Cl² (default 0.02).
 * @param cl_max             Maximum Cl (default 1.2).
 * @param cl_min             Minimum Cl (default -0.8).
 */
void wp_propeller_unit_set_aerodynamics( wp_propeller_unit *pu, wp_f32 cl_slope, wp_f32 cd0,
                                         wp_f32 dcl_dcd_ratio, wp_f32 cl_max, wp_f32 cl_min );

/**
 * @brief Set rotor inertia values.
 *
 * @param pu              Pointer to the unit.
 * @param rotor_inertia   Moment of inertia of rotor (kg·m²).
 * @param total_inertia   Total MoI including wake effects (kg·m²).
 */
void wp_propeller_unit_set_inertia( wp_propeller_unit *pu, wp_f32 rotor_inertia, wp_f32 total_inertia );

/**
 * @brief Set environmental parameters.
 *
 * @param pu          Pointer to the unit.
 * @param air_density Air density in kg/m³.
 * @param settling_tc Flow settling time constant in seconds (default 0.1).
 */
void wp_propeller_unit_set_environment( wp_propeller_unit *pu, wp_f32 air_density, wp_f32 settling_tc );

/**
 * @brief Set configuration flags.
 *
 * @param pu       Pointer to the unit.
 * @param reversed 1 for reverse rotation, 0 for normal.
 * @param ducted   1 if ducted, 0 if open.
 */
void wp_propeller_unit_set_config( wp_propeller_unit *pu, wp_s32 reversed, wp_s32 ducted );

/* -------------------------------------------------------------------------
 * Per-tick inputs & update
 * ---------------------------------------------------------------------- */

/**
 * @brief Set rotor speed and engine torque input.
 *
 * @param pu            Pointer to the unit.
 * @param rotor_rpm     Current rotor speed in RPM.
 * @param input_torque  Engine/motor output torque in N·m.
 */
void wp_propeller_unit_set_rotor_state( wp_propeller_unit *pu, wp_f32 rotor_rpm, wp_f32 input_torque );

/**
 * @brief Set disc flow components (from aircraft velocity + angular velocity).
 *
 * Typically computed as:
 *   - disc_flow.x = dot(velocity, prop_forward)  (axial flow)
 *   - disc_flow.y = dot(velocity, prop_right)    (lateral flow)
 *   - disc_flow.z = dot(velocity, prop_up)       (vertical flow)
 *
 * @param pu       Pointer to the unit.
 * @param flow_x   Axial flow (m/s).
 * @param flow_y   Lateral flow Y component (m/s).
 * @param flow_z   Lateral flow Z component (m/s).
 */
void wp_propeller_unit_set_flow( wp_propeller_unit *pu, wp_f32 flow_x, wp_f32 flow_y, wp_f32 flow_z );

/**
 * @brief Advance the propeller simulation by one fixed time step.
 *
 * Computes blade coefficients, thrust, drag forces, propwash, and
 * rotor acceleration; accumulates into thrust and torque vectors.
 *
 * @param pu Pointer to the unit.
 * @param dt Fixed time step in seconds.
 */
void wp_propeller_unit_fixed_update( wp_propeller_unit *pu, wp_f32 dt );

/* -------------------------------------------------------------------------
 * Outputs
 * ---------------------------------------------------------------------- */

/**
 * @brief Get the computed world-space thrust vector.
 *
 * @param pu Pointer to the unit.
 * @return   Thrust vector in N.
 */
wp_vec3f wp_propeller_unit_get_thrust( const wp_propeller_unit *pu );

/**
 * @brief Get the reaction torque vector.
 *
 * @param pu Pointer to the unit.
 * @return   Torque vector in N·m.
 */
wp_vec3f wp_propeller_unit_get_torque( const wp_propeller_unit *pu );

/**
 * @brief Get scalar thrust magnitude.
 *
 * @param pu Pointer to the unit.
 * @return   Thrust in N.
 */
wp_f32 wp_propeller_unit_get_thrust_value( const wp_propeller_unit *pu );

/**
 * @brief Get induced propwash velocity.
 *
 * @param pu Pointer to the unit.
 * @return   Induced velocity in m/s.
 */
wp_f32 wp_propeller_unit_get_propwash( const wp_propeller_unit *pu );

/**
 * @brief Get current rotor speed.
 *
 * @param pu Pointer to the unit.
 * @return   Rotor speed in RPM.
 */
wp_f32 wp_propeller_unit_get_rotor_rpm( const wp_propeller_unit *pu );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_VEHICLE_PROPELLER_UNIT_H */
