/**
 * @file wp_graphics_particle.c
 * @brief Implementation of the C graphics particle API.
 */

#include "workphone_graphics_particle.h"
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal structures
 * ====================================================================== */

#define WORKPHONE_EMITTER_INITIAL_CAP 4

typedef struct wp_particle_emitter
{
    wp_f32 emission_rate;
    wp_f32 emission_rate_min;
    wp_f32 emission_rate_max;
    wp_vec3f particle_size;
    wp_vec3f particle_size_min;
    wp_vec3f particle_size_max;
    wp_vec3f direction;
    wp_f32 velocity;
    wp_f32 velocity_min;
    wp_f32 velocity_max;
    wp_f32 ttl;
    wp_f32 ttl_min;
    wp_f32 ttl_max;
    void *native;
} wp_particle_emitter;

typedef struct wp_particle_system
{
    wp_particle_state state;
    wp_c8 template_name[WP_PARTICLE_MAX_TEMPLATE_NAME];
    wp_vec3f scale;
    wp_f32 fast_forward_time;
    wp_f32 fast_forward_interval;
    wp_particle_emitter **emitters;
    wp_s32 emitter_count;
    wp_s32 emitter_cap;
    void *native;
    wp_s32 is_dirty;
} wp_particle_system;

/* =========================================================================
 * Particle system - Lifecycle
 * ====================================================================== */

wp_particle_system *wp_particle_system_create( void )
{
    wp_particle_system *ps = (wp_particle_system *)malloc( sizeof( wp_particle_system ) );
    if( !ps )
    {
        return NULL;
    }

    memset( ps, 0, sizeof( wp_particle_system ) );

    ps->state = WORKPHONE_PARTICLE_STATE_STOPPED;
    ps->scale.x = 1.0f;
    ps->scale.y = 1.0f;
    ps->scale.z = 1.0f;
    ps->is_dirty = 1;

    return ps;
}

void wp_particle_system_destroy( wp_particle_system *ps )
{
    if( !ps )
    {
        return;
    }

    free( ps->emitters );
    free( ps );
}

/* =========================================================================
 * Particle system - State
 * ====================================================================== */

wp_particle_state wp_particle_system_get_state( const wp_particle_system *ps )
{
    if( !ps )
    {
        return WORKPHONE_PARTICLE_STATE_STOPPED;
    }
    return ps->state;
}

void wp_particle_system_set_state( wp_particle_system *ps, wp_particle_state state )
{
    if( !ps )
    {
        return;
    }
    ps->state = state;
}

/* =========================================================================
 * Particle system - Template name
 * ====================================================================== */

const wp_c8 *wp_particle_system_get_template_name( const wp_particle_system *ps )
{
    if( !ps )
    {
        return "";
    }
    return ps->template_name;
}

void wp_particle_system_set_template_name( wp_particle_system *ps, const wp_c8 *name )
{
    if( !ps )
    {
        return;
    }
    if( name )
    {
        strncpy( ps->template_name, name, WP_PARTICLE_MAX_TEMPLATE_NAME - 1 );
        ps->template_name[WP_PARTICLE_MAX_TEMPLATE_NAME - 1] = '\0';
    }
    else
    {
        ps->template_name[0] = '\0';
    }
    ps->is_dirty = 1;
}

/* =========================================================================
 * Particle system - Scale and fast-forward
 * ====================================================================== */

wp_vec3f wp_particle_system_get_scale( const wp_particle_system *ps )
{
    wp_vec3f zero;
    memset( &zero, 0, sizeof( zero ) );
    if( !ps )
    {
        return zero;
    }
    return ps->scale;
}

void wp_particle_system_set_scale( wp_particle_system *ps, wp_vec3f scale )
{
    if( !ps )
    {
        return;
    }
    ps->scale = scale;
    ps->is_dirty = 1;
}

wp_f32 wp_particle_system_get_fast_forward_time( const wp_particle_system *ps )
{
    if( !ps )
    {
        return 0.0f;
    }
    return ps->fast_forward_time;
}

wp_f32 wp_particle_system_get_fast_forward_interval( const wp_particle_system *ps )
{
    if( !ps )
    {
        return 0.0f;
    }
    return ps->fast_forward_interval;
}

void wp_particle_system_set_fast_forward( wp_particle_system *ps, wp_f32 time, wp_f32 interval )
{
    if( !ps )
    {
        return;
    }
    ps->fast_forward_time = time;
    ps->fast_forward_interval = interval;
}

/* =========================================================================
 * Particle system - Emitter management
 * ====================================================================== */

