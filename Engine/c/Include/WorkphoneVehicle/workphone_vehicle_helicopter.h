/**
 * @file workphone_vehicle_helicopter.h
 * @brief C99 API for a helicopter flight-dynamics controller.
 *
 * Simulates the four primary flight forces of a single-rotor helicopter:
 *   - Collective lift, scaled by engine throttle and a ground-effect factor.
 *   - Cyclic pitch and roll tilt forces applied as body torques.
 *   - Tail-rotor yaw torque.
 *   - Passive self-levelling stabilisation torque.
 *
 * The controller is physics-engine agnostic.  It writes its results into
 * @c net_force and @c net_torque each tick; the caller reads those values
 * and applies them to whatever rigid-body implementation they use.
 *
 * The ground-effect distance is similarly decoupled: the caller performs
 * their own downward raycast and passes the result in via
 * wp_helicopter_set_ground_distance().  Pass @c WP_HELICOPTER_NO_GROUND
 * when no ground is detected within range.
 *
 * Typical usage:
 * @code
 *   wp_helicopter heli;
 *   wp_helicopter_init( &heli );
 *
 *   // Apply the recommended centre-of-mass offset at startup:
 *   wp_vec3f com = wp_helicopter_get_com_offset( &heli );
 *   wp_rigidbody_set_cmass_local_position( body, com );
 *
 *   // Each variable-rate tick (input + throttle spool):
 *   wp_helicopter_set_inputs( &heli, collective, pitch, roll, yaw );
 *   wp_helicopter_update( &heli, dt );
 *
 *   // Each fixed-rate physics tick:
 *   wp_helicopter_set_transform( &heli, position, orientation );
 *   wp_helicopter_set_ground_distance( &heli, raycast_dist );
 *   wp_helicopter_fixed_update( &heli, fixed_dt );
 *
 *   wp_rigidbody_add_force ( body, heli.net_force,  WORKPHONE_FORCE_MODE_FORCE );
 *   wp_rigidbody_add_torque( body, heli.net_torque, WORKPHONE_FORCE_MODE_FORCE );
 * @endcode
 */

#ifndef WORKPHONE_VEHICLE_HELICOPTER_H
#define WORKPHONE_VEHICLE_HELICOPTER_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_vector.h"
#include "workphone_quat.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sentinel value for wp_helicopter::ground_distance indicating that
 *        no ground surface was found within the ground-effect radius.
 */
#define WP_HELICOPTER_NO_GROUND ( 1e30f )

/* -------------------------------------------------------------------------
 * Helicopter state structure
 * ---------------------------------------------------------------------- */

/**
 * @brief Complete state for one helicopter flight-dynamics instance.
 *
 * All fields are plain data; no heap allocation is required.
 * Initialise with wp_helicopter_init() before first use.
 */
typedef struct wp_helicopter
{
    /* -- tunable parameters -------------------------------------------- */

    /**
     * @brief Peak engine power in Watts (default 30 000).
     *
     * Reserved for future power-limiting calculations; not consumed by the
     * current lift model which uses lift_power directly.
     */
    wp_f32 engine_power;

    /** Throttle spool-up speed used as the lerp rate (default 0.5). */
    wp_f32 spool_up_speed;

    /** Lift force magnitude at full throttle in Newtons (default 4 000). */
    wp_f32 lift_power;

    /**
     * @brief Height above ground at which the ground effect becomes active,
     *        in metres (default 8).
     */
    wp_f32 ground_effect_height;

    /** Cyclic pitch torque magnitude in N·m (default 800). */
    wp_f32 pitch_force;

    /** Cyclic roll torque magnitude in N·m (default 800). */
    wp_f32 roll_force;

    /** Tail-rotor yaw torque magnitude in N·m (default 600). */
    wp_f32 yaw_torque;

    /** Self-levelling stabilisation gain (default 3). */
    wp_f32 stabilization;

    /* -- runtime state -------------------------------------------------- */

    /**
     * @brief Current engine throttle position [0, 1].
     *
     * Lerped toward collective_input each variable-rate tick.
     */
    wp_f32 engine_throttle;

    /* -- inputs (set before each update) -------------------------------- */

    /** Collective (lift) demand in [0, 1]. */
    wp_f32 collective_input;

    /** Cyclic pitch demand in [-1, 1]; positive nose-up. */
    wp_f32 pitch_input;

    /** Cyclic roll demand in [-1, 1]; positive right. */
    wp_f32 roll_input;

    /** Yaw pedal demand in [-1, 1]; positive nose-right. */
    wp_f32 yaw_input;

    /* -- transform (set before each fixed update) ----------------------- */

    /** World-space position of the helicopter in metres. */
    wp_vec3f position;

    /**
     * @brief World-space orientation of the helicopter.
     *
     * Used to derive the local right, up, and forward axes.
     * Initialised to identity by wp_helicopter_init().
     */
    wp_quatf orientation;

    /* -- environment input ---------------------------------------------- */

    /**
     * @brief Distance from position to the nearest ground surface in metres.
     *
     * Caller should perform a downward raycast each fixed tick and write the
     * hit distance here.  Set to WP_HELICOPTER_NO_GROUND when no surface is
     * within range.
     */
    wp_f32 ground_distance;

    /* -- outputs (valid after wp_helicopter_fixed_update) --------------- */

    /** Accumulated world-space force to apply to the rigid body (N). */
    wp_vec3f net_force;

    /** Accumulated world-space torque to apply to the rigid body (N·m). */
    wp_vec3f net_torque;
} wp_helicopter;

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Initialise a wp_helicopter with default parameters and zero state.
 *
 * Must be called before any other wp_helicopter function.
 *
 * @param h Pointer to the helicopter instance to initialise.
 */
