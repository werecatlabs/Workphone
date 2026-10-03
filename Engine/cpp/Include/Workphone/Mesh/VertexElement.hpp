#ifndef VertexElement_h__
#define VertexElement_h__

#include <Workphone/Interface/Mesh/IVertexElement.hpp>

namespace workphone
{

    /**
     * @brief Concrete implementation of IVertexElement describing a single vertex attribute.
     *
     * A vertex element describes where and how an attribute (position, normal, texcoord, etc.)
     * is stored inside a vertex stream. This class stores the stream source index, byte size
     * of the element, byte offset inside the vertex, semantic identifier, value type and an
     * optional index for elements that can have multiple entries (e.g. multiple texcoords).
     */
    class WPCore_API VertexElement : public IVertexElement
    {
    public:
        /**
         * @brief Default-constructs a vertex element with sensible defaults.
         *
         * Defaults:
         *  - source = 0
         *  - size = 0
         *  - offset = 0
         *  - semantic = VES_POSITION
         *  - type = VET_FLOAT3
         *  - index = 0
         */
        VertexElement();

        /**
         * @brief Constructs a vertex element with explicit properties.
         *
         * @param source  Index of the vertex buffer source/stream this element belongs to.
         * @param size    Size in bytes of this element within a single vertex.
         * @param offset  Byte offset from the start of the vertex to this element.
         * @param semantic  Semantic describing the meaning of this element (position, normal, etc.).
         * @param type    Storage / interpretation type of the element (float3, color, etc.).
         * @param index   Optional index for elements that can appear multiple times (default 0).
         */
        VertexElement( u16 source, u32 size, u32 offset, VertexElementSemantic semantic,
                       VertexElementType type, u8 index = 0 );

        /**
         * @brief Virtual destructor.
         */
        ~VertexElement() override;

        /**
         * @brief Resolves a raw pointer to the element data inside a vertex buffer.
         *
         * Given a pointer to the start of a vertex instance, this fills @p elementData with
         * a pointer to the element's data (vertex start + offset).
         *
         * @param vertexData    Pointer to the start of the vertex instance.
         * @param elementData   Out parameter receiving a void pointer to the element data.
         */
        void getElementData( void *vertexData, void **elementData ) const override;

        /**
         * @brief Resolves a raw pointer to the element data and returns it as a float pointer.
         *
         * Convenience overload for elements that are float-based. If the element is not stored
         * as floats the returned pointer must be interpreted accordingly by the caller.
         *
         * @param vertexData    Pointer to the start of the vertex instance.
         * @param elementData   Out parameter receiving a float pointer to the element data.
         */
        void getElementData( void *vertexData, f32 **elementData ) const override;

        /**
         * @brief Gets the vertex buffer source/stream index.
         * @return Source index.
         */
        u16 getSource() const override;

        /**
         * @brief Gets the element size in bytes.
         * @return Size in bytes.
         */
        u32 getSize() const override;

        /**
         * @brief Gets the element byte offset within the vertex.
         * @return Byte offset.
         */
        u32 getOffset() const override;

        /**
         * @brief Gets the element semantic as an unsigned integer.
         * @return Semantic id (casted from VertexElementSemantic).
         */
        u32 getSemantic() const override;

        /**
         * @brief Gets the element data type.
         * @return VertexElementType describing the storage/interpretation.
         */
        VertexElementType getType() const override;

        /**
         * @brief Gets the element index (used for multi-entry semantics like texcoords).
         * @return Element index.
         */
        u8 getIndex() const override;

        /**
         * @brief Sets the vertex buffer source/stream index.
         * @param source New source index.
         */
        void setSource( u16 source ) override;

        /**
         * @brief Sets the element size in bytes.
         * @param size New size in bytes.
         */
        void setSize( u32 size ) override;

        /**
         * @brief Sets the element byte offset within the vertex.
         * @param offset New byte offset.
         */
        void setOffset( u32 offset ) override;

        /**
         * @brief Sets the element semantic.
         * @param semantic Semantic id (castable to VertexElementSemantic).
         */
        void setSemantic( u32 semantic ) override;

        /**
         * @brief Sets the element type.
         * @param type New VertexElementType.
         */
        void setType( VertexElementType type ) override;

        /**
         * @brief Sets the element index.
         * @param index New index for multi-entry semantics.
         */
        void setIndex( u8 index ) override;

        /**
         * @brief Equality operator comparing all stored element properties.
         *
         * Compares source, size, offset, semantic, type and index.
         *
         * @param rhs Right-hand side vertex element.
         * @return true if all properties are equal, false otherwise.
         */
        bool operator==( const VertexElement &rhs ) const;

        /**
         * @brief Compares this element with another IVertexElement implementation.
         *
         * Useful when elements are held via interface pointers. Semantics are the same as
         * operator== (compares all stored properties).
         *
         * @param other Smart pointer to another IVertexElement to compare against.
         * @return true if they represent the same element, false otherwise.
         */
        bool compare( SmartPtr<IVertexElement> other ) const override;

    private:
        /** Size in bytes of this element inside a single vertex. */
        u32 m_size = 0;

        /** Byte offset from the start of the vertex to this element. */
        u32 m_offset = 0;

        /** Semantic describing the meaning of the element (default VES_POSITION). */
        u32 m_semantic = static_cast<u32>( VertexElementSemantic::VES_POSITION );

        /** Storage/interpretation type of the element (default VET_FLOAT3). */
        VertexElementType m_type = VertexElementType::VET_FLOAT3;

        /** Source/stream index identifying which vertex buffer this element belongs to. */
        u16 m_source = 0;

        /** Index of the semantic slot (e.g. multiple texture coordinate sets). */
        u8 m_index = 0;
    };
}  // namespace workphone

#endif  // VertexElement_h__
