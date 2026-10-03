/**
 * @file workphone_vehicle_extremehelicopter.h
 * @brief C99 API for a high-fidelity helicopter flight-dynamics controller.
 *
 * Extends the basic helicopter model with:
 *   - Physical rotor RPM dynamics driven by engine torque versus aerodynamic
 *     drag torque.
 *   - Blade-element induced-velocity estimation using momentum theory.
 *   - Main rotor thrust computed from tip speed and collective blade pitch,
 *     with the thrust vector tilted by cyclic pitch/roll input.
 *   - Torque reaction force applied about the rotor axis.
 *   - Tail-rotor anti-torque and yaw pedal control.
 *   - Translational lift bonus as airspeed increases through translational
 *     lift entry speed.
 *   - Ground-effect multiplier (caller-provided distance, no internal
 *     raycast).
 *   - Vortex ring state: lift loss when descending fast with high collective.
 *
 * The controller is physics-engine agnostic.  It accumulates results into
 * @c net_force and @c net_torque; the caller applies them to their rigid body.
 *
 * Caller responsibilities each fixed tick:
 *   1. Provide pilot inputs via wp_extreme_helicopter_set_inputs().
 *   2. Provide current rigid-body state via wp_extreme_helicopter_set_state().
 *   3. Call wp_extreme_helicopter_fixed_update().
 *   4. Apply net_force and net_torque to the rigid body.
 *
 * Typical usage:
 * @code
 *   wp_extreme_helicopter heli;
 *   wp_extreme_helicopter_init( &heli );
 *
 *   // Apply recommended CoM offset once at startup:
 *   wp_vec3f com = wp_extreme_helicopter_get_com_offset( &heli );
 *   wp_rigidbody_set_cmass_local_position( body, com );
 *
 *   // Each fixed tick:
 *   wp_extreme_helicopter_set_inputs( &heli, throttle, collective,
 *                                     pitch, roll, yaw );
 *   wp_extreme_helicopter_set_state( &heli, position, orientation,
 *                                    linear_velocity, ground_dist );
 *   wp_extreme_helicopter_fixed_update( &heli, fixed_dt );
 *
 *   wp_rigidbody_add_force ( body, heli.net_force,  WORKPHONE_FORCE_MODE_FORCE );
 *   wp_rigidbody_add_torque( body, heli.net_torque, WORKPHONE_FORCE_MODE_FORCE );
 * @endcode
 *
 * @note  Call wp_extreme_helicopter_rebuild_derived() if rotor_radius is
 *        changed after initialisation; other parameters may be changed freely.
 */

#ifndef WORKPHONE_VEHICLE_EXTREMEHELICOPTER_H
#define WORKPHONE_VEHICLE_EXTREMEHELICOPTER_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_vector.h"
#include "workphone_quat.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sentinel ground distance indicating no ground surface within range.
 */
#define WP_EXTREME_HELI_NO_GROUND ( 1e30f )

/* -------------------------------------------------------------------------
 * State structure
 * ---------------------------------------------------------------------- */

/**
 * @brief Complete state for one extreme-helicopter flight-dynamics instance.
 *
 * All fields are plain data; no heap allocation is required.
 * Initialise with wp_extreme_helicopter_init() before first use.
 */
