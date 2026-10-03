/**
 * @file wp_vector.h
 * @brief C API for 2D, 3D, and 4D vector math (wp_f32 and wp_f64 variants).
 */

#ifndef WORKPHONE_VECTOR_H
#define WORKPHONE_VECTOR_H

#include "workphone_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wp_vec2i
{
    wp_s16 x, y;
} wp_vec2i;

typedef struct wp_vec2f
{
    wp_f32 x, y;
} wp_vec2f;

typedef struct wp_vec2d
{
    wp_f64 x, y;
} wp_vec2d;

typedef struct wp_vec3i
{
    wp_f32 x, y, z;
} wp_vec3i;

typedef struct wp_vec3f
{
    wp_f32 x, y, z;
} wp_vec3f;

typedef struct wp_vec3d
{
    wp_f64 x, y, z;
} wp_vec3d;

typedef struct wp_vec4i
{
    wp_s16 x, y, z, w;
} wp_vec4i;

typedef struct wp_vec4f
{
    wp_f32 x, y, z, w;
} wp_vec4f;

typedef struct wp_vec4d
{
    wp_f64 x, y, z, w;
} wp_vec4d;

wp_vec2f wp_vec2f_make( wp_f32 x, wp_f32 y );
wp_vec2f wp_vec2f_negate( wp_vec2f a );
wp_vec2f wp_vec2f_add( wp_vec2f a, wp_vec2f b );
wp_vec2f wp_vec2f_sub( wp_vec2f a, wp_vec2f b );
wp_vec2f wp_vec2f_mul( wp_vec2f a, wp_vec2f b );
wp_vec2f wp_vec2f_scale( wp_vec2f a, wp_f32 s );
wp_vec2f wp_vec2f_div( wp_vec2f a, wp_vec2f b );
wp_vec2f wp_vec2f_divf( wp_vec2f a, wp_f32 s );
wp_vec2f wp_vec2f_normalize( wp_vec2f a );
wp_vec2f wp_vec2f_lerp( wp_vec2f a, wp_vec2f b, wp_f32 t );
wp_vec2f wp_vec2f_min( wp_vec2f a, wp_vec2f b );
wp_vec2f wp_vec2f_max( wp_vec2f a, wp_vec2f b );
wp_vec2f wp_vec2f_abs( wp_vec2f a );
wp_f32 wp_vec2f_dot( wp_vec2f a, wp_vec2f b );
wp_f32 wp_vec2f_length( wp_vec2f a );
wp_f32 wp_vec2f_length_sq( wp_vec2f a );
wp_f32 wp_vec2f_distance( wp_vec2f a, wp_vec2f b );
wp_f32 wp_vec2f_distance_sq( wp_vec2f a, wp_vec2f b );
wp_s32 wp_vec2f_equals( wp_vec2f a, wp_vec2f b );
wp_s32 wp_vec2f_is_zero_length( wp_vec2f a );

wp_vec3f wp_vec3f_make( wp_f32 x, wp_f32 y, wp_f32 z );
wp_vec3f wp_vec3f_negate( wp_vec3f a );
wp_vec3f wp_vec3f_add( wp_vec3f a, wp_vec3f b );
wp_vec3f wp_vec3f_sub( wp_vec3f a, wp_vec3f b );
wp_vec3f wp_vec3f_mul( wp_vec3f a, wp_vec3f b );
wp_vec3f wp_vec3f_scale( wp_vec3f a, wp_f32 s );
wp_vec3f wp_vec3f_div( wp_vec3f a, wp_vec3f b );
wp_vec3f wp_vec3f_divf( wp_vec3f a, wp_f32 s );
wp_vec3f wp_vec3f_cross( wp_vec3f a, wp_vec3f b );
wp_vec3f wp_vec3f_normalize( wp_vec3f a );
wp_vec3f wp_vec3f_lerp( wp_vec3f a, wp_vec3f b, wp_f32 t );
wp_vec3f wp_vec3f_lerp_quadratic( wp_vec3f a, wp_vec3f b, wp_vec3f c, wp_f32 t );
wp_vec3f wp_vec3f_min( wp_vec3f a, wp_vec3f b );
wp_vec3f wp_vec3f_max( wp_vec3f a, wp_vec3f b );
wp_vec3f wp_vec3f_abs( wp_vec3f a );
wp_vec3f wp_vec3f_mid( wp_vec3f a, wp_vec3f b );
wp_vec3f wp_vec3f_perpendicular( wp_vec3f a );
wp_f32 wp_vec3f_dot( wp_vec3f a, wp_vec3f b );
wp_f32 wp_vec3f_dot_abs( wp_vec3f a, wp_vec3f b );
wp_f32 wp_vec3f_length( wp_vec3f a );
wp_f32 wp_vec3f_length_sq( wp_vec3f a );
wp_f32 wp_vec3f_distance( wp_vec3f a, wp_vec3f b );
wp_f32 wp_vec3f_distance_sq( wp_vec3f a, wp_vec3f b );
wp_s32 wp_vec3f_equals( wp_vec3f a, wp_vec3f b );
wp_s32 wp_vec3f_equals_tol( wp_vec3f a, wp_vec3f b, wp_f32 tolerance );
wp_s32 wp_vec3f_is_zero_length( wp_vec3f a );
wp_s32 wp_vec3f_is_between_points( wp_vec3f p, wp_vec3f begin, wp_vec3f end );

