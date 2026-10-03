#include <WPPhysx/WPPhysxPCH.hpp>
#include "WPPhysx/PhysxMemoryOutputStream.hpp"
#include "PxPhysicsAPI.h"
#include "PxExtensionsAPI.h"

namespace workphone::physics
{
    MemoryOutputStream::MemoryOutputStream() : mData( nullptr ), mSize( 0 ), mCapacity( 0 )
    {
    }

    MemoryOutputStream::~MemoryOutputStream()
    {
        if( mData )
        {
            delete[] mData;
        }
    }

    auto MemoryOutputStream::write( const void *src, u32 size ) -> u32
    {
        auto expectedSize = mSize + size;
        if( expectedSize > mCapacity )
        {
            mCapacity = expectedSize + ( 4096 * 8 );

            auto newData = new physx::PxU8[mCapacity];
            PX_ASSERT( newData != NULL );

            if( newData )
            {
                memcpy( newData, mData, mSize );
                delete[] mData;
            }

            mData = newData;
        }

        memcpy( mData + mSize, src, size );
        mSize += size;
        return size;
    }

    auto MemoryOutputStream::getSize() const -> u32
    {
        return mSize;
    }

    auto MemoryOutputStream::getData() const -> physx::PxU8 *
    {
        return mData;
    }
} // namespace workphone::physics
