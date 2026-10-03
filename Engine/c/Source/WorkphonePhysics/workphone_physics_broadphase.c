/**
 * @file workphone_physics_broadphase.c
 * @brief Implementation of the C physics broadphase API.
 */

#include "workphone_physics_broadphase.h"
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal constants
 * ====================================================================== */

#ifndef WP_BROADPHASE_MAX_PROXIES
#    define WP_BROADPHASE_MAX_PROXIES 512
#endif

#ifndef WP_BROADPHASE_MAX_PAIRS
#    define WP_BROADPHASE_MAX_PAIRS ( WP_BROADPHASE_MAX_PROXIES * 4 )
#endif

/* =========================================================================
 * Internal structures
 * ====================================================================== */

typedef struct wp_proxy
{
    wp_rigidbody *body;
    wp_vec3f aabb_min;
    wp_vec3f aabb_max;
    wp_s32 active;
} wp_proxy;

typedef struct wp_broadphase
{
    wp_broadphase_type type;

    wp_vec3f world_min;
    wp_vec3f world_max;

    wp_proxy proxies[WP_BROADPHASE_MAX_PROXIES];
    wp_s32 proxy_count;

    wp_broadphase_pair pairs[WP_BROADPHASE_MAX_PAIRS];
    wp_s32 pair_count;

    void *native;
    void *user_data;
} wp_broadphase;

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static wp_vec3f wp_vec3f_zero_bp( void )
{
    wp_vec3f v;
    memset( &v, 0, sizeof( v ) );
    return v;
}

