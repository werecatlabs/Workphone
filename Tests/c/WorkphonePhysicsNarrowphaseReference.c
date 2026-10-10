/* Scalar kernels retained from the pre-optimization implementation for regression comparisons.
 * This file is compiled only into tests, never into WorkphonePhysics. */
#include "WorkphonePhysicsNarrowphaseReference.h"
#include <math.h>
#include <string.h>
static wp_f32 np_sqrtf( wp_f32 x )
{
    return sqrtf( x );
}

static wp_f32 np_dot( wp_vec3f a, wp_vec3f b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static wp_vec3f np_sub( wp_vec3f a, wp_vec3f b )
{
    wp_vec3f r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

static wp_vec3f np_add( wp_vec3f a, wp_vec3f b )
{
    wp_vec3f r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

static wp_vec3f np_scale( wp_vec3f v, wp_f32 s )
{
    wp_vec3f r;
    r.x = v.x * s;
    r.y = v.y * s;
    r.z = v.z * s;
    return r;
}

static wp_f32 np_len_sq( wp_vec3f v )
{
    return np_dot( v, v );
}

static wp_f32 np_len( wp_vec3f v )
{
    return np_sqrtf( np_len_sq( v ) );
}

static wp_vec3f np_normalize( wp_vec3f v )
{
    wp_f32 len = np_len( v );
    if ( len < 1e-8f )
    {
        wp_vec3f up;
        memset( &up, 0, sizeof( up ) );
        up.y = 1.0f;
        return up;
    }
    return np_scale( v, 1.0f / len );
}

static wp_vec3f np_cross( wp_vec3f a, wp_vec3f b )
{
    wp_vec3f r;
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}

static wp_f32 np_clampf( wp_f32 v, wp_f32 lo, wp_f32 hi )
{
    return v < lo ? lo : ( v > hi ? hi : v );
}

static wp_f32 np_fabsf( wp_f32 v )
{
    return v < 0.0f ? -v : v;
}

static wp_vec3f np_zero( void )
{
    wp_vec3f v;
    memset( &v, 0, sizeof( v ) );
    return v;
}

static wp_quatf np_quat_normalize( wp_quatf q )
{
    wp_f32 length = sqrtf( q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w );
    if ( length <= 1.0e-8f )
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

static wp_quatf np_quat_multiply( wp_quatf a, wp_quatf b )
{
    wp_quatf q;
    q.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    q.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    q.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    q.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    return q;
}

static wp_quatf np_quat_conjugate( wp_quatf q )
{
    q = np_quat_normalize( q );
    q.x = -q.x;
    q.y = -q.y;
    q.z = -q.z;
    return q;
}

static wp_vec3f np_quat_rotate( wp_quatf q, wp_vec3f v )
{
    wp_vec3f qv;
    wp_vec3f t;
    q = np_quat_normalize( q );
    qv.x = q.x;
    qv.y = q.y;
    qv.z = q.z;
    t = np_scale( np_cross( qv, v ), 2.0f );
    return np_add( v, np_add( np_scale( t, q.w ), np_cross( qv, t ) ) );
}

static void manifold_add_contact( wp_contact_manifold *m, wp_vec3f a, wp_vec3f b, wp_vec3f n,
                                  float d )
{
    m->contact_count = 1;
    m->is_touching = 1;
    m->contacts[0].position_world_on_a = a;
    m->contacts[0].position_world_on_b = b;
    m->contacts[0].normal_world_on_b = n;
    m->contacts[0].penetration_depth = d;
}
/* ----- box vs box (oriented SAT, 15 separating axes) -------------------- */

static wp_s32 np_obb_axis_overlap( wp_vec3f axis, wp_vec3f center_delta, const wp_vec3f axes_a[3],
                                   wp_vec3f half_a, const wp_vec3f axes_b[3], wp_vec3f half_b,
                                   wp_f32 tolerance, wp_f32 *minimum_overlap,
                                   wp_vec3f *minimum_axis )
{
    wp_f32 length_sq = np_len_sq( axis );
    wp_f32 radius_a;
    wp_f32 radius_b;
    wp_f32 center_distance;
    wp_f32 overlap;

    if ( length_sq <= 1.0e-12f )
    {
        return 1;
    }

    axis = np_scale( axis, 1.0f / np_sqrtf( length_sq ) );
    radius_a = np_fabsf( np_dot( axis, axes_a[0] ) ) * half_a.x +
               np_fabsf( np_dot( axis, axes_a[1] ) ) * half_a.y +
               np_fabsf( np_dot( axis, axes_a[2] ) ) * half_a.z;
    radius_b = np_fabsf( np_dot( axis, axes_b[0] ) ) * half_b.x +
               np_fabsf( np_dot( axis, axes_b[1] ) ) * half_b.y +
               np_fabsf( np_dot( axis, axes_b[2] ) ) * half_b.z;
    center_distance = np_dot( center_delta, axis );
    overlap = radius_a + radius_b - np_fabsf( center_distance );
    if ( overlap < -tolerance )
    {
        return 0;
    }
    if ( overlap < *minimum_overlap )
    {
        *minimum_overlap = overlap;
        *minimum_axis = center_distance >= 0.0f ? axis : np_scale( axis, -1.0f );
    }
    return 1;
}

wp_s32 reference_box_box( wp_vec3f center_a, wp_quatf orientation_a, wp_vec3f half_a,
                          wp_vec3f center_b, wp_quatf orientation_b, wp_vec3f half_b,
                          wp_f32 tolerance, wp_contact_manifold *out )
{
    const wp_vec3f local_axes[3] = {
        { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
    wp_vec3f axes_a[3];
    wp_vec3f axes_b[3];
    wp_vec3f center_delta = np_sub( center_b, center_a );
    wp_f32 minimum_overlap = 3.402823466e+38F;
    wp_vec3f normal = np_zero();
    wp_s32 i;
    wp_s32 j;

    for ( i = 0; i < 3; ++i )
    {
        axes_a[i] = np_quat_rotate( orientation_a, local_axes[i] );
        axes_b[i] = np_quat_rotate( orientation_b, local_axes[i] );
    }
    for ( i = 0; i < 3; ++i )
    {
        if ( !np_obb_axis_overlap( axes_a[i], center_delta, axes_a, half_a, axes_b, half_b,
                                   tolerance, &minimum_overlap, &normal ) ||
             !np_obb_axis_overlap( axes_b[i], center_delta, axes_a, half_a, axes_b, half_b,
                                   tolerance, &minimum_overlap, &normal ) )
        {
            return 0;
        }
    }
    for ( i = 0; i < 3; ++i )
    {
        for ( j = 0; j < 3; ++j )
        {
            if ( !np_obb_axis_overlap( np_cross( axes_a[i], axes_b[j] ), center_delta, axes_a,
                                       half_a, axes_b, half_b, tolerance, &minimum_overlap,
                                       &normal ) )
            {
                return 0;
            }
        }
    }

    {
        wp_f32 radius_a = np_fabsf( np_dot( normal, axes_a[0] ) ) * half_a.x +
                          np_fabsf( np_dot( normal, axes_a[1] ) ) * half_a.y +
                          np_fabsf( np_dot( normal, axes_a[2] ) ) * half_a.z;
        wp_f32 radius_b = np_fabsf( np_dot( normal, axes_b[0] ) ) * half_b.x +
                          np_fabsf( np_dot( normal, axes_b[1] ) ) * half_b.y +
                          np_fabsf( np_dot( normal, axes_b[2] ) ) * half_b.z;
        wp_vec3f on_a = np_add( center_a, np_scale( normal, radius_a ) );
        wp_vec3f on_b = np_sub( center_b, np_scale( normal, radius_b ) );
        manifold_add_contact( out, on_a, on_b, normal,
                              minimum_overlap > 0.0f ? minimum_overlap : 0.0f );
    }
    return 1;
}

static wp_s32 np_triangle_box_axis_overlaps( wp_vec3f axis, wp_vec3f a, wp_vec3f b, wp_vec3f c,
                                             wp_vec3f half_extents, wp_f32 tolerance )
{
    wp_f32 length_sq = np_len_sq( axis );
    wp_f32 pa;
    wp_f32 pb;
    wp_f32 pc;
    wp_f32 minimum;
    wp_f32 maximum;
    wp_f32 radius;

    if ( length_sq <= 1.0e-12f )
    {
        return 1;
    }

    axis = np_scale( axis, 1.0f / np_sqrtf( length_sq ) );
    pa = np_dot( a, axis );
    pb = np_dot( b, axis );
    pc = np_dot( c, axis );
    minimum = pa < pb ? pa : pb;
    minimum = minimum < pc ? minimum : pc;
    maximum = pa > pb ? pa : pb;
    maximum = maximum > pc ? maximum : pc;
    radius = np_fabsf( axis.x ) * half_extents.x + np_fabsf( axis.y ) * half_extents.y +
             np_fabsf( axis.z ) * half_extents.z;
    return !( minimum > radius + tolerance || maximum < -radius - tolerance );
}

wp_s32 reference_triangle_box( wp_vec3f a, wp_vec3f b, wp_vec3f c, wp_vec3f half_extents,
                               wp_f32 tolerance )
{
    const wp_vec3f box_axes[3] = {
        { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
    wp_vec3f edges[3];
    wp_vec3f triangle_normal;
    wp_s32 edge;
    wp_s32 axis;

    edges[0] = np_sub( b, a );
    edges[1] = np_sub( c, b );
    edges[2] = np_sub( a, c );
    triangle_normal = np_cross( edges[0], np_sub( c, a ) );

    for ( axis = 0; axis < 3; ++axis )
    {
        if ( !np_triangle_box_axis_overlaps( box_axes[axis], a, b, c, half_extents, tolerance ) )
        {
            return 0;
        }
    }
    if ( !np_triangle_box_axis_overlaps( triangle_normal, a, b, c, half_extents, tolerance ) )
    {
        return 0;
    }
    for ( edge = 0; edge < 3; ++edge )
    {
        for ( axis = 0; axis < 3; ++axis )
        {
            if ( !np_triangle_box_axis_overlaps( np_cross( edges[edge], box_axes[axis] ), a, b, c,
                                                 half_extents, tolerance ) )
            {
                return 0;
            }
        }
    }
    return 1;
}

static wp_f32 np_point_aabb_distance_sq( wp_vec3f point, wp_vec3f half_extents, wp_vec3f *closest )
{
    closest->x = np_clampf( point.x, -half_extents.x, half_extents.x );
    closest->y = np_clampf( point.y, -half_extents.y, half_extents.y );
    closest->z = np_clampf( point.z, -half_extents.z, half_extents.z );
    return np_len_sq( np_sub( point, *closest ) );
}

float reference_segment_box( wp_vec3f a, wp_vec3f b, wp_vec3f half )
{
    float low = 0, high = 1;
    wp_vec3f direction = np_sub( b, a );
    for ( int i = 0; i < 32; ++i )
    {
        float t1 = ( 2 * low + high ) / 3, t2 = ( low + 2 * high ) / 3;
        wp_vec3f scratch;
        float d1 =
            np_point_aabb_distance_sq( np_add( a, np_scale( direction, t1 ) ), half, &scratch );
        float d2 =
            np_point_aabb_distance_sq( np_add( a, np_scale( direction, t2 ) ), half, &scratch );
        if ( d1 < d2 )
            high = t2;
        else
            low = t1;
    }
    wp_vec3f scratch;
    return np_point_aabb_distance_sq( np_add( a, np_scale( direction, ( low + high ) * .5f ) ),
                                      half, &scratch );
}
