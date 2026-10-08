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
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* =========================================================================
 * Forward declarations for internal types
 * ====================================================================== */

typedef struct wp_grid_cell wp_grid_cell;
typedef struct wp_grid wp_grid;
typedef struct wp_octree_node wp_octree_node;
typedef struct wp_bvh_node wp_bvh_node;

typedef struct wp_contact_cache_entry {
    wp_contact_manifold manifold;
    wp_rigidbody *body_a;
    wp_rigidbody *body_b;
    wp_u32 last_update_frame;
    wp_vec3f last_pos_a;
    wp_vec3f last_pos_b;
    wp_s32 active;
} wp_contact_cache_entry;

/* Forward declarations for internal functions */
static wp_contact_cache_entry *find_manifold( wp_physics_scene *scene, wp_rigidbody *a, wp_rigidbody *b );
static wp_s32 should_update_manifold( wp_physics_scene *scene, wp_contact_cache_entry *entry, wp_rigidbody *a, wp_rigidbody *b );
static void update_manifold_cache( wp_physics_scene *scene, wp_rigidbody *a, wp_rigidbody *b, wp_contact_manifold *manifold );
static wp_grid *grid_create( wp_physics_scene *scene );
static void grid_destroy( wp_grid *grid );
static wp_s32 grid_insert_actor( wp_grid *grid, wp_rigidbody *body );
static void solve_grid_contacts( wp_physics_scene *scene, wp_grid *grid );

/* =========================================================================
 * Internal structure
 * ====================================================================== */

