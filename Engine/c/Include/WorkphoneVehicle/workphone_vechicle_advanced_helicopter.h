/**
 * @file workphone_vechicle_advanced_helicopter.h
 * @brief C99 API for an advanced helicopter flight-dynamics controller.
 *
 * Provides a mid-complexity helicopter model between the basic and
 * DCS-grade implementations:
 *
 *   - Physical rotor RPM dynamics (engine torque vs aerodynamic drag).
 *   - Lift formula based on rotor disc area and rotational speed squared,
 *     scaled by collective blade pitch and a lift coefficient.
 *   - Cyclic disc tilt applied to the thrust vector.
 *   - Tail-rotor anti-torque and yaw pedal control.
 *   - Ground-effect multiplier (caller-provided distance).
 *   - Effective Translational Lift (ETL) — a smooth boost that ramps up
 *     as airspeed increases above etl_start_speed.
 *
 * The controller is physics-engine agnostic.  Results are written into
 * net_force and net_torque; the caller applies them to their rigid body.
 *
 * Typical usage:
 * @code
 *   wp_advanced_helicopter heli;
 *   wp_advanced_helicopter_init( &heli );
 *
 *   // Apply recommended CoM offset once at startup:
 *   wp_vec3f com = wp_advanced_helicopter_get_com_offset( &heli );
 *   wp_rigidbody_set_cmass_local_position( body, com );
 *
 *   // Each fixed tick:
 *   wp_advanced_helicopter_set_inputs( &heli, throttle, collective,
 *                                      pitch, roll, yaw );
 *   wp_advanced_helicopter_set_state( &heli, position, orientation,
 *                                     velocity, ground_dist );
 *   wp_advanced_helicopter_fixed_update( &heli, fixed_dt );
 *
 *   wp_rigidbody_add_force ( body, heli.net_force,  WORKPHONE_FORCE_MODE_FORCE );
 *   wp_rigidbody_add_torque( body, heli.net_torque, WORKPHONE_FORCE_MODE_FORCE );
 * @endcode
 */

#ifndef WORKPHONE_VECHICLE_ADVANCED_HELICOPTER_H
#define WORKPHONE_VECHICLE_ADVANCED_HELICOPTER_H

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
#define WP_ADVANCED_HELI_NO_GROUND ( 1e30f )

/* -------------------------------------------------------------------------
 * State structure
 * ---------------------------------------------------------------------- */

/**
 * @brief Complete state for one advanced-helicopter flight-dynamics instance.
 *
 * All fields are plain data; no heap allocation is required.
 * Initialise with wp_advanced_helicopter_init() before first use.
 */
typedef struct wp_advanced_helicopter
{
    /* -- rotor physics -------------------------------------------------- */

    /** Rotor rotational inertia in kg·m² (default 120). */
    wp_f32 rotor_inertia;

    /** Peak engine torque delivered to the rotor shaft in N·m (default 5 000). */
    wp_f32 max_engine_torque;

    /**
     * @brief Aerodynamic rotor drag coefficient (default 15).
     *
     * drag_torque = rotor_drag * rotor_rpm.
     */
    wp_f32 rotor_drag;

    /** Maximum allowable rotor speed in RPM (default 450). */
    wp_f32 max_rotor_rpm;

    /* -- lift ----------------------------------------------------------- */

    /** Rotor disc area in m² (default 120). */
    wp_f32 rotor_area;

    /** Air density in kg/m³ (default 1.225 — ISA sea level). */
    wp_f32 air_density;

    /**
     * @brief Rotor lift coefficient (default 0.6).
     *
     * Scales the aerodynamic lift together with the collective pitch angle.
     */
    wp_f32 lift_coefficient;

    /* -- blade pitch ---------------------------------------------------- */

    /** Maximum collective blade pitch angle in degrees (default 15). */
    wp_f32 max_collective_pitch;

    /** Maximum cyclic disc tilt from neutral in degrees (default 10). */
    wp_f32 cyclic_tilt_angle;

    /* -- tail rotor ----------------------------------------------------- */

    /** Tail-rotor yaw torque magnitude at full pedal in N·m (default 2 000). */
    wp_f32 tail_rotor_force;

    /* -- ground effect -------------------------------------------------- */

    /**
     * @brief Height above ground below which ground effect is active,
     *        in metres (default 10).
     */
    wp_f32 ground_effect_height;

    /* -- effective translational lift (ETL) ----------------------------- */

    /**
     * @brief Airspeed at which ETL begins in m/s (default 8).
     *
     * Below this speed the ETL multiplier is 1.0.  It ramps to
     * etl_max_boost over the range [etl_start_speed, etl_start_speed * 3].
     */
    wp_f32 etl_start_speed;

    /**
     * @brief Maximum ETL lift multiplier reached at etl_start_speed * 3
     *        (default 1.35).
     */
    wp_f32 etl_max_boost;

    /* -- runtime state -------------------------------------------------- */

    /** Current rotor speed in RPM. */
    wp_f32 rotor_rpm;

    /**
     * @brief Current engine torque in N·m.
     *
     * Set each tick by wp_advanced_helicopter_fixed_update(); readable for
     * instrumentation or HUD display.
     */
    wp_f32 engine_torque;

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
     * Used for the ETL calculation.
     */
    wp_vec3f velocity;

    /**
     * @brief Distance from position to the nearest ground surface in metres.
     *
     * Set to WP_ADVANCED_HELI_NO_GROUND when no ground is within range.
     */
    wp_f32 ground_distance;

    /* -- outputs (valid after wp_advanced_helicopter_fixed_update) ------ */

    /** Accumulated world-space force to apply to the rigid body (N). */
    wp_vec3f net_force;

    /** Accumulated world-space torque to apply to the rigid body (N·m). */
    wp_vec3f net_torque;
} wp_advanced_helicopter;

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Initialise a wp_advanced_helicopter with default parameters and
 *        zero runtime state.
 *
 * Must be called before any other wp_advanced_helicopter function.
 *
 * @param h Pointer to the instance to initialise.
 */
