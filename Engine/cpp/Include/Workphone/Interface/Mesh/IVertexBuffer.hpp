#ifndef IVertexBuffer_h__
#define IVertexBuffer_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * An interface for a vertex buffer.
     */
    class WPCore_API IVertexBuffer : public ISharedObject
    {
    public:
        IVertexBuffer();

        /** Virtual destructor. */
        ~IVertexBuffer() override;

        /**
         * Sets the vertex declaration.
         * @param vertexDeclaration The vertex declaration to set.
         */
        virtual void setVertexDeclaration( SmartPtr<IVertexDeclaration> vertexDeclaration ) = 0;

        /**
         * Gets the vertex declaration.
         * @return The vertex declaration.
         */
        virtual SmartPtr<IVertexDeclaration> getVertexDeclaration() const = 0;

        /**
         * Sets the number of vertices.
         * @param numVertices The number of vertices to set.
         */
        virtual void setNumVertices( u32 numVertices ) = 0;

        /**
         * Gets the number of vertices.
         * @return The number of vertices.
         */
        virtual u32 getNumVertices() const = 0;

        /**
         * Creates the vertex data.
         * @param index The index of the vertex data.
         * @return A pointer to the created vertex data.
         */
        virtual void *createVertexData( s32 index = 0 ) = 0;

        /**
         * Gets the vertex data.
         * @param index The index of the vertex data.
         * @return A pointer to the vertex data.
         */
        virtual void *getVertexData( s32 index = 0 ) const = 0;

        /**
         * Gets an array of all the vertex data.
         * @return The array of vertex data.
         */
        virtual Array<u8 *> getDataArray() const = 0;

        /**
         * Creates a clone of the vertex buffer.
         * @return The cloned vertex buffer.
         */
        virtual SmartPtr<IVertexBuffer> clone() const = 0;

        /**
         * Compares the vertex buffer to another vertex buffer.
         * @param other The other vertex buffer to compare to.
         * @return Whether the vertex buffer is equal to the other vertex buffer.
         */
        virtual bool compare( SmartPtr<IVertexBuffer> other ) const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IVertexBuffer_h__
