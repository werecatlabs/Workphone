/**
 * @file wp_quat.c
 * @brief C implementation of a small quaternion helper API (wp_f32 and wp_f64).
 */

#include "workphone_quat.h"
#include "workphone_vector.h"
#include "workphone_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Note: The public header may declare types differently. To keep this
 * implementation self-contained (the header in this workspace was empty),
 * we provide local definitions that are compatible with a common layout.
 */

/* -------------------------------------------------------------------------
 * wp_quatf (wp_f32) helpers
 * ---------------------------------------------------------------------- */

wp_quatf wp_quatf_make( wp_f32 w, wp_f32 x, wp_f32 y, wp_f32 z )
{
    wp_quatf q;
    q.w = w;
    q.x = x;
    q.y = y;
    q.z = z;
    return q;
}

wp_quatf wp_quatf_identity( void )
{
    return wp_quatf_make( 1.0f, 0.0f, 0.0f, 0.0f );
}

wp_f32 wp_quatf_dot( wp_quatf a, wp_quatf b )
{
    return a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
}

wp_f32 wp_quatf_length( wp_quatf q )
{
    return wp_sqrtf( wp_quatf_dot( q, q ) );
}

wp_quatf wp_quatf_normalize( wp_quatf q )
{
    wp_f32 len = wp_quatf_length( q );
    if( len > WORKPHONE_EPSILON_F )
    {
        wp_f32 inv = 1.0f / len;
        return wp_quatf_make( q.w * inv, q.x * inv, q.y * inv, q.z * inv );
    }
    return q;
}

wp_quatf wp_quatf_conjugate( wp_quatf q )
{
    return wp_quatf_make( q.w, -q.x, -q.y, -q.z );
}

wp_quatf wp_quatf_mul( wp_quatf a, wp_quatf b )
{
    /* (aw,ax,ay,az)*(bw,bx,by,bz) */
    wp_quatf r;
    r.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    r.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    r.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    r.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    return r;
}

wp_vec3f wp_quatf_rotate_vec3( wp_quatf q, wp_vec3f v )
{
    /* v' = q * (0,v) * q^-1 */
    wp_quatf p = wp_quatf_make( 0.0f, v.x, v.y, v.z );
    wp_quatf qn = wp_quatf_conjugate( q );
    wp_quatf tmp = wp_quatf_mul( q, p );
    wp_quatf res = wp_quatf_mul( tmp, qn );
    wp_vec3f out;
    out.x = res.x;
    out.y = res.y;
    out.z = res.z;
    return out;
}

wp_quatf wp_quatf_from_axis_angle( wp_vec3f axis, wp_f32 angle_rad )
{
    wp_vec3f n = wp_vec3f_normalize( axis );
    wp_f32 half = 0.5f * angle_rad;
    wp_f32 s = wp_sinf( half );
    return wp_quatf_make( wp_cosf( half ), n.x * s, n.y * s, n.z * s );
}

void wp_quatf_to_axis_angle( wp_quatf q, wp_vec3f *out_axis, wp_f32 *out_angle )
{
    wp_quatf nq = wp_quatf_normalize( q );
    wp_f32 angle = 2.0f * wp_acosf( nq.w );
    wp_f32 s = wp_sqrtf( 1.0f - nq.w * nq.w );
    if( s < WORKPHONE_EPSILON_F )
    {
        /* If s is close to zero, direction of axis is not important */
        if( out_axis )
        {
            out_axis->x = 1.0f;
            out_axis->y = 0.0f;
            out_axis->z = 0.0f;
        }
    }
    else
    {
        if( out_axis )
        {
            out_axis->x = nq.x / s;
            out_axis->y = nq.y / s;
            out_axis->z = nq.z / s;
        }
    }
    if( out_angle )
        *out_angle = angle;
}

wp_quatf wp_quatf_slerp( wp_quatf a, wp_quatf b, wp_f32 t )
{
    wp_f32 cosTheta = wp_quatf_dot( a, b );
    /* If cosTheta < 0, use -b to take shortest path */
    if( cosTheta < 0.0f )
    {
        b.w = -b.w;
        b.x = -b.x;
        b.y = -b.y;
        b.z = -b.z;
        cosTheta = -cosTheta;
    }

    if( cosTheta > 0.9995f )
    {
        /* Linear interpolation for very close quaternions */
        wp_quatf res;
        res.w = a.w + t * ( b.w - a.w );
        res.x = a.x + t * ( b.x - a.x );
        res.y = a.y + t * ( b.y - a.y );
        res.z = a.z + t * ( b.z - a.z );
        return wp_quatf_normalize( res );
    }

    wp_f32 theta = wp_acosf( cosTheta );
    wp_f32 sinTheta = wp_sinf( theta );
    wp_f32 w1 = wp_sinf( ( 1.0f - t ) * theta ) / sinTheta;
    wp_f32 w2 = wp_sinf( t * theta ) / sinTheta;

    wp_quatf res;
    res.w = w1 * a.w + w2 * b.w;
    res.x = w1 * a.x + w2 * b.x;
    res.y = w1 * a.y + w2 * b.y;
    res.z = w1 * a.z + w2 * b.z;
    return res;
}

