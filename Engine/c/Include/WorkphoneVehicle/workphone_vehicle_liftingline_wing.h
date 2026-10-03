/**
 * @file workphone_vehicle_liftingline_wing.h
 * @brief C99 API for a lifting-line wing model.
 *
 * Models a finite-span wing using a multi-section lifting-line approach.
 * Each section computes local wind, angle of attack, lift, and drag.
 * Results are accumulated into net_force and net_torque for the caller
 * to apply to their rigid body.
 *
 * Typical usage:
 * @code
 *   wp_liftingline_wing wing;
 *   wp_liftingline_wing_init(&wing);
 *
 *   // Set geometry and state each tick:
 *   wp_liftingline_wing_set_geometry(&wing, span, chord, section_count);
 *   wp_liftingline_wing_set_state(&wing, pos, orientation, velocity, angular_velocity);
 *
 *   // Each physics tick:
 *   wp_liftingline_wing_fixed_update(&wing, dt);
 *
 *   // Apply wing.net_force and wing.net_torque to your rigid body.
 * @endcode
 */

#ifndef WORKPHONE_VEHICLE_LIFTINGLINE_WING_H
#define WORKPHONE_VEHICLE_LIFTINGLINE_WING_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_vector.h"
#include "workphone_quat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Compile-time capacity
 * ---------------------------------------------------------------------- */

/** Maximum number of sections for the lifting-line wing. */
#ifndef WP_LIFTINGLINE_WING_MAX_SECTIONS
#    define WP_LIFTINGLINE_WING_MAX_SECTIONS 16
#endif

/* -------------------------------------------------------------------------
 * Wing state structure
 * ---------------------------------------------------------------------- */

/**
 * @brief Complete state for one lifting-line wing instance.
 *
 * All fields are plain data; no heap allocation is required.
 * Initialise with wp_liftingline_wing_init() before first use.
 */
typedef struct wp_liftingline_wing
{
    /* -- geometry -- */
    wp_f32 span;          /**< Wing span in metres. */
    wp_f32 chord_length;  /**< Chord length in metres. */
    wp_f32 area;          /**< Total wing area in m². */
    wp_s32 section_count; /**< Number of spanwise sections. */

    /* -- aerodynamics -- */
    wp_f32 air_density; /**< Air density in kg/m³. */
    wp_f32 lift_slope;  /**< dCl/dα per radian (default: 2π). */
    wp_f32 cd_override; /**< Profile drag coefficient (default: 0.045). */

    /* -- rigid-body state -- */
    wp_vec3f position;         /**< World-space position of wing root. */
    wp_quatf orientation;      /**< World-space orientation. */
    wp_vec3f velocity;         /**< World-space linear velocity. */
    wp_vec3f angular_velocity; /**< World-space angular velocity. */

    /* -- outputs -- */
    wp_vec3f net_force;  /**< Accumulated aerodynamic force (N). */
    wp_vec3f net_torque; /**< Accumulated aerodynamic torque (N·m). */
} wp_liftingline_wing;

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Initialise a wp_liftingline_wing with default parameters and zero state.
 *
 * Must be called before any other wp_liftingline_wing function.
 *
 * @param w Pointer to the wing instance to initialise.
 */
void wp_liftingline_wing_init( wp_liftingline_wing *w );

/* -------------------------------------------------------------------------
 * Geometry
 * ---------------------------------------------------------------------- */

/**
 * @brief Set the wing geometry (span, chord, section count).
 *
 * @param w             Pointer to the wing instance.
 * @param span          Wing span in metres.
 * @param chord_length  Chord length in metres.
 * @param section_count Number of spanwise sections.
 */
void wp_liftingline_wing_set_geometry( wp_liftingline_wing *w, wp_f32 span, wp_f32 chord_length,
                                       wp_s32 section_count );

/* -------------------------------------------------------------------------
 * State
 * ---------------------------------------------------------------------- */

/**
 * @brief Set the current rigid-body state for aerodynamic calculations.
 *
 * @param w               Pointer to the wing instance.
 * @param position        World-space position of the wing root.
 * @param orientation     World-space orientation (unit quaternion).
 * @param velocity        World-space linear velocity in m/s.
 * @param angular_velocity World-space angular velocity in rad/s.
 */
void wp_liftingline_wing_set_state( wp_liftingline_wing *w, wp_vec3f position, wp_quatf orientation,
                                    wp_vec3f velocity, wp_vec3f angular_velocity );

/* -------------------------------------------------------------------------
 * Per-tick update
 * ---------------------------------------------------------------------- */

/**
 * @brief Advance the lifting-line simulation by one physics tick.
 *
 * Computes local wind, angle of attack, lift, and drag for each section,
 * and accumulates results into net_force and net_torque.
 *
 * @param w  Pointer to the wing instance.
 * @param dt Fixed simulation time step in seconds (e.g. 0.02 for 50 Hz).
 */
void wp_liftingline_wing_fixed_update( wp_liftingline_wing *w, wp_f32 dt );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_VEHICLE_LIFTINGLINE_WING_H */
