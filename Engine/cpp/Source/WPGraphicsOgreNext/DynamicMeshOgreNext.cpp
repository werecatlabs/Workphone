#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/DynamicMeshOgreNext.hpp>
#include <Workphone/WorkphoneInterface.hpp>
#include <Math/Array/OgreObjectMemoryManager.h>
#include <Ogre.h>
#include <OgreHardwareBufferManager.h>
#include <OgreHardwareIndexBuffer.h>
#include <OgreHardwareVertexBuffer.h>
#include <OgreVertexIndexData.h>
#include <algorithm>
#include <cstdint>
#include <limits>

namespace workphone::render
{
    namespace
    {
        auto getSceneManager( Ogre::SceneNode *owner ) -> Ogre::SceneManager *
        {
            return owner ? owner->getCreator() : nullptr;
        }

        auto getObjectMemoryManager( Ogre::SceneNode *owner ) -> Ogre::ObjectMemoryManager *
        {
            if( auto sceneManager = getSceneManager( owner ) )
            {
                return &sceneManager->_getEntityMemoryManager( Ogre::SCENE_DYNAMIC );
            }

            return nullptr;
        }

        auto toOgreOperationType( RenderOperationType operationType ) -> Ogre::OperationType
        {
            switch( operationType )
            {
            case RenderOperationType::OT_POINT_LIST:
                return Ogre::OT_POINT_LIST;
            case RenderOperationType::OT_LINE_LIST:
                return Ogre::OT_LINE_LIST;
            case RenderOperationType::OT_LINE_STRIP:
                return Ogre::OT_LINE_STRIP;
            case RenderOperationType::OT_TRIANGLE_STRIP:
                return Ogre::OT_TRIANGLE_STRIP;
            case RenderOperationType::OT_TRIANGLE_FAN:
                return Ogre::OT_TRIANGLE_FAN;
            case RenderOperationType::OT_TRIANGLE_LIST:
            default:
                return Ogre::OT_TRIANGLE_LIST;
            }
        }

        auto asBytes( const void *data ) -> const std::uint8_t *
        {
            return static_cast<const std::uint8_t *>( data );
        }

        auto readVector3( const SmartPtr<IVertexElement> &element, const std::uint8_t *vertex,
                          const Ogre::Vector3 &fallback ) -> Ogre::Vector3
        {
            if( !element )
            {
                return fallback;
            }

            f32 *elementData = nullptr;
            element->getElementData( const_cast<std::uint8_t *>( vertex ), &elementData );
            if( !elementData )
            {
                return fallback;
            }

            return Ogre::Vector3( elementData[0], elementData[1], elementData[2] );
        }

        auto readVector2( const SmartPtr<IVertexElement> &element,
                          const std::uint8_t *vertex ) -> Ogre::Vector2
        {
            if( !element )
            {
                return Ogre::Vector2::ZERO;
            }

            f32 *elementData = nullptr;
            element->getElementData( const_cast<std::uint8_t *>( vertex ), &elementData );
            if( !elementData )
            {
                return Ogre::Vector2::ZERO;
            }

            return Ogre::Vector2( elementData[0], elementData[1] );
        }

        auto readIndex( const SmartPtr<IIndexBuffer> &indexBuffer, size_t index ) -> u32
        {
            const auto *indexData = indexBuffer ? indexBuffer->getIndexData() : nullptr;
            if( !indexData )
            {
                return static_cast<u32>( index );
            }

            if( indexBuffer->getIndexType() == IIndexBuffer::Type::IT_16BIT ||
                indexBuffer->getIndexSize() == sizeof( u16 ) )
            {
                return static_cast<const u16 *>( indexData )[index];
            }

            return static_cast<const u32 *>( indexData )[index];
        }

    }  // namespace

    DynamicMeshOgreNext::DynamicMeshOgreNext( Ogre::IdType id,
                                              Ogre::ObjectMemoryManager *objectMemoryManager,
                                              Ogre::SceneManager *manager, Ogre::SceneNode *owner ) :
        DynamicRenderable( id, objectMemoryManager, manager ),
        m_owner( owner )
    {
        initialise( Ogre::OT_TRIANGLE_LIST, true );
    }