typedef struct wp_physics_scene
{
    wp_rigidbody **actors;
    wp_s32 actor_count, actor_capacity;
    wp_f32 *sleep_time;
    struct wp_scene_aabb *actor_bounds;

    wp_vec3f size;
    wp_vec3f gravity;

    wp_u32 min_threads;
    wp_u32 max_threads;

    wp_narrowphase *narrowphase;
    wp_spatial_partitioning_method spatial_partitioning;
    wp_spatial_partitioning_options spatial_options;
    wp_contact_options contact_options;
    wp_contact_cache_entry *contact_cache;
    wp_s32 contact_cache_count;
    wp_s32 contact_cache_capacity;
    wp_u32 frame_count;
    void *partition_data;

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
    wp_collision_shape_type type;
    wp_vec3f center;
    wp_quatf orientation;

    if( !body || !shape || !bounds || !wp_collision_shape_is_enabled( shape ) )
    {
        return 0;
    }

    memset( bounds, 0, sizeof( *bounds ) );
    type = wp_collision_shape_get_type( shape );
    center = shape_world_position( body, shape );
    orientation = shape_world_orientation( body, shape );

    if( type == WORKPHONE_COLLISION_SHAPE_SPHERE )
    {
        wp_f32 radius = wp_collision_shape_get_sphere_radius( shape );
        wp_vec3f extents = { radius, radius, radius };
        scene_aabb_set_center_extents( bounds, center, extents );
        return 1;
    }

    if( type == WORKPHONE_COLLISION_SHAPE_BOX )
    {
        wp_vec3f half = wp_collision_shape_get_box_half_extents( shape );
        wp_vec3f axis_x = { 1.0f, 0.0f, 0.0f };
        wp_vec3f axis_y = { 0.0f, 1.0f, 0.0f };
        wp_vec3f axis_z = { 0.0f, 0.0f, 1.0f };
        wp_vec3f extents;
        axis_x = quatf_rotate( orientation, axis_x );
        axis_y = quatf_rotate( orientation, axis_y );
        axis_z = quatf_rotate( orientation, axis_z );
        extents.x = fabsf( axis_x.x ) * half.x + fabsf( axis_y.x ) * half.y + fabsf( axis_z.x ) * half.z;
        extents.y = fabsf( axis_x.y ) * half.x + fabsf( axis_y.y ) * half.y + fabsf( axis_z.y ) * half.z;
        extents.z = fabsf( axis_x.z ) * half.x + fabsf( axis_y.z ) * half.y + fabsf( axis_z.z ) * half.z;
        scene_aabb_set_center_extents( bounds, center, extents );
        return 1;
    }

    if( type == WORKPHONE_COLLISION_SHAPE_CAPSULE )
    {
        wp_f32 radius = wp_collision_shape_get_capsule_radius( shape );
        wp_f32 half_height = wp_collision_shape_get_capsule_half_height( shape );
        wp_vec3f axis = { 0.0f, 1.0f, 0.0f };
        wp_vec3f extents;
        axis = quatf_rotate( orientation, axis );
        extents.x = fabsf( axis.x ) * half_height + radius;
        extents.y = fabsf( axis.y ) * half_height + radius;
        extents.z = fabsf( axis.z ) * half_height + radius;
        scene_aabb_set_center_extents( bounds, center, extents );
        return 1;
    }

    if( type == WORKPHONE_COLLISION_SHAPE_PLANE )
    {
        bounds->min.x = bounds->min.y = bounds->min.z = -FLT_MAX;
        bounds->max.x = bounds->max.y = bounds->max.z = FLT_MAX;
        bounds->valid = 1;
        return 1;
    }

    if( type == WORKPHONE_COLLISION_SHAPE_MESH )
    {
        const wp_collision_mesh_data *mesh = wp_collision_shape_get_mesh_data( shape );
        const wp_triangle_mesh *triangle_mesh = wp_collision_shape_get_triangle_mesh( shape );
        wp_u32 vertex;
        if( triangle_mesh )
        {
            const wp_vec3f local_min = wp_triangle_mesh_get_aabb_min( triangle_mesh );
            const wp_vec3f local_max = wp_triangle_mesh_get_aabb_max( triangle_mesh );
            wp_vec3f local_center;
            wp_vec3f local_half;
            wp_vec3f world_center;
            wp_vec3f axis_x = { 1.0f, 0.0f, 0.0f };
            wp_vec3f axis_y = { 0.0f, 1.0f, 0.0f };
            wp_vec3f axis_z = { 0.0f, 0.0f, 1.0f };
            wp_vec3f extents;

            local_center = vec3f_scale( vec3f_add( local_min, local_max ), 0.5f );
            local_half = vec3f_scale( vec3f_sub( local_max, local_min ), 0.5f );
            world_center = vec3f_add( center, quatf_rotate( orientation, local_center ) );
            axis_x = quatf_rotate( orientation, axis_x );
            axis_y = quatf_rotate( orientation, axis_y );
            axis_z = quatf_rotate( orientation, axis_z );
            extents.x = fabsf( axis_x.x ) * local_half.x + fabsf( axis_y.x ) * local_half.y +
                        fabsf( axis_z.x ) * local_half.z;
            extents.y = fabsf( axis_x.y ) * local_half.x + fabsf( axis_y.y ) * local_half.y +
                        fabsf( axis_z.y ) * local_half.z;
            extents.z = fabsf( axis_x.z ) * local_half.x + fabsf( axis_y.z ) * local_half.y +
                        fabsf( axis_z.z ) * local_half.z;
            scene_aabb_set_center_extents( bounds, world_center, extents );
            return 1;
        }

        if( !mesh || !mesh->vertices || mesh->vertex_count == 0u )
        {
            return 0;
        }

        for( vertex = 0; vertex < mesh->vertex_count; ++vertex )
        {
            wp_vec3f local;
            wp_vec3f world;
            local.x = mesh->vertices[vertex * 3u];
            local.y = mesh->vertices[vertex * 3u + 1u];
            local.z = mesh->vertices[vertex * 3u + 2u];
            world = vec3f_add( center, quatf_rotate( orientation, local ) );
            if( !bounds->valid )
            {
                bounds->min = bounds->max = world;
                bounds->valid = 1;
            }
            else
            {
                bounds->min.x = fminf( bounds->min.x, world.x );
                bounds->min.y = fminf( bounds->min.y, world.y );
                bounds->min.z = fminf( bounds->min.z, world.z );
                bounds->max.x = fmaxf( bounds->max.x, world.x );
                bounds->max.y = fmaxf( bounds->max.y, world.y );
                bounds->max.z = fmaxf( bounds->max.z, world.z );
            }
        }
        return bounds->valid;
    }

    return 0;
}

