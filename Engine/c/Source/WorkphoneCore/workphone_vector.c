/**
 * @file wp_vector.c
 * @brief Implementation of the C vector math API.
 */

#include "workphone_vector.h"

#define WORKPHONE_EPSILON_F 1e-7f
#define WORKPHONE_EPSILON_D 1e-7

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static wp_f32 wp_minf( wp_f32 a, wp_f32 b )
{
    return a < b ? a : b;
}
static wp_f32 wp_maxf( wp_f32 a, wp_f32 b )
{
    return a > b ? a : b;
}
static wp_f64 wp_mind( wp_f64 a, wp_f64 b )
{
    return a < b ? a : b;
}
static wp_f64 wp_maxd( wp_f64 a, wp_f64 b )
{
    return a > b ? a : b;
}

/* =========================================================================
 * wp_vec2f
 * ====================================================================== */

wp_vec2f wp_vec2f_make( wp_f32 x, wp_f32 y )
{
    wp_vec2f v;
    v.x = x;
    v.y = y;
    return v;
}

wp_vec2f wp_vec2f_negate( wp_vec2f a )
{
    wp_vec2f v;
    v.x = -a.x;
    v.y = -a.y;
    return v;
}

wp_vec2f wp_vec2f_add( wp_vec2f a, wp_vec2f b )
{
    wp_vec2f v;
    v.x = a.x + b.x;
    v.y = a.y + b.y;
    return v;
}

wp_vec2f wp_vec2f_sub( wp_vec2f a, wp_vec2f b )
{
    wp_vec2f v;
    v.x = a.x - b.x;
    v.y = a.y - b.y;
    return v;
}

wp_vec2f wp_vec2f_mul( wp_vec2f a, wp_vec2f b )
{
    wp_vec2f v;
    v.x = a.x * b.x;
    v.y = a.y * b.y;
    return v;
}

wp_vec2f wp_vec2f_scale( wp_vec2f a, wp_f32 s )
{
    wp_vec2f v;
    v.x = a.x * s;
    v.y = a.y * s;
    return v;
}

wp_vec2f wp_vec2f_div( wp_vec2f a, wp_vec2f b )
{
    wp_vec2f v;
    v.x = a.x / b.x;
    v.y = a.y / b.y;
    return v;
}

wp_vec2f wp_vec2f_divf( wp_vec2f a, wp_f32 s )
{
    wp_vec2f v;
    wp_f32 inv = 1.0f / s;
    v.x = a.x * inv;
    v.y = a.y * inv;
    return v;
}

wp_vec2f wp_vec2f_normalize( wp_vec2f a )
{
    wp_f32 len = wp_vec2f_length( a );
    if( len > WORKPHONE_EPSILON_F )
        return wp_vec2f_divf( a, len );
    return a;
}

wp_vec2f wp_vec2f_lerp( wp_vec2f a, wp_vec2f b, wp_f32 t )
{
    wp_vec2f v;
    wp_f32 inv = 1.0f - t;
    v.x = a.x * inv + b.x * t;
    v.y = a.y * inv + b.y * t;
    return v;
}

wp_vec2f wp_vec2f_min( wp_vec2f a, wp_vec2f b )
{
    wp_vec2f v;
    v.x = wp_minf( a.x, b.x );
    v.y = wp_minf( a.y, b.y );
    return v;
}

wp_vec2f wp_vec2f_max( wp_vec2f a, wp_vec2f b )
{
    wp_vec2f v;
    v.x = wp_maxf( a.x, b.x );
    v.y = wp_maxf( a.y, b.y );
    return v;
}

wp_vec2f wp_vec2f_abs( wp_vec2f a )
{
    wp_vec2f v;
    v.x = fabsf( a.x );
    v.y = fabsf( a.y );
    return v;
}

wp_f32 wp_vec2f_dot( wp_vec2f a, wp_vec2f b )
{
    return a.x * b.x + a.y * b.y;
}

wp_f32 wp_vec2f_length_sq( wp_vec2f a )
{
    return a.x * a.x + a.y * a.y;
}

wp_f32 wp_vec2f_length( wp_vec2f a )
{
    return sqrtf( wp_vec2f_length_sq( a ) );
}

