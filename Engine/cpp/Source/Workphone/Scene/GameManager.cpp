#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Scene/GameScene.hpp>
#include <Workphone/System/FSMManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/Scene/IComponentEvent.hpp>
#include <Workphone/Interface/Scene/IComponentEventListener.hpp>
#include <Workphone/Interface/Scene/IComponentSystem.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITaskLock.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/IThreadPool.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Util.hpp>
#include <Workphone/Jobs/ActorLoadJob.hpp>
#include <Workphone/Scene/Components/Camera.hpp>
#include <Workphone/Scene/Components/CollisionBox.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Scene/Transform.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Scene/Components/Script.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Thread/TryLockGuard.hpp>
#include <crtdbg.h>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, GameManager, workphone::scene::IGameManager );
    WP_CLASS_REGISTER_DERIVED( workphone::scene, GameManager::EventListener, IEventListener );

    constexpr auto size = 32768;
    constexpr auto maxSmoothSize = 128;

    GameManager::GameManager()
    {
        setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );
        setEventTaskFlags( Thread::Application_Flag );
    }

    GameManager::~GameManager()
    {
        std::fprintf( stderr, "TRACE GameManager destructor body start\n" );
        m_fsmManager = nullptr;
        m_componentFsmManagers.clear();
        m_eventListener = nullptr;
        m_scene = nullptr;
        std::fprintf( stderr, "TRACE GameManager destructor body end\n" );
    }

    void GameManager::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();
            WP_ASSERT( stateManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto fsmManager = factoryManager->make_ptr<FSMManager>();
            fsmManager->setGrowSize( 1024 );
            fsmManager->load( nullptr );
            setFsmManager( fsmManager );

            m_actorLoadingStates.resize( size );
            m_fsms.resize( size );
            m_gameFSMs.resize( size );

            m_fsmListeners.resize( size );
            m_gameFsmListeners.resize( size );

            m_scenes.resize( size );

            m_transforms.reserve( size );

            auto actorDefaultFlags = IGameActor::ActorFlagVisible | IGameActor::ActorFlagEnabled;

            m_actors.resize( size );

            m_updateComponents.resize( static_cast<s32>( Thread::UpdateState::Count ) );
            for( auto &v : m_updateComponents )
            {
                v.resize( static_cast<s32>( TaskId::Count ) );
            }

            auto eventListener = factoryManager->make_ptr<EventListener>();
            eventListener->setOwner( this );
            applicationManager->addObjectListener( eventListener );
            m_eventListener = eventListener;

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GameManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
#if defined( _DEBUG )
            if( !_CrtCheckMemory() )
            {
                std::fprintf( stderr, "CRT heap damaged before GameManager::unload\n" );
            }
#endif
            ScopedLoadstateWait loadstateWait( this );
            ScopedLock lock( this );

            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto factoryManager = applicationManager->getFactoryManagerPtr();
                WP_ASSERT( factoryManager );

                destroyActors();

#if defined( _DEBUG )
                if( !_CrtCheckMemory() )
                {
                    std::fprintf( stderr, "CRT heap damaged after GameManager::destroyActors\n" );
                }
#endif

                m_actors.clear();
                m_gameFSMs.clear();
                m_fsmListeners.clear();
                m_gameFsmListeners.clear();
                m_actorLoadJobs.clear();
                m_actorLoadingStates.clear();

                m_loadingActors.clear();
                m_loadingComponents.clear();
                m_unloadingActors.clear();
                m_unloadingComponents.clear();

                m_updateObjects.clear();
                m_updateComponents.clear();

                m_queueProperties.clear();
                m_dirtyActors.clear();
                m_dirtyComponents.clear();
                m_dirtyComponentTransforms.clear();
                m_dirtyTransforms.clear();
                m_dirtyActorTransforms.clear();

                if( m_scene )
                {
                    m_scene->unload( nullptr );
                    m_scene = nullptr;
                }

                for( auto &[id, system] : m_systems )
                {
                    if( system )
                    {
                        system->unload( nullptr );
                    }
                }

                m_systems.clear();

                m_components.clear();

                for( auto t : m_transforms )
                {
                    if( t )
                    {
                        t->unload( nullptr );
                    }
                }

                m_transforms.clear();

                for( auto t : m_smoothTransforms )
                {
                    if( t )
                    {
                        t->unload( nullptr );
                    }
                }

                m_smoothTransforms.clear();

                m_fsms.clear();

                if( auto fsmManager = getFsmManager() )
                {
                    fsmManager->unload( nullptr );
                    setFsmManager( nullptr );
                }

                for( auto &[id, fsmManager] : m_componentFsmManagers )
                {
                    if( fsmManager )
                    {
                        fsmManager->unload( nullptr );
                    }
                }

                m_componentFsmManagers.clear();

                if( m_eventListener )
                {
                    applicationManager->removeObjectListener( m_eventListener );
                    m_eventListener = nullptr;
                }

#if defined( _DEBUG )
                if( !_CrtCheckMemory() )
                {
                    std::fprintf( stderr, "CRT heap damaged at end of GameManager::unload\n" );
                }
#endif
                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GameManager::preUpdate()
    {
        try
        {
            if( isLoaded() )
            {
                ScopedLoadLock loadLock( this );

                auto scene = getCurrentScenePtr();
                if( scene )
                {
                    auto applicationManager = core::IApplicationManager::instancePtr();

                    auto task = Thread::getCurrentTask();

                    auto timer = applicationManager->getTimerPtr();

                    auto smoothDeltaTime = timer->getSmoothDeltaTime();
                    auto previousTime = timer->getPreviousTime( task );
                    auto transformTime = previousTime + smoothDeltaTime + ( 1.0 / 30.0 );

                    auto sceneLoadingState = scene->getSceneLoadingState();
                    if( sceneLoadingState == IGameScene::SceneLoadingState::Loaded )
                    {
                        auto sceneTask = getStateTask();

                        if( task == sceneTask )
                        {
                            try
                            {
                                for( auto &t : m_transforms )
                                {
                                    if( t )
                                    {
                                        t->update();
                                    }
                                }

                                if( !m_loadingActors.empty() )
                                {
                                    SmartPtr<IGameActor> actor;
                                    Pair<SmartPtr<IGameActor>, SmartPtr<Properties>> actorPair;
                                    while( m_loadingActors.try_pop( actorPair ) )
                                    {
                                        auto &actor = actorPair.first;

                                        if( auto &data = actorPair.second )
                                        {
                                            actor->load( data );
                                            actor->updateTransform();

                                            Parameter dataParam;
                                            dataParam.object = data;

                                            auto args = Array<Parameter>( { dataParam } );

                                            applicationManager->triggerEvent(
                                                EventType::Scene, IEvent::actorLoaded, args, this, actor,
                                                nullptr, false, Thread::Application_Flag );
                                        }
                                        else
                                        {
                                            auto args = Array<Parameter>();
                                            applicationManager->triggerEvent(
                                                EventType::Scene, IEvent::actorLoaded, args, this, actor,
                                                nullptr, false, Thread::Application_Flag );
                                        }
                                    }
                                }

                                if( !m_loadingComponents.empty() )
                                {
                                    Pair<SmartPtr<IComponent>, SmartPtr<Properties>> componentPair;
                                    while( m_loadingComponents.try_pop( componentPair ) )
                                    {
                                        auto &component = componentPair.first;

                                        if( auto &data = componentPair.second )
                                        {
                                            component->load( data );

                                            Parameter dataParam;
                                            dataParam.object = data;

                                            auto args = Array<Parameter>( { dataParam } );

                                            applicationManager->triggerEvent(
                                                EventType::Scene, IEvent::componentLoaded, args, this,
                                                component, nullptr, false, Thread::Application_Flag );
                                        }
                                        else
                                        {
                                            auto args = Array<Parameter>();
                                            applicationManager->triggerEvent(
                                                EventType::Scene, IEvent::componentLoaded, args, this,
                                                component, nullptr, false, Thread::Application_Flag );
                                        }

                                        auto actor = component->getActorPtr();
                                        if( actor )
                                        {
                                            actor->componentLoaded( component );
                                        }
                                    }
                                }

                                if( !m_unloadingActors.empty() )
                                {
                                    SmartPtr<IGameActor> actor;
                                    while( m_unloadingActors.try_pop( actor ) )
                                    {
                                        actor->unload( nullptr );
                                    }
                                }

                                if( !m_unloadingComponents.empty() )
                                {
                                    SmartPtr<IComponent> component;
                                    while( m_unloadingComponents.try_pop( component ) )
                                    {
                                        component->unload( nullptr );
                                    }
                                }

                                if( !m_queueProperties.empty() )
                                {
                                    Pair<ISharedObject *, SmartPtr<Properties>> p;
                                    while( m_queueProperties.try_pop( p ) )
                                    {
                                        try
                                        {
                                            auto &object = p.first;
                                            if( object->isDerived<IComponent>() )
                                            {
                                                auto component =
                                                    workphone::static_pointer_cast<IComponent>(
                                                        SmartPtr<ISharedObject>( object ) );
                                                component->setProperties( p.second );
                                            }
                                            else if( object->isDerived<IComponentEventListener>() )
                                            {
                                                auto component = workphone::static_pointer_cast<
                                                    IComponentEventListener>(
                                                    SmartPtr<ISharedObject>( object ) );
                                                component->setProperties( p.second );
                                            }
                                        }
                                        catch( std::exception &e )
                                        {
                                            WP_LOG_EXCEPTION( e );
                                        }

                                        p.first = nullptr;
                                        p.second = nullptr;
                                    }
                                }

                                if( !m_dirtyActorTransforms.empty() )
                                {
                                    Array<SmartPtr<IGameActor>> dirtyActorTransforms;
                                    dirtyActorTransforms.reserve( 1024 );

                                    SmartPtr<ITransform> transform;
                                    while( m_dirtyActorTransforms.try_pop( transform ) )
                                    {
                                        if( auto actor = transform->getActor() )
                                        {
                                            dirtyActorTransforms.push_back( actor );
                                        }
                                    }

                                    Array<SmartPtr<IGameActor>> uniqueActors;
                                    uniqueActors.reserve( dirtyActorTransforms.size() );

                                    for( auto &actor : dirtyActorTransforms )
                                    {
                                        if( std::find( uniqueActors.begin(), uniqueActors.end(),
                                                       actor ) == uniqueActors.end() )
                                        {
                                            uniqueActors.push_back( actor );
                                        }
                                    }

                                    for( auto &actor : uniqueActors )
                                    {
                                        actor->updateTransform();
                                    }
                                }

                                if( !m_dirtyActors.empty() )
                                {
                                    Pair<SmartPtr<IGameActor>, Pair<u32, u32>> actorFlagsEvent;
                                    while( m_dirtyActors.try_pop( actorFlagsEvent ) )
                                    {
                                        auto &flagsPair = actorFlagsEvent.second;

                                        auto previousFlags = flagsPair.first;
                                        auto newFlags = flagsPair.second;

                                        if( previousFlags != newFlags )
                                        {
                                            auto &actor = actorFlagsEvent.first;
                                            actor->updateDirty( newFlags, previousFlags );
                                        }
                                    }
                                }

                                if( !m_dirtyTransforms.empty() )
                                {
                                    Array<SmartPtr<IGameActor>> actors;
                                    actors.reserve( 1024 );

                                    SmartPtr<IGameActor> actor;
                                    while( m_dirtyTransforms.try_pop( actor ) )
                                    {
                                        actors.push_back( actor );
                                    }

                                    //std::cout << "Num actors: " << actors.size() << std::endl;

                                    for( auto &actor : actors )
                                    {
                                        actor->updateTransform();
                                    }
                                }

                                if( !m_dirtyComponents.empty() )
                                {
                                    SmartPtr<IComponent> component;
                                    while( m_dirtyComponents.try_pop( component ) )
                                    {
                                        //component->updateDirty();
                                    }
                                }

                                if( !m_dirtyComponentTransforms.empty() )
                                {
                                    SmartPtr<IComponent> component;
                                    while( m_dirtyComponentTransforms.try_pop( component ) )
                                    {
                                        if( component )
                                        {
                                            if( component->isLoaded() )
                                            {
                                                component->updateTransform();
                                            }
                                        }
                                    }
                                }

                                if( auto fsmManager = getFsmManager() )
                                {
                                    fsmManager->update();
                                }

                                for( auto &[id, fsmManager] : m_componentFsmManagers )
                                {
                                    fsmManager->update();
                                }

                                auto &components =
                                    getRegisteredComponents( Thread::UpdateState::PreUpdate, task );
                                for( auto &component : components )
                                {
                                    if( component )
                                    {
                                        component->preUpdate();
                                    }
                                }
                            }
                            catch( std::exception &e )
                            {
                                WP_LOG_EXCEPTION( e );
                            }
                        }
                    }

                    auto &components = getRegisteredComponents( Thread::UpdateState::Transform, task );
                    if( task == getSceneTask() )
                    {
                        auto actors = scene->getActors();
                        for( auto &actor : actors )
                        {
                            if( actor->isSmoothMotion() )
                            {
                                if( auto transform = actor->getTransform() )
                                {
                                    auto handle = actor->getHandle();
                                    auto id = handle->getInstanceId();
                                    auto transformTask = transform->getTask();

                                    auto smoothTransform = transform->getWorldTransform();
                                    if( getTransformState( id, transformTime, smoothDeltaTime,
                                                           smoothTransform, transformTask ) )
                                    {
                                        transform->setWorldTransform( smoothTransform );
                                        transform->setLocalDirty( true, false );
                                        transform->update();
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        for( auto component : components )
                        {
                            if( component )
                            {
                                if( component->isLoaded() )
                                {
                                    if( auto actor = component->getActorPtr() )
                                    {
                                        if( actor->isSmoothMotion() )
                                        {
                                            if( auto transform = actor->getTransform() )
                                            {
                                                auto handle = actor->getHandle();
                                                auto id = handle->getInstanceId();
                                                auto transformTask = transform->getTask();
                                                if( transformTask == TaskId::Application )
                                                {
                                                    continue;
                                                }

                                                auto renderTransform = Transform3<real_Num>();
                                                if( getTransformState( id, transformTime,
                                                                       smoothDeltaTime, renderTransform,
                                                                       transformTask ) )
                                                {
                                                    component->updateTransform( renderTransform );

                                                    auto children = actor->getChildren();
                                                    for( auto &child : children )
                                                    {
                                                        if( child->isSmoothMotion() )
                                                        {
                                                            updateActorTransformState( child,
                                                                                       renderTransform );
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    scene->preUpdate();
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GameManager::updateActorTransformState( SmartPtr<IGameActor> actor,
                                                 const Transform3<real_Num> &t )
    {
        if( actor )
        {
            if( auto transform = actor->getTransform() )
            {
                auto localTransform = transform->getLocalTransform();
                auto worldTransform = transform->getWorldTransform();

                Transform3<real_Num> actorWorldTransform;
                actorWorldTransform.transformFromParent( t, localTransform );

                auto components = actor->getComponents();
                for( auto &component : components )
                {
                    component->updateTransform( actorWorldTransform );
                }

                auto children = actor->getChildren();
                for( auto &child : children )
                {
                    if( child->isSmoothMotion() )
                    {
                        updateActorTransformState( child, actorWorldTransform );
                    }
                }
            }
        }
    }

    s32 GameManager::getLoadPriority( ISharedObject *obj )
    {
        if( obj->isDerived<Mesh>() )
        {
            return 100000;
        }
        if( obj->isDerived<MeshRenderer>() )
        {
            return 95000;
        }

        return 0;
    }

    void GameManager::update()
    {
        if( isLoaded() )
        {
            ScopedLoadLock loadLock( this );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto timer = applicationManager->getTimerPtr();
            WP_ASSERT( timer );

            if( auto scene = getCurrentScenePtr() )
            {
                scene->update();
            }

            auto sceneTask = getStateTask();
            auto task = Thread::getCurrentTask();

            if( task == sceneTask )
            {
                TryLockGuard lock( this );
                if( lock.locked() )
                {
                    for( auto &[key, system] : m_systems )
                    {
                        system->update();
                    }
                }
            }

            auto &components = getRegisteredComponents( Thread::UpdateState::Update, task );
            for( auto component : components )
            {
                if( component )
                {
                    component->update();
                }
            }
        }
    }

    void GameManager::postUpdate()
    {
        if( isLoaded() )
        {
            ScopedLoadLock loadLock( this );

            auto applicationManager = core::IApplicationManager::instancePtr();
            auto timer = applicationManager->getTimerPtr();

            auto t = timer->getTime();
            auto dt = timer->getDeltaTime();

            if( auto scene = getCurrentScenePtr() )
            {
                scene->postUpdate();
            }

            auto sceneTask = getStateTask();
            auto task = Thread::getCurrentTask();

            if( task == sceneTask )
            {
                TryLockGuard lock( this );
                if( lock.locked() )
                {
                    for( auto transform : m_smoothTransforms )
                    {
                        if( transform && transform->getTask() == task )
                        {
                            transform->update();

                            auto actor = transform->getActorPtr();
                            auto handle = actor->getHandle();
                            auto id = handle->getInstanceId();

                            auto worldTransform = transform->getWorldTransform();
                            auto frameTime = transform->getFrameTime();

                            addTransformState( id, frameTime, worldTransform );
                        }
                    }

                    auto &components = getRegisteredComponents( Thread::UpdateState::PostUpdate, task );
                    for( auto component : components )
                    {
                        if( component )
                        {
                            component->postUpdate();
                        }
                    }
                }
            }
        }
    }

    void GameManager::loadScene( const String &filePath, bool async )
    {
        if( !StringUtil::isNullOrEmpty( filePath ) )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto taskManager = applicationManager->getTaskManagerPtr();
            WP_ASSERT( taskManager );

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            auto renderLock = taskManager->lockTask( TaskId::Render );
            auto physicsLock = taskManager->lockTask( TaskId::Physics );

            auto scene = getCurrentScenePtr();
            if( scene )
            {
                auto ext = Path::getFileExtension( filePath );
                auto scenePath = filePath;

                if( StringUtil::isNullOrEmpty( ext ) )
                {
                    auto newFileName = scenePath + ApplicationUtil::builtinBinarySceneExt;
                    if( fileSystem->isExistingFile( newFileName, true, true ) )
                    {
                        scenePath = newFileName;
                    }
                    else
                    {
                        newFileName = scenePath + ApplicationUtil::builtinXmlSceneExt;
                        if( fileSystem->isExistingFile( newFileName, true, true ) )
                        {
                            scenePath = newFileName;
                        }
                        else
                        {
                            scenePath += ApplicationUtil::builtinSceneExt;
                        }
                    }
                }

                scene->clear();
                scene->loadScene( scenePath, async );
            }
        }
    }

    void GameManager::loadSceneDataStr( const String &data, bool async )
    {
        if( !StringUtil::isNullOrEmpty( data ) )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto taskManager = applicationManager->getTaskManagerPtr();
            WP_ASSERT( taskManager );

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            //auto renderLock = taskManager->lockTask( TaskId::Render );
            //auto physicsLock = taskManager->lockTask( TaskId::Physics );
            auto scene = getCurrentScenePtr();
            if( scene )
            {
                scene->clear();
                scene->loadSceneDataStr( data, async );
            }

            applicationManager->triggerEvent( EventType::Loading, IEvent::sceneChanged,
                                              Array<Parameter>(), this, this, nullptr );
        }
    }

    void GameManager::clear()
    {
        if( m_numActors == 0 )
        {
            return;
        }

        auto actors = getActors();
        for( auto actor : actors )
        {
            if( actor && actor->getLoadingState() != LoadingState::Unloaded )
            {
                destroyActor( actor );
            }
        }
    }

    void GameManager::setFsmManager( SmartPtr<IFSMManager> fsmManager )
    {
        m_fsmManager = fsmManager;
    }

    IFSMManager *GameManager::getComponentFsmManagerPtr( u32 typeId )
    {
        auto it = m_componentFsmManagers.find( typeId );
        if( it != m_componentFsmManagers.end() )
        {
            return it->second.get();
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto fsmManager = factoryManager->make_ptr<FSMManager>();
        fsmManager->setGrowSize( 1024 );
        fsmManager->load( nullptr );
        m_componentFsmManagers[typeId] = fsmManager;
        return fsmManager.get();
    }

    SmartPtr<IFSMManager> GameManager::getComponentFsmManager( u32 typeId )
    {
        auto it = m_componentFsmManagers.find( typeId );
        if( it != m_componentFsmManagers.end() )
        {
            return it->second;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto fsmManager = factoryManager->make_ptr<FSMManager>();
        fsmManager->setGrowSize( 1024 );
        fsmManager->load( nullptr );
        m_componentFsmManagers[typeId] = fsmManager;
        return fsmManager;
    }

    void GameManager::setComponentFsmManager( u32 typeId, SmartPtr<IFSMManager> fsmManager )
    {
        m_componentFsmManagers[typeId] = fsmManager;
    }

    SmartPtr<IGameScene> GameManager::getCurrentScene() const
    {
        return m_scene;
    }

    void GameManager::setCurrentScene( SmartPtr<IGameScene> scene )
    {
        m_scene = scene;
    }

    Array<SmartPtr<IGameActor>> GameManager::getActors() const
    {
        return m_actors.snapshot();
    }

    SmartPtr<IGameActor> GameManager::createActor()
    {
        try
        {
            if( !isLoaded() )
            {
                return nullptr;
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto fsmManager = getFsmManagerPtr();
            if( !fsmManager )
            {
                WP_LOG_ERROR( "FSM Manager is null" );
                return nullptr;
            }

            ScopedLock lock( this );

            auto newId = 0;
            for( auto actor : m_actors )
            {
                if( !actor )
                {
                    break;
                }

                newId++;
            }

            if( static_cast<size_t>( newId ) >= m_actors.size() )
                throw std::length_error( "Actor capacity exhausted" );
            auto actor = factoryManager->make_ptr<GameActor>( newId );

            if( auto handle = actor->getHandle() )
            {
                handle->setInstanceId( newId );

                handle->setId( newId );

                auto uuid = StringUtil::getUUID();
                handle->setUUID( uuid );
            }

            actor->load( nullptr );

            WP_ASSERT( m_actors[newId] == nullptr );
            m_actors[newId] = actor;

            auto fsm = fsmManager->createFSM();
            auto gameFSM = fsmManager->createFSM();

            m_fsms[newId] = fsm;
            m_gameFSMs[newId] = gameFSM;

            auto actorFsmListener = factoryManager->make_ptr<GameActor::FsmListener>();

            m_fsmListeners[newId] = actorFsmListener;

            actorFsmListener->setOwner( actor );
            fsm->addListener( actorFsmListener );

            ++m_numActors;
            return actor;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    IGameActor *GameManager::createActorPtr()
    {
        auto actor = createActor();
        return actor.get();
    }

    void GameManager::destroyActor( SmartPtr<IGameActor> actor, bool cascade /*= true */ )
    {
        try
        {
            if( !actor )
            {
                return;
            }

            const auto &destroyState = getLoadingState();
            if( destroyState != LoadingState::Loaded && destroyState != LoadingState::Unloading )
            {
                return;
            }

            if( cascade )
            {
                auto children = actor->getChildren();
                for( auto child : children )
                {
                    if( child )
                    {
                        destroyActor( child );
                    }
                }
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto fsmManager = getFsmManagerPtr();

            if( actor )
            {
                if( auto currentScene = actor->getScenePtr() )
                {
                    currentScene->removeActor( actor );
                }

                if( auto parent = actor->getParentPtr() )
                {
                    parent->removeChild( actor );
                }

                auto actorId = 0;

                if( auto handle = actor->getHandle() )
                {
                    actorId = handle->getInstanceId();
                }

                {
                    ScopedLock lock( this );
                    if( actorId >= static_cast<s32>( m_actors.size() ) || m_actors[actorId] != actor )
                    {
                        return;
                    }
                }

                auto fsm = m_fsms[actorId];
                auto gameFSM = m_gameFSMs[actorId];

                auto actorFsmListener = m_fsmListeners[actorId];
                auto actorGameFsmListener = m_gameFsmListeners[actorId];

                actor->unload( nullptr );

                if( fsm )
                {
                    if( actorFsmListener )
                    {
                        fsm->removeListener( actorFsmListener );
                        actorFsmListener = nullptr;
                    }

                    if( fsmManager )
                    {
                        fsmManager->destroyFSM( fsm );
                    }

                    fsm = nullptr;
                }

                if( gameFSM )
                {
                    if( actorGameFsmListener )
                    {
                        gameFSM->removeListener( actorGameFsmListener );
                        actorGameFsmListener = nullptr;
                    }

                    if( fsmManager )
                    {
                        fsmManager->destroyFSM( gameFSM );
                    }

                    gameFSM = nullptr;
                }

                removeDirty( actor );

                ScopedLock lock( this );
                for( u32 i = 0; i < static_cast<u32>( TaskId::Count ); ++i )
                {
                    if( actorId < static_cast<s32>( m_transformTimes[i].size() ) )
                    {
                        m_transformTimes[i][actorId].clear();
                    }

                    if( actorId < static_cast<s32>( m_transformStates[i].size() ) )
                    {
                        m_transformStates[i][actorId].clear();
                    }

                    if( actorId < static_cast<s32>( m_motionStates[i].size() ) )
                    {
                        m_motionStates[i][actorId].clear();
                    }

                    if( actorId < static_cast<s32>( m_lastTransformStates[i].size() ) )
                    {
                        auto &lastTransformStates = m_lastTransformStates[i][actorId];
                        lastTransformStates.clear();
                        lastTransformStates.resize( 1 );
                    }
                }

                m_fsmListeners[actorId] = nullptr;
                m_gameFsmListeners[actorId] = nullptr;

                m_fsms[actorId] = nullptr;
                m_gameFSMs[actorId] = nullptr;

                m_actors[actorId] = nullptr;

                actor = nullptr;

                --m_numActors;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GameManager::destroyActors()
    {
        try
        {
            if( m_numActors == 0 )
            {
                return;
            }

            ScopedLock lock( this );

            for( auto &actor : m_actors )
            {
                if( actor )
                {
                    destroyActor( actor );
                    actor = nullptr;
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GameManager::play()
    {
        if( auto scene = getCurrentScenePtr() )
        {
            scene->setState( IGameScene::State::Play );
        }
    }

    void GameManager::edit()
    {
        if( auto scene = getCurrentScenePtr() )
        {
            scene->setState( IGameScene::State::Edit );
        }
    }

    void GameManager::stop()
    {
        if( auto scene = getCurrentScenePtr() )
        {
            scene->setState( IGameScene::State::None );
        }
    }

    u32 GameManager::addTransformComponent( SmartPtr<ITransform> transformComponent )
    {
        auto id = static_cast<u32>( m_transforms.size() );
        for( auto i = 0u; i < m_transforms.size(); ++i )
        {
            if( !m_transforms[i] )
            {
                id = i;
                break;
            }
        }

        if( id == m_transforms.size() )
        {
            m_transforms.push_back( transformComponent );
            id = static_cast<u32>( m_transforms.size() - 1 );
        }
        else
        {
            m_transforms[id] = transformComponent;
        }

        if( auto handle = transformComponent->getHandle() )
        {
            handle->setInstanceId( id );
        }

        return id;
    }

    u32 GameManager::addComponent( SmartPtr<IComponent> component )
    {
        if( !component )
            throw std::invalid_argument( "Cannot register a null component" );
        auto handle = component->getHandle();
        if( handle && handle->getUUIDAsString().empty() )
            handle->setUUID( StringUtil::getUUID() );
        const auto type = component->getTypeInfo();
        SmartPtr<IComponentSystem> system;
        m_systems.tryGet( type, system );
        u32 systemId = std::numeric_limits<u32>::max();
        if( system )
        {
            systemId = system->addComponent( component );
            if( systemId == std::numeric_limits<u32>::max() )
                throw std::length_error( "Component system capacity exhausted" );
        }
        try
        {
            auto registry = m_components.writeLocked();
            auto &components = registry.emplace( type ).first->second;
            if( components.size() < size )
                components.resize( size );
            size_t position;
            if( system )
            {
                // System and manager must publish the same ID; removal uses the handle's ID.
                position = systemId;
                if( position >= components.size() )
                    components.resize( position + 1 );
                if( components[position] && components[position] != component )
                    throw std::logic_error(
                        "Component system ID collides with an existing registration" );
            }
            else
            {
                auto existing = std::find( components.begin(), components.end(), component );
                if( existing != components.end() )
                    return static_cast<u32>( existing - components.begin() );
                auto free = std::find( components.begin(), components.end(), nullptr );
                if( free == components.end() )
                    throw std::length_error( "Component capacity exhausted" );
                position = static_cast<size_t>( free - components.begin() );
            }
            components[position] = component;
            if( handle )
                handle->setInstanceId( static_cast<u32>( position ) );
            return static_cast<u32>( position );
        }
        catch( ... )
        {
            // Release the registry view before entering system code.
            if( system )
                system->removeComponent( systemId );
            throw;
        }
    }

    u32 GameManager::removeComponent( SmartPtr<IComponent> component )
    {
        if( !component )
        {
            return 0;
        }

        auto handle = component->getHandle();
        if( !handle )
        {
            return 0;
        }

        auto pos = handle->getInstanceId();

        auto typeInfo = component->getTypeInfo();

        SmartPtr<IComponentSystem> system;
        if( m_systems.tryGet( typeInfo, system ) && system )
        {
            system->removeComponent( component );
        }

        auto registry = m_components.writeLocked();
        auto itComponent = registry.find( typeInfo );
        if( itComponent != registry.end() )
        {
            auto &components = itComponent->second;
            if( pos < components.size() && components[pos] == component )
            {
                components[pos] = nullptr;
            }
        }

        return 0;
    }

    void GameManager::addSystem( u32 id, SmartPtr<IComponentSystem> system )
    {
        auto systems = m_systems.writeLocked();
        if( systems.contains( id ) )
            systems.at( id ) = system;
        else
            systems.emplace( id, system );
    }

    void GameManager::removeSystem( u32 id )
    {
        m_systems.erase( id );
    }

    SmartPtr<IGameActor> GameManager::getActor( u32 id ) const
    {
        WP_ASSERT( id < m_actors.size() );
        return m_actors[id];
    }

    SmartPtr<IGameActor> GameManager::getActorByName( const String &name ) const
    {
        const auto actors = getActors();
        for( const auto &actor : actors )
        {
            if( actor )
            {
                const auto actorName = actor->getName();

                if( actorName == name )
                {
                    return actor;
                }
            }
        }

        return nullptr;
    }

    Array<SmartPtr<IGameActor>> GameManager::getActorsByName( const String &name ) const
    {
        Array<SmartPtr<IGameActor>> actorsFound;
        actorsFound.reserve( 12 );

        const auto actors = getActors();
        for( const auto &actor : actors )
        {
            if( actor )
            {
                const auto actorName = actor->getName();

                if( actorName == name )
                {
                    actorsFound.push_back( actor );
                }
            }
        }

        return actorsFound;
    }

    SmartPtr<IGameActor> GameManager::getActorByFileId( const String &id ) const
    {
        auto uuid = StringUtil::parseUUID( id );

        auto actors = getActors();
        for( auto actor : actors )
        {
            if( auto handle = actor->getHandle() )
            {
                if( handle->getFileId() == uuid )
                {
                    return actor;
                }
            }
        }

        return nullptr;
    }

    IFSM *GameManager::getFSMPtr( u32 id ) const
    {
        if( id < m_fsms.size() )
        {
            return m_fsms[id].get();
        }

        return nullptr;
    }

    SmartPtr<IFSM> GameManager::getFSM( u32 id ) const
    {
        if( id < m_fsms.size() )
        {
            return m_fsms[id];
        }

        return nullptr;
    }

    SmartPtr<ITransform> GameManager::createTransform()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto transform = factoryManager->make_object<ITransform>();
        WP_ASSERT( transform );

        transform->load( nullptr );
        addTransformComponent( transform );

        return transform;
    }

    void GameManager::destroyTransform( SmartPtr<ITransform> transform )
    {
        if( transform )
        {
            auto handle = transform->getHandle();
            auto pos = handle->getInstanceId();

            transform->unload( nullptr );

            if( pos < m_transforms.size() && m_transforms[pos] == transform )
            {
                m_transforms[pos] = nullptr;
            }
            else
            {
                m_transforms.erase( transform );
            }
        }
    }

    SmartPtr<Transform> GameManager::getTransform( u32 id ) const
    {
        WP_ASSERT( id < m_transforms.size() );
        return m_transforms[id];
    }

    void GameManager::addDirty( SmartPtr<IGameActor> actor )
    {
        if( isLoaded() )
        {
            if( actor )
            {
                auto handle = actor->getHandle();
                if( handle )
                {
                    auto id = handle->getInstanceId();

                    auto pair = Pair<SmartPtr<IGameActor>, Pair<u32, u32>>();
                    pair.first = actor;

                    auto &flagsPair = pair.second;
                    flagsPair.first = actor->getPreviousFlags();
                    flagsPair.second = actor->getFlags();

                    m_dirtyActors.push( pair );
                }
            }
        }
    }

    void GameManager::removeDirty( SmartPtr<IGameActor> actor )
    {
        if( isLoaded() )
        {
            auto dirtyActors = Array<Pair<SmartPtr<IGameActor>, Pair<u32, u32>>>();
            dirtyActors.reserve( 32 );

            while( !m_dirtyActors.empty() )
            {
                Pair<SmartPtr<IGameActor>, Pair<u32, u32>> dirtyActor;
                if( m_dirtyActors.try_pop( dirtyActor ) )
                {
                    dirtyActors.push_back( dirtyActor );
                }
            }

            dirtyActors.erase(
                std::remove_if( dirtyActors.begin(), dirtyActors.end(),
                                [actor]( const Pair<SmartPtr<IGameActor>, Pair<u32, u32>> &p ) {
                                    return p.first == actor;
                                } ),
                dirtyActors.end() );

            for( auto &dirtyActor : dirtyActors )
            {
                m_dirtyActors.push( dirtyActor );
            }
        }
    }

    void GameManager::addDirtyComponent( SmartPtr<IComponent> component )
    {
        if( isLoaded() )
        {
            m_dirtyComponents.push( component );
        }
    }

    void GameManager::addDirtyComponentTransform( SmartPtr<IComponent> component )
    {
        if( isLoaded() )
        {
            m_dirtyComponentTransforms.push( component );
        }
    }

    void GameManager::loadObject( SmartPtr<ISharedObject> object, bool forceQueue )
    {
#if 1
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        ScopedLock lock( this, true );

        object->load( nullptr );

        if( object->isDerived<IGameActor>() )
        {
            auto args = Array<Parameter>();
            applicationManager->triggerEvent( EventType::Scene, IEvent::actorLoaded, args, this, object,
                                              nullptr, false, Thread::Application_Flag );
        }
        else if( object->isDerived<IComponent>() )
        {
            auto args = Array<Parameter>();
            applicationManager->triggerEvent( EventType::Scene, IEvent::componentLoaded, args, this,
                                              object, nullptr, false, Thread::Application_Flag );
        }
#else
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto task = Thread::getCurrentTask();
        if( task == TaskId::Primary )
        {
            ScopedLock lock( this, true );

            object->load( nullptr );

            if( object->isDerived<IGameActor>() )
            {
                auto args = Array<Parameter>();
                applicationManager->triggerEvent( EventType::Scene, IEvent::actorLoaded, args, this,
                                                  object, nullptr, false, Thread::Application_Flag );
            }
            else if( object->isDerived<IComponent>() )
            {
                auto args = Array<Parameter>();
                applicationManager->triggerEvent( EventType::Scene, IEvent::componentLoaded, args, this,
                                                  object, nullptr, false, Thread::Application_Flag );
            }
        }
        else if( task == getSceneTask() )
        {
            if( forceQueue )
            {
                if( object->isDerived<IGameActor>() )
                {
                    m_loadingActors.push( { object, nullptr } );
                }
                else if( object->isDerived<IComponent>() )
                {
                    m_loadingComponents.push( { object, nullptr } );
                }
            }
            else
            {
                TryLockGuard lock( this );
                if( lock.locked() )
                {
                    object->load( nullptr );

                    if( object->isDerived<IGameActor>() )
                    {
                        auto args = Array<Parameter>();
                        applicationManager->triggerEvent( EventType::Scene, IEvent::actorLoaded, args,
                                                          this, object, nullptr, false,
                                                          Thread::Application_Flag );
                    }
                    else if( object->isDerived<IComponent>() )
                    {
                        auto args = Array<Parameter>();
                        applicationManager->triggerEvent( EventType::Scene, IEvent::componentLoaded,
                                                          args, this, object, nullptr, false,
                                                          Thread::Application_Flag );
                    }
                }
                else
                {
                    if( object->isDerived<IGameActor>() )
                    {
                        m_loadingActors.push( { object, nullptr } );
                    }
                    else if( object->isDerived<IComponent>() )
                    {
                        m_loadingComponents.push( { object, nullptr } );
                    }
                }
            }
        }
        else
        {
            if( object->isDerived<IGameActor>() )
            {
                m_loadingActors.push( { object, nullptr } );
            }
            else if( object->isDerived<IComponent>() )
            {
                m_loadingComponents.push( { object, nullptr } );
            }
        }
#endif
    }

    void GameManager::loadObject( SmartPtr<ISharedObject> object, SmartPtr<ISharedObject> data,
                                  bool forceQueue )
    {
#if 1
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        ScopedLock lock( this, true );

        object->load( data );

        Parameter dataParam;
        dataParam.object = data;

        auto args = Array<Parameter>( { dataParam } );

        if( object->isDerived<IGameActor>() )
        {
            applicationManager->triggerEvent( EventType::Scene, IEvent::actorLoaded, args, this, object,
                                              nullptr, false, Thread::Application_Flag );
        }
        else if( object->isDerived<IComponent>() )
        {
            applicationManager->triggerEvent( EventType::Scene, IEvent::componentLoaded, args, this,
                                              object, nullptr, false, Thread::Application_Flag );
        }
#else
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto task = Thread::getCurrentTask();
        if( task == TaskId::Primary )
        {
            ScopedLock lock( this, true );

            object->load( data );

            Parameter dataParam;
            dataParam.object = data;

            auto args = Array<Parameter>( { dataParam } );

            if( object->isDerived<IGameActor>() )
            {
                applicationManager->triggerEvent( EventType::Scene, IEvent::actorLoaded, args, this,
                                                  object, nullptr, false, Thread::Application_Flag );
            }
            else if( object->isDerived<IComponent>() )
            {
                applicationManager->triggerEvent( EventType::Scene, IEvent::componentLoaded, args, this,
                                                  object, nullptr, false, Thread::Application_Flag );
            }
        }
        else if( task == getSceneTask() )
        {
            if( forceQueue )
            {
                if( object->isDerived<IGameActor>() )
                {
                    m_loadingActors.push( { object, data } );
                }
                else if( object->isDerived<IComponent>() )
                {
                    m_loadingComponents.push( { object, data } );
                }
            }
            else
            {
                TryLockGuard lock( this );
                if( lock.locked() )
                {
                    object->load( data );

                    Parameter dataParam;
                    dataParam.object = data;

                    auto args = Array<Parameter>( { dataParam } );

                    if( object->isDerived<IGameActor>() )
                    {
                        applicationManager->triggerEvent( EventType::Scene, IEvent::actorLoaded, args,
                                                          this, object, nullptr, false,
                                                          Thread::Application_Flag );
                    }
                    else if( object->isDerived<IComponent>() )
                    {
                        applicationManager->triggerEvent( EventType::Scene, IEvent::componentLoaded,
                                                          args, this, object, nullptr, false,
                                                          Thread::Application_Flag );
                    }
                }
                else
                {
                    if( object->isDerived<IGameActor>() )
                    {
                        m_loadingActors.push( { object, data } );
                    }
                    else if( object->isDerived<IComponent>() )
                    {
                        m_loadingComponents.push( { object, data } );
                    }
                }
            }
        }
        else
        {
            if( object->isDerived<IGameActor>() )
            {
                m_loadingActors.push( { object, data } );
            }
            else if( object->isDerived<IComponent>() )
            {
                m_loadingComponents.push( { object, data } );
            }
        }
#endif
    }

    void GameManager::unloadObject( SmartPtr<ISharedObject> object, bool forceQueue )
    {
#if 0
        object->unload( nullptr );
#else
        auto task = Thread::getCurrentTask();

        if( task == getSceneTask() )
        {
            if( forceQueue )
            {
                if( object->isDerived<IGameActor>() )
                {
                    m_unloadingActors.push( object );
                }
                else if( object->isDerived<IComponent>() )
                {
                    m_unloadingComponents.push( object );
                }
            }
            else
            {
                TryLockGuard lock( this );
                if( lock.locked() )
                {
                    object->unload( nullptr );
                }
                else
                {
                    if( object->isDerived<IGameActor>() )
                    {
                        m_unloadingActors.push( object );
                    }
                    else if( object->isDerived<IComponent>() )
                    {
                        m_unloadingComponents.push( object );
                    }
                }
            }
        }
        else
        {
            if( object->isDerived<IGameActor>() )
            {
                m_unloadingActors.push( object );
            }
            else if( object->isDerived<IComponent>() )
            {
                m_unloadingComponents.push( object );
            }
        }
#endif
    }

    void GameManager::queueProperties( SmartPtr<ISharedObject> object, SmartPtr<Properties> properties )
    {
        if( isLoaded() )
        {
            auto p = workphone::make_pair( object.get(), properties );
            m_queueProperties.push( p );
        }
    }

    void GameManager::makeActorTransformsDirty()
    {
        auto actors = getActors();
        for( auto actor : actors )
        {
            if( actor )
            {
                addDirtyActor( actor );
            }
        }
    }

    void GameManager::addDirtyActor( SmartPtr<IGameActor> actor )
    {
        if( isLoaded() )
        {
            m_dirtyTransforms.push( actor );
        }
    }

    void GameManager::addDirtyTransform( SmartPtr<ITransform> transform )
    {
        if( isLoaded() )
        {
            m_dirtyActorTransforms.push( transform );
        }
    }

    TaskId GameManager::getStateTask() const
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        if( auto threadPool = applicationManager->getThreadPool() )
        {
            if( threadPool->getNumThreads() > 0 )
            {
                auto hasTasks = applicationManager->hasTasks();
                return hasTasks ? TaskId::Application : TaskId::Primary;
            }
        }

        return TaskId::Primary;
    }

    TaskId GameManager::getSceneTask() const
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        if( applicationManager->isLoading() )
        {
            return TaskId::Primary;
        }

        auto taskManager = applicationManager->getTaskManager();
        if( taskManager )
        {
            auto task = taskManager->getTask( TaskId::Application );
            if( task )
            {
                if( task->isExecuting() )
                {
                    return TaskId::Application;
                }
                if( task->isPrimary() )
                {
                    return TaskId::Primary;
                }
            }
        }

        return applicationManager->hasTasks() ? TaskId::Application : TaskId::Primary;
    }

    Array<SmartPtr<IComponent>> GameManager::getComponents() const
    {
        Array<SmartPtr<IComponent>> components;
        auto registry = m_components.readLocked();
        for( const auto &it : registry )
        {
            auto &c = it.second;
            components.insert( components.begin(), c.begin(), c.end() );
        }

        return components;
    }

    Array<SmartPtr<IComponent>> GameManager::getComponents( u32 type ) const
    {
        auto registry = m_components.readLocked();
        auto it = registry.find( type );
        if( it != registry.end() )
        {
            return it->second;
        }

        return {};
    }

    Array<SmartPtr<ITransform>> GameManager::getTransforms() const
    {
        return m_transforms.snapshot();
    }

    void GameManager::setTransforms( const Array<SmartPtr<ITransform>> transforms )
    {
        m_transforms = { transforms.begin(), transforms.end() };
    }

    void GameManager::addSmoothTransform( SmartPtr<ITransform> transform )
    {
        m_smoothTransforms.push_back( transform );
    }

    void GameManager::removeSmoothTransform( SmartPtr<ITransform> transform )
    {
        m_smoothTransforms.erase(
            std::remove( m_smoothTransforms.begin(), m_smoothTransforms.end(), transform ),
            m_smoothTransforms.end() );
    }

    Array<String> GameManager::getComponentFactoryIgnoreList() const
    {
        return m_componentFactoryIgnoreList.snapshot();
    }

    Map<String, String> GameManager::getComponentFactoryMap() const
    {
        Map<String, String> map;
        for( auto &pair : m_componentFactoryMap )
        {
            map.emplace( pair.first, pair.second );
        }

        return map;
    }

    void GameManager::setComponentFactoryMap( const Map<String, String> &map )
    {
        m_componentFactoryMap.clear();

        for( auto &pair : map )
        {
            m_componentFactoryMap.emplace( pair.first, pair.second );
        }
    }

    String GameManager::getComponentFactoryType( const String &type ) const
    {
        ScopedLock lock( this );

        if( std::find( m_componentFactoryIgnoreList.begin(), m_componentFactoryIgnoreList.end(),
                       type ) != m_componentFactoryIgnoreList.end() )
        {
            return String( "" );
        }

        auto it = m_componentFactoryMap.find( type );
        if( it != m_componentFactoryMap.end() )
        {
            return it->second;
        }

        return "";
    }

    void GameManager::addTransformState( u32 id, time_interval time,
                                         const Transform3<real_Num> &transform )
    {
        ScopedLock lock( this );

        const auto requiredSize = static_cast<size_t>( id ) + 1;
        for( u32 i = 0; i < static_cast<u32>( TaskId::Count ); ++i )
        {
            if( m_transformTimes[i].size() < requiredSize )
            {
                m_transformTimes[i].resize( requiredSize );
            }
            if( m_transformStates[i].size() < requiredSize )
            {
                m_transformStates[i].resize( requiredSize );
            }
            if( m_lastTransformStates[i].size() < requiredSize )
            {
                m_lastTransformStates[i].resize( requiredSize );
            }
            if( m_lastTransformStates[i][id].empty() )
            {
                m_lastTransformStates[i][id].resize( 1 );
            }
            if( m_motionStates[i].size() < requiredSize )
            {
                m_motionStates[i].resize( requiredSize );
            }
        }

        auto task = Thread::getCurrentTask();
        auto &transformTimes = m_transformTimes[(u32)task];
        auto &transformStates = m_transformStates[(u32)task];
        auto &motionStates = m_motionStates[(u32)task];

        if( id >= transformTimes.size() )
        {
            return;
        }

        if( id >= transformStates.size() )
        {
            return;
        }

        if( id >= motionStates.size() )
        {
            return;
        }

        auto &times = transformTimes[id];
        auto &transforms = transformStates[id];
        auto &motions = motionStates[id];

        auto hasTime = false;
        for( auto &t : times )
        {
            if( Math<time_interval>::equals( t, time ) )
            {
                hasTime = true;
            }
        }

        if( !hasTime )
        {
            if( times.capacity() < maxSmoothSize )
            {
                times.reserve( maxSmoothSize );
            }

            if( transforms.capacity() < maxSmoothSize )
            {
                transforms.reserve( maxSmoothSize );
            }

            if( motions.capacity() < maxSmoothSize )
            {
                motions.reserve( maxSmoothSize );
            }

            if( times.size() >= maxSmoothSize )
            {
                times.resize( maxSmoothSize - 1 );
            }

            if( transforms.size() >= maxSmoothSize )
            {
                transforms.resize( maxSmoothSize - 1 );
            }

            times.insert( times.begin(), time );
            transforms.insert( transforms.begin(), transform );

            MotionState motion;
            motion.linearVelocity = Vector3<real_Num>::zero();
            motion.angularVelocity = Vector3<real_Num>::zero();
            motions.insert( motions.begin(), motion );
        }
    }

    void GameManager::addTransformState( u32 id, time_interval time,
                                         const Transform3<real_Num> &transform,
                                         const Vector3<real_Num> &linearVelocity,
                                         const Vector3<real_Num> &angularVelocity )
    {
        ScopedLock lock( this );

        const auto requiredSize = static_cast<size_t>( id ) + 1;
        for( u32 i = 0; i < static_cast<u32>( TaskId::Count ); ++i )
        {
            if( m_transformTimes[i].size() < requiredSize )
            {
                m_transformTimes[i].resize( requiredSize );
            }
            if( m_transformStates[i].size() < requiredSize )
            {
                m_transformStates[i].resize( requiredSize );
            }
            if( m_lastTransformStates[i].size() < requiredSize )
            {
                m_lastTransformStates[i].resize( requiredSize );
            }
            if( m_lastTransformStates[i][id].empty() )
            {
                m_lastTransformStates[i][id].resize( 1 );
            }
            if( m_motionStates[i].size() < requiredSize )
            {
                m_motionStates[i].resize( requiredSize );
            }
        }

        auto task = Thread::getCurrentTask();
        auto &transformTimes = m_transformTimes[(u32)task];
        auto &transformStates = m_transformStates[(u32)task];
        auto &motionStates = m_motionStates[(u32)task];

        const auto maxElementCount = 12;

        if( id >= transformTimes.size() )
        {
            return;
        }

        if( id >= transformStates.size() )
        {
            return;
        }

        if( id >= motionStates.size() )
        {
            return;
        }

        auto &times = transformTimes[id];
        auto &transforms = transformStates[id];
        auto &motions = motionStates[id];

        auto hasTime = false;
        for( auto &t : times )
        {
            if( Math<time_interval>::equals( t, time ) )
            {
                hasTime = true;
            }
        }

        if( !hasTime )
        {
            if( times.capacity() < maxElementCount )
            {
                times.reserve( maxElementCount );
            }

            if( transforms.capacity() < maxElementCount )
            {
                transforms.reserve( maxElementCount );
            }

            if( motions.capacity() < maxElementCount )
            {
                motions.reserve( maxElementCount );
            }

            if( times.size() >= maxElementCount )
            {
                times.resize( maxElementCount - 1 );
            }

            if( transforms.size() >= maxElementCount )
            {
                transforms.resize( maxElementCount - 1 );
            }

            if( motions.size() >= maxElementCount )
            {
                motions.resize( maxElementCount - 1 );
            }

            times.insert( times.begin(), time );
            transforms.insert( transforms.begin(), transform );

            MotionState motion;
            motion.linearVelocity = linearVelocity;
            motion.angularVelocity = angularVelocity;
            motions.insert( motions.begin(), motion );
        }
    }

    bool GameManager::getTransformState( u32 id, time_interval t, time_interval dt,
                                         Transform3<real_Num> &transform, TaskId task )
    {
        TryLockGuard lock( this );
        if( lock.locked() )
        {
            auto &transformTimes = m_transformTimes[(u32)task];
            auto &transformStates = m_transformStates[(u32)task];
            auto &lastTransformStates = m_lastTransformStates[(u32)task];
            auto &motionStates = m_motionStates[(u32)task];

            if( id >= transformTimes.size() )
            {
                return false;
            }

            if( id >= transformStates.size() )
            {
                return false;
            }

            if( id >= motionStates.size() )
            {
                return false;
            }

            const auto &times = transformTimes[id];
            const auto &transforms = transformStates[id];
            auto &lastTransforms = lastTransformStates[id];
            const auto &motions = motionStates[id];

            if( times.empty() )
            {
                return false;
            }

            if( transforms.empty() )
            {
                return false;
            }

            if( motions.empty() )
            {
                return false;
            }

            // Only one state available - we can only return it directly or extrapolate
            // using the stored motion state, but there is no previous state to derive
            // a velocity from.
            if( times.size() == 1 )
            {
                const auto &time0 = times.front();
                const auto &frontTransform = transforms.front();
                const auto &frontMotion = motions.front();

                // Exact match
                if( Math<time_interval>::equals( time0, t ) )
                {
                    transform = frontTransform;
                    lastTransforms[0] = transform;
                    return true;
                }

                // Extrapolate forward using the stored linear velocity.
                auto diffFrame = t - time0;
                if( diffFrame > 0.0 )
                {
                    auto position =
                        frontTransform.getPosition() + frontMotion.linearVelocity * (real_Num)diffFrame;
                    transform = Transform3<real_Num>( position, frontTransform.getOrientation(),
                                                      frontTransform.getScale() );
                    lastTransforms[0] = transform;
                    return true;
                }

                // t is before the single stored state - cannot extrapolate backward
                // without prior data, so return the state at time0.
                transform = frontTransform;
                lastTransforms[0] = transform;
                return true;
            }

            if( times.size() >= 2 && times.front() > t )
            {
                for( size_t i = 1; i < times.size(); ++i )
                {
                    auto &time = times[i];
                    if( time < t )
                    {
                        auto cur = i;
                        auto next = Math<s32>::clamp( static_cast<s32>( cur - 1 ), 0,
                                                      static_cast<s32>( times.size() - 1 ) );

                        auto &time0 = times[next];
                        auto &time1 = times[cur];

                        if( next == cur )
                        {
                            transform = transforms.front();
                            return true;
                        }

                        auto &transform0 = transforms[next];
                        auto &transform1 = transforms[cur];

                        auto &fPosition0 = transform0.getPosition();
                        auto &fPosition1 = transform1.getPosition();

                        auto &fOrientation0 = transform0.getOrientation();
                        auto &fOrientation1 = transform1.getOrientation();

                        auto position0 = Vector3<real_dNum>( fPosition0.x, fPosition0.y, fPosition0.z );
                        auto position1 = Vector3<real_dNum>( fPosition1.x, fPosition1.y, fPosition1.z );

                        auto orientation0 = Quaternion<real_dNum>( fOrientation0.w, fOrientation0.x,
                                                                   fOrientation0.y, fOrientation0.z );
                        auto orientation1 = Quaternion<real_dNum>( fOrientation1.w, fOrientation1.x,
                                                                   fOrientation1.y, fOrientation1.z );

                        auto diff = time0 - time1;
                        if( diff > 0.0 )
                        {
                            auto diffFrame = t - time1;
                            auto delta = diffFrame / diff;

                            auto p = Math<real_dNum>::lerp( position1, position0, delta );

                            auto o =
                                Quaternion<real_dNum>::slerp( delta, orientation1, orientation0, true );
                            o.normalise();

                            auto scale =
                                Math<real_Num>::lerp( transform1.getScale(), transform0.getScale(),
                                                      static_cast<real_Num>( delta ) );

                            transform = Transform3<real_Num>( Vector3<real_Num>( p.x, p.y, p.z ),
                                                              Quaternion<real_Num>( o.w, o.x, o.y, o.z ),
                                                              scale );

                            const auto &last = lastTransforms[0];
                            const auto &latest = transforms.front();
                            const auto elapsed = static_cast<real_Num>( t - times.front() );
                            const auto position = last.getPosition() +
                                                  ( transform.getPosition() - last.getPosition() ) * dt;
                            transform = Transform3<real_Num>( position, latest.getOrientation(),
                                                              latest.getScale() );
                            lastTransforms[0] = transform;
                            return true;
                        }
                    }
                }
            }
            else
            {
                auto timer = core::IApplicationManager::instancePtr()->getTimerPtr();
                if( timer->getTimeSinceSceneLoad() > 5.0 )
                {
                    // Predict from the newest physics sample, just as in the
                    // single-sample path. Accumulating from the previous rendered
                    // transform makes moving actors lag farther behind each frame.
                    const auto &last = lastTransforms[0];
                    const auto &latest = transforms.front();
                    const auto elapsed = static_cast<real_Num>( t - times.front() );
                    const auto position =
                        last.getPosition() + ( latest.getPosition() - last.getPosition() ) * t;
                    transform =
                        Transform3<real_Num>( position, latest.getOrientation(), latest.getScale() );
                    lastTransforms[0] = transform;
                    return true;
                }
            }
        }

        return false;
    }

    ConcurrentArray<SmartPtr<IComponent>> &GameManager::getRegisteredComponents(
        Thread::UpdateState state, TaskId task )
    {
        if( static_cast<s32>( state ) < m_updateComponents.size() )
        {
            auto &updateComponents = m_updateComponents[static_cast<s32>( state )];
            if( static_cast<s32>( task ) < updateComponents.size() )
            {
                return updateComponents[static_cast<s32>( task )];
            }
        }

        static ConcurrentArray<SmartPtr<IComponent>> emptyArray;
        return emptyArray;
    }

    void GameManager::registerComponentUpdate( TaskId task, Thread::UpdateState state,
                                               SmartPtr<IComponent> component )
    {
        auto &updateObjects = getRegisteredComponents( state, task );
        auto it = std::find( updateObjects.begin(), updateObjects.end(), component );
        if( it == updateObjects.end() )
        {
            updateObjects.push_back( component );
        }

        WP_ASSERT( std::unique( updateObjects.begin(), updateObjects.end() ) == updateObjects.end() );
    }

    void GameManager::unregisterComponentUpdate( TaskId task, Thread::UpdateState state,
                                                 SmartPtr<IComponent> component )
    {
        auto &updateObjects = getRegisteredComponents( state, task );
        updateObjects.erase( std::remove( updateObjects.begin(), updateObjects.end(), component ),
                             updateObjects.end() );
    }

    void GameManager::unregisterAllComponent( SmartPtr<IComponent> component )
    {
        for( u32 i = 0; i < static_cast<u32>( Thread::UpdateState::Count ); ++i )
        {
            for( u32 j = 0; j < static_cast<u32>( TaskId::Count ); ++j )
            {
                unregisterComponentUpdate( static_cast<TaskId>( j ),
                                           static_cast<Thread::UpdateState>( i ), component );
            }
        }
    }

    s32 GameManager::getNumActors() const
    {
        return m_numActors;
    }

    void GameManager::lock()
    {
        m_mutex.lock();
    }

    void GameManager::lock_shared()
    {
        m_mutex.lock_shared();
    }

    bool GameManager::try_lock()
    {
        return m_mutex.try_lock();
    }

    void GameManager::unlock()
    {
        m_mutex.unlock();
    }

    void GameManager::unlock_shared()
    {
        m_mutex.unlock_shared();
    }

    void GameManager::setComponentFactoryIgnoreList( const Array<String> &ignoreList )
    {
        m_componentFactoryIgnoreList = { ignoreList.begin(), ignoreList.end() };
    }

    Parameter GameManager::handleEvent( EventType eventType, hash_type eventValue,
                                        const Array<Parameter> &arguments,
                                        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                                        SmartPtr<IEvent> event )
    {
        if( auto scene = getCurrentScene() )
        {
            return scene->handleEvent( eventType, eventValue, arguments, sender, object, event );
        }

        return {};
    }

    void GameManager::EventListener::setOwner( SmartPtr<GameManager> owner )
    {
        m_owner = owner;
    }

    SmartPtr<GameManager> GameManager::EventListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    Parameter GameManager::EventListener::handleEvent( EventType eventType, hash_type eventValue,
                                                       const Array<Parameter> &arguments,
                                                       SmartPtr<ISharedObject> sender,
                                                       SmartPtr<ISharedObject> object,
                                                       SmartPtr<IEvent> event )
    {
        if( auto owner = getOwner() )
        {
            return owner->handleEvent( eventType, eventValue, arguments, sender, object, event );
        }

        return {};
    }

    GameManager::EventListener::EventListener() = default;

    GameManager::EventListener::~EventListener() = default;
}  // namespace workphone::scene
