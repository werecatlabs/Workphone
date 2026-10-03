/**
 * @file WorkphoneTypes.hpp
 * @brief Core type definitions, enums, and macros for the WorkPhone engine.
 * @details This file defines fundamental types, enums, and macros used throughout the engine, including:
 *          - Fixed-width integer and floating-point type aliases
 *          - Engine-specific enumerations for rendering, physics, input, and more
 *          - Platform and compiler detection macros
 *          - Assertion and deprecation macros
 * @note This header is included by most engine modules and should remain lightweight.
 * @see WorkphoneConfig.hpp for build configuration options.
 */
#ifndef __WP_CoreTypes_h__
#define __WP_CoreTypes_h__

#include <Workphone/WorkphoneConfig.hpp>

#include <cstddef>

#pragma warning( push )
#pragma warning( disable : 4244 )

// disable loss of data conversion warnings
#pragma warning( push )
#pragma warning( disable : 4244 )

// disable loss of data conversion warnings

#if WP_COMPILER == WP_COMPILER_MSVC
#    include <cstdint>
#endif

namespace workphone
{
    /**
     * @defgroup PrimitiveTypes Primitive Type Aliases
     * @brief Fixed-width type aliases for cross-platform consistency.
     * @{
     */

    /**
     * @typedef u8
     * @brief 8-bit unsigned integer (0 to 255).
     */
    using u8 = unsigned char;

    /**
     * @typedef s8
     * @brief 8-bit signed integer (-128 to 127).
     */
    using s8 = signed char;

    /**
     * @typedef c8
     * @brief 8-bit character type for text processing.
     */
    using c8 = char;

    /**
     * @typedef u16
     * @brief 16-bit unsigned integer (0 to 65,535).
     */
    using u16 = unsigned short;

    /**
     * @typedef s16
     * @brief 16-bit signed integer (-32,768 to 32,767).
     */
    using s16 = signed short;

    /**
     * @typedef c16
     * @brief 16-bit character type for Unicode text processing.
     */
    using c16 = wchar_t;

    /**
     * @typedef u32
     * @brief 32-bit unsigned integer (0 to 4,294,967,295).
     */
    using u32 = unsigned int;

    /**
     * @typedef s32
     * @brief 32-bit signed integer (-2,147,483,648 to 2,147,483,647).
     */
    using s32 = signed int;

#if WP_COMPILER == WP_COMPILER_MSVC
    /**
     * @typedef s64
     * @brief 64-bit signed integer (MSVC-specific implementation using cstdint).
     */
    using s64 = int64_t;

    /**
     * @typedef u64
     * @brief 64-bit unsigned integer (MSVC-specific implementation using cstdint).
     */
    using u64 = uint64_t;
#else
    /**
     * @typedef s64
     * @brief 64-bit signed integer (non-MSVC implementation).
     */
    using s64 = long long;

    /**
     * @typedef u64
     * @brief 64-bit unsigned integer (non-MSVC implementation).
     */
    using u64 = unsigned long long;
#endif

    /**
     * @typedef f32
     * @brief 32-bit single-precision floating point type (IEEE 754).
     */
    using f32 = float;

    /**
     * @typedef f64
     * @brief 64-bit double-precision floating point type (IEEE 754).
     */
    using f64 = double;

    /** @} */  // End of PrimitiveTypes group

    /**
     * @defgroup PrecisionTypes Precision-Dependent Types
     * @brief Type aliases that vary based on WP_DOUBLE_PRECISION configuration.
     * @{
     */

#if WP_DOUBLE_PRECISION
    /**
     * @typedef size_Num
     * @brief Size type for memory sizes and element counts.
     */
    using size_Num = std::size_t;

    /**
     * @typedef real_Num
     * @brief Primary real number type (double precision when WP_DOUBLE_PRECISION is enabled).
     */
    using real_Num = double;

    /**
     * @typedef real_dNum
     * @brief Double-precision real number type, always double regardless of precision setting.
     */
    using real_dNum = double;

    /**
     * @typedef graphics_Num
     * @brief Numeric type for graphics calculations (double precision).
     */
    using graphics_Num = double;

    /**
     * @typedef physics_Num
     * @brief Numeric type for physics simulations (double precision).
     */
    using physics_Num = double;
#else
    /**
     * @typedef size_Num
     * @brief Size type for memory sizes and element counts.
     */
    using size_Num = std::size_t;

    /**
     * @typedef real_Num
     * @brief Primary real number type (single precision when WP_DOUBLE_PRECISION is disabled).
     */
    using real_Num = float;

    /**
     * @typedef real_dNum
     * @brief Double-precision real number type, always double regardless of precision setting.
     */
    using real_dNum = double;

    /**
     * @typedef graphics_Num
     * @brief Numeric type for graphics calculations (single precision).
     */
    using graphics_Num = float;

    /**
     * @typedef physics_Num
     * @brief Numeric type for physics simulations (single precision).
     */
    using physics_Num = float;
#endif

    /** @} */  // End of PrecisionTypes group

    /**
     * @defgroup UtilityTypes Utility Types
     * @brief Common utility type aliases used throughout the engine.
     * @{
     */

    /**
     * @typedef time_interval
     * @brief Type used for engine time intervals, measured in seconds.
     * @details Always uses double precision for accurate time tracking over extended periods.
     */
    using time_interval = double;

    /**
     * @typedef bool32
     * @brief 32-bit boolean type for ABI compatibility with C APIs.
     * @details Uses 0 for false and non-zero (typically 1) for true.
     */
    using bool32 = int;

    /**
     * @typedef hash32
     * @brief 32-bit hash value type for fast hashing operations.
     */
    using hash32 = u32;

    /**
     * @typedef hash64
     * @brief 64-bit hash value type for reduced collision probability.
     */
    using hash64 = s64;

#if WP_USE_HASH32
    /**
     * @typedef hash_type
     * @brief Default hash type (32-bit when WP_USE_HASH32 is defined).
     */
    using hash_type = hash32;
#else
    /**
     * @typedef hash_type
     * @brief Default hash type (64-bit when WP_USE_HASH32 is not defined).
     */
    using hash_type = hash64;
#endif

    using LayerMask = unsigned long long;  ///< Type for layer masks in rendering and physics systems.

    /** @} */  // End of UtilityTypes group

}  // namespace workphone

#ifdef WP_PLATFORM_LINUX
#    include <cmath>
using std::isfinite;
#endif

#ifdef WP_PLATFORM_LINUX
/**
 * @typedef HWND
 * @brief Handle to a window (Linux stub).
 */
typedef void *HWND;
/**
 * @typedef UINT
 * @brief Unsigned integer type (Linux stub).
 */
typedef unsigned int UINT;
/**
 * @typedef WPARAM
 * @brief WPARAM type (Linux stub).
 */
typedef unsigned int WPARAM;
/**
 * @typedef LPARAM
 * @brief LPARAM type (Linux stub).
 */
typedef unsigned int LPARAM;
/**
 * @typedef HMODULE
 * @brief HMODULE type (Linux stub).
 */
typedef unsigned int HMODULE;
/**
 * @typedef DWORD
 * @brief DWORD type (Linux stub).
 */
typedef unsigned int DWORD;
/**
 * @typedef HANDLE
 * @brief HANDLE type (Linux stub).
 */
typedef unsigned int HANDLE;
#endif

#pragma warning( pop )  // restore warnings

#endif  // __WP_CoreTypes_h__
