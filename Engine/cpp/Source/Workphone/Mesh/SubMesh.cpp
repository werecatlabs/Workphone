#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/SubMesh.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Interface/Mesh/IVertexBoneAssignment.hpp>
#include <Workphone/Interface/Mesh/IVertexBuffer.hpp>
#include <Workphone/Interface/Mesh/IIndexBuffer.hpp>
#include <Workphone/Interface/Mesh/IVertexDeclaration.hpp>
#include <Workphone/Interface/Mesh/IVertexElement.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IResourceManager.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, SubMesh, ISubMesh );

    SubMesh::SubMesh() = default;

    SubMesh::~SubMesh() = default;

    void SubMesh::load( SmartPtr<ISharedObject> data )
    {
        if( m_vertexBuffer )
        {
            m_vertexBuffer->load( nullptr );
        }

        if( m_indexBuffer )
        {
            m_indexBuffer->load( nullptr );
        }
    }

    void SubMesh::unload( SmartPtr<ISharedObject> data )
    {
        if( m_vertexBuffer )
        {
            m_vertexBuffer->unload( nullptr );
            m_vertexBuffer = nullptr;
        }

        if( m_indexBuffer )
        {
            m_indexBuffer->unload( nullptr );
            m_indexBuffer = nullptr;
        }
    }

    void SubMesh::setMaterialName( const String &materialName )
    {
        m_materialName = materialName;
    }

    auto SubMesh::getMaterialName() const -> String
    {
        return m_materialName;
    }

    void SubMesh::setVertexBuffer( SmartPtr<IVertexBuffer> vertexBuffer )
    {
        m_vertexBuffer = vertexBuffer;
    }

    auto SubMesh::getVertexBuffer() const -> SmartPtr<IVertexBuffer>
    {
        return m_vertexBuffer;
    }

    void SubMesh::setIndexBuffer( SmartPtr<IIndexBuffer> indexBuffer )
    {
        m_indexBuffer = indexBuffer;
    }

    auto SubMesh::getIndexBuffer() const -> SmartPtr<IIndexBuffer>
    {
        return m_indexBuffer;
    }

    void SubMesh::updateAABB()
    {
        auto vertexBuffer = getVertexBuffer();
        auto vertexData = static_cast<u8 *>( vertexBuffer->getVertexData() );
        auto vertexDec = vertexBuffer->getVertexDeclaration();
        auto posElem = vertexDec->findElementBySemantic( VertexElementSemantic::VES_POSITION );

        auto numVerticies = vertexBuffer->getNumVertices();

        if( numVerticies > 0 )
        {
            m_aabb.setMinimum( Vector3<real_Num>( 1e10, 1e10, 1e10 ) );
            m_aabb.setMaximum( Vector3<real_Num>( -1e10, -1e10, -1e10 ) );
        }
        else
        {
            m_aabb.reset( Vector3<real_Num>::zero() );
        }

        for( u32 j = 0; j < numVerticies; ++j, vertexData += vertexDec->getSize() )
        {
            f32 *pReal = nullptr;
            posElem->getElementData( vertexData, &pReal );
            Vector3<real_Num> position( pReal[0], pReal[1], pReal[2] );

            m_aabb.merge( position );
        }
    }

    auto SubMesh::getAABB() const -> AABB3<real_Num>
    {
        return m_aabb;
    }

    auto SubMesh::clone() const -> SmartPtr<ISubMesh>
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto subMesh = factoryManager->make_ptr<SubMesh>();

        subMesh->setMaterialName( m_materialName );
        subMesh->setVertexBuffer( m_vertexBuffer->clone() );
        subMesh->setIndexBuffer( m_indexBuffer->clone() );
        subMesh->setAABB( m_aabb );

        return subMesh;
    }

    auto SubMesh::getUseSharedVertices() const -> bool
    {
        return m_useSharedVertices;
    }

    void SubMesh::setUseSharedVertices( bool useSharedVertices )
    {
        m_useSharedVertices = useSharedVertices;
    }

    auto SubMesh::compare( SmartPtr<ISubMesh> other ) const -> bool
    {
        return m_vertexBuffer->compare( other->getVertexBuffer() ) &&
               m_indexBuffer->compare( other->getIndexBuffer() );
    }

    void SubMesh::addBoneAssignment( SmartPtr<IVertexBoneAssignment> vba )
    {
        m_boneAssignments.push_back( vba );
    }

    void SubMesh::removeBoneAssignment( SmartPtr<IVertexBoneAssignment> vba )
    {
        m_boneAssignments.erase( std::remove( m_boneAssignments.begin(), m_boneAssignments.end(), vba ),
                                 m_boneAssignments.end() );
    }

    Array<SmartPtr<IVertexBoneAssignment>> SubMesh::getBoneAssignments() const
    {
        return m_boneAssignments;
    }

    RenderOperationType SubMesh::getRenderOperationType() const
    {
        return m_renderOperationType;
    }

    void SubMesh::setAABB( const AABB3<real_Num> &aabb )
    {
        m_aabb = aabb;
    }

    void SubMesh::setRenderOperationType( RenderOperationType renderOperationType )
    {
        m_renderOperationType = renderOperationType;
    }

    bool SubMesh::isValid() const
    {
        return m_vertexBuffer && m_indexBuffer;
    }

    void SubMesh::lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto meshManager = applicationManager->getMeshManager();
        meshManager->lock();
    }

    bool SubMesh::try_lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto meshManager = applicationManager->getMeshManager();
        return meshManager->try_lock();
    }

    void SubMesh::unlock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto meshManager = applicationManager->getMeshManager();
        meshManager->unlock();
    }

}  // namespace workphone
