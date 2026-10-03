#include <WPPhysx/WPPhysxPCH.hpp>
#include "WPPhysx/PhysxMemoryInputStream.hpp"
#include "PxPhysicsAPI.h"
#include "PxExtensionsAPI.h"

namespace workphone::physics
{

    MemoryInputStream::MemoryInputStream() : mData( nullptr ), mSize( 0 )
    {
    }

    MemoryInputStream::MemoryInputStream( physx::PxU8 *data, u32 length, bool manageMemory ) :
        mSize( length ),
        mData( data ),
        mPos( 0 ),
        m_manageMemory( manageMemory )
    {
    }

    MemoryInputStream::~MemoryInputStream()
    {
        if( mData )
        {
            delete[] mData;
        }
    }

    auto MemoryInputStream::read( void *dest, u32 count ) -> u32
    {
        u32 length = physx::PxMin<u32>( count, mSize - mPos );
        memcpy( dest, mData + mPos, length );
        mPos += length;
        return length;
    }

    auto MemoryInputStream::getLength() const -> u32
    {
        return mSize;
    }

    void MemoryInputStream::seek( u32 offset )
    {
        mPos = physx::PxMin<u32>( mSize, offset );
    }

    auto MemoryInputStream::tell() const -> u32
    {
        return mPos;
    }
} // namespace workphone::physics