static wp_s32 aabbs_overlap( const wp_proxy *a, const wp_proxy *b )
{
    if( a->aabb_max.x < b->aabb_min.x || a->aabb_min.x > b->aabb_max.x )
        return 0;
    if( a->aabb_max.y < b->aabb_min.y || a->aabb_min.y > b->aabb_max.y )
        return 0;
    if( a->aabb_max.z < b->aabb_min.z || a->aabb_min.z > b->aabb_max.z )
        return 0;
    return 1;
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_broadphase *wp_broadphase_create( wp_broadphase_type type )
{
    wp_broadphase *bp = (wp_broadphase *)malloc( sizeof( wp_broadphase ) );
    if( !bp )
    {
        return NULL;
    }

    memset( bp, 0, sizeof( wp_broadphase ) );

    bp->type = type;

    /* Default world bounds */
    bp->world_min.x = -1000.0f;
    bp->world_min.y = -1000.0f;
    bp->world_min.z = -1000.0f;
    bp->world_max.x = 1000.0f;
    bp->world_max.y = 1000.0f;
    bp->world_max.z = 1000.0f;

    return bp;
}

void wp_broadphase_destroy( wp_broadphase *bp )
{
    if( !bp )
    {
        return;
    }

    free( bp );
}

/* =========================================================================
 * Configuration
 * ====================================================================== */

wp_broadphase_type wp_broadphase_get_type( const wp_broadphase *bp )
{
    if( !bp )
    {
        return WORKPHONE_BROADPHASE_SAP;
    }
    return bp->type;
}

wp_vec3f wp_broadphase_get_world_min( const wp_broadphase *bp )
{
    if( !bp )
    {
        return wp_vec3f_zero_bp();
    }
    return bp->world_min;
}

void wp_broadphase_set_world_min( wp_broadphase *bp, wp_vec3f min )
{
    if( !bp )
    {
        return;
    }
    bp->world_min = min;
}

wp_vec3f wp_broadphase_get_world_max( const wp_broadphase *bp )
{
    if( !bp )
    {
        return wp_vec3f_zero_bp();
    }
    return bp->world_max;
}

void wp_broadphase_set_world_max( wp_broadphase *bp, wp_vec3f max )
{
    if( !bp )
    {
        return;
    }
    bp->world_max = max;
}

/* =========================================================================
 * Proxy management
 * ====================================================================== */

wp_s32 wp_broadphase_add_proxy( wp_broadphase *bp, wp_rigidbody *body, wp_vec3f aabb_min,
                                wp_vec3f aabb_max )
{
    wp_proxy *proxy;

    if( !bp || !body || bp->proxy_count >= WP_BROADPHASE_MAX_PROXIES )
    {
        return 0;
    }

    proxy = &bp->proxies[bp->proxy_count++];
    proxy->body = body;
    proxy->aabb_min = aabb_min;
    proxy->aabb_max = aabb_max;
    proxy->active = 1;

    return 1;
}

void wp_broadphase_remove_proxy( wp_broadphase *bp, wp_rigidbody *body )
{
    wp_s32 i;

    if( !bp || !body )
    {
        return;
    }

    for( i = 0; i < bp->proxy_count; ++i )
    {
        if( bp->proxies[i].body == body )
        {
            bp->proxies[i] = bp->proxies[bp->proxy_count - 1];
            memset( &bp->proxies[bp->proxy_count - 1], 0, sizeof( wp_proxy ) );
            --bp->proxy_count;
            return;
        }
    }
}

void wp_broadphase_update_proxy( wp_broadphase *bp, wp_rigidbody *body, wp_vec3f aabb_min,
                                 wp_vec3f aabb_max )
{
    wp_s32 i;

    if( !bp || !body )
    {
        return;
    }

    for( i = 0; i < bp->proxy_count; ++i )
    {
        if( bp->proxies[i].body == body )
        {
            bp->proxies[i].aabb_min = aabb_min;
            bp->proxies[i].aabb_max = aabb_max;
            return;
        }
    }
}

/* =========================================================================
 * Overlap pair cache
 * ====================================================================== */

void wp_broadphase_calculate_overlapping_pairs( wp_broadphase *bp )
{
    wp_s32 i, j;

    if( !bp )
    {
        return;
    }

    bp->pair_count = 0;

    /*
     * Brute-force O(n^2) AABB overlap test. When a native back-end is
     * bound (e.g. PhysX MBP / Bullet DBVT), this should delegate to it
     * instead.
     */
    for( i = 0; i < bp->proxy_count; ++i )
    {
        if( !bp->proxies[i].active )
        {
            continue;
        }

        for( j = i + 1; j < bp->proxy_count; ++j )
        {
            if( !bp->proxies[j].active )
            {
                continue;
            }

            if( bp->pair_count >= WP_BROADPHASE_MAX_PAIRS )
            {
                break;
            }

            if( aabbs_overlap( &bp->proxies[i], &bp->proxies[j] ) )
            {
                bp->pairs[bp->pair_count].body_a = bp->proxies[i].body;
                bp->pairs[bp->pair_count].body_b = bp->proxies[j].body;
                ++bp->pair_count;
            }
        }
    }
}

const wp_broadphase_pair *wp_broadphase_get_pair_cache( const wp_broadphase *bp )
{
    if( !bp || bp->pair_count == 0 )
    {
        return NULL;
    }
    return bp->pairs;
}

wp_s32 wp_broadphase_get_pair_count( const wp_broadphase *bp )
{
    if( !bp )
    {
        return 0;
    }
    return bp->pair_count;
}

/* =========================================================================
 * AABB query
 * ====================================================================== */

wp_s32 wp_broadphase_query_aabb( wp_broadphase *bp, wp_vec3f aabb_min, wp_vec3f aabb_max,
                                 wp_rigidbody **out_bodies, wp_s32 max_results )
{
    wp_s32 i;
    wp_s32 found = 0;
    wp_proxy query;

    if( !bp || !out_bodies || max_results <= 0 )
    {
        return 0;
    }

    memset( &query, 0, sizeof( query ) );
    query.aabb_min = aabb_min;
    query.aabb_max = aabb_max;
    query.active = 1;

    for( i = 0; i < bp->proxy_count && found < max_results; ++i )
    {
        if( !bp->proxies[i].active )
        {
            continue;
        }

        if( aabbs_overlap( &bp->proxies[i], &query ) )
        {
            out_bodies[found++] = bp->proxies[i].body;
        }
    }

    return found;
}

/* =========================================================================
 * Statistics
 * ====================================================================== */

wp_s32 wp_broadphase_get_proxy_count( const wp_broadphase *bp )
{
    if( !bp )
    {
        return 0;
    }
    return bp->proxy_count;
}

/* =========================================================================
 * Native / user data
 * ====================================================================== */

void *wp_broadphase_get_native( const wp_broadphase *bp )
{
    if( !bp )
    {
        return NULL;
    }
    return bp->native;
}

void wp_broadphase_set_native( wp_broadphase *bp, void *native )
{
    if( !bp )
    {
        return;
    }
    bp->native = native;
}

void *wp_broadphase_get_user_data( const wp_broadphase *bp )
{
    if( !bp )
    {
        return NULL;
    }
    return bp->user_data;
}

void wp_broadphase_set_user_data( wp_broadphase *bp, void *user_data )
{
    if( !bp )
    {
        return;
    }
    bp->user_data = user_data;
}
