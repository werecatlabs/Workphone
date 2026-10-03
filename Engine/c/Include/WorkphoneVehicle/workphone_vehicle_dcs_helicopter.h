/**
 * @file workphone_vehicle_dcs_helicopter.h
 * @brief C99 API for a DCS-inspired high-fidelity helicopter flight-dynamics
 *        controller.
 *
 * Extends the extreme-helicopter model with three additional sub-systems:
 *
 *   - **Dynamic inflow** – induced velocity lerps toward the momentum-theory
 *     target at a rate governed by inflow_lag, reproducing the real rotor's
 *     lag in building up downwash.
 *
 *   - **Blade flapping** – lateral airspeed (transformed to local space via
 *     the inverse orientation) drives a per-tick flapping offset that is
 *     blended into the disc-normal direction, giving an airspeed-dependent
 *     thrust tilt without explicit cyclic mixing.
 *
 *   - **Stability Augmentation System (SAS)** – a torque proportional and
 *     opposite to the current angular velocity is applied each tick to damp
 *     unwanted rotations.  The caller must supply angular_velocity via
 *     wp_dcs_helicopter_set_state().
 *
 * The controller is physics-engine agnostic.  Results are written into
 * net_force and net_torque; the caller applies them to their rigid body.
 *
 * Typical usage:
 * @code
 *   wp_dcs_helicopter heli;
 *   wp_dcs_helicopter_init( &heli );
 *
 *   // Apply recommended CoM offset once at startup:
 *   wp_vec3f com = wp_dcs_helicopter_get_com_offset( &heli );
 *   wp_rigidbody_set_cmass_local_position( body, com );
 *
 *   // Each fixed tick:
 *   wp_dcs_helicopter_set_inputs( &heli, throttle, collective, pitch, roll, yaw );
 *   wp_dcs_helicopter_set_state( &heli, position, orientation,
 *                                linear_velocity, angular_velocity );
 *   wp_dcs_helicopter_fixed_update( &heli, fixed_dt );
 *
 *   wp_rigidbody_add_force ( body, heli.net_force,  WORKPHONE_FORCE_MODE_FORCE );
 *   wp_rigidbody_add_torque( body, heli.net_torque, WORKPHONE_FORCE_MODE_FORCE );
 * @endcode
 *
 * @note  Call wp_dcs_helicopter_rebuild_derived() if rotor_radius is changed
 *        after initialisation.
 */

#ifndef WORKPHONE_VEHICLE_DCS_HELICOPTER_H
#define WORKPHONE_VEHICLE_DCS_HELICOPTER_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_vector.h"
#include "workphone_quat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * State structure
 * ---------------------------------------------------------------------- */

/**
 * @brief Complete state for one DCS-helicopter flight-dynamics instance.
 *
 * All fields are plain data; no heap allocation is required.
 * Initialise with wp_dcs_helicopter_init() before first use.
 */
typedef struct wp_dcs_helicopter
{
    /* -- rotor system --------------------------------------------------- */

    /** Main rotor radius in metres (default 6.5). */
    wp_f32 rotor_radius;

    /** Rotor rotational inertia in kg·m² (default 150). */
    wp_f32 rotor_inertia;

    /** Peak engine torque in N·m (default 6 500). */
    wp_f32 max_engine_torque;

    /**
     * @brief Aerodynamic rotor drag coefficient (default 25).
     *
     * drag_torque = rotor_drag * rotor_rpm.
     */
    wp_f32 rotor_drag;

    /** Maximum allowable rotor speed in RPM (default 450). */
    wp_f32 max_rpm;

    /* -- aerodynamics --------------------------------------------------- */

    /** Air density in kg/m³ (default 1.225 — ISA sea level). */
    wp_f32 air_density;

    /* -- blade pitch ---------------------------------------------------- */

    /** Maximum collective blade pitch angle in degrees (default 18). */
    wp_f32 max_collective_pitch;

    /** Maximum cyclic disc tilt from neutral in degrees (default 12). */
    wp_f32 cyclic_max_tilt;

    /* -- dynamic inflow ------------------------------------------------- */

    /**
     * @brief Inflow lag time constant used as the lerp rate (default 2.5).
     *
     * Higher values make induced_velocity respond more slowly to collective
     * changes, improving low-frequency rotor dynamics fidelity.
     */
    wp_f32 inflow_lag;

    /* -- blade flapping ------------------------------------------------- */

    /**
     * @brief Lerp rate toward the target flapping offset (default 6).
     *
     * Higher values produce snappier disc-tilt response to airspeed changes.
     */
    wp_f32 flapping_stiffness;

    /* -- tail rotor ----------------------------------------------------- */

    /** Tail-rotor yaw torque magnitude at full pedal in N·m (default 3 500). */
    wp_f32 tail_rotor_power;

    /* -- SAS ------------------------------------------------------------ */

    /**
     * @brief Angular damping gain for the Stability Augmentation System
     *        (default 2.5).
     *
     * A counter-torque of magnitude angular_damping * |angular_velocity| is
     * applied each tick to resist unwanted rotations.
     */
    wp_f32 angular_damping;

    /* -- derived (cached) ----------------------------------------------- */

    /**
     * @brief Rotor disc area in m², cached from rotor_radius at init.
     *
     * Recompute with wp_dcs_helicopter_rebuild_derived() if rotor_radius
     * is changed after initialisation.
     */
    wp_f32 rotor_area;

    /* -- runtime state -------------------------------------------------- */

    /** Current main-rotor speed in RPM. */
    wp_f32 rotor_rpm;

    /**
     * @brief Lagged induced velocity through the rotor disc in m/s.
     *
     * Updated each tick by wp_dcs_helicopter_fixed_update().
     */
    wp_f32 induced_velocity;

    /**
     * @brief Accumulated blade-flapping disc-tilt offset.
     *
     * Computed in local space from lateral airspeed and added to the
     * world-space disc normal each tick.
     */
    wp_vec3f flapping_offset;

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
     * Used for translational lift, retreating-blade stall, blade flapping
     * and the momentum-theory thrust formula.
     */
    wp_vec3f velocity;

    /**
     * @brief World-space angular velocity of the rigid body in rad/s.
     *
     * Used by the SAS to apply counter-rotation damping torque.
     */
    wp_vec3f angular_velocity;

    /* -- outputs (valid after wp_dcs_helicopter_fixed_update) ----------- */

    /** Accumulated world-space force to apply to the rigid body (N). */
    wp_vec3f net_force;

    /** Accumulated world-space torque to apply to the rigid body (N·m). */
    wp_vec3f net_torque;
} wp_dcs_helicopter;

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Initialise a wp_dcs_helicopter with default parameters and zero
 *        runtime state.
 *
 * Also computes the cached rotor_area from rotor_radius.
 * Must be called before any other wp_dcs_helicopter function.
 *
 * @param h Pointer to the instance to initialise.
 */
