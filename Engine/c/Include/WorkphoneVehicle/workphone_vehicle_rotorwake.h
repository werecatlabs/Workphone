/**
 * @file workphone_vehicle_rotorwake.h
 * @brief C99 API for a rotor wake vortex particle simulation.
 *
 * Simulates the downwash and tip-vortex structure shed by a spinning rotor
 * disc using a fixed-capacity particle pool.  Each tick the system:
 *   - Spawns new particles beneath the rotor disc at a rate proportional to
 *     rotor RPM.
 *   - Advects existing particles: integrates their velocity, applies a
 *     spiral vortex swirl force, and expires aged particles.
 *   - Accumulates an induced turbulence force for the caller to apply to
 *     their rigid body.
 *
 * Typical usage:
 * @code
 *   wp_rotor_wake wake;
 *   wp_rotor_wake_init( &wake );
 *
 *   // each simulation tick:
 *   wp_rotor_wake_set_rotor_rpm( &wake, current_rpm );
 *   wp_rotor_wake_set_transform( &wake, pos, right, forward, up );
 *   wp_rotor_wake_update( &wake, fixed_dt );
 *
 *   wp_vec3f force = wp_rotor_wake_get_induced_force( &wake );
 *   // apply force to rigid body ...
 * @endcode
 *
 * @note  The struct embeds the particle pool directly; its size is
 *        approximately WP_ROTOR_WAKE_MAX_PARTICLES * 32 bytes.  Prefer
 *        static, global, or heap allocation over the stack.
 */

#ifndef WORKPHONE_VEHICLE_ROTORWAKE_H
#define WORKPHONE_VEHICLE_ROTORWAKE_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Compile-time capacity
 * ---------------------------------------------------------------------- */

/**
 * @brief Hard upper bound on the number of wake particles in the pool.
 *
 * Override before including this header if a different capacity is needed.
 */
#ifndef WP_ROTOR_WAKE_MAX_PARTICLES
#    define WP_ROTOR_WAKE_MAX_PARTICLES 300
#endif

/* -------------------------------------------------------------------------
 * Particle type
 * ---------------------------------------------------------------------- */

/**
 * @brief A single vortex wake particle.
 */
typedef struct wp_rotor_wake_particle
{
    /** World-space position in metres. */
    wp_vec3f position;

    /** Current velocity in m/s. */
    wp_vec3f velocity;

    /** Remaining lifetime in seconds. */
    wp_f32 life;

    /** Non-zero when this slot is occupied by a live particle. */
    wp_s32 active;
} wp_rotor_wake_particle;

/* -------------------------------------------------------------------------
 * Simulation state
 * ---------------------------------------------------------------------- */

/**
 * @brief Complete state for one rotor wake simulation instance.
 *
 * No heap allocation is required; the particle pool is embedded directly.
 * Initialise with wp_rotor_wake_init() before first use.
 */
typedef struct wp_rotor_wake
{
    /* -- tunable parameters -------------------------------------------- */

    /**
     * @brief Active pool capacity (<= WP_ROTOR_WAKE_MAX_PARTICLES).
     *
     * Default: WP_ROTOR_WAKE_MAX_PARTICLES (300).
     */
    wp_s32 max_particles;

    /** Lifetime of each spawned particle in seconds (default 5). */
    wp_f32 particle_lifetime;

    /** Initial downward speed assigned to spawned particles in m/s (default 40). */
    wp_f32 downwash_strength;

    /** Vortex swirl acceleration magnitude applied each tick (default 15). */
    wp_f32 vortex_strength;

    /** Rotor disc radius in metres (default 6.5). */
    wp_f32 rotor_radius;

    /* -- inputs (set each tick before wp_rotor_wake_update) ------------ */

    /** Current rotor speed in RPM; controls the per-tick spawn rate. */
    wp_f32 rotor_rpm;

    /** World-space position of the rotor hub. */
    wp_vec3f body_position;

    /** World-space right axis of the rotor hub (unit vector). */
    wp_vec3f body_right;

    /** World-space forward axis of the rotor hub (unit vector). */
    wp_vec3f body_forward;

    /** World-space up axis of the rotor hub (unit vector). */
    wp_vec3f body_up;

    /* -- output --------------------------------------------------------- */

    /**
     * @brief Accumulated induced turbulence force for this tick (N).
     *
     * Apply this to the helicopter rigid body after each call to
     * wp_rotor_wake_update().
     */
    wp_vec3f induced_force;

    /* -- internal ring buffer ------------------------------------------ */

    /** Index of the next write slot; advances modulo max_particles. */
    wp_s32 head;

    /** Number of currently active (live) particles. */
    wp_s32 count;

    /** Fixed-size particle pool. */
    wp_rotor_wake_particle particles[WP_ROTOR_WAKE_MAX_PARTICLES];
} wp_rotor_wake;

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Initialise a wp_rotor_wake with default parameters and an empty pool.
 *
 * Must be called before any other wp_rotor_wake function.
 *
 * @param w Pointer to the rotor wake instance to initialise.
 */
