/**
 * @file workphone_physics_collision_cache.h
 * @brief Fixed-size cache for persistent collision pairs.
 */

#ifndef WORKPHONE_PHYSICS_COLLISION_CACHE_H
#define WORKPHONE_PHYSICS_COLLISION_CACHE_H

#include <stdint.h>
#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wp_collision_cache wp_collision_cache;
typedef struct wp_rigidbody wp_rigidbody;
typedef struct wp_collision_shape wp_collision_shape;

typedef struct wp_collision_cache_entry
{
    wp_rigidbody *body_a;
    wp_rigidbody *body_b;
    wp_collision_shape *shape_a;
    wp_collision_shape *shape_b;
    wp_u32 age;
    wp_u32 flags;
} wp_collision_cache_entry;

wp_collision_cache *wp_collision_cache_create( wp_s32 capacity );
void wp_collision_cache_destroy( wp_collision_cache *cache );
void wp_collision_cache_clear( wp_collision_cache *cache );

wp_s32 wp_collision_cache_add( wp_collision_cache *cache, wp_rigidbody *body_a,
                               wp_collision_shape *shape_a, wp_rigidbody *body_b,
                               wp_collision_shape *shape_b );
wp_s32 wp_collision_cache_find( const wp_collision_cache *cache, wp_rigidbody *body_a,
                                wp_collision_shape *shape_a, wp_rigidbody *body_b,
                                wp_collision_shape *shape_b );
void wp_collision_cache_remove_at( wp_collision_cache *cache, wp_s32 index );
void wp_collision_cache_step_ages( wp_collision_cache *cache );

const wp_collision_cache_entry *wp_collision_cache_get_entries( const wp_collision_cache *cache );
wp_s32 wp_collision_cache_get_count( const wp_collision_cache *cache );
wp_s32 wp_collision_cache_get_capacity( const wp_collision_cache *cache );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_PHYSICS_COLLISION_CACHE_H */
