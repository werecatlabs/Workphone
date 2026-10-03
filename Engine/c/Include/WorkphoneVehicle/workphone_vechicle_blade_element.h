/**
 * @file workphone_vechicle_blade_element.h
 * @brief C99 API for a blade-element-theory (BET) rotor simulation.
 *
 * Computes aerodynamic lift and drag for every blade segment each tick by
 * walking a two-level loop over blades and radial segments:
 *
 *   for each blade (azimuth angle = blade_index * 2π / blade_count):
 *     for each segment (radius = segment_index * segment_length):
 *       1. Compute the segment's local-space position via azimuth rotation.
 *       2. Transform to world space via the rotor orientation.
 *       3. Compute air velocity = tangential (rotational) + rigid-body
 *          powp_s32 velocity (linear + angular-velocity contribution).
 *       4. Derive angle of attack from collective pitch + cyclic mixing.
 *       5. Compute Cl / Cd using the thin-aerofoil linear lift model.
 *       6. Accumulate lift and drag into net_force and net_torque.
 *
 * The controller is physics-engine agnostic.  Results are written into
 * net_force and net_torque; the caller applies them to their rigid body.
 *
 * Typical usage:
 * @code
 *   wp_blade_element_rotor rotor;
 *   wp_blade_element_rotor_init( &rotor );
 *
 *   // Each fixed tick:
 *   wp_blade_element_rotor_set_rotor_state( &rotor, rpm, collective, cx, cy );
 *   wp_blade_element_rotor_set_state( &rotor, position, orientation,
 *                                     velocity, angular_velocity );
 *   wp_blade_element_rotor_fixed_update( &rotor, fixed_dt );
 *
 *   wp_rigidbody_add_force ( body, rotor.net_force,  WORKPHONE_FORCE_MODE_FORCE );
 *   wp_rigidbody_add_torque( body, rotor.net_torque, WORKPHONE_FORCE_MODE_FORCE );
 * @endcode
 *
 * @note  Call wp_blade_element_rotor_rebuild_derived() whenever rotor_radius
 *        or segments_per_blade is changed after initialisation.
 */

#ifndef WORKPHONE_VECHICLE_BLADE_ELEMENT_H
#define WORKPHONE_VECHICLE_BLADE_ELEMENT_H

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
 * @brief Complete state for one blade-element rotor instance.
 *
 * All fields are plain data; no heap allocation is required.
 * Initialise with wp_blade_element_rotor_init() before first use.
 */
typedef struct wp_blade_element_rotor
{
    /* -- rotor geometry ------------------------------------------------- */

    /**
     * @brief Number of rotor blades (default 4).
     *
     * Determines the azimuthal spacing: 2π / blade_count radians per blade.
     */
    wp_s32 blade_count;

    /**
     * @brief Number of radial integration segments per blade (default 12).
     *
     * More segments give higher accuracy at the cost of more computation.
     */
    wp_s32 segments_per_blade;

    /** Rotor disc radius in metres (default 6.5). */
    wp_f32 rotor_radius;

    /** Blade chord length in metres (default 0.35). */
    wp_f32 chord_length;

    /* -- aerodynamics --------------------------------------------------- */

    /** Air density in kg/m³ (default 1.225 — ISA sea level). */
    wp_f32 air_density;

    /**
     * @brief Two-dimensional blade lift slope dCl/dα per radian (default 5.7).
     *
     * Used in the linear thin-aerofoil approximation: Cl = lift_slope * α.
     */
    wp_f32 lift_slope;

    /**
     * @brief Profile drag coefficient at zero lift (default 0.01).
     *
     * Total drag is: Cd = drag_coeff + Cl² * 0.02 (induced drag term).
     */
    wp_f32 drag_coeff;

    /* -- rotor state (set each tick) ------------------------------------ */

    /** Rotor angular speed in RPM. */
    wp_f32 rotor_rpm;

    /**
     * @brief Symmetric collective blade pitch in degrees.
     *
     * Added to the cyclic-mixed pitch at every azimuth position.
     */
    wp_f32 collective_pitch;

    /**
     * @brief Cyclic pitch input.
     *
     * @c x is the longitudinal (pitch) cyclic demand;
     * @c y is the lateral (roll) cyclic demand.
     * Combined with @c collective_pitch via:
     *   blade_pitch = collective_pitch
     *               + cyclic_input.x * sin(blade_angle)
     *               + cyclic_input.y * cos(blade_angle)
     */
    wp_vec2f cyclic_input;

    /* -- derived (cached) ----------------------------------------------- */

    /**
     * @brief Length of each radial segment in metres.
     *
     * Cached as @c rotor_radius / @c segments_per_blade.
     * Recompute with wp_blade_element_rotor_rebuild_derived() if either
     * source field is changed after initialisation.
     */
    wp_f32 segment_length;

    /* -- rigid-body state (set each tick) ------------------------------- */

    /** World-space position of the rotor hub. */
    wp_vec3f position;

    /** World-space orientation of the rotor hub (unit quaternion). */
    wp_quatf orientation;

    /**
     * @brief World-space linear velocity of the rigid body in m/s.
     *
     * Combined with angular_velocity to compute per-segment powp_s32 velocity.
     */
    wp_vec3f velocity;

    /**
     * @brief World-space angular velocity of the rigid body in rad/s.
     *
     * Used to compute the rigid-body contribution to air velocity at each
     * blade segment (equivalent to rb.GetPointVelocity in Unity).
     */
    wp_vec3f angular_velocity;

    /* -- outputs (valid after wp_blade_element_rotor_fixed_update) ------ */

    /** Accumulated world-space aerodynamic force to apply to the body (N). */
    wp_vec3f net_force;

    /** Accumulated world-space aerodynamic torque to apply to the body (N·m). */
    wp_vec3f net_torque;
} wp_blade_element_rotor;

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Initialise a wp_blade_element_rotor with default parameters and
 *        zero runtime state.
 *
 * Also computes the cached segment_length.
 * Must be called before any other wp_blade_element_rotor function.
 *
 * @param r Pointer to the instance to initialise.
 */
