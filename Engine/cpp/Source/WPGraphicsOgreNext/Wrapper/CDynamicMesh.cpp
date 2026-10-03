#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CDynamicMesh.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSceneOgreNext.hpp>
#include <WPGraphicsOgreNext/DynamicMeshOgreNext.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Interface/Mesh/ISubMesh.hpp>
#include <Math/Array/OgreObjectMemoryManager.h>
#include <Ogre.h>
#include <algorithm>
#include <limits>
#include <utility>

namespace workphone::render
{
    namespace
    {
        auto getOwnerSceneNode( const SmartPtr<IGraphicsSceneNode> &owner ) -> Ogre::SceneNode *
        {
            Ogre::SceneNode *sceneNode = nullptr;
            if( owner )
            {
                owner->_getObject( reinterpret_cast<void **>( &sceneNode ) );
            }

            return sceneNode;
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone::render, CDynamicMesh, CGraphicsObjectOgreNext<DynamicMesh> );

    CDynamicMesh::CDynamicMesh() = default;

    CDynamicMesh::CDynamicMesh( SmartPtr<IGraphicsScene> creator ) : m_creator( std::move( creator ) )
    {
        setCreator( m_creator );
    }

    CDynamicMesh::~CDynamicMesh()
    {
        unload( nullptr );
    }

