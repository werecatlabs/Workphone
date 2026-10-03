/**
 * @file wp_physics_collision.h
 * @brief C API for physics collision queries and results.
 *
 * This module provides data structures and functions for raycasting,
 * line-segment intersection testing, and collision contact reporting.
 * A wp_raycast_hit stores the full result of a single ray/line query
 * (hit point, normal, distance, UV coordinates, triangle index, etc.).
 * A wp_contact_pair describes a collision event between two rigid bodies.
 */

#ifndef WORKPHONE_PHYSICS_COLLISION_H
#define WORKPHONE_PHYSICS_COLLISION_H

#include <stdint.h>
#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wp_raycast_hit wp_raycast_hit;
typedef struct wp_contact_pair wp_contact_pair;
typedef struct wp_collision_shape wp_collision_shape;
typedef struct wp_rigidbody wp_rigidbody;

enum
{
    WORKPHONE_RAYCAST_FLAG_STATIC = ( 1u << 0 ),
    WORKPHONE_RAYCAST_FLAG_DYNAMIC = ( 1u << 1 ),
    WORKPHONE_RAYCAST_FLAG_ALL = ( 1u << 0 ) | ( 1u << 1 )
};

typedef enum wp_contact_event
{
    WORKPHONE_CONTACT_EVENT_BEGIN = 0,
    WORKPHONE_CONTACT_EVENT_PERSIST = 1,
    WORKPHONE_CONTACT_EVENT_END = 2
} wp_contact_event;

/* =========================================================================
 * Raycast hit
 * ====================================================================== */

wp_raycast_hit *wp_raycast_hit_create( void );
void wp_raycast_hit_destroy( wp_raycast_hit *hit );

wp_vec3f wp_raycast_hit_get_point( const wp_raycast_hit *hit );
void wp_raycast_hit_set_point( wp_raycast_hit *hit, wp_vec3f powp_s32 );

wp_vec3f wp_raycast_hit_get_normal( const wp_raycast_hit *hit );
void wp_raycast_hit_set_normal( wp_raycast_hit *hit, wp_vec3f normal );

wp_f32 wp_raycast_hit_get_distance( const wp_raycast_hit *hit );
void wp_raycast_hit_set_distance( wp_raycast_hit *hit, wp_f32 distance );

wp_vec2f wp_raycast_hit_get_tex_coord( const wp_raycast_hit *hit );
void wp_raycast_hit_set_tex_coord( wp_raycast_hit *hit, wp_vec2f uv );

wp_vec2f wp_raycast_hit_get_tex_coord2( const wp_raycast_hit *hit );
void wp_raycast_hit_set_tex_coord2( wp_raycast_hit *hit, wp_vec2f uv );

wp_vec2f wp_raycast_hit_get_lightmap_coord( const wp_raycast_hit *hit );
void wp_raycast_hit_set_lightmap_coord( wp_raycast_hit *hit, wp_vec2f uv );

wp_vec2f wp_raycast_hit_get_barycentric( const wp_raycast_hit *hit );
void wp_raycast_hit_set_barycentric( wp_raycast_hit *hit, wp_vec2f bary );

wp_s32 wp_raycast_hit_get_triangle_index( const wp_raycast_hit *hit );
void wp_raycast_hit_set_triangle_index( wp_raycast_hit *hit, wp_s32 index );

wp_u32 wp_raycast_hit_get_collision_mask( const wp_raycast_hit *hit );
void wp_raycast_hit_set_collision_mask( wp_raycast_hit *hit, wp_u32 mask );

wp_u32 wp_raycast_hit_get_flags( const wp_raycast_hit *hit );
void wp_raycast_hit_set_flags( wp_raycast_hit *hit, wp_u32 flags );

wp_collision_shape *wp_raycast_hit_get_shape( const wp_raycast_hit *hit );
void wp_raycast_hit_set_shape( wp_raycast_hit *hit, wp_collision_shape *shape );

wp_rigidbody *wp_raycast_hit_get_body( const wp_raycast_hit *hit );
void wp_raycast_hit_set_body( wp_raycast_hit *hit, wp_rigidbody *body );

/* =========================================================================
 * Contact pair
 * ====================================================================== */

wp_contact_pair *wp_contact_pair_create( void );
void wp_contact_pair_destroy( wp_contact_pair *pair );

wp_contact_event wp_contact_pair_get_event( const wp_contact_pair *pair );
void wp_contact_pair_set_event( wp_contact_pair *pair, wp_contact_event event );

wp_rigidbody *wp_contact_pair_get_body_a( const wp_contact_pair *pair );
void wp_contact_pair_set_body_a( wp_contact_pair *pair, wp_rigidbody *body );

wp_rigidbody *wp_contact_pair_get_body_b( const wp_contact_pair *pair );
void wp_contact_pair_set_body_b( wp_contact_pair *pair, wp_rigidbody *body );

wp_collision_shape *wp_contact_pair_get_shape_a( const wp_contact_pair *pair );
void wp_contact_pair_set_shape_a( wp_contact_pair *pair, wp_collision_shape *shape );

wp_collision_shape *wp_contact_pair_get_shape_b( const wp_contact_pair *pair );
void wp_contact_pair_set_shape_b( wp_contact_pair *pair, wp_collision_shape *shape );

wp_vec3f wp_contact_pair_get_point( const wp_contact_pair *pair );
void wp_contact_pair_set_point( wp_contact_pair *pair, wp_vec3f powp_s32 );

wp_vec3f wp_contact_pair_get_normal( const wp_contact_pair *pair );
void wp_contact_pair_set_normal( wp_contact_pair *pair, wp_vec3f normal );

wp_f32 wp_contact_pair_get_impulse( const wp_contact_pair *pair );
void wp_contact_pair_set_impulse( wp_contact_pair *pair, wp_f32 impulse );

#ifdef __cplusplus
}
#endif

#endif
