#ifndef __IVertexElement_h__
#define __IVertexElement_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @brief A description of a single element in a vertex buffer.
     * This class describes the format of a single element in a vertex buffer,
     * including the data type, offset, and source of the element within the buffer.
     */
    class WPCore_API IVertexElement : public ISharedObject
    {
    public:
        IVertexElement();

        /** Destructor. */
        ~IVertexElement() override;

        /**
         * Copies the data for this element from the given vertex buffer to the given element buffer.
         * @param vertexData The vertex buffer.
         * @param elementData The element buffer.
         */
        virtual void getElementData( void *vertexData, void **elementData ) const = 0;

        /**
         * Copies the data for this element from the given vertex buffer to the given element buffer.
         * @param vertexData The vertex buffer.
         * @param elementData The element buffer.
         */
        virtual void getElementData( void *vertexData, f32 **elementData ) const = 0;

        /**
         * Gets the index of the buffer that this element comes from.
         * @return The buffer index.
         */
        virtual u16 getSource() const = 0;

        /**
         * Sets the index of the buffer that this element comes from.
         * @param source The buffer index.
         */
        virtual void setSource( u16 source ) = 0;

        /**
         * Gets the size of this element.
         * @return The size of this element.
         */
        virtual u32 getSize() const = 0;

        /**
         * Sets the size of this element.
         * @param size The size of this element.
         */
        virtual void setSize( u32 size ) = 0;

        /**
         * Gets the offset of this element from the start of the vertex buffer.
         * @return The offset of this element.
         */
        virtual u32 getOffset() const = 0;

        /**
         * Sets the offset of this element from the start of the vertex buffer.
         * @param offset The offset of this element.
         */
        virtual void setOffset( u32 offset ) = 0;

        /**
         * Gets the semantic of this element.
         * @return The semantic of this element.
         */
        virtual u32 getSemantic() const = 0;

        /**
         * Sets the semantic of this element.
         * @param semantic The semantic of this element.
         */
        virtual void setSemantic( u32 semantic ) = 0;

        /**
         * Gets the type of this element.
         * @return The type of this element.
         */
        virtual VertexElementType getType() const = 0;

        /**
         * Sets the type of this element.
         * @param type The type of this element.
         */
        virtual void setType( VertexElementType type ) = 0;

        /**
         * Gets the index of this element.
         * @return The index of this element.
         */
        virtual u8 getIndex() const = 0;

        /**
         * Sets the index of this element.
         * @param index The index of this element.
         */
        virtual void setIndex( u8 index ) = 0;

        /**
         * Compares this element to another element to see if they are equal.
         * @param other The element to compare to.
         * @return True if the elements are equal, false otherwise.
         */
        virtual bool compare( SmartPtr<IVertexElement> other ) const = 0;

        /**
         * Converts a pointer to the base vertex buffer into a pointer to this element.
         * @tparam T The type of the element.
         * @param pBase Pointer to the base vertex buffer.
         * @param pElem Pointer to the pointer to the element.
         */
        template <typename T>
        void baseVertexPointerToElement( void *pBase, T **pElem ) const;

        WP_CLASS_REGISTER_DECL;
    };

    template <typename T>
    void IVertexElement::baseVertexPointerToElement( void *pBase, T **pElem ) const
    {
        auto offset = getOffset();
        *pElem = reinterpret_cast<T *>( static_cast<char *>( pBase ) + offset );
    }

}  // namespace workphone

#endif  // IVertexElement_h__
