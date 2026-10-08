#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Core/DebugTrace.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Jobs/EventJob.hpp>
#include <Workphone/Memory/MemoryTracker.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Interface/Ai/IAiManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/Scene/IGamePrefabManager.hpp>
#include <Workphone/Interface/Database/IDatabase.hpp>
#include <Workphone/Interface/Database/IDatabaseManager.hpp>
#include <Workphone/Interface/System/IFSM.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Interface/IApplication.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IParticleManager.hpp>
#include <Workphone/Interface/Graphics/IVideoManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Input/IInputManager.hpp>
#include <Workphone/Interface/Input/IInputDeviceManager.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/Mesh/IMeshLoader.hpp>
#include <Workphone/Interface/Net/INetworkManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager2D.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene2.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <Workphone/Interface/System/IEditorManager.hpp>
#include <Workphone/Interface/System/ILogManager.hpp>
#include <Workphone/Interface/System/ICommandManager.hpp>
#include <Workphone/Interface/System/IPackageManager.hpp>
#include <Workphone/Interface/System/IPluginManager.hpp>
#include <Workphone/Interface/Procedural/IProceduralManager.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/Script/IScriptManager.hpp>
#include <Workphone/Interface/Sound/ISoundManager.hpp>
#include <Workphone/Interface/System/IAsyncOperation.hpp>
#include <Workphone/Interface/System/IConsole.hpp>
#include <Workphone/Interface/System/IEvent.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <crtdbg.h>

#if defined( _DEBUG )
#    define WP_CHECK_CRT_HEAP( label )                                                                 \
        do                                                                                             \
        {                                                                                              \
            if( !_CrtCheckMemory() )                                                                   \
            {                                                                                          \
                std::fprintf( stderr, "CRT heap damaged at ApplicationManager::unload: %s\n", label ); \
            }                                                                                          \
        } while( false )
#else
#    define WP_CHECK_CRT_HEAP( label ) ( (void)0 )
#endif
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Interface/System/IPlugin.hpp>
#include <Workphone/Interface/System/IProfiler.hpp>
#include <Workphone/Interface/System/IProcessManager.hpp>
#include <Workphone/Interface/System/ISelectionManager.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/System/IStateQueue.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/System/IThreadPool.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>
#include <Workphone/System/StateContext.hpp>
#include <Workphone/Interface/Vehicle/IVehicleManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <algorithm>

namespace workphone::core
{
    namespace
    {
        using UnloadPluginFunction = void ( * )( IApplicationManager * );

        void unloadPluginResources( SmartPtr<IPlugin> plugin, IApplicationManager *applicationManager )
        {
            if( !plugin || !plugin->getLibraryHandle() )
            {
                return;
            }

            auto function =
                reinterpret_cast<UnloadPluginFunction>( plugin->getFunction( "unloadPlugin" ) );
            if( function )
            {
                function( applicationManager );
            }
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone::core, ApplicationManager, IApplicationManager );

    ApplicationManager::ApplicationManager()
    {
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );
    }

    ApplicationManager::~ApplicationManager() = default;

    void ApplicationManager::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            m_settingsCachePath.reserve( WP_MAX_PATH );
            m_cachePath.reserve( WP_MAX_PATH );
            m_projectPath.reserve( WP_MAX_PATH );
            m_projectLibraryName.reserve( WP_MAX_PATH );
            m_mediaPath.reserve( WP_MAX_PATH );
            m_renderMediaPath.reserve( WP_MAX_PATH );

            const auto maxStrSize = 128;
            const auto maxStrings = 4096;

            m_ownedStringPool = std::make_unique<StringPool<c8>>( maxStrSize * maxStrings );
            m_ownedStringPool->setMaxStringSize( 256 );
            setStringPool( m_ownedStringPool.get() );

            const auto maxPropertyNameSize = 24;
            const auto maxPropertyValueSize = 128;
            const auto maxPropertyStrings = 32768;

            m_ownedPropertyNamePool =
                std::make_unique<StringPool<c8>>( maxPropertyStrings, maxPropertyNameSize );
            setPropertyNamePool( m_ownedPropertyNamePool.get() );

            m_ownedPropertyValuePool =
                std::make_unique<StringPool<c8>>( maxPropertyStrings, maxPropertyValueSize );
            setPropertyValuePool( m_ownedPropertyValuePool.get() );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ApplicationManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            std::fprintf( stderr, "TRACE ApplicationManager unload start\n" );
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                setQuit( true );
                setRunning( false );
                clearAllEvents();

                setPlayerSettings( nullptr );
                setEditorSettings( nullptr );

                setApplication( nullptr );
                removeObjectListeners();

                if( auto stateManager = getStateManager() )
                {
                    auto stateContexts = stateManager->getStateContexts();
                    for( auto &stateContext : stateContexts )
                    {
                        if( auto context =
                                workphone::dynamic_pointer_cast<StateContext>( stateContext ) )
                        {
                            for( u32 task = 0; task < static_cast<u32>( TaskId::Count ); ++task )
                            {
                                if( auto stateQueue = context->getStateQueue( task ) )
                                {
                                    stateQueue->clear();
                                }
                            }
                        }
                    }
                }

                if( auto threadPool = getThreadPool() )
                {
                    threadPool->unload( nullptr );
                    setThreadPool( nullptr );
                }

                if( auto prefabManager = getPrefabManager() )
                {
                    prefabManager->unload( nullptr );
                    setPrefabManager( nullptr );
                }