wp_vec4f wp_vec4f_make( wp_f32 x, wp_f32 y, wp_f32 z, wp_f32 w );
wp_vec4f wp_vec4f_negate( wp_vec4f a );
wp_vec4f wp_vec4f_add( wp_vec4f a, wp_vec4f b );
wp_vec4f wp_vec4f_sub( wp_vec4f a, wp_vec4f b );
wp_vec4f wp_vec4f_mul( wp_vec4f a, wp_vec4f b );
wp_vec4f wp_vec4f_scale( wp_vec4f a, wp_f32 s );
wp_vec4f wp_vec4f_div( wp_vec4f a, wp_vec4f b );
wp_vec4f wp_vec4f_divf( wp_vec4f a, wp_f32 s );
wp_vec4f wp_vec4f_normalize( wp_vec4f a );
wp_vec4f wp_vec4f_lerp( wp_vec4f a, wp_vec4f b, wp_f32 t );
wp_vec4f wp_vec4f_min( wp_vec4f a, wp_vec4f b );
wp_vec4f wp_vec4f_max( wp_vec4f a, wp_vec4f b );
wp_vec4f wp_vec4f_abs( wp_vec4f a );
wp_f32 wp_vec4f_dot( wp_vec4f a, wp_vec4f b );
wp_f32 wp_vec4f_length( wp_vec4f a );
wp_f32 wp_vec4f_length_sq( wp_vec4f a );
wp_f32 wp_vec4f_distance( wp_vec4f a, wp_vec4f b );
wp_f32 wp_vec4f_distance_sq( wp_vec4f a, wp_vec4f b );
wp_s32 wp_vec4f_equals( wp_vec4f a, wp_vec4f b );
wp_s32 wp_vec4f_is_zero_length( wp_vec4f a );

wp_vec2d wp_vec2d_make( wp_f64 x, wp_f64 y );
wp_vec2d wp_vec2d_negate( wp_vec2d a );
wp_vec2d wp_vec2d_add( wp_vec2d a, wp_vec2d b );
wp_vec2d wp_vec2d_sub( wp_vec2d a, wp_vec2d b );
wp_vec2d wp_vec2d_mul( wp_vec2d a, wp_vec2d b );
wp_vec2d wp_vec2d_scale( wp_vec2d a, wp_f64 s );
wp_vec2d wp_vec2d_div( wp_vec2d a, wp_vec2d b );
wp_vec2d wp_vec2d_divd( wp_vec2d a, wp_f64 s );
wp_vec2d wp_vec2d_normalize( wp_vec2d a );
wp_vec2d wp_vec2d_lerp( wp_vec2d a, wp_vec2d b, wp_f64 t );
wp_vec2d wp_vec2d_min( wp_vec2d a, wp_vec2d b );
wp_vec2d wp_vec2d_max( wp_vec2d a, wp_vec2d b );
wp_vec2d wp_vec2d_abs( wp_vec2d a );
wp_f64 wp_vec2d_dot( wp_vec2d a, wp_vec2d b );
wp_f64 wp_vec2d_length( wp_vec2d a );
wp_f64 wp_vec2d_length_sq( wp_vec2d a );
wp_f64 wp_vec2d_distance( wp_vec2d a, wp_vec2d b );
wp_f64 wp_vec2d_distance_sq( wp_vec2d a, wp_vec2d b );
wp_s32 wp_vec2d_equals( wp_vec2d a, wp_vec2d b );
wp_s32 wp_vec2d_is_zero_length( wp_vec2d a );