/* -------------------------------------------------------------------------
 * wp_quatd (wp_f64) helpers - mirrored implementations
 * ---------------------------------------------------------------------- */

wp_quatd wp_quatd_make( wp_f64 w, wp_f64 x, wp_f64 y, wp_f64 z )
{
    wp_quatd q;
    q.w = w;
    q.x = x;
    q.y = y;
    q.z = z;
    return q;
}

wp_quatd wp_quatd_identity( void )
{
    return wp_quatd_make( 1.0, 0.0, 0.0, 0.0 );
}

wp_f64 wp_quatd_dot( wp_quatd a, wp_quatd b )
{
    return a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
}

wp_f64 wp_quatd_length( wp_quatd q )
{
    return wp_sqrtd( wp_quatd_dot( q, q ) );
}

wp_quatd wp_quatd_normalize( wp_quatd q )
{
    wp_f64 len = wp_quatd_length( q );
    if( len > WORKPHONE_EPSILON_D )
    {
        wp_f64 inv = 1.0 / len;
        return wp_quatd_make( q.w * inv, q.x * inv, q.y * inv, q.z * inv );
    }
    return q;
}

wp_quatd wp_quatd_conjugate( wp_quatd q )
{
    return wp_quatd_make( q.w, -q.x, -q.y, -q.z );
}

wp_quatd wp_quatd_mul( wp_quatd a, wp_quatd b )
{
    wp_quatd r;
    r.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    r.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    r.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    r.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    return r;
}

wp_vec3d wp_quatd_rotate_vec3( wp_quatd q, wp_vec3d v )
{
    wp_quatd p = wp_quatd_make( 0.0, v.x, v.y, v.z );
    wp_quatd qn = wp_quatd_conjugate( q );
    wp_quatd tmp = wp_quatd_mul( q, p );
    wp_quatd res = wp_quatd_mul( tmp, qn );
    wp_vec3d out;
    out.x = res.x;
    out.y = res.y;
    out.z = res.z;
    return out;
}

wp_quatd wp_quatd_from_axis_angle( wp_vec3d axis, wp_f64 angle_rad )
{
    wp_vec3d n = wp_vec3d_normalize( axis );
    wp_f64 half = 0.5 * angle_rad;
    wp_f64 s = wp_sind( half );
    return wp_quatd_make( wp_cosd( half ), n.x * s, n.y * s, n.z * s );
}

void wp_quatd_to_axis_angle( wp_quatd q, wp_vec3d *out_axis, wp_f64 *out_angle )
{
    wp_quatd nq = wp_quatd_normalize( q );
    wp_f64 angle = 2.0 * wp_acosd( nq.w );
    wp_f64 s = wp_sqrtd( 1.0 - nq.w * nq.w );
    if( s < WORKPHONE_EPSILON_D )
    {
        if( out_axis )
        {
            out_axis->x = 1.0;
            out_axis->y = 0.0;
            out_axis->z = 0.0;
        }
    }
    else
    {
        if( out_axis )
        {
            out_axis->x = nq.x / s;
            out_axis->y = nq.y / s;
            out_axis->z = nq.z / s;
        }
    }
    if( out_angle )
        *out_angle = angle;
}

wp_quatd wp_quatd_slerp( wp_quatd a, wp_quatd b, wp_f64 t )
{
    wp_f64 cosTheta = wp_quatd_dot( a, b );
    if( cosTheta < 0.0 )
    {
        b.w = -b.w;
        b.x = -b.x;
        b.y = -b.y;
        b.z = -b.z;
        cosTheta = -cosTheta;
    }

    if( cosTheta > 0.999999 )
    {
        wp_quatd res;
        res.w = a.w + t * ( b.w - a.w );
        res.x = a.x + t * ( b.x - a.x );
        res.y = a.y + t * ( b.y - a.y );
        res.z = a.z + t * ( b.z - a.z );
        return wp_quatd_normalize( res );
    }

    wp_f64 theta = wp_acosd( cosTheta );
    wp_f64 sinTheta = wp_sind( theta );
    wp_f64 w1 = wp_sind( ( 1.0 - t ) * theta ) / sinTheta;
    wp_f64 w2 = wp_sind( t * theta ) / sinTheta;

    wp_quatd res;
    res.w = w1 * a.w + w2 * b.w;
    res.x = w1 * a.x + w2 * b.x;
    res.y = w1 * a.y + w2 * b.y;
    res.z = w1 * a.z + w2 * b.z;
    return res;
}

#ifdef __cplusplus
}
#endif
