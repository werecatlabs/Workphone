#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>

#if WP_GRAPHICS_SYSTEM_OGRENEXT

#    include <imgui.h>
#    include "WPGraphicsOgreNext/ImguiRenderableOgreNext.hpp"

#    include <OgreHardwareBufferManager.h>
#    include <OgreHardwareVertexBuffer.h>
#    include <OgreHardwareIndexBuffer.h>

namespace workphone::render
{

    ImguiRenderableOgreNext::ImguiRenderableOgreNext()
    {
        // use identity projection and view matrices
        mUseIdentityProjection = true;
        mUseIdentityView = true;

        //By default we want ImguiRenderables to still work in wireframe mode
        mPolygonModeOverrideable = false;

        mRenderOp.vertexData = OGRE_NEW Ogre::v1::VertexData( nullptr );
        mRenderOp.indexData = OGRE_NEW Ogre::v1::IndexData();

        mRenderOp.vertexData->vertexCount = 0;
        mRenderOp.vertexData->vertexStart = 0;

        mRenderOp.indexData->indexCount = 0;
        mRenderOp.indexData->indexStart = 0;
        //mRenderOp.operationType = Ogre::v1::RenderOperation::OT_TRIANGLE_LIST;
        mRenderOp.operationType = Ogre::OperationType::OT_TRIANGLE_LIST;
        mRenderOp.useIndexes = true;
        mRenderOp.useGlobalInstancingVertexBufferIsAvailable = false;

        Ogre::v1::VertexDeclaration *decl = mRenderOp.vertexData->vertexDeclaration;

        // vertex declaration
        size_t offset = 0;
        decl->addElement( 0, offset, Ogre::VET_FLOAT2, Ogre::VES_POSITION );
        offset += Ogre::v1::VertexElement::getTypeSize( Ogre::VET_FLOAT2 );
        decl->addElement( 0, offset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES, 0 );
        offset += Ogre::v1::VertexElement::getTypeSize( Ogre::VET_FLOAT2 );
        decl->addElement( 0, offset, Ogre::VET_COLOUR, Ogre::VES_DIFFUSE );
    }

    ImguiRenderableOgreNext::~ImguiRenderableOgreNext()
    {
        OGRE_DELETE mRenderOp.vertexData;
        OGRE_DELETE mRenderOp.indexData;
    }

    void ImguiRenderableOgreNext::updateVertexData( const ImDrawVert *vtxBuf, const ImDrawIdx *idxBuf,
                                                    unsigned int vtxCount, unsigned int idxCount )
    {
        mVertexCount = vtxCount;
        mIndexCount = idxCount;
        mRenderOp.vertexData->vertexStart = 0;
        mRenderOp.vertexData->vertexCount = vtxCount;
        mRenderOp.indexData->indexStart = 0;
        mRenderOp.indexData->indexCount = idxCount;

        if( vtxCount == 0 || idxCount == 0 )
            return;

        Ogre::v1::VertexBufferBinding *bind = mRenderOp.vertexData->vertexBufferBinding;

        if( bind->getBindings().empty() || mVertexBufferCapacity < vtxCount )
        {
            // Keep spare capacity so small UI changes do not recreate GPU buffers every frame.
            mVertexBufferCapacity = static_cast<std::size_t>( vtxCount ) + 5000u;

            bind->setBinding( 0, Ogre::v1::HardwareBufferManager::getSingleton().createVertexBuffer(
                                     sizeof( ImDrawVert ), mVertexBufferCapacity,
                                     Ogre::v1::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE ) );
        }
        if( mRenderOp.indexData->indexBuffer.isNull() || mIndexBufferCapacity < idxCount )
        {
            mIndexBufferCapacity = static_cast<std::size_t>( idxCount ) + 10000u;
            const auto indexType = sizeof( ImDrawIdx ) == 2
                                       ? Ogre::v1::HardwareIndexBuffer::IT_16BIT
                                       : Ogre::v1::HardwareIndexBuffer::IT_32BIT;

            mRenderOp.indexData->indexBuffer =
                Ogre::v1::HardwareBufferManager::getSingleton().createIndexBuffer(
                    indexType, mIndexBufferCapacity,
                    Ogre::v1::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE );
        }

        const auto vertexBuffer = bind->getBuffer( 0 );
        const auto indexBuffer = mRenderOp.indexData->indexBuffer;

        auto *vtxDst = static_cast<ImDrawVert *>(
            vertexBuffer->lock( Ogre::v1::HardwareBuffer::HBL_DISCARD ) );
        auto *idxDst = static_cast<ImDrawIdx *>(
            indexBuffer->lock( Ogre::v1::HardwareBuffer::HBL_DISCARD ) );

        memcpy( vtxDst, vtxBuf, static_cast<std::size_t>( vtxCount ) * sizeof( ImDrawVert ) );
        memcpy( idxDst, idxBuf, static_cast<std::size_t>( idxCount ) * sizeof( ImDrawIdx ) );

        vertexBuffer->unlock();
        indexBuffer->unlock();
    }

    void ImguiRenderableOgreNext::setDrawRange( unsigned int vertexOffset, unsigned int indexOffset,
                                                unsigned int indexCount )
    {
        if( vertexOffset >= mVertexCount || indexOffset >= mIndexCount ||
            indexCount > mIndexCount - indexOffset )
        {
            mRenderOp.vertexData->vertexCount = 0;
            mRenderOp.indexData->indexCount = 0;
            return;
        }

        mRenderOp.vertexData->vertexStart = vertexOffset;
        mRenderOp.vertexData->vertexCount = mVertexCount - vertexOffset;
        mRenderOp.indexData->indexStart = indexOffset;
        mRenderOp.indexData->indexCount = indexCount;
    }

    void ImguiRenderableOgreNext::getWorldTransforms( Ogre::Matrix4 *xform ) const
    {
        *xform = Ogre::Matrix4::IDENTITY;
    }

    void ImguiRenderableOgreNext::getRenderOperation( Ogre::v1::RenderOperation &op, bool casterPass )
    {
        op = mRenderOp;
    }

    auto ImguiRenderableOgreNext::getLights() const -> const Ogre::LightList &
    {
        static const Ogre::LightList l;
        return l;
    }

}  // namespace workphone::render

#endif
