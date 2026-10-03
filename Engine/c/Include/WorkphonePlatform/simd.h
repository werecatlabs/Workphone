#ifndef WORKPHONE_PLATFORM_SIMD_H
#define WORKPHONE_PLATFORM_SIMD_H

#include "workphone_platform.h"

/*
 * A small, ABI-stable four-wide floating-point type.  The public type is
 * deliberately an ordinary C struct: callers do not need to include an
 * architecture header and the implementation can change its native backend
 * without changing application code.
 */
typedef struct wp_simd_f32x4
{
    wp_f32 lane[4];
} wp_simd_f32x4;

WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_zero( void );
WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_set( wp_f32 x, wp_f32 y, wp_f32 z, wp_f32 w );
WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_splat( wp_f32 value );
WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_load( const wp_f32 *values );
WORKPHONE_API void wp_simd_f32x4_store( wp_f32 *values, wp_simd_f32x4 value );

WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_add( wp_simd_f32x4 left, wp_simd_f32x4 right );
WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_sub( wp_simd_f32x4 left, wp_simd_f32x4 right );
WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_mul( wp_simd_f32x4 left, wp_simd_f32x4 right );
WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_div( wp_simd_f32x4 left, wp_simd_f32x4 right );
WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_min( wp_simd_f32x4 left, wp_simd_f32x4 right );
WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_max( wp_simd_f32x4 left, wp_simd_f32x4 right );

WORKPHONE_API wp_f32 wp_simd_f32x4_dot( wp_simd_f32x4 left, wp_simd_f32x4 right );
WORKPHONE_API wp_f32 wp_simd_f32x4_sum( wp_simd_f32x4 value );

#endif /* WORKPHONE_PLATFORM_SIMD_H */
