/**
 * @file wp_physics_collision.c
 * @brief Implementation of the C physics collision API.
 */

#include "workphone_physics_collision.h"
#include <stdlib.h>
#include <string.h>

typedef struct wp_raycast_hit
{
    wp_vec3f point;
    wp_vec3f normal;
    wp_f32 distance;
    wp_vec2f tex_coord;
    wp_vec2f tex_coord2;
    wp_vec2f lightmap_coord;
    wp_vec2f barycentric;
    wp_s32 triangle_index;
    wp_u32 collision_mask;
    wp_u32 flags;
    wp_collision_shape *shape;
    wp_rigidbody *body;
} wp_raycast_hit;

typedef struct wp_contact_pair
{
    wp_contact_event event;
    wp_rigidbody *body_a;
    wp_rigidbody *body_b;
    wp_collision_shape *shape_a;
    wp_collision_shape *shape_b;
    wp_vec3f point;
    wp_vec3f normal;
    wp_f32 impulse;
} wp_contact_pair;

static wp_vec3f vec3f_zero( void )
{
    wp_vec3f v;
    memset( &v, 0, sizeof( v ) );
    return v;
}

static wp_vec2f vec2f_zero( void )
{
    wp_vec2f v;
    memset( &v, 0, sizeof( v ) );
    return v;
}

wp_raycast_hit *wp_raycast_hit_create( void )
{
    wp_raycast_hit *hit = (wp_raycast_hit *)malloc( sizeof( wp_raycast_hit ) );
    if( !hit )
    {
        return NULL;
    }

    memset( hit, 0, sizeof( wp_raycast_hit ) );
    hit->triangle_index = -1;
    hit->flags = WORKPHONE_RAYCAST_FLAG_ALL;

    return hit;
}

void wp_raycast_hit_destroy( wp_raycast_hit *hit )
{
    if( !hit )
    {
        return;
    }

    free( hit );
}

wp_vec3f wp_raycast_hit_get_point( const wp_raycast_hit *hit )
{
    if( !hit )
    {
        return vec3f_zero();
    }
    return hit->point;
}

void wp_raycast_hit_set_point( wp_raycast_hit *hit, wp_vec3f point )
{
    if( !hit )
    {
        return;
    }

    hit->point = point;
}

wp_vec3f wp_raycast_hit_get_normal( const wp_raycast_hit *hit )
{
    if( !hit )
    {
        return vec3f_zero();
    }
    return hit->normal;
}

void wp_raycast_hit_set_normal( wp_raycast_hit *hit, wp_vec3f normal )
{
    if( !hit )
    {
        return;
    }
    hit->normal = normal;
}

wp_f32 wp_raycast_hit_get_distance( const wp_raycast_hit *hit )
{
    if( !hit )
    {
        return 0.0f;
    }
    return hit->distance;
}

void wp_raycast_hit_set_distance( wp_raycast_hit *hit, wp_f32 distance )
{
    if( !hit )
    {
        return;
    }
    hit->distance = distance;
}

wp_vec2f wp_raycast_hit_get_tex_coord( const wp_raycast_hit *hit )
{
    if( !hit )
    {
        return vec2f_zero();
    }
    return hit->tex_coord;
}

void wp_raycast_hit_set_tex_coord( wp_raycast_hit *hit, wp_vec2f uv )
{
    if( !hit )
    {
        return;
    }
    hit->tex_coord = uv;
}

wp_vec2f wp_raycast_hit_get_tex_coord2( const wp_raycast_hit *hit )
{
    if( !hit )
    {
        return vec2f_zero();
    }
    return hit->tex_coord2;
}

void wp_raycast_hit_set_tex_coord2( wp_raycast_hit *hit, wp_vec2f uv )
{
    if( !hit )
    {
        return;
    }
    hit->tex_coord2 = uv;
}

wp_vec2f wp_raycast_hit_get_lightmap_coord( const wp_raycast_hit *hit )
{
    if( !hit )
    {
        return vec2f_zero();
    }
    return hit->lightmap_coord;
}

void wp_raycast_hit_set_lightmap_coord( wp_raycast_hit *hit, wp_vec2f uv )
{
    if( !hit )
    {
        return;
    }
    hit->lightmap_coord = uv;
}

wp_vec2f wp_raycast_hit_get_barycentric( const wp_raycast_hit *hit )
{
    if( !hit )
    {
        return vec2f_zero();
    }
    return hit->barycentric;
}

void wp_raycast_hit_set_barycentric( wp_raycast_hit *hit, wp_vec2f bary )
{
    if( !hit )
    {
        return;
    }
    hit->barycentric = bary;
}

wp_s32 wp_raycast_hit_get_triangle_index( const wp_raycast_hit *hit )
{
    if( !hit )
    {
        return -1;
    }
    return hit->triangle_index;
}

void wp_raycast_hit_set_triangle_index( wp_raycast_hit *hit, wp_s32 index )
{
    if( !hit )
    {
        return;
    }
    hit->triangle_index = index;
}

wp_u32 wp_raycast_hit_get_collision_mask( const wp_raycast_hit *hit )
{
    if( !hit )
    {
        return 0;
    }
    return hit->collision_mask;
}