wp_f32 wp_vec2f_distance_sq( wp_vec2f a, wp_vec2f b )
{
    return wp_vec2f_length_sq( wp_vec2f_sub( a, b ) );
}

wp_f32 wp_vec2f_distance( wp_vec2f a, wp_vec2f b )
{
    return sqrtf( wp_vec2f_distance_sq( a, b ) );
}

wp_s32 wp_vec2f_equals( wp_vec2f a, wp_vec2f b )
{
    return fabsf( a.x - b.x ) <= WORKPHONE_EPSILON_F && fabsf( a.y - b.y ) <= WORKPHONE_EPSILON_F;
}

wp_s32 wp_vec2f_is_zero_length( wp_vec2f a )
{
    return wp_vec2f_length_sq( a ) <= WORKPHONE_EPSILON_F * WORKPHONE_EPSILON_F;
}

/* =========================================================================
 * wp_vec3f
 * ====================================================================== */

wp_vec3f wp_vec3f_make( wp_f32 x, wp_f32 y, wp_f32 z )
{
    wp_vec3f v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

wp_vec3f wp_vec3f_negate( wp_vec3f a )
{
    wp_vec3f v;
    v.x = -a.x;
    v.y = -a.y;
    v.z = -a.z;
    return v;
}

wp_vec3f wp_vec3f_add( wp_vec3f a, wp_vec3f b )
{
    wp_vec3f v;
    v.x = a.x + b.x;
    v.y = a.y + b.y;
    v.z = a.z + b.z;
    return v;
}

wp_vec3f wp_vec3f_sub( wp_vec3f a, wp_vec3f b )
{
    wp_vec3f v;
    v.x = a.x - b.x;
    v.y = a.y - b.y;
    v.z = a.z - b.z;
    return v;
}

wp_vec3f wp_vec3f_mul( wp_vec3f a, wp_vec3f b )
{
    wp_vec3f v;
    v.x = a.x * b.x;
    v.y = a.y * b.y;
    v.z = a.z * b.z;
    return v;
}

wp_vec3f wp_vec3f_scale( wp_vec3f a, wp_f32 s )
{
    wp_vec3f v;
    v.x = a.x * s;
    v.y = a.y * s;
    v.z = a.z * s;
    return v;
}

wp_vec3f wp_vec3f_div( wp_vec3f a, wp_vec3f b )
{
    wp_vec3f v;
    v.x = a.x / b.x;
    v.y = a.y / b.y;
    v.z = a.z / b.z;
    return v;
}

wp_vec3f wp_vec3f_divf( wp_vec3f a, wp_f32 s )
{
    wp_vec3f v;
    wp_f32 inv = 1.0f / s;
    v.x = a.x * inv;
    v.y = a.y * inv;
    v.z = a.z * inv;
    return v;
}

wp_vec3f wp_vec3f_cross( wp_vec3f a, wp_vec3f b )
{
    wp_vec3f v;
    v.x = a.y * b.z - a.z * b.y;
    v.y = a.z * b.x - a.x * b.z;
    v.z = a.x * b.y - a.y * b.x;
    return v;
}

wp_vec3f wp_vec3f_normalize( wp_vec3f a )
{
    wp_f32 len = wp_vec3f_length( a );
    if( len > WORKPHONE_EPSILON_F )
        return wp_vec3f_divf( a, len );
    return a;
}

wp_vec3f wp_vec3f_lerp( wp_vec3f a, wp_vec3f b, wp_f32 t )
{
    wp_vec3f v;
    wp_f32 inv = 1.0f - t;
    v.x = a.x * inv + b.x * t;
    v.y = a.y * inv + b.y * t;
    v.z = a.z * inv + b.z * t;
    return v;
}

wp_vec3f wp_vec3f_lerp_quadratic( wp_vec3f a, wp_vec3f b, wp_vec3f c, wp_f32 t )
{
    return wp_vec3f_lerp( wp_vec3f_lerp( a, b, t ), wp_vec3f_lerp( b, c, t ), t );
}

wp_vec3f wp_vec3f_min( wp_vec3f a, wp_vec3f b )
{
    wp_vec3f v;
    v.x = wp_minf( a.x, b.x );
    v.y = wp_minf( a.y, b.y );
    v.z = wp_minf( a.z, b.z );
    return v;
}

wp_vec3f wp_vec3f_max( wp_vec3f a, wp_vec3f b )
{
    wp_vec3f v;
    v.x = wp_maxf( a.x, b.x );
    v.y = wp_maxf( a.y, b.y );
    v.z = wp_maxf( a.z, b.z );
    return v;
}

wp_vec3f wp_vec3f_abs( wp_vec3f a )
{
    wp_vec3f v;
    v.x = fabsf( a.x );
    v.y = fabsf( a.y );
    v.z = fabsf( a.z );
    return v;
}

wp_vec3f wp_vec3f_mid( wp_vec3f a, wp_vec3f b )
{
    return wp_vec3f_scale( wp_vec3f_add( a, b ), 0.5f );
}

wp_vec3f wp_vec3f_perpendicular( wp_vec3f a )
{
    wp_vec3f perp;
    wp_f32 len_sq = wp_vec3f_length_sq( a );
    if( len_sq <= WORKPHONE_EPSILON_F )
    {
        perp.x = 0.0f;
        perp.y = 0.0f;
        perp.z = 0.0f;
        return perp;
    }
    if( fabsf( a.x ) < fabsf( a.y ) )
    {
        perp.x = 0.0f;
        perp.y = -a.z;
        perp.z = a.y;
    }
    else
    {
        perp.x = -a.z;
        perp.y = 0.0f;
        perp.z = a.x;
    }
    return wp_vec3f_normalize( perp );
}

wp_f32 wp_vec3f_dot( wp_vec3f a, wp_vec3f b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

wp_f32 wp_vec3f_dot_abs( wp_vec3f a, wp_vec3f b )
{
    return fabsf( wp_vec3f_dot( a, b ) );
}

wp_f32 wp_vec3f_length_sq( wp_vec3f a )
{
    return a.x * a.x + a.y * a.y + a.z * a.z;
}

wp_f32 wp_vec3f_length( wp_vec3f a )
{
    return sqrtf( wp_vec3f_length_sq( a ) );
}

wp_f32 wp_vec3f_distance_sq( wp_vec3f a, wp_vec3f b )
{
    return wp_vec3f_length_sq( wp_vec3f_sub( a, b ) );
}

wp_f32 wp_vec3f_distance( wp_vec3f a, wp_vec3f b )
{
    return sqrtf( wp_vec3f_distance_sq( a, b ) );
}

wp_s32 wp_vec3f_equals( wp_vec3f a, wp_vec3f b )
{
    return fabsf( a.x - b.x ) <= WORKPHONE_EPSILON_F && fabsf( a.y - b.y ) <= WORKPHONE_EPSILON_F &&
           fabsf( a.z - b.z ) <= WORKPHONE_EPSILON_F;
}

wp_s32 wp_vec3f_equals_tol( wp_vec3f a, wp_vec3f b, wp_f32 tolerance )
{
    return fabsf( a.x - b.x ) <= tolerance && fabsf( a.y - b.y ) <= tolerance &&
           fabsf( a.z - b.z ) <= tolerance;
}

wp_s32 wp_vec3f_is_zero_length( wp_vec3f a )
{
    return wp_vec3f_length_sq( a ) <= WORKPHONE_EPSILON_F * WORKPHONE_EPSILON_F;
}

wp_s32 wp_vec3f_is_between_points( wp_vec3f p, wp_vec3f begin, wp_vec3f end )
{
    wp_f32 d = wp_vec3f_distance_sq( begin, end );
    return wp_vec3f_distance_sq( p, begin ) <= d && wp_vec3f_distance_sq( p, end ) <= d;
}

/* =========================================================================
 * wp_vec4f
 * ====================================================================== */

wp_vec4f wp_vec4f_make( wp_f32 x, wp_f32 y, wp_f32 z, wp_f32 w )
{
    wp_vec4f v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;
    return v;
}

wp_vec4f wp_vec4f_negate( wp_vec4f a )
{
    wp_vec4f v;
    v.x = -a.x;
    v.y = -a.y;
    v.z = -a.z;
    v.w = -a.w;
    return v;
}

wp_vec4f wp_vec4f_add( wp_vec4f a, wp_vec4f b )
{
    wp_vec4f v;
    v.x = a.x + b.x;
    v.y = a.y + b.y;
    v.z = a.z + b.z;
    v.w = a.w + b.w;
    return v;
}

wp_vec4f wp_vec4f_sub( wp_vec4f a, wp_vec4f b )
{
    wp_vec4f v;
    v.x = a.x - b.x;
    v.y = a.y - b.y;
    v.z = a.z - b.z;
    v.w = a.w - b.w;
    return v;
}

wp_vec4f wp_vec4f_mul( wp_vec4f a, wp_vec4f b )
{
    wp_vec4f v;
    v.x = a.x * b.x;
    v.y = a.y * b.y;
    v.z = a.z * b.z;
    v.w = a.w * b.w;
    return v;
}

wp_vec4f wp_vec4f_scale( wp_vec4f a, wp_f32 s )
{
    wp_vec4f v;
    v.x = a.x * s;
    v.y = a.y * s;
    v.z = a.z * s;
    v.w = a.w * s;
    return v;
}

wp_vec4f wp_vec4f_div( wp_vec4f a, wp_vec4f b )
{
    wp_vec4f v;
    v.x = a.x / b.x;
    v.y = a.y / b.y;
    v.z = a.z / b.z;
    v.w = a.w / b.w;
    return v;
}

wp_vec4f wp_vec4f_divf( wp_vec4f a, wp_f32 s )
{
    wp_vec4f v;
    wp_f32 inv = 1.0f / s;
    v.x = a.x * inv;
    v.y = a.y * inv;
    v.z = a.z * inv;
    v.w = a.w * inv;
    return v;
}

wp_vec4f wp_vec4f_normalize( wp_vec4f a )
{
    wp_f32 len = wp_vec4f_length( a );
    if( len > WORKPHONE_EPSILON_F )
        return wp_vec4f_divf( a, len );
    return a;
}

wp_vec4f wp_vec4f_lerp( wp_vec4f a, wp_vec4f b, wp_f32 t )
{
    wp_vec4f v;
    wp_f32 inv = 1.0f - t;
    v.x = a.x * inv + b.x * t;
    v.y = a.y * inv + b.y * t;
    v.z = a.z * inv + b.z * t;
    v.w = a.w * inv + b.w * t;
    return v;
}

wp_vec4f wp_vec4f_min( wp_vec4f a, wp_vec4f b )
{
    wp_vec4f v;
    v.x = wp_minf( a.x, b.x );
    v.y = wp_minf( a.y, b.y );
    v.z = wp_minf( a.z, b.z );
    v.w = wp_minf( a.w, b.w );
    return v;
}

wp_vec4f wp_vec4f_max( wp_vec4f a, wp_vec4f b )
{
    wp_vec4f v;
    v.x = wp_maxf( a.x, b.x );
    v.y = wp_maxf( a.y, b.y );
    v.z = wp_maxf( a.z, b.z );
    v.w = wp_maxf( a.w, b.w );
    return v;
}

wp_vec4f wp_vec4f_abs( wp_vec4f a )
{
    wp_vec4f v;
    v.x = fabsf( a.x );
    v.y = fabsf( a.y );
    v.z = fabsf( a.z );
    v.w = fabsf( a.w );
    return v;
}

wp_f32 wp_vec4f_dot( wp_vec4f a, wp_vec4f b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

wp_f32 wp_vec4f_length_sq( wp_vec4f a )
{
    return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
}

wp_f32 wp_vec4f_length( wp_vec4f a )
{
    return sqrtf( wp_vec4f_length_sq( a ) );
}

wp_f32 wp_vec4f_distance_sq( wp_vec4f a, wp_vec4f b )
{
    return wp_vec4f_length_sq( wp_vec4f_sub( a, b ) );
}

wp_f32 wp_vec4f_distance( wp_vec4f a, wp_vec4f b )
{
    return sqrtf( wp_vec4f_distance_sq( a, b ) );
}

wp_s32 wp_vec4f_equals( wp_vec4f a, wp_vec4f b )
{
    return fabsf( a.x - b.x ) <= WORKPHONE_EPSILON_F && fabsf( a.y - b.y ) <= WORKPHONE_EPSILON_F &&
           fabsf( a.z - b.z ) <= WORKPHONE_EPSILON_F && fabsf( a.w - b.w ) <= WORKPHONE_EPSILON_F;
}

wp_s32 wp_vec4f_is_zero_length( wp_vec4f a )
{
    return wp_vec4f_length_sq( a ) <= WORKPHONE_EPSILON_F * WORKPHONE_EPSILON_F;
}

/* =========================================================================
 * wp_vec2d
 * ====================================================================== */

wp_vec2d wp_vec2d_make( wp_f64 x, wp_f64 y )
{
    wp_vec2d v;
    v.x = x;
    v.y = y;
    return v;
}

wp_vec2d wp_vec2d_negate( wp_vec2d a )
{
    wp_vec2d v;
    v.x = -a.x;
    v.y = -a.y;
    return v;
}

wp_vec2d wp_vec2d_add( wp_vec2d a, wp_vec2d b )
{
    wp_vec2d v;
    v.x = a.x + b.x;
    v.y = a.y + b.y;
    return v;
}

wp_vec2d wp_vec2d_sub( wp_vec2d a, wp_vec2d b )
{
    wp_vec2d v;
    v.x = a.x - b.x;
    v.y = a.y - b.y;
    return v;
}

wp_vec2d wp_vec2d_mul( wp_vec2d a, wp_vec2d b )
{
    wp_vec2d v;
    v.x = a.x * b.x;
    v.y = a.y * b.y;
    return v;
}

wp_vec2d wp_vec2d_scale( wp_vec2d a, wp_f64 s )
{
    wp_vec2d v;
    v.x = a.x * s;
    v.y = a.y * s;
    return v;
}

wp_vec2d wp_vec2d_div( wp_vec2d a, wp_vec2d b )
{
    wp_vec2d v;
    v.x = a.x / b.x;
    v.y = a.y / b.y;
    return v;
}

wp_vec2d wp_vec2d_divd( wp_vec2d a, wp_f64 s )
{
    wp_vec2d v;
    wp_f64 inv = 1.0 / s;
    v.x = a.x * inv;
    v.y = a.y * inv;
    return v;
}

wp_vec2d wp_vec2d_normalize( wp_vec2d a )
{
    wp_f64 len = wp_vec2d_length( a );
    if( len > WORKPHONE_EPSILON_D )
        return wp_vec2d_divd( a, len );
    return a;
}

wp_vec2d wp_vec2d_lerp( wp_vec2d a, wp_vec2d b, wp_f64 t )
{
    wp_vec2d v;
    wp_f64 inv = 1.0 - t;
    v.x = a.x * inv + b.x * t;
    v.y = a.y * inv + b.y * t;
    return v;
}

wp_vec2d wp_vec2d_min( wp_vec2d a, wp_vec2d b )
{
    wp_vec2d v;
    v.x = wp_mind( a.x, b.x );
    v.y = wp_mind( a.y, b.y );
    return v;
}

wp_vec2d wp_vec2d_max( wp_vec2d a, wp_vec2d b )
{
    wp_vec2d v;
    v.x = wp_maxd( a.x, b.x );
    v.y = wp_maxd( a.y, b.y );
    return v;
}

wp_vec2d wp_vec2d_abs( wp_vec2d a )
{
    wp_vec2d v;
    v.x = fabs( a.x );
    v.y = fabs( a.y );
    return v;
}

wp_f64 wp_vec2d_dot( wp_vec2d a, wp_vec2d b )
{
    return a.x * b.x + a.y * b.y;
}

wp_f64 wp_vec2d_length_sq( wp_vec2d a )
{
    return a.x * a.x + a.y * a.y;
}

wp_f64 wp_vec2d_length( wp_vec2d a )
{
    return sqrt( wp_vec2d_length_sq( a ) );
}

wp_f64 wp_vec2d_distance_sq( wp_vec2d a, wp_vec2d b )
{
    return wp_vec2d_length_sq( wp_vec2d_sub( a, b ) );
}

wp_f64 wp_vec2d_distance( wp_vec2d a, wp_vec2d b )
{
    return sqrt( wp_vec2d_distance_sq( a, b ) );
}

wp_s32 wp_vec2d_equals( wp_vec2d a, wp_vec2d b )
{
    return fabs( a.x - b.x ) <= WORKPHONE_EPSILON_D && fabs( a.y - b.y ) <= WORKPHONE_EPSILON_D;
}

wp_s32 wp_vec2d_is_zero_length( wp_vec2d a )
{
    return wp_vec2d_length_sq( a ) <= WORKPHONE_EPSILON_D * WORKPHONE_EPSILON_D;
}

/* =========================================================================
 * wp_vec3d
 * ====================================================================== */

wp_vec3d wp_vec3d_make( wp_f64 x, wp_f64 y, wp_f64 z )
{
    wp_vec3d v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

wp_vec3d wp_vec3d_negate( wp_vec3d a )
{
    wp_vec3d v;
    v.x = -a.x;
    v.y = -a.y;
    v.z = -a.z;
    return v;
}

wp_vec3d wp_vec3d_add( wp_vec3d a, wp_vec3d b )
{
    wp_vec3d v;
    v.x = a.x + b.x;
    v.y = a.y + b.y;
    v.z = a.z + b.z;
    return v;
}

wp_vec3d wp_vec3d_sub( wp_vec3d a, wp_vec3d b )
{
    wp_vec3d v;
    v.x = a.x - b.x;
    v.y = a.y - b.y;
    v.z = a.z - b.z;
    return v;
}

wp_vec3d wp_vec3d_mul( wp_vec3d a, wp_vec3d b )
{
    wp_vec3d v;
    v.x = a.x * b.x;
    v.y = a.y * b.y;
    v.z = a.z * b.z;
    return v;
}

wp_vec3d wp_vec3d_scale( wp_vec3d a, wp_f64 s )
{
    wp_vec3d v;
    v.x = a.x * s;
    v.y = a.y * s;
    v.z = a.z * s;
    return v;
}

wp_vec3d wp_vec3d_div( wp_vec3d a, wp_vec3d b )
{
    wp_vec3d v;
    v.x = a.x / b.x;
    v.y = a.y / b.y;
    v.z = a.z / b.z;
    return v;
}

wp_vec3d wp_vec3d_divd( wp_vec3d a, wp_f64 s )
{
    wp_vec3d v;
    wp_f64 inv = 1.0 / s;
    v.x = a.x * inv;
    v.y = a.y * inv;
    v.z = a.z * inv;
    return v;
}

wp_vec3d wp_vec3d_cross( wp_vec3d a, wp_vec3d b )
{
    wp_vec3d v;
    v.x = a.y * b.z - a.z * b.y;
    v.y = a.z * b.x - a.x * b.z;
    v.z = a.x * b.y - a.y * b.x;
    return v;
}

wp_vec3d wp_vec3d_normalize( wp_vec3d a )
{
    wp_f64 len = wp_vec3d_length( a );
    if( len > WORKPHONE_EPSILON_D )
        return wp_vec3d_divd( a, len );
    return a;
}

wp_vec3d wp_vec3d_lerp( wp_vec3d a, wp_vec3d b, wp_f64 t )
{
    wp_vec3d v;
    wp_f64 inv = 1.0 - t;
    v.x = a.x * inv + b.x * t;
    v.y = a.y * inv + b.y * t;
    v.z = a.z * inv + b.z * t;
    return v;
}

wp_vec3d wp_vec3d_lerp_quadratic( wp_vec3d a, wp_vec3d b, wp_vec3d c, wp_f64 t )
{
    return wp_vec3d_lerp( wp_vec3d_lerp( a, b, t ), wp_vec3d_lerp( b, c, t ), t );
}

wp_vec3d wp_vec3d_min( wp_vec3d a, wp_vec3d b )
{
    wp_vec3d v;
    v.x = wp_mind( a.x, b.x );
    v.y = wp_mind( a.y, b.y );
    v.z = wp_mind( a.z, b.z );
    return v;
}

wp_vec3d wp_vec3d_max( wp_vec3d a, wp_vec3d b )
{
    wp_vec3d v;
    v.x = wp_maxd( a.x, b.x );
    v.y = wp_maxd( a.y, b.y );
    v.z = wp_maxd( a.z, b.z );
    return v;
}

wp_vec3d wp_vec3d_abs( wp_vec3d a )
{
    wp_vec3d v;
    v.x = fabs( a.x );
    v.y = fabs( a.y );
    v.z = fabs( a.z );
    return v;
}

wp_vec3d wp_vec3d_mid( wp_vec3d a, wp_vec3d b )
{
    return wp_vec3d_scale( wp_vec3d_add( a, b ), 0.5 );
}

wp_vec3d wp_vec3d_perpendicular( wp_vec3d a )
{
    wp_vec3d perp;
    wp_f64 len_sq = wp_vec3d_length_sq( a );
    if( len_sq <= WORKPHONE_EPSILON_D )
    {
        perp.x = 0.0;
        perp.y = 0.0;
        perp.z = 0.0;
        return perp;
    }
    if( fabs( a.x ) < fabs( a.y ) )
    {
        perp.x = 0.0;
        perp.y = -a.z;
        perp.z = a.y;
    }
    else
    {
        perp.x = -a.z;
        perp.y = 0.0;
        perp.z = a.x;
    }
    return wp_vec3d_normalize( perp );
}

wp_f64 wp_vec3d_dot( wp_vec3d a, wp_vec3d b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

wp_f64 wp_vec3d_dot_abs( wp_vec3d a, wp_vec3d b )
{
    return fabs( wp_vec3d_dot( a, b ) );
}

wp_f64 wp_vec3d_length_sq( wp_vec3d a )
{
    return a.x * a.x + a.y * a.y + a.z * a.z;
}

wp_f64 wp_vec3d_length( wp_vec3d a )
{
    return sqrt( wp_vec3d_length_sq( a ) );
}

wp_f64 wp_vec3d_distance_sq( wp_vec3d a, wp_vec3d b )
{
    return wp_vec3d_length_sq( wp_vec3d_sub( a, b ) );
}

wp_f64 wp_vec3d_distance( wp_vec3d a, wp_vec3d b )
{
    return sqrt( wp_vec3d_distance_sq( a, b ) );
}

wp_s32 wp_vec3d_equals( wp_vec3d a, wp_vec3d b )
{
    return fabs( a.x - b.x ) <= WORKPHONE_EPSILON_D && fabs( a.y - b.y ) <= WORKPHONE_EPSILON_D &&
           fabs( a.z - b.z ) <= WORKPHONE_EPSILON_D;
}

wp_s32 wp_vec3d_equals_tol( wp_vec3d a, wp_vec3d b, wp_f64 tolerance )
{
    return fabs( a.x - b.x ) <= tolerance && fabs( a.y - b.y ) <= tolerance &&
           fabs( a.z - b.z ) <= tolerance;
}

wp_s32 wp_vec3d_is_zero_length( wp_vec3d a )
{
    return wp_vec3d_length_sq( a ) <= WORKPHONE_EPSILON_D * WORKPHONE_EPSILON_D;
}

wp_s32 wp_vec3d_is_between_points( wp_vec3d p, wp_vec3d begin, wp_vec3d end )
{
    wp_f64 d = wp_vec3d_distance_sq( begin, end );
    return wp_vec3d_distance_sq( p, begin ) <= d && wp_vec3d_distance_sq( p, end ) <= d;
}

/* =========================================================================
 * wp_vec4d
 * ====================================================================== */

wp_vec4d wp_vec4d_make( wp_f64 x, wp_f64 y, wp_f64 z, wp_f64 w )
{
    wp_vec4d v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;
    return v;
}

wp_vec4d wp_vec4d_negate( wp_vec4d a )
{
    wp_vec4d v;
    v.x = -a.x;
    v.y = -a.y;
    v.z = -a.z;
    v.w = -a.w;
    return v;
}

wp_vec4d wp_vec4d_add( wp_vec4d a, wp_vec4d b )
{
    wp_vec4d v;
    v.x = a.x + b.x;
    v.y = a.y + b.y;
    v.z = a.z + b.z;
    v.w = a.w + b.w;
    return v;
}

wp_vec4d wp_vec4d_sub( wp_vec4d a, wp_vec4d b )
{
    wp_vec4d v;
    v.x = a.x - b.x;
    v.y = a.y - b.y;
    v.z = a.z - b.z;
    v.w = a.w - b.w;
    return v;
}

wp_vec4d wp_vec4d_mul( wp_vec4d a, wp_vec4d b )
{
    wp_vec4d v;
    v.x = a.x * b.x;
    v.y = a.y * b.y;
    v.z = a.z * b.z;
    v.w = a.w * b.w;
    return v;
}

wp_vec4d wp_vec4d_scale( wp_vec4d a, wp_f64 s )
{
    wp_vec4d v;
    v.x = a.x * s;
    v.y = a.y * s;
    v.z = a.z * s;
    v.w = a.w * s;
    return v;
}

wp_vec4d wp_vec4d_div( wp_vec4d a, wp_vec4d b )
{
    wp_vec4d v;
    v.x = a.x / b.x;
    v.y = a.y / b.y;
    v.z = a.z / b.z;
    v.w = a.w / b.w;
    return v;
}

wp_vec4d wp_vec4d_divd( wp_vec4d a, wp_f64 s )
{
    wp_vec4d v;
    wp_f64 inv = 1.0 / s;
    v.x = a.x * inv;
    v.y = a.y * inv;
    v.z = a.z * inv;
    v.w = a.w * inv;
    return v;
}

wp_vec4d wp_vec4d_normalize( wp_vec4d a )
{
    wp_f64 len = wp_vec4d_length( a );
    if( len > WORKPHONE_EPSILON_D )
        return wp_vec4d_divd( a, len );
    return a;
}

wp_vec4d wp_vec4d_lerp( wp_vec4d a, wp_vec4d b, wp_f64 t )
{
    wp_vec4d v;
    wp_f64 inv = 1.0 - t;
    v.x = a.x * inv + b.x * t;
    v.y = a.y * inv + b.y * t;
    v.z = a.z * inv + b.z * t;
    v.w = a.w * inv + b.w * t;
    return v;
}

wp_vec4d wp_vec4d_min( wp_vec4d a, wp_vec4d b )
{
    wp_vec4d v;
    v.x = wp_mind( a.x, b.x );
    v.y = wp_mind( a.y, b.y );
    v.z = wp_mind( a.z, b.z );
    v.w = wp_mind( a.w, b.w );
    return v;
}

wp_vec4d wp_vec4d_max( wp_vec4d a, wp_vec4d b )
{
    wp_vec4d v;
    v.x = wp_maxd( a.x, b.x );
    v.y = wp_maxd( a.y, b.y );
    v.z = wp_maxd( a.z, b.z );
    v.w = wp_maxd( a.w, b.w );
    return v;
}

wp_vec4d wp_vec4d_abs( wp_vec4d a )
{
    wp_vec4d v;
    v.x = fabs( a.x );
    v.y = fabs( a.y );
    v.z = fabs( a.z );
    v.w = fabs( a.w );
    return v;
}

wp_f64 wp_vec4d_dot( wp_vec4d a, wp_vec4d b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

wp_f64 wp_vec4d_length_sq( wp_vec4d a )
{
    return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
}

wp_f64 wp_vec4d_length( wp_vec4d a )
{
    return sqrt( wp_vec4d_length_sq( a ) );
}

wp_f64 wp_vec4d_distance_sq( wp_vec4d a, wp_vec4d b )
{
    return wp_vec4d_length_sq( wp_vec4d_sub( a, b ) );
}

wp_f64 wp_vec4d_distance( wp_vec4d a, wp_vec4d b )
{
    return sqrt( wp_vec4d_distance_sq( a, b ) );
}

wp_s32 wp_vec4d_equals( wp_vec4d a, wp_vec4d b )
{
    return fabs( a.x - b.x ) <= WORKPHONE_EPSILON_D && fabs( a.y - b.y ) <= WORKPHONE_EPSILON_D &&
           fabs( a.z - b.z ) <= WORKPHONE_EPSILON_D && fabs( a.w - b.w ) <= WORKPHONE_EPSILON_D;
}

wp_s32 wp_vec4d_is_zero_length( wp_vec4d a )
{
    return wp_vec4d_length_sq( a ) <= WORKPHONE_EPSILON_D * WORKPHONE_EPSILON_D;
}