    DynamicMeshOgreNext::DynamicMeshOgreNext( Ogre::SceneNode *owner ) :
        DynamicMeshOgreNext( Ogre::Id::generateNewId<DynamicMeshOgreNext>(),
                             getObjectMemoryManager( owner ), getSceneManager( owner ), owner )
    {
    }

    DynamicMeshOgreNext::DynamicMeshOgreNext( const String &name, Ogre::SceneNode *owner ) :
        DynamicMeshOgreNext( Ogre::Id::generateNewId<DynamicMeshOgreNext>(),
                             getObjectMemoryManager( owner ), getSceneManager( owner ), owner )
    {
        (void)name;
    }

    DynamicMeshOgreNext::~DynamicMeshOgreNext() = default;

    void DynamicMeshOgreNext::setDirty()
    {
        m_dirty = true;
    }

    void DynamicMeshOgreNext::update()
    {
        if( m_dirty )
        {
            fillHardwareBuffers();
        }
    }

    void DynamicMeshOgreNext::setOwner( Ogre::SceneNode *owner )
    {
        m_owner = owner;
    }

    auto DynamicMeshOgreNext::getOwner() const -> Ogre::SceneNode *
    {
        return m_owner;
    }

    void DynamicMeshOgreNext::setMesh( SmartPtr<ISubMesh> subMesh )
    {
        m_engineMesh = subMesh;
        if( m_engineMesh )
        {
            mRenderOp.operationType = toOgreOperationType( m_engineMesh->getRenderOperationType() );
        }

        setDirty();
    }

    auto DynamicMeshOgreNext::getSubMesh() const -> SmartPtr<ISubMesh>
    {
        return m_engineMesh;
    }

    void DynamicMeshOgreNext::createVertexDeclaration()
    {
        auto *decl = mRenderOp.vertexData->vertexDeclaration;

        u32 offset = 0;
        decl->addElement( 0, offset, Ogre::VET_FLOAT3, Ogre::VES_POSITION );
        offset += Ogre::v1::VertexElement::getTypeSize( Ogre::VET_FLOAT3 );

        decl->addElement( 0, offset, Ogre::VET_FLOAT3, Ogre::VES_NORMAL );
        offset += Ogre::v1::VertexElement::getTypeSize( Ogre::VET_FLOAT3 );

        decl->addElement( 0, offset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES, 0 );
        offset += Ogre::v1::VertexElement::getTypeSize( Ogre::VET_FLOAT2 );

        decl->addElement( 0, offset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES, 1 );
    }