void wp_raycast_hit_set_collision_mask( wp_raycast_hit *hit, wp_u32 mask )
{
    if( !hit )
    {
        return;
    }
    hit->collision_mask = mask;
}

wp_u32 wp_raycast_hit_get_flags( const wp_raycast_hit *hit )
{
    if( !hit )
    {
        return 0;
    }
    return hit->flags;
}

void wp_raycast_hit_set_flags( wp_raycast_hit *hit, wp_u32 flags )
{
    if( !hit )
    {
        return;
    }
    hit->flags = flags;
}

wp_collision_shape *wp_raycast_hit_get_shape( const wp_raycast_hit *hit )
{
    if( !hit )
    {
        return NULL;
    }
    return hit->shape;
}

void wp_raycast_hit_set_shape( wp_raycast_hit *hit, wp_collision_shape *shape )
{
    if( !hit )
    {
        return;
    }
    hit->shape = shape;
}

wp_rigidbody *wp_raycast_hit_get_body( const wp_raycast_hit *hit )
{
    if( !hit )
    {
        return NULL;
    }
    return hit->body;
}

void wp_raycast_hit_set_body( wp_raycast_hit *hit, wp_rigidbody *body )
{
    if( !hit )
    {
        return;
    }
    hit->body = body;
}

/* =========================================================================
 * Contact pair - Lifecycle
 * ====================================================================== */

wp_contact_pair *wp_contact_pair_create( void )
{
    wp_contact_pair *pair = (wp_contact_pair *)malloc( sizeof( wp_contact_pair ) );
    if( !pair )
    {
        return NULL;
    }

    memset( pair, 0, sizeof( wp_contact_pair ) );
    pair->event = WORKPHONE_CONTACT_EVENT_BEGIN;

    return pair;
}

void wp_contact_pair_destroy( wp_contact_pair *pair )
{
    if( !pair )
    {
        return;
    }

    free( pair );
}

/* =========================================================================
 * Contact pair - Event type
 * ====================================================================== */

wp_contact_event wp_contact_pair_get_event( const wp_contact_pair *pair )
{
    if( !pair )
    {
        return WORKPHONE_CONTACT_EVENT_BEGIN;
    }
    return pair->event;
}

void wp_contact_pair_set_event( wp_contact_pair *pair, wp_contact_event event )
{
    if( !pair )
    {
        return;
    }
    pair->event = event;
}

/* =========================================================================
 * Contact pair - Body and shape references
 * ====================================================================== */

wp_rigidbody *wp_contact_pair_get_body_a( const wp_contact_pair *pair )
{
    if( !pair )
    {
        return NULL;
    }
    return pair->body_a;
}

void wp_contact_pair_set_body_a( wp_contact_pair *pair, wp_rigidbody *body )
{
    if( !pair )
    {
        return;
    }
    pair->body_a = body;
}

wp_rigidbody *wp_contact_pair_get_body_b( const wp_contact_pair *pair )
{
    if( !pair )
    {
        return NULL;
    }
    return pair->body_b;
}

void wp_contact_pair_set_body_b( wp_contact_pair *pair, wp_rigidbody *body )
{
    if( !pair )
    {
        return;
    }
    pair->body_b = body;
}

wp_collision_shape *wp_contact_pair_get_shape_a( const wp_contact_pair *pair )
{
    if( !pair )
    {
        return NULL;
    }

    return pair->shape_a;
}

void wp_contact_pair_set_shape_a( wp_contact_pair *pair, wp_collision_shape *shape )
{
    if( !pair )
    {
        return;
    }

    pair->shape_a = shape;
}

wp_collision_shape *wp_contact_pair_get_shape_b( const wp_contact_pair *pair )
{
    if( !pair )
    {
        return NULL;
    }

    return pair->shape_b;
}

void wp_contact_pair_set_shape_b( wp_contact_pair *pair, wp_collision_shape *shape )
{
    if( !pair )
    {
        return;
    }
    pair->shape_b = shape;
}

wp_vec3f wp_contact_pair_get_point( const wp_contact_pair *pair )
{
    if( !pair )
    {
        return vec3f_zero();
    }
    return pair->point;
}

void wp_contact_pair_set_point( wp_contact_pair *pair, wp_vec3f point )
{
    if( !pair )
    {
        return;
    }

    pair->point = point;
}

wp_vec3f wp_contact_pair_get_normal( const wp_contact_pair *pair )
{
    if( !pair )
    {
        return vec3f_zero();
    }

    return pair->normal;
}

void wp_contact_pair_set_normal( wp_contact_pair *pair, wp_vec3f normal )
{
    if( !pair )
    {
        return;
    }

    pair->normal = normal;
}

wp_f32 wp_contact_pair_get_impulse( const wp_contact_pair *pair )
{
    if( !pair )
    {
        return 0.0f;
    }

    return pair->impulse;
}

void wp_contact_pair_set_impulse( wp_contact_pair *pair, wp_f32 impulse )
{
    if( !pair )
    {
        return;
    }
    pair->impulse = impulse;
}
