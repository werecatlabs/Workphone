#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/VertexBuffer.hpp>
#include <Workphone/Interface/Mesh/IVertexDeclaration.hpp>
#include <Workphone/Memory/Memory.hpp>
#include <limits>
#include <memory>
#include <stdexcept>
#include <cstring>
#include <vector>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, VertexBuffer, IVertexBuffer );

    VertexBuffer::VertexBuffer() : m_vertexData( 0 )
    {
        m_vertexData.resize( 1 );
    }

    VertexBuffer::~VertexBuffer()
    {
        // CPU buffers also own their data when no manager explicitly unloads them.
        for( auto data : m_vertexData )
            delete[] data;
    }

    void VertexBuffer::unload( SmartPtr<ISharedObject> data )
    {
        for( auto &i : m_vertexData )
        {
            u8 *data = i;
            if( data )
            {
                delete[] data;
                i = nullptr;
            }
        }

        m_vertexData.clear();
        m_vertexBudgets.clear();
        m_vertexBytes.clear();
        m_allocatedDeclaration = nullptr;
        m_allocatedNumVertices = m_numVertices = 0;

        if( m_vertexDeclaration )
        {
            m_vertexDeclaration->unload( nullptr );
            m_vertexDeclaration = nullptr;
        }
    }

    void VertexBuffer::setVertexDeclaration( SmartPtr<IVertexDeclaration> vertexDeclaration )
    {
        m_vertexDeclaration = vertexDeclaration;
    }

    auto VertexBuffer::getVertexDeclaration() const -> SmartPtr<IVertexDeclaration>
    {
        return m_vertexDeclaration;
    }

    void VertexBuffer::setNumVertices( u32 numVertices )
    {
        m_numVertices = numVertices;
    }

    auto VertexBuffer::getNumVertices() const -> u32
    {
        return m_numVertices;
    }

    auto VertexBuffer::createVertexData( s32 index ) -> void *
    {
        try
        {
            if( m_vertexDeclaration && index >= 0 )
            {
                if( index > std::numeric_limits<u16>::max() )
                    throw std::out_of_range( "Vertex stream index exceeds the declaration source range." );
                struct Replacement
                {
                    StreamBudget lease;
                    std::unique_ptr<u8[]> data;
                    u64 bytes = 0;
                };
                const auto newSize = std::max( m_vertexData.size(), size_t( index ) + 1 );
                std::vector<Replacement> replacements( newSize );
                for( size_t source = 0; source < newSize; ++source )
                {
                    const auto previous = source < m_vertexData.size() ? m_vertexData[source] : nullptr;
                    const auto previousBytes = source < m_vertexBytes.size() ? m_vertexBytes[source] : 0;
                    const auto bytes = u64( m_vertexDeclaration->getSize( static_cast<u16>( source ) ) ) * m_numVertices;
                    if( source != size_t( index ) && ( !previous || previousBytes == bytes ) ) continue;
                    if( bytes > std::numeric_limits<size_t>::max() )
                        throw std::length_error( "Vertex allocation exceeds the addressable size." );
                    auto &replacement = replacements[source];
                    String error;
                    replacement.lease = WorldStreamingScheduler::shared()->reserve(
                        StreamMemoryClass::SourceCpu, bytes, error );
                    if( !replacement.lease ) throw std::runtime_error( error.c_str() );
                    replacement.data.reset( new u8[size_t( bytes )] );
                    replacement.bytes = bytes;
                    // Count/layout changes resize other streams in the same
                    // transaction, so they cannot advertise shorter old storage.
                    if( previous && source != size_t( index ) )
                        std::memcpy( replacement.data.get(), previous, size_t( std::min( previousBytes, bytes ) ) );
                }
                // Old and staged allocations stay charged until the entire buffer
                // can commit. A failed admission preserves every stream and lease.
                m_vertexBudgets.resize( newSize );
                m_vertexBytes.resize( newSize );
                m_vertexData.resize( newSize );
                for( size_t source = 0; source < newSize; ++source )
                {
                    auto &replacement = replacements[source];
                    if( !replacement.lease ) continue;
                    delete[] m_vertexData[source];
                    m_vertexData[source] = replacement.data.release();
                    m_vertexBudgets[source] = std::move( replacement.lease );
                    m_vertexBytes[source] = replacement.bytes;
                }
                m_allocatedDeclaration = m_vertexDeclaration;
                m_allocatedNumVertices = m_numVertices;
                return m_vertexData[index];
            }
            if( m_allocatedDeclaration ) throw std::invalid_argument( "Invalid vertex declaration or stream index." );
            return nullptr;
        }
        catch( ... )
        {
            m_numVertices = m_allocatedNumVertices;
            m_vertexDeclaration = m_allocatedDeclaration;
            throw;
        }
    }

    auto VertexBuffer::getVertexData( int index ) const -> void *
    {
        return index >= 0 && size_t( index ) < m_vertexData.size() ? m_vertexData[index] : nullptr;
    }

    auto VertexBuffer::getDataArray() const -> Array<u8 *>
    {
        return m_vertexData;
    }

    auto VertexBuffer::clone() const -> SmartPtr<IVertexBuffer>
    {
        auto newVertexBuffer = workphone::make_ptr<VertexBuffer>();

        newVertexBuffer->setVertexDeclaration( m_vertexDeclaration->clone() );
        newVertexBuffer->setNumVertices( m_numVertices );
        for( size_t source = 0; source < m_vertexData.size(); ++source )
        {
            if( !m_vertexData[source] ) continue;
            auto destination = newVertexBuffer->createVertexData( static_cast<s32>( source ) );
            const auto numBytes = size_t( m_vertexDeclaration->getSize( static_cast<u16>( source ) ) ) * m_numVertices;
            std::memcpy( destination, m_vertexData[source], numBytes );
        }

        return newVertexBuffer;
    }

    auto VertexBuffer::compare( SmartPtr<IVertexBuffer> other ) const -> bool
    {
        if( m_numVertices != other->getNumVertices() )
        {
            return false;
        }

        return m_vertexDeclaration->compare( other->getVertexDeclaration() );
    }
}  // namespace workphone
