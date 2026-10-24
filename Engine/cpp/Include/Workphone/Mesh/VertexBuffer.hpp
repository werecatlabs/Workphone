#ifndef __FBVertexBuffer__H
#define __FBVertexBuffer__H

#include <Workphone/Interface/Mesh/IVertexBuffer.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>

namespace workphone
{

    /** Implementation of a vertex buffer. */
    class WPCore_API VertexBuffer : public IVertexBuffer, public SourceBudgetObject
    {
    public:
        using SourceBudgetObject::operator new;
        using SourceBudgetObject::operator delete;
        /** Constructor. */
        VertexBuffer();

        /** Destructor. */
        ~VertexBuffer() override;

        /** @copydoc IVertexBuffer::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc IVertexBuffer::setVertexDeclaration */
        void setVertexDeclaration( SmartPtr<IVertexDeclaration> vertexDeclaration ) override;

        /** @copydoc IVertexBuffer::getVertexDeclaration */
        SmartPtr<IVertexDeclaration> getVertexDeclaration() const override;

        /** @copydoc IVertexBuffer::setNumVertices */
        void setNumVertices( u32 numVertices ) override;

        /** @copydoc IVertexBuffer::getNumVertices */
        u32 getNumVertices() const override;

        /** @copydoc IVertexBuffer::createVertexData */
        void *createVertexData( s32 index = 0 ) override;

        /** @copydoc IVertexBuffer::getVertexData */
        void *getVertexData( s32 index = 0 ) const override;

        /** @copydoc IVertexBuffer::getDataArray */
        Array<u8 *> getDataArray() const override;

        /** @copydoc IVertexBuffer::clone */
        SmartPtr<IVertexBuffer> clone() const override;

        /** @copydoc IVertexBuffer::compare */
        bool compare( SmartPtr<IVertexBuffer> other ) const override;

        WP_CLASS_REGISTER_DECL;

    protected:
        // The vertex declaration.
        SmartPtr<IVertexDeclaration> m_vertexDeclaration;

        // The vertex data.
        Array<u8 *> m_vertexData;

        // Each allocation owns its lease. Mesh/resource aliases share the same
        // buffer and cannot duplicate or prematurely release its reservation.
        Array<StreamBudget> m_vertexBudgets;
        Array<u64> m_vertexBytes;
        SmartPtr<IVertexDeclaration> m_allocatedDeclaration;
        u32 m_allocatedNumVertices = 0;

        // The number of vertices.
        u32 m_numVertices = 0;
    };
}  // namespace workphone

#endif
