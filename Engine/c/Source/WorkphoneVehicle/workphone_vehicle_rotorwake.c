/**
 * @file workphone_vehicle_rotorwake.c
 * @brief C99 implementation of the rotor wake vortex particle simulation.
 */

#include "workphone_vehicle_rotorwake.h"
#include "workphone_math.h"
#include "workphone_vector.h"

#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

/**
 * @brief Return a pseudo-random float uniformly distributed in [-1, 1].
 */
static wp_f32 wp_wake_randf( void )
{
    return ( (wp_f32)rand() / (wp_f32)RAND_MAX ) * 2.0f - 1.0f;
}

/* =========================================================================
 * Internal sub-steps (called by wp_rotor_wake_update)
 * ====================================================================== */

/* =========================
 * SPAWN ROTOR WAKE
 * ========================= */
static void wp_rotor_wake_spawn( wp_rotor_wake *w )
{
    wp_s32 spawn_count = (wp_s32)( w->rotor_rpm * 0.05f );
    wp_s32 i;

    for( i = 0; i < spawn_count; ++i )
    {
        wp_rotor_wake_particle *p = &w->particles[w->head];

        /* Only increment count when we are filling a previously empty slot. */
        if( !p->active && w->count < w->max_particles )
            ++w->count;

        /* Position: random powp_s32 on the rotor disc plane */
        {
            wp_vec3f disc =
                wp_vec3f_add( wp_vec3f_scale( w->body_right, wp_wake_randf() * w->rotor_radius ),
                              wp_vec3f_scale( w->body_forward, wp_wake_randf() * w->rotor_radius ) );

            p->position = wp_vec3f_add( w->body_position, disc );
        }

        /* Velocity: straight down in rotor-local space */
        p->velocity = wp_vec3f_scale( w->body_up, -w->downwash_strength );
        p->life = w->particle_lifetime;
        p->active = 1;

        /* Advance the ring buffer write head */
        w->head = ( w->head + 1 ) % w->max_particles;
    }
}

/* =========================
 * UPDATE VORTEX MOTION
 * ========================= */
static void wp_rotor_wake_advect( wp_rotor_wake *w, wp_f32 dt )
{
    static const wp_vec3f world_up = { 0.0f, 1.0f, 0.0f };
    wp_s32 i;

    for( i = 0; i < w->max_particles; ++i )
    {
        wp_rotor_wake_particle *p = &w->particles[i];

        if( !p->active )
            continue;

        /* Spiral vortex motion: swirl = cross(world_up, normalize(velocity)) */
        if( !wp_vec3f_is_zero_length( p->velocity ) )
        {
            wp_vec3f vel_norm = wp_vec3f_normalize( p->velocity );
            wp_vec3f swirl = wp_vec3f_scale( wp_vec3f_cross( world_up, vel_norm ), w->vortex_strength );

            p->velocity = wp_vec3f_add( p->velocity, wp_vec3f_scale( swirl, dt ) );
        }

        p->position = wp_vec3f_add( p->position, wp_vec3f_scale( p->velocity, dt ) );
        p->life -= dt;

        if( p->life <= 0.0f )
        {
            p->active = 0;
            --w->count;
        }
    }
}

/* =========================
 * APPLY INDUCED FLOW TO HELICOPTER
 * ========================= */
static void wp_rotor_wake_accumulate_force( wp_rotor_wake *w )
{
    wp_vec3f total = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    wp_f32 max_dist = w->rotor_radius * 2.0f;
    wp_s32 i;

    for( i = 0; i < w->max_particles; ++i )
    {
        const wp_rotor_wake_particle *p = &w->particles[i];

        if( !p->active )
            continue;

        {
            wp_f32 dist = wp_vec3f_distance( w->body_position, p->position );

            if( dist < max_dist )
            {
                wp_f32 influence = 1.0f - dist / max_dist;
                total = wp_vec3f_add( total, wp_vec3f_scale( p->velocity, influence ) );
            }
        }
    }

    /* Apply turbulence force */
    w->induced_force = wp_vec3f_scale( total, 0.5f );
}

/* =========================================================================
 * Public API
 * ====================================================================== */

void wp_rotor_wake_init( wp_rotor_wake *w )
{
    memset( w, 0, sizeof( *w ) );

    /* Tunable defaults */
    w->max_particles = WP_ROTOR_WAKE_MAX_PARTICLES;
    w->particle_lifetime = 5.0f;
    w->downwash_strength = 40.0f;
    w->vortex_strength = 15.0f;
    w->rotor_radius = 6.5f;

    /* World-axes default to a standard identity orientation */
    w->body_right = wp_vec3f_make( 1.0f, 0.0f, 0.0f );
    w->body_forward = wp_vec3f_make( 0.0f, 0.0f, 1.0f );
    w->body_up = wp_vec3f_make( 0.0f, 1.0f, 0.0f );
}

void wp_rotor_wake_update( wp_rotor_wake *w, wp_f32 dt )
{
    wp_rotor_wake_spawn( w );
    wp_rotor_wake_advect( w, dt );
    wp_rotor_wake_accumulate_force( w );
}

void wp_rotor_wake_set_rotor_rpm( wp_rotor_wake *w, wp_f32 rpm )
{
    w->rotor_rpm = rpm;
}

void wp_rotor_wake_set_transform( wp_rotor_wake *w, wp_vec3f position, wp_vec3f right, wp_vec3f forward,
                                  wp_vec3f up )
{
    w->body_position = position;
    w->body_right = right;
    w->body_forward = forward;
    w->body_up = up;
}

wp_vec3f wp_rotor_wake_get_induced_force( const wp_rotor_wake *w )
{
    return w->induced_force;
}

wp_s32 wp_rotor_wake_get_particle_count( const wp_rotor_wake *w )
{
    return w->count;
}

const wp_rotor_wake_particle *wp_rotor_wake_get_particles( const wp_rotor_wake *w, wp_s32 *out_capacity )
{
    if( out_capacity )
        *out_capacity = w->max_particles;

    return w->particles;
}
