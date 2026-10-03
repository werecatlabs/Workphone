#ifndef WPPhysxPoolAllocator_h__
#define WPPhysxPoolAllocator_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include "foundation/PxAllocatorCallback.h"
#include <list>
#include <mutex>
#include <vector>

/**
 * @file WPPhysxPoolAllocator.hpp
 * @brief A simple pool allocator used by the PhysX integration.
 *
 * This allocator implements `physx::PxAllocatorCallback` and provides a
 * fixed-size memory pool with basic free-block bookkeeping. It is intended
 * to satisfy small and frequent allocations made by the PhysX runtime while
 * avoiding the overhead of the general-purpose heap. The allocator is
 * internally synchronized and safe for use from multiple threads.
 */
namespace workphone
{
    namespace physics
    {

        /**
         * @brief Pool-based allocator for PhysX
         *
         * `PhysxPoolAllocator` implements the `physx::PxAllocatorCallback`
         * interface so it can be supplied to the PhysX SDK. It maintains an
         * internal byte buffer (`mPool`) and a small list of free blocks
         * (`mFreeBlocks`) to reuse previously freed memory. A mutex protects
         * internal state for concurrent access.
         */
        class WP_PHYSX_API PhysxPoolAllocator : public physx::PxAllocatorCallback
        {
        public:
            /**
             * @brief Construct an allocator with a reasonable default pool size.
             *
             * The default constructor creates an allocator using the library's
             * default pool size and a default alignment of 16 bytes.
             */
            PhysxPoolAllocator();

            /**
             * @brief Construct an allocator with a custom pool size.
             *
             * @param poolSize Size in bytes of the internal pool buffer.
             *                 Larger values reduce the chance of falling back
             *                 to the global allocator.
             */
            PhysxPoolAllocator( size_t poolSize );

            /**
             * @brief Construct an allocator with a custom pool size and alignment.
             *
             * @param poolSize  Size in bytes of the internal pool buffer.
             * @param alignment Alignment in bytes to be used for returned
             *                  allocations. Must be a power of two.
             */
            PhysxPoolAllocator( size_t poolSize, size_t alignment );

            /**
             * @brief Destructor.
             *
             * Releases internal resources. Any outstanding allocations that
             * remain live at destruction are not explicitly tracked here and
             * behavior is undefined if PhysX still holds references.
             */
            ~PhysxPoolAllocator() override;

            /**
             * @brief Allocate memory.
             *
             * This method satisfies the `physx::PxAllocatorCallback` API and
             * attempts to allocate memory from the internal pool. If the pool
             * cannot satisfy the request, the implementation may fall back to
             * the global allocator (behavior defined in the .cpp).
             *
             * @param size      Number of bytes to allocate.
             * @param typeName  Optional name of the allocation type (for
             *                  debugging/profiling). May be null.
             * @param filename  Source filename where the allocation originated
             *                  (for debugging). May be null.
             * @param line      Source line number where the allocation
             *                  originated (for debugging).
             * @return Pointer to an aligned memory block of at least `size`
             *         bytes, or nullptr on failure.
             */
            void *allocate( size_t size, const char *typeName, const char *filename, int line ) override;

            /**
             * @brief Deallocate memory previously returned by `allocate`.
             *
             * The pointer `ptr` should be one previously returned by
             * `allocate`. Freed blocks are inserted into `mFreeBlocks` for
             * reuse.
             *
             * @param ptr Pointer to memory to free. If null, the call is a no-op.
             */
            void deallocate( void *ptr ) override;

        private:
            /**
             * Internal contiguous pool buffer used for satisfying allocations.
             */
            std::vector<char> mPool;

            /**
             * Current allocation cursor (offset into `mPool`). When an
             * allocation is performed, this index is advanced by the allocated
             * size (including any padding needed for alignment).
             */
            size_t mPoolIndex = 0;

            /**
             * Alignment (in bytes) used for returned allocations. Defaults to
             * 16 which is commonly suitable for SIMD and platform
             * requirements.
             */
            size_t mAlignment = 16;

            /**
             * A list of free blocks stored as pairs of <offset, size>. Freed
             * blocks are pushed into this list and reused for later
             * allocations. The representation uses offsets into `mPool`.
             */
            std::list<std::pair<size_t, size_t>> mFreeBlocks;

            /**
             * Mutex protecting the allocator state to make allocation and
             * deallocation thread-safe.
             */
            std::mutex mMutex;
        };

    } // end namespace physics
} // namespace workphone

#endif // WPPhysxPoolAllocator_h__