                if( auto gameManager = getGameManager() )
                {
                    gameManager->unload( nullptr );
                    setGameManager( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager game manager done\n" );
                WP_CHECK_CRT_HEAP( "game manager" );

                // Unload the physics manager before the state manager so that physics shapes
                // can properly unregister their state listeners and state contexts (via
                // destroyStateObject) while those objects are still alive. Unloading the state
                // manager first causes the ShapeStateListeners to be freed while the physics
                // shapes still hold stale handle-based weak references to them; when those
                // handles are later resolved the engine's pool allocator may have reused the
                // slot for a different object, producing an ABA-type use-after-free crash.
                if( auto physicsManager = getPhysicsManager() )
                {
                    physicsManager->unload( nullptr );
                    setPhysicsManager( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager physics manager done\n" );
                WP_CHECK_CRT_HEAP( "physics manager" );

                if( auto jobQueue = getJobQueue() )
                {
                    jobQueue->unload( nullptr );
                    setJobQueue( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager job queue done\n" );

                if( auto taskManager = getTaskManager() )
                {
                    taskManager->unload( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager task manager done\n" );
                WP_CHECK_CRT_HEAP( "task manager" );

                // These windows are created by the UI factory. Release their handles while
                // that factory and its shared-object destruction listener are still alive.
                if( auto window = getWindow() )
                {
                    setWindow( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager window done\n" );

                if( auto window = getSceneRenderWindow() )
                {
                    setSceneRenderWindow( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager scene window done\n" );

                // UI elements and listeners can own script objects. Release them before closing
                // the script manager so their unload paths still have a valid scripting runtime.
                if( auto ui = getUI() )
                {
                    ui->unload( nullptr );
                    setUI( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager ui done\n" );
                WP_CHECK_CRT_HEAP( "ui" );

                if( auto ui = getRenderUI() )
                {
                    ui->unload( nullptr );
                    setRenderUI( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager render ui done\n" );

                if( auto packageManager = getPackageManager() )
                {
                    packageManager->unload( nullptr );
                    setPackageManager( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager package manager done\n" );

                if( auto networkManager = getNetworkManager() )
                {
                    networkManager->unload( nullptr );
                    setNetworkManager( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager network manager done\n" );

                if( auto cameraManager = getCameraManager() )
                {
                    cameraManager->unload( nullptr );
                    setCameraManager( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager camera manager done\n" );

                if( auto videoManager = getVideoManager() )
                {
                    videoManager->unload( nullptr );
                    setVideoManager( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager video manager done\n" );

                if( auto aiManager = getAiManager() )
                {
                    aiManager->unload( nullptr );
                    setAiManager( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager ai manager done\n" );

                if( auto meshLoader = getMeshLoader() )
                {
                    meshLoader->unload( nullptr );
                    setMeshLoader( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager mesh loader done\n" );

                if( auto input = getInput() )
                {
                    input->unload( nullptr );
                    setInput( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager input done\n" );

                if( auto inputDeviceManager = getInputDeviceManager() )
                {
                    inputDeviceManager->unload( nullptr );
                    setInputDeviceManager( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager input device manager done\n" );

                if( auto selectionManager = getSelectionManager() )
                {
                    selectionManager->unload( nullptr );
                    setSelectionManager( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager selection manager done\n" );

                if( auto database = getDatabase() )
                {
                    database->unload( nullptr );
                    setDatabase( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager database done\n" );

                if( auto proceduralManager = getProceduralManager() )
                {
                    proceduralManager->unload( nullptr );
                    setProceduralManager( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager procedural manager done\n" );

                if( auto commandManager = getCommandManager() )
                {
                    commandManager->unload( nullptr );
                    setCommandManager( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager command manager done\n" );

                if( auto resourceDatabase = getResourceDatabase() )
                {
                    resourceDatabase->unload( nullptr );
                    setResourceDatabase( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager resource database done\n" );

                if( auto resourceManager = getResourceManager() )
                {
                    resourceManager->unload( nullptr );
                    setResourceManager( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager resource manager done\n" );

                if( auto processManager = getProcessManager() )
                {
                    processManager->unload( nullptr );
                    setProcessManager( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager process manager done\n" );

                if( auto meshLoader = getMeshLoader() )
                {
                    meshLoader->unload( nullptr );
                    meshLoader = nullptr;
                }

                if( auto meshManager = getMeshManager() )
                {
                    meshManager->unload( nullptr );
                    setMeshManager( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager mesh manager done\n" );

                if( auto vehicleManager = getVehicleManager() )
                {
                    vehicleManager->unload( nullptr );
                    setVehicleManager( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager vehicle manager done\n" );

                if( auto physicsManager2 = getPhysicsManager2D() )
                {
                    physicsManager2->unload( nullptr );
                    setPhysicsManager2D( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager physics2 manager done\n" );

                if( auto physicsManager = getPhysicsManager() )
                {
                    // Physics manager was already unloaded early (before the state manager).
                    // This guard is a no-op in normal teardown but protects against any path
                    // that skips the early unload.
                    physicsManager->unload( nullptr );
                    setPhysicsManager( nullptr );
                }

                if( auto soundManager = getSoundManager() )
                {
                    soundManager->unload( nullptr );
                    setSoundManager( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager sound manager done\n" );

                if( auto graphicsSystem = getGraphicsSystem() )
                {
                    graphicsSystem->unload( nullptr );
                    setGraphicsSystem( nullptr );
                }
                std::fprintf( stderr, "TRACE ApplicationManager graphics system done\n" );
                WP_CHECK_CRT_HEAP( "graphics system" );

                if( auto fileSystem = getFileSystem() )
                {
                    fileSystem->unload( nullptr );
                    setFileSystem( nullptr );
                }

                if( auto threadPool = getThreadPool() )
                {
                    threadPool->unload( nullptr );
                    setThreadPool( nullptr );
                }

                if( auto stateManager = getStateManager() )
                {
                    stateManager->unload( nullptr );
                    setStateManager( nullptr );
                }
                WP_CHECK_CRT_HEAP( "state manager" );

                if( auto logManager = getLogManager() )
                {
                    logManager->unload( nullptr );
                    setLogManager( nullptr );
                }

                if( auto timer = getTimer() )
                {
                    timer->unload( nullptr );
                    setTimer( nullptr );
                }

                if( auto profiler = getProfiler() )
                {
                    profiler->unload( nullptr );
                    setProfiler( nullptr );
                }

                if( auto fsmManager = getFsmManager() )
                {
                    fsmManager->unload( nullptr );
                    setFsmManager( nullptr );
                }

                // FSM listeners can hold weak references to the task manager.
                // Keep its storage alive until the listeners have been released.
                setTaskManager( nullptr );

                if( auto scriptManager = getScriptManager() )
                {
                    scriptManager->unload( nullptr );
                    setScriptManager( nullptr );
                }

                Array<SmartPtr<IPlugin>> pluginsToUnload;
                if( auto pluginManager = getPluginManager() )
                {
                    auto plugins = pluginManager->getPlugins();
                    pluginsToUnload.reserve( plugins.size() );

                    for( auto &pluginObject : plugins )
                    {
                        if( auto plugin = workphone::dynamic_pointer_cast<IPlugin>( pluginObject ) )
                        {
                            unloadPluginResources( plugin, this );
                            pluginsToUnload.push_back( plugin );
                        }
                    }

                    setPluginManager( nullptr );
                }

                auto loadedPlugins = m_plugins.snapshot();
                for( size_t i = loadedPlugins.size(); i > 0; --i )
                {
                    if( auto plugin = loadedPlugins[i - 1] )
                    {
                        plugin->unload( nullptr );
                    }
                }
                loadedPlugins.clear();
                m_plugins.clear();

                for( auto &plugin : pluginsToUnload )
                {
                    if( plugin )
                    {
                        plugin->unload( nullptr );
                    }
                }
                pluginsToUnload.clear();
                std::fprintf( stderr, "TRACE ApplicationManager plugins unloaded\n" );

                if( auto factoryManager = getFactoryManager() )
                {
                    factoryManager->unload( nullptr );
                    setFactoryManager( nullptr );
                }
                WP_CHECK_CRT_HEAP( "factory manager" );
                std::fprintf( stderr, "TRACE ApplicationManager managers unloaded\n" );

                setStringPool( nullptr );
                setPropertyNamePool( nullptr );
                setPropertyValuePool( nullptr );

                m_ownedStringPool.reset();
                m_ownedPropertyNamePool.reset();
                m_ownedPropertyValuePool.reset();

                setLoadingState( LoadingState::Unloaded );
                std::fprintf( stderr, "TRACE ApplicationManager unload end\n" );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ApplicationManager::setEditorSettings( SmartPtr<Properties> properties )
    {
        m_editorSettings = properties;
    }

    auto ApplicationManager::getPlayerSettings() const -> SmartPtr<Properties>
    {
        return m_playerSettings;
    }

    void ApplicationManager::setPlayerSettings( SmartPtr<Properties> properties )
    {
        m_playerSettings = properties;
    }

    auto ApplicationManager::getLogManager() const -> SmartPtr<ILogManager>
    {
        return m_logManager;
    }

    void ApplicationManager::setLogManager( SmartPtr<ILogManager> logManager )
    {
        m_logManager = logManager;
    }

    void ApplicationManager::setFactoryManager( SmartPtr<IFactoryManager> factoryManager )
    {
        m_factoryManagers[(u32)TaskId::Primary] = factoryManager;
    }

    auto ApplicationManager::getProcessManager() const -> SmartPtr<IProcessManager>
    {
        return m_processManager;
    }

    void ApplicationManager::setProcessManager( SmartPtr<IProcessManager> processManager )
    {
        m_processManager = processManager;
    }

    auto ApplicationManager::getApplication() const -> SmartPtr<IApplication>
    {
        auto p = m_application.load();
        return p.lock();
    }

    void ApplicationManager::setApplication( SmartPtr<IApplication> application )
    {
        m_application = application;
    }

    void ApplicationManager::setFileSystem( SmartPtr<IFileSystem> fileSystem )
    {
        m_fileSystem = fileSystem;
    }

    void ApplicationManager::setTimer( SmartPtr<ITimer> timer )
    {
        m_timer = timer;
    }

    void ApplicationManager::setFsmManager( SmartPtr<IFSMManager> fsmManager )
    {
        m_fsmManager = fsmManager;
    }

    auto ApplicationManager::getFsmManagerByTask( TaskId task ) const -> SmartPtr<IFSMManager>
    {
        return m_fsmManagers[static_cast<u32>( task )];
    }

    void ApplicationManager::setFsmManagerByTask( TaskId task, SmartPtr<IFSMManager> fsmManager )
    {
        auto iTask = static_cast<u32>( task );
        m_fsmManagers[iTask] = fsmManager;
    }

    auto ApplicationManager::getProceduralManager() const -> SmartPtr<procedural::IProceduralManager>
    {
        return m_proceduralManager;
    }

    void ApplicationManager::setProceduralManager(
        SmartPtr<procedural::IProceduralManager> proceduralManager )
    {
        m_proceduralManager = proceduralManager;
    }

    void ApplicationManager::setPhysicsManager2D( SmartPtr<physics::IPhysicsManager2D> physicsManager )
    {
        m_physicsManager2 = physicsManager;
    }

    auto ApplicationManager::getPhysicsManager2D() const -> SmartPtr<physics::IPhysicsManager2D>
    {
        return m_physicsManager2;
    }

    void ApplicationManager::setPhysicsManager( SmartPtr<physics::IPhysicsManager> physicsManager )
    {
        m_physicsManager3 = physicsManager;
    }

    auto ApplicationManager::getPhysicsManager() const -> SmartPtr<physics::IPhysicsManager>
    {
        return m_physicsManager3;
    }

    physics::IPhysicsManager *ApplicationManager::getPhysicsManagerPtr() const
    {
        return m_physicsManager3.get();
    }

    auto ApplicationManager::isEditor() const -> bool
    {
        return m_isEditor;
    }

    void ApplicationManager::setEditor( bool editor )
    {
        m_isEditor = editor;
    }

    auto ApplicationManager::isEditorCamera() const -> bool
    {
        return m_isEditorCamera;
    }

    void ApplicationManager::setEditorCamera( bool editorCamera )
    {
        m_isEditorCamera = editorCamera;
    }

    auto ApplicationManager::isPlaying() const -> bool
    {
        return m_isPlaying;
    }

    void ApplicationManager::setPlaying( bool playing )
    {
        m_isPlaying = playing;
    }

    auto ApplicationManager::isPaused() const -> bool
    {
        return m_isPaused;
    }

    void ApplicationManager::setPaused( bool paused )
    {
        m_isPaused = paused;
    }

    auto ApplicationManager::isRunning() const -> bool
    {
        return m_isRunning;
    }

    void ApplicationManager::setRunning( bool running )
    {
        m_isRunning = running;
    }

    auto ApplicationManager::getQuit() const -> bool
    {
        return m_quit;
    }

    void ApplicationManager::setQuit( bool quit )
    {
        m_quit = quit;
    }

    auto ApplicationManager::hasTasks() const -> bool
    {
        if( const auto taskManager = getTaskManager() )
        {
            return taskManager->getNumTasks() > 0;
        }

        return false;
    }

    auto ApplicationManager::getFSMPtr() const -> IFSM *
    {
        return m_fsm.get();
    }

    auto ApplicationManager::getFSM() const -> SmartPtr<IFSM>
    {
        return m_fsm;
    }

    void ApplicationManager::setFSM( SmartPtr<IFSM> fsm )
    {
        m_fsm = fsm;
    }

    auto ApplicationManager::getEditorSettings() const -> SmartPtr<Properties>
    {
        return m_editorSettings;
    }

    void ApplicationManager::setGraphicsSystem( SmartPtr<render::IGraphicsSystem> graphicsSystem )
    {
        m_graphicsSystem = graphicsSystem;
    }

    auto ApplicationManager::getVideoManager() const -> SmartPtr<render::IVideoManager>
    {
        return m_videoManager;
    }

    void ApplicationManager::setVideoManager( SmartPtr<render::IVideoManager> videoManager )
    {
        m_videoManager = videoManager;
    }

    auto ApplicationManager::getTaskManager() const -> SmartPtr<ITaskManager>
    {
        return m_taskManager;
    }

    void ApplicationManager::setTaskManager( SmartPtr<ITaskManager> taskManager )
    {
        m_taskManager = taskManager;
    }

    auto ApplicationManager::isPauseMenuActive() const -> bool
    {
        return m_isPauseMenuActive;
    }

    void ApplicationManager::setPauseMenuActive( bool pauseMenuActive )
    {
        m_isPauseMenuActive = pauseMenuActive;
    }

    bool ApplicationManager::isSceneLoading() const
    {
        return m_sceneLoading;
    }

    void ApplicationManager::setSceneLoading( bool loading )
    {
        m_sceneLoading = loading;
    }

    IProfiler *ApplicationManager::getProfilerPtr() const
    {
        return m_profiler.get();
    }

    auto ApplicationManager::getProfiler() const -> SmartPtr<IProfiler>
    {
        return m_profiler;
    }

    void ApplicationManager::setProfiler( SmartPtr<IProfiler> profiler )
    {
        m_profiler = profiler;
    }

    void ApplicationManager::setJobQueue( SmartPtr<IJobQueue> jobQueue )
    {
        m_jobQueue = jobQueue;
    }

    auto ApplicationManager::getParticleManager() const -> SmartPtr<render::IParticleManager>
    {
        return m_particleManager;
    }

    void ApplicationManager::setParticleManager( SmartPtr<render::IParticleManager> particleManager )
    {
        m_particleManager = particleManager;
    }

    void ApplicationManager::setScriptManager( SmartPtr<IScriptManager> scriptManager )
    {
        m_scriptManager = scriptManager;
    }

    auto ApplicationManager::getScriptManager() const -> SmartPtr<IScriptManager>
    {
        return m_scriptManager;
    }

    auto ApplicationManager::getInput() const -> SmartPtr<IInputManager>
    {
        return m_input;
    }

    void ApplicationManager::setInput( SmartPtr<IInputManager> input )
    {
        m_input = input;
    }

    void ApplicationManager::setInputDeviceManager( SmartPtr<IInputDeviceManager> inputManager )
    {
        m_inputManager = inputManager;
    }

    auto ApplicationManager::getInputDeviceManager() const -> SmartPtr<IInputDeviceManager>
    {
        return m_inputManager;
    }

    void ApplicationManager::setThreadPool( SmartPtr<IThreadPool> threadPool )
    {
        m_threadPool = threadPool;
    }

    void ApplicationManager::setConsole( SmartPtr<IConsole> console )
    {
        m_console = console;
    }

    auto ApplicationManager::getConsole() const -> SmartPtr<IConsole>
    {
        return m_console;
    }

    auto ApplicationManager::getCameraManager() const -> SmartPtr<scene::ICameraManager>
    {
        return m_cameraManager;
    }

    void ApplicationManager::setCameraManager( SmartPtr<scene::ICameraManager> cameraManager )
    {
        m_cameraManager = cameraManager;
    }

    void ApplicationManager::setStateManager( SmartPtr<IStateManager> stateManager )
    {
        m_stateManager = stateManager;
    }

    auto ApplicationManager::getStateManager() const -> SmartPtr<IStateManager>
    {
        return m_stateManager;
    }

    void ApplicationManager::setCommandManager( SmartPtr<ICommandManager> commandManager )
    {
        m_commandManager = commandManager;
    }

    ICommandManager *ApplicationManager::getCommandManagerPtr() const
    {
        return m_commandManager.get();
    }

    auto ApplicationManager::getCommandManager() const -> SmartPtr<ICommandManager>
    {
        return m_commandManager;
    }

    void ApplicationManager::setPrefabManager( SmartPtr<scene::IGamePrefabManager> prefabManager )
    {
        m_prefabManager = prefabManager;
    }

    auto ApplicationManager::getPrefabManager() const -> SmartPtr<scene::IGamePrefabManager>
    {
        return m_prefabManager;
    }

    auto ApplicationManager::getMeshLoader() const -> SmartPtr<IMeshLoader>
    {
        return m_meshLoader;
    }

    void ApplicationManager::setMeshLoader( SmartPtr<IMeshLoader> meshLoader )
    {
        m_meshLoader = meshLoader;
    }

    auto ApplicationManager::getResourceDatabase() const -> SmartPtr<IResourceDatabase>
    {
        return m_resourceDatabase;
    }

    void ApplicationManager::setResourceDatabase( SmartPtr<IResourceDatabase> resourceDatabase )
    {
        m_resourceDatabase = resourceDatabase;
    }

    void ApplicationManager::setGameManager( SmartPtr<scene::IGameManager> gameManager )
    {
        m_gameManager = gameManager;
    }

    auto ApplicationManager::getGameManager() const -> SmartPtr<scene::IGameManager>
    {
        return m_gameManager;
    }

    void ApplicationManager::setSelectionManager( SmartPtr<ISelectionManager> selectionManager )
    {
        m_selectionManager = selectionManager;
    }

    void ApplicationManager::setResourceManager( SmartPtr<IResourceManager> resourceManager )
    {
        m_resourceManager = resourceManager;
    }

    auto ApplicationManager::getResourceManager() const -> SmartPtr<IResourceManager>
    {
        return m_resourceManager;
    }

    ISelectionManager *ApplicationManager::getSelectionManagerPtr() const
    {
        return m_selectionManager.get();
    }

    auto ApplicationManager::getSelectionManager() const -> SmartPtr<ISelectionManager>
    {
        return m_selectionManager;
    }

    ISoundManager *ApplicationManager::getSoundManagerPtr() const
    {
        return m_soundManager.get();
    }

    auto ApplicationManager::getSoundManager() const -> SmartPtr<ISoundManager>
    {
        return m_soundManager;
    }

    void ApplicationManager::setSoundManager( SmartPtr<ISoundManager> soundManager )
    {
        m_soundManager = soundManager;
    }

    auto ApplicationManager::getEnableRenderer() const -> bool
    {
        return m_enableRenderer;
    }

    void ApplicationManager::setEnableRenderer( bool enable )
    {
        m_enableRenderer = enable;
    }

    auto ApplicationManager::getUI() const -> SmartPtr<ui::IUIManager>
    {
        return m_ui;
    }

    void ApplicationManager::setUI( SmartPtr<ui::IUIManager> ui )
    {
        m_ui = ui;
    }

    auto ApplicationManager::getRenderUI() const -> SmartPtr<ui::IUIManager>
    {
        return m_renderUI;
    }

    void ApplicationManager::setRenderUI( SmartPtr<ui::IUIManager> renderUI )
    {
        m_renderUI = renderUI;
    }

    auto ApplicationManager::getDatabase() const -> SmartPtr<IDatabaseManager>
    {
        return m_database;
    }

    void ApplicationManager::setDatabase( SmartPtr<IDatabaseManager> database )
    {
        m_database = database;
    }

    auto ApplicationManager::getProperties() const -> SmartPtr<Properties>
    {
        return m_properties;
    }

    void ApplicationManager::setProperties( SmartPtr<Properties> properties )
    {
        m_properties = properties;
    }

    auto ApplicationManager::getVehicleManager() const -> SmartPtr<vehicle::IVehicleManager>
    {
        return m_vehicleManager;
    }

    void ApplicationManager::setVehicleManager( SmartPtr<vehicle::IVehicleManager> vehicleManager )
    {
        m_vehicleManager = vehicleManager;
    }

    auto ApplicationManager::getCachePath() const -> String
    {
        ScopedLock lock( this );
        return m_cachePath;
    }

    void ApplicationManager::setCachePath( const String &cachePath )
    {
        ScopedLock lock( this );
        m_cachePath = cachePath;
    }

    auto ApplicationManager::getProjectPath() const -> String
    {
        ScopedLock lock( this );
        return m_projectPath;
    }

    void ApplicationManager::setProjectPath( const String &projectPath )
    {
        ScopedLock lock( this );
        m_projectPath = projectPath;
    }

    auto ApplicationManager::getProjectLibraryName() const -> String
    {
        ScopedLock lock( this );
        return m_projectLibraryName;
    }

    void ApplicationManager::setProjectLibraryName( const String &projectLibraryName )
    {
        ScopedLock lock( this );
        m_projectLibraryName = projectLibraryName;
    }

    void ApplicationManager::setSettingsPath( const String &settingsCachePath )
    {
        ScopedLock lock( this );
        m_settingsCachePath = settingsCachePath;
    }

    auto ApplicationManager::getBuildConfig() const -> String
    {
#if _DEBUG
        return String( "Debug" );
#else
        return String( "Release" );
#endif
    }

    auto ApplicationManager::getProjectLibraryExtension() const -> String
    {
        return ".dll";
        // return ".lib";
    }

    auto ApplicationManager::getProjectLibraryPath() const -> String
    {
        const auto cachePath = getCachePath();
        const auto libraryName = getProjectLibraryName();
        const auto libraryExt = getProjectLibraryExtension();
        const auto config = getBuildConfig();

        return cachePath + "/project/" + config + "/" + libraryName + libraryExt;
    }

    auto ApplicationManager::getMediaPath() const -> String
    {
        ScopedLock lock( this );
        return m_mediaPath;
    }

    void ApplicationManager::setMediaPath( const String &mediaPath )
    {
        WP_ASSERT( !( Path::isPathAbsolute( mediaPath ) == true ) );

        ScopedLock lock( this );
        m_mediaPath = mediaPath;
    }

    auto ApplicationManager::getRenderMediaPath() const -> String
    {
        ScopedLock lock( this );
        return m_renderMediaPath;
    }

    void ApplicationManager::setRenderMediaPath( const String &renderMediaPath )
    {
        WP_ASSERT( !( Path::isPathAbsolute( renderMediaPath ) == true ) );

        ScopedLock lock( this );
        m_renderMediaPath = renderMediaPath;
    }

    auto ApplicationManager::getSettingsPath() const -> String
    {
        ScopedLock lock( this );
        return m_settingsCachePath;
    }

    auto ApplicationManager::getStateTask() const -> TaskId
    {
        return hasTasks() ? TaskId::Application : TaskId::Primary;
    }

    auto ApplicationManager::getApplicationTask() const -> TaskId
    {
        if( auto taskManager = getTaskManager() )
        {
            if( auto task = taskManager->getTask( TaskId::Application ) )
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

        return hasTasks() ? TaskId::Application : TaskId::Primary;
    }

    auto ApplicationManager::getMeshManager() const -> SmartPtr<IResourceManager>
    {
        return m_meshManager;
    }

    void ApplicationManager::setMeshManager( SmartPtr<IResourceManager> meshManager )
    {
        m_meshManager = meshManager;
    }

    auto ApplicationManager::getLoadProgress() const -> s32
    {
        return m_loadProgress;
    }

    void ApplicationManager::setLoadProgress( s32 loadProgress )
    {
        m_loadProgress = loadProgress;
    }

    void ApplicationManager::addLoadProgress( s32 loadProgress )
    {
        m_loadProgress += loadProgress;
    }

    auto ApplicationManager::getActors() const -> Array<SmartPtr<scene::IGameActor>>
    {
        WP_ASSERT( isValid() );

        if( auto sceneManager = getGameManager() )
        {
            WP_ASSERT( sceneManager->isValid() );

            if( auto scene = sceneManager->getCurrentScene() )
            {
                WP_ASSERT( scene->isValid() );
                return scene->getActors();
            }
        }

        return {};
    }

    auto ApplicationManager::getPluginManager() const -> SmartPtr<IPluginManager>
    {
        return m_pluginManager;
    }

    void ApplicationManager::setPluginManager( SmartPtr<IPluginManager> pluginManager )
    {
        m_pluginManager = pluginManager;
    }

    void ApplicationManager::addPlugin( SmartPtr<ISharedObject> plugin )
    {
        WP_DEBUG_TRACE;

        WP_ASSERT( plugin );

        if( plugin )
        {
            plugin->load( nullptr );
            m_plugins.push_back( plugin );
        }
    }

    void ApplicationManager::removePlugin( SmartPtr<ISharedObject> plugin )
    {
        WP_DEBUG_TRACE;
        m_plugins.erase( std::remove( m_plugins.begin(), m_plugins.end(), plugin ), m_plugins.end() );
    }

    render::IGraphicsWindow *ApplicationManager::getWindowPtr() const
    {
        return m_window.get();
    }

    auto ApplicationManager::getWindow() const -> SmartPtr<render::IGraphicsWindow>
    {
        return m_window;
    }

    void ApplicationManager::setWindow( SmartPtr<render::IGraphicsWindow> window )
    {
        m_window = window;
    }

    ui::IUIWindow *ApplicationManager::getSceneRenderWindowPtr() const
    {
        return m_sceneRenderWindow.get();
    }

    auto ApplicationManager::getSceneRenderWindow() const -> SmartPtr<ui::IUIWindow>
    {
        return m_sceneRenderWindow;
    }

    void ApplicationManager::setSceneRenderWindow( SmartPtr<ui::IUIWindow> sceneRenderWindow )
    {
        m_sceneRenderWindow = sceneRenderWindow;
    }

    auto ApplicationManager::getActorComponent( SmartPtr<scene::IGameActor> actor, u32 typeId ) const
        -> SmartPtr<scene::IComponent>
    {
        const auto typeManager = TypeManager::instance();

        auto components = actor->getComponents();
        for( auto &component : components )
        {
            if( component )
            {
                auto componentTypeId = component->getTypeInfo();
                if( typeManager->isDerived( componentTypeId, typeId ) )
                {
                    return component;
                }
            }
        }

        auto children = actor->getChildren();
        for( auto &child : children )
        {
            auto component = getActorComponent( child, typeId );
            if( component )
            {
                return component;
            }
        }

        return nullptr;
    }

    scene::IComponent *ApplicationManager::getActorComponentPtr( scene::IGameActor *actor,
                                                                 u32 typeId ) const
    {
        const auto typeManager = TypeManager::instance();

        auto components = actor->getComponents();
        for( auto &component : components )
        {
            if( component )
            {
                auto componentTypeId = component->getTypeInfo();
                if( typeManager->isDerived( componentTypeId, typeId ) )
                {
                    return component.get();
                }
            }
        }

        auto children = actor->getChildren();
        for( auto &child : children )
        {
            auto component = getActorComponentPtr( child.get(), typeId );
            if( component )
            {
                return component;
            }
        }

        return nullptr;
    }

    auto ApplicationManager::getComponentByType( u32 typeId ) const -> SmartPtr<scene::IComponent>
    {
        auto actors = getActors();
        for( auto &actor : actors )
        {
            auto result = getActorComponent( actor, typeId );
            if( result )
            {
                return result;
            }
        }

        return nullptr;
    }

    scene::IComponent *ApplicationManager::getComponentPtrByType( u32 typeId ) const
    {
        auto actors = getActors();
        for( auto &actor : actors )
        {
            auto result = getActorComponentPtr( actor.get(), typeId );
            if( result )
            {
                return result;
            }
        }

        return nullptr;
    }

    auto ApplicationManager::triggerEvent( EventType eventType, hash_type eventValue,
                                           const Array<Parameter> &arguments,
                                           SmartPtr<ISharedObject> sender,
                                           SmartPtr<ISharedObject> object, SmartPtr<IEvent> event,
                                           bool sendNow, u32 taskFlags ) -> Parameter
    {
        auto factoryManager = getFactoryManagerPtr();
        if( !factoryManager )
        {
            return {};
        }

        auto eventJob = factoryManager->make_ptr<EventJob>();
        if( !eventJob )
        {
            WP_LOG_ERROR( "ApplicationManager::triggerEvent: failed to create EventJob." );
            return {};
        }

        eventJob->setEventType( eventType );
        eventJob->setEventValue( eventValue );
        eventJob->setArguments( arguments );
        eventJob->setSender( sender );
        eventJob->setObject( object );
        eventJob->setEvent( event );

        auto threadPool = getThreadPool();
        if( threadPool && threadPool->getNumThreads() > 0 )
        {
            if( !sendNow )
            {
                if( auto jobQueue = getJobQueuePtr() )
                {
                    if( !jobQueue->isRunning() )
                    {
                        // Match the no-queue behavior below. A temporarily stopped queue must not
                        // silently discard an event that the caller expects to be delivered.
                        eventJob->execute();
                    }
                    else if( taskFlags == std::numeric_limits<u32>::max() )
                    {
                        jobQueue->addJobAllTasks( eventJob );
                    }
                    else
                    {
                        struct TaskRoute
                        {
                            u32 flag;
                            TaskId task;
                        };

                        // Thread flags are deliberately not TaskId values. Keep the translation in
                        // one place and support every task flag exposed by Thread.
                        const TaskRoute routes[] = {
                            { Thread::Primary_Flag, TaskId::Primary },
                            { Thread::Ai_Flag, TaskId::Ai },
                            { Thread::Animation_Flag, TaskId::Animation },
                            { Thread::Application_Flag, TaskId::Application },
                            { Thread::Collision_Flag, TaskId::Collision },
                            { Thread::Controls_Flag, TaskId::Controls },
                            { Thread::Dynamics_Flag, TaskId::Dynamics },
                            { Thread::GarbageCollect_Flag, TaskId::GarbageCollect },
                            { Thread::Input_Flag, TaskId::Input },
                            { Thread::Physics_Flag, TaskId::Physics },
                            { Thread::None_Flag, TaskId::None },
                            { Thread::Render_Flag, TaskId::Render },
                            { Thread::Sound_Flag, TaskId::Sound }
                        };

                        u32 supportedFlags = 0;
                        for( const auto &route : routes )
                        {
                            supportedFlags |= route.flag;
                        }

                        if( taskFlags == 0 || ( taskFlags & ~supportedFlags ) != 0 )
                        {
                            WP_LOG_ERROR(
                                "ApplicationManager::triggerEvent: invalid task flag mask; routing "
                                "event to the application task." );
                            jobQueue->addJob( eventJob, TaskId::Application );
                        }
                        else
                        {
                            for( const auto &route : routes )
                            {
                                if( ( taskFlags & route.flag ) != 0 )
                                {
                                    jobQueue->addJob( eventJob, route.task );
                                }
                            }
                        }
                    }
                }
                else
                {
                    eventJob->execute();
                }
            }
            else
            {
                eventJob->execute();
            }
        }
        else
        {
            eventJob->execute();
            eventJob = nullptr;
        }

        return {};
    }

    void ApplicationManager::clearAllEvents()
    {
        if( auto jobQueue = getJobQueue() )
        {
            jobQueue->clearEventJobs();
        }

        if( auto taskManager = getTaskManager() )
        {
            taskManager->clearEventJobs();
        }
    }

    SmartPtr<INetworkManager> ApplicationManager::getNetworkManager() const
    {
        return m_networkManager;
    }

    void ApplicationManager::setNetworkManager( SmartPtr<INetworkManager> networkManager )
    {
        m_networkManager = networkManager;
    }

    SmartPtr<IPackageManager> ApplicationManager::getPackageManager() const
    {
        return m_packageManager;
    }

    void ApplicationManager::setPackageManager( SmartPtr<IPackageManager> packageManager )
    {
        m_packageManager = packageManager;
    }

    auto ApplicationManager::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        Array<SmartPtr<ISharedObject>> objects;
        objects.reserve( 32 );

        objects.emplace_back( getApplication() );
        objects.emplace_back( getTaskManager() );
        objects.emplace_back( getStateManager() );
        objects.emplace_back( getCommandManager() );
        objects.emplace_back( getDatabase() );
        objects.emplace_back( getFileSystem() );
        objects.emplace_back( getGraphicsSystem() );
        objects.emplace_back( getCameraManager() );
        return objects;
    }

    SmartPtr<ITimer> ApplicationManager::getTimer()
    {
        return m_timer;
    }

    SmartPtr<IFSMManager> ApplicationManager::getFsmManager() const
    {
        return m_fsmManager;
    }

    IThreadPool *ApplicationManager::getThreadPoolPtr() const
    {
        return m_threadPool.get();
    }

    SmartPtr<IThreadPool> ApplicationManager::getThreadPool() const
    {
        return m_threadPool;
    }

    SmartPtr<IFactoryManager> ApplicationManager::getFactoryManager() const
    {
        return m_factoryManagers[(u32)TaskId::Primary];
    }

    IFileSystem *ApplicationManager::getFileSystemPtr() const
    {
        return m_fileSystem.get();
    }

    SmartPtr<IFileSystem> ApplicationManager::getFileSystem() const
    {
        return m_fileSystem;
    }

    SmartPtr<IJobQueue> ApplicationManager::getJobQueue() const
    {
        return m_jobQueue;
    }

    void ApplicationManager::lock()
    {
        m_mutex.lock();
    }

    bool ApplicationManager::try_lock()
    {
        return m_mutex.try_lock();
    }

    void ApplicationManager::unlock()
    {
        m_mutex.unlock();
    }

    void ApplicationManager::setEditorManager( SmartPtr<IEditorManager> editorManager )
    {
        m_editorManager = editorManager;
    }

    SmartPtr<IEditorManager> ApplicationManager::getEditorManager() const
    {
        return m_editorManager;
    }

    IEditorManager *ApplicationManager::getEditorManagerPtr() const
    {
        return m_editorManager.get();
    }

    void ApplicationManager::setAiManager( SmartPtr<IAiManager> aiManager )
    {
        m_aiManager = aiManager;
    }

    SmartPtr<IAiManager> ApplicationManager::getAiManager() const
    {
        return m_aiManager;
    }

    SmartPtr<render::IGraphicsSystem> ApplicationManager::getGraphicsSystem() const
    {
        return m_graphicsSystem;
    }

    void ApplicationManager::setPropertyValuePool( StringPool<c8> *pool )
    {
        m_propertyValuePool = pool;
    }

    StringPool<c8> *ApplicationManager::getPropertyValuePool() const
    {
        return m_propertyValuePool;
    }

    void ApplicationManager::setPropertyNamePool( StringPool<c8> *pool )
    {
        m_propertyNamePool = pool;
    }

    StringPool<c8> *ApplicationManager::getPropertyNamePool() const
    {
        return m_propertyNamePool;
    }

    void ApplicationManager::setStringPoolW( StringPool<wchar_t> *pool )
    {
        m_stringPoolW = pool;
    }

    StringPool<wchar_t> *ApplicationManager::getStringPoolW() const
    {
        return m_stringPoolW;
    }

    void ApplicationManager::setStringPool( StringPool<c8> *pool )
    {
        m_stringPool = pool;
    }

    StringPool<c8> *ApplicationManager::getStringPool() const
    {
        return m_stringPool;
    }

    ui::IUIManager *ApplicationManager::getRenderUIPtr() const
    {
        return m_renderUI.get();
    }

    ui::IUIManager *ApplicationManager::getUIPtr() const
    {
        return m_ui.get();
    }

    scene::IGameManager *ApplicationManager::getGameManagerPtr() const
    {
        return m_gameManager.get();
    }

    IResourceDatabase *ApplicationManager::getResourceDatabasePtr() const
    {
        return m_resourceDatabase.get();
    }

    IFSMManager *ApplicationManager::getFsmManagerPtr() const
    {
        return m_fsmManager.get();
    }

    IScriptManager *ApplicationManager::getScriptManagerPtr() const
    {
        return m_scriptManager.get();
    }

    ITaskManager *ApplicationManager::getTaskManagerPtr() const
    {
        return m_taskManager.get();
    }

    IJobQueue *ApplicationManager::getJobQueuePtr() const
    {
        return m_jobQueue.get();
    }

    IFactoryManager *ApplicationManager::getFactoryManagerPtr() const
    {
        return m_factoryManagers[(u32)TaskId::Primary].get();
    }

    render::IGraphicsSystem *ApplicationManager::getGraphicsSystemPtr() const
    {
        return m_graphicsSystem.get();
    }

    ITimer *ApplicationManager::getTimerPtr() const
    {
        return m_timer.get();
    }

    IStateManager *ApplicationManager::getStateManagerPtr() const
    {
        return m_stateManager.get();
    }

}  // namespace workphone::core