typedef struct wp_extreme_helicopter
{
    /* -- rotor system --------------------------------------------------- */

    /** Main rotor radius in metres (default 6.5). */
    wp_f32 rotor_radius;

    /**
     * @brief Rotor rotational inertia in kg·m² (default 140).
     *
     * Higher values slow the RPM response to torque changes.
     */
    wp_f32 rotor_inertia;

    /** Peak engine torque delivered to the rotor shaft in N·m (default 6 000). */
    wp_f32 max_engine_torque;

    /**
     * @brief Aerodynamic drag coefficient applied against rotor RPM
     *        (default 20).
     *
     * drag_torque = rotor_drag_coeff * rotor_rpm.
     */
    wp_f32 rotor_drag_coeff;

    /** Maximum allowable rotor speed in RPM (default 450). */
    wp_f32 max_rpm;

    /* -- blade pitch ---------------------------------------------------- */

    /** Maximum collective blade pitch angle in degrees (default 18). */
    wp_f32 max_collective_pitch;

    /** Maximum cyclic disc tilt from neutral in degrees (default 12). */
    wp_f32 cyclic_tilt_max;

    /* -- aerodynamics --------------------------------------------------- */

    /** Air density in kg/m³ (default 1.225 — ISA sea level). */
    wp_f32 air_density;

    /**
     * @brief Two-dimensional blade lift slope, dCl/dα per radian (default 5.7).
     *
     * Used in the linear thin-aerofoil approximation: Cl = blade_lift_slope * α.
     */
    wp_f32 blade_lift_slope;

    /* -- tail rotor ----------------------------------------------------- */

    /** Tail-rotor yaw torque magnitude at full pedal in N·m (default 3 000). */
    wp_f32 tail_rotor_power;

    /* -- ground effect -------------------------------------------------- */

    /**
     * @brief Height above ground below which ground effect is active,
     *        in metres (default 10).
     */
    wp_f32 ground_effect_height;

    /* -- vortex ring state ---------------------------------------------- */

    /**
     * @brief Descent rate threshold for vortex ring state entry in m/s
     *        (default −5; must be negative).
     *
     * When the downward speed exceeds this magnitude and collective is
     * above 0.3, lift is reduced to 40 % of normal.
     */
    wp_f32 vortex_descent_rate;

    /* -- derived (cached) ----------------------------------------------- */

    /**
     * @brief Rotor disc area in m², cached from rotor_radius at init.
     *
     * Recompute with wp_extreme_helicopter_rebuild_derived() if rotor_radius
     * is changed after initialisation.
     */
    wp_f32 rotor_area;

    /* -- runtime state -------------------------------------------------- */

    /** Current main-rotor speed in RPM. */
    wp_f32 rotor_rpm;

    /**
     * @brief Momentum-theory induced velocity through the rotor disc in m/s.
     *
     * Updated each tick by wp_extreme_helicopter_fixed_update().
     */
    wp_f32 induced_velocity;

    /* -- inputs (set before each fixed update) -------------------------- */

    /** Engine throttle demand in [0, 1]. */
    wp_f32 throttle;

    /** Collective blade pitch demand in [−1, 1]. */
    wp_f32 collective;

    /** Cyclic pitch demand in [−1, 1]; positive nose-up. */
    wp_f32 pitch;

    /** Cyclic roll demand in [−1, 1]; positive right. */
    wp_f32 roll;

    /** Yaw pedal demand in [−1, 1]; positive nose-right. */
    wp_f32 yaw;

    /* -- rigid-body state (set before each fixed update) ---------------- */

    /** World-space position of the helicopter. */
    wp_vec3f position;

    /** World-space orientation of the helicopter (unit quaternion). */
    wp_quatf orientation;

    /**
     * @brief World-space linear velocity of the rigid body in m/s.
     *
     * Used for translational lift and vortex ring state calculations.
     */
    wp_vec3f velocity;

    /**
     * @brief Distance from position to the nearest ground surface in metres.
     *
     * Set to WP_EXTREME_HELI_NO_GROUND when no ground is within range.
     */
    wp_f32 ground_distance;

    /* -- outputs (valid after wp_extreme_helicopter_fixed_update) ------- */

    /** Accumulated world-space force to apply to the rigid body (N). */
    wp_vec3f net_force;

    /** Accumulated world-space torque to apply to the rigid body (N·m). */
    wp_vec3f net_torque;
} wp_extreme_helicopter;

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Initialise a wp_extreme_helicopter with default parameters and
 *        zero runtime state.
 *
 * Also computes the cached rotor_area from rotor_radius.
 * Must be called before any other wp_extreme_helicopter function.
 *
 * @param h Pointer to the instance to initialise.
 */
