#include "workphone_physics_geometry.h"
#include "workphone_physics_triangle_mesh.h"
#include <float.h>
#include <math.h>
#include <string.h>

#ifdef _MSC_VER
#include <intrin.h>
static __declspec( align( 8 ) ) volatile __int64 next_id;
uint64_t wp_physics_next_lifetime_id( void )
{
    return (uint64_t)_InterlockedIncrement64( &next_id );
}
#else
static uint64_t next_id;
uint64_t wp_physics_next_lifetime_id( void )
{
    return __atomic_add_fetch( &next_id, 1, __ATOMIC_RELAXED );
}
#endif

static wp_vec3f add( wp_vec3f a, wp_vec3f b )
{
    return ( wp_vec3f ){ a.x + b.x, a.y + b.y, a.z + b.z };
}
static wp_vec3f scale( wp_vec3f a, wp_f32 s )
{
    return ( wp_vec3f ){ a.x * s, a.y * s, a.z * s };
}
static wp_vec3f cross( wp_vec3f a, wp_vec3f b )
{
    return ( wp_vec3f ){ a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
static wp_quatf normalize( wp_quatf q )
{
    wp_f32 n = sqrtf( q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z );
    if ( n <= 1.e-8f )
        return ( wp_quatf ){ 1, 0, 0, 0 };
    return ( wp_quatf ){ q.w / n, q.x / n, q.y / n, q.z / n };
}
static wp_quatf multiply( wp_quatf a, wp_quatf b )
{
    return normalize( ( wp_quatf ){ a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
                                    a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                                    a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
                                    a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w } );
}
static wp_vec3f rotate( wp_quatf q, wp_vec3f v )
{
    wp_vec3f xyz = { q.x, q.y, q.z }, t = scale( cross( xyz, v ), 2 );
    return add( v, add( scale( t, q.w ), cross( xyz, t ) ) );
}
static void include_point( wp_prepared_shape *p, wp_vec3f v )
{
    if ( !p->bounds_valid )
    {
        p->minimum = p->maximum = v;
        p->bounds_valid = 1;
        return;
    }
    p->minimum.x = fminf( p->minimum.x, v.x );
    p->maximum.x = fmaxf( p->maximum.x, v.x );
    p->minimum.y = fminf( p->minimum.y, v.y );
    p->maximum.y = fmaxf( p->maximum.y, v.y );
    p->minimum.z = fminf( p->minimum.z, v.z );
    p->maximum.z = fmaxf( p->maximum.z, v.z );
}
const wp_prepared_shape *wp_shape_prepare( const wp_rigidbody *body,
                                           const wp_collision_shape *shape, wp_s32 *rebuilt )
{
    wp_prepared_shape *p = wp_collision_shape_get_prepared_storage( shape );
    wp_collision_shape_type type;
    wp_vec3f extent = { 0, 0, 0 }, bound_center, local, body_position;
    wp_quatf body_orientation;
    uint64_t revision = wp_rigidbody_get_bounds_revision( body ),
             own = wp_collision_shape_get_revision( shape );
    if ( rebuilt )
        *rebuilt = 0;
    if ( !p )
        return NULL;
    if ( p->body == body && p->body_id == wp_rigidbody_get_lifetime_id( body ) &&
         p->body_revision == revision && p->shape_revision == own && !p->borrowed_vertices )
        return p;
    if ( rebuilt )
        *rebuilt = 1;
    memset( p, 0, sizeof( *p ) );
    p->body = body;
    p->body_id = wp_rigidbody_get_lifetime_id( body );
    p->body_revision = revision;
    p->shape_revision = own;
    body_orientation = normalize( wp_rigidbody_get_orientation( body ) );
    local = wp_collision_shape_get_local_position( shape );
    body_position = wp_rigidbody_get_position( body );
    p->center = add( body_position, rotate( body_orientation, local ) );
    p->orientation =
        multiply( body_orientation, wp_collision_shape_get_local_orientation( shape ) );
    p->inverse =
        ( wp_quatf ){ p->orientation.w, -p->orientation.x, -p->orientation.y, -p->orientation.z };
    p->axes[0] = rotate( p->orientation, ( wp_vec3f ){ 1, 0, 0 } );
    p->axes[1] = rotate( p->orientation, ( wp_vec3f ){ 0, 1, 0 } );
    p->axes[2] = rotate( p->orientation, ( wp_vec3f ){ 0, 0, 1 } );
    p->half = wp_collision_shape_get_box_half_extents( shape );
    p->radius = wp_collision_shape_get_sphere_radius( shape );
    p->half_height = wp_collision_shape_get_capsule_half_height( shape );
    p->segment_start = add( p->center, scale( p->axes[1], -p->half_height ) );
    p->segment_end = add( p->center, scale( p->axes[1], p->half_height ) );
    p->roundoff = 8 * FLT_EPSILON *
                  ( fabsf( local.x ) + fabsf( local.y ) + fabsf( local.z ) +
                    fabsf( body_position.x ) + fabsf( body_position.y ) + fabsf( body_position.z ) +
                    fabsf( p->center.x ) + fabsf( p->center.y ) + fabsf( p->center.z ) + 1 );
    bound_center = p->center;
    type = wp_collision_shape_get_type( shape );
    if ( type == WORKPHONE_COLLISION_SHAPE_PLANE )
        return p;
    if ( type == WORKPHONE_COLLISION_SHAPE_SPHERE )
        extent = ( wp_vec3f ){ p->radius, p->radius, p->radius };
    else if ( type == WORKPHONE_COLLISION_SHAPE_CAPSULE )
    {
        p->radius = wp_collision_shape_get_capsule_radius( shape );
        extent = ( wp_vec3f ){ fabsf( p->axes[1].x ) * p->half_height + p->radius,
                               fabsf( p->axes[1].y ) * p->half_height + p->radius,
                               fabsf( p->axes[1].z ) * p->half_height + p->radius };
    }
    else
    {
        wp_vec3f half = p->half;
        if ( type == WORKPHONE_COLLISION_SHAPE_MESH )
        {
            const wp_triangle_mesh *mesh = wp_collision_shape_get_triangle_mesh( shape );
            if ( mesh )
            {
                wp_vec3f lo = wp_triangle_mesh_get_aabb_min( mesh ),
                         hi = wp_triangle_mesh_get_aabb_max( mesh );
                half = ( wp_vec3f ){ ( hi.x - lo.x ) * .5f, ( hi.y - lo.y ) * .5f,
                                     ( hi.z - lo.z ) * .5f };
                bound_center =
                    add( p->center, rotate( p->orientation, scale( add( lo, hi ), .5f ) ) );
            }
            else
            {
                const wp_collision_mesh_data *data = wp_collision_shape_get_mesh_data( shape );
                p->borrowed_vertices = 1;
                if ( data && data->vertices )
                    for ( wp_u32 i = 0; i < data->vertex_count; ++i )
                        include_point(
                            p,
                            add( p->center, rotate( p->orientation,
                                                    ( wp_vec3f ){ data->vertices[i * 3],
                                                                  data->vertices[i * 3 + 1],
                                                                  data->vertices[i * 3 + 2] } ) ) );
                return p;
            }
        }
        else if ( type != WORKPHONE_COLLISION_SHAPE_BOX )
            return p;
        extent = ( wp_vec3f ){ fabsf( p->axes[0].x ) * half.x + fabsf( p->axes[1].x ) * half.y +
                                   fabsf( p->axes[2].x ) * half.z,
                               fabsf( p->axes[0].y ) * half.x + fabsf( p->axes[1].y ) * half.y +
                                   fabsf( p->axes[2].y ) * half.z,
                               fabsf( p->axes[0].z ) * half.x + fabsf( p->axes[1].z ) * half.y +
                                   fabsf( p->axes[2].z ) * half.z };
    }
    p->minimum = add( bound_center, scale( extent, -1 ) );
    p->maximum = add( bound_center, extent );
    p->roundoff += 8 * FLT_EPSILON *
                   ( fabsf( bound_center.x ) + fabsf( bound_center.y ) + fabsf( bound_center.z ) +
                     fabsf( extent.x ) + fabsf( extent.y ) + fabsf( extent.z ) );
    p->bounds_valid = isfinite( p->minimum.x ) && isfinite( p->minimum.y ) &&
                      isfinite( p->minimum.z ) && isfinite( p->maximum.x ) &&
                      isfinite( p->maximum.y ) && isfinite( p->maximum.z );
    return p;
}
wp_s32 wp_prepared_shapes_may_overlap( const wp_prepared_shape *a, const wp_prepared_shape *b,
                                       wp_f32 tolerance )
{
    wp_f32 t = fmaxf( tolerance, 0 ) + a->roundoff + b->roundoff;
    if ( !a->bounds_valid || !b->bounds_valid )
        return 1;
    return !( a->minimum.x > b->maximum.x + t || b->minimum.x > a->maximum.x + t ||
              a->minimum.y > b->maximum.y + t || b->minimum.y > a->maximum.y + t ||
              a->minimum.z > b->maximum.z + t || b->minimum.z > a->maximum.z + t );
}
