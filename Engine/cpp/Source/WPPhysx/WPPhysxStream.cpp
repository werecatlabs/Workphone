#include <WPPhysx/WPPhysxPCH.hpp>
#include "WPPhysx/WPPhysxStream.hpp"
#include "PxPhysicsAPI.h"
#include "extensions/PxExtensionsAPI.h"

namespace workphone::physics
{
    using namespace physx;

    PhysxStream::PhysxStream() = default;

    PhysxStream::PhysxStream( SmartPtr<IStream> ds )
    {
        m_dataStream = ds;
    }

    PhysxStream::~PhysxStream() = default;

    auto PhysxStream::readByte() const -> PxU8
    {
        PxU8 byte = 0;
        m_dataStream->read( &byte, sizeof( PxU8 ) );
        return byte;
    }

    auto PhysxStream::readWord() const -> PxU16
    {
        PxU16 word = 0;
        m_dataStream->read( &word, sizeof( PxU16 ) );
        return word;
    }

    auto PhysxStream::readDword() const -> PxU32
    {
        PxU32 dword = 0;
        m_dataStream->read( &dword, sizeof( PxU32 ) );
        return dword;
    }

    auto PhysxStream::readFloat() const -> float
    {
        float f = 0;
        m_dataStream->read( &f, sizeof( float ) );
        return f;
    }

    auto PhysxStream::readDouble() const -> double
    {
        double d = 0;
        m_dataStream->read( &d, sizeof( double ) );
        return d;
    }

    void PhysxStream::readBuffer( void *buffer, PxU32 size ) const
    {
        m_dataStream->read( buffer, size );
    }

    auto PhysxStream::storeByte( PxU8 b ) -> PxInputStream &
    {
        m_dataStream->write( &b, sizeof( PxU8 ) );
        return *this;
    }

    auto PhysxStream::storeWord( PxU16 w ) -> PxInputStream &
    {
        m_dataStream->write( &w, sizeof( PxU16 ) );
        return *this;
    }

    auto PhysxStream::storeDword( PxU32 d ) -> PxInputStream &
    {
        m_dataStream->write( &d, sizeof( PxU32 ) );
        return *this;
    }

    auto PhysxStream::storeFloat( PxReal f ) -> PxInputStream &
    {
        m_dataStream->write( &f, sizeof( PxReal ) );
        return *this;
    }

    auto PhysxStream::storeDouble( PxF64 f ) -> PxInputStream &
    {
        m_dataStream->write( &f, sizeof( PxU64 ) );
        return *this;
    }

    auto PhysxStream::storeBuffer( const void *buffer, PxU32 size ) -> PxInputStream &
    {
        m_dataStream->write( buffer, size );
        return *this;
    }

    auto PhysxStream::getOgreDataStream() -> SmartPtr<IStream>
    {
        return m_dataStream;
    }

    auto PhysxStream::read( void *dest, PxU32 count ) -> PxU32
    {
        return static_cast<u32>( m_dataStream->read( dest, count ) );
    }
} // namespace workphone::physics