void wp_dcs_helicopter_init( wp_dcs_helicopter *h );

/**
 * @brief Recompute derived fields after rotor_radius has been changed.
 *
 * Only required if rotor_radius is modified after wp_dcs_helicopter_init()
 * has already been called.
 *
 * @param h Pointer to the instance to update.
 */
void wp_dcs_helicopter_rebuild_derived( wp_dcs_helicopter *h );

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
wp_vec3f wp_dcs_helicopter_get_com_offset( const wp_dcs_helicopter *h );

/* -------------------------------------------------------------------------
 * Per-tick update
 * ---------------------------------------------------------------------- */

/**
 * @brief Advance the flight-dynamics simulation by one fixed physics tick.
 *
 * Executes, in order:
 *  1. Rotor RPM      – integrates engine minus drag torque.
 *  2. Dynamic inflow – lerps induced_velocity toward the momentum-theory
 *                      target at a rate set by inflow_lag.
 *  3. Blade flapping – updates flapping_offset from local-space lateral
 *                      airspeed.
 *  4. Main rotor     – computes momentum-theory thrust, applies translational-
 *                      lift and retreating-blade-stall modifiers, tilts the
 *                      disc by cyclic + flapping, adds torque reaction.
 *  5. Tail rotor     – adds anti-torque and yaw pedal torque.
 *  6. SAS            – applies angular-damping counter-torque.
 *
 * Results are accumulated into net_force and net_torque.
 *
 * @param h  Pointer to the instance.
 * @param dt Fixed simulation time step in seconds (e.g. 0.02 for 50 Hz).
 */
void wp_dcs_helicopter_fixed_update( wp_dcs_helicopter *h, wp_f32 dt );

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
void wp_dcs_helicopter_set_inputs( wp_dcs_helicopter *h, wp_f32 throttle, wp_f32 collective,
                                   wp_f32 pitch, wp_f32 roll, wp_f32 yaw );

/**
 * @brief Set the current rigid-body state.
 *
 * Call before wp_dcs_helicopter_fixed_update() each tick with values read
 * from the physics engine.
 *
 * @param h                Pointer to the instance.
 * @param position         Current world-space position.
 * @param orientation      Current world-space orientation (unit quaternion).
 * @param linear_velocity  Current world-space linear velocity in m/s.
 * @param angular_velocity Current world-space angular velocity in rad/s.
 */
void wp_dcs_helicopter_set_state( wp_dcs_helicopter *h, wp_vec3f position, wp_quatf orientation,
                                  wp_vec3f linear_velocity, wp_vec3f angular_velocity );

/* -------------------------------------------------------------------------
 * Outputs
 * ---------------------------------------------------------------------- */

/**
 * @brief Return the current rotor speed.
 *
 * @param h Pointer to the instance.
 * @return  Rotor speed in RPM.
 */
wp_f32 wp_dcs_helicopter_get_rotor_rpm( const wp_dcs_helicopter *h );

/**
 * @brief Return the current lagged induced velocity.
 *
 * @param h Pointer to the instance.
 * @return  Induced velocity in m/s.
 */
wp_f32 wp_dcs_helicopter_get_induced_velocity( const wp_dcs_helicopter *h );

/**
 * @brief Return the current blade-flapping disc-tilt offset.
 *
 * Useful for visualising rotor disc tilt in a debug renderer.
 *
 * @param h Pointer to the instance.
 * @return  Flapping offset vector.
 */
wp_vec3f wp_dcs_helicopter_get_flapping_offset( const wp_dcs_helicopter *h );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_VEHICLE_DCS_HELICOPTER_H */