    void CDynamicMesh::load( SmartPtr<ISharedObject> data )
    {
        if( getLoadingState() == LoadingState::Loaded )
        {
            return;
        }

        setLoadingState( LoadingState::Loading );

        Ogre::SceneManager *sceneManager = nullptr;
        if( auto creator = getCreator() )
        {
            creator->_getObject( reinterpret_cast<void **>( &sceneManager ) );
        }

        if( !sceneManager )
        {
            setLoadingState( LoadingState::Unloaded );
            return;
        }

        auto *ownerNode = getOwnerSceneNode( getOwner() );
        auto id = Ogre::Id::generateNewId<DynamicMeshOgreNext>();
        auto *memoryManager = &sceneManager->_getEntityMemoryManager( Ogre::SCENE_DYNAMIC );

        m_dynamicMesh = new DynamicMeshOgreNext( id, memoryManager, sceneManager, ownerNode );
        setGraphicsObject( m_dynamicMesh );

        if( !m_materialName.empty() )
        {
            m_dynamicMesh->setDatablockOrMaterialName(
                m_materialName.c_str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
        }

        m_dynamicMesh->setCastShadows( getCastShadows() );
        m_dynamicMesh->setVisible( true );
        if( getRenderQueueGroup() != 0 )
        {
            m_dynamicMesh->setRenderQueueGroup( getRenderQueueGroup() );
        }
        if( getVisibilityFlags() != 0 )
        {
            m_dynamicMesh->setVisibilityFlags( getVisibilityFlags() );
        }

        if( auto subMesh = DynamicMesh::getSubMesh() )
        {
            m_dynamicMesh->setMesh( subMesh );
            m_dynamicMesh->update();
        }

        if( ownerNode && !m_dynamicMesh->isAttached() )
        {
            ownerNode->attachObject( m_dynamicMesh );
        }

        setLoadingState( LoadingState::Loaded );
        DynamicMesh::load( data );
    }

    void CDynamicMesh::unload( SmartPtr<ISharedObject> data )
    {
        if( m_dynamicMesh )
        {
            if( m_dynamicMesh->isAttached() )
            {
                m_dynamicMesh->detachFromParent();
            }

            delete m_dynamicMesh;
            m_dynamicMesh = nullptr;
            setGraphicsObject( nullptr );
        }

        DynamicMesh::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void CDynamicMesh::update()
    {
        if( m_dynamicMesh )
        {
            m_dynamicMesh->update();
        }
    }

    void CDynamicMesh::setMaterialName( const String &materialName, s32 index )
    {
        if( index > 0 )
        {
            return;
        }

        m_materialName = materialName;
        if( m_dynamicMesh )
        {
            m_dynamicMesh->setDatablockOrMaterialName(
                materialName.c_str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
        }
    }

    auto CDynamicMesh::getMaterialName( s32 index ) const -> String
    {
        if( index > 0 )
        {
            return StringUtil::EmptyString;
        }

        if( !m_materialName.empty() )
        {
            return m_materialName;
        }

        if( m_dynamicMesh )
        {
            return m_dynamicMesh->getDatablockOrMaterialName().c_str();
        }

        return StringUtil::EmptyString;
    }

    void CDynamicMesh::setCastShadows( bool castShadows )
    {
        DynamicMesh::setCastShadows( castShadows );
        if( m_dynamicMesh )
        {
            m_dynamicMesh->setCastShadows( castShadows );
        }
    }

    auto CDynamicMesh::getCastShadows() const -> bool
    {
        return DynamicMesh::getCastShadows();
    }

    void CDynamicMesh::setReceiveShadows( bool receiveShadows )
    {
        DynamicMesh::setReceiveShadows( receiveShadows );
        m_receiveShadows = receiveShadows;
    }

    auto CDynamicMesh::getReceiveShadows() const -> bool
    {
        return DynamicMesh::getReceiveShadows();
    }

    void CDynamicMesh::setRecieveShadows( bool recieveShadows )
    {
        setReceiveShadows( recieveShadows );
    }

    auto CDynamicMesh::getRecieveShadows() const -> bool
    {
        return getReceiveShadows();
    }

    void CDynamicMesh::setVisible( bool visible )
    {
        DynamicMesh::setVisible( visible );
        if( m_dynamicMesh )
        {
            m_dynamicMesh->setVisible( visible );
        }
    }

    auto CDynamicMesh::isVisible() const -> bool
    {
        return m_dynamicMesh ? m_dynamicMesh->isVisible() : DynamicMesh::isVisible();
    }

    void CDynamicMesh::setRenderQueueGroup( u32 renderQueue )
    {
        DynamicMesh::setRenderQueueGroup( renderQueue );
        if( m_dynamicMesh )
        {
            auto clampedQueue =
                std::min<u32>( renderQueue, std::numeric_limits<Ogre::uint8>::max() );
            m_dynamicMesh->setRenderQueueGroup( static_cast<Ogre::uint8>( clampedQueue ) );
        }
    }

    void CDynamicMesh::setVisibilityFlags( u32 flags )
    {
        DynamicMesh::setVisibilityFlags( flags );
        if( m_dynamicMesh )
        {
            m_dynamicMesh->setVisibilityFlags( flags );
        }
    }

    auto CDynamicMesh::getVisibilityFlags() const -> u32
    {
        return m_dynamicMesh ? m_dynamicMesh->getVisibilityFlags() : DynamicMesh::getVisibilityFlags();
    }

    auto CDynamicMesh::clone( const String &name ) const -> SmartPtr<IGraphicsObject>
    {
        auto dynamicMesh = workphone::make_ptr<CDynamicMesh>( m_creator );
        dynamicMesh->setName( name );
        dynamicMesh->setMesh( m_mesh );
        dynamicMesh->setSubMesh( DynamicMesh::getSubMesh() );
        dynamicMesh->setMaterialName( m_materialName );
        dynamicMesh->setCastShadows( getCastShadows() );
        dynamicMesh->setReceiveShadows( getReceiveShadows() );
        dynamicMesh->setVisible( isVisible() );
        dynamicMesh->setVisibilityFlags( getVisibilityFlags() );
        dynamicMesh->setRenderQueueGroup( getRenderQueueGroup() );
        dynamicMesh->setDirty( true );
        return dynamicMesh;
    }

    void CDynamicMesh::_getObject( void **ppObject ) const
    {
        if( ppObject )
        {
            *ppObject = m_dynamicMesh;
        }
    }

    void CDynamicMesh::setSubMesh( SmartPtr<ISubMesh> subMesh )
    {
        DynamicMesh::setSubMesh( subMesh );
        if( m_dynamicMesh )
        {
            m_dynamicMesh->setMesh( subMesh );
        }
    }

    void CDynamicMesh::setMesh( SmartPtr<IMesh> mesh )
    {
        m_mesh = mesh;
        DynamicMesh::setMesh( mesh );

        if( mesh && mesh->getNumSubMeshes() > 0 )
        {
            setSubMesh( mesh->getSubMesh( 0 ) );
        }
        else
        {
            setSubMesh( nullptr );
        }
    }

    auto CDynamicMesh::getMesh() const -> SmartPtr<IMesh>
    {
        return m_mesh ? m_mesh : DynamicMesh::getMesh();
    }

    auto CDynamicMesh::getSubMesh() const -> SmartPtr<ISubMesh>
    {
        return DynamicMesh::getSubMesh();
    }

    void CDynamicMesh::setDirty( bool dirty )
    {
        DynamicMesh::setDirty( dirty );
        if( dirty && m_dynamicMesh )
        {
            m_dynamicMesh->setDirty();
        }
    }

    void CDynamicMesh::setOwner( SmartPtr<IGraphicsSceneNode> owner )
    {
        CGraphicsObjectOgreNext<DynamicMesh>::setOwner( owner );

        auto *sceneNode = getOwnerSceneNode( owner );
        if( m_dynamicMesh )
        {
            if( m_dynamicMesh->isAttached() )
            {
                m_dynamicMesh->detachFromParent();
            }

            m_dynamicMesh->setOwner( sceneNode );
            if( sceneNode )
            {
                sceneNode->attachObject( m_dynamicMesh );
            }
        }
    }
}  // namespace workphone::render