void wp_rotor_wake_init( wp_rotor_wake *w );

/* -------------------------------------------------------------------------
 * Per-tick update
 * ---------------------------------------------------------------------- */

/**
 * @brief Advance the rotor wake simulation by one time step.
 *
 * Executes, in order:
 *  1. Spawn  – emits new particles based on rotor_rpm.
 *  2. Advect – integrates velocity, applies vortex swirl, ages and expires.
 *  3. Force  – accumulates induced_force from nearby active particles.
 *
 * @param w  Pointer to the rotor wake instance.
 * @param dt Fixed simulation time step in seconds (e.g. 0.02 for 50 Hz).
 */
void wp_rotor_wake_update( wp_rotor_wake *w, wp_f32 dt );

/* -------------------------------------------------------------------------
 * Inputs
 * ---------------------------------------------------------------------- */

/**
 * @brief Set the current rotor speed.
 *
 * @param w   Pointer to the rotor wake instance.
 * @param rpm Rotor speed in RPM.
 */
void wp_rotor_wake_set_rotor_rpm( wp_rotor_wake *w, wp_f32 rpm );

/**
 * @brief Set the world-space transform of the rotor hub.
 *
 * All three axis vectors should be unit-length and mutually orthogonal.
 *
 * @param w        Pointer to the rotor wake instance.
 * @param position World-space hub position.
 * @param right    World-space right axis.
 * @param forward  World-space forward axis.
 * @param up       World-space up axis.
 */
void wp_rotor_wake_set_transform( wp_rotor_wake *w, wp_vec3f position, wp_vec3f right, wp_vec3f forward,
                                  wp_vec3f up );

/* -------------------------------------------------------------------------
 * Outputs
 * ---------------------------------------------------------------------- */

/**
 * @brief Return the induced turbulence force accumulated during the last update.
 *
 * @param w Pointer to the rotor wake instance.
 * @return  Force vector in Newtons.
 */
wp_vec3f wp_rotor_wake_get_induced_force( const wp_rotor_wake *w );

/**
 * @brief Return the number of currently live wake particles.
 *
 * @param w Pointer to the rotor wake instance.
 * @return  Active particle count in [0, max_particles].
 */
wp_s32 wp_rotor_wake_get_particle_count( const wp_rotor_wake *w );

/**
 * @brief Provide read-only access to the particle pool for debug rendering.
 *
 * Iterate up to @p *out_capacity entries; only slots where `active != 0`
 * hold valid data.
 *
 * @param w            Pointer to the rotor wake instance.
 * @param out_capacity Filled with the pool capacity (max_particles).
 *                     May be NULL.
 * @return             Pointer to the first entry in the particle pool.
 */
const wp_rotor_wake_particle *wp_rotor_wake_get_particles( const wp_rotor_wake *w,
                                                           wp_s32 *out_capacity );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_VEHICLE_ROTORWAKE_H */
