#include "workphone_graphics_particle_simulation.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

struct wp_particle_simulation
{
    wp_particle_simulation_settings settings;
    wp_particle_sample *samples;
    wp_u32 capacity, count, dropped, seed, random;
    double remainder, emission, elapsed;
    wp_particle_state state;
};

static int finite_value( wp_f32 value )
{
    return value >= -FLT_MAX && value <= FLT_MAX;
}

static int valid_vector( wp_vec3f value )
{
    return finite_value( value.x ) && finite_value( value.y ) && finite_value( value.z ) &&
        fabs( value.x ) <= 1e6 && fabs( value.y ) <= 1e6 && fabs( value.z ) <= 1e6;
}

static wp_f32 random_range( wp_particle_simulation *s, wp_f32 low, wp_f32 high )
{
    wp_u32 x = s->random;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s->random = x;
    return low + (high - low) * (wp_f32)( (x >> 8) * (1.0 / 16777216.0) );
}

void wp_particle_simulation_default_settings( wp_particle_simulation_settings *settings )
{
    wp_u32 i;
    if( !settings ) return;
    memset( settings, 0, sizeof( *settings ) );
    settings->rate = 10.0f;
    settings->lifetime_min = settings->lifetime_max = 1.0f;
    settings->size_min = settings->size_max = 0.1f;
    settings->velocity.y = 1.0f;
    settings->looping = 1;
    for( i = 0; i < 4; ++i ) settings->color_start[i] = settings->color_end[i] = 1.0f;
}

wp_particle_simulation *wp_particle_simulation_create( wp_u32 capacity, wp_u32 seed )
{
    wp_particle_simulation *s;
    if( !capacity || capacity > WP_PARTICLE_SIMULATION_MAX_CAPACITY ) return NULL;
    s = (wp_particle_simulation *)calloc( 1, sizeof( *s ) );
    if( !s ) return NULL;
    s->samples = (wp_particle_sample *)calloc( capacity, sizeof( *s->samples ) );
    if( !s->samples ) { free( s ); return NULL; }
    s->capacity = capacity;
    s->seed = s->random = seed ? seed : 1u;
    wp_particle_simulation_default_settings( &s->settings );
    return s;
}

void wp_particle_simulation_destroy( wp_particle_simulation *s )
{
    if( !s ) return;
    free( s->samples );
    free( s );
}

wp_s32 wp_particle_simulation_configure( wp_particle_simulation *s,
                                       const wp_particle_simulation_settings *settings )
{
    wp_u32 i;
    if( !s || !settings || !finite_value( settings->rate ) || settings->rate < 0.0f ||
        settings->rate > 1e6f || !finite_value( settings->lifetime_min ) ||
        !finite_value( settings->lifetime_max ) || settings->lifetime_min <= 0.0f ||
        settings->lifetime_max < settings->lifetime_min || settings->lifetime_max > 86400.0f ||
        !finite_value( settings->size_min ) || !finite_value( settings->size_max ) ||
        settings->size_min < 0.0f || settings->size_max < settings->size_min ||
        settings->size_max > 1e6f || !finite_value( settings->duration ) ||
        settings->duration < 0.0f || settings->duration > 86400.0f ||
        !valid_vector( settings->velocity ) || !valid_vector( settings->gravity ) ) return 0;
    for( i = 0; i < 4; ++i )
        if( !finite_value( settings->color_start[i] ) || !finite_value( settings->color_end[i] ) ||
            settings->color_start[i] < 0.0f || settings->color_start[i] > 1.0f ||
            settings->color_end[i] < 0.0f || settings->color_end[i] > 1.0f ) return 0;
    s->settings = *settings;
    return 1;
}

wp_s32 wp_particle_simulation_set_state( wp_particle_simulation *s, wp_particle_state state )
{
    if( !s || (state != WORKPHONE_PARTICLE_STATE_STOPPED && state != WORKPHONE_PARTICLE_STATE_STARTED &&
        state != WORKPHONE_PARTICLE_STATE_PAUSED && state != WORKPHONE_PARTICLE_STATE_STOPPED_FADE) ) return 0;
    if( state == WORKPHONE_PARTICLE_STATE_STOPPED ||
        (state == WORKPHONE_PARTICLE_STATE_STARTED && s->state == WORKPHONE_PARTICLE_STATE_STOPPED) )
    {
        s->count = s->dropped = 0;
        s->elapsed = s->emission = s->remainder = 0.0;
        s->random = s->seed;
    }
    s->state = state;
    return 1;
}