void wp_blade_element_rotor_init( wp_blade_element_rotor *r );

/**
 * @brief Recompute derived fields after geometry has been changed.
 *
 * Must be called if rotor_radius or segments_per_blade are modified after
 * wp_blade_element_rotor_init() has already been called.
 *
 * @param r Pointer to the instance to update.
 */
void wp_blade_element_rotor_rebuild_derived( wp_blade_element_rotor *r );

/* -------------------------------------------------------------------------
 * Per-tick update
 * ---------------------------------------------------------------------- */

/**
 * @brief Run one blade-element integration pass and accumulate forces.
 *
 * Iterates over every blade and every radial segment, computing the local
 * air velocity, angle of attack, lift and drag at each element, and
 * summing the results into net_force and net_torque.
 *
 * @param r  Pointer to the instance.
 * @param dt Fixed simulation time step in seconds (reserved for future use;
 *           the BET integration itself is not time-stepped).
 */
void wp_blade_element_rotor_fixed_update( wp_blade_element_rotor *r, wp_f32 dt );

/* -------------------------------------------------------------------------
 * Inputs
 * ---------------------------------------------------------------------- */

/**
 * @brief Set the rotor control state for the next tick.
 *
 * @param r               Pointer to the instance.
 * @param rpm             Rotor angular speed in RPM.
 * @param collective_pitch Symmetric collective pitch in degrees.
 * @param cyclic_x        Longitudinal cyclic demand (pitch).
 * @param cyclic_y        Lateral cyclic demand (roll).
 */
void wp_blade_element_rotor_set_rotor_state( wp_blade_element_rotor *r, wp_f32 rpm,
                                             wp_f32 collective_pitch, wp_f32 cyclic_x, wp_f32 cyclic_y );

/**
 * @brief Set the current rigid-body state used for point-velocity calculation.
 *
 * Call before wp_blade_element_rotor_fixed_update() each tick with values
 * read from the physics engine.
 *
 * @param r                Pointer to the instance.
 * @param position         World-space hub position.
 * @param orientation      World-space hub orientation (unit quaternion).
 * @param velocity         World-space linear velocity of the body in m/s.
 * @param angular_velocity World-space angular velocity of the body in rad/s.
 */
void wp_blade_element_rotor_set_state( wp_blade_element_rotor *r, wp_vec3f position,
                                       wp_quatf orientation, wp_vec3f velocity,
                                       wp_vec3f angular_velocity );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_VECHICLE_BLADE_ELEMENT_H */
