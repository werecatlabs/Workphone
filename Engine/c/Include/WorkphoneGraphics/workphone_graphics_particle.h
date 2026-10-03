/**
 * @file wp_graphics_particle.h
 * @brief C API for particle systems and emitters.
 *
 * A particle system manages one or more emitters that spawn short-lived
 * particles to simulate effects such as fire, smoke, sparks, and dust.
 * Each emitter controls emission rate, particle size, direction, velocity,
 * and time-to-live, all with optional min/max randomisation ranges.
 */

#ifndef WORKPHONE_GRAPHICS_PARTICLE_H
#define WORKPHONE_GRAPHICS_PARTICLE_H

#include <stdint.h>
#include "workphone_vector.h"
#include "workphone_graphics_object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wp_particle_system wp_particle_system;
typedef struct wp_particle_emitter wp_particle_emitter;

typedef enum wp_particle_state
{
    WORKPHONE_PARTICLE_STATE_STOPPED = 0,
    WORKPHONE_PARTICLE_STATE_STARTED = 1,
    WORKPHONE_PARTICLE_STATE_PAUSED = 2,
    WORKPHONE_PARTICLE_STATE_PAUSED_TIMED = 3,
    WORKPHONE_PARTICLE_STATE_STOPPED_FADE = 4
} wp_particle_state;

#ifndef WP_PARTICLE_MAX_TEMPLATE_NAME
#    define WP_PARTICLE_MAX_TEMPLATE_NAME 256
#endif

/* =========================================================================
 * Particle system
 * ====================================================================== */

wp_particle_system *wp_particle_system_create( void );
void wp_particle_system_destroy( wp_particle_system *ps );

wp_particle_state wp_particle_system_get_state( const wp_particle_system *ps );
void wp_particle_system_set_state( wp_particle_system *ps, wp_particle_state state );

const wp_c8 *wp_particle_system_get_template_name( const wp_particle_system *ps );
void wp_particle_system_set_template_name( wp_particle_system *ps, const wp_c8 *name );

wp_vec3f wp_particle_system_get_scale( const wp_particle_system *ps );
void wp_particle_system_set_scale( wp_particle_system *ps, wp_vec3f scale );

wp_f32 wp_particle_system_get_fast_forward_time( const wp_particle_system *ps );
wp_f32 wp_particle_system_get_fast_forward_interval( const wp_particle_system *ps );
void wp_particle_system_set_fast_forward( wp_particle_system *ps, wp_f32 time, wp_f32 interval );

wp_s32 wp_particle_system_add_emitter( wp_particle_system *ps, wp_particle_emitter *emitter );
void wp_particle_system_remove_emitter( wp_particle_system *ps, wp_s32 index );
wp_particle_emitter *wp_particle_system_get_emitter( const wp_particle_system *ps, wp_s32 index );
wp_s32 wp_particle_system_get_emitter_count( const wp_particle_system *ps );

wp_s32 wp_particle_system_is_dirty( const wp_particle_system *ps );
void wp_particle_system_mark_dirty( wp_particle_system *ps );
void wp_particle_system_clear_dirty( wp_particle_system *ps );

void *wp_particle_system_get_native( const wp_particle_system *ps );
void wp_particle_system_set_native( wp_particle_system *ps, void *native );

/* =========================================================================
 * Particle emitter
 * ====================================================================== */

wp_particle_emitter *wp_particle_emitter_create( void );
void wp_particle_emitter_destroy( wp_particle_emitter *em );

wp_f32 wp_particle_emitter_get_emission_rate( const wp_particle_emitter *em );
void wp_particle_emitter_set_emission_rate( wp_particle_emitter *em, wp_f32 rate );
wp_f32 wp_particle_emitter_get_emission_rate_min( const wp_particle_emitter *em );
void wp_particle_emitter_set_emission_rate_min( wp_particle_emitter *em, wp_f32 rate );
wp_f32 wp_particle_emitter_get_emission_rate_max( const wp_particle_emitter *em );
void wp_particle_emitter_set_emission_rate_max( wp_particle_emitter *em, wp_f32 rate );

wp_vec3f wp_particle_emitter_get_particle_size( const wp_particle_emitter *em );
void wp_particle_emitter_set_particle_size( wp_particle_emitter *em, wp_vec3f size );
wp_vec3f wp_particle_emitter_get_particle_size_min( const wp_particle_emitter *em );
void wp_particle_emitter_set_particle_size_min( wp_particle_emitter *em, wp_vec3f size );
wp_vec3f wp_particle_emitter_get_particle_size_max( const wp_particle_emitter *em );
void wp_particle_emitter_set_particle_size_max( wp_particle_emitter *em, wp_vec3f size );

wp_vec3f wp_particle_emitter_get_direction( const wp_particle_emitter *em );
void wp_particle_emitter_set_direction( wp_particle_emitter *em, wp_vec3f direction );

wp_f32 wp_particle_emitter_get_velocity( const wp_particle_emitter *em );
void wp_particle_emitter_set_velocity( wp_particle_emitter *em, wp_f32 velocity );
wp_f32 wp_particle_emitter_get_velocity_min( const wp_particle_emitter *em );
void wp_particle_emitter_set_velocity_min( wp_particle_emitter *em, wp_f32 velocity );
wp_f32 wp_particle_emitter_get_velocity_max( const wp_particle_emitter *em );
void wp_particle_emitter_set_velocity_max( wp_particle_emitter *em, wp_f32 velocity );

wp_f32 wp_particle_emitter_get_ttl( const wp_particle_emitter *em );
void wp_particle_emitter_set_ttl( wp_particle_emitter *em, wp_f32 ttl );
wp_f32 wp_particle_emitter_get_ttl_min( const wp_particle_emitter *em );
void wp_particle_emitter_set_ttl_min( wp_particle_emitter *em, wp_f32 ttl );
wp_f32 wp_particle_emitter_get_ttl_max( const wp_particle_emitter *em );
void wp_particle_emitter_set_ttl_max( wp_particle_emitter *em, wp_f32 ttl );

void *wp_particle_emitter_get_native( const wp_particle_emitter *em );
void wp_particle_emitter_set_native( wp_particle_emitter *em, void *native );

#ifdef __cplusplus
}
#endif

#endif
