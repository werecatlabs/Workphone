#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CSceneNodeOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSceneOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CCameraOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CLightOgreNext.hpp>
#include <WPGraphicsOgreNext/OgreUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreEntity.h>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, CSceneNodeOgreNext, GraphicsSceneNode );

    CSceneNodeOgreNext::CSceneNodeOgreNext()
    {
        setupStateContext();

        constexpr auto size = sizeof( CSceneNodeOgreNext );
    }

    CSceneNodeOgreNext::CSceneNodeOgreNext( SmartPtr<IGraphicsScene> creator ) : GraphicsSceneNode()
    {
        m_creator = creator;

        setupStateContext();
    }

    CSceneNodeOgreNext::~CSceneNodeOgreNext()
    {
        unload( nullptr );
    }

    void CSceneNodeOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );

            ScopedLock lock( graphicsSystem );

            auto task = Thread::getCurrentTask();
            auto renderTask = graphicsSystem->getRenderTask();
            //WP_ASSERT( Thread::getTaskFlag( Thread::Render_Flag ) );

            setLoadingState( LoadingState::Loading );

            Ogre::SceneManager *smgr = nullptr;

            if( auto creator = getCreator() )
            {
                creator->_getObject( reinterpret_cast<void **>( &smgr ) );
            }

            if( !smgr )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            auto name = getName();
            WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );

            auto sceneType = isStatic() ? Ogre::SCENE_STATIC : Ogre::SCENE_DYNAMIC;
            m_sceneNode = smgr->createSceneNode( sceneType );

            auto graphicsObjects = getObjects();
            for( auto graphicsObject : graphicsObjects )
            {
                if( !graphicsObject->isLoaded() )
                {
                    const auto loadingState = graphicsObject->getLoadingState();
                    if( loadingState != LoadingState::LoadingQueued &&
                        loadingState != LoadingState::Loading )
                    {
                        graphicsObject->load( nullptr );
                    }
                    else
                    {
                        // LoadingQueued/Loading objects are owned by the render task.
                        // Reading their native pointer here would race its creation.
                        continue;
                    }
                }

                if( !graphicsObject->isLoaded() )
                {
                    continue;
                }

                Ogre::MovableObject *moveable = nullptr;
                graphicsObject->_getObject( reinterpret_cast<void **>( &moveable ) );

                if( moveable )
                {
                    moveable->setStatic( isStatic() );

                    if( !moveable->isAttached() )
                    {
                        m_sceneNode->attachObject( moveable );
                    }
                }
            }

            if( auto stateContext = getStateContext() )
            {
                if( auto transformState =
                        stateContext->invalidateStateDataById<TransformStateData>( getId(), false ) )
                {
                    const auto &transform = transformState->localTransform;
                    const auto &position = transform.getPosition();
                    const auto &orientation = transform.getOrientation();
                    const auto &scale = transform.getScale();

                    auto pos = OgreUtil::convertToOgre( position );
                    if( !OgreUtil::equals( pos, m_sceneNode->getPosition() ) )
                    {
                        m_sceneNode->setPosition( pos );
                    }

                    auto orient = OgreUtil::convertToOgre( orientation );
                    if( !OgreUtil::equals( orient, m_sceneNode->getOrientation() ) )
                    {
                        orient.normalise();
                        m_sceneNode->setOrientation( orient );
                    }

                    auto vScale = OgreUtil::convertToOgre( scale );
                    if( !OgreUtil::equals( vScale, m_sceneNode->getScale() ) )
                    {
                        // check if scale is valid
                        if( vScale.x == 0.0f || vScale.y == 0.0f || vScale.z == 0.0f )
                        {
                            vScale = Ogre::Vector3::UNIT_SCALE * 0.0001f;
                        }

                        m_sceneNode->setScale( vScale );
                    }

                    transformState->derivedTransform = transform;

                    auto derivedPosition = m_sceneNode->_getDerivedPositionUpdated();
                    auto derivedScale = m_sceneNode->_getDerivedScaleUpdated();
                    auto derivedOrientation = m_sceneNode->_getDerivedOrientationUpdated();

                    auto &derivedTransform = transformState->worldTransform;

                    auto derivedPos = OgreUtil::convert( derivedPosition );
                    auto derivedOri = OgreUtil::convert( derivedOrientation );
                    auto derivedScl = OgreUtil::convert( derivedScale );

                    derivedTransform.setPosition( derivedPos );
                    derivedTransform.setOrientation( derivedOri );
                    derivedTransform.setScale( derivedScl );

                    auto localAABB = calculateAABB();
                    setLocalAABB( localAABB );

                    Matrix4<real_Num> transformMat;
                    transformMat.makeTransform( derivedPos, derivedScl, derivedOri );

                    auto worldAABB = localAABB.transform( transformMat );
                    setWorldAABB( worldAABB );
                }
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CSceneNodeOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                // Native attachments are created and removed under the graphics-system
                // lock. Use the same lock while detaching and destroying this Ogre node
                // so a queued particle load cannot attach to a node being destroyed.
                ScopedLock lock( graphicsSystem );

                setStateContext( nullptr );
                setStateListener( nullptr );

                // remove all the children
                removeChildren();

                if( auto pParent = getParent() )
                {
                    auto parent = workphone::static_pointer_cast<CSceneNodeOgreNext>( pParent );
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

                SmartPtr<CGraphicsSceneOgreNext> smgr = getCreator();
                if( smgr )
                {
                    // remove ogre scene node
                    if( m_sceneNode != nullptr )
                    {
                        m_sceneNode->detachAllObjects();
                        m_graphicsObjects.clear();

                        auto ogreSmgr = smgr->getSceneManager();
                        WP_ASSERT( ogreSmgr );

                        ogreSmgr->destroySceneNode( m_sceneNode );
                        m_sceneNode = nullptr;
                    }
                }
                else
                {
                    m_sceneNode = nullptr;
                }

                GraphicsSceneNode::unload( data );
                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CSceneNodeOgreNext::setupNode( Ogre::SceneNode *sceneNode )
    {
        m_sceneNode = sceneNode;

        // m_nodeListener = new NodeListener(this);
        // m_sceneNode->setListener(m_nodeListener);

        setLoadingState( LoadingState::Loaded );
    }

    void CSceneNodeOgreNext::destroy()
    {
        // remove ogre scene node
        if( m_sceneNode != nullptr )
        {
            // remove all the objects from the scene node
            detachAllObjects();

            Ogre::SceneManager *ogreSmgr = nullptr;

            if( auto creator = getCreator() )
            {
                creator->_getObject( reinterpret_cast<void **>( &ogreSmgr ) );
            }

            ogreSmgr->destroySceneNode( m_sceneNode );
            m_sceneNode = nullptr;
        }
    }

    void CSceneNodeOgreNext::setFixedYawAxis( bool useFixed, const Vector3F &fixedAxis )
    {
        if( m_sceneNode )
        {
            Ogre::Vector3 axis( fixedAxis.X(), fixedAxis.Y(), fixedAxis.Z() );
            m_sceneNode->setFixedYawAxis( useFixed, axis );
        }
    }

    void CSceneNodeOgreNext::setStatic( bool isstatic )
    {
        if( GraphicsSceneNode::isStatic() == isstatic )
        {
            return;
        }

        GraphicsSceneNode::setStatic( isstatic );

        if( m_sceneNode )
        {
            m_sceneNode->setStatic( isstatic );

            for( u32 i = 0; i < m_sceneNode->numAttachedObjects(); ++i )
            {
                if( auto object = m_sceneNode->getAttachedObject( i ) )
                {
                    object->setStatic( isstatic );
                }
            }
        }
    }

    auto CSceneNodeOgreNext::getLocalAABB() const -> AABB3F
    {
        auto localAABB = calculateAABB();
        return localAABB;
    }

    auto CSceneNodeOgreNext::getWorldAABB() const -> AABB3F
    {
        Matrix4<real_Num> transform;
        transform.makeTransform( getPosition(), getScale(), getOrientation() );
        auto worldAABB = calculateAABB();
        worldAABB.transform( transform );
        return worldAABB;
    }

    void CSceneNodeOgreNext::attachObject( SmartPtr<IGraphicsObject> object )
    {
        if( !object )
        {
            WP_LOG_ERROR( "CSceneNodeOgre::attachObject Object is null." );
            return;
        }

        if( std::find( m_graphicsObjects.begin(), m_graphicsObjects.end(), object ) !=
            m_graphicsObjects.end() )
        {
            WP_LOG_WARNING( "CSceneNodeOgre::attachObject Object is already attached to this node." );
            return;
        }

        // Record the logical attachment immediately. Native graphics objects may be
        // created asynchronously, and their load path needs to know the intended node.
        object->setOwner( this );

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        WP_ASSERT( graphicsSystem );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        if( object->isDerived<IParticleSystem>() && !object->isLoaded() )
        {
            if( std::find( m_graphicsObjects.begin(), m_graphicsObjects.end(), object ) ==
                m_graphicsObjects.end() )
            {
                m_graphicsObjects.push_back( object );
            }

            // Particle presets are configured on the wrapper before attachment.
            // Native creation and attachment are both completed by the render task;
            // returning here prevents this thread from racing Ogre's attachment index.
            const auto loadingState = object->getLoadingState();
            if( loadingState != LoadingState::LoadingQueued &&
                loadingState != LoadingState::Loading )
            {
                graphicsSystem->loadObject( object, true );
            }

            return;
        }

        const auto renderTask = graphicsSystem->getRenderTask();
        const auto stateTask = graphicsSystem->getStateTask();
        const auto task = Thread::getCurrentTask();

        if( isThreadSafe() )
        {
            if( !isLoaded() )
            {
                load( nullptr );
            }

            if( !object->isLoaded() )
            {
                if( auto creator = object->getCreator() )
                {
                    // A queued graphics load may already be executing on the render task.
                    // Loading the same object synchronously here races native object creation
                    // and can leave Ogre's attachment index out of sync.
                    const auto loadingState = object->getLoadingState();
                    if( creator->isLoaded() && loadingState != LoadingState::LoadingQueued &&
                        loadingState != LoadingState::Loading )
                    {
                        object->load( nullptr );
                    }
                }
            }

            if( object->isLoaded() )
            {
                ScopedLock lock( graphicsSystem );

                if( object->isDerived<IGraphicsCamera>() )
                {
                    auto camera = workphone::static_ptr_cast<CCameraOgreNext>( object );

                    Ogre::MovableObject *movable = nullptr;
                    object->_getObject( reinterpret_cast<void **>( &movable ) );

                    if( movable )
                    {
                        movable->setStatic( isStatic() );

                        if( !movable->isAttached() )
                        {
                            if( m_sceneNode )
                            {
                                m_sceneNode->attachObject( movable );
                            }
                        }
                        else
                        {
                            movable->detachFromParent();

                            if( m_sceneNode )
                            {
                                m_sceneNode->attachObject( movable );
                            }
                        }

                        object->setOwner( this );
                        if( std::find( m_graphicsObjects.begin(), m_graphicsObjects.end(), object ) ==
                            m_graphicsObjects.end() )
                        {
                            m_graphicsObjects.push_back( object );
                        }
                    }
                    else
                    {
                        auto message = factoryManager->make_ptr<StateMessageObject>();
                        message->setType( STATE_MESSAGE_ATTACH_OBJECT );
                        message->setObject( object );

                        if( auto stateContext = getStateContext() )
                        {
                            stateContext->addMessage( stateTask, message );
                        }
                    }
                }
                else if( object->isDerived<IGraphicsLight>() )
                {
                    auto light = workphone::static_pointer_cast<CLightOgreNext>( object );

                    auto lightNode = light->getLightNode();
                    if( lightNode )
                    {
                        lightNode->setStatic( isStatic() );

                        auto lightNodeParent = lightNode->getParentSceneNode();
                        if( lightNodeParent )
                        {
                            lightNodeParent->removeChild( lightNode );
                        }

                        if( m_sceneNode )
                        {
                            m_sceneNode->addChild( lightNode );
                        }

                        object->setOwner( this );
                        m_graphicsObjects.push_back( object );
                    }
                    else
                    {
                        auto message = factoryManager->make_ptr<StateMessageObject>();
                        message->setSender( this );
                        message->setType( STATE_MESSAGE_ATTACH_OBJECT );
                        message->setObject( object );

                        if( auto stateContext = getStateContext() )
                        {
                            stateContext->addMessage( stateTask, message );
                        }
                    }
                }
                else
                {
                    Ogre::MovableObject *movable = nullptr;
                    object->_getObject( reinterpret_cast<void **>( &movable ) );

                    if( movable && m_sceneNode )
                    {
                        if( !movable->isAttached() )
                        {
                            movable->setStatic( isStatic() );
                            m_sceneNode->attachObject( movable );
                        }
                        else if( movable->getParentSceneNode() != m_sceneNode )
                        {
                            movable->detachFromParent();
                            movable->setStatic( isStatic() );
                            m_sceneNode->attachObject( movable );
                        }

                        object->setOwner( this );
                        if( std::find( m_graphicsObjects.begin(), m_graphicsObjects.end(), object ) ==
                            m_graphicsObjects.end() )
                        {
                            m_graphicsObjects.push_back( object );
                        }
                    }
                    else
                    {
                        auto message = factoryManager->make_ptr<StateMessageObject>();
                        message->setSender( this );
                        message->setType( STATE_MESSAGE_ATTACH_OBJECT );
                        message->setObject( object );

                        if( auto stateContext = getStateContext() )
                        {
                            stateContext->addMessage( stateTask, message );
                        }
                    }
                }

                object->makeDirty();
            }
            else
            {
                auto message = factoryManager->make_ptr<StateMessageObject>();
                message->setSender( this );
                message->setType( STATE_MESSAGE_ATTACH_OBJECT );
                message->setObject( object );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->addMessage( stateTask, message );
                }
            }
        }
        else
        {
            auto message = factoryManager->make_ptr<StateMessageObject>();
            message->setSender( this );
            message->setType( STATE_MESSAGE_ATTACH_OBJECT );
            message->setObject( object );

            if( auto stateContext = getStateContext() )
            {
                stateContext->addMessage( stateTask, message );
            }
        }
    }

    void CSceneNodeOgreNext::detachObject( SmartPtr<IGraphicsObject> object )
    {
        if( !object )
        {
            WP_LOG_ERROR( "CSceneNodeOgre::detachObject Object is null." );
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        WP_ASSERT( graphicsSystem );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto renderTask = graphicsSystem->getRenderTask();
        auto stateTask = graphicsSystem->getStateTask();
        auto task = Thread::getCurrentTask();

        if( isLoaded() && task == renderTask )
        {
            ScopedLock lock( graphicsSystem );

            if( object->isDerived<IGraphicsLight>() )
            {
                auto light = workphone::static_pointer_cast<CLightOgreNext>( object );
                auto lightNode = light->getLightNode();
                if( lightNode )
                {
                    if( auto parent = lightNode->getParentSceneNode() )
                    {
                        if( parent == m_sceneNode )
                        {
                            parent->removeChild( lightNode );
                        }
                    }
                }
            }
            else
            {
                Ogre::MovableObject *moveable = nullptr;
                object->_getObject( reinterpret_cast<void **>( &moveable ) );

                if( moveable )
                {
                    if( auto parent = moveable->getParentSceneNode() )
                    {
                        if( parent == m_sceneNode )
                        {
                            parent->detachObject( moveable );
                        }
                    }
                }
            }

            object->setOwner( nullptr );
            m_graphicsObjects.erase(
                std::remove( m_graphicsObjects.begin(), m_graphicsObjects.end(), object ),
                m_graphicsObjects.end() );
        }
        else
        {
            auto message = factoryManager->make_ptr<StateMessageObject>();
            message->setSender( this );
            message->setType( STATE_MESSAGE_DETACH_OBJECT );
            message->setObject( object );

            if( auto stateContext = getStateContext() )
            {
                stateContext->addMessage( stateTask, message );
            }
        }
    }

    void CSceneNodeOgreNext::detachAllObjects()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        const auto renderTask = graphicsSystem->getRenderTask();
        const auto stateTask = graphicsSystem->getStateTask();
        const auto task = Thread::getCurrentTask();

        const auto &loadingState = getLoadingState();
        if( loadingState == LoadingState::Loaded && task == renderTask )
        {
            ScopedLock lock( graphicsSystem );
            m_sceneNode->detachAllObjects();
            m_graphicsObjects.clear();
        }
        else
        {
            auto message = factoryManager->make_ptr<StateMessageType>();
            message->setSender( this );
            message->setType( STATE_MESSAGE_DETACH_ALL_OBJECTS );

            auto stateContext = getStateContext();
            if( stateContext )
            {
                stateContext->addMessage( stateTask, message );
            }
        }
    }

    auto CSceneNodeOgreNext::addChildSceneNode( const String &name ) -> SmartPtr<IGraphicsSceneNode>
    {
        SmartPtr<IGraphicsSceneNode> sceneNode;

        if( auto creator = getCreatorPtr() )
        {
            if( StringUtil::isNullOrEmpty( name ) )
            {
                sceneNode = creator->addSceneNode();
            }
            else
            {
                sceneNode = creator->addSceneNode( name );
            }
        }

        addChild( sceneNode );

        return sceneNode;
    }

    auto CSceneNodeOgreNext::addChildSceneNode( const Vector3F &position )
        -> SmartPtr<IGraphicsSceneNode>
    {
        if( auto creator = getCreatorPtr() )
        {
            auto sceneNode = creator->addSceneNode();
            addChild( sceneNode );
            sceneNode->setPosition( position );
            return sceneNode;
        }

        return nullptr;
    }

    void CSceneNodeOgreNext::addChild( SmartPtr<IGraphicsSceneNode> child )
    {
        if( child )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );

            GraphicsSceneNode::addChild( child );
            child->setStatic( isStatic() );

            if( !isLoaded() )
            {
                load( nullptr );
            }

            if( !child->isLoaded() )
            {
                child->load( nullptr );
            }

            Ogre::SceneNode *sceneNode = nullptr;
            child->_getObject( reinterpret_cast<void **>( &sceneNode ) );

            if( m_sceneNode )
            {
                if( sceneNode )
                {
                    if( auto parent = sceneNode->getParent() )
                    {
                        parent->removeChild( sceneNode );
                    }

                    m_sceneNode->addChild( sceneNode );
                }
            }
        }
    }

    auto CSceneNodeOgreNext::removeChild( SmartPtr<IGraphicsSceneNode> child ) -> bool
    {
        if( !child )
        {
            return false;
        }

        // Do not enqueue a render-thread removal for an object that is not a child. Apart
        // from reporting success incorrectly, that stale message can detach the object from
        // its real parent when it is processed later.
        if( std::find( m_children.begin(), m_children.end(), child ) == m_children.end() )
        {
            return false;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        const auto renderTask = graphicsSystem->getRenderTask();
        const auto stateTask = graphicsSystem->getStateTask();
        const auto task = Thread::getCurrentTask();

        const auto &loadingState = getLoadingState();
        if( loadingState == LoadingState::Unloading )
        {
            auto it = std::find( m_children.begin(), m_children.end(), child );
            if( it != m_children.end() )
            {
                m_children.erase( it );

                child->setParent( nullptr );

                Ogre::SceneNode *sceneNode = nullptr;
                child->_getObject( reinterpret_cast<void **>( &sceneNode ) );

                if( sceneNode )
                {
                    if( auto parent = sceneNode->getParent() )
                    {
                        parent->removeChild( sceneNode );
                    }
                }

                return true;
            }

            return false;
        }

        if( loadingState != LoadingState::Unloading && loadingState != LoadingState::Unloaded )
        {
            auto isRenderTask = task == renderTask;
            if( loadingState == LoadingState::Loaded && isRenderTask )
            {
                if( task == renderTask || applicationManager->getQuit() ||
                    !applicationManager->isRunning() )
                {
                    const auto &childLoadingState = child->getLoadingState();
                    if( childLoadingState == LoadingState::Loaded )
                    {
                        ScopedLock lock( graphicsSystem );

                        auto it = std::find( m_children.begin(), m_children.end(), child );
                        if( it != m_children.end() )
                        {
                            m_children.erase( it );

                            child->setParent( nullptr );

                            Ogre::SceneNode *sceneNode = nullptr;
                            child->_getObject( reinterpret_cast<void **>( &sceneNode ) );

                            if( sceneNode )
                            {
                                if( auto parent = sceneNode->getParent() )
                                {
                                    parent->removeChild( sceneNode );
                                }
                            }

                            return true;
                        }

                        return false;
                    }

                    if( !( childLoadingState == LoadingState::Unloading ||
                           childLoadingState == LoadingState::Unloaded ) )
                    {
                        auto message = factoryManager->make_ptr<StateMessageObject>();
                        message->setSender( this );
                        message->setType( STATE_MESSAGE_REMOVE_CHILD );
                        message->setObject( child );

                        auto stateContext = getStateContext();
                        stateContext->addMessage( stateTask, message );
                    }
                }
            }
            else
            {
                auto message = factoryManager->make_ptr<StateMessageObject>();
                message->setSender( this );
                message->setType( STATE_MESSAGE_REMOVE_CHILD );
                message->setObject( child );

                auto stateContext = getStateContext();
                if( stateContext )
                {
                    stateContext->addMessage( stateTask, message );
                }
            }

            return true;
        }

        return false;
    }

    void CSceneNodeOgreNext::needUpdate( bool forceParentUpdate )
    {
        //
        // m_sceneNode->needUpdate(forceParentUpdate);
    }

    auto CSceneNodeOgreNext::clone( SmartPtr<IGraphicsSceneNode> parent, const String &name ) const
        -> SmartPtr<IGraphicsSceneNode>
    {
        SmartPtr<CSceneNodeOgreNext> sceneNode = parent->addChildSceneNode();

        // set properties
        sceneNode->setPosition( getPosition() );
        sceneNode->setOrientation( getOrientation() );
        sceneNode->setScale( getScale() );

        // clone attached objects
        for( const auto &graphicsObject : m_graphicsObjects )
        {
            SmartPtr<IGraphicsObject> cloneGraphicsObject = graphicsObject->clone();
            sceneNode->attachObject( cloneGraphicsObject );
        }

        return sceneNode;
    }

    void CSceneNodeOgreNext::_getObject( void **ppObject ) const
    {
        *ppObject = m_sceneNode;
    }

    auto CSceneNodeOgreNext::getSceneNode() const -> Ogre::SceneNode *
    {
        return m_sceneNode;
    }

    void CSceneNodeOgreNext::updateBounds()
    {
    }

    auto CSceneNodeOgreNext::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = GraphicsSceneNode::getProperties();

        auto name = getName();

        properties->setProperty( "name", name );

        auto iNumAttachedObjects = getNumObjects();
        properties->setProperty( "NumObjects", iNumAttachedObjects );

        if( m_sceneNode )
        {
            auto sceneNodePosition = m_sceneNode->getPosition();
            auto sceneNodeScale = m_sceneNode->getScale();
            auto sceneNodeOrientation = OgreUtil::convert( m_sceneNode->getOrientation() );

            Vector3<real_Num> localRotation;
            //sceneNodeOrientation.toDegrees( localRotation );

            //sceneNodeOrientation.normalise().toDegrees( localRotation );

            Vector3<real_Num> localRotation2;
            //m_state->getOrientation().toDegrees( localRotation2 );

            properties->setProperty( "sceneNodePosition",
                                     Ogre::StringConverter::toString( sceneNodePosition ).c_str() );
            properties->setProperty( "sceneNodeScale",
                                     Ogre::StringConverter::toString( sceneNodeScale ).c_str() );
            properties->setProperty( "sceneNodeOrientation", localRotation );

            properties->setProperty( "stateOrientation", localRotation2 );
        }

        return properties;
    }

    void CSceneNodeOgreNext::setPosition( const Vector3F &position )
    {
        WP_ASSERT( position.isFinite() );

        GraphicsSceneNode::setPosition( position );

        if( auto sceneNode = getSceneNode() )
        {
            const auto pos = OgreUtil::convertToOgre( position );
            if( !OgreUtil::equals( pos, sceneNode->getPosition() ) )
            {
                sceneNode->setPosition( pos );
            }

            if( auto stateContext = getStateContext() )
            {
                if( auto stateData =
                        stateContext->invalidateStateDataById<TransformStateData>( getId(), false ) )
                {
                    auto derivedPosition = sceneNode->_getDerivedPositionUpdated();
                    stateData->worldTransform.setPosition( OgreUtil::convert( derivedPosition ) );
                }
            }
        }
    }

    auto CSceneNodeOgreNext::getPosition() const -> Vector3F
    {
        if( auto sceneNode = getSceneNode() )
        {
            return OgreUtil::convert( sceneNode->getPosition() );
        }

        return GraphicsSceneNode::getPosition();
    }

    auto CSceneNodeOgreNext::getWorldPosition() const -> Vector3F
    {
        if( auto parent = getParent() )
        {
            return parent->getWorldPosition() + getPosition();
        }

        if( auto sceneNode = getSceneNode() )
        {
            return OgreUtil::convert( sceneNode->_getDerivedPositionUpdated() );
        }

        return GraphicsSceneNode::getWorldPosition();
    }

    void CSceneNodeOgreNext::setOrientation( const QuaternionF &orientation )
    {
        WP_ASSERT( orientation.isSane() );

        GraphicsSceneNode::setOrientation( orientation );

        if( auto sceneNode = getSceneNode() )
        {
            auto orient = OgreUtil::convertToOgre( orientation );
            orient.normalise();
            if( !OgreUtil::equals( orient, sceneNode->getOrientation() ) )
            {
                sceneNode->setOrientation( orient );
            }

            if( auto stateContext = getStateContext() )
            {
                if( auto stateData =
                        stateContext->invalidateStateDataById<TransformStateData>( getId(), false ) )
                {
                    auto derivedOrientation = sceneNode->_getDerivedOrientationUpdated();
                    stateData->worldTransform.setOrientation( OgreUtil::convert( derivedOrientation ) );
                }
            }
        }
    }

    auto CSceneNodeOgreNext::getOrientation() const -> QuaternionF
    {
        if( auto sceneNode = getSceneNode() )
        {
            return OgreUtil::convert( sceneNode->getOrientation() );
        }

        return GraphicsSceneNode::getOrientation();
    }

    auto CSceneNodeOgreNext::getWorldOrientation() const -> QuaternionF
    {
        if( auto sceneNode = getSceneNode() )
        {
            return OgreUtil::convert( sceneNode->_getDerivedOrientationUpdated() );
        }

        return GraphicsSceneNode::getWorldOrientation();
    }

    void CSceneNodeOgreNext::setScale( const Vector3F &scale )
    {
        WP_ASSERT( scale.isFinite() );

        GraphicsSceneNode::setScale( scale );

        if( auto sceneNode = getSceneNode() )
        {
            auto ogreScale = OgreUtil::convertToOgre( scale );
            if( ogreScale.x == 0.0f || ogreScale.y == 0.0f || ogreScale.z == 0.0f )
            {
                ogreScale = Ogre::Vector3::UNIT_SCALE * 0.0001f;
            }

            if( !OgreUtil::equals( ogreScale, sceneNode->getScale() ) )
            {
                sceneNode->setScale( ogreScale );
            }

            if( auto stateContext = getStateContext() )
            {
                if( auto stateData =
                        stateContext->invalidateStateDataById<TransformStateData>( getId(), false ) )
                {
                    auto derivedScale = sceneNode->_getDerivedScaleUpdated();
                    stateData->worldTransform.setScale( OgreUtil::convert( derivedScale ) );
                }
            }
        }
    }

    auto CSceneNodeOgreNext::getScale() const -> Vector3F
    {
        if( auto sceneNode = getSceneNode() )
        {
            return OgreUtil::convert( sceneNode->getScale() );
        }

        return GraphicsSceneNode::getScale();
    }

    auto CSceneNodeOgreNext::getWorldScale() const -> Vector3F
    {
        if( auto sceneNode = getSceneNode() )
        {
            return OgreUtil::convert( sceneNode->_getDerivedScaleUpdated() );
        }

        return GraphicsSceneNode::getWorldScale();
    }

    void CSceneNodeOgreNext::setTransform( const Transform3<real_Num> &transform )
    {
        if( isThreadSafe() )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto transformState =
                        stateContext->invalidateStateDataById<TransformStateData>( getId(), false ) )
                {
                    transformState->localTransform = transform;

                    if( auto sceneNode = getSceneNode() )
                    {
                        const auto &position = transform.getPosition();
                        const auto &orientation = transform.getOrientation();
                        const auto &scale = transform.getScale();

                        auto pos = OgreUtil::convertToOgre( position );
                        if( !OgreUtil::equals( pos, sceneNode->getPosition() ) )
                        {
                            sceneNode->setPosition( pos );
                        }

                        auto orient = OgreUtil::convertToOgre( orientation );
                        if( !OgreUtil::equals( orient, sceneNode->getOrientation() ) )
                        {
                            orient.normalise();
                            sceneNode->setOrientation( orient );
                        }

                        auto vScale = OgreUtil::convertToOgre( scale );
                        if( !OgreUtil::equals( vScale, sceneNode->getScale() ) )
                        {
                            // check if scale is valid
                            if( vScale.x == 0.0f || vScale.y == 0.0f || vScale.z == 0.0f )
                            {
                                vScale = Ogre::Vector3::UNIT_SCALE * 0.0001f;
                            }

                            sceneNode->setScale( vScale );
                        }

                        transformState->derivedTransform = transform;

                        auto derivedPosition = sceneNode->_getDerivedPositionUpdated();
                        auto derivedScale = sceneNode->_getDerivedScaleUpdated();
                        auto derivedOrientation = sceneNode->_getDerivedOrientationUpdated();

                        auto &derivedTransform = transformState->worldTransform;
                        derivedTransform.setPosition( OgreUtil::convert( derivedPosition ) );
                        derivedTransform.setOrientation( OgreUtil::convert( derivedOrientation ) );
                        derivedTransform.setScale( OgreUtil::convert( derivedScale ) );
                    }
                }
            }
        }
        else
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData =
                        stateContext->invalidateStateDataById<TransformStateData>( getId(), true ) )
                {
                    stateData->localTransform = transform;
                }
            }
        }
    }

    void CSceneNodeOgreNext::setWorldTransform( const Transform3<real_Num> &transform )
    {
        auto task = Thread::getCurrentTask();
        if( task == TaskId::Render )
        {
            auto sceneNode = getSceneNode();

            const auto &position = transform.getPosition();
            const auto &orientation = transform.getOrientation();
            const auto &scale = transform.getScale();

            if( auto stateContext = getStateContext() )
            {
                if( auto stateData =
                        stateContext->invalidateStateDataById<TransformStateData>( getId(), false ) )
                {
                    auto &t = stateData->localTransform;
                    t.setPosition( position );
                    t.setOrientation( orientation );
                    t.setScale( scale );
                }
            }

            auto pos = OgreUtil::convertToOgre( position );
            if( !OgreUtil::equals( pos, sceneNode->getPosition() ) )
            {
                sceneNode->setPosition( pos );
            }

            auto orient = OgreUtil::convertToOgre( orientation );
            if( !OgreUtil::equals( orient, sceneNode->getOrientation() ) )
            {
                orient.normalise();
                sceneNode->setOrientation( orient );
            }

            auto vScale = OgreUtil::convertToOgre( scale );
            if( !OgreUtil::equals( vScale, sceneNode->getScale() ) )
            {
                // check if scale is valid
                if( vScale.x == 0.0f || vScale.y == 0.0f || vScale.z == 0.0f )
                {
                    vScale = Ogre::Vector3::UNIT_SCALE * 0.0001f;
                }

                sceneNode->setScale( vScale );
            }
        }
        else
        {
            const auto &position = transform.getPosition();
            const auto &orientation = transform.getOrientation();
            const auto &scale = transform.getScale();

            if( auto stateContext = getStateContext() )
            {
                if( auto stateData =
                        stateContext->invalidateStateDataById<TransformStateData>( getId(), true ) )
                {
                    auto &t = stateData->localTransform;
                    t.setPosition( position );
                    t.setOrientation( orientation );
                    t.setScale( scale );
                }
            }
        }
    }

    bool CSceneNodeOgreNext::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( message && message->getSender() == this )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto factoryManager = applicationManager->getFactoryManager();
            auto renderTask = graphicsSystem->getRenderTask();
            auto currentTaskId = Thread::getCurrentTask();

            if( message->isExactly<StateMessageVector3>() )
            {
                auto positionMessage = workphone::static_pointer_cast<StateMessageVector3>( message );
                auto value = positionMessage->getValue();
                auto type = message->getType();

                if( type == STATE_MESSAGE_POSITION )
                {
                    setPosition( value );
                    return true;
                }
                else if( type == STATE_MESSAGE_SCALE )
                {
                    setScale( value );
                    return true;
                }
                else if( type == STATE_MESSAGE_LOOK_AT )
                {
                    lookAt( value );
                    return true;
                }
            }
            else if( message->isExactly<StateMessageOrientation>() )
            {
                auto orientationMessage =
                    workphone::static_pointer_cast<StateMessageOrientation>( message );
                setOrientation( orientationMessage->getOrientation() );
                return true;
            }
            else if( message->isExactly<StateMessageObject>() )
            {
                auto objectMessage = workphone::static_pointer_cast<StateMessageObject>( message );
                auto object = objectMessage->getObject();
                auto type = objectMessage->getType();
                WP_ASSERT( objectMessage );

                if( type == STATE_MESSAGE_ADD_CHILD )
                {
                    addChild( object );
                    return true;
                }
                else if( type == STATE_MESSAGE_REMOVE_CHILD )
                {
                    removeChild( object );
                    return true;
                }
                else if( type == STATE_MESSAGE_ATTACH_OBJECT )
                {
                    attachObject( object );
                    return true;
                }
                else if( type == STATE_MESSAGE_DETACH_OBJECT )
                {
                    detachObject( object );
                    return true;
                }
                else if( type == STATE_MESSAGE_DETACH_ALL_OBJECTS )
                {
                    detachAllObjects();
                    return true;
                }
            }
            else if( message->isExactly<StateMessageType>() )
            {
                auto stateMessage = workphone::static_pointer_cast<StateMessageType>( message );
                if( stateMessage->getType() == STATE_MESSAGE_REMOVE )
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool CSceneNodeOgreNext::handleStateChanged( SmartPtr<IState> &state )
    {
        if( state && state->getOwnerPtr() == this )
        {
            if( isLoaded() )
            {
                auto result = GraphicsSceneNode::handleStateChanged( state );

                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
                WP_ASSERT( graphicsSystem );

                Ogre::SceneNode *sceneNode = nullptr;
                _getObject( reinterpret_cast<void **>( &sceneNode ) );

                if( sceneNode )
                {
                    if( auto stateData = state->getData() )
                    {
                        if( stateData->isDerived<TransformStateData>() )
                        {
                            auto transformState = SafeReadPtr<TransformStateData>( stateData );

                            const auto &transform = transformState->localTransform;
                            const auto &position = transform.getPosition();
                            const auto &orientation = transform.getOrientation();
                            const auto &scale = transform.getScale();

                            auto pos = OgreUtil::convertToOgre( position );
                            if( !OgreUtil::equals( pos, sceneNode->getPosition() ) )
                            {
                                sceneNode->setPosition( pos );
                            }

                            auto orient = OgreUtil::convertToOgre( orientation );
                            if( !OgreUtil::equals( orient, sceneNode->getOrientation() ) )
                            {
                                orient.normalise();
                                sceneNode->setOrientation( orient );
                            }

                            auto vScale = OgreUtil::convertToOgre( scale );
                            if( !OgreUtil::equals( vScale, sceneNode->getScale() ) )
                            {
                                // check if scale is valid
                                if( vScale.x == 0.0f || vScale.y == 0.0f || vScale.z == 0.0f )
                                {
                                    vScale = Ogre::Vector3::UNIT_SCALE * 0.0001f;
                                }

                                sceneNode->setScale( vScale );
                            }

                            transformState->derivedTransform = transform;

                            auto derivedPosition = sceneNode->_getDerivedPositionUpdated();
                            auto derivedScale = sceneNode->_getDerivedScaleUpdated();
                            auto derivedOrientation = sceneNode->_getDerivedOrientationUpdated();

                            auto &derivedTransform = transformState->worldTransform;

                            auto derivedPos = OgreUtil::convert( derivedPosition );
                            auto derivedOri = OgreUtil::convert( derivedOrientation );
                            auto derivedScl = OgreUtil::convert( derivedScale );

                            derivedTransform.setPosition( derivedPos );
                            derivedTransform.setOrientation( derivedOri );
                            derivedTransform.setScale( derivedScl );

                            auto localAABB = calculateAABB();
                            setLocalAABB( localAABB );

                            Matrix4<real_Num> transformMat;
                            transformMat.makeTransform( derivedPos, derivedScl, derivedOri );

                            auto worldAABB = localAABB.transform( transformMat );
                            setWorldAABB( worldAABB );

                            result = true;
                        }
                        else if( stateData->isDerived<SceneNodeStateData>() )
                        {
                            auto sceneNodeState = SafeReadPtr<SceneNodeStateData>( stateData );
                            result = true;
                        }

                        for( auto &graphicsObject : m_graphicsObjects )
                        {
                            if( graphicsObject->isDerived<CLightOgreNext>() )
                            {
                                auto light =
                                    workphone::static_pointer_cast<CLightOgreNext>( graphicsObject );
                                auto stateObject = light->getStateContextPtr();
                                if( stateObject )
                                {
                                    stateObject->setDirty( true );
                                }
                            }
                            else if( graphicsObject->isDerived<CCameraOgreNext>() )
                            {
                                auto camera =
                                    workphone::static_pointer_cast<CCameraOgreNext>( graphicsObject );
                                auto stateObject = camera->getStateContextPtr();
                                if( stateObject )
                                {
                                    stateObject->setDirty( true );
                                }
                            }
                        }

                        return result;
                    }
                }
            }
        }

        return false;
    }

    auto CSceneNodeOgreNext::calculateAABB() const -> AABB3F
    {
        Ogre::Aabb box;

        for( u32 i = 0; i < m_sceneNode->numAttachedObjects(); ++i )
        {
            Ogre::MovableObject *object = m_sceneNode->getAttachedObject( i );
            auto aabb = object->getLocalAabb();

            box.merge( aabb );
        }

        const Ogre::Vector3 &minPoint = box.getMinimum();
        const Ogre::Vector3 &maxPoint = box.getMaximum();

        return { minPoint.x, minPoint.y, minPoint.z, maxPoint.x, maxPoint.y, maxPoint.z };
    }

    auto CSceneNodeOgreNext::_getRenderSystemTransform() const -> void *
    {
        return nullptr;
    }

    void CSceneNodeOgreNext::setupStateContext()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManagerPtr();
        WP_ASSERT( stateManager );

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        WP_ASSERT( graphicsSystem );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto scene = getCreatorPtr();

        auto stateContext = scene->getSceneNodeContextPtr();
        WP_ASSERT( stateContext );
        setStateContext( stateContext );

        auto transformState = factoryManager->make_ptr<State>();
        transformState->setId( getId() );
        transformState->setOwner( this );
        stateContext->addState( transformState );

        auto transformData = factoryManager->make_ptr<TransformStateData>();
        transformState->setData( transformData );

        auto sceneNodeState = factoryManager->make_ptr<State>();
        sceneNodeState->setId( getId() );
        sceneNodeState->setOwner( this );
        stateContext->addState( sceneNodeState );

        auto sceneNodeData = factoryManager->make_ptr<SceneNodeStateData>();
        sceneNodeState->setData( sceneNodeData );

        auto boundingBoxState = factoryManager->make_ptr<State>();
        boundingBoxState->setId( getId() );
        boundingBoxState->setOwner( this );
        stateContext->addState( boundingBoxState );

        auto boundingBoxData = factoryManager->make_ptr<BoundingBoxStateData>();
        boundingBoxState->setData( boundingBoxData );
    }

    CSceneNodeOgreNext::NodeListener::NodeListener( CSceneNodeOgreNext *owner ) : m_owner( owner )
    {
    }

    CSceneNodeOgreNext::NodeListener::~NodeListener() = default;

    void CSceneNodeOgreNext::NodeListener::nodeUpdated( const Ogre::Node *node )
    {
    }

    void CSceneNodeOgreNext::NodeListener::nodeDestroyed( const Ogre::Node *node )
    {
    }

    void CSceneNodeOgreNext::NodeListener::nodeAttached( const Ogre::Node *node )
    {
        Ogre::SceneManager *pSceneManager = nullptr;

        SmartPtr<IGraphicsScene> smgr = m_owner->getCreator();
        smgr->_getObject( reinterpret_cast<void **>( &pSceneManager ) );

        auto sceneNode = (Ogre::SceneNode *)node;
        if( pSceneManager->getRootSceneNode() == sceneNode->getParentSceneNode() )
        {
            // m_owner->registerForUpdates(true, false);
        }
    }

    void CSceneNodeOgreNext::NodeListener::nodeDetached( const Ogre::Node *node )
    {
        Ogre::SceneManager *pSceneManager = nullptr;

        SmartPtr<IGraphicsScene> smgr = m_owner->getCreator();
        smgr->_getObject( reinterpret_cast<void **>( &pSceneManager ) );

        auto sceneNode = (Ogre::SceneNode *)node;
        if( pSceneManager->getRootSceneNode() == sceneNode->getParentSceneNode() )
        {
            // m_owner->registerForUpdates(false, false);
        }
    }

}  // namespace workphone::render
