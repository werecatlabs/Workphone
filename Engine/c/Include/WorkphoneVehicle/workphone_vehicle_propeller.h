#ifndef WORKPHONE_VEHICLE_PROPELLER_H
#define WORKPHONE_VEHICLE_PROPELLER_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_vector.h"
#include "workphone_quat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Simple propeller model for use by the vehicle systems. This is a
 * physics-engine-agnostic, compact implementation inspired by the
 * CAircraftPropeller/CAircraftPropellerUnit C++ classes. It provides
 * a pragmatic thrust/propwash/torque approximation suitable for game
 * simulations.
 */

typedef struct wp_propeller
{
    /* geometry */
    wp_f32 diameter; /* metres */
    wp_f32 pitch;    /* metres (geometric) */
    wp_s32 blades;

    /* performance */
    wp_f32 peak_power_w; /* engine peak power (watts) used by simple model */
    wp_f32 throttle;     /* [0,1] commanded throttle */
    wp_f32 engine_rps;   /* engine shaft speed in revs/sec (n) */

    /* environment */
    wp_f32 air_density; /* kg/m^3 */

    /* runtime state (inputs) */
    wp_vec3f position;         /* world-space prop hub position */
    wp_quatf orientation;      /* world-space orientation */
    wp_vec3f velocity;         /* body linear velocity (world-space) */
    wp_vec3f angular_velocity; /* body angular velocity (world-space) */

    /* outputs */
    wp_vec3f thrust;     /* world-space thrust vector (N) */
    wp_f32 thrust_value; /* scalar thrust magnitude (N) */
    wp_f32 propwash;     /* approximate induced velocity (m/s) */
    wp_vec3f torque;     /* reaction torque vector (N*m) */
} wp_propeller;

/* Lifecycle */
void wp_propeller_init( wp_propeller *p );

/* Geometry / engine */
void wp_propeller_set_geometry( wp_propeller *p, wp_f32 diameter, wp_f32 pitch, wp_s32 blades );
void wp_propeller_set_engine( wp_propeller *p, wp_f32 peak_power_w );
void wp_propeller_set_throttle( wp_propeller *p, wp_f32 throttle );
void wp_propeller_set_engine_rps( wp_propeller *p, wp_f32 rps );

/* State inputs */
void wp_propeller_set_state( wp_propeller *p, wp_vec3f position, wp_quatf orientation, wp_vec3f velocity,
                             wp_vec3f angular_velocity );

/* Per-tick update */
void wp_propeller_fixed_update( wp_propeller *p, wp_f32 dt );

/* Outputs */
wp_vec3f wp_propeller_get_thrust( const wp_propeller *p );
wp_f32 wp_propeller_get_thrust_value( const wp_propeller *p );
wp_f32 wp_propeller_get_propwash( const wp_propeller *p );
wp_vec3f wp_propeller_get_torque( const wp_propeller *p );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_VEHICLE_PROPELLER_H */
