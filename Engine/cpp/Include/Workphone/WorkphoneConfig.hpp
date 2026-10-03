/**
 * @file WorkphoneConfig.hpp
 * @brief Platform, compiler, and architecture detection configuration header.
 *
 * This header provides compile-time detection macros for identifying the
 * target compiler, platform, architecture, and endianness. It also defines
 * various utility macros for cross-platform compatibility including:
 * - DLL export/import declarations
 * - Calling conventions
 * - Force inline directives
 * - Memory allocation helpers
 * - Thread-local storage
 */

#ifndef __WP_CONFIG_H
#define __WP_CONFIG_H

//------------------------------------------------------------------------------
/**
 * @defgroup CompilerDetection Compiler Detection
 * @brief Macros for detecting the active compiler and version.
 *
 * Sets WP_COMPILER to one of the WP_COMPILER_* constants and WP_COMP_VER
 * to a numeric version value for version comparisons.
 * @{
 */
//------------------------------------------------------------------------------
#if defined( __clang__ )
#    define WP_COMPILER WP_COMPILER_CLANG
#    define WP_COMP_VER \
        ( ( ( __clang_major__ ) * 100 ) + ( __clang_minor__ * 10 ) + __clang_patchlevel__ )
#elif defined( __GCCE__ )
#    define WP_COMPILER WP_COMPILER_GCCE
#    define WP_COMP_VER _MSC_VER
#elif defined( __WINSCW__ )
#    define WP_COMPILER WP_COMPILER_WINSCW
#    define WP_COMP_VER _MSC_VER
#elif defined( _MSC_VER )
#    define WP_COMPILER WP_COMPILER_MSVC
#    define WP_COMP_VER _MSC_VER
#elif defined( __GNUC__ )
#    define WP_COMPILER WP_COMPILER_GNUC
#    define WP_COMP_VER ( ( ( __GNUC__ ) * 100 ) + ( __GNUC_MINOR__ * 10 ) + __GNUC_PATCHLEVEL__ )
#elif defined( __BORLANDC__ )
#    define WP_COMPILER WP_COMPILER_BORL
#    define WP_COMP_VER __BCPLUSPLUS__
#    define __FUNCTION__ __FUNC__
#else
#    pragma error "No known compiler."
#endif
/** @} */  // end of CompilerDetection

//------------------------------------------------------------------------------
/**
 * @defgroup PlatformDetection Platform Detection
 * @brief Macros for detecting the target operating system/platform.
 *
 * Defines one of the following based on the target platform:
 * - WP_PLATFORM_WIN32: Windows desktop
 * - WP_PLATFORM_WINRT: Windows Runtime (UWP/Phone)
 * - WP_PLATFORM_FLASHCC: Adobe Flash
 * - WP_PLATFORM_IOS: Apple iOS
 * - WP_PLATFORM_APPLE: macOS
 * - WP_PLATFORM_ANDROID: Android
 * - WP_PLATFORM_NACL: Native Client
 * - WP_PLATFORM_LINUX: Linux (default fallback)
 * @{
 */
//------------------------------------------------------------------------------
#if defined( __WIN32__ ) || defined( _WIN32 )
#    if defined( WINAPI_FAMILY )
#        define __WP_HAVE_DIRECTXMATH 1
#        include <winapifamily.h>
#        if WINAPI_FAMILY == WINAPI_FAMILY_APP || WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
#            define DESKTOP_APP 1
#            define PHONE 2
#            define WP_PLATFORM_WINRT
#            define _SCL_SECURE_NO_WARNINGS
#            if WINAPI_FAMILY == WINAPI_FAMILY_APP
#                define WP_WINRT_TARGET_TYPE DESKTOP_APP
#            endif
#            if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
#                define WP_WINRT_TARGET_TYPE PHONE
#            endif
#        else
#            define WP_PLATFORM_WIN32
#        endif
#    else
#        define WP_PLATFORM_WIN32
#    endif
#elif defined( __FLASHCC__ )
#    define WP_PLATFORM_FLASHCC
#elif defined( __APPLE_CC__ )
#    if __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ >= 60000 || \
        __IPHONE_OS_VERSION_MIN_REQUIRED >= 60000
