#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/VertexBuffer.hpp>
#include <Workphone/Interface/Mesh/IVertexDeclaration.hpp>
#include <Workphone/Memory/Memory.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, VertexBuffer, IVertexBuffer );

    VertexBuffer::VertexBuffer() : m_vertexData( 0 )
    {
        m_vertexData.resize( 1 );
    }

    VertexBuffer::~VertexBuffer() = default;

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
        if( m_vertexDeclaration )
        {
            auto newSize = index + 1;
            if( newSize >= static_cast<s32>( m_vertexData.size() ) )
            {
                auto copy = m_vertexData;
                m_vertexData.resize( newSize );

                for( u32 i = 0; i < copy.size(); ++i )
                {
                    m_vertexData[i] = copy[i];
                }
            }

            auto vertexSize = m_vertexDeclaration->getSize( index );
            auto numBytes = vertexSize * m_numVertices;
            auto buffer = new u8[numBytes];
            m_vertexData[index] = buffer;
            return buffer;
        }

        return nullptr;
    }

    auto VertexBuffer::getVertexData( int index ) const -> void *
    {
        return m_vertexData[index];
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
        newVertexBuffer->createVertexData();

        u32 numBytes = m_vertexDeclaration->getSize() * m_numVertices;
        Memory::Memcpy( newVertexBuffer->m_vertexData[0], m_vertexData[0], numBytes );

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
