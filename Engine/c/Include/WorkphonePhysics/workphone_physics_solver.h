/**
 * @file wp_physics_solver.h
 * @brief C API for physics solver configuration.
 *
 * A solver configuration controls how the physics engine resolves
 * constraints and contacts. It specifies iteration counts, timestep
 * policy, broadphase type, continuous-collision-detection settings,
 * and tolerance values. A wp_solver_config is attached to a
 * wp_physics_scene before simulation begins.
 */

#ifndef WORKPHONE_PHYSICS_SOLVER_H
#define WORKPHONE_PHYSICS_SOLVER_H

#include <stdint.h>
#include "workphone_vector.h"
#include "workphone_physics_broadphase.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

typedef struct wp_solver_config wp_solver_config;

/* -------------------------------------------------------------------------
 * Broadphase type
 * ---------------------------------------------------------------------- */

/* -------------------------------------------------------------------------
 * Solver type
 * ---------------------------------------------------------------------- */

typedef enum wp_solver_type
{
    WORKPHONE_SOLVER_PGS = 0, /**< Projected Gauss-Seidel (iterative). */
    WORKPHONE_SOLVER_TGS = 1  /**< Temporal Gauss-Seidel.               */
} wp_solver_type;

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_solver_config *wp_solver_config_create( void );
void wp_solver_config_destroy( wp_solver_config *cfg );

/* =========================================================================
 * Solver iterations
 * ====================================================================== */

wp_u32 wp_solver_config_get_position_iterations( const wp_solver_config *cfg );
void wp_solver_config_set_position_iterations( wp_solver_config *cfg, wp_u32 iters );

wp_u32 wp_solver_config_get_velocity_iterations( const wp_solver_config *cfg );
void wp_solver_config_set_velocity_iterations( wp_solver_config *cfg, wp_u32 iters );

/* =========================================================================
 * Timestep
 * ====================================================================== */

wp_f32 wp_solver_config_get_fixed_timestep( const wp_solver_config *cfg );
void wp_solver_config_set_fixed_timestep( wp_solver_config *cfg, wp_f32 dt );

wp_f32 wp_solver_config_get_max_timestep( const wp_solver_config *cfg );
void wp_solver_config_set_max_timestep( wp_solver_config *cfg, wp_f32 dt );

wp_u32 wp_solver_config_get_max_substeps( const wp_solver_config *cfg );
void wp_solver_config_set_max_substeps( wp_solver_config *cfg, wp_u32 substeps );

/* =========================================================================
 * Solver and broadphase type
 * ====================================================================== */

wp_solver_type wp_solver_config_get_solver_type( const wp_solver_config *cfg );
void wp_solver_config_set_solver_type( wp_solver_config *cfg, wp_solver_type type );

wp_broadphase_type wp_solver_config_get_broadphase( const wp_solver_config *cfg );
void wp_solver_config_set_broadphase( wp_solver_config *cfg, wp_broadphase_type type );

/* =========================================================================
 * Continuous collision detection (CCD)
 * ====================================================================== */

wp_s32 wp_solver_config_get_ccd_enabled( const wp_solver_config *cfg );
void wp_solver_config_set_ccd_enabled( wp_solver_config *cfg, wp_s32 enabled );

/* =========================================================================
 * Tolerances
 * ====================================================================== */

wp_f32 wp_solver_config_get_bounce_threshold( const wp_solver_config *cfg );
void wp_solver_config_set_bounce_threshold( wp_solver_config *cfg, wp_f32 threshold );

wp_f32 wp_solver_config_get_sleep_threshold( const wp_solver_config *cfg );
void wp_solver_config_set_sleep_threshold( wp_solver_config *cfg, wp_f32 threshold );

wp_f32 wp_solver_config_get_contact_offset( const wp_solver_config *cfg );
void wp_solver_config_set_contact_offset( wp_solver_config *cfg, wp_f32 offset );

wp_f32 wp_solver_config_get_rest_offset( const wp_solver_config *cfg );
void wp_solver_config_set_rest_offset( wp_solver_config *cfg, wp_f32 offset );

/* =========================================================================
 * Gravity shortcut (per-config override)
 * ====================================================================== */

wp_vec3f wp_solver_config_get_gravity( const wp_solver_config *cfg );
void wp_solver_config_set_gravity( wp_solver_config *cfg, wp_vec3f gravity );

/* =========================================================================
 * Native / user data
 * ====================================================================== */

void *wp_solver_config_get_native( const wp_solver_config *cfg );
void wp_solver_config_set_native( wp_solver_config *cfg, void *native );
void *wp_solver_config_get_user_data( const wp_solver_config *cfg );
void wp_solver_config_set_user_data( wp_solver_config *cfg, void *user_data );

#ifdef __cplusplus
}
#endif

#endif
