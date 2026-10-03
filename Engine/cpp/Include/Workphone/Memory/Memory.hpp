#ifndef __WP_Memory_h__
#define __WP_Memory_h__

/**
 * @file Memory.hpp
 * @brief Lightweight memory utilities and allocation helpers used by the engine.
 *
 * This header provides small convenience helpers for safe deletion,
 * basic memory operations (copy/set/compare) and abstractions for
 * scalable/aligned allocation. All functions are declared as static
 * utilities on the `Memory` class.
 */

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Memory/SafePtr.hpp>
#include <Workphone/Memory/SafeReadPtr.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Memory/WeakPtr.hpp>
#include <Workphone/Memory/AtomicSmartPtr.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Memory/AtomicRawPtr.hpp>
#include <Workphone/Memory/SharedPtr.hpp>
#include <Workphone/Memory/AtomicSharedPtr.hpp>

#if WP_USE_BOOST
#    include <boost/smart_ptr/atomic_shared_ptr.hpp>
#else
#    include <memory>
#endif

#include <cstddef>

namespace workphone
{

    /**
     * @brief Safely deletes a single object pointer and nulls it.
     *
     * Deletes the object pointed to by `data` if non-null, then assigns
     * nullptr to the reference. This helper avoids repeating the common
     * pattern of checking then deleting.
     *
     * @tparam T Type of the object to delete.
     * @param[in,out] data Reference to the pointer to delete and clear.
     */
    template <class T>
    static void WP_SAFE_DELETE( T *&data )
    {
        if( data )
        {
            T *ptr = data;
            delete ptr;

            data = nullptr;
        }
    }

    /**
     * @brief Safely deletes an array pointer and nulls it.
     *
     * Deletes the array pointed to by `data` (using `delete[]`) if non-null,
     * then assigns nullptr to the reference.
     *
     * @tparam T Element type of the array to delete.
     * @param[in,out] data Reference to the pointer to delete and clear.
     */
    template <class T>
    static void WP_SAFE_DELETE_ARRAY( T *&data )
    {
        if( data )
        {
            T *ptr = data;
            delete[] ptr;

            data = nullptr;
        }
    }

    /**
     * @brief General purpose memory utilities.
     *
     * The `Memory` class groups a set of static helpers for copying,
     * setting, comparing memory blocks, and for allocating/freeing memory
     * with (optionally) scalable or aligned allocators. Implementations
     * are platform / project specific and are provided in the corresponding
     * source file.
     */
    class WPCore_API Memory
    {
    public:
        /**
         * @brief Copy `count` bytes from `src` to `dest`.
         *
         * Behaviour mirrors a byte-wise memcpy. The function expects that
         * the destination buffer is large enough to hold `count` bytes.
         *
         * @param[out] dest Destination buffer.
         * @param[in] src Source buffer.
         * @param[in] count Number of bytes to copy (s32).
         */
        static void Memcpy( void *dest, const void *src, s32 count );

        /**
         * @brief Copy `count0` bytes from `src0` to `dest0` using an
         *        alignment friendly algorithm.
         *
         * This variant is intended for memory regions where alignment
         * guarantees allow optimized copy paths (SIMD, word-sized copies).
         * The exact behaviour is implementation-defined.
         *
         * @param[out] dest0 Destination buffer.
         * @param[in] src0 Source buffer.
         * @param[in] count0 Number of bytes to copy (s32).
         */
        static void AlignedMemcpy( void *dest0, const void *src0, s32 count0 );

        /**
         * @brief Fill `count` bytes of `dst` with the byte value `byteValue`.
         *
         * Behaviour mirrors a byte-wise memset. `byteValue` is interpreted as an
         * integer but written as a single byte repeated over `count` bytes.
         *
         * @param[out] dst Destination buffer.
         * @param[in] byteValue Byte value to set (interpreted as s32 but written as a byte).
         * @param[in] count Number of bytes to set (s32).
         */
        static void Memset( void *dst, s32 byteValue, s32 count );

        /**
         * @brief Compare two memory regions.
         *
         * Compares `num` bytes starting at `ptr1` and `ptr2`.
         *
         * @param[in] ptr1 First memory region.
         * @param[in] ptr2 Second memory region.
         * @param[in] num Number of bytes to compare (s32).
         * @return Zero if equal, <0 if ptr1 < ptr2, >0 if ptr1 > ptr2 (same convention as memcmp).
         */
        static s32 Memcmp( const void *ptr1, const void *ptr2, s32 num );

        /**
         * @brief Perform a runtime heap integrity check.
         *
         * This function requests a check of the process heap(s) and returns
         * whether the heap is considered valid. The exact checks performed
         * depend on the platform / runtime.
         *
         * @return true if heap is OK, false if corruption or inconsistency detected.
         */
        static bool CheckHeap();

        /**
         * @brief Allocate memory with the requested alignment from a scalable allocator.
         *
         * Use the corresponding `ScalableAlignedFree` to free memory returned by this call.
         * The function may use a scalable allocator (tbbmalloc/hoard/other) if configured;
         * otherwise it falls back to a platform allocator.
         *
         * @param[in] size Number of bytes to allocate.
         * @param[in] alignment Required alignment in bytes (must be a power of two).
         * @return Pointer to the allocated memory, or nullptr on failure.
         */
        static void *ScalableAlignedMalloc( size_t size, size_t alignment );

        /**
         * @brief Free memory allocated by `ScalableAlignedMalloc`.
         *
         * @param[in] ptr Pointer previously returned by `ScalableAlignedMalloc`.
         */
        static void ScalableAlignedFree( void *ptr );

        /**
         * @brief Allocate memory from a scalable allocator.
         *
         * Use `ScalableFree` to release memory returned by this call.
         *
         * @param[in] size Number of bytes to allocate.
         * @return Pointer to the allocated memory, or nullptr on failure.
         */
        static void *ScalableMalloc( size_t size );

        /**
         * @brief Free memory allocated by `ScalableMalloc`.
         *
         * @param[in] ptr Pointer previously returned by `ScalableMalloc`.
         */
        static void ScalableFree( void *ptr );
    };

}  // namespace workphone

#endif  // Memory_h__
