#ifndef WORKPHONE_GRAPHICS_PARTICLE_SIMULATION_H
#define WORKPHONE_GRAPHICS_PARTICLE_SIMULATION_H

#include <stddef.h>
#include "workphone_graphics_particle.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WP_PARTICLE_SIMULATION_MAX_CAPACITY 1000000u
#define WP_PARTICLE_SIMULATION_STEP ( 1.0 / 120.0 )

typedef struct wp_particle_simulation wp_particle_simulation;
typedef struct wp_particle_simulation_settings
{
    wp_f32 rate;
    wp_f32 lifetime_min, lifetime_max;
    wp_f32 size_min, size_max;
    wp_vec3f velocity;
    wp_vec3f gravity;
    wp_f32 color_start[4], color_end[4];
    wp_f32 duration; /* Zero means indefinite. */
    wp_s32 looping;
} wp_particle_simulation_settings;

typedef struct wp_particle_sample
{
    wp_vec3f position, velocity;
    wp_f32 age, lifetime, size;
    wp_f32 color[4];
} wp_particle_sample;

void wp_particle_simulation_default_settings( wp_particle_simulation_settings *settings );
wp_particle_simulation *wp_particle_simulation_create( wp_u32 capacity, wp_u32 seed );
void wp_particle_simulation_destroy( wp_particle_simulation *simulation );
/* Transactional validation. Settings affect new particles; gravity/color also affect live ones. */
wp_s32 wp_particle_simulation_configure( wp_particle_simulation *simulation,
                                       const wp_particle_simulation_settings *settings );
/* Started after Stopped resets the seed and clocks. Stopped clears immediately;
 * StoppedFade drains. Paused freezes time. Timed pause is unsupported and rejected. */
wp_s32 wp_particle_simulation_set_state( wp_particle_simulation *simulation, wp_particle_state state );
wp_particle_state wp_particle_simulation_get_state( const wp_particle_simulation *simulation );
/* Fixed-step, no allocations. Accepts finite dt in [0,1], otherwise no mutation.
 * Emission beyond capacity is dropped, counted, and never creates later bursts. */
wp_s32 wp_particle_simulation_advance( wp_particle_simulation *simulation, wp_f32 seconds );
wp_u32 wp_particle_simulation_get_count( const wp_particle_simulation *simulation );
wp_u32 wp_particle_simulation_get_dropped( const wp_particle_simulation *simulation );
/* Borrowed storage, valid until next mutation. Caller must synchronize access. */
const wp_particle_sample *wp_particle_simulation_get_samples( const wp_particle_simulation *simulation );

#ifdef __cplusplus
}
#endif
#endif