#        define WP_PLATFORM_IOS
#    else
#        define WP_PLATFORM_APPLE
#    endif
#elif defined( __ANDROID__ )
#    define WP_PLATFORM_ANDROID
#elif defined( __native_client__ )
#    define WP_PLATFORM_NACL
#else
#    define WP_PLATFORM_LINUX
#endif
/** @} */  // end of PlatformDetection

//------------------------------------------------------------------------------
/**
 * @defgroup ArchitectureDetection Architecture Detection
 * @brief Macros for detecting the target CPU architecture (32-bit or 64-bit).
 * @{
 */
//------------------------------------------------------------------------------

/** @brief Constant representing 32-bit architecture. */
#define WP_ARCHITECTURE_32 1

/** @brief Constant representing 64-bit architecture. */
#define WP_ARCHITECTURE_64 2

#if defined( __x86_64__ ) || defined( _M_X64 ) || defined( __powerpc64__ ) || defined( __alpha__ ) || \
    defined( __ia64__ ) || defined( __s390__ ) || defined( __s390x__ ) || defined( __arm64__ ) ||     \
    defined( __aarch64__ ) || defined( __mips64 ) || defined( __mips64_ )
#    define WP_ARCH_TYPE WP_ARCHITECTURE_64
#else
#    define WP_ARCH_TYPE WP_ARCHITECTURE_32
#endif
/** @} */  // end of ArchitectureDetection

//------------------------------------------------------------------------------
/**
 * @defgroup StructPacking Structure Packing
 * @brief Cross-platform structure packing attribute.
 * @{
 */
//------------------------------------------------------------------------------
#if defined( WIN32 ) || defined( __WATCOMC__ ) || defined( _WIN32 ) || defined( __WIN32__ )
#    define __PACKED /**< No-op on Windows (use #pragma pack instead). */
#else
#    define __PACKED __attribute__( ( packed ) ) /**< GCC packed attribute. */
#endif
/** @} */  // end of StructPacking

#ifdef WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
#        ifdef WPCore_EXPORTS
#            define WPCore_API __declspec( dllexport )
#        else
#            define WPCore_API __declspec( dllimport )
#        endif  // WP_EXPORT
#    else
#        define WPCore_API
#    endif  // _WP_STATIC_LIB_
#else
#    define WPCore_API
#endif

//------------------------------------------------------------------------------
/**
 * @defgroup CallingConvention Calling Convention
 * @brief Macros defining the function calling convention.
 * @{
 */
//------------------------------------------------------------------------------
#ifndef _WP_STATIC_LIB_
#    if defined( _STDCALL_SUPPORTED )
#        define WP_CALL_CONV __stdcall /**< Use stdcall convention. */
#    else
#        define WP_CALL_CONV __cdecl /**< Use cdecl convention. */
#    endif
#else
#    define WP_CALL_CONV /**< No explicit calling convention for static libs. */
#endif
/** @} */  // end of CallingConvention

//------------------------------------------------------------------------------
/**
 * @defgroup DLLExport DLL Export/Import Declarations
 * @brief Macros for controlling symbol visibility in shared libraries.
 * @{
 */
//------------------------------------------------------------------------------
#ifdef WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
#        ifdef WPCore_EXPORTS
#            define WPCore_API __declspec( dllexport ) /**< Export symbol when building DLL. */
#        else
#            define WPCore_API __declspec( dllimport ) /**< Import symbol when using DLL. */
#        endif
#    else
#        define WPCore_API /**< No decoration for static library builds. */
#    endif
#else
#    define WPCore_API /**< No decoration on non-Windows platforms. */
#endif
/** @} */  // end of DLLExport

//------------------------------------------------------------------------------
/**
 * @defgroup ForceInline Force Inline
 * @brief Cross-platform force inline directive.
 *
 * When WP_USE_FORCE_INLINE is enabled, uses compiler-specific attributes
 * to force function inlining.
 * @{
 */
