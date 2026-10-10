/**
 * @file wp_physics_scene.c
 * @brief Implementation of the C physics scene API.
 */

#include "workphone_physics_scene.h"
#include <limits.h>
#include "workphone_physics_rigidbody.h"
#include "workphone_physics_collisionshape.h"
#include "workphone_physics_constraint.h"
#include "workphone_physics_narrowphase.h"
#include "workphone_physics_triangle_mesh.h"
#include "workphone_physics_bounds.h"
#include "workphone_physics_geometry.h"
#include "workphone_physics_cache_internal.h"
#include "workphone_physics_narrowphase_internal.h"
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* =========================================================================
 * Forward declarations for internal types
 * ====================================================================== */

typedef struct wp_contact_cache_record
{
    wp_contact_manifold manifold;
    uint64_t body_ids[2], shape_ids[2], revisions[2], shape_revisions[2];
    uint64_t seen_epoch, last_update_frame;
} wp_contact_cache_record;

typedef struct wp_actor_bounds_cache {
    uint64_t revision;
    wp_s32 valid;
    wp_s32 borrowed_vertices;
} wp_actor_bounds_cache;

/* =========================================================================
 * Internal structure
 * ====================================================================== */

typedef struct wp_physics_scene
{
    wp_rigidbody **actors;
    wp_s32 actor_count, actor_capacity;
    wp_f32 *sleep_time;
    wp_s32 *actor_proxies;
    wp_actor_bounds_cache *bounds_cache;
    wp_scene_broadphase_stats broadphase_stats;
    wp_s32 obb_enabled;

    wp_vec3f size;
    wp_vec3f gravity;

    wp_u32 min_threads;
    wp_u32 max_threads;

    wp_narrowphase *narrowphase;
    wp_spatial_partitioning_method spatial_partitioning;

    wp_contact_options contact_options;
    wp_collision_cache *contact_cache;
    uint64_t contact_epoch, frame_count;
    wp_broadphase *broadphase;

    void *native;
    void *user_data;
} wp_physics_scene;

typedef struct wp_scene_aabb
{
    wp_vec3f min;
    wp_vec3f max;
    wp_s32 valid;
} wp_scene_aabb;

static wp_vec3f vec3f_zero( void )
{
    wp_vec3f v;
    memset( &v, 0, sizeof( v ) );
    return v;
}

