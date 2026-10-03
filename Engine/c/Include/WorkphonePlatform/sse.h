#ifndef WORKPHONE_PLATFORM_SSE_H
#define WORKPHONE_PLATFORM_SSE_H

#include "simd.h"

/* MSVC does not define __SSE__ for every x86 target, although the relevant
 * intrinsic header is still available when SSE2 is enabled (and always on
 * x64). */
#if defined( __SSE__ ) || defined( _M_X64 ) || \
    ( defined( _M_IX86_FP ) && _M_IX86_FP >= 1 )
#    define WP_PLATFORM_SSE_AVAILABLE 1
#    include <xmmintrin.h>
#else
#    define WP_PLATFORM_SSE_AVAILABLE 0
#endif

#if WP_PLATFORM_SSE_AVAILABLE
typedef __m128 wp_sse_f32x4;
#else
typedef wp_simd_f32x4 wp_sse_f32x4;
#endif

WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_zero( void );
WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_set( wp_f32 x, wp_f32 y, wp_f32 z, wp_f32 w );
WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_splat( wp_f32 value );
WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_load( const wp_f32 *values );
WORKPHONE_API void wp_sse_f32x4_store( wp_f32 *values, wp_sse_f32x4 value );

WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_add( wp_sse_f32x4 left, wp_sse_f32x4 right );
WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_sub( wp_sse_f32x4 left, wp_sse_f32x4 right );
WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_mul( wp_sse_f32x4 left, wp_sse_f32x4 right );
WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_div( wp_sse_f32x4 left, wp_sse_f32x4 right );
WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_min( wp_sse_f32x4 left, wp_sse_f32x4 right );
WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_max( wp_sse_f32x4 left, wp_sse_f32x4 right );

WORKPHONE_API wp_f32 wp_sse_f32x4_sum( wp_sse_f32x4 value );
WORKPHONE_API wp_f32 wp_sse_f32x4_dot( wp_sse_f32x4 left, wp_sse_f32x4 right );

#endif /* WORKPHONE_PLATFORM_SSE_H */
