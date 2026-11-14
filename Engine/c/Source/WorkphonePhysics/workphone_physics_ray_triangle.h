#ifndef WORKPHONE_PHYSICS_RAY_TRIANGLE_H
#define WORKPHONE_PHYSICS_RAY_TRIANGLE_H

#include "workphone_vector.h"
#include <math.h>

/* Strict ray-aligned edge functions shared by cooked and fallback mesh queries.
 * Promote the original float coordinates before subtracting/shearing: adjacent
 * triangles then evaluate the same shared edge in opposite orders. No outside
 * barycentric tolerance or absolute triangle-area cutoff is introduced.
 * Keep declarations and arithmetic compatible with the native C89 contract. */
static wp_s32 wp_ray_triangle_distance_exact( wp_vec3f origin, wp_vec3f direction,
                                             wp_f32 max_distance, wp_vec3f va,
                                             wp_vec3f vb, wp_vec3f vc,
                                             wp_f32 *out_distance )
{
    double a[3], b[3], c[3], d[3];
    double sx, sy, sz;
    double ax, ay, bx, by, cx, cy;
    double e0, e1, e2, determinant, distance;
    int kx, ky, kz, temporary;

    a[0] = (double)va.x - (double)origin.x;
    a[1] = (double)va.y - (double)origin.y;
    a[2] = (double)va.z - (double)origin.z;
    b[0] = (double)vb.x - (double)origin.x;
    b[1] = (double)vb.y - (double)origin.y;
    b[2] = (double)vb.z - (double)origin.z;
    c[0] = (double)vc.x - (double)origin.x;
    c[1] = (double)vc.y - (double)origin.y;
    c[2] = (double)vc.z - (double)origin.z;
    d[0] = (double)direction.x;
    d[1] = (double)direction.y;
    d[2] = (double)direction.z;
    kz = 0;
    if( fabs( d[1] ) > fabs( d[kz] ) ) kz = 1;
    if( fabs( d[2] ) > fabs( d[kz] ) ) kz = 2;
    if( d[kz] == 0.0 ) return 0;
    kx = ( kz + 1 ) % 3;
    ky = ( kx + 1 ) % 3;
    if( d[kz] < 0.0 )
    {
        temporary = kx;
        kx = ky;
        ky = temporary;
    }
    sx = d[kx] / d[kz];
    sy = d[ky] / d[kz];
    sz = 1.0 / d[kz];
    ax = a[kx] - sx * a[kz];
    ay = a[ky] - sy * a[kz];
    bx = b[kx] - sx * b[kz];
    by = b[ky] - sy * b[kz];
    cx = c[kx] - sx * c[kz];
    cy = c[ky] - sy * c[kz];
    e0 = bx * cy - by * cx;
    e1 = cx * ay - cy * ax;
    e2 = ax * by - ay * bx;
    if( ( e0 < 0.0 || e1 < 0.0 || e2 < 0.0 ) &&
        ( e0 > 0.0 || e1 > 0.0 || e2 > 0.0 ) ) return 0;
    determinant = e0 + e1 + e2;
    if( determinant == 0.0 ) return 0;
    distance = ( e0 * ( sz * a[kz] ) + e1 * ( sz * b[kz] ) +
                 e2 * ( sz * c[kz] ) ) / determinant;
    /* Ordered comparisons also reject NaN intersection results. */
    if( !( distance >= 0.0 && distance <= (double)max_distance ) ) return 0;
    if( out_distance ) *out_distance = (wp_f32)distance;
    return 1;
}

/* A nondegenerate clipped triangle may have a cross product smaller than the
 * generic float-vector normalization threshold. Ray hits still require its
 * actual unit normal, including the small triangles accepted above. */
static wp_vec3f wp_ray_triangle_normal_exact( wp_vec3f a, wp_vec3f b, wp_vec3f c )
{
    double abx, aby, abz, acx, acy, acz, nx, ny, nz, length;
    wp_vec3f result;
    result.x = 0.0f;
    result.y = 0.0f;
    result.z = 0.0f;
    abx = (double)b.x - (double)a.x;
    aby = (double)b.y - (double)a.y;
    abz = (double)b.z - (double)a.z;
    acx = (double)c.x - (double)a.x;
    acy = (double)c.y - (double)a.y;
    acz = (double)c.z - (double)a.z;
    nx = aby * acz - abz * acy;
    ny = abz * acx - abx * acz;
    nz = abx * acy - aby * acx;
    length = sqrt( nx * nx + ny * ny + nz * nz );
    if( length > 0.0 )
    {
        result.x = (wp_f32)( nx / length );
        result.y = (wp_f32)( ny / length );
        result.z = (wp_f32)( nz / length );
    }
    return result;
}

#endif