static wp_s32 body_world_aabb( const wp_rigidbody *body, wp_scene_aabb *bounds )
{
    wp_s32 shape_index;
    if( !body || !bounds )
    {
        return 0;
    }

    memset( bounds, 0, sizeof( *bounds ) );
    for( shape_index = 0; shape_index < wp_rigidbody_get_shape_count( body ); ++shape_index )
    {
        wp_scene_aabb shape_bounds;
        if( shape_world_aabb( body, wp_rigidbody_get_shape( body, shape_index ), &shape_bounds ) )
        {
            scene_aabb_include( bounds, &shape_bounds );
        }
    }
    return bounds->valid;
}

static wp_s32 scene_aabbs_overlap( const wp_scene_aabb *a, const wp_scene_aabb *b )
{
    if( !a || !b || !a->valid || !b->valid )
    {
        return 0;
    }
    return !( a->max.x < b->min.x || a->min.x > b->max.x || a->max.y < b->min.y || a->min.y > b->max.y ||
              a->max.z < b->min.z || a->min.z > b->max.z );
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
    wp_scene_aabb *bounds;
    if( required <= scene->actor_capacity ) return 1;
    if( capacity < 8 ) capacity = 8;
    while( capacity < required ) {
        if( capacity > INT_MAX / 2 ) return 0;
        capacity *= 2;
    }
    actors = (wp_rigidbody **)calloc( capacity, sizeof(*actors) );
    sleep_time = (wp_f32 *)calloc( capacity, sizeof(*sleep_time) );
    bounds = (wp_scene_aabb *)calloc( capacity, sizeof(*bounds) );
    if( !actors || !sleep_time || !bounds ) {
        free(actors); free(sleep_time); free(bounds);
        return 0;
    }
    if( scene->actor_count ) {
        memcpy(actors, scene->actors, scene->actor_count * sizeof(*actors));
        memcpy(sleep_time, scene->sleep_time, scene->actor_count * sizeof(*sleep_time));
    }
    free(scene->actors); free(scene->sleep_time); free(scene->actor_bounds);
    scene->actors = actors;
    scene->sleep_time = sleep_time;
    scene->actor_bounds = bounds;
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
    scene->narrowphase = wp_narrowphase_create( WORKPHONE_NARROWPHASE_HYBRID );
    if( !scene->narrowphase )
    {
        free(scene->actors); free(scene->sleep_time); free(scene->actor_bounds);
        free( scene );
        return NULL;
    }

    /* Default contact options */
    scene->contact_options.strategy = WP_CONTACT_STRATEGY_ALWAYS;
    scene->contact_options.separation_threshold = 0.01f;
    scene->contact_options.fixed_update_frequency = 1;
    scene->contact_options.distance_frequency_scale = 1.0f;

    /* Initialize manifold cache */
    scene->contact_cache_capacity = 1024;
    scene->contact_cache = (wp_contact_cache_entry *)calloc(
        scene->contact_cache_capacity, sizeof( wp_contact_cache_entry ) );
    scene->contact_cache_count = 0;

    return scene;
}

