/**
 * @file workphone_physics_collision_cache.c
 * @brief Fixed-size collision cache implementation.
 */

#include "workphone_physics_collision_cache.h"
#include <stdlib.h>
#include <string.h>

typedef struct wp_collision_cache
{
    wp_collision_cache_entry *entries;
    wp_s32 count;
    wp_s32 capacity;
} wp_collision_cache;

static wp_s32 same_pair( const wp_collision_cache_entry *e, wp_rigidbody *body_a,
                         wp_collision_shape *shape_a, wp_rigidbody *body_b, wp_collision_shape *shape_b )
{
    return ( e->body_a == body_a && e->shape_a == shape_a && e->body_b == body_b &&
             e->shape_b == shape_b ) ||
           ( e->body_a == body_b && e->shape_a == shape_b && e->body_b == body_a &&
             e->shape_b == shape_a );
}

wp_collision_cache *wp_collision_cache_create( wp_s32 capacity )
{
    wp_collision_cache *cache;
    if( capacity <= 0 )
    {
        capacity = 256;
    }
    cache = (wp_collision_cache *)malloc( sizeof( wp_collision_cache ) );
    if( !cache )
    {
        return NULL;
    }
    cache->entries =
        (wp_collision_cache_entry *)calloc( (size_t)capacity, sizeof( wp_collision_cache_entry ) );
    if( !cache->entries )
    {
        free( cache );
        return NULL;
    }
    cache->count = 0;
    cache->capacity = capacity;
    return cache;
}

void wp_collision_cache_destroy( wp_collision_cache *cache )
{
    if( cache )
    {
        free( cache->entries );
        free( cache );
    }
}

void wp_collision_cache_clear( wp_collision_cache *cache )
{
    if( cache )
    {
        memset( cache->entries, 0, (size_t)cache->capacity * sizeof( wp_collision_cache_entry ) );
        cache->count = 0;
    }
}

wp_s32 wp_collision_cache_add( wp_collision_cache *cache, wp_rigidbody *body_a,
                               wp_collision_shape *shape_a, wp_rigidbody *body_b,
                               wp_collision_shape *shape_b )
{
    wp_s32 existing;
    wp_collision_cache_entry *e;
    if( !cache || !body_a || !body_b || cache->count >= cache->capacity )
    {
        return -1;
    }
    existing = wp_collision_cache_find( cache, body_a, shape_a, body_b, shape_b );
    if( existing >= 0 )
    {
        cache->entries[existing].age = 0u;
        return existing;
    }
    e = &cache->entries[cache->count];
    e->body_a = body_a;
    e->shape_a = shape_a;
    e->body_b = body_b;
    e->shape_b = shape_b;
    e->age = 0u;
    e->flags = 0u;
    return cache->count++;
}

wp_s32 wp_collision_cache_find( const wp_collision_cache *cache, wp_rigidbody *body_a,
                                wp_collision_shape *shape_a, wp_rigidbody *body_b,
                                wp_collision_shape *shape_b )
{
    wp_s32 i;
    if( !cache )
    {
        return -1;
    }
    for( i = 0; i < cache->count; ++i )
    {
        if( same_pair( &cache->entries[i], body_a, shape_a, body_b, shape_b ) )
        {
            return i;
        }
    }
    return -1;
}

void wp_collision_cache_remove_at( wp_collision_cache *cache, wp_s32 index )
{
    if( !cache || index < 0 || index >= cache->count )
    {
        return;
    }
    cache->entries[index] = cache->entries[cache->count - 1];
    memset( &cache->entries[cache->count - 1], 0, sizeof( wp_collision_cache_entry ) );
    --cache->count;
}

void wp_collision_cache_step_ages( wp_collision_cache *cache )
{
    wp_s32 i;
    if( !cache )
    {
        return;
    }
    for( i = 0; i < cache->count; ++i )
    {
        ++cache->entries[i].age;
    }
}

const wp_collision_cache_entry *wp_collision_cache_get_entries( const wp_collision_cache *cache )
{
    return cache ? cache->entries : NULL;
}

wp_s32 wp_collision_cache_get_count( const wp_collision_cache *cache )
{
    return cache ? cache->count : 0;
}

wp_s32 wp_collision_cache_get_capacity( const wp_collision_cache *cache )
{
    return cache ? cache->capacity : 0;
}
