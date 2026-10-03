#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CInstancedObjectOgreNext.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreItem.h>
#include <OgreResourceGroupManager.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreSubItem.h>

namespace workphone::render
{
    namespace
    {
        const String instancedObjectMaterialNamePropertyStr = "instancedObjectMaterialName";
        const String instancedObjectManagerNamePropertyStr = "instancedObjectManagerName";
        const String instancedObjectPositionPropertyStr = "instancedObjectPosition";
        const String instancedObjectOrientationPropertyStr = "instancedObjectOrientation";
        const String instancedObjectScalePropertyStr = "instancedObjectScale";
    }  // namespace

    CInstancedObjectOgreNext::CInstancedObjectOgreNext( SmartPtr<IGraphicsScene> creator,
                                                        SmartPtr<CInstanceManagerOgreNext> manager,
                                                        const String &materialName,
                                                        const String &managerName ) :
        m_manager( manager ),
        m_materialName( materialName ),
        m_managerName( managerName )
    {
        setCreator( creator );
        setupStateObject();
        setScale( Vector3F::UNIT );
        setVisible( true );
    }

    CInstancedObjectOgreNext::~CInstancedObjectOgreNext()
    {
        if( getLoadingState() != LoadingState::Unloaded )
        {
            unload( nullptr );
        }
    }

    void CInstancedObjectOgreNext::load( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() )
        {
            return;
        }

        setLoadingState( LoadingState::Loading );

        Ogre::SceneManager *smgr = nullptr;
        if( auto creator = getCreator() )
        {
            creator->_getObject( reinterpret_cast<void **>( &smgr ) );
        }

        auto manager = m_manager.load();
        if( !smgr || !manager || StringUtil::isNullOrEmpty( manager->getMeshName() ) )
        {
            setLoadingState( LoadingState::Error );
            return;
        }

        const auto &groupName = StringUtil::isNullOrEmpty( manager->getGroupName() )
                                    ? Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME
                                    : manager->getGroupName();
        auto *item = smgr->createItem( manager->getMeshName().c_str(), groupName.c_str(),
                                       Ogre::SCENE_DYNAMIC,
                                       StringUtil::isNullOrEmpty( m_materialName ) );
        if( !item )
        {
            setLoadingState( LoadingState::Error );
            return;
        }

        m_item = item;
        setGraphicsObject( item );

        applyMaterial();
        applyRenderState();
        applyCustomParams();