wp_particle_state wp_particle_simulation_get_state( const wp_particle_simulation *s )
{
    return s ? s->state : WORKPHONE_PARTICLE_STATE_STOPPED;
}

static void step( wp_particle_simulation *s )
{
    wp_u32 i, c, spawn, available, dropped;
    double emission_dt;
    const wp_f32 dt = (wp_f32)WP_PARTICLE_SIMULATION_STEP;
    i = 0;
    while( i < s->count )
    {
        wp_particle_sample *p = &s->samples[i];
        p->age += dt;
        if( (double)p->age + 1e-6 >= p->lifetime )
        {
            s->samples[i] = s->samples[--s->count];
            continue;
        }
        p->position.x += p->velocity.x * dt + 0.5f * s->settings.gravity.x * dt * dt;
        p->position.y += p->velocity.y * dt + 0.5f * s->settings.gravity.y * dt * dt;
        p->position.z += p->velocity.z * dt + 0.5f * s->settings.gravity.z * dt * dt;
        p->velocity.x += s->settings.gravity.x * dt;
        p->velocity.y += s->settings.gravity.y * dt;
        p->velocity.z += s->settings.gravity.z * dt;
        for( c = 0; c < 4; ++c )
            p->color[c] = s->settings.color_start[c] +
                (s->settings.color_end[c] - s->settings.color_start[c]) * (p->age / p->lifetime);
        ++i;
    }
    emission_dt = WP_PARTICLE_SIMULATION_STEP;
    if( s->state != WORKPHONE_PARTICLE_STATE_STARTED ) emission_dt = 0.0;
    if( !s->settings.looping && s->settings.duration > 0.0f )
    {
        double remaining = s->settings.duration - s->elapsed;
        if( remaining < emission_dt ) emission_dt = remaining > 0.0 ? remaining : 0.0;
    }
    s->elapsed += WP_PARTICLE_SIMULATION_STEP;
    s->emission += s->settings.rate * emission_dt;
    spawn = (wp_u32)floor( s->emission + 1e-8 );
    s->emission -= spawn;
    if( s->emission < 0.0 ) s->emission = 0.0;
    available = s->capacity - s->count;
    dropped = spawn > available ? spawn - available : 0;
    if( dropped > (wp_u32)~0u - s->dropped ) s->dropped = (wp_u32)~0u;
    else s->dropped += dropped;
    if( spawn > available ) spawn = available;
    for( i = 0; i < spawn; ++i )
    {
        wp_particle_sample *p = &s->samples[s->count++];
        memset( p, 0, sizeof( *p ) );
        p->velocity = s->settings.velocity;
        p->lifetime = random_range( s, s->settings.lifetime_min, s->settings.lifetime_max );
        p->size = random_range( s, s->settings.size_min, s->settings.size_max );
        for( c = 0; c < 4; ++c ) p->color[c] = s->settings.color_start[c];
    }
    if( s->state == WORKPHONE_PARTICLE_STATE_STOPPED_FADE && !s->count )
        s->state = WORKPHONE_PARTICLE_STATE_STOPPED;
}

wp_s32 wp_particle_simulation_advance( wp_particle_simulation *s, wp_f32 seconds )
{
    wp_u32 steps, i;
    if( !s || !finite_value( seconds ) || seconds < 0.0f || seconds > 1.0f ) return 0;
    if( s->state == WORKPHONE_PARTICLE_STATE_STOPPED || s->state == WORKPHONE_PARTICLE_STATE_PAUSED ) return 1;
    s->remainder += seconds;
    steps = (wp_u32)floor( (s->remainder + 1e-8) / WP_PARTICLE_SIMULATION_STEP );
    s->remainder -= steps * WP_PARTICLE_SIMULATION_STEP;
    if( s->remainder < 0.0 ) s->remainder = 0.0;
    for( i = 0; i < steps; ++i ) step( s );
    return 1;
}

wp_u32 wp_particle_simulation_get_count( const wp_particle_simulation *s ) { return s ? s->count : 0; }
wp_u32 wp_particle_simulation_get_dropped( const wp_particle_simulation *s ) { return s ? s->dropped : 0; }
const wp_particle_sample *wp_particle_simulation_get_samples( const wp_particle_simulation *s )
{
    return s ? s->samples : NULL;
}