void wp_physics_scene_destroy( wp_physics_scene *scene )
{
    if( !scene )
    {
        return;
    }

    if( scene->partition_data ) {
        if( scene->spatial_partitioning == WP_SPATIAL_PARTITION_GRID ) {
            grid_destroy( (wp_grid *)scene->partition_data );
        } else {
            free( scene->partition_data );
        }
    }

    wp_narrowphase_destroy( scene->narrowphase );
    scene->narrowphase = NULL;
    
    if( scene->contact_cache ) {
        free( scene->contact_cache );
    }

    free(scene->actors); free(scene->sleep_time); free(scene->actor_bounds);
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
    scene->actor_count = 0;
}

/* =========================================================================
 * Actor management
 * ====================================================================== */

wp_s32 wp_physics_scene_add_actor( wp_physics_scene *scene, wp_rigidbody *body )
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

    if( scene->actor_count == INT_MAX || !reserve_scene_actors(scene, scene->actor_count + 1) )
        return 0;
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
            scene->actors[i] = scene->actors[last];
            scene->sleep_time[i] = scene->sleep_time[last];
            scene->actors[last] = NULL;
            scene->sleep_time[last] = 0.0f;
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

static wp_contact_cache_entry *find_manifold( wp_physics_scene *scene, wp_rigidbody *a, wp_rigidbody *b )
{
    for( wp_s32 i = 0; i < scene->contact_cache_capacity; ++i ) {
        wp_contact_cache_entry *entry = &scene->contact_cache[i];
        if( entry->active && ((entry->body_a == a && entry->body_b == b) || (entry->body_a == b && entry->body_b == a)) ) {
            return entry;
        }
    }
    return NULL;
}

static wp_s32 should_update_manifold( wp_physics_scene *scene, wp_contact_cache_entry *entry, wp_rigidbody *a, wp_rigidbody *b )
{
    if( !entry ) return 1;

    wp_u32 frame = scene->frame_count;
    wp_contact_options opts = scene->contact_options;

    if( opts.strategy == WP_CONTACT_STRATEGY_ALWAYS ) return 1;

    /* Check separation threshold */
    wp_vec3f pos_a = wp_rigidbody_get_position( a );
    wp_vec3f pos_b = wp_rigidbody_get_position( b );
    wp_vec3f diff_a = vec3f_sub( pos_a, entry->last_pos_a );
    wp_vec3f diff_b = vec3f_sub( pos_b, entry->last_pos_b );
    wp_f32 sep_sq = vec3f_dot( diff_a, diff_a ) + vec3f_dot( diff_b, diff_b );

    if( sep_sq > (opts.separation_threshold * opts.separation_threshold) ) return 1;

    /* Strategy based updates */
    if( opts.strategy == WP_CONTACT_STRATEGY_FIXED ) {
        if( (frame - entry->last_update_frame) >= opts.fixed_update_frequency ) return 1;
    } else if( opts.strategy == WP_CONTACT_STRATEGY_DISTANCE ) {
        wp_vec3f rel_pos = vec3f_sub( pos_a, pos_b );
        wp_f32 dist_sq = vec3f_dot( rel_pos, rel_pos );
        wp_f32 freq = 1.0f + (dist_sq * opts.distance_frequency_scale);
        if( (frame - entry->last_update_frame) >= (wp_u32)freq ) return 1;
    }

    return 0;
}

static void update_manifold_cache( wp_physics_scene *scene, wp_rigidbody *a, wp_rigidbody *b, wp_contact_manifold *manifold )
{
    wp_contact_cache_entry *entry = find_manifold( scene, a, b );
    if( !entry ) {
        for( wp_s32 i = 0; i < scene->contact_cache_capacity; ++i ) {
            if( !scene->contact_cache[i].active ) {
                entry = &scene->contact_cache[i];
                entry->active = 1;
                entry->body_a = a;
                entry->body_b = b;
                break;
            }
        }
    }

    if( entry ) {
        entry->manifold = *manifold;
        entry->last_update_frame = scene->frame_count;
        entry->last_pos_a = wp_rigidbody_get_position( a );
        entry->last_pos_b = wp_rigidbody_get_position( b );
    }
}

