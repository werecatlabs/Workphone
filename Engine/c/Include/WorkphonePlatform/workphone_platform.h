#ifndef WORKPHONE_PLATFORM_H
#define WORKPHONE_PLATFORM_H

/*
 * Platform-neutral information used by the C libraries.
 *
 * This header deliberately does not include an operating-system header.  A
 * platform implementation may use those headers internally, but consumers of
 * the core libraries only need the stable Workphone types below.
 */

#include "workphone_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum wp_platform_os
{
    WP_PLATFORM_OS_UNKNOWN = 0,
    WP_PLATFORM_OS_WINDOWS,
    WP_PLATFORM_OS_APPLE,
    WP_PLATFORM_OS_LINUX,
    WP_PLATFORM_OS_ANDROID,
    WP_PLATFORM_OS_DREAMCAST
} wp_platform_os;

typedef enum wp_platform_architecture
{
    WP_PLATFORM_ARCH_UNKNOWN = 0,
    WP_PLATFORM_ARCH_X86,
    WP_PLATFORM_ARCH_X64,
    WP_PLATFORM_ARCH_ARM,
    WP_PLATFORM_ARCH_ARM64,
    WP_PLATFORM_ARCH_SH4
} wp_platform_architecture;

typedef enum wp_platform_simd
{
    WP_PLATFORM_SIMD_NONE = 0,
    WP_PLATFORM_SIMD_SSE2,
    WP_PLATFORM_SIMD_AVX,
    WP_PLATFORM_SIMD_AVX2,
    WP_PLATFORM_SIMD_NEON
} wp_platform_simd;

/** Return the operating system selected by the compiler/toolchain. */
WORKPHONE_API wp_platform_os wp_platform_get_os( void );

/** Return the target CPU architecture selected by the compiler/toolchain. */
WORKPHONE_API wp_platform_architecture wp_platform_get_architecture( void );

/** Return the best SIMD instruction set available to this process. */
WORKPHONE_API wp_platform_simd wp_platform_get_simd( void );

/** Return the native pointer width in bits. */
WORKPHONE_API wp_u32 wp_platform_get_pointer_bits( void );

/** Return non-zero when the target uses little-endian byte order. */
WORKPHONE_API wp_bool wp_platform_is_little_endian( void );

/** Return a stable, lower-case name for an OS value. */
WORKPHONE_API const wp_c8 *wp_platform_os_name( wp_platform_os os );

/** Return a stable, lower-case name for an architecture value. */
WORKPHONE_API const wp_c8 *wp_platform_architecture_name( wp_platform_architecture architecture );

/** Return a stable name for a SIMD value. */
WORKPHONE_API const wp_c8 *wp_platform_simd_name( wp_platform_simd simd );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_PLATFORM_H */