void wp_advanced_helicopter_init( wp_advanced_helicopter *h );

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
wp_vec3f wp_advanced_helicopter_get_com_offset( const wp_advanced_helicopter *h );

/* -------------------------------------------------------------------------
 * Per-tick update
 * ---------------------------------------------------------------------- */

/**
 * @brief Advance the flight-dynamics simulation by one fixed physics tick.
 *
 * Executes, in order:
 *  1. Rotor RPM  – integrates engine minus drag torque.
 *  2. Lift       – computes disc-area lift scaled by RPM², collective pitch,
 *                  ground effect and ETL; tilts thrust vector by cyclic
 *                  input; adds torque reaction.
 *  3. Tail rotor – adds anti-torque and yaw pedal torque.
 *
 * Results are accumulated into net_force and net_torque.
 *
 * @param h  Pointer to the instance.
 * @param dt Fixed simulation time step in seconds (e.g. 0.02 for 50 Hz).
 */
void wp_advanced_helicopter_fixed_update( wp_advanced_helicopter *h, wp_f32 dt );

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
void wp_advanced_helicopter_set_inputs( wp_advanced_helicopter *h, wp_f32 throttle, wp_f32 collective,
                                        wp_f32 pitch, wp_f32 roll, wp_f32 yaw );

/**
 * @brief Set the current rigid-body state used for aerodynamic modifiers.
 *
 * Call before wp_advanced_helicopter_fixed_update() each tick with values
 * read from the physics engine.
 *
 * @param h            Pointer to the instance.
 * @param position     Current world-space position.
 * @param orientation  Current world-space orientation (unit quaternion).
 * @param velocity     Current world-space linear velocity in m/s.
 * @param ground_dist  Distance to nearest ground surface in metres, or
 *                     WP_ADVANCED_HELI_NO_GROUND if none within range.
 */
void wp_advanced_helicopter_set_state( wp_advanced_helicopter *h, wp_vec3f position,
                                       wp_quatf orientation, wp_vec3f velocity, wp_f32 ground_dist );

/* -------------------------------------------------------------------------
 * Outputs
 * ---------------------------------------------------------------------- */

/**
 * @brief Return the current rotor speed.
 *
 * @param h Pointer to the instance.
 * @return  Rotor speed in RPM.
 */
wp_f32 wp_advanced_helicopter_get_rotor_rpm( const wp_advanced_helicopter *h );

/**
 * @brief Return the current engine torque.
 *
 * @param h Pointer to the instance.
 * @return  Engine torque in N·m.
 */
wp_f32 wp_advanced_helicopter_get_engine_torque( const wp_advanced_helicopter *h );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_VECHICLE_ADVANCED_HELICOPTER_H */