/* =========================================================================
 * Spatial Partitioning Internal Implementation
 * ====================================================================== */

typedef struct wp_grid_cell {
    wp_rigidbody **actors;
    wp_s32 count, capacity;
    wp_f32 sleep_timer;
    wp_u32 last_update_frame;
} wp_grid_cell;

typedef struct wp_grid {
    wp_grid_cell *cells;
    wp_s32 dim_x, dim_y, dim_z;
    wp_vec3f cell_size;
    wp_vec3f origin;
} wp_grid;

struct wp_octree_node {
    wp_scene_aabb bounds;
    wp_rigidbody *actors[WP_SCENE_MAX_ACTORS];
    wp_s32 count;
    struct wp_octree_node *children[8];
    wp_f32 sleep_timer;
    wp_u32 last_update_frame;
};

typedef struct wp_bvh_node {
    wp_scene_aabb bounds;
    wp_u32 first_actor;
    wp_u32 actor_count;
    wp_u32 second_child; /* index to other child */
    wp_s32 axis;
    wp_f32 sleep_timer;
    wp_u32 last_update_frame;
} wp_bvh_node;

static wp_grid *grid_create( wp_physics_scene *scene ) {
    wp_grid *grid = (wp_grid *)malloc( sizeof( wp_grid ) );
    if( !grid ) return NULL;

    grid->dim_x = 10;
    grid->dim_y = 10;
    grid->dim_z = 10;
    grid->cells = (wp_grid_cell *)calloc( grid->dim_x * grid->dim_y * grid->dim_z, sizeof( wp_grid_cell ) );

    if( !grid->cells ) { free(grid); return NULL; }
    grid->origin = vec3f_scale( scene->size, -0.5f );
    grid->cell_size.x = scene->size.x / (wp_f32)grid->dim_x;
    grid->cell_size.y = scene->size.y / (wp_f32)grid->dim_y;
    grid->cell_size.z = scene->size.z / (wp_f32)grid->dim_z;

    return grid;
}

static void grid_destroy( wp_grid *grid ) {
    if( grid ) {
        for(wp_s32 i = 0; i < grid->dim_x * grid->dim_y * grid->dim_z; ++i)
            free(grid->cells[i].actors);
        free( grid->cells );
        free( grid );
    }
}

static wp_s32 grid_insert_actor( wp_grid *grid, wp_rigidbody *body ) {
    wp_scene_aabb bounds;
    body_world_aabb( body, &bounds );

    wp_vec3f min = vec3f_sub( bounds.min, grid->origin );
    wp_vec3f max = vec3f_sub( bounds.max, grid->origin );

    wp_s32 start_x = (wp_s32)(min.x / grid->cell_size.x);
    wp_s32 start_y = (wp_s32)(min.y / grid->cell_size.y);
    wp_s32 start_z = (wp_s32)(min.z / grid->cell_size.z);
    wp_s32 end_x = (wp_s32)(max.x / grid->cell_size.x);
    wp_s32 end_y = (wp_s32)(max.y / grid->cell_size.y);
    wp_s32 end_z = (wp_s32)(max.z / grid->cell_size.z);

    start_x = (wp_s32)clampf_scene( (wp_f32)start_x, 0.0f, (wp_f32)( grid->dim_x - 1 ) );
    start_y = (wp_s32)clampf_scene( (wp_f32)start_y, 0.0f, (wp_f32)( grid->dim_y - 1 ) );
    start_z = (wp_s32)clampf_scene( (wp_f32)start_z, 0.0f, (wp_f32)( grid->dim_z - 1 ) );
    end_x = (wp_s32)clampf_scene( (wp_f32)end_x, 0.0f, (wp_f32)( grid->dim_x - 1 ) );
    end_y = (wp_s32)clampf_scene( (wp_f32)end_y, 0.0f, (wp_f32)( grid->dim_y - 1 ) );
    end_z = (wp_s32)clampf_scene( (wp_f32)end_z, 0.0f, (wp_f32)( grid->dim_z - 1 ) );

    for( wp_s32 x = start_x; x <= end_x; ++x ) {
        for( wp_s32 y = start_y; y <= end_y; ++y ) {
            for( wp_s32 z = start_z; z <= end_z; ++z ) {
                wp_grid_cell *cell = &grid->cells[x * grid->dim_y * grid->dim_z + y * grid->dim_z + z];
                if( cell->count == cell->capacity ) {
                    wp_s32 capacity;
                    wp_rigidbody **actors;
                    if( cell->capacity > INT_MAX / 2 ) return 0;
                    capacity = cell->capacity ? cell->capacity * 2 : 8;
                    actors = (wp_rigidbody **)realloc(cell->actors, capacity * sizeof(*actors));
                    if( !actors ) return 0;
                    cell->actors = actors;
                    cell->capacity = capacity;
                }
                cell->actors[cell->count++] = body;
            }
        }
    }
    return 1;
}