wp_s32 wp_particle_system_add_emitter( wp_particle_system *ps, wp_particle_emitter *emitter )
{
    wp_particle_emitter **new_arr;
    wp_s32 new_cap;

    if( !ps || !emitter )
    {
        return -1;
    }

    if( ps->emitter_count >= ps->emitter_cap )
    {
        new_cap = ( ps->emitter_cap == 0 ) ? WORKPHONE_EMITTER_INITIAL_CAP : ps->emitter_cap * 2;
        new_arr = (wp_particle_emitter **)realloc( ps->emitters,
                                                   (wp_u32)new_cap * sizeof( wp_particle_emitter * ) );
        if( !new_arr )
        {
            return -1;
        }
        ps->emitters = new_arr;
        ps->emitter_cap = new_cap;
    }

    ps->emitters[ps->emitter_count] = emitter;
    ps->is_dirty = 1;
    return ps->emitter_count++;
}

void wp_particle_system_remove_emitter( wp_particle_system *ps, wp_s32 index )
{
    wp_s32 remaining;

    if( !ps || index < 0 || index >= ps->emitter_count )
    {
        return;
    }

    remaining = ps->emitter_count - index - 1;
    if( remaining > 0 )
    {
        memmove( &ps->emitters[index], &ps->emitters[index + 1],
                 (wp_u32)remaining * sizeof( wp_particle_emitter * ) );
    }

    ps->emitter_count--;
    ps->is_dirty = 1;
}

wp_particle_emitter *wp_particle_system_get_emitter( const wp_particle_system *ps, wp_s32 index )
{
    if( !ps || index < 0 || index >= ps->emitter_count )
    {
        return NULL;
    }
    return ps->emitters[index];
}

wp_s32 wp_particle_system_get_emitter_count( const wp_particle_system *ps )
{
    if( !ps )
    {
        return 0;
    }
    return ps->emitter_count;
}

/* =========================================================================
 * Particle system - Dirty flag and native handle
 * ====================================================================== */

wp_s32 wp_particle_system_is_dirty( const wp_particle_system *ps )
{
    if( !ps )
    {
        return 0;
    }
    return ps->is_dirty;
}

void wp_particle_system_mark_dirty( wp_particle_system *ps )
{
    if( !ps )
    {
        return;
    }
    ps->is_dirty = 1;
}

void wp_particle_system_clear_dirty( wp_particle_system *ps )
{
    if( !ps )
    {
        return;
    }
    ps->is_dirty = 0;
}

void *wp_particle_system_get_native( const wp_particle_system *ps )
{
    if( !ps )
    {
        return NULL;
    }
    return ps->native;
}

void wp_particle_system_set_native( wp_particle_system *ps, void *native )
{
    if( !ps )
    {
        return;
    }
    ps->native = native;
}

/* =========================================================================
 * Particle emitter - Lifecycle
 * ====================================================================== */

wp_particle_emitter *wp_particle_emitter_create( void )
{
    wp_particle_emitter *em = (wp_particle_emitter *)malloc( sizeof( wp_particle_emitter ) );
    if( !em )
    {
        return NULL;
    }

    memset( em, 0, sizeof( wp_particle_emitter ) );

    em->emission_rate = 10.0f;
    em->emission_rate_min = 10.0f;
    em->emission_rate_max = 10.0f;
    em->particle_size.x = 1.0f;
    em->particle_size.y = 1.0f;
    em->particle_size.z = 1.0f;
    em->particle_size_min = em->particle_size;
    em->particle_size_max = em->particle_size;
    em->direction.y = 1.0f;
    em->velocity = 1.0f;
    em->velocity_min = 1.0f;
    em->velocity_max = 1.0f;
    em->ttl = 5.0f;
    em->ttl_min = 5.0f;
    em->ttl_max = 5.0f;

    return em;
}

void wp_particle_emitter_destroy( wp_particle_emitter *em )
{
    if( !em )
    {
        return;
    }
    free( em );
}

/* =========================================================================
 * Particle emitter - Emission rate
 * ====================================================================== */

wp_f32 wp_particle_emitter_get_emission_rate( const wp_particle_emitter *em )
{
    if( !em )
    {
        return 0.0f;
    }
    return em->emission_rate;
}

void wp_particle_emitter_set_emission_rate( wp_particle_emitter *em, wp_f32 rate )
{
    if( !em )
    {
        return;
    }
    em->emission_rate = rate;
}

wp_f32 wp_particle_emitter_get_emission_rate_min( const wp_particle_emitter *em )
{
    if( !em )
    {
        return 0.0f;
    }
    return em->emission_rate_min;
}

void wp_particle_emitter_set_emission_rate_min( wp_particle_emitter *em, wp_f32 rate )
{
    if( !em )
    {
        return;
    }
    em->emission_rate_min = rate;
}

wp_f32 wp_particle_emitter_get_emission_rate_max( const wp_particle_emitter *em )
{
    if( !em )
    {
        return 0.0f;
    }
    return em->emission_rate_max;
}

