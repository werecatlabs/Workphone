#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/IndexBuffer.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <limits>
#include <memory>
#include <stdexcept>

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
        m_indexBudget.reset();
        m_allocatedNumIndices = m_numIndices = 0;
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
        try
        {
            const auto numBytes = u64( m_numIndices ) * m_indexSize;
            if( numBytes > std::numeric_limits<size_t>::max() )
                throw std::length_error( "Index allocation exceeds the addressable size." );
            String error;
            auto lease = WorldStreamingScheduler::shared()->reserve(
                StreamMemoryClass::SourceCpu, numBytes, error );
            if( !lease ) throw std::runtime_error( error.c_str() );
            std::unique_ptr<u8[]> replacement( new u8[size_t( numBytes )] );
            delete[] static_cast<u8 *>( m_indexData );
            m_indexData = replacement.release();
            m_indexBudget = std::move( lease );
            m_allocatedNumIndices = m_numIndices;
            m_allocatedIndexSize = m_indexSize;
            m_allocatedIndexType = m_indexType;
            return m_indexData;
        }
        catch( ... )
        {
            m_numIndices = m_allocatedNumIndices;
            m_indexSize = m_allocatedIndexSize;
            m_indexType = m_allocatedIndexType;
            throw;
        }
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
