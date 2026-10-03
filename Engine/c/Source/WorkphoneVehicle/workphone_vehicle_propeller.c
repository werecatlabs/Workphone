/**
 * @file workphone_vehicle_propeller.c
 * @brief C99 implementation of a simple propeller model.
 */

#include "workphone_vehicle_propeller.h"
#include "workphone_math.h"
#include "workphone_vector.h"
#include "workphone_quat.h"
#include <string.h>

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

void wp_propeller_init( wp_propeller *p )
{
    memset( p, 0, sizeof( *p ) );
    p->diameter = 0.28f;
    p->pitch = 0.08f;
    p->blades = 2;
    p->peak_power_w = 0.0f;
    p->throttle = 0.0f;
    p->engine_rps = 0.0f;
    p->air_density = 1.225f;
    p->thrust_value = 0.0f;
    p->propwash = 0.0f;
    p->thrust = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    p->torque = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
}

/* -------------------------------------------------------------------------
 * Configuration & state setters
 * ---------------------------------------------------------------------- */

void wp_propeller_set_geometry( wp_propeller *p, wp_f32 diameter, wp_f32 pitch, wp_s32 blades )
{
    p->diameter = diameter;
    p->pitch = pitch;
    p->blades = blades;
}

void wp_propeller_set_engine( wp_propeller *p, wp_f32 peak_power_w )
{
    p->peak_power_w = peak_power_w;
}

void wp_propeller_set_throttle( wp_propeller *p, wp_f32 throttle )
{
    p->throttle = wp_clampf( throttle, 0.0f, 1.0f );
}

void wp_propeller_set_engine_rps( wp_propeller *p, wp_f32 rps )
{
    p->engine_rps = rps;
}

void wp_propeller_set_state( wp_propeller *p, wp_vec3f position, wp_quatf orientation, wp_vec3f velocity,
                             wp_vec3f angular_velocity )
{
    p->position = position;
    p->orientation = orientation;
    p->velocity = velocity;
    p->angular_velocity = angular_velocity;
}

/* -------------------------------------------------------------------------
 * Per-tick update
 * ---------------------------------------------------------------------- */

void wp_propeller_fixed_update( wp_propeller *p, wp_f32 dt )
{
    (void)dt;

    /* Local thrust axis (-Z) rotated into world space */
    wp_vec3f local_thrust = wp_vec3f_make( 0.0f, 0.0f, -1.0f );
    wp_vec3f thrust_axis = wp_quatf_rotate_vec3( p->orientation, local_thrust );

    /* Simple power-based thrust estimate (inspired by EPropellerSimple)
     * thrust = throttle * P * (a + b * J^2) / (n * D)
     * where J = advance ratio = V / (n * D), n = revs/sec
     */
    wp_f32 thrust = 0.0f;
    if( p->engine_rps > 0.0f && p->peak_power_w > 0.0f )
    {
        wp_f32 speed = wp_vec3f_length( p->velocity );
        wp_f32 D = p->diameter > 1e-6f ? p->diameter : 1e-6f;
        wp_f32 advanceRatio = speed / ( p->engine_rps * D );

        const wp_f32 a = 1.0f;
        const wp_f32 b = 1.0f;

        wp_f32 denom = p->engine_rps * D;
        if( denom <= 1e-9f )
            denom = 1e-9f;

        thrust = p->throttle * p->peak_power_w * ( a + b * advanceRatio * advanceRatio ) / denom;
    }

    p->thrust_value = thrust;

    /* World-space thrust vector */
    p->thrust = wp_vec3f_scale( thrust_axis, thrust );

    /* Approximate propwash (induced velocity) using momentum theory */
    wp_f32 A = WORKPHONE_PI_F * 0.25f * p->diameter * p->diameter;
    if( A > 0.0f && p->air_density > 0.0f )
    {
        p->propwash = wp_sqrtf( wp_absf( thrust ) / ( 2.0f * p->air_density * A ) );
    }
    else
    {
        p->propwash = 0.0f;
    }

    /* Reaction torque as a small fraction of thrust */
    const wp_f32 torque_factor = 0.015f;
    p->torque = wp_vec3f_scale( thrust_axis, -thrust * torque_factor );
}

/* -------------------------------------------------------------------------
 * Outputs
 * ---------------------------------------------------------------------- */

wp_vec3f wp_propeller_get_thrust( const wp_propeller *p )
{
    return p->thrust;
}

wp_f32 wp_propeller_get_thrust_value( const wp_propeller *p )
{
    return p->thrust_value;
}

wp_f32 wp_propeller_get_propwash( const wp_propeller *p )
{
    return p->propwash;
}

wp_vec3f wp_propeller_get_torque( const wp_propeller *p )
{
    return p->torque;
}
