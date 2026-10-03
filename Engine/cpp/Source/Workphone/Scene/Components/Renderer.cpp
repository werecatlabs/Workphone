#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Renderer.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Scene/Components/Material.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IMaterialManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITaskLock.hpp>
#include <Workphone/Interface/System/IThreadPool.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/State/Messages/StateMessageLoad.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Renderer, Component );

    u32 Renderer::m_idExt = 0;

    const String Renderer::castShadowsStr = "castShadows";
    const String Renderer::recieveShadowsStr = "recieveShadows";
    const String Renderer::reflectionsStr = "reflections";
    const String Renderer::occulsionStr = "occulsion";
    const String Renderer::materialUpdateStr = "Update Materials";
    const String Renderer::updateBoundsStr = "Update Bounds";
    const String Renderer::boundingBoxStr = "boundingBox";
    const String Renderer::zOrderStr = "zOrder";
    const String Renderer::visibilityFlagsStr = "visibilityFlags";
    const String Renderer::materialNameStr = "materialName";

    const Array<String> Renderer::castShadowStateNames = { "Off", "On", "DoubleSided", "ShadowsOnly" };
    const Array<String> Renderer::recieveShadowsNames = { "Off", "On" };
    const Array<String> Renderer::reflectionsStateNames = { "Off", "Default", "Blend", "Simple" };
    const Array<String> Renderer::occulsionStateNames = { "Off", "On", "Dynamic", "Static" };

    Renderer::Renderer()
    {
        setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );
    }

    Renderer::~Renderer() = default;

    void Renderer::updateFlags( u32 flags, u32 oldFlags )
    {
        if( auto actor = getActorPtr() )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( graphicsSystem )
            {
                auto smgr = graphicsSystem->getGraphicsScenePtr();
                WP_ASSERT( smgr );

                auto rootNode = smgr->getRootSceneNode();

                if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagInScene ) !=
                    BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagInScene ) )
                {
                    auto graphicsObject = getGraphicsObject();
                    if( !graphicsObject )
                    {
                        updateMesh();
                        updateMaterials();
                        updateTransform();
                    }

                    updateVisibility();
                }
                else if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagEnabled ) !=
                         BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagEnabled ) )
                {
                    updateVisibility();
                }
                else if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagStatic ) !=
                         BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagStatic ) )
                {
                    updateStatic();
                }
            }
        }

        Component::updateFlags( flags, oldFlags );
    }

    void Renderer::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( getLoadingState() == LoadingState::Loaded )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );

            Component::load( data );
            updateStatic();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Renderer::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto loadingState = getLoadingState();

            if( loadingState == LoadingState::Allocated )
            {
                return;
            }

            if( loadingState == LoadingState::Unloaded )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( graphicsSystem )
            {
                auto smgr = graphicsSystem->getGraphicsScenePtr();
                if( !smgr )
                {
                    setGraphicsObject( nullptr );
                    setGraphicsNode( nullptr );
                    Component::unload( data );
                    setLoadingState( LoadingState::Unloaded );
                    return;
                }

                auto graphicsNode = getGraphicsNode();
                if( graphicsNode )
                {
                    auto graphicsObject = getGraphicsObject();
                    if( graphicsObject )
                    {
                        graphicsNode->detachObject( graphicsObject );
                        smgr->removeGraphicsObject( graphicsObject );
                        setGraphicsObject( nullptr );
                    }

                    graphicsNode->detachAllObjects();
                    smgr->removeSceneNode( graphicsNode );
                    setGraphicsNode( nullptr );
                }
                else
                {
                    setGraphicsObject( nullptr );
                    setGraphicsNode( nullptr );
                }
            }

            Component::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    Array<SmartPtr<ISharedObject>> Renderer::getChildObjects() const
    {
        auto graphicsObject = getGraphicsObject();
        auto graphicsNode = getGraphicsNode();
        auto material = getSharedMaterial();

        auto objects = Component::getChildObjects();
        objects.reserve( 4 );

        if( graphicsObject )
        {
            objects.emplace_back( graphicsObject );
        }
        if( graphicsNode )
        {
            objects.emplace_back( graphicsNode );
        }
        if( material )
        {
            objects.emplace_back( material );
        }

        return objects;
    }

    SmartPtr<Properties> Renderer::getProperties() const
    {
        if( auto properties = Component::getProperties() )
        {
            properties->setPropertyAsEnum( castShadowsStr, static_cast<s32>( m_castShadows.load() ),
                                           castShadowStateNames );

            properties->setPropertyAsEnum(
                recieveShadowsStr, static_cast<s32>( m_recieveShadows.load() ), recieveShadowsNames );
            properties->setPropertyAsEnum( reflectionsStr, static_cast<s32>( m_reflections.load() ),
                                           reflectionsStateNames );
            properties->setPropertyAsEnum( occulsionStr, static_cast<s32>( m_occulsion.load() ),
                                           occulsionStateNames );

            properties->setProperty( zOrderStr, m_zOrder.load() );
            properties->setProperty( visibilityFlagsStr, m_visibilityFlags.load() );
            properties->setProperty( materialNameStr, m_materialName.load() );

            properties->setButtonPressed( materialUpdateStr );
            properties->setButtonPressed( updateBoundsStr );
            properties->setProperty( boundingBoxStr, getBoundingBox() );

            return properties;
        }

        return nullptr;
    }

    void Renderer::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        Component::setProperties( properties );

        s32 castShadows = static_cast<s32>( m_castShadows.load() );
        if( properties->getPropertyValue( castShadowsStr, castShadows ) )
        {
            setCastShadows( static_cast<CastShadows>( castShadows ) );
        }

        s32 recieveShadows = static_cast<s32>( m_recieveShadows.load() );
        if( properties->getPropertyValue( recieveShadowsStr, recieveShadows ) )
        {
            setRecieveShadows( static_cast<RecieveShadows>( recieveShadows ) );
        }

        s32 reflections = static_cast<s32>( m_reflections.load() );
        if( properties->getPropertyValue( reflectionsStr, reflections ) )
        {
            setReflections( static_cast<Reflections>( reflections ) );
        }

        s32 occulsion = static_cast<s32>( m_occulsion.load() );
        if( properties->getPropertyValue( occulsionStr, occulsion ) )
        {
            setOcculsion( static_cast<Occulsion>( occulsion ) );
        }

        u32 zOrder = m_zOrder.load();
        if( properties->getPropertyValue( zOrderStr, zOrder ) )
        {
            setZOrder( zOrder );
        }

        u32 visibilityFlags = m_visibilityFlags.load();
        if( properties->getPropertyValue( visibilityFlagsStr, visibilityFlags ) )
        {
            setVisibilityFlags( visibilityFlags );
        }

        String materialName = m_materialName.load();
        if( properties->getPropertyValue( materialNameStr, materialName ) )
        {
            setMaterialName( materialName );
        }

        if( properties->isButtonPressed( materialUpdateStr ) )
        {
            if( auto actor = getActorPtr() )
            {
                auto components = actor->getComponents();
                for( auto component : components )
                {
                    component->updateMaterials();
                }
            }
        }

        if( properties->isButtonPressed( updateBoundsStr ) )
        {
            m_boundingBox = calculateBoundingBox();
        }
    }

    SmartPtr<render::IMaterial> Renderer::getSharedMaterial() const
    {
        return m_sharedMaterial;
    }

    void Renderer::setSharedMaterial( SmartPtr<render::IMaterial> sharedMaterial )
    {
        m_sharedMaterial = sharedMaterial;
    }

    String Renderer::getMaterialName() const
    {
        return m_materialName;
    }

    void Renderer::setMaterialName( const String &materialName )
    {
        m_materialName = materialName;
    }

    FSMReturnType Renderer::handleComponentEvent( u32 state, FSMEvent eventType )
    {
        Component::handleComponentEvent( state, eventType );

        switch( eventType )
        {
        case FSMEvent::Change:
        {
        }
        break;
        case FSMEvent::Enter:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Destroyed:
            {
            }
            break;
            case State::Edit:
            case State::Play:
            {
                if( !getGraphicsObject() )
                {
                    updateMesh();
                }

                updateMaterials();
                updateTransform();

                m_boundingBox = calculateBoundingBox();

                applyGraphicsState();
            }
            break;
            default:
            {
            }
            }
        }
        break;
        case FSMEvent::Leave:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Edit:
            case State::Play:
            {
                if( auto graphicsNode = getGraphicsNode() )
                {
                    if( auto parent = graphicsNode->getParent() )
                    {
                        parent->removeChild( graphicsNode );
                    }

                    if( m_graphicsObject )
                    {
                        m_graphicsObject->setVisible( false );
                    }
                }
            }
            break;
            default:
            {
            }
            }
        }
        break;
        case FSMEvent::Pending:
        {
        }
        break;
        case FSMEvent::Complete:
        {
        }
        break;
        case FSMEvent::NewState:
        {
        }
        break;
        case FSMEvent::WaitForChange:
        {
        }
        break;
        default:
        {
        }
        break;
        }

        return FSMReturnType::Ok;
    }

    void Renderer::updateMesh()
    {
    }

    void Renderer::updateMaterials()
    {
    }

    void Renderer::updateVisibility()
    {
        if( auto actor = getActorPtr() )
        {
            if( auto graphicsObject = getGraphicsObject() )
            {
                auto enabled = isEnabled() && actor->isEnabledInScene() && isLODVisible();
                graphicsObject->setVisible( enabled );
            }
        }
    }

    void Renderer::setLODVisible( bool visible )
    {
        if( m_lodVisible.load() == visible )
        {
            return;
        }

        m_lodVisible = visible;
        updateVisibility();
    }

    bool Renderer::isLODVisible() const
    {
        return m_lodVisible.load();
    }

    Parameter Renderer::handleEvent( EventType eventType, hash_type eventValue,
                                     const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                     SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( Thread::getTaskFlag( Thread::Application_Flag ) )
        {
            if( eventValue == IEvent::loadingStateChanged )
            {
                if( auto graphicsObject = getGraphicsObject() )
                {
                    if( sender == graphicsObject || object == graphicsObject )
                    {
                        auto loadingState = static_cast<LoadingState>( arguments[1].getU32() );
                        if( loadingState == LoadingState::Loaded )
                        {
                            updateMaterials();

                            if( auto actor = getActorPtr() )
                            {
                                if( auto root = actor->getSceneRoot() )
                                {
                                    root->updateTransform();
                                }
                            }
                        }
                    }
                }
            }
            else if( eventValue == visibilityChanged )
            {
                if( auto actor = getActorPtr() )
                {
                    if( auto graphicsObject = getGraphicsObject() )
                    {
                        auto enabled = isEnabled() && actor->isEnabledInScene() && isLODVisible();
                        graphicsObject->setVisible( enabled );
                    }
                }
            }
            else if( eventValue == staticChanged )
            {
                updateStatic();
            }
        }

        return Component::handleEvent( eventType, eventValue, arguments, sender, object, event );
    }

    void Renderer::updateTransform()
    {
        auto state = getState();
        switch( state )
        {
        case State::Edit:
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto taskManager = applicationManager->getTaskManagerPtr();
            auto threadPool = applicationManager->getThreadPoolPtr();

            if( threadPool && threadPool->getNumThreads() > 0 && taskManager &&
                taskManager->getNumTasks() > 0 )
            {
                if( auto actor = getActorPtr() )
                {
                    if( auto actorTransform = actor->getTransformPtr() )
                    {
                        auto t = actorTransform->getWorldTransform();

                        if( auto node = getGraphicsNode() )
                        {
                            node->setTransform( t );
                        }
                    }
                }
            }
            else
            {
                if( auto actor = getActorPtr() )
                {
                    if( auto actorTransform = actor->getTransformPtr() )
                    {
                        auto t = actorTransform->getWorldTransform();
                        if( auto node = getGraphicsNode() )
                        {
                            node->setTransform( t );
                        }
                    }
                }
            }
        }
        break;
        case State::Play:
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto taskManager = applicationManager->getTaskManagerPtr();
            auto threadPool = applicationManager->getThreadPoolPtr();

            if( threadPool && threadPool->getNumThreads() > 0 && taskManager &&
                taskManager->getNumTasks() > 0 )
            {
                if( auto actor = getActorPtr() )
                {
                    if( !actor->isSmoothMotion() )
                    {
                        if( auto actorTransform = actor->getTransformPtr() )
                        {
                            auto t = actorTransform->getWorldTransform();

                            if( auto node = getGraphicsNode() )
                            {
                                node->setTransform( t );
                            }
                        }
                    }
                }
            }
            else
            {
                if( auto actor = getActorPtr() )
                {
                    if( auto actorTransform = actor->getTransformPtr() )
                    {
                        auto t = actorTransform->getWorldTransform();
                        if( auto node = getGraphicsNode() )
                        {
                            node->setTransform( t );
                        }
                    }
                }
            }
        }
        break;
        default:
        {
        }
        break;
        }
    }

    SmartPtr<render::IGraphicsObject> Renderer::getGraphicsObject() const
    {
        return m_graphicsObject;
    }

    void Renderer::setGraphicsObject( SmartPtr<render::IGraphicsObject> graphicsObject )
    {
        m_graphicsObject = graphicsObject;
    }

    void Renderer::setGraphicsNode( SmartPtr<render::IGraphicsSceneNode> graphicshNode )
    {
        m_graphicsNode = graphicshNode;
    }

    AABB3<real_Num> Renderer::getBoundingBox() const
    {
        return m_boundingBox;
    }

    void Renderer::updateTransform( const Transform3<real_Num> &transform )
    {
        if( auto graphicsNode = getGraphicsNode() )
        {
            graphicsNode->setTransform( transform );
        }
    }

    void Renderer::updateStatic()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto sceneManager = applicationManager->getGameManagerPtr();

        if( auto actor = getActorPtr() )
        {
            auto isstatic = actor->isStatic();
            if( auto graphicsNode = getGraphicsNode() )
            {
                graphicsNode->setStatic( isstatic );
            }

            if( !isstatic )
            {
                sceneManager->registerComponentUpdate( TaskId::Render, Thread::UpdateState::Transform,
                                                       this );
            }
            else
            {
                sceneManager->unregisterComponentUpdate( TaskId::Render, Thread::UpdateState::Transform,
                                                         this );
            }
        }
    }

    AABB3<real_Num> Renderer::calculateBoundingBox()
    {
        return m_boundingBox;
    }

    void Renderer::applyGraphicsState()
    {
        if( auto graphicsObject = getGraphicsObject() )
        {
            graphicsObject->setCastShadows( m_castShadows != CastShadows::Off );
            graphicsObject->setReceiveShadows( m_recieveShadows == RecieveShadows::On );
            graphicsObject->setZOrder( m_zOrder.load() );
            graphicsObject->setVisibilityFlags( m_visibilityFlags.load() );
        }
    }

    Renderer::CastShadows Renderer::getCastShadows() const
    {
        return m_castShadows.load();
    }

    void Renderer::setCastShadows( CastShadows castShadows )
    {
        m_castShadows = castShadows;
        applyGraphicsState();
    }

    Renderer::RecieveShadows Renderer::getRecieveShadows() const
    {
        return m_recieveShadows.load();
    }

    void Renderer::setRecieveShadows( RecieveShadows recieveShadows )
    {
        m_recieveShadows = recieveShadows;
        applyGraphicsState();
    }

    Renderer::Reflections Renderer::getReflections() const
    {
        return m_reflections.load();
    }

    void Renderer::setReflections( Reflections reflections )
    {
        m_reflections = reflections;
    }

    Renderer::Occulsion Renderer::getOcculsion() const
    {
        return m_occulsion.load();
    }

    void Renderer::setOcculsion( Occulsion occulsion )
    {
        m_occulsion = occulsion;
    }

    u32 Renderer::getZOrder() const
    {
        return m_zOrder.load();
    }

    void Renderer::setZOrder( u32 zOrder )
    {
        m_zOrder = zOrder;
        if( auto graphicsObject = getGraphicsObject() )
            graphicsObject->setZOrder( zOrder );
    }

    u32 Renderer::getVisibilityFlags() const
    {
        return m_visibilityFlags.load();
    }

    void Renderer::setVisibilityFlags( u32 flags )
    {
        m_visibilityFlags = flags;
        if( auto graphicsObject = getGraphicsObject() )
            graphicsObject->setVisibilityFlags( flags );
    }

}  // namespace workphone::scene