void wp_extreme_helicopter_init( wp_extreme_helicopter *h );

/**
 * @brief Recompute derived fields after rotor_radius has been changed.
 *
 * Only required if rotor_radius is modified after wp_extreme_helicopter_init()
 * has already been called.
 *
 * @param h Pointer to the instance to update.
 */
void wp_extreme_helicopter_rebuild_derived( wp_extreme_helicopter *h );

/* -------------------------------------------------------------------------
 * Setup helpers
 * ---------------------------------------------------------------------- */

/**
 * @brief Return the recommended local centre-of-mass offset.
 *
 * Apply this to the rigid body once at startup to lower the CoM and
 * improve roll stability.
 *
 * @param h Pointer to the instance.
 * @return  Local-space CoM offset vector.
 */
wp_vec3f wp_extreme_helicopter_get_com_offset( const wp_extreme_helicopter *h );

/* -------------------------------------------------------------------------
 * Per-tick update
 * ---------------------------------------------------------------------- */

/**
 * @brief Advance the flight-dynamics simulation by one fixed physics tick.
 *
 * Executes, in order:
 *  1. Rotor RPM    – integrates engine minus drag torque.
 *  2. Induced flow – estimates induced velocity from momentum theory.
 *  3. Main rotor   – computes thrust, applies ground effect / translational
 *                    lift / vortex ring modifiers, tilts disc by cyclic
 *                    input, adds torque reaction.
 *  4. Tail rotor   – adds anti-torque and yaw pedal torque.
 *
 * Results are accumulated into net_force and net_torque.
 *
 * @param h  Pointer to the instance.
 * @param dt Fixed simulation time step in seconds (e.g. 0.02 for 50 Hz).
 */
void wp_extreme_helicopter_fixed_update( wp_extreme_helicopter *h, wp_f32 dt );

/* -------------------------------------------------------------------------
 * Inputs
 * ---------------------------------------------------------------------- */

/**
 * @brief Set all pilot control inputs for the next tick.
 *
 * @param h          Pointer to the instance.
 * @param throttle   Engine throttle, clamped to [0, 1].
 * @param collective Collective pitch demand in [−1, 1].
 * @param pitch      Cyclic pitch demand in [−1, 1].
 * @param roll       Cyclic roll demand in [−1, 1].
 * @param yaw        Yaw pedal demand in [−1, 1].
 */
void wp_extreme_helicopter_set_inputs( wp_extreme_helicopter *h, wp_f32 throttle, wp_f32 collective,
                                       wp_f32 pitch, wp_f32 roll, wp_f32 yaw );

/**
 * @brief Set the current rigid-body state used for aerodynamic modifiers.
 *
 * Call before wp_extreme_helicopter_fixed_update() each tick with values
 * read back from the physics engine.
 *
 * @param h              Pointer to the instance.
 * @param position       Current world-space position.
 * @param orientation    Current world-space orientation (unit quaternion).
 * @param velocity       Current world-space linear velocity in m/s.
 * @param ground_dist    Distance to nearest ground surface in metres, or
 *                       WP_EXTREME_HELI_NO_GROUND if none within range.
 */
void wp_extreme_helicopter_set_state( wp_extreme_helicopter *h, wp_vec3f position, wp_quatf orientation,
                                      wp_vec3f velocity, wp_f32 ground_dist );

/* -------------------------------------------------------------------------
 * Outputs
 * ---------------------------------------------------------------------- */

/**
 * @brief Return the current rotor speed.
 *
 * @param h Pointer to the instance.
 * @return  Rotor speed in RPM.
 */
wp_f32 wp_extreme_helicopter_get_rotor_rpm( const wp_extreme_helicopter *h );

/**
 * @brief Return the last computed induced velocity through the rotor disc.
 *
 * @param h Pointer to the instance.
 * @return  Induced velocity in m/s.
 */
wp_f32 wp_extreme_helicopter_get_induced_velocity( const wp_extreme_helicopter *h );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_VEHICLE_EXTREMEHELICOPTER_H */
