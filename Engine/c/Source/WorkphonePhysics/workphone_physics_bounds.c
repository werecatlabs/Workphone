#include "workphone_physics_bounds.h"
#include "workphone_physics_collisionshape.h"
#include "workphone_physics_triangle_mesh.h"
#include <float.h>
#include <math.h>
#include <string.h>

static wp_vec3f add( wp_vec3f a, wp_vec3f b )
{
    return (wp_vec3f){ a.x + b.x, a.y + b.y, a.z + b.z };
}
static wp_vec3f sub( wp_vec3f a, wp_vec3f b )
{
    return (wp_vec3f){ a.x - b.x, a.y - b.y, a.z - b.z };
}
static wp_vec3f scale( wp_vec3f a, wp_f32 s )
{
    return (wp_vec3f){ a.x * s, a.y * s, a.z * s };
}
static wp_f32 dot( wp_vec3f a, wp_vec3f b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
static wp_vec3f cross( wp_vec3f a, wp_vec3f b )
{
    return (wp_vec3f){ a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                       a.x * b.y - a.y * b.x };
}
static wp_vec3f rotate( wp_quatf q, wp_vec3f v )
{
    wp_vec3f xyz = { q.x, q.y, q.z };
    wp_vec3f t = scale( cross( xyz, v ), 2.0f );
    return add( v, add( scale( t, q.w ), cross( xyz, t ) ) );
}
static void axes_from_quat( wp_quatf q, wp_vec3f axes[3] )
{
    /* Match contact geometry even for quaternions rounded by repeated edits. */
    wp_f32 length = sqrtf( q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w );
    if( !isfinite( length ) )
    {
        axes[0] = axes[1] = axes[2] = (wp_vec3f){ NAN, NAN, NAN };
        return;
    }
    if( length > 1.0e-8f )
    {
        q.x /= length;
        q.y /= length;
        q.z /= length;
        q.w /= length;
    }
    else
        q = (wp_quatf){ 1, 0, 0, 0 }; /* wp_quatf stores w first. */
    axes[0] = rotate( q, (wp_vec3f){ 1, 0, 0 } );
    axes[1] = rotate( q, (wp_vec3f){ 0, 1, 0 } );
    axes[2] = rotate( q, (wp_vec3f){ 0, 0, 1 } );
}
static wp_vec3f basis_transform( const wp_vec3f axes[3], wp_vec3f v )
{
    return add( scale( axes[0], v.x ), add( scale( axes[1], v.y ), scale( axes[2], v.z ) ) );
}
static wp_s32 finite_vector( wp_vec3f v )
{
    return isfinite( v.x ) && isfinite( v.y ) && isfinite( v.z );
}

/* Fit in body-local geometry space, never by rotating an existing world AABB.
 * A single child's local basis gives boxes/capsules/mesh bounds a tight fit.
 * Compounds use a conservative body-local union, including every enabled child. */
static void fit_local_obb( const wp_rigidbody *body, wp_body_obb *out )
{
    wp_collision_shape *only = NULL;
    wp_s32 i, j, count = 0;
    double minimum[3] = { DBL_MAX, DBL_MAX, DBL_MAX };
    double maximum[3] = { -DBL_MAX, -DBL_MAX, -DBL_MAX };
    memset( out, 0, sizeof( *out ) );
    for( i = 0; i < wp_rigidbody_get_shape_count( body ); ++i )
    {
        wp_collision_shape *shape = wp_rigidbody_get_shape( body, i );
        if( wp_collision_shape_is_enabled( shape ) )
        {
            only = shape;
            ++count;
        }
    }
    if( !count ) return;
    axes_from_quat( count == 1 ? wp_collision_shape_get_local_orientation( only ) :
                                  (wp_quatf){ 1, 0, 0, 0 }, out->axes );
    for( i = 0; i < wp_rigidbody_get_shape_count( body ); ++i )
    {
        wp_collision_shape *shape = wp_rigidbody_get_shape( body, i );
        wp_collision_shape_type type;
        wp_vec3f center, half = { 0, 0, 0 }, axes[3];
        if( !wp_collision_shape_is_enabled( shape ) ) continue;
        type = wp_collision_shape_get_type( shape );
        center = wp_collision_shape_get_local_position( shape );
        axes_from_quat( wp_collision_shape_get_local_orientation( shape ), axes );
        if( type == WORKPHONE_COLLISION_SHAPE_BOX )
            half = wp_collision_shape_get_box_half_extents( shape );
        else if( type == WORKPHONE_COLLISION_SHAPE_MESH )
        {
            const wp_triangle_mesh *mesh = wp_collision_shape_get_triangle_mesh( shape );
            wp_vec3f lo, hi;
            /* Borrowed uncooked vertices have no observable mutation contract.
             * Preserve the scene's live AABB fallback instead of caching them. */
            if( !mesh ) return;
            lo = wp_triangle_mesh_get_aabb_min( mesh );
            hi = wp_triangle_mesh_get_aabb_max( mesh );
            half = scale( sub( hi, lo ), 0.5f );
            center = add( center, basis_transform( axes, scale( add( lo, hi ), 0.5f ) ) );
        }
        else if( type != WORKPHONE_COLLISION_SHAPE_SPHERE &&
                 type != WORKPHONE_COLLISION_SHAPE_CAPSULE ) return; /* planes/unbounded */
        if( !finite_vector( center ) || !finite_vector( half ) ||
            half.x < 0 || half.y < 0 || half.z < 0 ) return;
        for( j = 0; j < 3; ++j )
        {
            wp_f32 c = dot( out->axes[j], center ), radius;
            if( type == WORKPHONE_COLLISION_SHAPE_SPHERE )
                radius = wp_collision_shape_get_sphere_radius( shape );
            else if( type == WORKPHONE_COLLISION_SHAPE_CAPSULE )
                radius = wp_collision_shape_get_capsule_radius( shape ) +
                         fabsf( dot( out->axes[j], axes[1] ) ) *
                         wp_collision_shape_get_capsule_half_height( shape );
            else
                radius = fabsf( dot( out->axes[j], axes[0] ) ) * half.x +
                         fabsf( dot( out->axes[j], axes[1] ) ) * half.y +
                         fabsf( dot( out->axes[j], axes[2] ) ) * half.z;
            if( !isfinite( c ) || !isfinite( radius ) || radius < 0 ) return;
            /* Double intervals keep small radii at large local offsets. */
            minimum[j] = fmin( minimum[j], (double)c - radius );
            maximum[j] = fmax( maximum[j], (double)c + radius );
            out->roundoff = fmaxf( out->roundoff, 8.0f * FLT_EPSILON *
                (fabsf( center.x ) + fabsf( center.y ) + fabsf( center.z ) + radius + 1.0f) );
        }
    }
    out->half = (wp_vec3f){ (wp_f32)((maximum[0] - minimum[0]) * 0.5),
                            (wp_f32)((maximum[1] - minimum[1]) * 0.5),
                            (wp_f32)((maximum[2] - minimum[2]) * 0.5) };
    out->center = basis_transform( out->axes,
        (wp_vec3f){ (wp_f32)((maximum[0] + minimum[0]) * 0.5),
                    (wp_f32)((maximum[1] + minimum[1]) * 0.5),
                    (wp_f32)((maximum[2] + minimum[2]) * 0.5) } );
    out->valid = finite_vector( out->half ) && finite_vector( out->center );
}

const wp_body_obb *wp_body_get_obb( wp_rigidbody *body, wp_s32 *local_rebuilt,
                                    wp_s32 *world_updated )
{
    wp_body_obb_cache *cache = wp_rigidbody_get_obb_cache( body );
    uint64_t geometry_revision = wp_rigidbody_get_geometry_revision( body );
    uint64_t pose_revision = wp_rigidbody_get_bounds_revision( body );
    *local_rebuilt = *world_updated = 0;
    if( !cache ) return NULL;
    if( cache->geometry_revision != geometry_revision )
    {
        fit_local_obb( body, &cache->local );
        cache->geometry_revision = geometry_revision;
        cache->pose_revision = 0;
        *local_rebuilt = 1;
    }
    if( cache->pose_revision != pose_revision )
    {
        wp_vec3f body_axes[3], extents;
        wp_body_obb *world = &cache->world;
        *world = cache->local;
        if( world->valid )
        {
            wp_vec3f position = wp_rigidbody_get_position( body );
            axes_from_quat( wp_rigidbody_get_orientation( body ), body_axes );
            world->center = add( position, basis_transform( body_axes, cache->local.center ) );
            /* Local offsets and body translation can be huge while their world
             * sum is small. World-center magnitude alone cannot bound roundoff. */
            world->roundoff += 8.0f * FLT_EPSILON *
                (fabsf( position.x ) + fabsf( position.y ) + fabsf( position.z ) +
                 fabsf( cache->local.center.x ) + fabsf( cache->local.center.y ) +
                 fabsf( cache->local.center.z ) + 1.0f);
            for( wp_s32 j = 0; j < 3; ++j )
                world->axes[j] = basis_transform( body_axes, cache->local.axes[j] );
            extents.x = fabsf( world->axes[0].x ) * world->half.x +
                        fabsf( world->axes[1].x ) * world->half.y +
                        fabsf( world->axes[2].x ) * world->half.z;
            extents.y = fabsf( world->axes[0].y ) * world->half.x +
                        fabsf( world->axes[1].y ) * world->half.y +
                        fabsf( world->axes[2].y ) * world->half.z;
            extents.z = fabsf( world->axes[0].z ) * world->half.x +
                        fabsf( world->axes[1].z ) * world->half.y +
                        fabsf( world->axes[2].z ) * world->half.z;
            world->valid = finite_vector( world->center ) && finite_vector( extents );
            /* Avoid SAT overhead when both oriented bounds are already well
             * represented by their enclosing world AABBs (including alignment). */
            world->useful = (double)extents.x * extents.y * extents.z >
                           (double)world->half.x * world->half.y * world->half.z * 1.1;
        }
        cache->pose_revision = pose_revision;
        *world_updated = 1;
    }
    return &cache->world;
}

wp_s32 wp_body_obb_may_overlap( const wp_body_obb *a, const wp_body_obb *b,
                                wp_f32 tolerance )
{
    wp_vec3f delta;
    wp_f32 radii[3][3], ah[3], bh[3], padding;
    wp_s32 i, j;
    if( !a || !b || !a->valid || !b->valid ) return 1;
    delta = sub( b->center, a->center );
    ah[0] = a->half.x; ah[1] = a->half.y; ah[2] = a->half.z;
    bh[0] = b->half.x; bh[1] = b->half.y; bh[2] = b->half.z;
    /* Scale-aware roundoff allowance, in addition to contact tolerance. This
     * must err toward retaining pairs, including touching/near-parallel boxes. */
    padding = fmaxf( tolerance, 0.0f ) + a->roundoff + b->roundoff + 8.0f * FLT_EPSILON *
        (fabsf( a->center.x ) + fabsf( a->center.y ) + fabsf( a->center.z ) +
         fabsf( b->center.x ) + fabsf( b->center.y ) + fabsf( b->center.z ) +
         ah[0] + ah[1] + ah[2] + bh[0] + bh[1] + bh[2] + 1.0f);
    for( i = 0; i < 3; ++i )
        for( j = 0; j < 3; ++j )
            radii[i][j] = fabsf( dot( a->axes[i], b->axes[j] ) );
    for( i = 0; i < 3; ++i )
    {
        wp_f32 rb = radii[i][0] * bh[0] + radii[i][1] * bh[1] + radii[i][2] * bh[2];
        wp_f32 ra = radii[0][i] * ah[0] + radii[1][i] * ah[1] + radii[2][i] * ah[2];
        if( fabsf( dot( delta, a->axes[i] ) ) > ah[i] + rb + padding ||
            fabsf( dot( delta, b->axes[i] ) ) > bh[i] + ra + padding ) return 0;
    }
    return 1;
}
