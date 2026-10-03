#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/VertexElement.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

namespace workphone
{
    VertexElement::VertexElement() = default;

    VertexElement::VertexElement( u16 source, u32 size, u32 offset, VertexElementSemantic semantic,
                                  VertexElementType type, u8 index ) :
        m_source( source ),
        m_size( size ),
        m_offset( offset ),
        m_semantic( static_cast<u32>( semantic ) ),
        m_type( type ),
        m_index( index )
    {
    }

    VertexElement::~VertexElement() = default;

    void VertexElement::getElementData( void *vertexData, void **elementData ) const
    {
        *elementData = static_cast<void *>( static_cast<unsigned char *>( vertexData ) + m_offset );
    }

    void VertexElement::getElementData( void *vertexData, f32 **elementData ) const
    {
        auto pData = static_cast<u8 *>( vertexData ) + m_offset;
        *elementData = reinterpret_cast<f32 *>( pData );
    }

    auto VertexElement::getOffset() const -> u32
    {
        return m_offset;
    }

    auto VertexElement::getSemantic() const -> u32
    {
        return m_semantic;
    }

    auto VertexElement::getType() const -> VertexElementType
    {
        return m_type;
    }

    auto VertexElement::getIndex() const -> u8
    {
        return m_index;
    }

    void VertexElement::setSource( u16 source )
    {
        m_source = source;
    }

    void VertexElement::setSize( u32 size )
    {
        m_size = size;
    }

    void VertexElement::setOffset( u32 offset )
    {
        m_offset = offset;
    }

    void VertexElement::setSemantic( u32 semantic )
    {
        m_semantic = semantic;
    }

    void VertexElement::setType( VertexElementType type )
    {
        m_type = type;
    }

    void VertexElement::setIndex( u8 index )
    {
        m_index = index;
    }

    auto VertexElement::compare( SmartPtr<IVertexElement> other ) const -> bool
    {
        return *this == *workphone::static_pointer_cast<VertexElement>( other );
    }

    auto VertexElement::operator==( const VertexElement &rhs ) const -> bool
    {
        if( m_type != rhs.m_type || m_index != rhs.m_index || m_offset != rhs.m_offset ||
            m_semantic != rhs.m_semantic || m_source != rhs.m_source )
        {
            return false;
        }

        return true;
    }

    auto VertexElement::getSize() const -> u32
    {
        return m_size;
    }

    auto VertexElement::getSource() const -> u16
    {
        return m_source;
    }
}  // namespace workphone