static void solve_grid_contacts( wp_physics_scene *scene, wp_grid *grid ) {
    wp_u32 frame = scene->frame_count;
    wp_u32 freq = scene->spatial_options.update_frequency ? scene->spatial_options.update_frequency : 1;

    for( wp_s32 i = 0; i < grid->dim_x * grid->dim_y * grid->dim_z; ++i ) {
        wp_grid_cell *cell = &grid->cells[i];

        if( (frame % freq) != 0 ) continue;
        if( cell->sleep_timer > 0.5f ) continue;

        for( wp_s32 a = 0; a < cell->count; ++a ) {
            wp_rigidbody *body_a = cell->actors[a];
            for( wp_s32 b = a + 1; b < cell->count; ++b ) {
                wp_rigidbody *body_b = cell->actors[b];

                if( !body_a || !body_b ) continue;
                if( body_inverse_mass( body_a ) <= 0.0f && body_inverse_mass( body_b ) <= 0.0f ) continue;

                wp_scene_aabb bounds_a, bounds_b;
                body_world_aabb( body_a, &bounds_a );
                body_world_aabb( body_b, &bounds_b );
                if( !scene_aabbs_overlap( &bounds_a, &bounds_b ) ) continue;

                for( wp_s32 sa = 0; sa < wp_rigidbody_get_shape_count( body_a ); ++sa ) {
                    wp_collision_shape *shape_a = wp_rigidbody_get_shape( body_a, sa );
                    for( wp_s32 sb = 0; sb < wp_rigidbody_get_shape_count( body_b ); ++sb ) {
                        wp_collision_shape *shape_b = wp_rigidbody_get_shape( body_b, sb );

                        if( !shape_pair_filter_passes( body_a, shape_a, body_b, shape_b ) ) continue;

                        wp_contact_manifold manifold;
                        wp_contact_cache_entry *entry = find_manifold( scene, body_a, body_b );

                        if( entry && !should_update_manifold( scene, entry, body_a, body_b ) ) {
                            manifold = entry->manifold;
                        } else {
                            if( wp_narrowphase_test_pair( scene->narrowphase, body_a, shape_a, body_b, shape_b, &manifold ) ) {
                                update_manifold_cache( scene, body_a, body_b, &manifold );
                            } else {
                                if( entry ) entry->active = 0;
                                continue;
                            }
                        }

                        for( wp_s32 c = 0; c < manifold.contact_count; ++c ) {
                            resolve_contact( body_a, shape_a, body_b, shape_b, &manifold.contacts[c], 1.0f / (wp_f32)manifold.contact_count );
                        }
                    }
                }
            }
        }
        cell->sleep_timer = 0.0f;
    }
}

