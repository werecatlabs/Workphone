#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxAllocator.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{

    PhysxAllocator::PhysxAllocator() = default;

    PhysxAllocator::~PhysxAllocator() = default;

    auto PhysxAllocator::allocate( size_t size, const c8 *typeName, const c8 *filename, s32 line )
        -> void *
    {
        void *ptr = Memory::ScalableAlignedMalloc( size, 16 );
        if( !ptr )
        {
            std::terminate();
        }

#if WP_PHYSX_DEBUG
#    if WP_ENABLE_MEMORY_TRACKER
        auto &memoryTracker = MemoryTracker::get();
        memoryTracker._recordAlloc( ptr, size, 0, filename, line, typeName );
#    endif
#endif

        return ptr;
    }

    void PhysxAllocator::deallocate( void *ptr )
    {
        if( ptr )
        {
            Memory::ScalableAlignedFree( ptr );

#if WP_PHYSX_DEBUG
#    if WP_ENABLE_MEMORY_TRACKER
            auto &memoryTracker = MemoryTracker::get();
            memoryTracker._recordDealloc( ptr );
#    endif
#endif
        }
    }
} // namespace workphone::physics
