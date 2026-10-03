/**
 * @file wp_physics_solver.c
 * @brief Implementation of the C physics solver configuration API.
 */

#include "workphone_physics_solver.h"
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal structure
 * ====================================================================== */

typedef struct wp_solver_config
{
    /* Iterations */
    wp_u32 position_iterations;
    wp_u32 velocity_iterations;

    /* Timestep */
    wp_f32 fixed_timestep;
    wp_f32 max_timestep;
    wp_u32 max_substeps;

    /* Types */
    wp_solver_type solver_type;
    wp_broadphase_type broadphase;

    /* CCD */
    wp_s32 ccd_enabled;

    /* Tolerances */
    wp_f32 bounce_threshold;
    wp_f32 sleep_threshold;
    wp_f32 contact_offset;
    wp_f32 rest_offset;

    /* Gravity */
    wp_vec3f gravity;

    /* Opaque pointers */
    void *native;
    void *user_data;
} wp_solver_config;

static wp_vec3f vec3f_zero( void )
{
    wp_vec3f v;
    memset( &v, 0, sizeof( v ) );
    return v;
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_solver_config *wp_solver_config_create( void )
{
    wp_solver_config *cfg = (wp_solver_config *)malloc( sizeof( wp_solver_config ) );
    if( !cfg )
    {
        return NULL;
    }

    memset( cfg, 0, sizeof( wp_solver_config ) );

    /* PhysX-aligned defaults */
    cfg->position_iterations = 4;
    cfg->velocity_iterations = 1;

    cfg->fixed_timestep = 1.0f / 60.0f;
    cfg->max_timestep = 1.0f / 30.0f;
    cfg->max_substeps = 8;

    cfg->solver_type = WORKPHONE_SOLVER_PGS;
    cfg->broadphase = WORKPHONE_BROADPHASE_SAP;

    cfg->bounce_threshold = -2.0f;
    cfg->sleep_threshold = 0.05f;
    cfg->contact_offset = 0.02f;
    cfg->rest_offset = 0.0f;

    cfg->gravity.y = -9.81f;

    return cfg;
}

void wp_solver_config_destroy( wp_solver_config *cfg )
{
    if( !cfg )
    {
        return;
    }

    free( cfg );
}

/* =========================================================================
 * Solver iterations
 * ====================================================================== */

wp_u32 wp_solver_config_get_position_iterations( const wp_solver_config *cfg )
{
    if( !cfg )
    {
        return 0;
    }
    return cfg->position_iterations;
}

void wp_solver_config_set_position_iterations( wp_solver_config *cfg, wp_u32 iters )
{
    if( !cfg )
    {
        return;
    }
    cfg->position_iterations = iters;
}

wp_u32 wp_solver_config_get_velocity_iterations( const wp_solver_config *cfg )
{
    if( !cfg )
    {
        return 0;
    }
    return cfg->velocity_iterations;
}

void wp_solver_config_set_velocity_iterations( wp_solver_config *cfg, wp_u32 iters )
{
    if( !cfg )
    {
        return;
    }
    cfg->velocity_iterations = iters;
}

/* =========================================================================
 * Timestep
 * ====================================================================== */

wp_f32 wp_solver_config_get_fixed_timestep( const wp_solver_config *cfg )
{
    if( !cfg )
    {
        return 0.0f;
    }
    return cfg->fixed_timestep;
}

void wp_solver_config_set_fixed_timestep( wp_solver_config *cfg, wp_f32 dt )
{
    if( !cfg )
    {
        return;
    }
    cfg->fixed_timestep = dt;
}

wp_f32 wp_solver_config_get_max_timestep( const wp_solver_config *cfg )
{
    if( !cfg )
    {
        return 0.0f;
    }
    return cfg->max_timestep;
}

void wp_solver_config_set_max_timestep( wp_solver_config *cfg, wp_f32 dt )
{
    if( !cfg )
    {
        return;
    }
    cfg->max_timestep = dt;
}

wp_u32 wp_solver_config_get_max_substeps( const wp_solver_config *cfg )
{
    if( !cfg )
    {
        return 0;
    }
    return cfg->max_substeps;
}

void wp_solver_config_set_max_substeps( wp_solver_config *cfg, wp_u32 substeps )
{
    if( !cfg )
    {
        return;
    }
    cfg->max_substeps = substeps;
}

/* =========================================================================
 * Solver and broadphase type
 * ====================================================================== */

wp_solver_type wp_solver_config_get_solver_type( const wp_solver_config *cfg )
{
    if( !cfg )
    {
        return WORKPHONE_SOLVER_PGS;
    }
    return cfg->solver_type;
}

void wp_solver_config_set_solver_type( wp_solver_config *cfg, wp_solver_type type )
{
    if( !cfg )
    {
        return;
    }
    cfg->solver_type = type;
}

wp_broadphase_type wp_solver_config_get_broadphase( const wp_solver_config *cfg )
{
    if( !cfg )
    {
        return WORKPHONE_BROADPHASE_SAP;
    }
    return cfg->broadphase;
}

void wp_solver_config_set_broadphase( wp_solver_config *cfg, wp_broadphase_type type )
{
    if( !cfg )
    {
        return;
    }
    cfg->broadphase = type;
}

/* =========================================================================
 * CCD
 * ====================================================================== */

wp_s32 wp_solver_config_get_ccd_enabled( const wp_solver_config *cfg )
{
    if( !cfg )
    {
        return 0;
    }
    return cfg->ccd_enabled;
}

void wp_solver_config_set_ccd_enabled( wp_solver_config *cfg, wp_s32 enabled )
{
    if( !cfg )
    {
        return;
    }
    cfg->ccd_enabled = enabled;
}

/* =========================================================================
 * Tolerances
 * ====================================================================== */

wp_f32 wp_solver_config_get_bounce_threshold( const wp_solver_config *cfg )
{
    if( !cfg )
    {
        return 0.0f;
    }
    return cfg->bounce_threshold;
}

void wp_solver_config_set_bounce_threshold( wp_solver_config *cfg, wp_f32 threshold )
{
    if( !cfg )
    {
        return;
    }
    cfg->bounce_threshold = threshold;
}

wp_f32 wp_solver_config_get_sleep_threshold( const wp_solver_config *cfg )
{
    if( !cfg )
    {
        return 0.0f;
    }
    return cfg->sleep_threshold;
}

void wp_solver_config_set_sleep_threshold( wp_solver_config *cfg, wp_f32 threshold )
{
    if( !cfg )
    {
        return;
    }
    cfg->sleep_threshold = threshold;
}

wp_f32 wp_solver_config_get_contact_offset( const wp_solver_config *cfg )
{
    if( !cfg )
    {
        return 0.0f;
    }
    return cfg->contact_offset;
}

void wp_solver_config_set_contact_offset( wp_solver_config *cfg, wp_f32 offset )
{
    if( !cfg )
    {
        return;
    }
    cfg->contact_offset = offset;
}

wp_f32 wp_solver_config_get_rest_offset( const wp_solver_config *cfg )
{
    if( !cfg )
    {
        return 0.0f;
    }
    return cfg->rest_offset;
}

void wp_solver_config_set_rest_offset( wp_solver_config *cfg, wp_f32 offset )
{
    if( !cfg )
    {
        return;
    }
    cfg->rest_offset = offset;
}

/* =========================================================================
 * Gravity
 * ====================================================================== */

wp_vec3f wp_solver_config_get_gravity( const wp_solver_config *cfg )
{
    if( !cfg )
    {
        return vec3f_zero();
    }
    return cfg->gravity;
}

void wp_solver_config_set_gravity( wp_solver_config *cfg, wp_vec3f gravity )
{
    if( !cfg )
    {
        return;
    }
    cfg->gravity = gravity;
}

/* =========================================================================
 * Native / user data
 * ====================================================================== */

void *wp_solver_config_get_native( const wp_solver_config *cfg )
{
    if( !cfg )
    {
        return NULL;
    }
    return cfg->native;
}

void wp_solver_config_set_native( wp_solver_config *cfg, void *native )
{
    if( !cfg )
    {
        return;
    }
    cfg->native = native;
}

void *wp_solver_config_get_user_data( const wp_solver_config *cfg )
{
    if( !cfg )
    {
        return NULL;
    }
    return cfg->user_data;
}

void wp_solver_config_set_user_data( wp_solver_config *cfg, void *user_data )
{
    if( !cfg )
    {
        return;
    }
    cfg->user_data = user_data;
}
