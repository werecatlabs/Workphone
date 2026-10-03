#include "sse.h"

WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_zero( void )
{
#if WP_PLATFORM_SSE_AVAILABLE
    return _mm_setzero_ps();
#else
    return wp_simd_f32x4_zero();
#endif
}

WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_set( wp_f32 x, wp_f32 y, wp_f32 z, wp_f32 w )
{
#if WP_PLATFORM_SSE_AVAILABLE
    return _mm_set_ps( w, z, y, x );
#else
    return wp_simd_f32x4_set( x, y, z, w );
#endif
}

WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_splat( wp_f32 value )
{
#if WP_PLATFORM_SSE_AVAILABLE
    return _mm_set1_ps( value );
#else
    return wp_simd_f32x4_splat( value );
#endif
}

WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_load( const wp_f32 *values )
{
#if WP_PLATFORM_SSE_AVAILABLE
    return _mm_loadu_ps( values );
#else
    return wp_simd_f32x4_load( values );
#endif
}

WORKPHONE_API void wp_sse_f32x4_store( wp_f32 *values, wp_sse_f32x4 value )
{
#if WP_PLATFORM_SSE_AVAILABLE
    _mm_storeu_ps( values, value );
#else
    wp_simd_f32x4_store( values, value );
#endif
}

WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_add( wp_sse_f32x4 left, wp_sse_f32x4 right )
{
#if WP_PLATFORM_SSE_AVAILABLE
    return _mm_add_ps( left, right );
#else
    return wp_simd_f32x4_add( left, right );
#endif
}

WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_sub( wp_sse_f32x4 left, wp_sse_f32x4 right )
{
#if WP_PLATFORM_SSE_AVAILABLE
    return _mm_sub_ps( left, right );
#else
    return wp_simd_f32x4_sub( left, right );
#endif
}

WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_mul( wp_sse_f32x4 left, wp_sse_f32x4 right )
{
#if WP_PLATFORM_SSE_AVAILABLE
    return _mm_mul_ps( left, right );
#else
    return wp_simd_f32x4_mul( left, right );
#endif
}

WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_div( wp_sse_f32x4 left, wp_sse_f32x4 right )
{
#if WP_PLATFORM_SSE_AVAILABLE
    return _mm_div_ps( left, right );
#else
    return wp_simd_f32x4_div( left, right );
#endif
}

WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_min( wp_sse_f32x4 left, wp_sse_f32x4 right )
{
#if WP_PLATFORM_SSE_AVAILABLE
    return _mm_min_ps( left, right );
#else
    return wp_simd_f32x4_min( left, right );
#endif
}

WORKPHONE_API wp_sse_f32x4 wp_sse_f32x4_max( wp_sse_f32x4 left, wp_sse_f32x4 right )
{
#if WP_PLATFORM_SSE_AVAILABLE
    return _mm_max_ps( left, right );
#else
    return wp_simd_f32x4_max( left, right );
#endif
}

WORKPHONE_API wp_f32 wp_sse_f32x4_sum( wp_sse_f32x4 value )
{
    wp_f32 lanes[4];
    wp_sse_f32x4_store( lanes, value );
    return lanes[0] + lanes[1] + lanes[2] + lanes[3];
}

WORKPHONE_API wp_f32 wp_sse_f32x4_dot( wp_sse_f32x4 left, wp_sse_f32x4 right )
{
    return wp_sse_f32x4_sum( wp_sse_f32x4_mul( left, right ) );
}
