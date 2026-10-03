#ifndef MemoryPool_h__
#define MemoryPool_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <cstddef>
#include <iostream>

namespace workphone
{
    /**
     * @file MemoryPool.hpp
     * @brief Simple fixed-size block memory pool allocator.
     *
     * This class implements a very small, performant pool allocator that hands out
     * fixed-size blocks from a contiguous buffer. It is intended for fast
     * allocation/deallocation of many objects of the same size.
     *
     * Usage notes:
     * - The pool does not call constructors/destructors for objects stored in the blocks.
     * - The allocator performs constant-time allocate/deallocate operations.
     * - The class does not perform thread synchronization; use external locking
     *   for concurrent access.
     */
    class WPCore_API MemoryPool
    {
    public:
        /**
         * @brief Create an empty MemoryPool.
         *
         * The default constructor creates a pool with size 0. You may want to
         * construct and then reinitialize (or replace) before first use.
         */
        MemoryPool();

        /**
         * @brief Create a MemoryPool that manages `block_count` blocks of `block_size` bytes.
         *
         * @param block_size  Size in bytes of each block handed out by this pool.
         * @param block_count Number of blocks to reserve in the pool.
         *
         * @note block_size should be large enough and appropriately aligned for the objects you intend
         * to place in the pool.
         * @note If allocation of the underlying buffer fails, behaviour depends on the platform (may
         * throw or std::bad_alloc).
         */
        MemoryPool( u32 block_size, u32 block_count );

        /**
         * @brief Destroy the memory pool and free its buffer.
         *
         * Any outstanding pointers returned by allocate() become invalid after the destructor runs.
         */
        ~MemoryPool();

        /**
         * @brief Allocate a block from the pool.
         *
         * Returns a pointer to a free block of size at least `block_size_`.
         *
         * @return Pointer to the allocated block, or nullptr if the pool is exhausted.
         *
         * @warning The returned memory is raw storage; constructors are not invoked.
         * @warning Passing the pointer to anything other than deallocate() (or freeing it manually)
         *          results in undefined behaviour.
         *
         * Complexity: O(1)
         */
        void *allocate();

        /**
         * @brief Return a previously allocated block to the pool.
         *
         * @param block Pointer previously returned from allocate().
         *
         * @warning Passing a pointer that was not allocated by this pool, or a pointer
         *          that has already been deallocated, results in undefined behaviour.
         *
         * Complexity: O(1)
         */
        void deallocate( void *block );

    private:
        /// Size in bytes of each block managed by the pool.
        std::size_t block_size_;

        /// Total number of blocks in the pool.
        std::size_t block_count_;

        /// Pointer to the start of the contiguous pool buffer.
        char *pool_;

        /**
         * @brief Pointer used internally to track the head of the free-list.
         *
         * Implementation detail: the free-list is typically stored using the freed
         * blocks themselves to contain the next-pointer. `head_` points to the
         * beginning of that free-list.
         */
        char *head_;
    };
}  // namespace workphone

#endif  // MemoryPool_h__