wp_vec3d wp_vec3d_make( wp_f64 x, wp_f64 y, wp_f64 z );
wp_vec3d wp_vec3d_negate( wp_vec3d a );
wp_vec3d wp_vec3d_add( wp_vec3d a, wp_vec3d b );
wp_vec3d wp_vec3d_sub( wp_vec3d a, wp_vec3d b );
wp_vec3d wp_vec3d_mul( wp_vec3d a, wp_vec3d b );
wp_vec3d wp_vec3d_scale( wp_vec3d a, wp_f64 s );
wp_vec3d wp_vec3d_div( wp_vec3d a, wp_vec3d b );
wp_vec3d wp_vec3d_divd( wp_vec3d a, wp_f64 s );
wp_vec3d wp_vec3d_cross( wp_vec3d a, wp_vec3d b );
wp_vec3d wp_vec3d_normalize( wp_vec3d a );
wp_vec3d wp_vec3d_lerp( wp_vec3d a, wp_vec3d b, wp_f64 t );
wp_vec3d wp_vec3d_lerp_quadratic( wp_vec3d a, wp_vec3d b, wp_vec3d c, wp_f64 t );
wp_vec3d wp_vec3d_min( wp_vec3d a, wp_vec3d b );
wp_vec3d wp_vec3d_max( wp_vec3d a, wp_vec3d b );
wp_vec3d wp_vec3d_abs( wp_vec3d a );
wp_vec3d wp_vec3d_mid( wp_vec3d a, wp_vec3d b );
wp_vec3d wp_vec3d_perpendicular( wp_vec3d a );
wp_f64 wp_vec3d_dot( wp_vec3d a, wp_vec3d b );
wp_f64 wp_vec3d_dot_abs( wp_vec3d a, wp_vec3d b );
wp_f64 wp_vec3d_length( wp_vec3d a );
wp_f64 wp_vec3d_length_sq( wp_vec3d a );
wp_f64 wp_vec3d_distance( wp_vec3d a, wp_vec3d b );
wp_f64 wp_vec3d_distance_sq( wp_vec3d a, wp_vec3d b );
wp_s32 wp_vec3d_equals( wp_vec3d a, wp_vec3d b );
wp_s32 wp_vec3d_equals_tol( wp_vec3d a, wp_vec3d b, wp_f64 tolerance );
wp_s32 wp_vec3d_is_zero_length( wp_vec3d a );
wp_s32 wp_vec3d_is_between_points( wp_vec3d p, wp_vec3d begin, wp_vec3d end );

wp_vec4d wp_vec4d_make( wp_f64 x, wp_f64 y, wp_f64 z, wp_f64 w );
wp_vec4d wp_vec4d_negate( wp_vec4d a );
wp_vec4d wp_vec4d_add( wp_vec4d a, wp_vec4d b );
wp_vec4d wp_vec4d_sub( wp_vec4d a, wp_vec4d b );
wp_vec4d wp_vec4d_mul( wp_vec4d a, wp_vec4d b );
wp_vec4d wp_vec4d_scale( wp_vec4d a, wp_f64 s );
wp_vec4d wp_vec4d_div( wp_vec4d a, wp_vec4d b );
wp_vec4d wp_vec4d_divd( wp_vec4d a, wp_f64 s );
wp_vec4d wp_vec4d_normalize( wp_vec4d a );
wp_vec4d wp_vec4d_lerp( wp_vec4d a, wp_vec4d b, wp_f64 t );
wp_vec4d wp_vec4d_min( wp_vec4d a, wp_vec4d b );
wp_vec4d wp_vec4d_max( wp_vec4d a, wp_vec4d b );
wp_vec4d wp_vec4d_abs( wp_vec4d a );
wp_f64 wp_vec4d_dot( wp_vec4d a, wp_vec4d b );
wp_f64 wp_vec4d_length( wp_vec4d a );
wp_f64 wp_vec4d_length_sq( wp_vec4d a );
wp_f64 wp_vec4d_distance( wp_vec4d a, wp_vec4d b );
wp_f64 wp_vec4d_distance_sq( wp_vec4d a, wp_vec4d b );
wp_s32 wp_vec4d_equals( wp_vec4d a, wp_vec4d b );
wp_s32 wp_vec4d_is_zero_length( wp_vec4d a );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_VECTOR_H */