static wp_vec3f vec3f_add( wp_vec3f a, wp_vec3f b )
{
    wp_vec3f r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

static wp_vec3f vec3f_sub( wp_vec3f a, wp_vec3f b )
{
    wp_vec3f r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

static wp_vec3f vec3f_cross( wp_vec3f a, wp_vec3f b )
{
    wp_vec3f r;
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}

static wp_vec3f vec3f_scale( wp_vec3f v, wp_f32 s )
{
    wp_vec3f r;
    r.x = v.x * s;
    r.y = v.y * s;
    r.z = v.z * s;
    return r;
}

static wp_f32 vec3f_dot( wp_vec3f a, wp_vec3f b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static wp_vec3f vec3f_normalize( wp_vec3f v )
{
    wp_f32 len = sqrtf( vec3f_dot( v, v ) );
    if( len <= 1.0e-7f )
    {
        return vec3f_zero();
    }
    return vec3f_scale( v, 1.0f / len );
}

static wp_f32 clampf_scene( wp_f32 value, wp_f32 minimum, wp_f32 maximum )
{
    return value < minimum ? minimum : ( value > maximum ? maximum : value );
}

static wp_quatf quatf_normalize( wp_quatf q )
{
    wp_f32 length = sqrtf( q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w );
    if( length <= 1.0e-8f )
    {
        memset( &q, 0, sizeof( q ) );
        q.w = 1.0f;
        return q;
    }

    q.x /= length;
    q.y /= length;
    q.z /= length;
    q.w /= length;
    return q;
}

static wp_quatf quatf_multiply( wp_quatf a, wp_quatf b )
{
    wp_quatf q;
    q.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    q.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    q.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    q.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    return q;
}

static wp_quatf quatf_conjugate( wp_quatf q )
{
    q.x = -q.x;
    q.y = -q.y;
    q.z = -q.z;
    return q;
}

static wp_quatf quatf_nlerp( wp_quatf from, wp_quatf to, wp_f32 amount )
{
    wp_quatf result;
    wp_f32 dot = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
    if( dot < 0.0f )
    {
        to.x = -to.x;
        to.y = -to.y;
        to.z = -to.z;
        to.w = -to.w;
    }
    amount = clampf_scene( amount, 0.0f, 1.0f );
    result.x = from.x + ( to.x - from.x ) * amount;
    result.y = from.y + ( to.y - from.y ) * amount;
    result.z = from.z + ( to.z - from.z ) * amount;
    result.w = from.w + ( to.w - from.w ) * amount;
    return quatf_normalize( result );
}

static wp_vec3f quatf_rotate( wp_quatf q, wp_vec3f v )
{
    wp_vec3f qv;
    wp_vec3f t;

    q = quatf_normalize( q );
    qv.x = q.x;
    qv.y = q.y;
    qv.z = q.z;
    t = vec3f_scale( vec3f_cross( qv, v ), 2.0f );
    return vec3f_add( v, vec3f_add( vec3f_scale( t, q.w ), vec3f_cross( qv, t ) ) );
}

static wp_vec3f shape_world_position( const wp_rigidbody *body, const wp_collision_shape *shape )
{
    wp_vec3f body_position = wp_rigidbody_get_position( body );
    wp_quatf body_orientation = wp_rigidbody_get_orientation( body );
    wp_vec3f local_position = wp_collision_shape_get_local_position( shape );
    return vec3f_add( body_position, quatf_rotate( body_orientation, local_position ) );
}

static wp_quatf shape_world_orientation( const wp_rigidbody *body, const wp_collision_shape *shape )
{
    return quatf_normalize( quatf_multiply( wp_rigidbody_get_orientation( body ),
                                            wp_collision_shape_get_local_orientation( shape ) ) );
}

static void scene_aabb_set_center_extents( wp_scene_aabb *bounds, wp_vec3f center, wp_vec3f extents )
{
    bounds->min.x = center.x - extents.x;
    bounds->min.y = center.y - extents.y;
    bounds->min.z = center.z - extents.z;
    bounds->max.x = center.x + extents.x;
    bounds->max.y = center.y + extents.y;
    bounds->max.z = center.z + extents.z;
    bounds->valid = 1;
}

static void scene_aabb_include( wp_scene_aabb *bounds, const wp_scene_aabb *other )
{
    if( !other || !other->valid )
    {
        return;
    }
    if( !bounds->valid )
    {
        *bounds = *other;
        return;
    }

    bounds->min.x = fminf( bounds->min.x, other->min.x );
    bounds->min.y = fminf( bounds->min.y, other->min.y );
    bounds->min.z = fminf( bounds->min.z, other->min.z );
    bounds->max.x = fmaxf( bounds->max.x, other->max.x );
    bounds->max.y = fmaxf( bounds->max.y, other->max.y );
    bounds->max.z = fmaxf( bounds->max.z, other->max.z );
}

static wp_s32 shape_world_aabb( const wp_rigidbody *body, const wp_collision_shape *shape,
                                wp_scene_aabb *bounds )
{
    const wp_prepared_shape *p;
    memset( bounds, 0, sizeof( *bounds ) );
    if ( !body || !shape || !wp_collision_shape_is_enabled( shape ) )
        return 0;
    if ( wp_collision_shape_get_type( shape ) == WORKPHONE_COLLISION_SHAPE_PLANE )
    {
        bounds->min = ( wp_vec3f ){ -FLT_MAX, -FLT_MAX, -FLT_MAX };
        bounds->max = ( wp_vec3f ){ FLT_MAX, FLT_MAX, FLT_MAX };
        bounds->valid = 1;
        return 1;
    }
    p = wp_shape_prepare( body, shape, NULL );
    bounds->min = p->minimum;
    bounds->max = p->maximum;
    bounds->valid = p->bounds_valid;
    return bounds->valid;
}

static wp_s32 body_world_aabb( const wp_rigidbody *body, wp_scene_aabb *bounds,
                                wp_s32 *borrowed_vertices )
{
    wp_s32 shape_index;
    if( !body || !bounds )
    {
        return 0;
    }

    memset( bounds, 0, sizeof( *bounds ) );
    *borrowed_vertices = 0;
    for( shape_index = 0; shape_index < wp_rigidbody_get_shape_count( body ); ++shape_index )
    {
        wp_scene_aabb shape_bounds;
        const wp_collision_shape *shape = wp_rigidbody_get_shape( body, shape_index );
        if( wp_collision_shape_is_enabled( shape ) &&
            wp_collision_shape_get_type( shape ) == WORKPHONE_COLLISION_SHAPE_MESH &&
            !wp_collision_shape_get_triangle_mesh( shape ) )
        {
            *borrowed_vertices = 1;
        }
        if( shape_world_aabb( body, shape, &shape_bounds ) )
        {
            scene_aabb_include( bounds, &shape_bounds );
        }
    }
    return bounds->valid;
}

static wp_vec3f plane_world_normal( const wp_collision_shape *shape )
{
    /*
     * Plane normals use the engine's world-space convention. The owning
     * actor is often rotated to orient its rendered mesh, which must not
     * rotate the infinite collision plane.
     */
    return vec3f_normalize( quatf_rotate( wp_collision_shape_get_local_orientation( shape ),
                                          wp_collision_shape_get_plane_normal( shape ) ) );
}

static wp_s32 filter_passes( wp_rigidbody *body, wp_collision_shape *shape, wp_u32 collision_type,
                             wp_u32 collision_mask )
{
    wp_u32 body_type;
    wp_u32 body_mask;
    wp_u32 shape_type;
    wp_u32 shape_mask;

    if( !body || !shape || !wp_rigidbody_has_flag( body, WORKPHONE_RIGIDBODY_FLAG_ENABLED ) ||
        !wp_collision_shape_is_enabled( shape ) )
    {
        return 0;
    }

    if( collision_type == 0u && collision_mask == 0u )
    {
        return 1;
    }

    body_type = wp_rigidbody_get_collision_type( body );
    body_mask = wp_rigidbody_get_collision_mask( body );
    shape_type = wp_collision_shape_get_collision_type( shape );
    shape_mask = wp_collision_shape_get_collision_mask( shape );

    return ( collision_mask == 0u || ( body_type & collision_mask ) != 0 ||
             ( shape_type & collision_mask ) != 0 ) &&
           ( collision_type == 0u || ( collision_type & body_mask ) != 0 ||
             ( collision_type & shape_mask ) != 0 );
}

static wp_s32 ray_sphere( wp_vec3f start, wp_vec3f dir, wp_f32 max_dist, wp_vec3f center, wp_f32 radius,
                          wp_f32 *out_t, wp_vec3f *out_normal )
{
    wp_vec3f m = vec3f_sub( start, center );
    wp_f32 b = vec3f_dot( m, dir );
    wp_f32 c = vec3f_dot( m, m ) - radius * radius;
    wp_f32 discr;
    wp_f32 t;

    if( c > 0.0f && b > 0.0f )
    {
        return 0;
    }

    discr = b * b - c;
    if( discr < 0.0f )
    {
        return 0;
    }

    t = -b - sqrtf( discr );
    if( t < 0.0f )
    {
        t = 0.0f;
    }
    if( t > max_dist )
    {
        return 0;
    }

    *out_t = t;
    *out_normal = vec3f_normalize( vec3f_sub( vec3f_add( start, vec3f_scale( dir, t ) ), center ) );
    return 1;
}

static wp_s32 ray_plane( wp_vec3f start, wp_vec3f dir, wp_f32 max_dist, wp_vec3f normal, wp_f32 offset,
                         wp_f32 *out_t, wp_vec3f *out_normal )
{
    wp_f32 denom = vec3f_dot( normal, dir );
    wp_f32 t;

    if( fabsf( denom ) <= 1.0e-7f )
    {
        return 0;
    }

    t = ( offset - vec3f_dot( normal, start ) ) / denom;
    if( t < 0.0f || t > max_dist )
    {
        return 0;
    }

    *out_t = t;
    *out_normal = normal;
    return 1;
}

static wp_s32 ray_aabb( wp_vec3f start, wp_vec3f dir, wp_f32 max_dist, wp_vec3f min, wp_vec3f max,
                        wp_f32 *out_t, wp_vec3f *out_normal )
{
    wp_f32 tmin = 0.0f;
    wp_f32 tmax = max_dist;
    wp_vec3f normal = vec3f_zero();

#define WP_RAY_AABB_AXIS( axis ) \
    if( fabsf( dir.axis ) < 1.0e-7f ) \
    { \
        if( start.axis < min.axis || start.axis > max.axis ) \
            return 0; \
    } \
    else \
    { \
        wp_f32 inv = 1.0f / dir.axis; \
        wp_f32 t1 = ( min.axis - start.axis ) * inv; \
        wp_f32 t2 = ( max.axis - start.axis ) * inv; \
        wp_f32 sign = -1.0f; \
        if( t1 > t2 ) \
        { \
            wp_f32 tmp = t1; \
            t1 = t2; \
            t2 = tmp; \
            sign = 1.0f; \
        } \
        if( t1 > tmin ) \
        { \
            tmin = t1; \
            normal = vec3f_zero(); \
            normal.axis = sign; \
        } \
        if( t2 < tmax ) \
            tmax = t2; \
        if( tmin > tmax ) \
            return 0; \
    }

    WP_RAY_AABB_AXIS( x )
    WP_RAY_AABB_AXIS( y )
    WP_RAY_AABB_AXIS( z )

#undef WP_RAY_AABB_AXIS

    *out_t = tmin;
    *out_normal = normal;
    return 1;
}

static wp_s32 ray_triangle( wp_vec3f start, wp_vec3f dir, wp_f32 max_dist, wp_vec3f a, wp_vec3f b,
                            wp_vec3f c, wp_f32 *out_t, wp_vec3f *out_normal )
{
    const wp_f32 epsilon = 1.0e-7f;
    wp_vec3f edge1 = vec3f_sub( b, a );
    wp_vec3f edge2 = vec3f_sub( c, a );
    wp_vec3f p = vec3f_cross( dir, edge2 );
    wp_f32 determinant = vec3f_dot( edge1, p );
    wp_vec3f s;
    wp_f32 inverse;
    wp_f32 u;
    wp_vec3f q;
    wp_f32 v;
    wp_f32 t;

    if( fabsf( determinant ) <= epsilon )
    {
        return 0;
    }

    inverse = 1.0f / determinant;
    s = vec3f_sub( start, a );
    u = vec3f_dot( s, p ) * inverse;
    if( u < 0.0f || u > 1.0f )
    {
        return 0;
    }

    q = vec3f_cross( s, edge1 );
    v = vec3f_dot( dir, q ) * inverse;
    if( v < 0.0f || u + v > 1.0f )
    {
        return 0;
    }

    t = vec3f_dot( edge2, q ) * inverse;
    if( t < 0.0f || t > max_dist )
    {
        return 0;
    }

    *out_t = t;
    *out_normal = vec3f_normalize( vec3f_cross( edge1, edge2 ) );
    return 1;
}

/* Grow opaque scene storage atomically, retaining sleep state and scratch space. */
static wp_s32 reserve_scene_actors( wp_physics_scene *scene, wp_s32 required )
{
    wp_s32 capacity = scene->actor_capacity ? scene->actor_capacity : WP_SCENE_MAX_ACTORS;
    wp_rigidbody **actors;
    wp_f32 *sleep_time;
    wp_s32 *proxies;
    wp_actor_bounds_cache *cache;
    if( required <= scene->actor_capacity ) return 1;
    if( capacity < 8 ) capacity = 8;
    while( capacity < required ) {
        if( capacity > INT_MAX / 2 ) return 0;
        capacity *= 2;
    }
    actors = (wp_rigidbody **)calloc( capacity, sizeof(*actors) );
    sleep_time = (wp_f32 *)calloc( capacity, sizeof(*sleep_time) );
    proxies = (wp_s32 *)malloc( (size_t)capacity * sizeof(*proxies) );
    cache = (wp_actor_bounds_cache *)calloc( capacity, sizeof(*cache) );
    if( !actors || !sleep_time || !proxies || !cache ) {
        free(actors); free(sleep_time); free(proxies); free(cache);
        return 0;
    }
    if( scene->actor_count ) {
        memcpy(actors, scene->actors, scene->actor_count * sizeof(*actors));
        memcpy(sleep_time, scene->sleep_time, scene->actor_count * sizeof(*sleep_time));
        memcpy(proxies, scene->actor_proxies, scene->actor_count * sizeof(*proxies));
        memcpy(cache, scene->bounds_cache, scene->actor_count * sizeof(*cache));
    }
    free(scene->actors); free(scene->sleep_time); free(scene->actor_proxies);
    free(scene->bounds_cache);
    scene->actors = actors;
    scene->sleep_time = sleep_time;
    scene->actor_proxies = proxies;
    scene->bounds_cache = cache;
    scene->actor_capacity = capacity;
    return 1;
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_physics_scene *wp_physics_scene_create( void )
{
    wp_physics_scene *scene = (wp_physics_scene *)malloc( sizeof( wp_physics_scene ) );
    if( !scene )
    {
        return NULL;
    }

    memset( scene, 0, sizeof( wp_physics_scene ) );
    if( !reserve_scene_actors(scene, 1) ) {
        free(scene);
        return NULL;
    }

    /* Default gravity: Earth-like, Y-down */
    scene->gravity.y = -9.81f;

    /* Default scene bounds */
    scene->size.x = 1000.0f;
    scene->size.y = 1000.0f;
    scene->size.z = 1000.0f;

    scene->min_threads = 1;
    scene->max_threads = 1;
    scene->spatial_partitioning = WP_SPATIAL_PARTITION_BVH;
    scene->broadphase = wp_broadphase_create( WORKPHONE_BROADPHASE_DBVT );
    scene->narrowphase = wp_narrowphase_create( WORKPHONE_NARROWPHASE_HYBRID );
    if( !scene->narrowphase || !scene->broadphase )
    {
        wp_broadphase_destroy( scene->broadphase );
        wp_narrowphase_destroy( scene->narrowphase );
        free(scene->actors); free(scene->sleep_time); free(scene->actor_proxies);
        free(scene->bounds_cache);
        free( scene );
        return NULL;
    }

    /* Default contact options */
    scene->contact_options.strategy = WP_CONTACT_STRATEGY_ALWAYS;
    scene->obb_enabled = 1;
    scene->contact_options.separation_threshold = 0.01f;
    scene->contact_options.fixed_update_frequency = 1;
    scene->contact_options.distance_frequency_scale = 1.0f;

    return scene;
}

void wp_physics_scene_destroy( wp_physics_scene *scene )
{
    if( !scene )
    {
        return;
    }

    wp_broadphase_destroy( scene->broadphase );

    wp_narrowphase_destroy( scene->narrowphase );
    scene->narrowphase = NULL;

    wp_collision_cache_destroy( scene->contact_cache );

    free(scene->actors); free(scene->sleep_time); free(scene->actor_proxies);
    free(scene->bounds_cache);
    free( scene );
}

void wp_physics_scene_clear( wp_physics_scene *scene )
{
    if( !scene )
    {
        return;
    }

    memset( scene->actors, 0, scene->actor_capacity * sizeof(*scene->actors) );
    memset( scene->sleep_time, 0, scene->actor_capacity * sizeof(*scene->sleep_time) );
    memset( scene->bounds_cache, 0, scene->actor_capacity * sizeof(*scene->bounds_cache) );
    memset( &scene->broadphase_stats, 0, sizeof(scene->broadphase_stats) );
    wp_broadphase_clear( scene->broadphase );
    wp_collision_cache_clear( scene->contact_cache );
    scene->actor_count = 0;
}

/* =========================================================================
 * Actor management
 * ====================================================================== */

wp_s32 wp_physics_scene_add_actor( wp_physics_scene *scene, wp_rigidbody *body )
{
    wp_s32 i, proxy;

    if( !scene || !body )
    {
        return 0;
    }

    for( i = 0; i < scene->actor_count; ++i )
    {
        if( scene->actors[i] == body )
        {
            return 1;
        }
    }

    if( scene->actor_count == INT_MAX || !reserve_scene_actors(scene, scene->actor_count + 1) )
        return 0;
    /* Allocate the proxy at registration, so simulation never needs to allocate
     * tree storage or silently omit a body under memory pressure. */
    proxy = wp_broadphase_create_proxy( scene->broadphase, body, vec3f_zero(), vec3f_zero() );
    if( proxy == WP_BROADPHASE_INVALID_PROXY ) return 0;
    wp_broadphase_configure_proxy( scene->broadphase, proxy, 0, 0, (wp_u32)scene->actor_count );
    scene->actor_proxies[scene->actor_count] = proxy;
    memset( &scene->bounds_cache[scene->actor_count], 0, sizeof(*scene->bounds_cache) );
    scene->actors[scene->actor_count] = body;
    scene->sleep_time[scene->actor_count] = 0.0f;
    ++scene->actor_count;
    return 1;
}

void wp_physics_scene_remove_actor( wp_physics_scene *scene, wp_rigidbody *body )
{
    wp_s32 i;

    if( !scene || !body )
    {
        return;
    }

    for( i = 0; i < scene->actor_count; ++i )
    {
        if( scene->actors[i] == body )
        {
            wp_s32 last = scene->actor_count - 1;
            wp_s32 cache_index;
            wp_broadphase_destroy_proxy( scene->broadphase, scene->actor_proxies[i] );
            for ( cache_index = wp_collision_cache_get_count( scene->contact_cache ) - 1;
                  cache_index >= 0; --cache_index )
            {
                const wp_collision_cache_entry *entry =
                    &wp_collision_cache_get_entries( scene->contact_cache )[cache_index];
                if ( entry->body_a == body || entry->body_b == body )
                    wp_collision_cache_remove_at( scene->contact_cache, cache_index );
            }
            scene->actors[i] = scene->actors[last];
            scene->sleep_time[i] = scene->sleep_time[last];
            scene->actor_proxies[i] = scene->actor_proxies[last];
            scene->bounds_cache[i] = scene->bounds_cache[last];
            scene->actors[last] = NULL;
            scene->sleep_time[last] = 0.0f;
            scene->actor_proxies[last] = WP_BROADPHASE_INVALID_PROXY;
            memset( &scene->bounds_cache[last], 0, sizeof(*scene->bounds_cache) );
            --scene->actor_count;
            return;
        }
    }
}

wp_rigidbody *wp_physics_scene_get_actor( const wp_physics_scene *scene, wp_s32 index )
{
    if( !scene || index < 0 || index >= scene->actor_count )
    {
        return NULL;
    }

    return scene->actors[index];
}

wp_s32 wp_physics_scene_get_actor_count( const wp_physics_scene *scene )
{
    if( !scene )
    {
        return 0;
    }
    return scene->actor_count;
}

/* =========================================================================
 * Scene bounds
 * ====================================================================== */

wp_vec3f wp_physics_scene_get_size( const wp_physics_scene *scene )
{
    if( !scene )
    {
        return vec3f_zero();
    }
    return scene->size;
}

void wp_physics_scene_set_size( wp_physics_scene *scene, wp_vec3f size )
{
    if( !scene )
    {
        return;
    }
    scene->size = size;
}

/* =========================================================================
 * Gravity
 * ====================================================================== */

wp_vec3f wp_physics_scene_get_gravity( const wp_physics_scene *scene )
{
    if( !scene )
    {
        return vec3f_zero();
    }
    return scene->gravity;
}

void wp_physics_scene_set_gravity( wp_physics_scene *scene, wp_vec3f gravity )
{
    if( !scene )
    {
        return;
    }
    scene->gravity = gravity;
}

/* =========================================================================
 * Simulation
 * ====================================================================== */

static wp_s32 body_is_simulated( const wp_rigidbody *body )
{
    return body && wp_rigidbody_has_flag( body, WORKPHONE_RIGIDBODY_FLAG_ENABLED ) &&
           !wp_rigidbody_is_sleeping( body ) &&
           wp_rigidbody_get_type( body ) != WORKPHONE_RIGIDBODY_STATIC;
}

static wp_f32 body_inverse_mass( const wp_rigidbody *body )
{
    wp_f32 mass;
    if( !body || wp_rigidbody_get_type( body ) != WORKPHONE_RIGIDBODY_DYNAMIC )
    {
        return 0.0f;
    }

    mass = wp_rigidbody_get_mass( body );
    return mass > 1.0e-8f ? 1.0f / mass : 0.0f;
}

static wp_vec3f body_world_center_of_mass( const wp_rigidbody *body )
{
    if( !body )
    {
        return vec3f_zero();
    }
    return vec3f_add( wp_rigidbody_get_position( body ),
                      quatf_rotate( wp_rigidbody_get_orientation( body ),
                                    wp_rigidbody_get_cmass_local_position( body ) ) );
}

static wp_vec3f body_apply_inverse_inertia( const wp_rigidbody *body, wp_vec3f world_value )
{
    wp_vec3f result = vec3f_zero();
    wp_quatf orientation;
    wp_quatf inverse_orientation;
    wp_vec3f local_value;
    wp_vec3f inertia;

    if( !body || wp_rigidbody_get_type( body ) != WORKPHONE_RIGIDBODY_DYNAMIC )
    {
        return result;
    }

    orientation = wp_rigidbody_get_orientation( body );
    inverse_orientation = quatf_conjugate( orientation );
    local_value = quatf_rotate( inverse_orientation, world_value );
    inertia = wp_rigidbody_get_inertia_tensor( body );
    local_value.x = inertia.x > 1.0e-8f ? local_value.x / inertia.x : 0.0f;
    local_value.y = inertia.y > 1.0e-8f ? local_value.y / inertia.y : 0.0f;
    local_value.z = inertia.z > 1.0e-8f ? local_value.z / inertia.z : 0.0f;
    return quatf_rotate( orientation, local_value );
}

static wp_vec3f body_point_velocity( const wp_rigidbody *body, wp_vec3f offset_from_com )
{
    if( !body )
    {
        return vec3f_zero();
    }
    return vec3f_add( wp_rigidbody_get_linear_velocity( body ),
                      vec3f_cross( wp_rigidbody_get_angular_velocity( body ), offset_from_com ) );
}

static wp_f32 contact_effective_mass( const wp_rigidbody *body_a, const wp_rigidbody *body_b,
                                      wp_vec3f offset_a, wp_vec3f offset_b, wp_vec3f direction,
                                      wp_f32 inverse_mass_a, wp_f32 inverse_mass_b )
{
    wp_vec3f angular_a = body_apply_inverse_inertia( body_a, vec3f_cross( offset_a, direction ) );
    wp_vec3f angular_b = body_apply_inverse_inertia( body_b, vec3f_cross( offset_b, direction ) );
    return inverse_mass_a + inverse_mass_b +
           vec3f_dot( direction, vec3f_add( vec3f_cross( angular_a, offset_a ),
                                            vec3f_cross( angular_b, offset_b ) ) );
}

static wp_s32 scene_contains_actor( const wp_physics_scene *scene, const wp_rigidbody *body )
{
    wp_s32 i;
    if( !scene || !body )
    {
        return 0;
    }
    for( i = 0; i < scene->actor_count; ++i )
    {
        if( scene->actors[i] == body )
        {
            return 1;
        }
    }
    return 0;
}

static wp_constraint *find_constraint_between( const wp_rigidbody *body_a, const wp_rigidbody *body_b )
{
    wp_s32 i;
    if( !body_a || !body_b )
    {
        return NULL;
    }
    for( i = 0; i < wp_rigidbody_get_constraint_count( body_a ); ++i )
    {
        wp_constraint *constraint = wp_rigidbody_get_constraint( body_a, i );
        if( constraint && ( ( wp_constraint_get_body_a( constraint ) == body_a &&
                              wp_constraint_get_body_b( constraint ) == body_b ) ||
                            ( wp_constraint_get_body_a( constraint ) == body_b &&
                              wp_constraint_get_body_b( constraint ) == body_a ) ) )
        {
            return constraint;
        }
    }
    return NULL;
}

static wp_s32 collision_filter_values_pass( wp_u32 type_a, wp_u32 mask_a, wp_u32 type_b, wp_u32 mask_b )
{
    /*
     * Keep the default group compatible with the engine's PhysX filter:
     * two objects whose category is zero collide with one another. Explicit
     * groups opt in when either object's mask accepts the other category.
     */
    return ( type_a == 0u && type_b == 0u ) || ( type_a & mask_b ) != 0u || ( type_b & mask_a ) != 0u;
}

static wp_s32 shape_pair_filter_passes( const wp_rigidbody *body_a, const wp_collision_shape *shape_a,
                                        const wp_rigidbody *body_b, const wp_collision_shape *shape_b )
{
    if( !body_a || !body_b || !shape_a || !shape_b ||
        !wp_rigidbody_has_flag( body_a, WORKPHONE_RIGIDBODY_FLAG_ENABLED ) ||
        !wp_rigidbody_has_flag( body_b, WORKPHONE_RIGIDBODY_FLAG_ENABLED ) ||
        !wp_collision_shape_is_enabled( shape_a ) || !wp_collision_shape_is_enabled( shape_b ) )
    {
        return 0;
    }

    {
        wp_constraint *constraint = find_constraint_between( body_a, body_b );
        if( constraint &&
            !wp_constraint_has_flag( constraint, WORKPHONE_CONSTRAINT_FLAG_COLLISION_ENABLED ) )
        {
            return 0;
        }
    }

    return collision_filter_values_pass(
               wp_rigidbody_get_collision_type( body_a ), wp_rigidbody_get_collision_mask( body_a ),
               wp_rigidbody_get_collision_type( body_b ), wp_rigidbody_get_collision_mask( body_b ) ) &&
           collision_filter_values_pass( wp_collision_shape_get_collision_type( shape_a ),
                                         wp_collision_shape_get_collision_mask( shape_a ),
                                         wp_collision_shape_get_collision_type( shape_b ),
                                         wp_collision_shape_get_collision_mask( shape_b ) );
}

static void integrate_body( wp_rigidbody *body, wp_vec3f gravity, wp_f32 dt )
{
    wp_rigidbody_type type;
    wp_vec3f position;
    wp_vec3f velocity;
    wp_vec3f angular_velocity;
    wp_quatf orientation;
    wp_f32 angular_speed;

    if( !body_is_simulated( body ) )
    {
        return;
    }

    type = wp_rigidbody_get_type( body );
    position = wp_rigidbody_get_position( body );
    velocity = wp_rigidbody_get_linear_velocity( body );
    angular_velocity = wp_rigidbody_get_angular_velocity( body );
    orientation = wp_rigidbody_get_orientation( body );

    if( type == WORKPHONE_RIGIDBODY_DYNAMIC )
    {
        wp_f32 inverse_mass = body_inverse_mass( body );
        wp_vec3f force = wp_rigidbody_get_accumulated_force( body );
        wp_vec3f acceleration = wp_rigidbody_get_accumulated_acceleration( body );
        wp_vec3f torque = wp_rigidbody_get_accumulated_torque( body );
        wp_vec3f angular_acceleration = wp_rigidbody_get_accumulated_angular_acceleration( body );
        acceleration = vec3f_add( acceleration, vec3f_scale( force, inverse_mass ) );
        if( wp_rigidbody_has_flag( body, WORKPHONE_RIGIDBODY_FLAG_GRAVITY ) )
        {
            acceleration = vec3f_add( acceleration, wp_rigidbody_has_gravity_override( body )
                                                        ? wp_rigidbody_get_gravity_override( body )
                                                        : gravity );
        }

        /* Torque is world-space; the principal inertia tensor is body-space. */
        angular_acceleration = vec3f_add(angular_acceleration, body_apply_inverse_inertia(body, torque));

        velocity = vec3f_add( velocity, vec3f_scale( acceleration, dt ) );
        angular_velocity = vec3f_add( angular_velocity, vec3f_scale( angular_acceleration, dt ) );
    }

    velocity = vec3f_scale( velocity, 1.0f / ( 1.0f + wp_rigidbody_get_linear_damping( body ) * dt ) );
    angular_velocity =
        vec3f_scale( angular_velocity, 1.0f / ( 1.0f + wp_rigidbody_get_angular_damping( body ) * dt ) );

    angular_speed = sqrtf( vec3f_dot( angular_velocity, angular_velocity ) );
    if( angular_speed > wp_rigidbody_get_max_angular_velocity( body ) &&
        wp_rigidbody_get_max_angular_velocity( body ) > 0.0f )
    {
        angular_velocity = vec3f_scale( angular_velocity,
                                        wp_rigidbody_get_max_angular_velocity( body ) / angular_speed );
    }

    position = vec3f_add( position, vec3f_scale( velocity, dt ) );
    if( angular_speed > 1.0e-8f )
    {
        wp_quatf omega;
        wp_quatf derivative;
        omega.x = angular_velocity.x;
        omega.y = angular_velocity.y;
        omega.z = angular_velocity.z;
        omega.w = 0.0f;
        derivative = quatf_multiply( omega, orientation );
        orientation.x += derivative.x * ( 0.5f * dt );
        orientation.y += derivative.y * ( 0.5f * dt );
        orientation.z += derivative.z * ( 0.5f * dt );
        orientation.w += derivative.w * ( 0.5f * dt );
        orientation = quatf_normalize( orientation );
    }

    wp_rigidbody_set_position( body, position );
    wp_rigidbody_set_orientation( body, orientation );
    wp_rigidbody_set_linear_velocity( body, velocity );
    wp_rigidbody_set_angular_velocity( body, angular_velocity );
}

static wp_vec3f constraint_world_position( const wp_rigidbody *body, const wp_constraint *constraint,
                                           wp_s32 actor )
{
    wp_vec3f local_position = wp_constraint_get_local_position( constraint, actor );
    if( !body )
    {
        return local_position;
    }
    return vec3f_add( wp_rigidbody_get_position( body ),
                      quatf_rotate( wp_rigidbody_get_orientation( body ), local_position ) );
}

static wp_quatf constraint_world_orientation( const wp_rigidbody *body, const wp_constraint *constraint,
                                              wp_s32 actor )
{
    wp_quatf local_orientation = wp_constraint_get_local_orientation( constraint, actor );
    if( !body )
    {
        return quatf_normalize( local_orientation );
    }
    return quatf_normalize( quatf_multiply( wp_rigidbody_get_orientation( body ), local_orientation ) );
}

static wp_f32 vec3f_length( wp_vec3f value )
{
    return sqrtf( vec3f_dot( value, value ) );
}

static void apply_constraint_linear_velocity( wp_rigidbody *body_a, wp_rigidbody *body_b,
                                              wp_vec3f delta_velocity, wp_f32 inverse_mass_a,
                                              wp_f32 inverse_mass_b )
{
    wp_f32 inverse_mass_sum = inverse_mass_a + inverse_mass_b;
    if( inverse_mass_sum <= 1.0e-8f )
    {
        return;
    }

    if( inverse_mass_a > 0.0f )
    {
        wp_rigidbody_set_linear_velocity(
            body_a, vec3f_add( wp_rigidbody_get_linear_velocity( body_a ),
                               vec3f_scale( delta_velocity, inverse_mass_a / inverse_mass_sum ) ) );
        wp_rigidbody_wake_up( body_a );
    }
    if( inverse_mass_b > 0.0f )
    {
        wp_rigidbody_set_linear_velocity(
            body_b, vec3f_sub( wp_rigidbody_get_linear_velocity( body_b ),
                               vec3f_scale( delta_velocity, inverse_mass_b / inverse_mass_sum ) ) );
        wp_rigidbody_wake_up( body_b );
    }
}

static void apply_constraint_angular_velocity( wp_rigidbody *body_a, wp_rigidbody *body_b,
                                               wp_vec3f delta_velocity, wp_f32 inverse_mass_a,
                                               wp_f32 inverse_mass_b )
{
    wp_f32 inverse_mass_sum = inverse_mass_a + inverse_mass_b;
    if( inverse_mass_sum <= 1.0e-8f )
    {
        return;
    }

    if( inverse_mass_a > 0.0f )
    {
        wp_rigidbody_set_angular_velocity(
            body_a, vec3f_add( wp_rigidbody_get_angular_velocity( body_a ),
                               vec3f_scale( delta_velocity, inverse_mass_a / inverse_mass_sum ) ) );
        wp_rigidbody_wake_up( body_a );
    }
    if( inverse_mass_b > 0.0f )
    {
        wp_rigidbody_set_angular_velocity(
            body_b, vec3f_sub( wp_rigidbody_get_angular_velocity( body_b ),
                               vec3f_scale( delta_velocity, inverse_mass_b / inverse_mass_sum ) ) );
        wp_rigidbody_wake_up( body_b );
    }
}

static void solve_constraint_orientation( wp_constraint *constraint, wp_rigidbody *body_a,
                                          wp_rigidbody *body_b, wp_f32 inverse_mass_a,
                                          wp_f32 inverse_mass_b, wp_f32 dt )
{
    wp_f32 inverse_mass_sum = inverse_mass_a + inverse_mass_b;
    wp_quatf frame_a;
    wp_quatf frame_b;
    wp_quatf relative;
    wp_f32 angular_error;

    if( inverse_mass_sum <= 1.0e-8f )
    {
        return;
    }

    frame_a = constraint_world_orientation( body_a, constraint, 0 );
    frame_b = constraint_world_orientation( body_b, constraint, 1 );
    relative = quatf_normalize( quatf_multiply( quatf_conjugate( frame_a ), frame_b ) );
    if( relative.w < 0.0f )
    {
        relative.x = -relative.x;
        relative.y = -relative.y;
        relative.z = -relative.z;
        relative.w = -relative.w;
    }
    angular_error =
        2.0f *
        atan2f( sqrtf( relative.x * relative.x + relative.y * relative.y + relative.z * relative.z ),
                clampf_scene( relative.w, -1.0f, 1.0f ) );

    if( dt > 1.0e-8f &&
        angular_error / ( inverse_mass_sum * dt * dt ) > wp_constraint_get_break_torque( constraint ) )
    {
        wp_constraint_set_flag( constraint, WORKPHONE_CONSTRAINT_FLAG_BROKEN, 1 );
        return;
    }

    if( body_a && inverse_mass_a > 0.0f )
    {
        wp_quatf desired_a = quatf_multiply(
            frame_b, quatf_conjugate( wp_constraint_get_local_orientation( constraint, 0 ) ) );
        wp_rigidbody_set_orientation( body_a,
                                      quatf_nlerp( wp_rigidbody_get_orientation( body_a ), desired_a,
                                                   inverse_mass_a / inverse_mass_sum ) );
        wp_rigidbody_wake_up( body_a );
    }
    if( body_b && inverse_mass_b > 0.0f )
    {
        wp_quatf desired_b = quatf_multiply(
            frame_a, quatf_conjugate( wp_constraint_get_local_orientation( constraint, 1 ) ) );
        wp_rigidbody_set_orientation( body_b,
                                      quatf_nlerp( wp_rigidbody_get_orientation( body_b ), desired_b,
                                                   inverse_mass_b / inverse_mass_sum ) );
        wp_rigidbody_wake_up( body_b );
    }

    {
        wp_vec3f angular_velocity_a =
            body_a ? wp_rigidbody_get_angular_velocity( body_a ) : vec3f_zero();
        wp_vec3f angular_velocity_b =
            body_b ? wp_rigidbody_get_angular_velocity( body_b ) : vec3f_zero();
        apply_constraint_angular_velocity( body_a, body_b,
                                           vec3f_sub( angular_velocity_b, angular_velocity_a ),
                                           inverse_mass_a, inverse_mass_b );
    }
}

static void solve_constraint_drive( wp_constraint *constraint, wp_rigidbody *body_a,
                                    wp_rigidbody *body_b, wp_quatf frame_a,
                                    wp_vec3f relative_position_local, wp_f32 inverse_mass_a,
                                    wp_f32 inverse_mass_b, wp_f32 dt )
{
    wp_s32 axis;
    wp_vec3f target = wp_constraint_get_drive_position( constraint );
    wp_vec3f velocity_a = body_a ? wp_rigidbody_get_linear_velocity( body_a ) : vec3f_zero();
    wp_vec3f velocity_b = body_b ? wp_rigidbody_get_linear_velocity( body_b ) : vec3f_zero();
    wp_vec3f relative_velocity_local =
        quatf_rotate( quatf_conjugate( frame_a ), vec3f_sub( velocity_b, velocity_a ) );
    wp_vec3f drive_delta_local = vec3f_zero();

    for( axis = 0; axis < 3; ++axis )
    {
        wp_constraint_drive_desc drive = wp_constraint_get_drive( constraint, (wp_d6_drive)axis );
        wp_f32 *current = axis == 0
                              ? &relative_position_local.x
                              : ( axis == 1 ? &relative_position_local.y : &relative_position_local.z );
        wp_f32 *desired = axis == 0 ? &target.x : ( axis == 1 ? &target.y : &target.z );
        wp_f32 *relative_velocity =
            axis == 0 ? &relative_velocity_local.x
                      : ( axis == 1 ? &relative_velocity_local.y : &relative_velocity_local.z );
        wp_f32 acceleration =
            drive.stiffness * ( *desired - *current ) - drive.damping * *relative_velocity;
        wp_f32 max_delta = drive.force_limit * dt;
        wp_f32 velocity_delta;

        if( drive.stiffness <= 0.0f && drive.damping <= 0.0f )
        {
            continue;
        }
        if( !drive.is_acceleration )
        {
            acceleration *= inverse_mass_a + inverse_mass_b;
        }
        velocity_delta = acceleration * dt;
        if( isfinite( max_delta ) )
        {
            velocity_delta = clampf_scene( velocity_delta, -max_delta, max_delta );
        }
        if( axis == 0 )
        {
            drive_delta_local.x = -velocity_delta;
        }
        else if( axis == 1 )
        {
            drive_delta_local.y = -velocity_delta;
        }
        else
        {
            drive_delta_local.z = -velocity_delta;
        }
    }

    apply_constraint_linear_velocity( body_a, body_b, quatf_rotate( frame_a, drive_delta_local ),
                                      inverse_mass_a, inverse_mass_b );
}

static void solve_constraint( wp_physics_scene *scene, wp_constraint *constraint, wp_f32 dt )
{
    wp_rigidbody *body_a;
    wp_rigidbody *body_b;
    wp_f32 inverse_mass_a;
    wp_f32 inverse_mass_b;
    wp_f32 inverse_mass_sum;
    wp_vec3f anchor_a;
    wp_vec3f anchor_b;
    wp_quatf frame_a;
    wp_vec3f relative_local;
    wp_vec3f correction_local;
    wp_vec3f correction_world;
    wp_vec3f relative_velocity_local;
    wp_vec3f constrained_velocity_local;
    wp_s32 axis;
    wp_s32 angular_locked = 1;

    if( !constraint || wp_constraint_has_flag( constraint, WORKPHONE_CONSTRAINT_FLAG_BROKEN ) )
    {
        return;
    }

    body_a = wp_constraint_get_body_a( constraint );
    body_b = wp_constraint_get_body_b( constraint );
    if( ( body_a && !scene_contains_actor( scene, body_a ) ) ||
        ( body_b && !scene_contains_actor( scene, body_b ) ) || ( !body_a && !body_b ) )
    {
        return;
    }

    inverse_mass_a = body_inverse_mass( body_a );
    inverse_mass_b = body_inverse_mass( body_b );
    inverse_mass_sum = inverse_mass_a + inverse_mass_b;
    if( inverse_mass_sum <= 1.0e-8f )
    {
        return;
    }

    anchor_a = constraint_world_position( body_a, constraint, 0 );
    anchor_b = constraint_world_position( body_b, constraint, 1 );
    frame_a = constraint_world_orientation( body_a, constraint, 0 );
    relative_local = quatf_rotate( quatf_conjugate( frame_a ), vec3f_sub( anchor_b, anchor_a ) );
    correction_local = vec3f_zero();

    if( wp_constraint_get_type( constraint ) == WORKPHONE_CONSTRAINT_FIXED )
    {
        correction_local = relative_local;
    }
    else
    {
        wp_constraint_linear_limit limit = wp_constraint_get_linear_limit( constraint );
        for( axis = 0; axis < 3; ++axis )
        {
            wp_d6_motion motion = wp_constraint_get_motion( constraint, (wp_d6_axis)axis );
            wp_f32 value =
                axis == 0 ? relative_local.x : ( axis == 1 ? relative_local.y : relative_local.z );
            wp_f32 correction = 0.0f;
            if( motion == WORKPHONE_D6_MOTION_LOCKED )
            {
                correction = value;
            }
            else if( motion == WORKPHONE_D6_MOTION_LIMITED )
            {
                if( value > limit.value )
                {
                    correction = value - limit.value;
                }
                else if( value < -limit.value )
                {
                    correction = value + limit.value;
                }
            }

            if( axis == 0 )
            {
                correction_local.x = correction;
            }
            else if( axis == 1 )
            {
                correction_local.y = correction;
            }
            else
            {
                correction_local.z = correction;
            }
        }
        angular_locked = wp_constraint_get_motion( constraint, WORKPHONE_D6_AXIS_TWIST ) ==
                             WORKPHONE_D6_MOTION_LOCKED &&
                         wp_constraint_get_motion( constraint, WORKPHONE_D6_AXIS_SWING1 ) ==
                             WORKPHONE_D6_MOTION_LOCKED &&
                         wp_constraint_get_motion( constraint, WORKPHONE_D6_AXIS_SWING2 ) ==
                             WORKPHONE_D6_MOTION_LOCKED;
    }

    correction_world = quatf_rotate( frame_a, correction_local );
    if( dt > 1.0e-8f && vec3f_length( correction_world ) / ( inverse_mass_sum * dt * dt ) >
                            wp_constraint_get_break_force( constraint ) )
    {
        wp_constraint_set_flag( constraint, WORKPHONE_CONSTRAINT_FLAG_BROKEN, 1 );
        return;
    }

    if( inverse_mass_a > 0.0f )
    {
        wp_rigidbody_set_position(
            body_a,
            vec3f_add( wp_rigidbody_get_position( body_a ),
                       vec3f_scale( correction_world, 0.8f * inverse_mass_a / inverse_mass_sum ) ) );
        wp_rigidbody_wake_up( body_a );
    }
    if( inverse_mass_b > 0.0f )
    {
        wp_rigidbody_set_position(
            body_b,
            vec3f_sub( wp_rigidbody_get_position( body_b ),
                       vec3f_scale( correction_world, 0.8f * inverse_mass_b / inverse_mass_sum ) ) );
        wp_rigidbody_wake_up( body_b );
    }

    relative_velocity_local =
        quatf_rotate( quatf_conjugate( frame_a ),
                      vec3f_sub( body_b ? wp_rigidbody_get_linear_velocity( body_b ) : vec3f_zero(),
                                 body_a ? wp_rigidbody_get_linear_velocity( body_a ) : vec3f_zero() ) );
    constrained_velocity_local = vec3f_zero();
    if( wp_constraint_get_type( constraint ) == WORKPHONE_CONSTRAINT_FIXED )
    {
        constrained_velocity_local = relative_velocity_local;
    }
    else
    {
        for( axis = 0; axis < 3; ++axis )
        {
            wp_d6_motion motion = wp_constraint_get_motion( constraint, (wp_d6_axis)axis );
            wp_f32 correction =
                axis == 0 ? correction_local.x : ( axis == 1 ? correction_local.y : correction_local.z );
            wp_f32 velocity =
                axis == 0 ? relative_velocity_local.x
                          : ( axis == 1 ? relative_velocity_local.y : relative_velocity_local.z );
            if( motion == WORKPHONE_D6_MOTION_LOCKED ||
                ( motion == WORKPHONE_D6_MOTION_LIMITED && correction != 0.0f ) )
            {
                if( axis == 0 )
                {
                    constrained_velocity_local.x = velocity;
                }
                else if( axis == 1 )
                {
                    constrained_velocity_local.y = velocity;
                }
                else
                {
                    constrained_velocity_local.z = velocity;
                }
            }
        }
    }
    apply_constraint_linear_velocity( body_a, body_b,
                                      quatf_rotate( frame_a, constrained_velocity_local ),
                                      inverse_mass_a, inverse_mass_b );

    if( wp_constraint_get_type( constraint ) == WORKPHONE_CONSTRAINT_D6 )
    {
        solve_constraint_drive( constraint, body_a, body_b, frame_a, relative_local, inverse_mass_a,
                                inverse_mass_b, dt );
    }
    if( wp_constraint_get_type( constraint ) == WORKPHONE_CONSTRAINT_FIXED || angular_locked )
    {
        solve_constraint_orientation( constraint, body_a, body_b, inverse_mass_a, inverse_mass_b, dt );
    }
}

static void solve_scene_constraints( wp_physics_scene *scene, wp_f32 dt )
{
    wp_s32 actor_index;
    for( actor_index = 0; actor_index < scene->actor_count; ++actor_index )
    {
        wp_rigidbody *body = scene->actors[actor_index];
        wp_s32 constraint_index;
        if( !body )
        {
            continue;
        }
        for( constraint_index = 0; constraint_index < wp_rigidbody_get_constraint_count( body );
             ++constraint_index )
        {
            wp_constraint *constraint = wp_rigidbody_get_constraint( body, constraint_index );
            /*
             * Solve each joint once. Actor A owns the canonical pass; a
             * world-to-actor joint (A == NULL) is solved through actor B.
             */
            if( constraint && ( wp_constraint_get_body_a( constraint ) == body ||
                                ( !wp_constraint_get_body_a( constraint ) &&
                                  wp_constraint_get_body_b( constraint ) == body ) ) )
            {
                solve_constraint( scene, constraint, dt );
            }
        }
    }
}

static wp_vec3f contact_normal_a_to_b( wp_rigidbody *body_a, wp_collision_shape *shape_a,
                                       wp_rigidbody *body_b, wp_collision_shape *shape_b,
                                       const wp_contact_point *contact )
{
    wp_vec3f normal;

    if( contact )
    {
        normal = contact->normal_world_on_b;
        if( vec3f_dot( normal, normal ) > 1.0e-8f )
        {
            return vec3f_normalize( normal );
        }
    }

    normal =
        vec3f_sub( shape_world_position( body_b, shape_b ), shape_world_position( body_a, shape_a ) );
    if( vec3f_dot( normal, normal ) <= 1.0e-8f )
    {
        normal.x = 1.0f;
    }
    return vec3f_normalize( normal );
}

static void resolve_contact( wp_rigidbody *body_a, wp_collision_shape *shape_a, wp_rigidbody *body_b,
                             wp_collision_shape *shape_b, const wp_contact_point *contact,
                             wp_f32 position_correction_scale )
{
    wp_f32 inverse_mass_a = body_inverse_mass( body_a );
    wp_f32 inverse_mass_b = body_inverse_mass( body_b );
    wp_f32 inverse_mass_sum = inverse_mass_a + inverse_mass_b;
    wp_vec3f normal;
    wp_vec3f velocity_a;
    wp_vec3f velocity_b;
    wp_vec3f angular_velocity_a;
    wp_vec3f angular_velocity_b;
    wp_vec3f offset_a;
    wp_vec3f offset_b;
    wp_vec3f relative_velocity;
    wp_f32 velocity_along_normal;
    wp_f32 effective_mass;
    wp_f32 impulse_magnitude;
    wp_vec3f impulse;

    if( !contact || inverse_mass_sum <= 1.0e-8f || wp_collision_shape_is_trigger( shape_a ) ||
        wp_collision_shape_is_trigger( shape_b ) )
    {
        return;
    }

    normal = contact_normal_a_to_b( body_a, shape_a, body_b, shape_b, contact );

    if( contact->penetration_depth > 0.001f )
    {
        wp_f32 correction_magnitude = ( contact->penetration_depth - 0.001f ) * 0.8f *
                                      position_correction_scale / inverse_mass_sum;
        wp_vec3f correction = vec3f_scale( normal, correction_magnitude );
        if( inverse_mass_a > 0.0f )
        {
            wp_rigidbody_set_position( body_a, vec3f_sub( wp_rigidbody_get_position( body_a ),
                                                          vec3f_scale( correction, inverse_mass_a ) ) );
        }
        if( inverse_mass_b > 0.0f )
        {
            wp_rigidbody_set_position( body_b, vec3f_add( wp_rigidbody_get_position( body_b ),
                                                          vec3f_scale( correction, inverse_mass_b ) ) );
        }
    }

    offset_a = vec3f_sub( contact->position_world_on_a, body_world_center_of_mass( body_a ) );
    offset_b = vec3f_sub( contact->position_world_on_b, body_world_center_of_mass( body_b ) );
    velocity_a = wp_rigidbody_get_linear_velocity( body_a );
    velocity_b = wp_rigidbody_get_linear_velocity( body_b );
    angular_velocity_a = wp_rigidbody_get_angular_velocity( body_a );
    angular_velocity_b = wp_rigidbody_get_angular_velocity( body_b );
    relative_velocity =
        vec3f_sub( body_point_velocity( body_b, offset_b ), body_point_velocity( body_a, offset_a ) );
    velocity_along_normal = vec3f_dot( relative_velocity, normal );
    if( velocity_along_normal >= 0.0f )
    {
        return;
    }

    effective_mass = contact_effective_mass( body_a, body_b, offset_a, offset_b, normal, inverse_mass_a,
                                             inverse_mass_b );
    if( effective_mass <= 1.0e-8f )
    {
        return;
    }
    impulse_magnitude = -( 1.0f + clampf_scene( contact->combined_restitution, 0.0f, 1.0f ) ) *
                        velocity_along_normal / effective_mass;
    impulse = vec3f_scale( normal, impulse_magnitude );
    velocity_a = vec3f_sub( velocity_a, vec3f_scale( impulse, inverse_mass_a ) );
    velocity_b = vec3f_add( velocity_b, vec3f_scale( impulse, inverse_mass_b ) );
    angular_velocity_a = vec3f_sub(
        angular_velocity_a, body_apply_inverse_inertia( body_a, vec3f_cross( offset_a, impulse ) ) );
    angular_velocity_b = vec3f_add(
        angular_velocity_b, body_apply_inverse_inertia( body_b, vec3f_cross( offset_b, impulse ) ) );

    relative_velocity =
        vec3f_sub( vec3f_add( velocity_b, vec3f_cross( angular_velocity_b, offset_b ) ),
                   vec3f_add( velocity_a, vec3f_cross( angular_velocity_a, offset_a ) ) );
    {
        wp_vec3f tangent = vec3f_sub( relative_velocity,
                                      vec3f_scale( normal, vec3f_dot( relative_velocity, normal ) ) );
        wp_f32 tangent_length_sq = vec3f_dot( tangent, tangent );
        if( tangent_length_sq > 1.0e-8f )
        {
            wp_f32 tangent_impulse;
            wp_f32 max_friction_impulse;
            wp_f32 tangent_effective_mass;
            tangent = vec3f_scale( tangent, 1.0f / sqrtf( tangent_length_sq ) );
            tangent_effective_mass = contact_effective_mass( body_a, body_b, offset_a, offset_b, tangent,
                                                             inverse_mass_a, inverse_mass_b );
            if( tangent_effective_mass <= 1.0e-8f )
            {
                tangent_effective_mass = inverse_mass_sum;
            }
            tangent_impulse = -vec3f_dot( relative_velocity, tangent ) / tangent_effective_mass;
            max_friction_impulse =
                impulse_magnitude * clampf_scene( contact->combined_friction, 0.0f, 10.0f );
            tangent_impulse =
                clampf_scene( tangent_impulse, -max_friction_impulse, max_friction_impulse );
            impulse = vec3f_scale( tangent, tangent_impulse );
            velocity_a = vec3f_sub( velocity_a, vec3f_scale( impulse, inverse_mass_a ) );
            velocity_b = vec3f_add( velocity_b, vec3f_scale( impulse, inverse_mass_b ) );
            angular_velocity_a =
                vec3f_sub( angular_velocity_a,
                           body_apply_inverse_inertia( body_a, vec3f_cross( offset_a, impulse ) ) );
            angular_velocity_b =
                vec3f_add( angular_velocity_b,
                           body_apply_inverse_inertia( body_b, vec3f_cross( offset_b, impulse ) ) );
        }
    }

    if( inverse_mass_a > 0.0f )
    {
        wp_rigidbody_set_linear_velocity( body_a, velocity_a );
        wp_rigidbody_set_angular_velocity( body_a, angular_velocity_a );
    }
    if( inverse_mass_b > 0.0f )
    {
        wp_rigidbody_set_linear_velocity( body_b, velocity_b );
        wp_rigidbody_set_angular_velocity( body_b, angular_velocity_b );
    }
}

/* =========================================================================
 * Contact cache and manifold management
 * ====================================================================== */

static wp_s32 can_reuse_manifold( wp_physics_scene *scene, const wp_contact_cache_record *r,
                                  wp_rigidbody *a, wp_collision_shape *sa, wp_rigidbody *b,
                                  wp_collision_shape *sb )
{
    wp_s32 reverse = r->manifold.body_a != a;
    wp_s32 ai = reverse ? 1 : 0, bi = 1 - ai;
    uint64_t frequency = scene->contact_options.fixed_update_frequency;
    /* Borrowed uncooked arrays can change without a revision notification. */
    if ( ( wp_collision_shape_get_type( sa ) == WORKPHONE_COLLISION_SHAPE_MESH &&
           !wp_collision_shape_get_triangle_mesh( sa ) ) ||
         ( wp_collision_shape_get_type( sb ) == WORKPHONE_COLLISION_SHAPE_MESH &&
           !wp_collision_shape_get_triangle_mesh( sb ) ) )
        return 0;
    if ( r->body_ids[ai] != wp_rigidbody_get_lifetime_id( a ) ||
         r->body_ids[bi] != wp_rigidbody_get_lifetime_id( b ) ||
         r->shape_ids[ai] != wp_collision_shape_get_lifetime_id( sa ) ||
         r->shape_ids[bi] != wp_collision_shape_get_lifetime_id( sb ) ||
         r->revisions[ai] != wp_rigidbody_get_bounds_revision( a ) ||
         r->revisions[bi] != wp_rigidbody_get_bounds_revision( b ) ||
         r->shape_revisions[ai] != wp_collision_shape_get_revision( sa ) ||
         r->shape_revisions[bi] != wp_collision_shape_get_revision( sb ) )
        return 0;
    if ( scene->contact_options.strategy == WP_CONTACT_STRATEGY_DISTANCE )
    {
        wp_vec3f delta =
            vec3f_sub( wp_rigidbody_get_position( a ), wp_rigidbody_get_position( b ) );
        double f = 1.0 + (double)vec3f_dot( delta, delta ) *
                             scene->contact_options.distance_frequency_scale;
        frequency = isfinite( f ) && f > 1 ? (uint64_t)fmin( f, (double)UINT32_MAX ) : 1;
    }
    if ( frequency < 1 )
        frequency = 1;
    return scene->frame_count - r->last_update_frame < frequency;
}
static void swap_manifold( wp_contact_manifold *m )
{
    wp_rigidbody *body = m->body_a;
    wp_collision_shape *shape = m->shape_a;
    m->body_a = m->body_b;
    m->body_b = body;
    m->shape_a = m->shape_b;
    m->shape_b = shape;
    for ( wp_s32 i = 0; i < m->contact_count; ++i )
    {
        wp_vec3f p = m->contacts[i].position_world_on_a;
        m->contacts[i].position_world_on_a = m->contacts[i].position_world_on_b;
        m->contacts[i].position_world_on_b = p;
        m->contacts[i].normal_world_on_b = vec3f_scale( m->contacts[i].normal_world_on_b, -1 );
    }
}
static void store_manifold( wp_physics_scene *scene, wp_s32 index, const wp_contact_manifold *m )
{
    wp_contact_cache_record *r;
    if ( index < 0 )
        index = wp_collision_cache_add( scene->contact_cache, m->body_a, m->shape_a, m->body_b,
                                        m->shape_b );
    if ( index < 0 )
    {
        ++scene->broadphase_stats.cache_failures;
        return;
    }
    r = (wp_contact_cache_record *)wp_collision_cache_payload( scene->contact_cache, index );
    r->manifold = *m;
    r->body_ids[0] = wp_rigidbody_get_lifetime_id( m->body_a );
    r->body_ids[1] = wp_rigidbody_get_lifetime_id( m->body_b );
    r->shape_ids[0] = wp_collision_shape_get_lifetime_id( m->shape_a );
    r->shape_ids[1] = wp_collision_shape_get_lifetime_id( m->shape_b );
    r->revisions[0] = wp_rigidbody_get_bounds_revision( m->body_a );
    r->revisions[1] = wp_rigidbody_get_bounds_revision( m->body_b );
    r->shape_revisions[0] = wp_collision_shape_get_revision( m->shape_a );
    r->shape_revisions[1] = wp_collision_shape_get_revision( m->shape_b );
    r->seen_epoch = scene->contact_epoch;
    r->last_update_frame = scene->frame_count;
}

/* One contact consumer shared by all spatial-partition selections. */
static void solve_broadphase_pair( const wp_broadphase_pair *pair, void *context )
{
    wp_physics_scene *scene = (wp_physics_scene *)context;
    wp_rigidbody *body_a = pair->body_a;
    wp_rigidbody *body_b = pair->body_b;
    wp_s32 sa, sb;
    ++scene->broadphase_stats.candidate_pairs;
    if( scene->obb_enabled )
    {
        wp_s32 local_rebuilt, world_updated;
        const wp_body_obb *a = wp_body_get_obb( body_a, &local_rebuilt, &world_updated );
        const wp_body_obb *b;
        scene->broadphase_stats.obb_local_rebuilds += local_rebuilt;
        scene->broadphase_stats.obb_world_updates += world_updated;
        b = wp_body_get_obb( body_b, &local_rebuilt, &world_updated );
        scene->broadphase_stats.obb_local_rebuilds += local_rebuilt;
        scene->broadphase_stats.obb_world_updates += world_updated;
        /* The solver can move bodies during this visitor. Bounds are refreshed
         * against live revisions here, without modifying the AABB tree snapshot. */
        if( a && b && a->valid && b->valid && ( a->useful || b->useful ) )
        {
            ++scene->broadphase_stats.obb_tests;
            if( !wp_body_obb_may_overlap( a, b,
                    wp_narrowphase_get_contact_tolerance( scene->narrowphase ) ) )
            {
                /* Unvisited cached contacts retire at this substep epoch. */
                ++scene->broadphase_stats.obb_rejections;
                return;
            }
        }
    }
    for( sa = 0; sa < wp_rigidbody_get_shape_count( body_a ); ++sa ) {
        wp_collision_shape *shape_a = wp_rigidbody_get_shape( body_a, sa );
        for( sb = 0; sb < wp_rigidbody_get_shape_count( body_b ); ++sb ) {
            wp_collision_shape *shape_b = wp_rigidbody_get_shape( body_b, sb );
            wp_contact_manifold manifold;
            wp_s32 cache_index = -1;
            wp_contact_cache_record *record = NULL;
            wp_s32 c;
            if( !shape_pair_filter_passes( body_a, shape_a, body_b, shape_b ) ) continue;
            if ( ( wp_rigidbody_get_shape_count( body_a ) > 1 ||
                   wp_rigidbody_get_shape_count( body_b ) > 1 ) &&
                 !wp_prepared_shapes_may_overlap(
                     wp_shape_prepare( body_a, shape_a, NULL ),
                     wp_shape_prepare( body_b, shape_b, NULL ),
                     wp_narrowphase_get_contact_tolerance( scene->narrowphase ) ) )
            {
                ++scene->broadphase_stats.child_pair_rejections;
                continue;
            }
            if ( scene->contact_options.strategy != WP_CONTACT_STRATEGY_ALWAYS &&
                 scene->contact_cache )
            {
                cache_index = wp_collision_cache_find_counted(
                    scene->contact_cache, body_a, shape_a, body_b, shape_b,
                    &scene->broadphase_stats.cache_probes );
                record = (wp_contact_cache_record *)wp_collision_cache_payload(
                    scene->contact_cache, cache_index );
            }
            if ( record && can_reuse_manifold( scene, record, body_a, shape_a, body_b, shape_b ) )
            {
                manifold = record->manifold;
                if ( manifold.body_a != body_a )
                    swap_manifold( &manifold );
                wp_manifold_refresh_materials( &manifold );
                record->seen_epoch = scene->contact_epoch;
                ++scene->broadphase_stats.cache_hits;
            }
            else
            {
                if ( scene->contact_options.strategy != WP_CONTACT_STRATEGY_ALWAYS )
                    ++scene->broadphase_stats.cache_misses;
                ++scene->broadphase_stats.narrowphase_tests;
                if ( !wp_narrowphase_test_pair( scene->narrowphase, body_a, shape_a, body_b,
                                                shape_b, &manifold ) )
                {
                    if ( cache_index >= 0 )
                        wp_collision_cache_remove_at( scene->contact_cache, cache_index );
                    continue;
                }
                if ( scene->contact_options.strategy != WP_CONTACT_STRATEGY_ALWAYS )
                    store_manifold( scene, cache_index, &manifold );
            }
            for( c = 0; c < manifold.contact_count; ++c ) {
                resolve_contact( body_a, shape_a, body_b, shape_b, &manifold.contacts[c],
                                 1.0f / (wp_f32)manifold.contact_count );
            }
        }
    }
}

static void solve_scene_contacts( wp_physics_scene *scene )
{
    wp_s32 i;
    if( !scene ) return;
    ++scene->contact_epoch;
    if ( scene->contact_options.strategy != WP_CONTACT_STRATEGY_ALWAYS && !scene->contact_cache )
    {
        scene->contact_cache =
            wp_collision_cache_create_with_payload( 64, sizeof( wp_contact_cache_record ) );
        if ( !scene->contact_cache )
            ++scene->broadphase_stats.cache_failures;
    }
    /* Body/shape mutations and cooked-mesh refits invalidate revisions even on
     * sleeping/statics. Uncooked borrowed arrays retain per-substep refresh.
     * Capture revisions before solving: positional corrections must invalidate
     * the next substep, not bless the snapshot currently stored in the tree. */
    for( i = 0; i < scene->actor_count; ++i ) {
        wp_rigidbody *body = scene->actors[i];
        wp_actor_bounds_cache *cache = &scene->bounds_cache[i];
        uint64_t revision = wp_rigidbody_get_bounds_revision( body );
        wp_s32 enabled = wp_rigidbody_has_flag( body, WORKPHONE_RIGIDBODY_FLAG_ENABLED );
        if( enabled ) {
            if( cache->revision != revision || cache->borrowed_vertices ) {
                wp_scene_aabb bounds;
                cache->valid = body_world_aabb( body, &bounds, &cache->borrowed_vertices );
                cache->revision = revision;
                ++scene->broadphase_stats.bounds_rebuilds;
                if ( wp_rigidbody_get_type( body ) == WORKPHONE_RIGIDBODY_STATIC )
                    ++scene->broadphase_stats.dirty_static_updates;
                if( cache->valid )
                    wp_broadphase_move_proxy( scene->broadphase, scene->actor_proxies[i], bounds.min, bounds.max );
            } else ++scene->broadphase_stats.bounds_reuses;
            enabled = cache->valid;
        }
        wp_broadphase_configure_proxy( scene->broadphase, scene->actor_proxies[i], enabled,
                                      body_inverse_mass( body ) > 0.0f, (wp_u32)i );
    }
    wp_broadphase_visit_pairs( scene->broadphase, solve_broadphase_pair, scene );
    for ( i = wp_collision_cache_get_count( scene->contact_cache ) - 1; i >= 0; --i )
    {
        wp_contact_cache_record *r =
            (wp_contact_cache_record *)wp_collision_cache_payload( scene->contact_cache, i );
        if ( r->seen_epoch != scene->contact_epoch )
        {
            wp_collision_cache_remove_at( scene->contact_cache, i );
            ++scene->broadphase_stats.cache_retired;
        }
    }
}

static void finish_simulation_step( wp_physics_scene *scene, wp_f32 dt )
{
    wp_s32 i;
    for( i = 0; i < scene->actor_count; ++i )
    {
        wp_rigidbody *body = scene->actors[i];
        if( !body )
        {
            continue;
        }

        if ( wp_rigidbody_get_type( body ) != WORKPHONE_RIGIDBODY_DYNAMIC ||
             wp_rigidbody_is_sleeping( body ) )
        {
            scene->sleep_time[i] = 0;
            wp_rigidbody_clear_force( body );
            wp_rigidbody_clear_torque( body );
            continue;
        }
        {
            wp_vec3f force = wp_rigidbody_get_accumulated_force( body );
            wp_vec3f acceleration = wp_rigidbody_get_accumulated_acceleration( body );
            wp_vec3f torque = wp_rigidbody_get_accumulated_torque( body );
            wp_vec3f angular_acceleration = wp_rigidbody_get_accumulated_angular_acceleration( body );
            wp_f32 input_motion = vec3f_dot( force, force ) + vec3f_dot( acceleration, acceleration ) +
                                  vec3f_dot( torque, torque ) +
                                  vec3f_dot( angular_acceleration, angular_acceleration );

            /*
             * Sleeping requires sustained inactivity. Sleeping a body after a
             * single low-speed step repeatedly erased the small velocity
             * increments used to start heavy vehicles.
             */
            if( wp_rigidbody_get_type( body ) == WORKPHONE_RIGIDBODY_DYNAMIC &&
                !wp_rigidbody_is_sleeping( body ) )
            {
                wp_vec3f velocity = wp_rigidbody_get_linear_velocity( body );
                wp_vec3f angular_velocity = wp_rigidbody_get_angular_velocity( body );
                wp_f32 threshold = wp_rigidbody_get_sleep_threshold( body );
                wp_f32 motion =
                    vec3f_dot( velocity, velocity ) + vec3f_dot( angular_velocity, angular_velocity );
                if( threshold > 0.0f && motion < threshold * threshold && input_motion <= 1.0e-12f )
                {
                    scene->sleep_time[i] += dt;
                    if( scene->sleep_time[i] >= 0.5f )
                    {
                        wp_rigidbody_put_to_sleep( body );
                        scene->sleep_time[i] = 0.0f;
                    }
                }
                else
                {
                    scene->sleep_time[i] = 0.0f;
                }
            }
            else
            {
                scene->sleep_time[i] = 0.0f;
            }
        }

        wp_rigidbody_clear_force( body );
        wp_rigidbody_clear_torque( body );
    }
}

static wp_f32 body_smallest_collision_extent( const wp_rigidbody *body )
{
    wp_f32 smallest = FLT_MAX;
    wp_s32 shape_index;

    if( !body )
    {
        return smallest;
    }

    for( shape_index = 0; shape_index < wp_rigidbody_get_shape_count( body ); ++shape_index )
    {
        const wp_collision_shape *shape = wp_rigidbody_get_shape( body, shape_index );
        wp_f32 extent = FLT_MAX;
        if( !shape || !wp_collision_shape_is_enabled( shape ) )
        {
            continue;
        }

        switch( wp_collision_shape_get_type( shape ) )
        {
        case WORKPHONE_COLLISION_SHAPE_SPHERE:
            extent = wp_collision_shape_get_sphere_radius( shape );
            break;
        case WORKPHONE_COLLISION_SHAPE_BOX:
        {
            const wp_vec3f half = wp_collision_shape_get_box_half_extents( shape );
            extent = fminf( half.x, fminf( half.y, half.z ) );
            /*
             * 2D boxes deliberately have a very large hidden-Z extent. The
             * in-plane dimensions still determine the useful sweep distance.
             */
            if( half.z > 10000.0f )
            {
                extent = fminf( half.x, half.y );
            }
            break;
        }
        case WORKPHONE_COLLISION_SHAPE_CAPSULE:
            extent = wp_collision_shape_get_capsule_radius( shape );
            break;
        default:
            break;
        }

        if( extent > 1.0e-4f && extent < smallest )
        {
            smallest = extent;
        }
    }
    return smallest;
}

static wp_s32 calculate_motion_substeps( const wp_physics_scene *scene, wp_f32 dt )
{
    wp_s32 substeps = (wp_s32)ceilf( dt / ( 1.0f / 60.0f ) );
    wp_s32 actor_index;

    if( substeps < 1 )
    {
        substeps = 1;
    }

    for( actor_index = 0; actor_index < scene->actor_count; ++actor_index )
    {
        const wp_rigidbody *body = scene->actors[actor_index];
        wp_f32 extent;
        wp_f32 speed;
        wp_f32 angular_speed;
        wp_s32 translation_steps;
        wp_s32 rotation_steps;

        if( !body || !wp_rigidbody_has_flag( body, WORKPHONE_RIGIDBODY_FLAG_ENABLED ) ||
            wp_rigidbody_get_type( body ) == WORKPHONE_RIGIDBODY_STATIC )
        {
            continue;
        }

        extent = body_smallest_collision_extent( body );
        speed = sqrtf( vec3f_dot( wp_rigidbody_get_linear_velocity( body ),
                                  wp_rigidbody_get_linear_velocity( body ) ) );
        angular_speed = sqrtf( vec3f_dot( wp_rigidbody_get_angular_velocity( body ),
                                          wp_rigidbody_get_angular_velocity( body ) ) );

        translation_steps =
            extent < FLT_MAX ? (wp_s32)ceilf( speed * dt / fmaxf( extent * 0.5f, 0.01f ) ) : 1;
        rotation_steps = (wp_s32)ceilf( angular_speed * dt / 0.125f );
        if( translation_steps > substeps )
        {
            substeps = translation_steps;
        }
        if( rotation_steps > substeps )
        {
            substeps = rotation_steps;
        }
    }

    if( substeps > 64 )
    {
        substeps = 64;
    }
    return substeps;
}

void wp_physics_scene_simulate( wp_physics_scene *scene, wp_f32 dt )
{
    wp_s32 substeps;
    wp_s32 step;
    wp_f32 substep_dt;

    if( !scene )
    {
        return;
    }

    if( !isfinite( dt ) || dt <= 0.0f )
    {
        return;
    }

    dt = clampf_scene( dt, 0.0f, 0.25f );
    memset( &scene->broadphase_stats, 0, sizeof(scene->broadphase_stats) );
    wp_narrowphase_reset_stats( scene->narrowphase );
    for ( wp_s32 i = 0; i < scene->actor_count; ++i )
    {
        wp_rigidbody_type type = wp_rigidbody_get_type( scene->actors[i] );
        if ( type == WORKPHONE_RIGIDBODY_STATIC )
            ++scene->broadphase_stats.static_actors;
        else if ( type == WORKPHONE_RIGIDBODY_DYNAMIC )
            ++scene->broadphase_stats.dynamic_actors;
        else if ( type == WORKPHONE_RIGIDBODY_KINEMATIC )
            ++scene->broadphase_stats.kinematic_actors;
    }
    substeps = calculate_motion_substeps( scene, dt );
    scene->broadphase_stats.collision_substeps = (wp_u32)substeps;
    substep_dt = dt / (wp_f32)substeps;

    for( step = 0; step < substeps; ++step )
    {
        wp_s32 i;
        for( i = 0; i < scene->actor_count; ++i )
        {
            if ( wp_rigidbody_get_type( scene->actors[i] ) != WORKPHONE_RIGIDBODY_STATIC )
                integrate_body( scene->actors[i], scene->gravity, substep_dt );
        }

        solve_scene_constraints( scene, substep_dt );
        solve_scene_contacts( scene );
        solve_scene_constraints( scene, substep_dt );
    }

    finish_simulation_step( scene, dt );
    scene->frame_count++;
}

wp_scene_broadphase_stats wp_physics_scene_get_broadphase_stats( const wp_physics_scene *scene )
{
    wp_scene_broadphase_stats stats = {0};
    return scene ? scene->broadphase_stats : stats;
}

wp_s32 wp_physics_scene_set_broadphase_simd_enabled( wp_physics_scene *scene, wp_s32 enabled )
{
    return scene ? wp_broadphase_set_simd_enabled( scene->broadphase, enabled ) : 0;
}

void wp_physics_scene_set_broadphase_obb_enabled( wp_physics_scene *scene, wp_s32 enabled )
{
    if( scene ) scene->obb_enabled = enabled != 0;
}

wp_s32 wp_physics_scene_get_broadphase_obb_enabled( const wp_physics_scene *scene )
{
    return scene ? scene->obb_enabled : 0;
}

wp_s32 wp_physics_scene_fetch_results( wp_physics_scene *scene, wp_s32 block )
{
    if( !scene )
    {
        return 0;
    }

    /* The software backend steps synchronously, so results are already available. */
    (void)block;

    return 1;
}

/* =========================================================================
 * Threading
 * ====================================================================== */

wp_u32 wp_physics_scene_get_min_threads( const wp_physics_scene *scene )
{
    if( !scene )
    {
        return 0;
    }

    return scene->min_threads;
}

void wp_physics_scene_set_min_threads( wp_physics_scene *scene, wp_u32 min_threads )
{
    if( !scene )
    {
        return;
    }

    scene->min_threads = min_threads;
}

wp_u32 wp_physics_scene_get_max_threads( const wp_physics_scene *scene )
{
    if( !scene )
    {
        return 0;
    }

    return scene->max_threads;
}

void wp_physics_scene_set_max_threads( wp_physics_scene *scene, wp_u32 max_threads )
{
    if( !scene )
    {
        return;
    }

    scene->max_threads = max_threads;
}

/* =========================================================================
 * Raycasting
 * ====================================================================== */

wp_s32 wp_physics_scene_ray_test( wp_physics_scene *scene, wp_vec3f start, wp_vec3f direction,
                                  wp_vec3f *out_hit_pos, wp_vec3f *out_hit_normal, wp_u32 collision_type,
                                  wp_u32 collision_mask )
{
    wp_vec3f end;

    if( !scene )
    {
        return 0;
    }

    direction = vec3f_normalize( direction );
    if( vec3f_dot( direction, direction ) <= 1.0e-7f )
    {
        return 0;
    }

    end = vec3f_add( start, vec3f_scale( direction, 100000.0f ) );
    return wp_physics_scene_intersects( scene, start, end, out_hit_pos, out_hit_normal, collision_type,
                                        collision_mask );
}

wp_s32 wp_physics_scene_intersects( wp_physics_scene *scene, wp_vec3f start, wp_vec3f end,
                                    wp_vec3f *out_hit_pos, wp_vec3f *out_hit_normal,
                                    wp_u32 collision_type, wp_u32 collision_mask )
{
    return wp_physics_scene_intersects_ex( scene, start, end, out_hit_pos, out_hit_normal, NULL, NULL,
                                           collision_type, collision_mask );
}

wp_s32 wp_physics_scene_intersects_ex( wp_physics_scene *scene, wp_vec3f start, wp_vec3f end,
                                       wp_vec3f *out_hit_pos, wp_vec3f *out_hit_normal,
                                       wp_rigidbody **out_body, wp_collision_shape **out_shape,
                                       wp_u32 collision_type, wp_u32 collision_mask )
{
    return wp_physics_scene_intersects_actor_types_ex( scene, start, end, out_hit_pos, out_hit_normal,
                                                       out_body, out_shape, collision_type,
                                                       collision_mask, WORKPHONE_SCENE_QUERY_ALL );
}

wp_s32 wp_physics_scene_intersects_actor_types_ex( wp_physics_scene *scene, wp_vec3f start, wp_vec3f end,
                                                   wp_vec3f *out_hit_pos, wp_vec3f *out_hit_normal,
                                                   wp_rigidbody **out_body,
                                                   wp_collision_shape **out_shape, wp_u32 collision_type,
                                                   wp_u32 collision_mask, wp_u32 actor_types )
{
    wp_vec3f delta;
    wp_f32 max_dist;
    wp_vec3f dir;
    wp_s32 hit_any = 0;
    wp_f32 best_t = 0.0f;
    wp_vec3f best_pos;
    wp_vec3f best_normal;
    wp_rigidbody *best_body = NULL;
    wp_collision_shape *best_shape = NULL;
    wp_s32 i;

    if( out_body )
    {
        *out_body = NULL;
    }
    if( out_shape )
    {
        *out_shape = NULL;
    }
    if( out_hit_pos )
    {
        *out_hit_pos = vec3f_zero();
    }
    if( out_hit_normal )
    {
        *out_hit_normal = vec3f_zero();
    }

    if( !scene )
    {
        return 0;
    }

    delta = vec3f_sub( end, start );
    max_dist = sqrtf( vec3f_dot( delta, delta ) );
    if( max_dist <= 1.0e-7f )
    {
        return 0;
    }
    dir = vec3f_scale( delta, 1.0f / max_dist );

    for( i = 0; i < scene->actor_count; ++i )
    {
        wp_rigidbody *body = scene->actors[i];
        wp_s32 shape_count;
        wp_s32 s;

        if( !body )
        {
            continue;
        }

        {
            const wp_rigidbody_type body_type = wp_rigidbody_get_type( body );
            const wp_u32 body_query_type =
                body_type == WORKPHONE_RIGIDBODY_STATIC
                    ? WORKPHONE_SCENE_QUERY_STATIC
                    : ( body_type == WORKPHONE_RIGIDBODY_KINEMATIC ? WORKPHONE_SCENE_QUERY_KINEMATIC
                                                                   : WORKPHONE_SCENE_QUERY_DYNAMIC );
            if( ( actor_types & body_query_type ) == 0u )
            {
                continue;
            }
        }

        shape_count = wp_rigidbody_get_shape_count( body );

        for( s = 0; s < shape_count; ++s )
        {
            wp_collision_shape *shape = wp_rigidbody_get_shape( body, s );
            wp_collision_shape_type type;
            wp_vec3f shape_pos;
            wp_quatf shape_orientation;
            wp_f32 t = 0.0f;
            wp_vec3f normal = vec3f_zero();
            wp_s32 hit = 0;

            if( !filter_passes( body, shape, collision_type, collision_mask ) )
            {
                continue;
            }

            type = wp_collision_shape_get_type( shape );
            shape_pos = shape_world_position( body, shape );
            shape_orientation = shape_world_orientation( body, shape );

            if( type == WORKPHONE_COLLISION_SHAPE_SPHERE )
            {
                hit = ray_sphere( start, dir, max_dist, shape_pos,
                                  wp_collision_shape_get_sphere_radius( shape ), &t, &normal );
            }
            else if( type == WORKPHONE_COLLISION_SHAPE_BOX )
            {
                wp_vec3f h = wp_collision_shape_get_box_half_extents( shape );
                wp_vec3f min = vec3f_scale( h, -1.0f );
                wp_vec3f max = h;
                wp_quatf inverse_orientation = quatf_conjugate( shape_orientation );
                wp_vec3f local_start =
                    quatf_rotate( inverse_orientation, vec3f_sub( start, shape_pos ) );
                wp_vec3f local_dir = quatf_rotate( inverse_orientation, dir );
                hit = ray_aabb( local_start, local_dir, max_dist, min, max, &t, &normal );
                if( hit )
                {
                    normal = quatf_rotate( shape_orientation, normal );
                }
            }
            else if( type == WORKPHONE_COLLISION_SHAPE_PLANE )
            {
                wp_vec3f plane_normal = plane_world_normal( shape );
                wp_f32 plane_offset =
                    vec3f_dot( plane_normal, shape_pos ) - wp_collision_shape_get_plane_offset( shape );
                hit = ray_plane( start, dir, max_dist, plane_normal, plane_offset, &t, &normal );
            }
            else if( type == WORKPHONE_COLLISION_SHAPE_CAPSULE )
            {
                wp_vec3f capsule_axis;
                wp_vec3f top;
                wp_vec3f bottom;
                wp_f32 t_top;
                wp_f32 t_bottom;
                wp_vec3f n_top;
                wp_vec3f n_bottom;
                wp_f32 hh = wp_collision_shape_get_capsule_half_height( shape );
                wp_f32 r = wp_collision_shape_get_capsule_radius( shape );
                capsule_axis.x = 0.0f;
                capsule_axis.y = hh;
                capsule_axis.z = 0.0f;
                capsule_axis = quatf_rotate( shape_orientation, capsule_axis );
                top = vec3f_add( shape_pos, capsule_axis );
                bottom = vec3f_sub( shape_pos, capsule_axis );
                hit = ray_sphere( start, dir, max_dist, top, r, &t_top, &n_top );
                if( ray_sphere( start, dir, max_dist, bottom, r, &t_bottom, &n_bottom ) &&
                    ( !hit || t_bottom < t_top ) )
                {
                    t = t_bottom;
                    normal = n_bottom;
                    hit = 1;
                }
                else if( hit )
                {
                    t = t_top;
                    normal = n_top;
                }
            }
            else if( type == WORKPHONE_COLLISION_SHAPE_MESH )
            {
                const wp_collision_mesh_data *mesh = wp_collision_shape_get_mesh_data( shape );
                if( mesh && mesh->vertices && mesh->indices )
                {
                    const wp_triangle_mesh *triangle_mesh =
                        wp_collision_shape_get_triangle_mesh( shape );
                    const wp_quatf inverse_orientation = quatf_conjugate( shape_orientation );
                    const wp_vec3f local_start =
                        quatf_rotate( inverse_orientation, vec3f_sub( start, shape_pos ) );
                    const wp_vec3f local_dir = quatf_rotate( inverse_orientation, dir );
                    const wp_f32 mesh_max_dist = hit_any ? best_t : max_dist;
                    wp_vec3f local_normal = vec3f_zero();
                    wp_s32 triangle_index = -1;

                    /*
                     * Mesh data is stored in shape-local space. Transforming
                     * the ray once avoids three quaternion rotations for every
                     * triangle. The cooked triangle mesh also provides a BVH,
                     * reducing suspension queries to nearby leaves.
                     */
                    if( triangle_mesh )
                    {
                        hit = wp_triangle_mesh_raycast( triangle_mesh, local_start, local_dir,
                                                        mesh_max_dist, &t, &local_normal,
                                                        &triangle_index );
                    }
                    else
                    {
                        wp_u32 triangle;
                        for( triangle = 0; triangle < mesh->triangle_count; ++triangle )
                        {
                            const wp_u32 ia = mesh->indices[triangle * 3u];
                            const wp_u32 ib = mesh->indices[triangle * 3u + 1u];
                            const wp_u32 ic = mesh->indices[triangle * 3u + 2u];
                            wp_vec3f a;
                            wp_vec3f b;
                            wp_vec3f c;
                            wp_f32 triangle_t;
                            wp_vec3f triangle_normal;

                            if( ia >= mesh->vertex_count || ib >= mesh->vertex_count ||
                                ic >= mesh->vertex_count )
                            {
                                continue;
                            }

                            a.x = mesh->vertices[ia * 3u];
                            a.y = mesh->vertices[ia * 3u + 1u];
                            a.z = mesh->vertices[ia * 3u + 2u];
                            b.x = mesh->vertices[ib * 3u];
                            b.y = mesh->vertices[ib * 3u + 1u];
                            b.z = mesh->vertices[ib * 3u + 2u];
                            c.x = mesh->vertices[ic * 3u];
                            c.y = mesh->vertices[ic * 3u + 1u];
                            c.z = mesh->vertices[ic * 3u + 2u];

                            if( ray_triangle( local_start, local_dir, mesh_max_dist, a, b, c,
                                              &triangle_t, &triangle_normal ) &&
                                ( !hit || triangle_t < t ) )
                            {
                                hit = 1;
                                t = triangle_t;
                                local_normal = triangle_normal;
                            }
                        }
                    }

                    if( hit )
                    {
                        normal = quatf_rotate( shape_orientation, local_normal );
                    }
                }
            }

            if( hit && ( !hit_any || t < best_t ) )
            {
                /*
                 * Scene-query normals face the ray origin. Triangle winding
                 * differs between imported track meshes; exposing the raw
                 * winding normal made downward suspension rays occasionally
                 * report a downward-facing road surface.
                 */
                normal = vec3f_normalize( normal );
                if( vec3f_dot( normal, dir ) > 0.0f )
                {
                    normal = vec3f_scale( normal, -1.0f );
                }
                hit_any = 1;
                best_t = t;
                best_pos = vec3f_add( start, vec3f_scale( dir, t ) );
                best_normal = normal;
                best_body = body;
                best_shape = shape;
            }
        }
    }

    if( hit_any )
    {
        if( out_hit_pos )
        {
            *out_hit_pos = best_pos;
        }
        if( out_hit_normal )
        {
            *out_hit_normal = best_normal;
        }
        if( out_body )
        {
            *out_body = best_body;
        }
        if( out_shape )
        {
            *out_shape = best_shape;
        }
    }

    return hit_any;
}

/* =========================================================================
 * Native / user data
 * ====================================================================== */

void *wp_physics_scene_get_native( const wp_physics_scene *scene )
{
    if( !scene )
    {
        return NULL;
    }
    return scene->native;
}

void wp_physics_scene_set_native( wp_physics_scene *scene, void *native )
{
    if( !scene )
    {
        return;
    }
    scene->native = native;
}

void *wp_physics_scene_get_user_data( const wp_physics_scene *scene )
{
    if( !scene )
    {
        return NULL;
    }
    return scene->user_data;
}

void wp_physics_scene_set_user_data( wp_physics_scene *scene, void *user_data )
{
    if( !scene )
    {
        return;
    }
    scene->user_data = user_data;
}


void wp_physics_scene_set_spatial_partitioning( wp_physics_scene *scene, wp_spatial_partitioning_method method )
{
    if( !scene )
    {
        return;
    }
    scene->spatial_partitioning = method;
}

wp_spatial_partitioning_method wp_physics_scene_get_spatial_partitioning( const wp_physics_scene *scene )
{
    if( !scene )
    {
        return WP_SPATIAL_PARTITION_NONE;
    }
    return scene->spatial_partitioning;
}


void wp_physics_scene_set_contact_options( wp_physics_scene *scene, const wp_contact_options *options )
{
    if( !scene || !options ) return;
    if ( scene->contact_options.strategy != options->strategy )
        wp_collision_cache_clear( scene->contact_cache );
    scene->contact_options = *options;
    if ( scene->contact_options.strategy < WP_CONTACT_STRATEGY_ALWAYS ||
         scene->contact_options.strategy >= WP_CONTACT_STRATEGY_COUNT )
        scene->contact_options.strategy = WP_CONTACT_STRATEGY_ALWAYS;
    if ( scene->contact_options.strategy == WP_CONTACT_STRATEGY_ALWAYS )
        wp_collision_cache_clear( scene->contact_cache );
}
wp_narrowphase_stats wp_physics_scene_get_narrowphase_stats( const wp_physics_scene *scene )
{
    return wp_narrowphase_get_stats( scene ? scene->narrowphase : NULL );
}
void wp_physics_scene_set_narrowphase_timing_enabled( wp_physics_scene *scene, wp_s32 enabled )
{
    if(scene) wp_narrowphase_set_timing_enabled(scene->narrowphase,enabled);
}
wp_s32 wp_physics_scene_set_narrowphase_simd_enabled( wp_physics_scene *scene, wp_s32 enabled )
{
    return scene ? wp_narrowphase_set_simd_enabled( scene->narrowphase, enabled ) : 0;
}
void wp_physics_scene_set_narrowphase_mesh_obb_enabled( wp_physics_scene *scene, wp_s32 enabled )
{
    if ( scene )
        wp_narrowphase_set_mesh_obb_enabled( scene->narrowphase, enabled );
}
