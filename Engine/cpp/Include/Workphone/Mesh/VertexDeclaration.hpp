#ifndef CVertexDeclaration_h__
#define CVertexDeclaration_h__

#include <Workphone/Interface/Mesh/IVertexDeclaration.hpp>

namespace workphone
{

    /**
     * @brief Concrete implementation of the IVertexDeclaration interface.
     *
     * A vertex declaration describes the memory layout of vertex buffer data by
     * holding a collection of vertex elements. Each element describes one attribute
     * (position, normal, texcoord, etc.) and how it is stored within a vertex buffer
     * source. This class manages the collection of elements and provides utility
     * operations such as element lookup, cloning and size/stride computation per
     * vertex buffer source.
     *
     * Notes:
     * - Elements are stored in insertion order.
     * - This class does not perform GPU resource binding; it only describes layout.
     */
    class WPCore_API VertexDeclaration : public IVertexDeclaration
    {
    public:
        /**
         * @brief Construct an empty vertex declaration.
         *
         * Initializes an empty container of vertex elements.
         */
        VertexDeclaration();

        /**
         * @brief Virtual destructor.
         *
         * Destroys the vertex declaration and releases any owned element objects.
         */
        ~VertexDeclaration() override;

        /**
         * @copydoc IVertexDeclaration::unload
         *
         * Implementation note: unload will release or reset internal data associated
         * with the declaration when the owning shared object is unloaded.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Add a new vertex element to this declaration.
         *
         * Creates and appends a new IVertexElement describing an attribute stored
         * in the specified vertex buffer source.
         *
         * @param source The index of the vertex buffer source (stream) this element
         *               belongs to.
         * @param offset Byte offset from the start of the vertex to the element data.
         * @param elementSemantic Semantic describing the role of the element
         *                        (e.g., position, normal, texcoord).
         * @param elementType Data type/format of the element (e.g., float3, ubyte4).
         * @param index Semantic index used when multiple elements share the same semantic.
         * @return SmartPtr<IVertexElement> A smart pointer to the newly created element.
         */
        SmartPtr<IVertexElement> addElement( u16 source, u32 offset,
                                             VertexElementSemantic elementSemantic,
                                             VertexElementType elementType, u32 index = 0 ) override;

        /**
         * @brief Compute the stride (size in bytes) for the specified vertex buffer source.
         *
         * The stride is computed as the maximum offset + size of the element for the
         * given source. If no elements exist for the source, the result is 0.
         *
         * @param source The vertex buffer source index to calculate the stride for.
         *               Defaults to 0.
         * @return u32 The size in bytes of a single vertex for the given source.
         */
        u32 getSize( u16 source = 0 ) const override;

        /**
         * @brief Find the first element matching the given semantic and index.
         *
         * @param elementSemantic The semantic to search for.
         * @param index The semantic index when there are multiple elements with the same semantic.
         * @return SmartPtr<IVertexElement> Smart pointer to the matching element, or nullptr if not
         * found.
         */
        SmartPtr<IVertexElement> findElementBySemantic( VertexElementSemantic elementSemantic,
                                                        u32 index = 0 ) override;

        /**
         * @brief Retrieve all elements that reference the specified vertex buffer source.
         *
         * The returned array contains copies of the smart pointers to elements that
         * belong to the requested source. The order of elements matches their insertion order.
         *
         * @param source The vertex buffer source index to filter elements by.
         * @return Array<SmartPtr<IVertexElement>> Array of element smart pointers (by value).
         */
        Array<SmartPtr<IVertexElement>> findElementsBySource( u16 source ) const override;

        /**
         * @brief Create a deep copy of this vertex declaration.
         *
         * The cloned declaration will contain cloned vertex elements so that the
         * result can be modified independently of the original.
         *
         * @return SmartPtr<IVertexDeclaration> Smart pointer to the cloned declaration.
         */
        SmartPtr<IVertexDeclaration> clone() const override;

        /**
         * @brief Get a modifiable reference to the underlying element container.
         *
         * Use with care: modifying the returned array will change this declaration.
         *
         * @return Array<SmartPtr<IVertexElement>>& Reference to the internal element array.
         */
        Array<SmartPtr<IVertexElement>> &getElements() override;

        /**
         * @brief Get a read-only reference to the underlying element container.
         *
         * @return const Array<SmartPtr<IVertexElement>>& Const reference to the internal element array.
         */
        const Array<SmartPtr<IVertexElement>> &getElements() const override;

        /**
         * @brief Compare this declaration to another for semantic and layout equality.
         *
         * Two declarations are considered equal if they have the same number of elements
         * and each corresponding element matches in source, offset, semantic, type and index.
         *
         * @param other Smart pointer to the other declaration to compare against.
         * @return bool true if equal, false otherwise.
         */
        bool compare( SmartPtr<IVertexDeclaration> other ) const override;

    private:
        /** Container holding the vertex elements in insertion order. */
        Array<SmartPtr<IVertexElement>> m_elements;
    };
}  // namespace workphone

#endif  // CVertexDeclaration_h__
