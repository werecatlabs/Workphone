#include "workphone_physics_cache_internal.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct wp_collision_cache
{
    wp_collision_cache_entry *entries;
    wp_s32 *buckets, *next;
    unsigned char *payload;
    size_t payload_size;
    wp_s32 count, capacity;
};
static size_t pair_hash( wp_rigidbody *a, wp_collision_shape *sa, wp_rigidbody *b,
                         wp_collision_shape *sb )
{
    uintptr_t aa = (uintptr_t)a, ab = (uintptr_t)sa, ba = (uintptr_t)b, bb = (uintptr_t)sb;
    uint64_t h;
    if ( aa > ba || ( aa == ba && ab > bb ) )
    {
        uintptr_t t = aa;
        aa = ba;
        ba = t;
        t = ab;
        ab = bb;
        bb = t;
    }
    h = (uint64_t)aa ^ ( (uint64_t)ab * UINT64_C( 0x9e3779b97f4a7c15 ) ) ^
        ( (uint64_t)ba * UINT64_C( 0xbf58476d1ce4e5b9 ) ) ^
        ( (uint64_t)bb * UINT64_C( 0x94d049bb133111eb ) );
    h ^= h >> 30;
    h *= UINT64_C( 0xbf58476d1ce4e5b9 );
    h ^= h >> 27;
    h *= UINT64_C( 0x94d049bb133111eb );
    return (size_t)( h ^ ( h >> 31 ) );
}
static wp_s32 bucket_for( const wp_collision_cache *c, const wp_collision_cache_entry *e )
{
    return (wp_s32)( pair_hash( e->body_a, e->shape_a, e->body_b, e->shape_b ) &
                     (size_t)( c->capacity * 2 - 1 ) );
}
static wp_s32 reserve( wp_collision_cache *c, wp_s32 wanted )
{
    wp_s32 cap = c->capacity ? c->capacity : 16, i;
    wp_collision_cache_entry *entries;
    wp_s32 *buckets, *next;
    unsigned char *payload = NULL;
    while ( cap < wanted )
    {
        if ( cap > INT_MAX / 4 )
            return 0;
        cap *= 2;
    }
    if ( cap == c->capacity )
        return 1;
    if ( cap > INT_MAX / 2 || (size_t)cap > SIZE_MAX / sizeof( *entries ) ||
         ( c->payload_size && (size_t)cap > SIZE_MAX / c->payload_size ) )
        return 0;
    entries = (wp_collision_cache_entry *)calloc( (size_t)cap, sizeof( *entries ) );
    buckets = (wp_s32 *)malloc( (size_t)cap * 2 * sizeof( *buckets ) );
    next = (wp_s32 *)malloc( (size_t)cap * sizeof( *next ) );
    if ( c->payload_size )
        payload = (unsigned char *)calloc( (size_t)cap, c->payload_size );
    if ( !entries || !buckets || !next || ( c->payload_size && !payload ) )
    {
        free( entries );
        free( buckets );
        free( next );
        free( payload );
        return 0;
    }
    if ( c->count )
    {
        memcpy( entries, c->entries, (size_t)c->count * sizeof( *entries ) );
        if ( payload )
            memcpy( payload, c->payload, (size_t)c->count * c->payload_size );
    }
    free( c->entries );
    free( c->buckets );
    free( c->next );
    free( c->payload );
    c->entries = entries;
    c->buckets = buckets;
    c->next = next;
    c->payload = payload;
    c->capacity = cap;
    memset( buckets, 0xff, (size_t)cap * 2 * sizeof( *buckets ) );
    for ( i = 0; i < c->count; ++i )
    {
        wp_s32 b = bucket_for( c, &entries[i] );
        next[i] = buckets[b];
        buckets[b] = i;
    }
    return 1;
}
wp_collision_cache *wp_collision_cache_create_with_payload( wp_s32 capacity, size_t payload_size )
{
    wp_collision_cache *c = (wp_collision_cache *)calloc( 1, sizeof( *c ) );
    if ( !c )
        return NULL;
    c->payload_size = payload_size;
    if ( !reserve( c, capacity > 0 ? capacity : 256 ) )
    {
        free( c );
        return NULL;
    }
    return c;
}
wp_collision_cache *wp_collision_cache_create( wp_s32 capacity )
{
    return wp_collision_cache_create_with_payload( capacity, 0 );
}
void wp_collision_cache_destroy( wp_collision_cache *c )
{
    if ( c )
    {
        free( c->entries );
        free( c->buckets );
        free( c->next );
        free( c->payload );
        free( c );
    }
}
void wp_collision_cache_clear( wp_collision_cache *c )
{
    if ( !c )
        return;
    c->count = 0;
    memset( c->buckets, 0xff, (size_t)c->capacity * 2 * sizeof( *c->buckets ) );
}
wp_s32 wp_collision_cache_find_counted( const wp_collision_cache *c, wp_rigidbody *a,
                                        wp_collision_shape *sa, wp_rigidbody *b,
                                        wp_collision_shape *sb, uint64_t *probes )
{
    wp_s32 i;
    if ( !c )
        return -1;
    i = c->buckets[pair_hash( a, sa, b, sb ) & (size_t)( c->capacity * 2 - 1 )];
    while ( i >= 0 )
    {
        const wp_collision_cache_entry *e = &c->entries[i];
        if ( probes )
            ++*probes;
        if ( ( e->body_a == a && e->shape_a == sa && e->body_b == b && e->shape_b == sb ) ||
             ( e->body_a == b && e->shape_a == sb && e->body_b == a && e->shape_b == sa ) )
            return i;
        i = c->next[i];
    }
    return -1;
}
wp_s32 wp_collision_cache_find( const wp_collision_cache *c, wp_rigidbody *a,
                                wp_collision_shape *sa, wp_rigidbody *b, wp_collision_shape *sb )
{
    return wp_collision_cache_find_counted( c, a, sa, b, sb, NULL );
}
wp_s32 wp_collision_cache_add( wp_collision_cache *c, wp_rigidbody *a, wp_collision_shape *sa,
                               wp_rigidbody *b, wp_collision_shape *sb )
{
    wp_s32 i, bucket;
    wp_collision_cache_entry *e;
    if ( !c || !a || !b )
        return -1;
    i = wp_collision_cache_find( c, a, sa, b, sb );
    if ( i >= 0 )
    {
        c->entries[i].age = 0;
        return i;
    }
    if ( c->count == INT_MAX || !reserve( c, c->count + 1 ) )
        return -1;
    i = c->count++;
    e = &c->entries[i];
    *e = ( wp_collision_cache_entry ){ a, b, sa, sb, 0, 0 };
    if ( c->payload_size )
        memset( c->payload + (size_t)i * c->payload_size, 0, c->payload_size );
    bucket = bucket_for( c, e );
    c->next[i] = c->buckets[bucket];
    c->buckets[bucket] = i;
    return i;
}
static wp_s32 *link_for( wp_collision_cache *c, wp_s32 index )
{
    wp_s32 *link = &c->buckets[bucket_for( c, &c->entries[index] )];
    while ( *link != index )
        link = &c->next[*link];
    return link;
}
void wp_collision_cache_remove_at( wp_collision_cache *c, wp_s32 index )
{
    wp_s32 *link, last;
    if ( !c || index < 0 || index >= c->count )
        return;
    link = link_for( c, index );
    *link = c->next[index];
    last = --c->count;
    if ( index != last )
    {
        link = link_for( c, last );
        *link = index;
        c->entries[index] = c->entries[last];
        c->next[index] = c->next[last];
        if ( c->payload_size )
            memcpy( c->payload + (size_t)index * c->payload_size,
                    c->payload + (size_t)last * c->payload_size, c->payload_size );
    }
}
void *wp_collision_cache_payload( wp_collision_cache *c, wp_s32 index )
{
    return c && index >= 0 && index < c->count && c->payload
               ? c->payload + (size_t)index * c->payload_size
               : NULL;
}
void wp_collision_cache_step_ages( wp_collision_cache *c )
{
    if ( c )
        for ( wp_s32 i = 0; i < c->count; ++i )
            if ( c->entries[i].age < UINT32_MAX )
                ++c->entries[i].age;
}
const wp_collision_cache_entry *wp_collision_cache_get_entries( const wp_collision_cache *c )
{
    return c ? c->entries : NULL;
}
wp_s32 wp_collision_cache_get_count( const wp_collision_cache *c )
{
    return c ? c->count : 0;
}
wp_s32 wp_collision_cache_get_capacity( const wp_collision_cache *c )
{
    return c ? c->capacity : 0;
}
