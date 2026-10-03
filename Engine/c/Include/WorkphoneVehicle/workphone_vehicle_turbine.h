/**
 * @file workphone_vehicle_turbine.h
 * @brief C99 API for a helicopter turbine engine simulation.
 *
 * Models a twin-spool turboshaft engine with:
 *   - Gas-generator spool (N1) driven by fuel flow.
 *   - Power-turbine spool (N2) driven by N1.
 *   - A proportional RPM governor that trims fuel flow to maintain a target
 *     rotor speed.
 *   - Inter-turbine temperature (ITT) monitoring with automatic fuel trim on
 *     over-temperature.
 *   - A final torque output computed from the N2 fraction.
 *
 * Typical usage:
 * @code
 *   wp_turbine engine;
 *   wp_turbine_init( &engine );
 *
 *   // each simulation tick:
 *   wp_turbine_set_throttle( &engine, pilot_throttle );
 *   wp_turbine_set_rotor_rpm( &engine, measured_rotor_rpm );
 *   wp_turbine_update( &engine, fixed_dt );
 *
 *   wp_f32 torque = wp_turbine_get_output_torque( &engine );
 * @endcode
 */

#ifndef WORKPHONE_VEHICLE_TURBINE_H
#define WORKPHONE_VEHICLE_TURBINE_H

#include "workphone_config.h"
#include "workphone_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Turbine state structure
 * ---------------------------------------------------------------------- */

/**
 * @brief Complete state for one turboshaft engine instance.
 *
 * All fields are plain data; no heap allocation is required.
 * Initialise with wp_turbine_init() before first use.
 */
typedef struct wp_turbine
{
    /* -- tunable parameters -------------------------------------------- */

    /** Maximum N1 spool value (percentage, default 100). */
    wp_f32 max_n1;

    /** Maximum N2 spool value (percentage, default 100). */
    wp_f32 max_n2;

    /** N1 acceleration rate (percentage per second, default 15). */
    wp_f32 n1_accel;

    /** N2 acceleration rate (percentage per second, default 8). */
    wp_f32 n2_accel;

    /** Fuel-flow response rate used as the lerp speed (default 2). */
    wp_f32 fuel_response;

    /** Governor target rotor speed in RPM (default 400). */
    wp_f32 target_rotor_rpm;

    /**
     * @brief Proportional governor gain.
     *
     * Scales the RPM error into a fuel-flow correction each second
     * (default 0.015).
     */
    wp_f32 governor_gain;

    /** Gear ratio between power turbine and main rotor (default 25). */
    wp_f32 gear_ratio;

    /** Maximum output shaft torque in N·m (default 7000). */
    wp_f32 max_torque;

    /** Inter-turbine temperature limit in °C (default 850). */
    wp_f32 max_temp;

    /* -- runtime state -------------------------------------------------- */

    /** Normalised fuel flow [0, 1]. */
    wp_f32 fuel_flow;

    /** Gas-generator spool speed as a percentage of max_n1. */
    wp_f32 n1;

    /** Power-turbine spool speed as a percentage of max_n2. */
    wp_f32 n2;

    /** Simulated inter-turbine temperature in °C. */
    wp_f32 temperature;

    /** Output shaft torque in N·m, updated each call to wp_turbine_update(). */
    wp_f32 output_torque;

    /** Current main-rotor RPM fed back by the caller each tick. */
    wp_f32 rotor_rpm;

    /** Normalised throttle demand [0, 1] set by the caller. */
    wp_f32 throttle_input;
} wp_turbine;

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Initialise a wp_turbine with default parameters and zero runtime state.
 *
 * Must be called before any other wp_turbine function.
 *
 * @param t Pointer to the turbine instance to initialise.
 */
void wp_turbine_init( wp_turbine *t );

/* -------------------------------------------------------------------------
 * Per-tick update
 * ---------------------------------------------------------------------- */

/**
 * @brief Advance the turbine simulation by one time step.
 *
 * Executes, in order:
 *  1. Fuel system  – lerps fuel_flow toward throttle_input.
 *  2. Spool dynamics – moves N1 and N2 toward their targets.
 *  3. Governor – trims fuel_flow to maintain target_rotor_rpm.
 *  4. Temperature – updates ITT and applies fuel trim on over-temp.
 *  5. Torque output – computes output_torque from N2 fraction.
 *
 * @param t  Pointer to the turbine instance.
 * @param dt Fixed simulation time step in seconds (e.g. 0.02 for 50 Hz).
 */
void wp_turbine_update( wp_turbine *t, wp_f32 dt );

/* -------------------------------------------------------------------------
 * Inputs
 * ---------------------------------------------------------------------- */

/**
 * @brief Set the pilot throttle demand.
 *
 * @param t        Pointer to the turbine instance.
 * @param throttle Normalised throttle position in [0, 1].
 */
void wp_turbine_set_throttle( wp_turbine *t, wp_f32 throttle );

/**
 * @brief Provide the current main-rotor speed for governor feedback.
 *
 * Must be called before wp_turbine_update() each tick with the latest
 * measured (or simulated) rotor RPM.
 *
 * @param t   Pointer to the turbine instance.
 * @param rpm Current rotor speed in RPM.
 */
void wp_turbine_set_rotor_rpm( wp_turbine *t, wp_f32 rpm );

/* -------------------------------------------------------------------------
 * Outputs
 * ---------------------------------------------------------------------- */

/**
 * @brief Return the computed output shaft torque in N·m.
 *
 * @param t Pointer to the turbine instance.
 * @return  Output torque in N·m.
 */
wp_f32 wp_turbine_get_output_torque( const wp_turbine *t );

/**
 * @brief Return the current gas-generator spool speed (N1) as a percentage.
 *
 * @param t Pointer to the turbine instance.
 * @return  N1 percentage [0, max_n1].
 */
wp_f32 wp_turbine_get_n1( const wp_turbine *t );

/**
 * @brief Return the current power-turbine spool speed (N2) as a percentage.
 *
 * @param t Pointer to the turbine instance.
 * @return  N2 percentage [0, max_n2].
 */
wp_f32 wp_turbine_get_n2( const wp_turbine *t );

/**
 * @brief Return the current inter-turbine temperature in °C.
 *
 * @param t Pointer to the turbine instance.
 * @return  Temperature in °C.
 */
wp_f32 wp_turbine_get_temperature( const wp_turbine *t );

/**
 * @brief Return the current normalised fuel flow.
 *
 * @param t Pointer to the turbine instance.
 * @return  Fuel flow in [0, 1].
 */
wp_f32 wp_turbine_get_fuel_flow( const wp_turbine *t );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_VEHICLE_TURBINE_H */