static void solve_scene_contacts( wp_physics_scene *scene )
{
    if( !scene ) return;

    if( scene->spatial_partitioning == WP_SPATIAL_PARTITION_GRID && scene->partition_data ) {
        wp_grid *grid = (wp_grid *)scene->partition_data;
        
        wp_s32 complete = 1;
        for(wp_s32 i = 0; i < grid->dim_x * grid->dim_y * grid->dim_z; ++i)
            grid->cells[i].count = 0;
        for( wp_s32 i = 0; i < scene->actor_count; ++i ) {
            if( scene->actors[i] && !grid_insert_actor(grid, scene->actors[i]) ) {
                complete = 0;
                break;
            }
        }
        if( complete ) { solve_grid_contacts(scene, grid); return; }
        /* Allocation failure must not silently drop contacts. Fall back below. */
    }
    {
        wp_s32 actor_a;
        wp_scene_aabb *actor_bounds = scene->actor_bounds;

        for( actor_a = 0; actor_a < scene->actor_count; ++actor_a ) {
            body_world_aabb( scene->actors[actor_a], &actor_bounds[actor_a] );
        }

        for( actor_a = 0; actor_a < scene->actor_count; ++actor_a ) {
            wp_rigidbody *body_a = scene->actors[actor_a];
            if( !body_a || !wp_rigidbody_has_flag( body_a, WORKPHONE_RIGIDBODY_FLAG_ENABLED ) ) continue;

            for( wp_s32 actor_b = actor_a + 1; actor_b < scene->actor_count; ++actor_b ) {
                wp_rigidbody *body_b = scene->actors[actor_b];
                if( !body_b || !wp_rigidbody_has_flag( body_b, WORKPHONE_RIGIDBODY_FLAG_ENABLED ) ||
                    ( body_inverse_mass( body_a ) <= 0.0f && body_inverse_mass( body_b ) <= 0.0f ) ||
                    !scene_aabbs_overlap( &actor_bounds[actor_a], &actor_bounds[actor_b] ) ) continue;

                for( wp_s32 sa = 0; sa < wp_rigidbody_get_shape_count( body_a ); ++sa ) {
                    wp_collision_shape *shape_a = wp_rigidbody_get_shape( body_a, sa );
                    for( wp_s32 sb = 0; sb < wp_rigidbody_get_shape_count( body_b ); ++sb ) {
                        wp_collision_shape *shape_b = wp_rigidbody_get_shape( body_b, sb );
                        
                        if( !shape_pair_filter_passes( body_a, shape_a, body_b, shape_b ) ) continue;

                        wp_contact_manifold manifold;
                        wp_contact_cache_entry *entry = find_manifold( scene, body_a, body_b );
                        
                        if( entry && !should_update_manifold( scene, entry, body_a, body_b ) ) {
                            manifold = entry->manifold;
                        } else {
                            if( wp_narrowphase_test_pair( scene->narrowphase, body_a, shape_a, body_b, shape_b, &manifold ) ) {
                                update_manifold_cache( scene, body_a, body_b, &manifold );
                            } else {
                                if( entry ) entry->active = 0;
                                continue;
                            }
                        }

                        for( wp_s32 c = 0; c < manifold.contact_count; ++c ) {
                            resolve_contact( body_a, shape_a, body_b, shape_b, &manifold.contacts[c], 1.0f / (wp_f32)manifold.contact_count );
                        }
                    }
                }
            }
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
    substeps = calculate_motion_substeps( scene, dt );
    substep_dt = dt / (wp_f32)substeps;

    for( step = 0; step < substeps; ++step )
    {
        wp_s32 i;
        for( i = 0; i < scene->actor_count; ++i )
        {
            integrate_body( scene->actors[i], scene->gravity, substep_dt );
        }

        solve_scene_constraints( scene, substep_dt );
        solve_scene_contacts( scene );
        solve_scene_constraints( scene, substep_dt );
    }

    finish_simulation_step( scene, dt );
    scene->frame_count++;
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
    scene->contact_options = *options;
}