void wp_particle_emitter_set_emission_rate_max( wp_particle_emitter *em, wp_f32 rate )
{
    if( !em )
    {
        return;
    }
    em->emission_rate_max = rate;
}

/* =========================================================================
 * Particle emitter - Particle size
 * ====================================================================== */

static wp_vec3f wp_vec3f_zero_val( void )
{
    wp_vec3f v;
    memset( &v, 0, sizeof( v ) );
    return v;
}

wp_vec3f wp_particle_emitter_get_particle_size( const wp_particle_emitter *em )
{
    if( !em )
    {
        return wp_vec3f_zero_val();
    }
    return em->particle_size;
}

void wp_particle_emitter_set_particle_size( wp_particle_emitter *em, wp_vec3f size )
{
    if( !em )
    {
        return;
    }
    em->particle_size = size;
}

wp_vec3f wp_particle_emitter_get_particle_size_min( const wp_particle_emitter *em )
{
    if( !em )
    {
        return wp_vec3f_zero_val();
    }
    return em->particle_size_min;
}

void wp_particle_emitter_set_particle_size_min( wp_particle_emitter *em, wp_vec3f size )
{
    if( !em )
    {
        return;
    }
    em->particle_size_min = size;
}

wp_vec3f wp_particle_emitter_get_particle_size_max( const wp_particle_emitter *em )
{
    if( !em )
    {
        return wp_vec3f_zero_val();
    }
    return em->particle_size_max;
}

void wp_particle_emitter_set_particle_size_max( wp_particle_emitter *em, wp_vec3f size )
{
    if( !em )
    {
        return;
    }
    em->particle_size_max = size;
}

/* =========================================================================
 * Particle emitter - Direction
 * ====================================================================== */

wp_vec3f wp_particle_emitter_get_direction( const wp_particle_emitter *em )
{
    if( !em )
    {
        return wp_vec3f_zero_val();
    }
    return em->direction;
}

void wp_particle_emitter_set_direction( wp_particle_emitter *em, wp_vec3f direction )
{
    if( !em )
    {
        return;
    }
    em->direction = direction;
}

/* =========================================================================
 * Particle emitter - Velocity
 * ====================================================================== */

wp_f32 wp_particle_emitter_get_velocity( const wp_particle_emitter *em )
{
    if( !em )
    {
        return 0.0f;
    }
    return em->velocity;
}

void wp_particle_emitter_set_velocity( wp_particle_emitter *em, wp_f32 velocity )
{
    if( !em )
    {
        return;
    }
    em->velocity = velocity;
}

wp_f32 wp_particle_emitter_get_velocity_min( const wp_particle_emitter *em )
{
    if( !em )
    {
        return 0.0f;
    }
    return em->velocity_min;
}

void wp_particle_emitter_set_velocity_min( wp_particle_emitter *em, wp_f32 velocity )
{
    if( !em )
    {
        return;
    }
    em->velocity_min = velocity;
}

wp_f32 wp_particle_emitter_get_velocity_max( const wp_particle_emitter *em )
{
    if( !em )
    {
        return 0.0f;
    }
    return em->velocity_max;
}

void wp_particle_emitter_set_velocity_max( wp_particle_emitter *em, wp_f32 velocity )
{
    if( !em )
    {
        return;
    }
    em->velocity_max = velocity;
}

/* =========================================================================
 * Particle emitter - Time-to-live
 * ====================================================================== */

wp_f32 wp_particle_emitter_get_ttl( const wp_particle_emitter *em )
{
    if( !em )
    {
        return 0.0f;
    }
    return em->ttl;
}

void wp_particle_emitter_set_ttl( wp_particle_emitter *em, wp_f32 ttl )
{
    if( !em )
    {
        return;
    }
    em->ttl = ttl;
}

wp_f32 wp_particle_emitter_get_ttl_min( const wp_particle_emitter *em )
{
    if( !em )
    {
        return 0.0f;
    }
    return em->ttl_min;
}

void wp_particle_emitter_set_ttl_min( wp_particle_emitter *em, wp_f32 ttl )
{
    if( !em )
    {
        return;
    }
    em->ttl_min = ttl;
}

wp_f32 wp_particle_emitter_get_ttl_max( const wp_particle_emitter *em )
{
    if( !em )
    {
        return 0.0f;
    }
    return em->ttl_max;
}

void wp_particle_emitter_set_ttl_max( wp_particle_emitter *em, wp_f32 ttl )
{
    if( !em )
    {
        return;
    }
    em->ttl_max = ttl;
}

/* =========================================================================
 * Particle emitter - Native handle
 * ====================================================================== */

void *wp_particle_emitter_get_native( const wp_particle_emitter *em )
{
    if( !em )
    {
        return NULL;
    }
    return em->native;
}

void wp_particle_emitter_set_native( wp_particle_emitter *em, void *native )
{
    if( !em )
    {
        return;
    }
    em->native = native;
}