        setLoadingState( LoadingState::Loaded );
        Base::load( data );
    }

    void CInstancedObjectOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        auto *item = m_item.load();
        if( item )
        {
            item->detachFromParent();
        }

        if( m_instanceNode )
        {
            m_instanceNode->detachAllObjects();

            if( auto parent = m_instanceNode->getParentSceneNode() )
            {
                parent->removeAndDestroyChild( m_instanceNode );
            }
            else if( auto creator = getCreator() )
            {
                Ogre::SceneManager *smgr = nullptr;
                creator->_getObject( reinterpret_cast<void **>( &smgr ) );
                if( smgr )
                {
                    smgr->destroySceneNode( m_instanceNode );
                }
            }

            m_instanceNode = nullptr;
        }

        if( item )
        {
            if( auto creator = getCreator() )
            {
                Ogre::SceneManager *smgr = nullptr;
                creator->_getObject( reinterpret_cast<void **>( &smgr ) );
                if( smgr )
                {
                    smgr->destroyItem( item );
                }
            }
        }

        m_item = nullptr;
        setGraphicsObject( nullptr );
        Base::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void CInstancedObjectOgreNext::setPosition( const Vector3F &position )
    {
        m_position = position;
        updateInstanceNodeTransform();
    }

    Vector3F CInstancedObjectOgreNext::getPosition() const
    {
        return m_position;
    }

    void CInstancedObjectOgreNext::setOrientation( const QuaternionF &orientation )
    {
        m_orientation = orientation;
        updateInstanceNodeTransform();
    }

    QuaternionF CInstancedObjectOgreNext::getOrientation() const
    {
        return m_orientation;
    }

    void CInstancedObjectOgreNext::setScale( const Vector3F &scale )
    {
        m_scale = scale;
        updateInstanceNodeTransform();
    }

    Vector3F CInstancedObjectOgreNext::getScale() const
    {
        return m_scale;
    }

    void CInstancedObjectOgreNext::setCustomParam( u8 idx, const Vector4F &newParam )
    {
        if( idx < MaxCustomParams )
        {
            m_customParams[idx] = newParam;
            m_customParamSet[idx] = true;
        }

        if( auto *item = m_item.load() )
        {
            for( size_t i = 0; i < item->getNumSubItems(); ++i )
            {
                if( auto *subItem = item->getSubItem( i ) )
                {
                    subItem->setCustomParameter(
                        idx, Ogre::Vector4( newParam.X(), newParam.Y(), newParam.Z(), newParam.W() ) );
                }
            }
        }
    }

    Vector4F CInstancedObjectOgreNext::getCustomParam( u8 idx )
    {
        if( idx < MaxCustomParams )
        {
            return m_customParams[idx];
        }

        return Vector4F::ZERO;
    }

    void CInstancedObjectOgreNext::attachToParent( SmartPtr<IGraphicsSceneNode> parent )
    {
        if( !parent )
        {
            return;
        }

        if( !isLoaded() )
        {
            load( nullptr );
        }

        auto *item = m_item.load();
        if( !item )
        {
            return;
        }

        Ogre::SceneNode *parentNode = nullptr;
        parent->_getObject( reinterpret_cast<void **>( &parentNode ) );
        if( !parentNode )
        {
            return;
        }

        if( item->isAttached() )
        {
            item->detachFromParent();
        }

        if( m_instanceNode && m_instanceNode->getParentSceneNode() != parentNode )
        {
            m_instanceNode->detachAllObjects();
            if( auto oldParent = m_instanceNode->getParentSceneNode() )
            {
                oldParent->removeAndDestroyChild( m_instanceNode );
            }
            m_instanceNode = nullptr;
        }

        if( !m_instanceNode )
        {
            m_instanceNode = parentNode->createChildSceneNode( Ogre::SCENE_DYNAMIC );
        }

        updateInstanceNodeTransform();
        m_instanceNode->attachObject( item );
    }

    void CInstancedObjectOgreNext::detachFromParent( SmartPtr<IGraphicsSceneNode> parent )
    {
        if( auto *item = m_item.load() )
        {
            if( item->isAttached() )
            {
                item->detachFromParent();
            }
        }

        if( m_instanceNode )
        {
            m_instanceNode->detachAllObjects();
            if( auto parentNode = m_instanceNode->getParentSceneNode() )
            {
                parentNode->removeAndDestroyChild( m_instanceNode );
            }
            m_instanceNode = nullptr;
        }

        Base::detachFromParent( parent );
    }

    void CInstancedObjectOgreNext::setVisible( bool visible )
    {
        Base::setVisible( visible );
        if( auto *item = m_item.load() )
        {
            item->setVisible( visible );
        }
    }

    void CInstancedObjectOgreNext::setCastShadows( bool castShadows )
    {
        Base::setCastShadows( castShadows );
        if( auto *item = m_item.load() )
        {
            item->setCastShadows( castShadows );
        }
    }

    void CInstancedObjectOgreNext::setVisibilityFlags( u32 flags )
    {
        Base::setVisibilityFlags( flags );
        if( auto *item = m_item.load() )
        {
            item->setVisibilityFlags( flags );
        }
    }

    void CInstancedObjectOgreNext::setRenderQueueGroup( u32 queueID )
    {
        Base::setRenderQueueGroup( queueID );
        if( auto *item = m_item.load() )
        {
            item->setRenderQueueGroup( queueID );
        }
    }

    SmartPtr<IGraphicsObject> CInstancedObjectOgreNext::clone( const String &name ) const
    {
        auto clone = workphone::make_ptr<CInstancedObjectOgreNext>(
            getCreator(), m_manager.load(), m_materialName, m_managerName );
        clone->setPosition( m_position );
        clone->setOrientation( m_orientation );
        clone->setScale( m_scale );
        clone->setVisible( isVisible() );
        clone->setCastShadows( getCastShadows() );
        clone->setVisibilityFlags( getVisibilityFlags() );
        clone->setRenderQueueGroup( getRenderQueueGroup() );

        if( !StringUtil::isNullOrEmpty( name ) )
        {
            clone->setName( name );
        }

        for( u8 i = 0; i < MaxCustomParams; ++i )
        {
            if( m_customParamSet[i] )
            {
                clone->setCustomParam( i, m_customParams[i] );
            }
        }

        return clone;
    }

    void CInstancedObjectOgreNext::_getObject( void **ppObject ) const
    {
        if( ppObject )
        {
            *ppObject = m_item.load();
        }
    }

    SmartPtr<Properties> CInstancedObjectOgreNext::getProperties() const
    {
        auto properties = Base::getProperties();
        properties->setProperty( instancedObjectMaterialNamePropertyStr, m_materialName );
        properties->setProperty( instancedObjectManagerNamePropertyStr, m_managerName );
        properties->setProperty( instancedObjectPositionPropertyStr, m_position );
        properties->setProperty( instancedObjectOrientationPropertyStr, m_orientation );
        properties->setProperty( instancedObjectScalePropertyStr, m_scale );
        return properties;
    }

    void CInstancedObjectOgreNext::setProperties( SmartPtr<Properties> properties )
    {
        Base::setProperties( properties );

        if( !properties )
        {
            return;
        }

        properties->getPropertyValue( instancedObjectMaterialNamePropertyStr, m_materialName );
        properties->getPropertyValue( instancedObjectManagerNamePropertyStr, m_managerName );
        properties->getPropertyValue( instancedObjectPositionPropertyStr, m_position );
        properties->getPropertyValue( instancedObjectOrientationPropertyStr, m_orientation );
        properties->getPropertyValue( instancedObjectScalePropertyStr, m_scale );

        applyMaterial();
        applyRenderState();
        updateInstanceNodeTransform();
    }

    void CInstancedObjectOgreNext::setupStateObject()
    {
        GraphicsObject<IInstancedObject>::setupStateObject();

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        auto factoryManager = graphicsSystem->getFactoryManagerPtr();

        if( auto stateContext = getStateContext() )
        {
            auto state = factoryManager->make_ptr<State>();
            state->setId( getId() );
            state->setOwner( this );
            stateContext->addState( state );

            auto stateData = factoryManager->make_ptr<GraphicsObjectData>();
            state->setData( stateData );
        }
    }

    void CInstancedObjectOgreNext::applyMaterial()
    {
        if( auto *item = m_item.load() )
        {
            if( !StringUtil::isNullOrEmpty( m_materialName ) )
            {
                item->setDatablockOrMaterialName( m_materialName.c_str() );
            }
        }
    }

    void CInstancedObjectOgreNext::applyRenderState()
    {
        if( auto *item = m_item.load() )
        {
            item->setVisible( isVisible() );
            item->setCastShadows( getCastShadows() );
            item->setVisibilityFlags( getVisibilityFlags() );
            item->setRenderQueueGroup( getRenderQueueGroup() );
        }
    }

    void CInstancedObjectOgreNext::applyCustomParams()
    {
        for( u8 i = 0; i < MaxCustomParams; ++i )
        {
            if( m_customParamSet[i] )
            {
                setCustomParam( i, m_customParams[i] );
            }
        }
    }

    void CInstancedObjectOgreNext::updateInstanceNodeTransform()
    {
        if( m_instanceNode )
        {
            m_instanceNode->setPosition(
                Ogre::Vector3( m_position.X(), m_position.Y(), m_position.Z() ) );
            m_instanceNode->setOrientation( Ogre::Quaternion(
                m_orientation.W(), m_orientation.X(), m_orientation.Y(), m_orientation.Z() ) );
            m_instanceNode->setScale( Ogre::Vector3( m_scale.X(), m_scale.Y(), m_scale.Z() ) );
        }
    }
}  // namespace workphone::render
