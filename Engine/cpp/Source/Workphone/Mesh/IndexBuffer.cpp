#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/IndexBuffer.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>

namespace workphone
{

    IndexBuffer::IndexBuffer()
    {
        m_indexSize = sizeof( u32 );
    }

    IndexBuffer::~IndexBuffer()
    {
        unload( nullptr );
    }

    void IndexBuffer::unload( SmartPtr<ISharedObject> data )
    {
        if( m_indexData )
        {
            delete[] static_cast<u8 *>( m_indexData );
            m_indexData = nullptr;
        }
    }

    void IndexBuffer::setIndexType( Type indexType )
    {
        m_indexType = indexType;

        switch( m_indexType )
        {
        case Type::IT_16BIT:
            m_indexSize = sizeof( u16 );
            break;
        case Type::IT_32BIT:
            m_indexSize = sizeof( u32 );
            break;
        default:
        {
        }
        }
    }

    auto IndexBuffer::getIndexType() const -> IIndexBuffer::Type
    {
        return m_indexType;
    }

    void IndexBuffer::setNumIndices( u32 numIndices )
    {
        m_numIndices = numIndices;
    }

    auto IndexBuffer::getNumIndices() const -> u32
    {
        return m_numIndices;
    }

    auto IndexBuffer::getIndexSize() const -> u32
    {
        return m_indexSize;
    }

    auto IndexBuffer::createIndexData() -> void *
    {
        auto replacement = new u8[size_t( m_numIndices ) * m_indexSize];
        delete[] static_cast<u8 *>( m_indexData );
        m_indexData = replacement;
        return m_indexData;
    }


    auto IndexBuffer::getIndexData() const -> void *
    {
        return m_indexData;
    }

    auto IndexBuffer::clone() const -> SmartPtr<IIndexBuffer>
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto indexBuffer = factoryManager->make_ptr<IndexBuffer>();

        indexBuffer->setIndexType( m_indexType );
        indexBuffer->setNumIndices( m_numIndices );
        indexBuffer->setIndexSize( m_indexSize );
        indexBuffer->createIndexData();

        switch( m_indexType )
        {
        case Type::IT_16BIT:
        {
            auto indices = static_cast<u16 *>( m_indexData );
            auto newIndices = static_cast<u16 *>( indexBuffer->getIndexData() );
            for( u32 i = 0; i < m_numIndices; ++i )
            {
                newIndices[i] = indices[i];
            }
        }
        break;
        case Type::IT_32BIT:
        {
            auto indices = static_cast<u32 *>( m_indexData );
            auto newIndices = static_cast<u32 *>( indexBuffer->getIndexData() );
            for( u32 i = 0; i < m_numIndices; ++i )
            {
                newIndices[i] = indices[i];
            }
        }
        break;
        default:
        {
            WP_ASSERT( false );
        }
        }

        return indexBuffer;
    }

    void IndexBuffer::setIndexSize( u32 size )
    {
        m_indexSize = size;
    }

    auto IndexBuffer::compare( SmartPtr<IIndexBuffer> other ) const -> bool
    {
        return false;
    }
}  // namespace workphone
