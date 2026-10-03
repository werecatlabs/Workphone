#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CSceneNodeOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CCameraOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsSceneOgre.hpp>
#include <WPGraphicsOgre/Addons/OgreUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>

namespace workphone
{
    namespace render
    {
        u32 CSceneNodeOgre::m_nameExt = 0;

        WP_CLASS_REGISTER_DERIVED( workphone::render, CSceneNodeOgre, GraphicsSceneNode );

        CSceneNodeOgre::CSceneNodeOgre() :
            m_sceneNode( nullptr ),

            m_lastUpdate( 0 ),
            m_transformUpdate( 0 ),

            m_isCulled( true )
        {
            m_nodeListener = nullptr;

            static const auto SceneNodeStr = String( "SceneNode" );
            auto name = SceneNodeStr + StringUtil::toString( m_nameExt++ );
            setName( name );

            auto applicationManager = core::IApplicationManager::instance();
            auto stateManager = applicationManager->getStateManager();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto factoryManager = applicationManager->getFactoryManager();

            auto stateContext = stateManager->addStateContext();
            stateContext->setOwner( this );

            auto sceneNodeStateListener = factoryManager->make_ptr<SceneNodeStateListener>();
            sceneNodeStateListener->setOwner( this );
            m_stateListener = sceneNodeStateListener;
            stateContext->addStateListener( sceneNodeStateListener );

            auto transformState = factoryManager->make_ptr<State>();
            stateContext->addState( transformState );

            auto transformStateData = factoryManager->make_ptr<TransformStateData>();
            transformState->setData( transformStateData );

            auto state = factoryManager->make_ptr<State>();
            //state->setSceneNode( this );
            stateContext->addState( state );

            auto stateData = factoryManager->make_ptr<SceneNodeStateData>();
            state->setData( stateData );

            setStateContext( stateContext );

            auto stateTask = graphicsSystem->getStateTask();
            stateContext->setTaskId( stateTask );
        }

        CSceneNodeOgre::CSceneNodeOgre( SmartPtr<IGraphicsScene> creator ) :
            m_sceneNode( nullptr ),

            m_lastUpdate( 0 ),
            m_transformUpdate( 0 ),

            m_isCulled( true )
        {
            m_nodeListener = nullptr;

            m_creator = creator;

            static const auto SceneNodeStr = String( "SceneNode" );
            auto name = SceneNodeStr + StringUtil::toString( m_nameExt++ );
            setName( name );

            auto applicationManager = core::IApplicationManager::instance();
            auto stateManager = applicationManager->getStateManager();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto factoryManager = applicationManager->getFactoryManager();

            auto stateContext = stateManager->addStateContext();
            stateContext->setOwner( this );

            auto sceneNodeStateListener = factoryManager->make_ptr<SceneNodeStateListener>();
            sceneNodeStateListener->setOwner( this );
            m_stateListener = sceneNodeStateListener;
            stateContext->addStateListener( sceneNodeStateListener );

            auto transformState = factoryManager->make_ptr<State>();
            stateContext->addState( transformState );

            auto transformStateData = factoryManager->make_ptr<TransformStateData>();
            transformState->setData( transformStateData );

            auto state = factoryManager->make_ptr<State>();
            //state->setSceneNode( this );
            stateContext->addState( state );

            auto stateData = factoryManager->make_ptr<SceneNodeStateData>();
            state->setData( stateData );

            setStateContext( stateContext );

            auto stateTask = graphicsSystem->getStateTask();
            stateContext->setTaskId( stateTask );
        }

        CSceneNodeOgre::~CSceneNodeOgre()
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                unload( nullptr );
            }

