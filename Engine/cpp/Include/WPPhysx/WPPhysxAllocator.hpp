#ifndef WPPhysxAllocator_H
#define WPPhysxAllocator_H

/**
 * @file WPPhysxAllocator.hpp
 * @brief Custom allocator that bridges the engine allocator API with PhysX.
 *
 * This header defines `PhysxAllocator`, an implementation of PhysX's
 * `physx::PxAllocatorCallback` that routes memory allocation and
 * deallocation through the engine's allocation system. Using a custom
 * allocator allows the engine to track and control memory used by PhysX,
 * enabling unified memory reporting, custom alignment, and platform-specific
 * allocation policies.
 */

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <foundation/PxAllocatorCallback.h>

namespace workphone
{
    namespace physics
    {
        /**
         * @brief PhysX allocator callback implementation.
         *
         * Implements `physx::PxAllocatorCallback` so PhysX will call into
         * the engine when it needs to allocate or free memory. The engine can
         * then apply its own allocation hooks, tracking, and debugging
         * facilities to memory requests originating from PhysX.
         */
        class WP_PHYSX_API PhysxAllocator : public physx::PxAllocatorCallback
        {
        public:
            /**
             * @brief Construct a new PhysxAllocator.
             *
             * Perform any lightweight initialization required for the
             * allocator. Heavy or platform-specific setup should be done
             * elsewhere if necessary.
             */
            PhysxAllocator();

            /**
             * @brief Destroy the PhysxAllocator.
             *
             * Ensure any allocator-related resources are released. PhysX
             * will not call into this object after it has been destroyed.
             */
            ~PhysxAllocator() override;

            /**
             * @brief Allocate memory for PhysX.
             *
             * This function is called by PhysX to request a block of memory.
             * The implementation should return a pointer to a memory block
             * of at least `size` bytes, or `nullptr` on failure.
             *
             * @param size Number of bytes to allocate.
             * @param typeName Optional allocation type name (may be null).
             * @param filename Optional source filename requesting allocation.
             * @param line Source line number for the allocation request.
             * @return void* Pointer to allocated memory or nullptr.
             */
            void *allocate( size_t size, const c8 *typeName, const c8 *filename, s32 line ) override;

            /**
             * @brief Deallocate memory previously returned by `allocate`.
             *
             * This function is called by PhysX to free memory previously
             * allocated via `allocate`.
             *
             * @param ptr Pointer previously returned by `allocate`.
             */
            void deallocate( void *ptr ) override;
        };
    } // end namespace physics
} // namespace workphone

#endif
