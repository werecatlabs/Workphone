#ifndef WORKPHONE_PHYSICS_CACHE_INTERNAL_H
#define WORKPHONE_PHYSICS_CACHE_INTERNAL_H
#include "workphone_physics_collision_cache.h"
#include <stddef.h>

/* Dense entries and payloads grow/swap together. Utility and scene caches
 * share the same symmetric body/shape key and hash implementation. */
wp_collision_cache *wp_collision_cache_create_with_payload( wp_s32 capacity, size_t payload_size );
void *wp_collision_cache_payload( wp_collision_cache *cache, wp_s32 index );
wp_s32 wp_collision_cache_find_counted( const wp_collision_cache *cache, wp_rigidbody *a,
                                        wp_collision_shape *sa, wp_rigidbody *b,
                                        wp_collision_shape *sb, uint64_t *probes );
#endif