//------------------------------------------------------------------------------
#if defined( _MSC_VER ) && _MSC_VER > 1200
#    if WP_USE_FORCE_INLINE
#        define WPForceInline __forceinline
#    else
#        define WPForceInline inline
#    endif
#elif defined( __clang__ )
#    if WP_USE_FORCE_INLINE
#        define WPForceInline inline __attribute__( ( always_inline ) )
#    else
#        define WPForceInline inline
#    endif
#elif defined( __MINGW32__ )
#    if WP_USE_FORCE_INLINE
#        define WPForceInline __inline
#    else
#        define WPForceInline inline
#    endif
#else
#    if WP_USE_FORCE_INLINE
#        define WPForceInline inline
#    else
#        define WPForceInline inline
#    endif
#endif
/** @} */  // end of ForceInline

//------------------------------------------------------------------------------
/**
 * @defgroup ThreadLocalStorage Thread Local Storage
 * @brief Cross-platform thread-local storage specifier.
 * @{
 */
//------------------------------------------------------------------------------
#if !defined( _MSC_VER )
#    define WP_THREAD_LOCAL_STORAGE thread_local
#else
#    define WP_THREAD_LOCAL_STORAGE thread_local
#endif
/** @} */  // end of ThreadLocalStorage

//------------------------------------------------------------------------------
/**
 * @defgroup InterlockedFunctions Interlocked Functions
 * @brief Enable MSVC interlocked intrinsics on supported compilers.
 * @{
 */
//------------------------------------------------------------------------------
#if defined( _MSC_VER )
#    define WP_USE_INTERLOCKED_FUNCTIONS
#endif
/** @} */  // end of InterlockedFunctions

//------------------------------------------------------------------------------
/**
 * @defgroup Endianness Endianness Detection
 * @brief Macros for detecting and configuring byte order.
 * @{
 */
//------------------------------------------------------------------------------
#if defined( __sparc__ ) || defined( __sun__ )
#    define __BIG_ENDIAN__
#endif

/** @brief Constant representing little-endian byte order. */
#define WP_ENDIAN_LITTLE 1

/** @brief Constant representing big-endian byte order. */
#define WP_ENDIAN_BIG 2

#ifdef WP_CONFIG_BIG_ENDIAN
#    define WP_ENDIAN WP_ENDIAN_BIG
#else
#    define WP_ENDIAN WP_ENDIAN_LITTLE
#endif
/** @} */  // end of Endianness

//------------------------------------------------------------------------------
/**
 * @defgroup ExceptionSpecifiers Exception Specifiers
 * @brief Cross-platform noexcept specifier.
 * @{
 */
//------------------------------------------------------------------------------
#if defined WP_PLATFORM_WIN32
#    define WP_NOEXCEPT noexcept
#else
#    define WP_NOEXCEPT _NOEXCEPT
#endif
/** @} */  // end of ExceptionSpecifiers

/** @brief Disable Windows min/max macros that conflict with std::min/std::max. */
#define NOMINMAX

//------------------------------------------------------------------------------
/**
 * @defgroup MemoryTracking Memory Tracking
 * @brief Custom new/delete macros for memory leak detection.
 *
 * When WP_ENABLE_MEMORY_TRACKER and WP_USE_CUSTOM_NEW_DELETE are defined,
 * WP_NEW records file, line, and function information for allocations.
 * @{
 */
#if WP_ENABLE_MEMORY_TRACKER
#    if defined _DEBUG
#        ifdef _WIN32
#            define _CRTDBG_MAP_ALLOC
#            include <cstdlib>
#            include <crtdbg.h>
#        endif
#    endif
#endif