    void DynamicMeshOgreNext::fillHardwareBuffers()
    {
        if( !m_engineMesh )
        {
            prepareHardwareBuffers( 0, 0 );
            mBox.setExtents( Ogre::Vector3::ZERO, Ogre::Vector3::ZERO );
            m_dirty = false;
            return;
        }

        auto vertexBuffer = m_engineMesh->getVertexBuffer();
        auto indexBuffer = m_engineMesh->getIndexBuffer();

        if( !vertexBuffer )
        {
            prepareHardwareBuffers( 0, 0 );
            mBox.setExtents( Ogre::Vector3::ZERO, Ogre::Vector3::ZERO );
            m_dirty = false;
            return;
        }

        const auto vertexCount = static_cast<size_t>( vertexBuffer->getNumVertices() );
        const auto indexCount =
            indexBuffer ? static_cast<size_t>( indexBuffer->getNumIndices() ) : vertexCount;

        prepareHardwareBuffers( vertexCount, indexCount );

        if( vertexCount == 0 || !vertexBuffer->getVertexData() )
        {
            mBox.setExtents( Ogre::Vector3::ZERO, Ogre::Vector3::ZERO );
            m_dirty = false;
            return;
        }

        auto decl = vertexBuffer->getVertexDeclaration();
        if( !decl || decl->getSize( 0 ) == 0 )
        {
            prepareHardwareBuffers( 0, 0 );
            mBox.setExtents( Ogre::Vector3::ZERO, Ogre::Vector3::ZERO );
            m_dirty = false;
            return;
        }

        const auto posElem =
            decl->findElementBySemantic( VertexElementSemantic::VES_POSITION );
        const auto normalElem =
            decl->findElementBySemantic( VertexElementSemantic::VES_NORMAL );
        const auto uvElem0 =
            decl->findElementBySemantic( VertexElementSemantic::VES_TEXTURE_COORDINATES, 0 );
        const auto uvElem1 =
            decl->findElementBySemantic( VertexElementSemantic::VES_TEXTURE_COORDINATES, 1 );

        const auto fbVertexSize = decl->getSize( 0 );
        const auto *fbVertexBase = asBytes( vertexBuffer->getVertexData() );

        auto vbuf = mRenderOp.vertexData->vertexBufferBinding->getBuffer( 0 );
        auto *ogreVtx = static_cast<f32 *>( vbuf->lock( Ogre::v1::HardwareBuffer::HBL_DISCARD ) );

        Ogre::Vector3 vaabMin( 1e10f, 1e10f, 1e10f );
        Ogre::Vector3 vaabMax( -1e10f, -1e10f, -1e10f );

        for( size_t j = 0; j < vertexCount; ++j )
        {
            const auto *fbVertex = fbVertexBase + j * fbVertexSize;

            auto pos = readVector3( posElem, fbVertex, Ogre::Vector3::ZERO );
            pos.z = -pos.z;

            vaabMin.x = std::min( vaabMin.x, pos.x );
            vaabMin.y = std::min( vaabMin.y, pos.y );
            vaabMin.z = std::min( vaabMin.z, pos.z );
            vaabMax.x = std::max( vaabMax.x, pos.x );
            vaabMax.y = std::max( vaabMax.y, pos.y );
            vaabMax.z = std::max( vaabMax.z, pos.z );

            *ogreVtx++ = pos.x;
            *ogreVtx++ = pos.y;
            *ogreVtx++ = pos.z;

            auto normal = readVector3( normalElem, fbVertex, Ogre::Vector3::UNIT_Y );
            normal.z = -normal.z;
            *ogreVtx++ = normal.x;
            *ogreVtx++ = normal.y;
            *ogreVtx++ = normal.z;

            auto uv0 = readVector2( uvElem0, fbVertex );
            *ogreVtx++ = uv0.x;
            *ogreVtx++ = uv0.y;

            auto uv1 = readVector2( uvElem1, fbVertex );
            *ogreVtx++ = uv1.x;
            *ogreVtx++ = uv1.y;
        }

        vbuf->unlock();

        auto ibuf = mRenderOp.indexData->indexBuffer;
        const bool use16bit = ibuf->getType() == Ogre::v1::HardwareIndexBuffer::IT_16BIT;
        const bool reverseWinding = mRenderOp.operationType == Ogre::OT_TRIANGLE_LIST ||
                                    mRenderOp.operationType == Ogre::OT_TRIANGLE_STRIP ||
                                    mRenderOp.operationType == Ogre::OT_TRIANGLE_FAN;

        if( use16bit )
        {
            auto *dst = static_cast<u16 *>( ibuf->lock( Ogre::v1::HardwareBuffer::HBL_DISCARD ) );
            for( size_t i = 0; i < indexCount; ++i )
            {
                const auto srcIndex = reverseWinding ? indexCount - i - 1u : i;
                const auto index = readIndex( indexBuffer, srcIndex );
                *dst++ = index <= std::numeric_limits<u16>::max() ? static_cast<u16>( index ) : 0u;
            }
        }
        else
        {
            auto *dst = static_cast<u32 *>( ibuf->lock( Ogre::v1::HardwareBuffer::HBL_DISCARD ) );
            for( size_t i = 0; i < indexCount; ++i )
            {
                const auto srcIndex = reverseWinding ? indexCount - i - 1u : i;
                *dst++ = readIndex( indexBuffer, srcIndex );
            }
        }

        ibuf->unlock();

        mBox.setExtents( vaabMin, vaabMax );
        m_dirty = false;
    }
}  // namespace workphone::render