void wp_helicopter_init( wp_helicopter *h );

/* -------------------------------------------------------------------------
 * Setup helpers
 * ---------------------------------------------------------------------- */

/**
 * @brief Return the recommended local centre-of-mass offset.
 *
 * Apply this to the rigid body once at startup to lower the centre of mass
 * and improve roll stability (mirrors Unity's @c rb.centerOfMass assignment
 * in the original C# Start()).
 *
 * @param h Pointer to the helicopter instance.
 * @return  Local-space centre-of-mass offset vector.
 */
wp_vec3f wp_helicopter_get_com_offset( const wp_helicopter *h );

/* -------------------------------------------------------------------------
 * Per-tick updates
 * ---------------------------------------------------------------------- */

/**
 * @brief Advance the throttle spool simulation (variable-rate tick).
 *
 * Lerps engine_throttle toward collective_input at spool_up_speed.
 * Call once per variable-rate game loop iteration.
 *
 * @param h  Pointer to the helicopter instance.
 * @param dt Variable time step in seconds.
 */
void wp_helicopter_update( wp_helicopter *h, wp_f32 dt );

/**
 * @brief Compute flight forces for one fixed physics tick.
 *
 * Executes, in order:
 *  1. Lift         – collective lift scaled by engine throttle and ground effect.
 *  2. Tilt         – cyclic pitch and roll torques.
 *  3. Yaw          – tail-rotor yaw torque.
 *  4. Stabilisation – passive self-levelling torque.
 *
 * Results are written to net_force and net_torque.  Apply both to the rigid
 * body after this call returns.
 *
 * @param h  Pointer to the helicopter instance.
 * @param dt Fixed simulation time step in seconds (e.g. 0.02 for 50 Hz).
 */
void wp_helicopter_fixed_update( wp_helicopter *h, wp_f32 dt );

/* -------------------------------------------------------------------------
 * Inputs
 * ---------------------------------------------------------------------- */

/**
 * @brief Set all pilot control inputs for the next tick.
 *
 * @param h          Pointer to the helicopter instance.
 * @param collective Lift demand, clamped to [0, 1].
 * @param pitch      Cyclic pitch demand in [-1, 1].
 * @param roll       Cyclic roll demand in [-1, 1].
 * @param yaw        Yaw pedal demand in [-1, 1].
 */
void wp_helicopter_set_inputs( wp_helicopter *h, wp_f32 collective, wp_f32 pitch, wp_f32 roll,
                               wp_f32 yaw );

/**
 * @brief Update the world-space transform used for axis derivation.
 *
 * Call before wp_helicopter_fixed_update() each physics tick with the
 * current position and orientation read back from the rigid body.
 *
 * @param h           Pointer to the helicopter instance.
 * @param position    Current world-space position.
 * @param orientation Current world-space orientation (unit quaternion).
 */
void wp_helicopter_set_transform( wp_helicopter *h, wp_vec3f position, wp_quatf orientation );

/**
 * @brief Provide the downward raycast distance for ground-effect calculation.
 *
 * @param h        Pointer to the helicopter instance.
 * @param distance Distance to the nearest ground surface in metres, or
 *                 WP_HELICOPTER_NO_GROUND if no hit within range.
 */
void wp_helicopter_set_ground_distance( wp_helicopter *h, wp_f32 distance );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_VEHICLE_HELICOPTER_H */