#if WP_ENABLE_MEMORY_TRACKER
#    ifdef _DEBUG
#        define WP_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#        define WP_PLACEMENT_NEW( ptr ) new( ptr )
#        define WP_DELETE delete
#    else
#        define WP_NEW new
#        define WP_PLACEMENT_NEW( ptr ) new( ptr )
#        define WP_DELETE delete
#    endif
#else
#    define WP_NEW new
#    define WP_PLACEMENT_NEW( ptr ) new( ptr )
#    define WP_DELETE delete
#endif
/** @} */  // end of MemoryTracking

//------------------------------------------------------------------------------
/**
 * @defgroup RestrictAliasing Restrict Aliasing
 * @brief Compiler-specific restrict pointer qualifiers for optimization hints.
 * @{
 */
//------------------------------------------------------------------------------
#if WP_RESTRICT_ALIASING != 0
#    if WP_COMPILER == WP_COMPILER_MSVC
#        define RESTRICT_ALIAS __restrict        /**< MSVC restrict qualifier. */
#        define RESTRICT_ALIAS_RETURN __restrict /**< MSVC restrict for return values. */
#    else
#        define RESTRICT_ALIAS __restrict__ /**< GCC/Clang restrict qualifier. */
#        define RESTRICT_ALIAS_RETURN       /**< No return restrict on GCC. */
#    endif
#else
#    define RESTRICT_ALIAS
#    define RESTRICT_ALIAS_RETURN
#endif
/** @} */  // end of RestrictAliasing

//------------------------------------------------------------------------------
/**
 * @defgroup MemoryAllocator Memory Allocator Configuration
 * @brief Constants for selecting the memory allocation strategy.
 * @{
 */
//------------------------------------------------------------------------------
#define WP_MEMORY_ALLOCATOR_NONE 0  /**< No custom allocator. */
#define WP_MEMORY_ALLOCATOR_STD 1   /**< Standard library allocator. */
#define WP_MEMORY_ALLOCATOR_USER 3  /**< User-defined allocator. */
#define WP_MEMORY_ALLOCATOR_TRACK 5 /**< Tracking allocator for debugging. */

#define WP_MEMORY_ALLOCATOR WP_MEMORY_ALLOCATOR_NONE /**< Currently selected allocator. */
/** @} */                                            // end of MemoryAllocator

//------------------------------------------------------------------------------
/**
 * @defgroup CompilerConstants Compiler Constants
 * @brief Numeric identifiers for supported compilers.
 * @{
 */
//------------------------------------------------------------------------------
#define WP_COMPILER_MSVC 1   /**< Microsoft Visual C++. */
#define WP_COMPILER_GNUC 2   /**< GNU Compiler Collection. */
#define WP_COMPILER_BORL 3   /**< Borland C++. */
#define WP_COMPILER_WINSCW 4 /**< Nokia WinSCW. */
#define WP_COMPILER_GCCE 5   /**< GNU Compiler for Symbian. */
#define WP_COMPILER_CLANG 6  /**< LLVM Clang. */
/** @} */                    // end of CompilerConstants

//------------------------------------------------------------------------------
/**
 * @defgroup MallocDecl Malloc Declaration Attributes
 * @brief Compiler hints for malloc-like functions.
 * @{
 */
//------------------------------------------------------------------------------
#if WP_COMPILER == WP_COMPILER_MSVC
#    define WP_DECL_MALLOC __declspec( restrict ) __declspec( noalias )
#else
#    define WP_DECL_MALLOC __attribute__( ( malloc ) )
#endif
/** @} */  // end of MallocDecl

//------------------------------------------------------------------------------
/**
 * @defgroup CppStandard C++ Standard Version
 * @brief Constants for C++ language standard detection.
 * @{
 */
//------------------------------------------------------------------------------
#define WP_CPP_2020 202002L         /**< C++20 standard version. */
#define WP_CPP_2017 201703L         /**< C++17 standard version. */
#define WP_CPP_2011 201103L         /**< C++11 standard version. */
#define WP_CPP_STANDARD __cplusplus /**< Current C++ standard in use. */
/** @} */                           // end of CppStandard

/**
 * @def WP_ENABLE_ASSERTS
 * @brief Enable or disable engine assertions.
 */
