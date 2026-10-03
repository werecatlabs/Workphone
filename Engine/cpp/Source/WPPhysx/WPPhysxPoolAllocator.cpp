#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxPoolAllocator.hpp>
#include <Workphone/Workphone.hpp>
#include <foundation/PxAssert.h>
#include <foundation/PxErrorCallback.h>
#include <cstdio>
#include <cassert>

#if WP_USE_TBB
#    include <tbb/scalable_allocator.h>
#endif

namespace workphone::physics
{
    PhysxPoolAllocator::PhysxPoolAllocator()
    {
        auto poolSize = 128 * 1024 * 1024;
        mPool.resize( poolSize );
    }

    PhysxPoolAllocator::PhysxPoolAllocator( size_t poolSize )
    {
        mPool.resize( poolSize );
    }

    PhysxPoolAllocator::PhysxPoolAllocator( size_t poolSize, size_t alignment ) : mAlignment( alignment )
    {
        mPool.resize( poolSize );
    }

    PhysxPoolAllocator::~PhysxPoolAllocator() = default;

    auto PhysxPoolAllocator::allocate( size_t size, const char *typeName, const char *filename,
                                       int line ) -> void *
    {
        std::lock_guard<std::mutex> lock( mMutex );

        // Allocate memory from the pool if there is a free block of sufficient size
        for( auto it = mFreeBlocks.begin(); it != mFreeBlocks.end(); ++it )
        {
            auto block = ( *it );
            if( block.second >= size )
            {
                auto ptr = &mPool[block.first];
                mFreeBlocks.erase( it );

                memset( ptr, 0, block.second );
                return ptr;
            }
        }

        // Calculate the size of the memory block, including alignment padding
        size_t alignedSize = ( size + mAlignment - 1 ) & ~( mAlignment - 1 );

        // Otherwise, allocate memory from the end of the pool
        if( mPoolIndex + alignedSize <= mPool.size() )
        {
            void *ptr = &mPool[mPoolIndex];
            mPoolIndex += alignedSize;
            return ptr;
        }

        // Out of memory
        return nullptr;
    }

    void PhysxPoolAllocator::deallocate( void *ptr )
    {
        if( ptr )
        {
            std::lock_guard<std::mutex> lock( mMutex );

            // Deallocate memory back to the pool
            size_t offset = reinterpret_cast<char *>( ptr ) - &mPool[0];
            size_t size = mPoolIndex - offset;
            WP_ASSERT( size < 2 * 1024 * 1024 );

            mFreeBlocks.emplace_back( offset, size );
            mPoolIndex = offset;

            memset( ptr, 0, size );
        }
    }
} // namespace workphone::physics