            destroyStateContext();
        }

        void CSceneNodeOgre::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                WP_ASSERT( Thread::getTaskFlag( Thread::Render_Flag ) );

                setLoadingState( LoadingState::Loading );

                auto creator = getCreator();
                if( creator )
                {
                    Ogre::SceneManager *smgr = nullptr;
                    creator->_getObject( (void **)&smgr );

                    auto handle = getHandle();
                    WP_ASSERT( handle );

                    auto name = getName();
                    if( StringUtil::isNullOrEmpty( name ) )
                    {
                        name = "SceneNode" + StringUtil::toString( m_nameExt++ );
                        setName( name );
                    }

                    m_sceneNode = smgr->createSceneNode( name.c_str() );

                    // auto parent = getParent();
                    // if (parent)
                    //{
                    //	Ogre::SceneNode* parentSceneNode = nullptr;
                    //	parent->_getObject((void**)&parentSceneNode);

                    //	if (parentSceneNode)
                    //	{
                    //		parentSceneNode->addChild(m_sceneNode);
                    //	}
                    //}
                }

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CSceneNodeOgre::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                const auto &state = getLoadingState();
                if( state != LoadingState::Unloaded )
                {
                    setLoadingState( LoadingState::Unloading );

                    auto applicationManager = core::IApplicationManager::instance();
                    WP_ASSERT( applicationManager );

                    if( !m_graphicsObjects.empty() )
                    {
                        for( auto obj : m_graphicsObjects )
                        {
                            obj->setOwner( nullptr );
                        }

                        m_sceneNode->detachAllObjects();
                        m_graphicsObjects.clear();
                    }

                    destroyStateContext();

                    // remove all the children
                    removeChildren();

                    if( auto pParent = getParent() )
                    {
                        auto parent = workphone::static_pointer_cast<CSceneNodeOgre>( pParent );
                        if( parent )
                        {
                            parent->removeChild( this );
                        }
                    }

                    if( m_nodeListener )
                    {
                        if( m_sceneNode )
                        {
                            m_sceneNode->setListener( nullptr );
                        }

                        delete m_nodeListener;
                        m_nodeListener = nullptr;
                    }

                    SmartPtr<CGraphicsSceneOgre> smgr = getCreator();
                    if( smgr )
                    {
                        // remove ogre scene node
                        if( m_sceneNode != nullptr )
                        {
                            m_sceneNode->detachAllObjects();
                            m_graphicsObjects.clear();

                            auto ogreSmgr = smgr->getSceneManager();
                            WP_ASSERT( ogreSmgr );

                            if( m_sceneNode != ogreSmgr->getRootSceneNode() )
                            {
                                ogreSmgr->destroySceneNode( m_sceneNode );
                            }

                            m_sceneNode = nullptr;
                        }
                    }
                    else
                    {
                        m_sceneNode = nullptr;
                    }

                    setCreator( nullptr );

                    GraphicsSceneNode::unload( nullptr );

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CSceneNodeOgre::attachObject( SmartPtr<IGraphicsObject> object )
        {
            try
            {
                WP_ASSERT( object );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                if( isThreadSafe() )
                {
                    WP_ASSERT( m_sceneNode );

                    const auto &objectLoadingState = object->getLoadingState();
                    if( objectLoadingState == LoadingState::Loaded )
                    {
                        ScopedLock lock( graphicsSystem );

                        if( object->isDerived<IGraphicsCamera>() )
                        {
                            object->setOwner( this );

                            auto camera = workphone::static_pointer_cast<CCameraOgre>( object );

                            Ogre::MovableObject *moveable = nullptr;
                            object->_getObject( (void **)&moveable );

                            if( moveable )
                            {
                                if( !moveable->isAttached() )
                                {
                                    m_sceneNode->attachObject( moveable );
                                }
                                else
                                {
                                    moveable->detachFromParent();
                                    m_sceneNode->attachObject( moveable );
                                }
                            }

                            object->setAttached( true );

                            m_graphicsObjects.push_back( object );
                        }
                        else
                        {
                            object->setOwner( this );

                            Ogre::MovableObject *moveable = nullptr;
                            object->_getObject( (void **)&moveable );

                            if( moveable )
                            {
                                if( !moveable->isAttached() )
                                {
                                    m_sceneNode->attachObject( moveable );
                                }
                                else
                                {
                                    WP_LOG_ERROR(
                                        "CSceneNodeOgre::attachObject Object already attached." );
                                }
                            }

                            object->setAttached( true );

                            m_graphicsObjects.push_back( object );
                        }
                    }
                    else
                    {
                        auto message = factoryManager->make_ptr<StateMessageObject>();
                        message->setType( STATE_MESSAGE_ATTACH_OBJECT );
                        message->setObject( object );
                        addMessage( message );
                    }

                    if( auto stateContext = getStateContext() )
                    {
                        stateContext->setDirty( true );
                    }
                }
                else
                {
                    auto message = factoryManager->make_ptr<StateMessageObject>();
                    message->setType( STATE_MESSAGE_ATTACH_OBJECT );
                    message->setObject( object );
                    addMessage( message );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CSceneNodeOgre::detachObject( SmartPtr<IGraphicsObject> object )
        {
            try
            {
                WP_ASSERT( object );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                if( isThreadSafe() )
                {
                    ScopedLock lock( graphicsSystem );

                    Ogre::MovableObject *moveable = nullptr;
                    object->_getObject( (void **)&moveable );

                    if( m_sceneNode )
                    {
                        if( moveable )
                        {
                            m_sceneNode->detachObject( moveable );
                        }
                    }

                    auto it = std::find( m_graphicsObjects.begin(), m_graphicsObjects.end(), object );
                    if( it != m_graphicsObjects.end() )
                    {
                        m_graphicsObjects.erase( it );
                    }

                    object->setAttached( false );
                    object->setOwner( nullptr );
                }
                else
                {
                    auto message = factoryManager->make_ptr<StateMessageObject>();
                    message->setType( STATE_MESSAGE_DETACH_OBJECT );
                    message->setObject( object );
                    addMessage( message );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CSceneNodeOgre::detachAllObjects()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();

            if( isThreadSafe() )
            {
                ScopedLock lock( graphicsSystem );

                for( auto obj : m_graphicsObjects )
                {
                    obj->setOwner( nullptr );
                }

                m_sceneNode->detachAllObjects();
                m_graphicsObjects.clear();
            }
            else
            {
                auto factoryManager = applicationManager->getFactoryManager();
                auto renderTask = graphicsSystem->getRenderTask();

                auto stateMessage = factoryManager->make_ptr<StateMessageType>();
                stateMessage->setType( STATE_MESSAGE_DETACH_ALL_OBJECTS );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->addMessage( renderTask, stateMessage );
                }
            }
        }

        SmartPtr<IGraphicsSceneNode> CSceneNodeOgre::addChildSceneNode( const String &name )
        {
            SmartPtr<IGraphicsSceneNode> sceneNode;

            auto creator = getCreator();

            if( name.length() == 0 )
            {
                sceneNode = creator->addSceneNode();
            }
            else
            {
                sceneNode = creator->addSceneNode( name );
            }

            addChild( sceneNode );

            return sceneNode;
        }

        SmartPtr<IGraphicsSceneNode> CSceneNodeOgre::addChildSceneNode( const Vector3F &position )
        {
            auto creator = getCreator();
            auto sceneNode = creator->addSceneNode();
            addChild( sceneNode );
            sceneNode->setPosition( position );
            return sceneNode;
        }

        u32 CSceneNodeOgre::getNumObjects() const
        {
            return (u32)m_graphicsObjects.size();
        }

        void CSceneNodeOgre::addChild( SmartPtr<IGraphicsSceneNode> child )
        {
            if( !child )
            {
                return;
            }

            if( isThreadSafe() && child->isLoaded() )
            {
                if( auto parent = child->getParent() )
                {
                    parent->removeChild( child );
                }

                auto pThis = getSharedFromThis<IGraphicsSceneNode>();
                child->setParent( pThis );

                m_children.push_back( child );

                Ogre::SceneNode *sceneNode = nullptr;
                child->_getObject( (void **)&sceneNode );

                if( m_sceneNode )
                {
                    if( sceneNode )
                    {
                        m_sceneNode->addChild( sceneNode );
                    }
                }
            }
            else
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto graphicsSystem = applicationManager->getGraphicsSystem();
                auto factoryManager = applicationManager->getFactoryManager();

                if( !child->isLoaded() )
                {
                    graphicsSystem->loadObject( child );
                }

                auto message = factoryManager->make_ptr<StateMessageObject>();
                message->setType( STATE_MESSAGE_ADD_CHILD );
                message->setObject( child );
                addMessage( message );
            }
        }

        bool CSceneNodeOgre::removeChild( SmartPtr<IGraphicsSceneNode> child )
        {
            if( isThreadSafe() )
            {
                auto it = std::find( m_children.begin(), m_children.end(), child );
                if( it != m_children.end() )
                {
                    m_children.erase( it );

                    child->setParent( nullptr );

                    Ogre::SceneNode *sceneNode = nullptr;
                    child->_getObject( (void **)&sceneNode );

                    if( sceneNode )
                    {
                        m_sceneNode->removeChild( sceneNode );
                    }

                    return true;
                }
            }
            else
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto factoryManager = applicationManager->getFactoryManager();

                auto message = factoryManager->make_ptr<StateMessageObject>();
                message->setType( STATE_MESSAGE_REMOVE_CHILD );
                message->setObject( child );
                addMessage( message );
            }

            return false;
        }

        Array<SmartPtr<IGraphicsSceneNode>> CSceneNodeOgre::getChildren() const
        {
            return { m_children.begin(), m_children.end() };
        }

        u32 CSceneNodeOgre::getNumChildren() const
        {
            return static_cast<u32>( m_children.size() );
        }

        void CSceneNodeOgre::needUpdate( bool forceParentUpdate )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();

            ScopedLock lock( graphicsSystem );

            m_sceneNode->needUpdate( forceParentUpdate );
        }

        SmartPtr<IGraphicsSceneNode> CSceneNodeOgre::clone( SmartPtr<IGraphicsSceneNode> parent,
                                                    const String &name ) const
        {
            SmartPtr<CSceneNodeOgre> sceneNode = parent->addChildSceneNode();

            // set properties
            sceneNode->setPosition( getPosition() );
            sceneNode->setOrientation( getOrientation() );
            sceneNode->setScale( getScale() );

            // clone attached objects
            for( u32 i = 0; i < m_graphicsObjects.size(); ++i )
            {
                const SmartPtr<IGraphicsObject> &graphicsObject = m_graphicsObjects[i];

                SmartPtr<IGraphicsObject> cloneGraphicsObject = graphicsObject->clone();
                sceneNode->attachObject( cloneGraphicsObject );
            }

            return sceneNode;
        }

        void CSceneNodeOgre::showBoundingBox( bool show )
        {
            if( m_sceneNode )
            {
                m_sceneNode->showBoundingBox( show );
            }
        }

        bool CSceneNodeOgre::getShowBoundingBox() const
        {
            if( m_sceneNode )
            {
                return m_sceneNode->getShowBoundingBox();
            }

            return false;
        }

        void CSceneNodeOgre::_getObject( void **ppObject ) const
        {
            *ppObject = m_sceneNode;
        }

        Ogre::SceneNode *CSceneNodeOgre::getSceneNode() const
        {
            return m_sceneNode;
        }

        void CSceneNodeOgre::setSceneNode( Ogre::SceneNode *sceneNode )
        {
            m_sceneNode = sceneNode;
        }

        void CSceneNodeOgre::_updateBoundingBox()
        {
        }

        void CSceneNodeOgre::updateBounds()
        {
            if( m_sceneNode )
            {
                m_sceneNode->_updateBounds();
            }
        }

        void CSceneNodeOgre::setVisibilityFlags( u32 flags )
        {
            //m_visibilityFlags = flags;

            for( u32 i = 0; i < m_graphicsObjects.size(); ++i )
            {
                SmartPtr<IGraphicsObject> graphicsObject = m_graphicsObjects[i];
                graphicsObject->setVisibilityFlags( flags );
            }

            //for( u32 i = 0; i < m_children.size(); ++i )
            //{
            //    m_children[i]->setVisibilityFlags( flags );
            //}
        }

        u32 CSceneNodeOgre::getVisibilityFlags() const
        {
            return 0;
        }

        s32 CSceneNodeOgre::ScriptReceiver::setProperty( hash_type id, const Parameter &param )
        {
            // if(id == StringUtil::VISIBILITY_MASK_HASH)
            //{
            //	m_node->setVisibilityFlags(param.data.iData);
            // }
            // else if(id == StringUtil::VISIBLE_HASH)
            //{
            //	m_node->setVisible(param.data.bData ? true : false);
            // }

            return 0;
        }

        s32 CSceneNodeOgre::ScriptReceiver::setProperty( hash_type id, const Parameters &params )
        {
            // if(id == StringUtil::POSITION_3_HASH)
            //{
            //	Vector3F position;
            //	position[0] = params[0].data.fData;
            //	position[1] = params[1].data.fData;
            //	position[2] = params[2].data.fData;
            //	m_node->setPosition(position);

            //	return 0;
            //}
            // else if(id == StringUtil::ORIENTATION_HASH)
            //{
            //	QuaternionF orientation;
            //	orientation.W() = params[0].data.fData;
            //	orientation.X() = params[1].data.fData;
            //	orientation.Y() = params[2].data.fData;
            //	orientation.Z() = params[3].data.fData;
            //	m_node->setOrientation(orientation);

            //	return 0;
            //}

            return 0;
        }

        s32 CSceneNodeOgre::ScriptReceiver::setProperty( hash_type hash, void *param )
        {
            // if(hash == StringUtil::POSITION_3_HASH)
            //{
            //	Vector3F position = *static_cast<Vector3F*>(param);
            //	m_node->setPosition(position);
            //	return 0;
            // }
            // else if(hash == StringUtil::SCALE_3_HASH)
            //{
            //	Vector3F scale = *static_cast<Vector3F*>(param);
            //	m_node->setScale(scale);
            //	return 0;
            // }
            // else if(hash == StringUtil::ORIENTATION_HASH)
            //{
            //	QuaternionF orientation = *static_cast<QuaternionF*>(param);
            //	m_node->setOrientation(orientation);
            //	return 0;
            // }

            return 0;
        }

        s32 CSceneNodeOgre::ScriptReceiver::getProperty( hash_type hash, void *param ) const
        {
            // if(hash == StringUtil::POSITION_3_HASH)
            //{
            //	Vector3F& position = *static_cast<Vector3F*>(param);
            //	position = m_node->getPosition();
            //	return 0;
            // }
            // else if(hash == StringUtil::SCALE_3_HASH)
            //{
            //	Vector3F& scale = *static_cast<Vector3F*>(param);
            //	scale = m_node->getScale();
            //	return 0;
            // }
            // else if(hash == StringUtil::ORIENTATION_HASH)
            //{
            //	QuaternionF& orientation = *static_cast<QuaternionF*>(param);
            //	orientation = m_node->getOrientation();
            //	return 0;
            // }

            return 0;
        }

        s32 CSceneNodeOgre::ScriptReceiver::getProperty( hash_type id, Parameters &params ) const
        {
            return 0;
        }

        s32 CSceneNodeOgre::ScriptReceiver::getProperty( hash_type id, Parameter &param ) const
        {
            return 0;
        }

        s32 CSceneNodeOgre::ScriptReceiver::callFunction( hash_type hashId, const Parameters &params,
                                                          Parameters &results )
        {
            // if(hashId == StringUtil::ADD_HASH)
            //{
            //	m_node->add();
            //	return 0;
            // }
            // else if(hashId == StringUtil::REMOVE_HASH)
            //{
            //	m_node->remove();
            //	return 0;
            // }

            return 0;
        }

        CSceneNodeOgre::ScriptReceiver::ScriptReceiver( CSceneNodeOgre *node ) : m_node( node )
        {
        }

        void CSceneNodeOgre::setProperties( SmartPtr<Properties> properties )
        {
        }

        Array<SmartPtr<ISharedObject>> CSceneNodeOgre::getChildObjects() const
        {
            Array<SmartPtr<ISharedObject>> childObjects;

            auto creator = getCreator();
            childObjects.push_back( creator );

            auto parent = getParent();
            childObjects.push_back( parent );

            // childObjects.push_back( m_stateContext );
            // childObjects.push_back( m_stateListener );
            return childObjects;
        }

        SmartPtr<Properties> CSceneNodeOgre::getProperties() const
        {
            auto properties = workphone::make_ptr<Properties>();

            auto name = getName();

            properties->setProperty( IGraphicsSceneNode::nameStr, name );

            auto iNumAttachedObjects = getNumObjects();
            properties->setProperty( IGraphicsSceneNode::numObjectsStr, iNumAttachedObjects );

            if( m_sceneNode )
            {
                auto sceneNodePosition = m_sceneNode->getPosition();
                auto sceneNodeScale = m_sceneNode->getScale();
                auto sceneNodeOrientation = OgreUtil::convert( m_sceneNode->getOrientation() );

                Euler<real_Num> euler( sceneNodeOrientation );
                Vector3<real_Num> localRotation = euler.toDegrees();

                Euler<real_Num> euler2( sceneNodeOrientation.normaliseCopy() );
                localRotation = euler2.toDegrees();

                Euler<real_Num> euler3;  //(m_state->getOrientation());
                Vector3<real_Num> localRotation2 = euler3.toDegrees();

                properties->setProperty( IGraphicsSceneNode::sceneNodePositionStr,
                                         Ogre::StringConverter::toString( sceneNodePosition ).c_str() );
                properties->setProperty( IGraphicsSceneNode::sceneNodeScaleStr,
                                         Ogre::StringConverter::toString( sceneNodeScale ).c_str() );
                properties->setProperty( IGraphicsSceneNode::sceneNodeOrientationStr, localRotation );

                properties->setProperty( IGraphicsSceneNode::stateOrientationStr, localRotation2 );
            }

            return properties;
        }

        bool CSceneNodeOgre::SceneNodeStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            WP_ASSERT( message );
            WP_ASSERT( m_owner );

            if( auto owner = getOwner() )
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto graphicsSystem = applicationManager->getGraphicsSystem();
                auto factoryManager = applicationManager->getFactoryManager();
                auto renderTask = graphicsSystem->getRenderTask();

                auto currentTaskId = Thread::getCurrentTask();
                if( currentTaskId == renderTask )
                {
                    if( !message )
                    {
                        return false;
                    }

                    if( message->isExactly<StateMessageVector3>() )
                    {
                        auto positionMessage =
                            workphone::static_pointer_cast<StateMessageVector3>( message );
                        auto value = positionMessage->getValue();
                        auto type = message->getType();

                        if( type == STATE_MESSAGE_POSITION )
                        {
                            m_owner->setPosition( value );
                        }
                        else if( type == STATE_MESSAGE_SCALE )
                        {
                            m_owner->setScale( value );
                        }
                        else if( type == STATE_MESSAGE_LOOK_AT )
                        {
                            m_owner->lookAt( value );
                        }
                    }
                    else if( message->isExactly<StateMessageOrientation>() )
                    {
                        auto orientationMessage =
                            workphone::static_pointer_cast<StateMessageOrientation>( message );
                        m_owner->setOrientation( orientationMessage->getOrientation() );
                    }
                    else if( message->isExactly<StateMessageObject>() )
                    {
                        auto objectMessage =
                            workphone::static_pointer_cast<StateMessageObject>( message );
                        WP_ASSERT( objectMessage );

                        if( objectMessage->getType() == STATE_MESSAGE_ADD_CHILD )
                        {
                            m_owner->addChild( objectMessage->getObject() );
                        }
                        else if( objectMessage->getType() == STATE_MESSAGE_ATTACH_OBJECT )
                        {
                            m_owner->attachObject( objectMessage->getObject() );
                        }
                        else if( objectMessage->getType() == STATE_MESSAGE_DETACH_OBJECT )
                        {
                            m_owner->detachObject( objectMessage->getObject() );
                        }
                        else if( objectMessage->getType() == STATE_MESSAGE_DETACH_ALL_OBJECTS )
                        {
                            m_owner->detachAllObjects();
                        }
                    }
                    else if( message->isExactly<StateMessageType>() )
                    {
                        auto stateMessage = workphone::static_pointer_cast<StateMessageType>( message );
                        if( stateMessage->getType() == STATE_MESSAGE_ADD )
                        {
                        }
                        else if( stateMessage->getType() == STATE_MESSAGE_REMOVE )
                        {
                        }
                    }
                    else if( message->isExactly<StateMessageVisible>() )
                    {
                        auto visibleMessage =
                            workphone::static_pointer_cast<StateMessageVisible>( message );
                        //m_owner->setVisible( visibleMessage->isVisible(), visibleMessage->getCascade() );
                    }
                }
            }

            return false;
        }

        bool CSceneNodeOgre::SceneNodeStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            WP_ASSERT( m_owner );

            Ogre::SceneNode *sceneNode = nullptr;
            m_owner->_getObject( (void **)&sceneNode );

            if( sceneNode )
            {
                auto stateData = state->getData();

                if( stateData->isDerived<TransformStateData>() )
                {
                    auto sceneNodeState = SafeReadPtr<TransformStateData>( stateData );
                    if( sceneNodeState )
                    {
                        auto &t = sceneNodeState->localTransform;
                        auto position = t.getPosition();
                        auto scale = t.getScale();
                        auto orientation = t.getOrientation();

                        auto pos = OgreUtil::convertToOgre( position );
                        auto orient = OgreUtil::convertToOgre( orientation );
                        auto vScale = OgreUtil::convertToOgre( scale );

                        //if( sceneNode->getName().find( "poly" ) != String::npos )
                        //{
                        //    int stop = 0;
                        //    stop = 0;
                        //}

                        //auto visible = sceneNodeState->isVisible();
                        //sceneNode->setVisible( visible );

                        if( ( !OgreUtil::equals( pos, sceneNode->getPosition() ) ) ||
                            ( !OgreUtil::equals( orient, sceneNode->getOrientation() ) ) ||
                            ( !OgreUtil::equals( vScale, sceneNode->getScale() ) ) )
                        {
                            sceneNode->setPosition( pos );

                            orient.normalise();
                            sceneNode->setOrientation( orient );

                            sceneNode->setScale( vScale );

                            sceneNode->_update( true, false );

                            auto worldPosition = sceneNode->_getDerivedPosition();
                            auto worldOrientation = sceneNode->_getDerivedOrientation();
                            auto worldScale = sceneNode->_getDerivedScale();

                            //sceneNodeState->setAbsolutePosition(
                            //    Vector3<real_Num>( worldPosition.x, worldPosition.y, worldPosition.z ) );
                            //sceneNodeState->setAbsoluteOrientation(
                            //    Quaternion<real_Num>( worldOrientation.w, worldOrientation.x,
                            //                          worldOrientation.y, worldOrientation.z ) );
                            //sceneNodeState->setAbsoluteScale(
                            //    Vector3<real_Num>( worldScale.x, worldScale.y, worldScale.z ) );
                        }
                    }
                }

                if( stateData->isDerived<SceneNodeStateData>() )
                {
                    auto sceneNodeState = SafeReadPtr<SceneNodeStateData>( stateData );
                    if( sceneNodeState )
                    {
                        auto axis = OgreUtil::convertToOgre( sceneNodeState->yawFixedAxis );
                        sceneNode->setFixedYawAxis( sceneNodeState->yawFixed, axis );
                    }
                }
            }

            return false;
        }

        CSceneNodeOgre *CSceneNodeOgre::SceneNodeStateListener::getOwner() const
        {
            return m_owner;
        }

        void CSceneNodeOgre::SceneNodeStateListener::setOwner( CSceneNodeOgre *owner )
        {
            m_owner = owner;
        }

        CSceneNodeOgre::SceneNodeStateListener::SceneNodeStateListener() = default;

        CSceneNodeOgre::SceneNodeStateListener::~SceneNodeStateListener()
        {
            m_owner = nullptr;
        }

        AABB3F CSceneNodeOgre::calculateAABB() const
        {
            ScopedLock lock( this );

            Ogre::AxisAlignedBox box;

            for( u32 i = 0; i < m_sceneNode->numAttachedObjects(); ++i )
            {
                Ogre::MovableObject *object = m_sceneNode->getAttachedObject( i );
                if( object->getTypeFlags() & Ogre::SceneManager::ENTITY_TYPE_MASK )
                {
                    auto entity = static_cast<Ogre::Entity *>( object );
                    box.merge( entity->getBoundingBox() );
                }
            }

            const Ogre::Vector3 &minPoint = box.getMinimum();
            const Ogre::Vector3 &maxPoint = box.getMaximum();

            return AABB3F( minPoint.x, minPoint.y, minPoint.z, maxPoint.x, maxPoint.y, maxPoint.z );
        }

        void CSceneNodeOgre::destroyStateContext()
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();

            if( auto stateContext = getStateContext() )
            {
                if( auto stateListener = getStateListener() )
                {
                    stateContext->removeStateListener( stateListener );
                }

                if( stateManager )
                {
                    stateManager->removeStateContext( stateContext );
                }

                stateContext->unload( nullptr );
                setStateContext( nullptr );
            }

            if( auto stateListener = getStateListener() )
            {
                stateListener->unload( nullptr );
                setStateListener( nullptr );
            }
        }

        CSceneNodeOgre::NodeListener::NodeListener( CSceneNodeOgre *owner ) : m_owner( owner )
        {
        }

        CSceneNodeOgre::NodeListener::~NodeListener()
        {
        }

        void CSceneNodeOgre::NodeListener::nodeUpdated( const Ogre::Node *node )
        {
        }

        void CSceneNodeOgre::NodeListener::nodeDestroyed( const Ogre::Node *node )
        {
        }

        void CSceneNodeOgre::NodeListener::nodeAttached( const Ogre::Node *node )
        {
            Ogre::SceneManager *pSceneManager = nullptr;

            auto smgr = m_owner->getCreator();
            smgr->_getObject( (void **)&pSceneManager );

            auto sceneNode = (Ogre::SceneNode *)node;
            if( pSceneManager->getRootSceneNode() == sceneNode->getParentSceneNode() )
            {
                // m_owner->registerForUpdates(true, false);
            }
        }

        void CSceneNodeOgre::NodeListener::nodeDetached( const Ogre::Node *node )
        {
            Ogre::SceneManager *pSceneManager = nullptr;

            auto smgr = m_owner->getCreator();
            smgr->_getObject( (void **)&pSceneManager );

            auto sceneNode = (Ogre::SceneNode *)node;
            if( pSceneManager->getRootSceneNode() == sceneNode->getParentSceneNode() )
            {
                // m_owner->registerForUpdates(false, false);
            }
        }
    }  // end namespace render
}  // namespace workphone