#if WP_ENABLE_ASSERTS
#    if defined( WP_PLATFORM_WIN32 ) && defined( _MSC_VER )
/**
 * @def WP_ASSERT_TRUE
 * @brief Triggers a debug break if the condition is true (Win32/MSVC).
 */
#        include <cstdio>
#        define WP_ASSERT_TRUE( _CONDITION_ ) \
            if( ( _CONDITION_ ) )             \
            {                                 \
                __debugbreak();               \
            }
/**
 * @def WP_ASSERT
 * @brief Triggers a debug break if the condition is false (Win32/MSVC).
 */
#        define WP_ASSERT( _CONDITION_ )                                                          \
            if( !( _CONDITION_ ) )                                                                \
            {                                                                                     \
                std::fprintf( stderr, "WP_ASSERT failed: %s at %s:%d\\n", #_CONDITION_, __FILE__, \
                              __LINE__ );                                                         \
                __debugbreak();                                                                   \
            }
#    else
#        include <assert.h>
/**
 * @def WP_ASSERT_TRUE
 * @brief Asserts that the condition is false (non-Win32).
 */
#        define WP_ASSERT_TRUE( _CONDITION_ ) assert( ( _CONDITION_ ) ? false : true );
/**
 * @def WP_ASSERT
 * @brief Asserts that the condition is true (non-Win32).
 */
#        define WP_ASSERT( _CONDITION_ ) assert( _CONDITION_ );
#    endif
#else
#    define WP_ASSERT_TRUE( _CONDITION_ )
#    define WP_ASSERT( _CONDITION_ )
#endif

/**
 * @def WP_DEPRECATED
 * @brief Marks a function or variable as deprecated.
 */
#ifdef WP_PLATFORM_WIN32
#    define WP_DEPRECATED( x ) __declspec( deprecated ) x
#else
#    define WP_DEPRECATED( x ) x
#endif

/**
 * @def WP_UNUSED
 * @brief Suppresses unused variable warnings.
 */
#ifdef WP_PLATFORM_WIN32
#    define WP_UNUSED( unused ) (void)( ( unused ) )
#else
#    define WP_UNUSED( unused ) (void)( ( unused ) )
#endif

/**
 * @defgroup VersionMacros Version Information
 * @brief Engine version number macros.
 * @{
 */

/**
 * @def WP_VERSION_MAJOR
 * @brief Major version number of the WorkPhone engine.
 * @details Incremented for major releases with breaking changes.
 */
#define WP_VERSION_MAJOR 0

/**
 * @def WP_VERSION_MINOR
 * @brief Minor version number of the WorkPhone engine.
 * @details Incremented for feature releases with backward compatibility.
 */
#define WP_VERSION_MINOR 9

/**
 * @def WP_VERSION_PATCH
 * @brief Patch version number of the WorkPhone engine.
 * @details Incremented for bug fixes and minor improvements.
 */
#define WP_VERSION_PATCH 0

/**
 * @def WP_INTERFACE_API
 * @brief Calling convention for exported engine interfaces.
 */
/**
 * @def WP_INTERFACE_EXPORT
 * @brief Export macro for engine interfaces.
 */
#if defined( __CYGWIN32__ )
#    define WP_INTERFACE_API __stdcall
#    define WP_INTERFACE_EXPORT __declspec( dllexport )
#elif defined( WIN32 ) || defined( _WIN32 ) || defined( __WIN32__ ) || defined( _WIN64 ) || \
    defined( WINAPI_FAMILY )
// #    define WP_INTERFACE_API __stdcall
#    define WP_INTERFACE_API __cdecl
#    define WP_INTERFACE_EXPORT __declspec( dllexport )
#elif defined( __MACH__ ) || defined( __ANDROID__ ) || defined( __linux__ )
#    define WP_INTERFACE_API
#    define WP_INTERFACE_EXPORT
#else
#    define WP_INTERFACE_API
#    define WP_INTERFACE_EXPORT
#endif

#endif  // __WP_CONFIG_H
