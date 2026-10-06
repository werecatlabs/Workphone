#include "workphone_graphics_particle_simulation.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CHECK(value) do { if( !(value) ) { fprintf(stderr, "particles: line %d: %s\n", __LINE__, #value); return 1; } } while(0)

int main( void )
{
    wp_particle_simulation *a, *b, *small;
    wp_particle_simulation_settings settings, invalid;
    wp_particle_sample saved;
    const wp_particle_sample *samples;
    unsigned int i;
    CHECK( !wp_particle_simulation_create(0, 1) );
    CHECK( !wp_particle_simulation_create(WP_PARTICLE_SIMULATION_MAX_CAPACITY + 1, 1) );
    a = wp_particle_simulation_create(256, 42);
    b = wp_particle_simulation_create(256, 42);
    small = wp_particle_simulation_create(2, 42);
    CHECK( a && b && small );
    wp_particle_simulation_default_settings( &settings );
    settings.rate = 120.0f;
    settings.lifetime_min = 3.0f; settings.lifetime_max = 4.0f;
    settings.size_min = 0.1f; settings.size_max = 0.5f;
    CHECK( wp_particle_simulation_configure(a, &settings) );
    CHECK( wp_particle_simulation_configure(b, &settings) );
    CHECK( wp_particle_simulation_configure(small, &settings) );
    CHECK( wp_particle_simulation_set_state(a, WORKPHONE_PARTICLE_STATE_STARTED) );
    CHECK( wp_particle_simulation_set_state(b, WORKPHONE_PARTICLE_STATE_STARTED) );
    CHECK( wp_particle_simulation_advance(a, 1.0f) );
    for( i = 0; i < 60; ++i ) CHECK( wp_particle_simulation_advance(b, 1.0f / 60.0f) );
    CHECK( wp_particle_simulation_get_count(a) == 120 );
    CHECK( wp_particle_simulation_get_count(b) == 120 );
    CHECK( memcmp(wp_particle_simulation_get_samples(a), wp_particle_simulation_get_samples(b),
        120 * sizeof(wp_particle_sample)) == 0 );
    samples = wp_particle_simulation_get_samples(a);
    CHECK( fabs(samples[0].position.y - 119.0 / 120.0) < 1e-5 );
    CHECK( samples[0].lifetime >= 3.0f && samples[0].lifetime <= 4.0f );
    saved = samples[0];
    CHECK( wp_particle_simulation_set_state(a, WORKPHONE_PARTICLE_STATE_PAUSED) );
    CHECK( wp_particle_simulation_advance(a, 1.0f) );
    CHECK( memcmp(&saved, wp_particle_simulation_get_samples(a), sizeof(saved)) == 0 );
    CHECK( !wp_particle_simulation_advance(a, -1.0f) );
    CHECK( !wp_particle_simulation_advance(a, 2.0f) );
    CHECK( !wp_particle_simulation_set_state(a, WORKPHONE_PARTICLE_STATE_PAUSED_TIMED) );
    invalid = settings; invalid.rate = -1.0f;
    CHECK( !wp_particle_simulation_configure(a, &invalid) );
    CHECK( wp_particle_simulation_set_state(a, WORKPHONE_PARTICLE_STATE_STOPPED_FADE) );
    for( i = 0; i < 5; ++i ) CHECK( wp_particle_simulation_advance(a, 1.0f) );
    CHECK( wp_particle_simulation_get_count(a) == 0 );
    CHECK( wp_particle_simulation_get_state(a) == WORKPHONE_PARTICLE_STATE_STOPPED );
    CHECK( wp_particle_simulation_set_state(small, WORKPHONE_PARTICLE_STATE_STARTED) );
    CHECK( wp_particle_simulation_advance(small, 1.0f) );
    CHECK( wp_particle_simulation_get_count(small) == 2 );
    CHECK( wp_particle_simulation_get_dropped(small) == 118 );
    CHECK( wp_particle_simulation_set_state(b, WORKPHONE_PARTICLE_STATE_STOPPED) );
    CHECK( wp_particle_simulation_get_count(b) == 0 );
    CHECK( wp_particle_simulation_set_state(a, WORKPHONE_PARTICLE_STATE_STARTED) );
    CHECK( wp_particle_simulation_set_state(b, WORKPHONE_PARTICLE_STATE_STARTED) );
    CHECK( wp_particle_simulation_advance(a, 0.5f) && wp_particle_simulation_advance(b, 0.5f) );
    CHECK( memcmp(wp_particle_simulation_get_samples(a), wp_particle_simulation_get_samples(b),
        60 * sizeof(wp_particle_sample)) == 0 ); /* Restart restores seed. */
    settings.looping = 0; settings.duration = 0.25f;
    CHECK( wp_particle_simulation_set_state(a, WORKPHONE_PARTICLE_STATE_STOPPED) );
    CHECK( wp_particle_simulation_configure(a, &settings) );
    CHECK( wp_particle_simulation_set_state(a, WORKPHONE_PARTICLE_STATE_STARTED) );
    CHECK( wp_particle_simulation_advance(a, 1.0f) );
    CHECK( wp_particle_simulation_get_count(a) == 30 );
    wp_particle_simulation_destroy(a); wp_particle_simulation_destroy(b); wp_particle_simulation_destroy(small);
    puts("PASS: deterministic stepping, trajectories, pause, drain, restart, capacity and duration");
    return 0;
}
