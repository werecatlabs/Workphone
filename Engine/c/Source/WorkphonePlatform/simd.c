#include "simd.h"

#if defined( _WIN32 ) || defined( _WIN64 )
#    include <intrin.h>
#elif defined( __x86_64__ ) || defined( __i386__ )
#    include <cpuid.h>
#endif

#if defined( __SSE__ )
#    include <xmmintrin.h>
#endif

#if defined( __ARM_NEON ) || defined( __ARM_NEON__ )
#    include <arm_neon.h>
#endif

static wp_platform_os wp_platform_detect_os( void )
{
#if defined( __ANDROID__ )
    return WP_PLATFORM_OS_ANDROID;
#elif defined( _WIN32 ) || defined( _WIN64 )
    return WP_PLATFORM_OS_WINDOWS;
#elif defined( __APPLE__ )
    return WP_PLATFORM_OS_APPLE;
#elif defined( __linux__ )
    return WP_PLATFORM_OS_LINUX;
#elif defined( __sh__ ) || defined( __SH4__ )
    return WP_PLATFORM_OS_DREAMCAST;
#else
    return WP_PLATFORM_OS_UNKNOWN;
#endif
}

static wp_platform_architecture wp_platform_detect_architecture( void )
{
#if defined( __x86_64__ ) || defined( _M_X64 ) || defined( _M_AMD64 )
    return WP_PLATFORM_ARCH_X64;
#elif defined( __i386__ ) || defined( _M_IX86 )
    return WP_PLATFORM_ARCH_X86;
#elif defined( __aarch64__ ) || defined( _M_ARM64 )
    return WP_PLATFORM_ARCH_ARM64;
#elif defined( __arm__ ) || defined( _M_ARM )
    return WP_PLATFORM_ARCH_ARM;
#elif defined( __sh__ ) || defined( __SH4__ )
    return WP_PLATFORM_ARCH_SH4;
#else
    return WP_PLATFORM_ARCH_UNKNOWN;
#endif
}

static wp_platform_simd wp_platform_detect_simd( void )
{
#if defined( __aarch64__ ) || defined( _M_ARM64 )
    return WP_PLATFORM_SIMD_NEON;
#elif defined( __ARM_NEON ) || defined( __ARM_NEON__ )
    return WP_PLATFORM_SIMD_NEON;
#elif defined( __AVX2__ )
    return WP_PLATFORM_SIMD_AVX2;
#elif defined( __AVX__ )
    return WP_PLATFORM_SIMD_AVX;
#elif defined( __SSE2__ ) || defined( _M_X64 )
    return WP_PLATFORM_SIMD_SSE2;
#elif defined( _WIN32 ) || defined( _WIN64 )
    int regs[4];
    __cpuid( regs, 1 );
    if( ( regs[3] & ( 1 << 26 ) ) != 0 )
        return WP_PLATFORM_SIMD_SSE2;
    return WP_PLATFORM_SIMD_NONE;
#elif defined( __x86_64__ ) || defined( __i386__ )
    unsigned int max_leaf;
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;

    if( __get_cpuid( 0, &max_leaf, &ebx, &ecx, &edx ) == 0 || max_leaf < 1 )
        return WP_PLATFORM_SIMD_NONE;

    if( __get_cpuid( 1, &eax, &ebx, &ecx, &edx ) == 0 )
        return WP_PLATFORM_SIMD_NONE;
    if( ( edx & ( 1u << 26 ) ) != 0 )
        return WP_PLATFORM_SIMD_SSE2;
#endif
    return WP_PLATFORM_SIMD_NONE;
}

static wp_simd_f32x4 wp_simd_f32x4_binary_scalar( wp_simd_f32x4 left,
                                                   wp_simd_f32x4 right, wp_u32 operation )
{
    wp_simd_f32x4 result;
    wp_u32 i;

    for( i = 0; i < 4; ++i )
    {
        switch( operation )
        {
        case 0: result.lane[i] = left.lane[i] + right.lane[i]; break;
        case 1: result.lane[i] = left.lane[i] - right.lane[i]; break;
        case 2: result.lane[i] = left.lane[i] * right.lane[i]; break;
        case 3: result.lane[i] = left.lane[i] / right.lane[i]; break;
        case 4: result.lane[i] = left.lane[i] < right.lane[i] ? left.lane[i] : right.lane[i]; break;
        default: result.lane[i] = left.lane[i] > right.lane[i] ? left.lane[i] : right.lane[i]; break;
        }
    }
    return result;
}

WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_zero( void )
{
    return wp_simd_f32x4_splat( 0.0f );
}

WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_set( wp_f32 x, wp_f32 y, wp_f32 z, wp_f32 w )
{
    wp_simd_f32x4 result;
    result.lane[0] = x;
    result.lane[1] = y;
    result.lane[2] = z;
    result.lane[3] = w;
    return result;
}

WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_splat( wp_f32 value )
{
    return wp_simd_f32x4_set( value, value, value, value );
}

WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_load( const wp_f32 *values )
{
    wp_simd_f32x4 result;
    result.lane[0] = values[0];
    result.lane[1] = values[1];
    result.lane[2] = values[2];
    result.lane[3] = values[3];
    return result;
}

WORKPHONE_API void wp_simd_f32x4_store( wp_f32 *values, wp_simd_f32x4 value )
{
    values[0] = value.lane[0];
    values[1] = value.lane[1];
    values[2] = value.lane[2];
    values[3] = value.lane[3];
}

WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_add( wp_simd_f32x4 left, wp_simd_f32x4 right )
{
#if defined( __SSE__ )
    wp_simd_f32x4 result;
    _mm_storeu_ps( result.lane, _mm_add_ps( _mm_loadu_ps( left.lane ), _mm_loadu_ps( right.lane ) ) );
    return result;
#elif defined( __ARM_NEON ) || defined( __ARM_NEON__ )
    wp_simd_f32x4 result;
    vst1q_f32( result.lane, vaddq_f32( vld1q_f32( left.lane ), vld1q_f32( right.lane ) ) );
    return result;
#else
    return wp_simd_f32x4_binary_scalar( left, right, 0 );
#endif
}

WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_sub( wp_simd_f32x4 left, wp_simd_f32x4 right )
{
#if defined( __SSE__ )
    wp_simd_f32x4 result;
    _mm_storeu_ps( result.lane, _mm_sub_ps( _mm_loadu_ps( left.lane ), _mm_loadu_ps( right.lane ) ) );
    return result;
#elif defined( __ARM_NEON ) || defined( __ARM_NEON__ )
    wp_simd_f32x4 result;
    vst1q_f32( result.lane, vsubq_f32( vld1q_f32( left.lane ), vld1q_f32( right.lane ) ) );
    return result;
#else
    return wp_simd_f32x4_binary_scalar( left, right, 1 );
#endif
}

WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_mul( wp_simd_f32x4 left, wp_simd_f32x4 right )
{
#if defined( __SSE__ )
    wp_simd_f32x4 result;
    _mm_storeu_ps( result.lane, _mm_mul_ps( _mm_loadu_ps( left.lane ), _mm_loadu_ps( right.lane ) ) );
    return result;
#elif defined( __ARM_NEON ) || defined( __ARM_NEON__ )
    wp_simd_f32x4 result;
    vst1q_f32( result.lane, vmulq_f32( vld1q_f32( left.lane ), vld1q_f32( right.lane ) ) );
    return result;
#else
    return wp_simd_f32x4_binary_scalar( left, right, 2 );
#endif
}

WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_div( wp_simd_f32x4 left, wp_simd_f32x4 right )
{
    /* Division is scalar on SSE1 and on older NEON targets. */
    return wp_simd_f32x4_binary_scalar( left, right, 3 );
}

WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_min( wp_simd_f32x4 left, wp_simd_f32x4 right )
{
#if defined( __SSE__ )
    wp_simd_f32x4 result;
    _mm_storeu_ps( result.lane, _mm_min_ps( _mm_loadu_ps( left.lane ), _mm_loadu_ps( right.lane ) ) );
    return result;
#else
    return wp_simd_f32x4_binary_scalar( left, right, 4 );
#endif
}

WORKPHONE_API wp_simd_f32x4 wp_simd_f32x4_max( wp_simd_f32x4 left, wp_simd_f32x4 right )
{
#if defined( __SSE__ )
    wp_simd_f32x4 result;
    _mm_storeu_ps( result.lane, _mm_max_ps( _mm_loadu_ps( left.lane ), _mm_loadu_ps( right.lane ) ) );
    return result;
#else
    return wp_simd_f32x4_binary_scalar( left, right, 5 );
#endif
}

WORKPHONE_API wp_f32 wp_simd_f32x4_dot( wp_simd_f32x4 left, wp_simd_f32x4 right )
{
    wp_simd_f32x4 product = wp_simd_f32x4_mul( left, right );
    return wp_simd_f32x4_sum( product );
}

WORKPHONE_API wp_f32 wp_simd_f32x4_sum( wp_simd_f32x4 value )
{
    return value.lane[0] + value.lane[1] + value.lane[2] + value.lane[3];
}

WORKPHONE_API wp_platform_os wp_platform_get_os( void )
{
    return wp_platform_detect_os();
}

WORKPHONE_API wp_platform_architecture wp_platform_get_architecture( void )
{
    return wp_platform_detect_architecture();
}

WORKPHONE_API wp_platform_simd wp_platform_get_simd( void )
{
    return wp_platform_detect_simd();
}

WORKPHONE_API wp_u32 wp_platform_get_pointer_bits( void )
{
    return (wp_u32)( sizeof( void * ) * 8u );
}

WORKPHONE_API wp_bool wp_platform_is_little_endian( void )
{
    const wp_u16 value = 1;
    return *( (const wp_u8 *)&value ) == 1 ? wp_true : wp_false;
}

WORKPHONE_API const wp_c8 *wp_platform_os_name( wp_platform_os os )
{
    switch( os )
    {
    case WP_PLATFORM_OS_WINDOWS: return "windows";
    case WP_PLATFORM_OS_APPLE: return "apple";
    case WP_PLATFORM_OS_LINUX: return "linux";
    case WP_PLATFORM_OS_ANDROID: return "android";
    case WP_PLATFORM_OS_DREAMCAST: return "dreamcast";
    default: return "unknown";
    }
}

WORKPHONE_API const wp_c8 *wp_platform_architecture_name( wp_platform_architecture architecture )
{
    switch( architecture )
    {
    case WP_PLATFORM_ARCH_X86: return "x86";
    case WP_PLATFORM_ARCH_X64: return "x64";
    case WP_PLATFORM_ARCH_ARM: return "arm";
    case WP_PLATFORM_ARCH_ARM64: return "arm64";
    case WP_PLATFORM_ARCH_SH4: return "sh4";
    default: return "unknown";
    }
}

WORKPHONE_API const wp_c8 *wp_platform_simd_name( wp_platform_simd simd )
{
    switch( simd )
    {
    case WP_PLATFORM_SIMD_SSE2: return "sse2";
    case WP_PLATFORM_SIMD_AVX: return "avx";
    case WP_PLATFORM_SIMD_AVX2: return "avx2";
    case WP_PLATFORM_SIMD_NEON: return "neon";
    default: return "none";
    }
}
