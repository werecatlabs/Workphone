#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Application.hpp>
#include <Workphone/Interface/Graphics/IGraphicsPipeline.hpp>
#include <Workphone/Scene/Directors/GraphicsSettingsDirector.hpp>
#include <Workphone/AI/AiManager.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/DebugTrace.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/IFont.hpp>
#include <Workphone/Interface/Graphics/IFontManager.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IMaterialManager.hpp>
#include <Workphone/Interface/Graphics/IMaterialPass.hpp>
#include <Workphone/Interface/Graphics/IMaterialTechnique.hpp>
#include <Workphone/Interface/Graphics/IRenderTarget.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Interface/Input/IGameInput.hpp>
#include <Workphone/Interface/Input/IInputDeviceManager.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/Interface/Input/IInputManager.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <Workphone/Interface/Sound/ISound.hpp>
#include <Workphone/Interface/Sound/ISoundManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>
#include <Workphone/Interface/Scene/IGamePrefab.hpp>
#include <Workphone/Interface/Scene/IGamePrefabManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/System/IFrameStatistics.hpp>
#include <Workphone/Interface/System/IFSM.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Interface/System/IConfigFile.hpp>
#include <Workphone/Interface/System/ISelectionManager.hpp>
#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Interface/System/IResourceManager.hpp>
#include <Workphone/Interface/System/IResourceGroupManager.hpp>
#include <Workphone/Interface/System/IThreadPool.hpp>
#include <Workphone/Interface/Vehicle/IVehicleManager.hpp>
#include <Workphone/System/Director.hpp>
#include <Workphone/Scene/Components/Camera.hpp>
#include <Workphone/Scene/Components/CarController.hpp>
#include <Workphone/Scene/Components/CollisionBox.hpp>
#include <Workphone/Scene/Components/CollisionPlane.hpp>
#include <Workphone/Scene/Components/CollisionSphere.hpp>
#include <Workphone/Scene/Components/CollisionTerrain.hpp>
#include <Workphone/Scene/Components/Cubemap.hpp>
#include <Workphone/Scene/Components/Light.hpp>
#include <Workphone/Scene/Components/Material.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Scene/Components/ParticleSystem.hpp>
#include <Workphone/Scene/Components/WheelController.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Scene/Systems/UI/LayoutTransformSystem.hpp>
#include <Workphone/Scene/Systems/LODSystem.hpp>
#include <Workphone/Scene/Components/LODGroup.hpp>
#include <Workphone/System/PluginManager.hpp>
#include <Workphone/Workphone.hpp>

#if defined WP_PLATFORM_APPLE
#    include <Workphone/Core/OSX/macUtils.hpp>
#endif

namespace workphone::core
{

    const hash_type Application::defaultVP = StringUtil::getHash( "defaultVP" );

    WP_CLASS_REGISTER_DERIVED( workphone::core, core::Application, core::IApplication );
    WP_CLASS_REGISTER_DERIVED( workphone::core, core::Application::ApplicationEventListener,
                               IEventListener );

    Application::Application()
    {
#if defined _DEBUG
#    ifdef _WIN32
        _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#    endif
#endif

        static const auto className = String( "Application" );
        //setName( className );

        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );

        m_pluginsConfigFilePath = String( "wp_plugins.cfg" );
        m_renderUIHint = "RenderUI";
    }

    Application::~Application()
    {
    }

    void Application::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto currentThreadId = Thread::ThreadId::Primary;
            Thread::setCurrentThreadId( currentThreadId );

            auto task = TaskId::Primary;
            Thread::setCurrentTask( task );

            auto taskFlags = std::numeric_limits<u32>::max();
            Thread::setTaskFlags( taskFlags );

            auto applicationManager = IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            WP_ASSERT( applicationManager->isValid() );

            if( !applicationManager->isLoaded() )
            {
                applicationManager->load( data );
            }

            applicationManager->setApplication( this );

            createTimer();
            WP_ASSERT( applicationManager->isValid() );

            createLogManager();
            WP_ASSERT( applicationManager->isValid() );

            WP_DEBUG_TRACE;

            createFactoryManager();
            WP_ASSERT( applicationManager->isValid() );

            createFsmManager();
            WP_LOG( "Fsm manager created." );
            WP_ASSERT( applicationManager->isValid() );

            createFsm();
            WP_ASSERT( applicationManager->isValid() );

            createThreadPool();
            WP_ASSERT( applicationManager->isValid() );

            createJobQueue();
            WP_ASSERT( applicationManager->isValid() );

            createPluginManager();
            WP_ASSERT( applicationManager->isValid() );

            createPlugins();
            WP_ASSERT( applicationManager->isValid() );

            allocatePoolMemory();
            WP_LOG( "Pool memory allocated." );
            WP_ASSERT( applicationManager->isValid() );

            createComponentsContainer();
            WP_ASSERT( applicationManager->isValid() );

            createPlatformManager();
            WP_ASSERT( applicationManager->isValid() );

            createProcessManager();
            WP_ASSERT( applicationManager->isValid() );

            createStateManager();
            WP_ASSERT( applicationManager->isValid() );

            createProfiler();
            WP_ASSERT( applicationManager->isValid() );

            createTaskManager();
            WP_ASSERT( applicationManager->isValid() );

            createTasks();
            WP_ASSERT( applicationManager->isValid() );

            createFileSystem();
            WP_LOG( "FileSystem created." );
            WP_ASSERT( applicationManager->isValid() );

            createSoundManager();
            WP_LOG( "SoundManager created." );
            WP_ASSERT( applicationManager->isValid() );

            createScriptManager();
            WP_LOG( "ScriptManager created." );
            WP_ASSERT( applicationManager->isValid() );

            createSceneManager();
            WP_LOG( "Create scene manager." );
            WP_ASSERT( applicationManager->isValid() );

            if( !createGraphicsSystem() )
            {
                WP_LOG( "GraphicsSystem not created." );
                WP_ASSERT( applicationManager->isValid() );
            }
            else
            {
                WP_LOG( "GraphicsSystem created." );
                WP_ASSERT( applicationManager->isValid() );
            }

            loadGraphicsResources();
            WP_LOG( "Resources loaded." );
            WP_ASSERT( applicationManager->isValid() );

            createGraphicsScene();
            WP_LOG( "SceneManager(s) created." );
            WP_ASSERT( applicationManager->isValid() );

            createCamera();
            WP_LOG( "Cameras created." );
            WP_ASSERT( applicationManager->isValid() );

            createViewports();
            WP_LOG( "Viewports created." );
            WP_ASSERT( applicationManager->isValid() );

            createMeshLoader();
            WP_LOG( "Mesh loader created." );
            WP_ASSERT( applicationManager->isValid() );

            createInputSystem();
            WP_LOG( "Create input system." );
            WP_ASSERT( applicationManager->isValid() );

            createUI();
            WP_LOG( "UI created." );
            WP_ASSERT( applicationManager->isValid() );

            createRenderUI();
            WP_LOG( "RenderUI created." );
            WP_ASSERT( applicationManager->isValid() );

            setupRenderpipeline();
            WP_LOG( "Renderpipeline created." );
            WP_ASSERT( applicationManager->isValid() );

            createPhysics();
            WP_LOG( "Physics created." );
            WP_ASSERT( applicationManager->isValid() );

            createRenderWindow();
            WP_LOG( "RenderWindow created." );
            WP_ASSERT( applicationManager->isValid() );

            WP_LOG( "Finished creating base components." );

            createPrefabManager();

            auto applicationEventListener = workphone::make_ptr<ApplicationEventListener>();
            applicationEventListener->setOwner( this );
            applicationManager->addObjectListener( applicationEventListener );
            m_applicationEventListener = applicationEventListener;

            createDefaultMaterialUI();
            createDefaultMaterial();
            createDefaultMaterials();
            createDefaultFont();

            loadScripts();

            if( auto sceneManager = applicationManager->getGameManager() )
            {
                auto factoryManager = applicationManager->getFactoryManager();
                auto componentSystemFactories =
                    factoryManager->getFactoriesByObjectType<scene::IComponentSystem>();

                //for( auto componentSystemFactory : componentSystemFactories )
                {
                    auto system = workphone::make_ptr<scene::LayoutTransformSystem>();
                    system->load( nullptr );

                    sceneManager->addSystem( scene::LayoutTransform::typeInfo(), system );
                }

                {
                    auto system = workphone::make_ptr<scene::LODSystem>();
                    system->load( nullptr );

                    sceneManager->addSystem( scene::LODGroup::typeInfo(), system );
                }
            }

            createAiManager();

            createScene();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Application::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            WP_DEBUG_TRACE;

            auto applicationManager = IApplicationManager::instance();
            if( !applicationManager )
            {
                return;
            }

            WP_ASSERT( applicationManager->isValid() );

            m_fsm = nullptr;

            if( auto viewport = getViewport() )
            {
                auto rt = viewport->getRenderTarget();
                if( rt )
                {
                    rt->removeViewport( viewport );
                }

                //WP_ASSERT( viewport->getReferences() == 1 );
                setViewport( nullptr );
            }

            if( auto camera = getCamera() )
            {
                auto creator = camera->getCreator();
                if( creator )
                {
                    creator->removeGraphicsObject( camera );
                }

                setCamera( nullptr );
            }

            m_cameraSceneNode = nullptr;
            m_viewport = nullptr;
            m_window = nullptr;

            if( m_frameStatistics )
            {
                m_frameStatistics->unload( data );
                m_frameStatistics = nullptr;
            }

            m_sceneMgr = nullptr;

            if( m_applicationEventListener )
            {
                applicationManager->removeObjectListener( m_applicationEventListener );
                m_applicationEventListener = nullptr;
            }

            applicationManager->unload( nullptr );
            applicationManager->setLoadingState( LoadingState::Unloaded );
            IApplicationManager::setInstance( nullptr );
            applicationManager = nullptr;
            m_applicationManager = nullptr;  // Break the reference cycle: ApplicationManager holds a
                                             // SmartPtr back to Application; clearing this field
                                             // allows ApplicationManager to be destroyed, which in
                                             // turn releases its reference to Application.
            WP_ASSERT( core::IApplicationManager::instance() == nullptr );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Application::run()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        WP_ASSERT( applicationManager->isValid() );

        auto threadPool = applicationManager->getThreadPool();
        auto taskManager = applicationManager->getTaskManager();
        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto stateManager = applicationManager->getStateManager();
        auto physicsManager = applicationManager->getPhysicsManager();
        auto inputManager = applicationManager->getInputDeviceManager();

        auto fsmManager = applicationManager->getFsmManager();

        auto timer = applicationManager->getTimer();

        auto task = TaskId::Primary;
        Thread::setCurrentTask( task );

        auto threadId = Thread::ThreadId::Primary;
        Thread::setCurrentThreadId( threadId );

        if( timer )
        {
            timer->reset();
            timer->setSceneLoadTime( 0.0 );
        }

        auto gameManager = applicationManager->getGameManager();

        if( taskManager )
        {
            taskManager->setState( ITaskManager::State::FreeStep );
        }

        if( threadPool )
        {
            threadPool->setState( IThreadPool::State::Start );
        }

        while( applicationManager->isRunning() )
        {
            iterate();
            Thread::yield();
        }

        // clean up
        destroyScene();
    }

    void Application::iterate()
    {
        auto applicationManager = IApplicationManager::instance();
        auto taskManager = applicationManager->getTaskManager();
        auto fsmManager = applicationManager->getFsmManager();

        if( fsmManager )
        {
            fsmManager->update();
        }

        if( taskManager )
        {
            taskManager->update();
        }
    }

    void Application::update()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManager();
        auto taskManager = applicationManager->getTaskManager();
        auto sceneManager = applicationManager->getGameManager();
        auto fsmManager = applicationManager->getFsmManager();
        auto timer = applicationManager->getTimer();
        auto soundManager = applicationManager->getSoundManager();
        auto inputManager = applicationManager->getInputDeviceManager();
        auto cameraManager = applicationManager->getCameraManager();

        stateManager->preUpdate();

        auto t = timer->getTime();
        auto dt = timer->getDeltaTime();

        if( sceneManager )
        {
            sceneManager->preUpdate();
        }

        if( inputManager )
        {
            inputManager->preUpdate();
        }

        if( soundManager )
        {
            soundManager->preUpdate();
        }

        if( cameraManager )
        {
            cameraManager->update();
        }

        stateManager->update();

        if( m_frameStatistics )
        {
            m_frameStatistics->update();
        }

        if( sceneManager )
        {
            sceneManager->update();
        }

        if( inputManager )
        {
            inputManager->update();
        }

        switch( auto task = Thread::getCurrentTask() )
        {
        case TaskId::Application:
        {
        }
        break;
        case TaskId::GarbageCollect:
        {
            try
            {
                auto timer = applicationManager->getTimer();

                auto dt = timer->getDeltaTime();
                auto t = timer->getTime();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }
        break;
        case TaskId::Physics:
        {
            try
            {
                auto physicsManager = applicationManager->getPhysicsManager();

                if( timer->getTimeSinceSceneLoad() > 3.0 )
                {
                    if( physicsManager )
                    {
                        physicsManager->preUpdate();
                        physicsManager->update();
                        physicsManager->postUpdate();

                        auto physicsScene = physicsManager->getPhysicsScene();
                        if( physicsScene )
                        {
                            physicsScene->preUpdate();
                            physicsScene->update();
                            physicsScene->postUpdate();
                        }
                    }

                    if( auto vehicleManager = applicationManager->getVehicleManager() )
                    {
                        vehicleManager->preUpdate();
                        vehicleManager->update();
                        vehicleManager->postUpdate();
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }
        break;
        case TaskId::Primary:
        {
            auto jobQueue = applicationManager->getJobQueue();
            if( jobQueue )
            {
                jobQueue->preUpdate();
                jobQueue->update();
                jobQueue->postUpdate();
            }

            if( applicationManager->getQuit() )
            {
                applicationManager->setRunning( false );
            }

            if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
            {
                graphicsSystem->messagePump();
            }
        }
        break;
        case TaskId::Render:
        {
            try
            {
                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                graphicsSystem->update();

                if( applicationManager->getQuit() )
                {
                    applicationManager->setRunning( false );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }
        break;
        case TaskId::Sound:
        {
        }
        break;
        default:
        {
        }
        break;
        }

        if( sceneManager )
        {
            sceneManager->postUpdate();
        }

        if( inputManager )
        {
            inputManager->postUpdate();
        }

        stateManager->postUpdate();
    }

    Parameter Application::handleEvent( EventType eventType, hash_type eventValue,
                                        const Array<Parameter> &arguments,
                                        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                                        SmartPtr<IEvent> event )
    {
        return {};
    }

    IFSM *Application::getFSMPtr() const
    {
        ScopedLock lock( this );
        return m_fsm.get();
    }

    SmartPtr<IFSM> Application::getFSM() const
    {
        ScopedLock lock( this );
        return m_fsm;
    }

    void Application::setFSM( SmartPtr<IFSM> fsm )
    {
        ScopedLock lock( this );
        m_fsm = fsm;
    }

    bool Application::createGraphicsSystem()
    {
        try
        {
            auto applicationManager = IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            WP_ASSERT( applicationManager->isValid() );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );
            WP_ASSERT( factoryManager->isValid() );

            if( auto graphicsSystem = factoryManager->make_object<render::IGraphicsSystem>() )
            {
                WP_ASSERT( graphicsSystem );
                WP_ASSERT( graphicsSystem->isValid() );

                applicationManager->setGraphicsSystem( graphicsSystem );
                WP_ASSERT( applicationManager->getGraphicsSystem() );
                WP_ASSERT( applicationManager->isValid() );

                SmartPtr<Properties> gameGraphics;
                auto settingsPath = applicationManager->getProjectPath();
                if( !settingsPath.empty() ) settingsPath += "/";
                auto settingsText = Path::readAllText( settingsPath + "GameGraphics.settings" );
                if( !settingsText.empty() )
                {
                    gameGraphics = workphone::make_ptr<Properties>();
                    DataUtil::parse( settingsText, gameGraphics.get() );
                    auto options = workphone::make_ptr<scene::GraphicsSettingsDirector>();
                    options->setProperties( gameGraphics );
                    graphicsSystem->setRendererType( options->getRenderApi() );
                }
                graphicsSystem->load( nullptr );

                WP_ASSERT( applicationManager->isValid() );

                if( !graphicsSystem->configure( nullptr ) )
                {
                    return false;
                }

                if( gameGraphics )
                {
                    if( auto pipeline = graphicsSystem->getGraphicsPipeline() )
                    {
                        // Restore the preset first, then the saved per-effect overrides.
                        pipeline->setProperties( gameGraphics );
                        pipeline->setProperties( gameGraphics );
                    }
                }

                WP_ASSERT( applicationManager->isValid() );

                auto window = graphicsSystem->getDefaultWindow();
                WP_ASSERT( window );
                applicationManager->setWindow( window );

                WP_ASSERT( applicationManager->isValid() );

                // todo refactor
                auto resourceGroupManager = graphicsSystem->getResourceGroupManager();
                WP_ASSERT( resourceGroupManager );
                resourceGroupManager->load( nullptr );

                auto resourceDatabase = factoryManager->make_object<IResourceDatabase>();
                applicationManager->setResourceDatabase( resourceDatabase );
                resourceDatabase->load( nullptr );

                createDefaultFont();

                if( getCreateFrameStatistics() )
                {
                    m_frameStatistics = factoryManager->make_object<IFrameStatistics>();
                    m_frameStatistics->load( nullptr );
                }

                return true;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return false;
    }

    void Application::createDefaultFont()
    {
        auto applicationManager = IApplicationManager::instance();
        auto fileSystem = applicationManager->getFileSystem();

        auto resourceDatabase = applicationManager->getResourceDatabase();
        if( resourceDatabase )
        {
            auto director = workphone::make_ptr<Director>();
            director->load( nullptr );

            const auto fontFileName = String( "cuckoo.ttf" );
            auto directorProperties = director->getProperties();
            directorProperties->setProperty( "font_type", String( "arial" ) );
            directorProperties->setProperty( "font_source", String( fontFileName ) );
            directorProperties->setProperty( "font_size", 12 );
            directorProperties->setProperty( "font_resolution", 96 );

            if( fileSystem->isExistingFile( fontFileName, true, true ) )
            {
                const auto fontName = String( "default" );
                auto result =
                    resourceDatabase->createOrRetrieveFromDirector<render::IFont>( fontName, director );
                if( result.first )
                {
                    auto &font = result.first;
                    font->setProperties( directorProperties );
                }
            }
        }
    }

    void Application::createRenderWindow()
    {
    }

    void Application::createGraphicsScene()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        WP_ASSERT( applicationManager->isValid() );

        if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
        {
            auto name = String( "DefaultSceneManager" );
            auto type = String( "GameSceneManager" );

            m_sceneMgr = graphicsSystem->addGraphicsScene( type, name );
        }
    }

    void Application::loadGraphicsResources()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( graphicsSystem )
        {
            auto resourceGroupManager = graphicsSystem->getResourceGroupManager();
            if( resourceGroupManager )
            {
                graphicsSystem->loadObject( resourceGroupManager, true );
            }
            else
            {
                WP_LOG( "ResourceGroupManager not found." );
            }
        }
    }

    void Application::setupRenderpipeline()
    {
    }

    void Application::createMeshLoader()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        WP_ASSERT( applicationManager->isValid() );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );
        WP_ASSERT( factoryManager->isValid() );

        auto meshLoader = factoryManager->make_object<IMeshLoader>();
        applicationManager->setMeshLoader( meshLoader );
    }

    void Application::createPhysics()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        WP_ASSERT( applicationManager->isValid() );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );
        WP_ASSERT( factoryManager->isValid() );

        auto physicsManager = factoryManager->make_object<physics::IPhysicsManager>();
        if( physicsManager )
        {
            WP_ASSERT( physicsManager->isValid() );

            applicationManager->setPhysicsManager( physicsManager );
            physicsManager->load( nullptr );

            auto physicsScene = physicsManager->addScene();
            physicsManager->setPhysicsScene( physicsScene );

            auto vehicleManager = factoryManager->make_object<vehicle::IVehicleManager>();
            applicationManager->setVehicleManager( vehicleManager );
        }
    }

    void Application::createInputSystem()
    {
        try
        {
            auto applicationManager = IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            if( graphicsSystem )
            {
                if( auto inputManager = factoryManager->make_object<IInputDeviceManager>() )
                {
                    applicationManager->setInputDeviceManager( inputManager );

                    if( auto window = graphicsSystem->getDefaultWindow() )
                    {
                        inputManager->setWindow( window );

#if defined WP_PLATFORM_WIN32
                        inputManager->setCreateMouse( true );
                        inputManager->setCreateKeyboard( true );
#elif defined WP_PLATFORM_APPLE
                        inputManager->setCreateMouse( false );
                        inputManager->setCreateKeyboard( false );
#endif

                        inputManager->load( nullptr );

                        static const String gameInputName = String( "gameInput0" );
                        auto hash = StringUtil::getHash( gameInputName );
                        auto gameInput0 = inputManager->addGameInput( hash );
                        gameInput0->setPlayerIndex( 0 );
                    }
                    else
                    {
                        WP_LOG( "Could not initialise input system. No window found." );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Application::createSceneManager()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        WP_ASSERT( applicationManager->isValid() );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );
        WP_ASSERT( factoryManager->isValid() );

        auto sceneManager = factoryManager->make_object<scene::IGameManager>();
        WP_ASSERT( sceneManager );
        WP_ASSERT( sceneManager->isValid() );

        if( sceneManager )
        {
            applicationManager->setGameManager( sceneManager );

            sceneManager->load( nullptr );
            WP_ASSERT( sceneManager->isValid() );

            auto scene = factoryManager->make_object<scene::IGameScene>();
            if( scene )
            {
                WP_ASSERT( scene->isValid() );
                scene->load( nullptr );

                static const auto sceneLabel = String( "Application" );
                scene->setLabel( sceneLabel );

                sceneManager->setCurrentScene( scene );
                WP_ASSERT( scene->isValid() );
            }
        }
    }

    void Application::createCamera()
    {
        if( m_sceneMgr )
        {
            auto applicationManager = IApplicationManager::instance();
            auto renderWindow = applicationManager->getWindow();

            WP_ASSERT( m_sceneMgr );
            WP_ASSERT( m_sceneMgr->isValid() );

            m_camera = m_sceneMgr->addGraphicsObjectByType<render::IGraphicsCamera>();
            WP_ASSERT( m_camera );

            const auto cameraName = String( "DefaultCamera" );
            m_camera->setName( cameraName );

            auto rootNode = m_sceneMgr->getRootSceneNode();
            WP_ASSERT( rootNode );

            m_cameraSceneNode = rootNode->addChildSceneNode( cameraName );
            WP_ASSERT( m_cameraSceneNode );

            m_cameraSceneNode->attachObject( m_camera );

            auto cameraPosition = Vector3<real_Num>::zero();
            cameraPosition += Vector3<real_Num>::unitY() * 5.0;
            cameraPosition += Vector3<real_Num>::unitZ() * 20.0;
            m_cameraSceneNode->setPosition( cameraPosition );

            m_camera->setNearClipDistance( 0.01f );
            m_camera->setFarClipDistance( 1000.0f );

            auto renderTexture = renderWindow->getTexture();
            m_camera->setTargetTexture( renderTexture );

            m_camera->setVisible( true );

            WP_ASSERT( m_sceneMgr->isValid() );
            WP_ASSERT( m_cameraSceneNode->isValid() );
            WP_ASSERT( m_camera->isValid() );
        }
    }

    SmartPtr<scene::IGameActor> Application::createDefaultCamera( bool addToScene )
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();

        auto name = String( "Camera" );
        actor->setName( name );

        auto c = actor->addComponent<scene::Camera>();
        WP_ASSERT( c );

        c->setEnableShadows( true );

        if( addToScene )
        {
            scene->addActor( actor );
            scene->registerAllUpdates( actor );
        }

        return actor;
    }

    void Application::createRigidStaticMesh()
    {
        try
        {
            auto applicationManager = IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto selectionManager = applicationManager->getSelectionManager();
            WP_ASSERT( selectionManager );

            auto selection = selectionManager->getSelection();
            for( auto selected : selection )
            {
                if( selected->isDerived<scene::IGameActor>() )
                {
                    auto actor = workphone::static_pointer_cast<scene::IGameActor>( selected );
                    createRigidStaticMesh( actor, true );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Application::createRigidDynamicMesh()
    {
        try
        {
            auto applicationManager = IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto selectionManager = applicationManager->getSelectionManager();
            WP_ASSERT( selectionManager );

            auto selection = selectionManager->getSelection();
            for( auto selected : selection )
            {
                if( selected->isDerived<scene::IGameActor>() )
                {
                    auto actor = workphone::static_pointer_cast<scene::IGameActor>( selected );
                    createRigidDynamicMesh( actor, true );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<Properties> Application::importScene( const String &filePath )
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto dataStr = fileSystem->readAllText( filePath );

        auto pScene = workphone::make_ptr<Properties>();

        DataUtil::parse( dataStr, pScene.get() );

        return pScene;
    }

    SmartPtr<scene::IGameActor> Application::createDefaultCubemap( bool addToScene )
    {
        try
        {
            auto applicationManager = IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );
            WP_ASSERT( applicationManager->isValid() );

            auto sceneManager = applicationManager->getGameManager();
            WP_ASSERT( sceneManager );
            WP_ASSERT( sceneManager->isValid() );

            auto scene = sceneManager->getCurrentScene();
            WP_ASSERT( scene );
            WP_ASSERT( scene->isValid() );

            auto actor = sceneManager->createActor();
            WP_ASSERT( actor );

            auto name = String( "Cubemap" );
            actor->setName( name );

            auto c = actor->addComponent<scene::CollisionBox>();
            WP_ASSERT( c );

            auto cubemap = actor->addComponent<scene::Cubemap>();
            WP_ASSERT( cubemap );

            auto meshComponent = actor->addComponent<scene::Mesh>();
            WP_ASSERT( meshComponent );
            meshComponent->setMeshPath( "cube_internal.fbmeshbin" );

            auto meshRenderer = actor->addComponent<scene::MeshRenderer>();
            WP_ASSERT( meshRenderer );

            auto material = actor->addComponent<scene::Material>();
            WP_ASSERT( material );
            WP_ASSERT( material->isValid() );

            if( material )
            {
                material->setMaterialPath( "Standard.mat" );
            }

            if( addToScene )
            {
                scene->addActor( actor );
                WP_ASSERT( scene->isValid() );
            }

            return actor;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    SmartPtr<scene::IGameActor> Application::createDefaultCube( bool addToScene )
    {
        try
        {
            auto applicationManager = IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            WP_ASSERT( applicationManager->isValid() );

            auto sceneManager = applicationManager->getGameManager();
            WP_ASSERT( sceneManager );
            WP_ASSERT( sceneManager->isValid() );

            auto scene = sceneManager->getCurrentScene();
            WP_ASSERT( scene );
            WP_ASSERT( scene->isValid() );

            auto actor = sceneManager->createActor();
            WP_ASSERT( actor );

            const auto name = String( "Cube" );
            actor->setName( name );

            auto c = actor->addComponent<scene::CollisionBox>();
            WP_ASSERT( c );

            auto rigidbody = actor->addComponent<scene::Rigidbody>();
            WP_ASSERT( rigidbody );

            auto meshComponent = actor->addComponent<scene::Mesh>();
            WP_ASSERT( meshComponent );
            meshComponent->setMeshPath( "cube_internal.fbmeshbin" );

            auto meshRenderer = actor->addComponent<scene::MeshRenderer>();
            WP_ASSERT( meshRenderer );

            auto material = actor->addComponent<scene::Material>();
            WP_ASSERT( material );
            WP_ASSERT( material->isValid() );

            if( material )
            {
                material->setMaterialPath( "Standard.mat" );
            }

            if( addToScene )
            {
                scene->addActor( actor );
                WP_ASSERT( scene->isValid() );
            }

            return actor;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    SmartPtr<scene::IGameActor> Application::createDefaultCubeMesh( bool addToScene )
    {
        try
        {
            auto applicationManager = IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            WP_ASSERT( applicationManager->isValid() );

            auto sceneManager = applicationManager->getGameManager();
            WP_ASSERT( sceneManager );
            WP_ASSERT( sceneManager->isValid() );

            auto scene = sceneManager->getCurrentScene();
            WP_ASSERT( scene );
            WP_ASSERT( scene->isValid() );

            auto actor = sceneManager->createActor();
            WP_ASSERT( actor );

            const auto name = String( "Cube Mesh" );
            actor->setName( name );

            auto c = actor->addComponent<scene::CollisionMesh>();
            WP_ASSERT( c );

            auto rigidbody = actor->addComponent<scene::Rigidbody>();
            WP_ASSERT( rigidbody );

            auto meshComponent = actor->addComponent<scene::Mesh>();
            WP_ASSERT( meshComponent );
            meshComponent->setMeshPath( "cube_internal.fbmeshbin" );

            auto meshRenderer = actor->addComponent<scene::MeshRenderer>();
            WP_ASSERT( meshRenderer );

            auto material = actor->addComponent<scene::Material>();
            WP_ASSERT( material );
            WP_ASSERT( material->isValid() );

            if( material )
            {
                material->setMaterialPath( "Standard.mat" );
            }

            if( addToScene )
            {
                scene->addActor( actor );
                WP_ASSERT( scene->isValid() );
            }

            return actor;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    SmartPtr<scene::IGameActor> Application::createDefaultGround( bool addToScene )
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();
        actor->setStatic( true );

        auto name = String( "Ground" );
        actor->setName( name );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        WP_ASSERT( collisionBox );
        if( collisionBox )
        {
            collisionBox->setExtents( Vector3<real_Num>::unit() * 500.0f );
        }

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        WP_ASSERT( rigidbody );

        auto meshComponent = actor->addComponent<scene::Mesh>();
        WP_ASSERT( meshComponent );
        meshComponent->setMeshPath( "cube_internal.fbmeshbin" );

        auto meshRenderer = actor->addComponent<scene::MeshRenderer>();
        WP_ASSERT( meshRenderer );

        auto material = actor->addComponent<scene::Material>();
        WP_ASSERT( material );

        if( material )
        {
            material->setMaterialPath( "Standard.mat" );
            material->updateMaterial();
        }

        auto groundPosition = Vector3<real_Num>::unitY() * static_cast<real_Num>( -250.0 );
        // groundPosition += Vector3<real_Num>::unitZ() * -250.0f;

        auto groundScale = Vector3<real_Num>::unit() * static_cast<real_Num>( 500.0 );

        auto transform = actor->getTransform();
        if( transform )
        {
            transform->setLocalPosition( groundPosition );
            transform->setLocalScale( groundScale );
        }

        if( addToScene )
        {
            scene->addActor( actor );
            WP_ASSERT( scene->isValid() );
        }

        return actor;
    }

    SmartPtr<scene::IGameActor> Application::createDefaultTerrain( bool addToScene )
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();
        actor->setStatic( true );

        auto name = String( "Terrain" );
        actor->setName( name );

        auto terrain = actor->addComponent<scene::TerrainSystem>();

        auto terrainCollision = actor->addComponent<scene::CollisionTerrain>();

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        WP_ASSERT( rigidbody );

        if( addToScene )
        {
            scene->addActor( actor );
            scene->registerAllUpdates( actor );
        }

        return actor;
    }

    SmartPtr<scene::IGameActor> Application::createDefaultConstraint()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();
        actor->setStatic( true );

        auto name = String( "Constraint" );
        actor->setName( name );

        // auto id = StringUtil::getHash(name);
        // actor->setId(id);

        // auto uuid = StringUtil::getUUID();
        // actor->setUUID(uuid);

        auto terrain = actor->addComponent<scene::Constraint>();

        scene->addActor( actor );
        scene->registerAllUpdates( actor );

        return actor;
    }

    SmartPtr<scene::IGameActor> Application::createDirectionalLight( bool addToScene )
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();

        static const auto name = String( "Directional Light" );
        actor->setName( name );

        auto light = actor->addComponent<scene::Light>();
        if( light )
        {
            light->setLightType( LightTypes::LT_DIRECTIONAL );
        }

        if( addToScene )
        {
            scene->addActor( actor );
            scene->registerAllUpdates( actor );
        }

        return actor;
    }

    SmartPtr<scene::IGameActor> Application::createPointLight( bool addToScene )
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();

        auto name = String( "Main Light" );
        actor->setName( name );

        // auto id = StringUtil::getHash(name);
        // actor->setId(id);

        // auto uuid = StringUtil::getUUID();
        // actor->setUUID(uuid);

        // todo remove
        // for test the renderer
        auto light = actor->addComponent<scene::Light>();
        if( light )
        {
            light->setLightType( LightTypes::LT_POINT );
        }

        if( addToScene )
        {
            scene->addActor( actor );
            scene->registerAllUpdates( actor );
        }

        auto uniqueId = 0;  // StringUtil::getHash(uuid);
        return actor;
    }

    SmartPtr<scene::IGameActor> Application::createDefaultPlane( bool addToScene )
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();

        auto name = String( "Plane" );
        actor->setName( name );

        auto localOrientation = Quaternion<real_Num>::angleAxis(
            Math<real_Num>::DegToRad( static_cast<real_Num>( -90.0 ) ), Vector3<real_Num>::unitX() );
        actor->setLocalOrientation( localOrientation );
        actor->setLocalScale( Vector3<real_Num>::unit() * static_cast<real_Num>( 100.0 ) );

        auto c = actor->addComponent<scene::CollisionPlane>();
        WP_ASSERT( c );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        WP_ASSERT( rigidbody );

        auto meshComponent = actor->addComponent<scene::Mesh>();
        WP_ASSERT( meshComponent );
        meshComponent->setMeshPath( "plane.fbmeshbin" );

        auto meshRenderer = actor->addComponent<scene::MeshRenderer>();

        auto material = actor->addComponent<scene::Material>();
        WP_ASSERT( material );

        if( material )
        {
            material->setMaterialPath( "Standard.mat" );
            material->updateMaterial();
        }

        if( addToScene )
        {
            scene->addActor( actor );
            scene->registerAllUpdates( actor );
        }

        return actor;
    }

    SmartPtr<scene::IGameActor> Application::createDefaultVehicle( bool addToScene )
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto prefabManager = applicationManager->getPrefabManager();
        WP_ASSERT( prefabManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        f32 wheelBase = 2.530f;
        f32 length = 4.405f;
        f32 width = 1.810f;
        f32 height = 1.170f;
        f32 mass = 1370.0f;

        auto actor = sceneManager->createActor();

        auto name = String( "Vehicle" );
        actor->setName( name );

        // auto id = StringUtil::getHash(name);
        // actor->setId(id);

        // auto uuid = StringUtil::getUUID();
        // actor->setUUID(uuid);

        auto c = actor->addComponent<scene::CollisionBox>();
        WP_ASSERT( c );

        auto bodyDimensions = Vector3<real_Num>( length, height, width ) * 0.5f;
        c->setExtents( bodyDimensions );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        WP_ASSERT( rigidbody );

        auto vehicle = actor->addComponent<scene::CarController>();
        WP_ASSERT( vehicle );

        auto cubeMeshPath = String( "cube_internal.fbmeshbin" );

        auto meshComponent = actor->addComponent<scene::Mesh>();
        WP_ASSERT( meshComponent );
        meshComponent->setMeshPath( cubeMeshPath );

        auto meshRenderer = actor->addComponent<scene::MeshRenderer>();
        WP_ASSERT( meshRenderer );

        auto material = actor->addComponent<scene::Material>();
        WP_ASSERT( material );

        if( material )
        {
            material->setMaterialPath( "Standard.mat" );
            material->updateMaterial();
        }

        auto meshActor = sceneManager->createActor();

        auto meshActorName = String( "mesh" );
        meshActor->setName( meshActorName );

        actor->addChild( meshActor );

        Array<Vector3<real_Num>> wheelPositions;
        wheelPositions.resize( 4 );

        auto wheelOffset = -1.0f;
        wheelPositions[static_cast<u32>( scene::CarController::Wheels::FRONT_LEFT )] =
            Vector3<real_Num>( -width / 2.0f, wheelOffset, wheelBase / 2.0f );
        wheelPositions[static_cast<u32>( scene::CarController::Wheels::FRONT_RIGHT )] =
            Vector3<real_Num>( width / 2.0f, wheelOffset, wheelBase / 2.0f );
        wheelPositions[static_cast<u32>( scene::CarController::Wheels::REAR_LEFT )] =
            Vector3<real_Num>( -width / 2.0f, wheelOffset, -wheelBase / 2.0f );
        wheelPositions[static_cast<u32>( scene::CarController::Wheels::REAR_RIGHT )] =
            Vector3<real_Num>( width / 2.0f, wheelOffset, -wheelBase / 2.0f );

        auto wheelMeshSize = Vector3<real_Num>( 0.1f, 0.3f, 0.3f );

        for( u32 i = 0; i < 4; ++i )
        {
            auto wheelMeshActor = sceneManager->createActor();

            auto wheelMeshActorName = String( "wheel_" ) + StringUtil::toString( i );
            wheelMeshActor->setName( wheelMeshActorName );

            auto wheelMeshComponent = wheelMeshActor->addComponent<scene::Mesh>();
            WP_ASSERT( wheelMeshComponent );
            wheelMeshComponent->setMeshPath( cubeMeshPath );

            auto meshRenderer = wheelMeshActor->addComponent<scene::MeshRenderer>();
            WP_ASSERT( meshRenderer );

            auto material = wheelMeshActor->addComponent<scene::Material>();
            WP_ASSERT( material );

            meshActor->addChild( wheelMeshActor );

            wheelMeshActor->setLocalPosition( wheelPositions[i] );
            wheelMeshActor->setLocalScale( wheelMeshSize );
        }

        auto dynamicsActor = sceneManager->createActor();

        auto dynamicsName = String( "dynamics" );
        dynamicsActor->setName( dynamicsName );

        // auto dynamicsId = StringUtil::getHash(dynamicsName);
        // dynamicsActor->setId(dynamicsId);

        // auto dynamicsUuid = StringUtil::getUUID();
        // dynamicsActor->setUUID(dynamicsUuid);

        actor->addChild( dynamicsActor );

        for( u32 i = 0; i < 4; ++i )
        {
            auto wheelActor = sceneManager->createActor();
            dynamicsActor->addChild( wheelActor );

            auto wheelName = String( "Wheel" );
            wheelActor->setName( wheelName );

            auto wheel = wheelActor->addComponent<scene::WheelController>();
            WP_ASSERT( wheel );
        }

        // auto prefab = prefabManager->loadPrefab("f40.fbx");
        // if (prefab)
        //{
        //	auto vehicleMesh = prefab->createActor();
        //
        //	//auto vehicleMeshTransform = vehicleMesh->getLocalTransform();
        //	//if (vehicleMeshTransform)
        //	//{
        //	//	vehicleMeshTransform->setScale(Vector3<real_Num>::unit() * 0.0254f);
        //	//}

        //	vehicleMesh->setName("F40");
        //	actor->addChild(vehicleMesh);
        //}

        auto groundPosition = Vector3<real_Num>::unitY() * static_cast<real_Num>( 5.0 );
        auto groundScale = Vector3<real_Num>::unit() * static_cast<real_Num>( 1.0 );

        auto transform = actor->getTransform();
        if( transform )
        {
            transform->setLocalPosition( groundPosition );
            transform->setLocalScale( groundScale );
        }

        actor->updateTransform();

        if( addToScene )
        {
            scene->addActor( actor );
            scene->registerAllUpdates( actor );
        }

        return actor;
    }

    SmartPtr<scene::IGameActor> Application::createDefaultCar( bool addToScene )
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto prefabManager = applicationManager->getPrefabManager();
        WP_ASSERT( prefabManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();

        auto name = String( "Car" );
        actor->setName( name );

        // auto id = StringUtil::getHash(name);
        // actor->setId(id);

        // auto uuid = StringUtil::getUUID();
        // actor->setUUID(uuid);

        auto c = actor->addComponent<scene::CollisionBox>();
        WP_ASSERT( c );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        WP_ASSERT( rigidbody );

        auto vehicle = actor->addComponent<scene::CarController>();
        WP_ASSERT( vehicle );

        auto cubeMeshPath = String( "cube_internal.fbmeshbin" );

        auto meshComponent = actor->addComponent<scene::Mesh>();
        WP_ASSERT( meshComponent );
        meshComponent->setMeshPath( cubeMeshPath );

        auto meshRenderer = actor->addComponent<scene::MeshRenderer>();
        WP_ASSERT( meshRenderer );

        auto material = actor->addComponent<scene::Material>();
        WP_ASSERT( material );

        if( material )
        {
            material->setMaterialPath( "Standard.mat" );
            material->updateMaterial();
        }

        auto dynamicsActor = sceneManager->createActor();

        auto dynamicsName = String( "dynamics" );
        dynamicsActor->setName( dynamicsName );

        // auto dynamicsId = StringUtil::getHash(dynamicsName);
        // dynamicsActor->setId(dynamicsId);

        // auto dynamicsUuid = StringUtil::getUUID();
        // dynamicsActor->setUUID(dynamicsUuid);

        actor->addChild( dynamicsActor );

        for( u32 i = 0; i < 4; ++i )
        {
            auto wheelActor = sceneManager->createActor();
            dynamicsActor->addChild( wheelActor );

            auto wheelName = String( "Wheel" );
            wheelActor->setName( wheelName );

            auto wheel = wheelActor->addComponent<scene::WheelController>();
            WP_ASSERT( wheel );
        }

        auto rigActor = sceneManager->createActor();

        auto rigName = String( "rig" );
        rigActor->setName( rigName );
        actor->addChild( rigActor );

        //auto prefab = prefabManager->loadPrefab( "f40.fbx" );
        //if( prefab )
        //{
        //    auto vehicleMesh = prefab->createActor();

        //    // auto vehicleMeshTransform = vehicleMesh->getLocalTransform();
        //    // if (vehicleMeshTransform)
        //    //{
        //    //	vehicleMeshTransform->setScale(Vector3<real_Num>::unit() * 0.0254f);
        //    // }

        //    vehicleMesh->setName( "F40" );
        //    rigActor->addChild( vehicleMesh );
        //}

        auto groundPosition = Vector3<real_Num>::unitY() * 5.0f;
        auto groundScale = Vector3<real_Num>::unit() * 1.0f;

        auto transform = actor->getTransform();
        if( transform )
        {
            transform->setLocalPosition( groundPosition );
            transform->setLocalScale( groundScale );
        }

        actor->updateTransform();

        if( addToScene )
        {
            scene->addActor( actor );
            scene->registerAllUpdates( actor );
        }

        return actor;
    }

    SmartPtr<scene::IGameActor> Application::createDefaultTruck( bool addToScene )
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto prefabManager = applicationManager->getPrefabManager();
        WP_ASSERT( prefabManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();

        auto name = String( "Truck" );
        actor->setName( name );

        // auto id = StringUtil::getHash(name);
        // actor->setId(id);

        // auto uuid = StringUtil::getUUID();
        // actor->setUUID(uuid);

        auto c = actor->addComponent<scene::CollisionBox>();
        WP_ASSERT( c );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        WP_ASSERT( rigidbody );

        auto vehicle = actor->addComponent<scene::CarController>();
        WP_ASSERT( vehicle );

        auto meshComponent = actor->addComponent<scene::Mesh>();
        WP_ASSERT( meshComponent );
        meshComponent->setMeshPath( "cube_internal.fbmeshbin" );

        auto meshRenderer = actor->addComponent<scene::MeshRenderer>();
        WP_ASSERT( meshRenderer );

        auto material = actor->addComponent<scene::Material>();
        WP_ASSERT( material );

        if( material )
        {
            material->setMaterialPath( "Standard.mat" );
            material->updateMaterial();
        }

        auto dynamicsActor = sceneManager->createActor();

        auto dynamicsName = String( "dynamics" );
        dynamicsActor->setName( dynamicsName );

        // auto dynamicsId = StringUtil::getHash(dynamicsName);
        // dynamicsActor->setId(dynamicsId);

        // auto dynamicsUuid = StringUtil::getUUID();
        // dynamicsActor->setUUID(dynamicsUuid);

        actor->addChild( dynamicsActor );

        for( u32 i = 0; i < 4; ++i )
        {
            auto wheelActor = sceneManager->createActor();
            dynamicsActor->addChild( wheelActor );

            auto wheelName = String( "Wheel" );
            wheelActor->setName( wheelName );

            auto wheel = wheelActor->addComponent<scene::WheelController>();
            WP_ASSERT( wheel );
        }

        auto rigActor = sceneManager->createActor();

        auto rigName = String( "rig" );
        rigActor->setName( rigName );
        actor->addChild( rigActor );

        auto prefab = prefabManager->loadPrefab( "f40.fbx" );
        if( prefab )
        {
            auto vehicleMesh = prefab->createActor();

            // auto vehicleMeshTransform = vehicleMesh->getLocalTransform();
            // if (vehicleMeshTransform)
            //{
            //	vehicleMeshTransform->setScale(Vector3<real_Num>::unit() * 0.0254f);
            // }

            vehicleMesh->setName( "F40" );
            rigActor->addChild( vehicleMesh );
        }

        auto groundPosition = Vector3<real_Num>::unitY() * 5.0f;
        auto groundScale = Vector3<real_Num>::unit() * 1.0f;

        auto transform = actor->getTransform();
        if( transform )
        {
            transform->setLocalPosition( groundPosition );
            transform->setLocalScale( groundScale );
        }

        actor->updateTransform();

        if( addToScene )
        {
            scene->addActor( actor );
            scene->registerAllUpdates( actor );
        }

        return actor;
    }

    SmartPtr<scene::IGameActor> Application::createDefaultParticleSystem( bool addToScene )
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();
        WP_ASSERT( actor );

        auto particleSystem = actor->addComponent<scene::ParticleSystem>();
        WP_ASSERT( particleSystem );

        return actor;
    }

    SmartPtr<render::IMaterial> Application::createDefaultMaterialUI()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( graphicsSystem )
        {
            auto materialManager = graphicsSystem->getMaterialManager();
            WP_ASSERT( materialManager );

            auto materialName = String( "DefaultUI" );
            SmartPtr<render::IMaterial> defaultMat =
                materialManager->loadFromFile( materialName + ".mat" );
            if( !defaultMat )
            {
                static const auto uuid =
                    String( "2f261ebe-5db8-11ed-9b6a-0242ac120002" );  //StringUtil::getUUID();

                auto resource = materialManager->createOrRetrieve( uuid, materialName, "material" );
                auto material = workphone::static_pointer_cast<render::IMaterial>( resource.first );
                if( material )
                {
                    material->setMaterialType( MaterialType::UI );
                    graphicsSystem->loadObject( material );

                    auto techniques = material->getTechniques();
                    SmartPtr<render::IMaterialTechnique> technique;

                    if( !techniques.empty() )
                    {
                        technique = techniques[0];
                    }

                    if( !technique )
                    {
                        technique = material->createTechnique();
                    }

                    if( technique )
                    {
                        auto passes = technique->getPasses();
                        SmartPtr<render::IMaterialPass> pass;

                        if( !passes.empty() )
                        {
                            pass = passes[0];
                        }

                        if( !pass )
                        {
                            pass = technique->createPass();
                        }

                        if( pass )
                        {
                            auto textures = pass->getTextureUnits();
                            if( !textures.empty() )
                            {
                                //textures[0];
                            }
                        }
                    }
                }

                return material;
            }

            return defaultMat;
        }

        return nullptr;
    }

    SmartPtr<render::IMaterial> Application::createDefaultMaterial()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( graphicsSystem )
        {
            auto materialManager = graphicsSystem->getMaterialManager();
            WP_ASSERT( materialManager );

            auto resource = materialManager->create( StringUtil::getUUID() );
            auto material = workphone::static_pointer_cast<render::IMaterial>( resource );
            if( material )
            {
                auto techniques = material->getTechniques();
                SmartPtr<render::IMaterialTechnique> technique;

                if( !techniques.empty() )
                {
                    technique = techniques[0];
                }

                if( !technique )
                {
                    technique = material->createTechnique();
                }

                if( technique )
                {
                    auto passes = technique->getPasses();
                    SmartPtr<render::IMaterialPass> pass;

                    if( !passes.empty() )
                    {
                        pass = passes[0];
                    }

                    if( !pass )
                    {
                        pass = technique->createPass();
                    }

                    if( pass )
                    {
                    }
                }
            }

            return material;
        }

        return nullptr;
    }

    void Application::createDefaultMaterials()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( graphicsSystem )
        {
            auto resourceDatabase = applicationManager->getResourceDatabase();

            auto filePath = String( "default" );
            auto result = resourceDatabase->createOrRetrieveByType<render::IMaterial>( filePath );
            if( result.first && result.second )
            {
                auto material = result.first;
                graphicsSystem->loadObject( material );
            }
        }
    }

    void Application::createViewports()
    {
        try
        {
            auto applicationManager = IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
            {
                if( auto window = graphicsSystem->getDefaultWindow() )
                {
                    if( m_camera )
                    {
                        m_camera->setAutoAspectRatio( true );

                        auto vp = window->addViewport( defaultVP, m_camera );
                        WP_ASSERT( vp );

                        auto viewportColour = ColourF::Blue * 0.2f;
                        vp->setBackgroundColour( viewportColour );

                        vp->setClearEveryFrame( true );
                        vp->setOverlaysEnabled( true );
                        vp->setEnableUI( true );
                        vp->setEnableSceneRender( true );
                        vp->setAutoUpdated( true );

                        vp->setActive( true );

                        m_viewport = vp;

                        WP_ASSERT( m_viewport );
                        WP_ASSERT( m_viewport->isValid() );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    bool Application::createScriptManager()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto scriptManager = factoryManager->make_object<IScriptManager>();
        if( scriptManager )
        {
            scriptManager->load( nullptr );
            applicationManager->setScriptManager( scriptManager );
        }

        return scriptManager != nullptr;
    }

    bool Application::createSoundManager()
    {
        auto applicationManager = IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        if( auto soundManager = factoryManager->make_object<ISoundManager>() )
        {
            soundManager->load( nullptr );
            applicationManager->setSoundManager( soundManager );
        }

        return true;
    }

    void Application::createPluginManager()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto pluginManager = factoryManager->make_ptr<PluginManager>();
        applicationManager->setPluginManager( pluginManager );
    }

    void Application::createPlugins()
    {
        try
        {
            auto applicationManager = IApplicationManager::instancePtr();
            auto jobQueue = applicationManager->getJobQueuePtr();

#ifndef WP_STATIC_LIB
            auto corePlugin = workphone::make_ptr<WorkphonePlugin>();
            applicationManager->addPlugin( corePlugin );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            auto configFile = factoryManager->make_object<IConfigFile>();

            auto pluginsConfigFilePath = getPluginsConfigFilePath();
            if( !StringUtil::isNullOrEmpty( pluginsConfigFilePath ) )
            {
                configFile->loadFromFilePath( pluginsConfigFilePath );
            }

            auto pluginKey = String( "Plugin" );
            auto plugins = configFile->getSettings( pluginKey );

            for( const auto &pluginPath : plugins )
            {
                auto job = factoryManager->make_ptr<LoadPluginJob>();
                job->setPluginPath( pluginPath );
                //jobQueue->addJob( job );
                job->execute();
            }

            while( jobQueue->hasJobs() )
            {
                jobQueue->update();
            }
#else
            auto corePlugin = workphone::make_ptr<WorkphonePlugin>();
            applicationManager->addPlugin( corePlugin );

            auto factoryManager = applicationManager->getFactoryManager();
            auto configFile = factoryManager->make_object<IConfigFile>();

            auto pluginsConfigFilePath = getPluginsConfigFilePath();
            if( !StringUtil::isNullOrEmpty( pluginsConfigFilePath ) )
            {
                configFile->loadFromFilePath( pluginsConfigFilePath );

                auto pluginKey = String( "Plugin" );
                auto plugins = configFile->getSettings( pluginKey );

                for( auto pluginPath : plugins )
                {
                    auto job = workphone::make_ptr<LoadPluginJob>();
                    job->setPluginPath( pluginPath );
                    job->execute();
                }
            }
#endif
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Application::createProcessManager()
    {
        auto applicationManager = IApplicationManager::instance();

        auto processManager = workphone::make_ptr<ProcessManager>();
        applicationManager->setProcessManager( processManager );
    }

    void Application::destroyScene()
    {
    }

    void Application::loadScripts()
    {
        try
        {
            auto applicationManager = IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            auto scriptManager = applicationManager->getScriptManagerPtr();
            if( scriptManager )
            {
                auto supportedExtensions = scriptManager->getSupportedFileExtensions();
                for( const auto &scriptExt : supportedExtensions )
                {
                    auto scripts = fileSystem->getFileNamesWithExtension( scriptExt );
                    Util::sort( scripts.begin(), scripts.end() );
                    scriptManager->loadScripts( scripts );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    FSMReturnType Application::handleEvent( u32 state, FSMEvent eventType )
    {
        return FSMReturnType::Ok;
    }

    void Application::createLogManager()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto logManager = workphone::make_ptr<LogManagerDefault>();
        WP_ASSERT( logManager );
        WP_ASSERT( logManager->isValid() );

        applicationManager->setLogManager( logManager );
        logManager->open( "Application.log" );

        WP_ASSERT( logManager->isValid() );
    }

    void Application::createAiManager()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto aiManager = workphone::make_ptr<AiManager>();
        WP_ASSERT( aiManager );
        WP_ASSERT( aiManager->isValid() );

        applicationManager->setAiManager( aiManager );
        aiManager->load( nullptr );

        WP_ASSERT( aiManager->isValid() );
    }

    void Application::createFactoryManager()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = workphone::make_ptr<FactoryManager>();
        factoryManager->load( nullptr );

        applicationManager->setFactoryManager( factoryManager );
        WP_ASSERT( applicationManager->getFactoryManager() );
    }

    void Application::allocatePoolMemory()
    {
        auto applicationManager = IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();
        if( factoryManager )
        {
            auto factories = factoryManager->getFactories();
            for( auto factory : factories )
            {
                factory->allocatePoolData();
            }
        }
    }

    void Application::createStateManager()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        WP_ASSERT( applicationManager->isValid() );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );
        WP_ASSERT( factoryManager->isValid() );

        auto stateManager = factoryManager->make_object<IStateManager>();
        stateManager->load( nullptr );
        applicationManager->setStateManager( stateManager );
    }

    void Application::createComponentsContainer()
    {
    }

    void Application::createPlatformManager()
    {
    }

    void Application::createFileSystem()
    {
        try
        {
            auto applicationManager = IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto fileSystem = factoryManager->make_object<IFileSystem>();
            fileSystem->load( nullptr );
            applicationManager->setFileSystem( fileSystem );

            auto workingDirectory = Path::getWorkingDirectory();
            fileSystem->addFolder( workingDirectory );

            auto mediaPath = String( "" );
            auto scriptsPath = String( "" );

            if( applicationManager->isEditor() )
            {
#if defined WP_PLATFORM_WIN32
                mediaPath = String( "../../../../../Media" );
                scriptsPath = String( "../../../../../Media/Scripts/" );
#elif defined WP_PLATFORM_APPLE
                mediaPath = String( "../../Media" );
                scriptsPath = String( "../../Media/Scripts/" );
#else
                mediaPath = String( "../../Media" );
                scriptsPath = String( "../../Media/Scripts/" );
#endif
            }
            else
            {
#if defined WP_PLATFORM_WIN32
                mediaPath = String( "../../../../../Media" );
                scriptsPath = String( "../../../../../Media/Scripts/" );
#elif defined WP_PLATFORM_APPLE
                mediaPath = String( "../../Media" );
                scriptsPath = String( "../../Media/Scripts/" );
#else
                mediaPath = String( "../../Media" );
                scriptsPath = String( "../../Media/Scripts/" );
#endif
            }

            auto absolutePath = Path::lexically_normal( workingDirectory, mediaPath );

            auto packs = fileSystem->getFileNamesWithExtension( workingDirectory, ".fbpack" );
            for( auto &pack : packs )
            {
                fileSystem->addFileArchive( pack, true, true, IFileSystem::ArchiveType::Zip );
            }

#if defined WP_PLATFORM_WIN32
            fileSystem->addFolder( mediaPath, false );
            fileSystem->addFolder( scriptsPath, true );
            applicationManager->setMediaPath( mediaPath );
#else
            auto absoluteMediaPath = Path::lexically_normal( workingDirectory, mediaPath );
            fileSystem->addFolder( mediaPath, false );
            fileSystem->addFolder( scriptsPath, true );
            applicationManager->setMediaPath( mediaPath );
#endif

            fileSystem->addFileArchive( ".FBCache", true, true, IFileSystem::ArchiveType::Folder );
            fileSystem->addFileArchive( "./", true, true, IFileSystem::ArchiveType::Folder );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Application::createUI()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        WP_ASSERT( applicationManager->isValid() );

        auto graphicsSystem = applicationManager->getGraphicsSystem();

        auto factoryManager = applicationManager->getFactoryManager();

        if( auto ui = factoryManager->make_object<ui::IUIManager>() )
        {
            applicationManager->setUI( ui );
            graphicsSystem->loadObject( ui );
        }
    }

    void Application::createRenderUI()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        WP_ASSERT( applicationManager->isValid() );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            WP_LOG_ERROR( "No render system" );
            return;
        }

        auto factoryManager = graphicsSystem->getFactoryManager();

        if( auto renderUI = factoryManager->make_object<ui::IUIManager>() )
        {
            applicationManager->setRenderUI( renderUI );
            graphicsSystem->loadObject( renderUI );
        }
    }

    void Application::createScene()
    {
        // no default scene created
    }

    void Application::createTimer()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        WP_ASSERT( applicationManager->isValid() );

        auto timer = workphone::make_ptr<TimerMT>();
        timer->load( nullptr );
        applicationManager->setTimer( timer );
    }

    void Application::createPrefabManager()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        WP_ASSERT( applicationManager->isValid() );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );
        WP_ASSERT( factoryManager->isValid() );

        auto prefabManager = factoryManager->make_object<scene::IGamePrefabManager>();

        if( prefabManager )
        {
            WP_ASSERT( prefabManager->isValid() );
            applicationManager->setPrefabManager( prefabManager );
        }
    }

    void Application::createProfiler()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto profiler = factoryManager->make_object<IProfiler>();
        applicationManager->setProfiler( profiler );
    }

    void Application::createThreadPool()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        WP_ASSERT( applicationManager->isValid() );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto numThreads = static_cast<u32>( getActiveThreads() );

        auto maxThreads = Thread::hardware_concurrency();
        if( numThreads > maxThreads )
        {
            numThreads = maxThreads;
        }

        auto threadPool = factoryManager->make_ptr<ThreadPool>();
        applicationManager->setThreadPool( threadPool );

        threadPool->setNumThreads( numThreads );
        threadPool->load( nullptr );

        for( u32 i = 0; i < numThreads; ++i )
        {
            auto workerThread = threadPool->getThread( i );
            workerThread->setTargetFPS( 100.0 );
        }

        threadPool->setState( IThreadPool::State::Start );
    }

    void Application::createTaskManager()
    {
        auto applicationManager = IApplicationManager::instance();

        auto taskManager = workphone::make_ptr<TaskManager>();
        applicationManager->setTaskManager( taskManager );

        taskManager->load( nullptr );
    }

    void Application::createTasks()
    {
        try
        {
            auto applicationManager = IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            WP_ASSERT( applicationManager->isValid() );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto taskManager = applicationManager->getTaskManager();
            WP_ASSERT( taskManager );

            auto profiler = applicationManager->getProfiler();

            auto primaryTask = taskManager->getTask( TaskId::Primary );
            primaryTask->setTask( TaskId::Primary );
            primaryTask->setThreadTaskFlags( Thread::Primary_Flag );
            primaryTask->setPrimary( true );
            primaryTask->setEnabled( true );
            primaryTask->setOwner( this );
            primaryTask->setTargetFPS( 1000.0 );

            auto applicationTask = taskManager->getTask( TaskId::Application );
            applicationTask->setTask( TaskId::Application );
            applicationTask->setThreadTaskFlags( Thread::Application_Flag );
            applicationTask->setPrimary( false );
            applicationTask->setEnabled( true );
            applicationTask->setOwner( this );
            applicationTask->setTargetFPS( 100.0 );

            auto renderTask = taskManager->getTask( TaskId::Render );
            renderTask->setTask( TaskId::Render );
            renderTask->setThreadTaskFlags( Thread::Render_Flag );
            renderTask->setPrimary( true );
            renderTask->setEnabled( true );
            renderTask->setOwner( this );
            renderTask->setTargetFPS( 1000.0 );

            auto physicsTask = taskManager->getTask( TaskId::Physics );
            physicsTask->setTask( TaskId::Physics );
            physicsTask->setThreadTaskFlags( Thread::Physics_Flag );
            physicsTask->setPrimary( false );
            physicsTask->setEnabled( true );
            physicsTask->setOwner( this );
            physicsTask->setTargetFPS( 100.0 );

            if( auto inputTask = taskManager->getTask( TaskId::Input ) )
            {
                inputTask->setTask( TaskId::Input );
                inputTask->setThreadTaskFlags( Thread::Input_Flag );

                inputTask->setPrimary( false );
                inputTask->setEnabled( true );
                inputTask->setOwner( this );
                inputTask->setTargetFPS( 30.0 );

                auto profile = profiler->addProfile();
                profile->setLabel( "Input" );
                inputTask->setProfile( profile );
            }

            auto garbageCollectTask = taskManager->getTask( TaskId::GarbageCollect );
            garbageCollectTask->setTask( TaskId::GarbageCollect );
            garbageCollectTask->setThreadTaskFlags( Thread::GarbageCollect_Flag );
            garbageCollectTask->setPrimary( false );
            garbageCollectTask->setEnabled( true );
            garbageCollectTask->setOwner( this );
            garbageCollectTask->setTargetFPS( 60.0 );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Application::createJobQueue()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        WP_ASSERT( applicationManager->isValid() );

        auto factoryManager = applicationManager->getFactoryManager();

        auto jobQueue = factoryManager->make_ptr<JobQueue>();
        jobQueue->load( nullptr );
        applicationManager->setJobQueue( jobQueue );
    }

    void Application::createFsmManager()
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );
        WP_ASSERT( factoryManager->isValid() );

        auto fsmManager = factoryManager->make_ptr<FSMManager>();
        WP_ASSERT( fsmManager );
        WP_ASSERT( fsmManager->isValid() );

        fsmManager->load( nullptr );

        applicationManager->setFsmManager( fsmManager );
        WP_ASSERT( applicationManager->getFsmManager() );
    }

    void Application::createFsm()
    {
        // no finite state machine created by default
    }

    u32 Application::getActiveThreads() const
    {
        return m_activeThreads;
    }

    void Application::setActiveThreads( u32 activeThreads )
    {
        m_activeThreads = activeThreads;
    }

    SmartPtr<scene::IGameActor> Application::createSceneObject( u32 type,
                                                                SmartPtr<IBuildDirector> director )
    {
        return nullptr;
    }

    SmartPtr<scene::IGameActor> Application::createPanel( SmartPtr<IBuildDirector> director,
                                                          const String &hint, bool addToScene )
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();

        auto name = String( "Panel" );
        actor->setName( name );

        auto canvasTransform = actor->addComponent<scene::LayoutTransform>();
        if( canvasTransform )
        {
            auto size = Vector2<real_Num>( 1920, 1080 );
            canvasTransform->setSize( size );
        }

        auto image = actor->addComponent<scene::Image>();
        image->setTextureName( "panel.png" );

        return actor;
    }

    SmartPtr<scene::IGameActor> Application::createButton( const String &label,
                                                           SmartPtr<IBuildDirector> director,
                                                           const String &hint, bool addToScene )
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();
        WP_ASSERT( actor );

        auto name = String( "Button" );
        actor->setName( name );

        auto size = Vector2<real_Num>( 300, 60 );

        auto anchor = Vector2<real_Num>( 0.5f, 0.5f );

        auto canvasTransform = actor->addComponent<scene::LayoutTransform>();
        if( canvasTransform )
        {
            canvasTransform->setSize( size );

            canvasTransform->setAnchor( anchor );

            canvasTransform->setHorizontalAlignment( HorizontalAlignment::CENTER );
            canvasTransform->setVerticalAlignment( VerticalAlignment::CENTER );
            canvasTransform->updateAnchorFromAlignment();
        }

        auto button = actor->addComponent<scene::Button>();
        WP_ASSERT( button );

        button->setCascadeInput( false );

        //if( auto image = actor->addComponent<scene::Image>() )
        //{
        //    auto textureName = String( "rounded_filled_1024.png" );
        //    image->setTextureName( textureName );

        //    auto colour = ColourF::White * 0.3f;
        //    colour.a = 1.0f;

        //    image->setColour( colour );

        //    button->setImage( image );
        //}

        //auto actorText = sceneManager->createActor();
        //actor->addChild( actorText );

        //if( auto textCanvasTransform = actorText->addComponent<scene::LayoutTransform>() )
        //{
        //    auto textSize = Vector2<real_Num>( 300, 60 );
        //    textCanvasTransform->setSize( textSize );

        //    textCanvasTransform->setAnchor( anchor );

        //    textCanvasTransform->setHorizontalAlignment( HorizontalAlignment::CENTER );
        //    textCanvasTransform->setVerticalAlignment( VerticalAlignment::CENTER );
        //    textCanvasTransform->updateAnchorFromAlignment();
        //}

        //if( auto text = actorText->addComponent<scene::Text>() )
        //{
        //    text->setVerticalAlignment( 2 );
        //    text->setHorizontalAlignment( 2 );
        //    text->setText( label );

        //    auto textName = String( "Text" );
        //    actorText->setName( textName );

        //    button->setText( text );
        //    text->updateElementState();
        //}

        if( addToScene )
        {
            scene->addActor( actor );
        }

        return actor;
    }

    SmartPtr<scene::IGameActor> Application::createText( const String &label,
                                                         SmartPtr<IBuildDirector> director,
                                                         const String &hint, bool addToScene )
    {
        using namespace scene;

        auto applicationManager = IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto gameManager = applicationManager->getGameManager();
        auto gameScene = gameManager->getCurrentScene();

        auto actor = gameManager->createActor();

        const auto name = String( "Text" );
        actor->setName( name );

        auto canvasTransform = actor->addComponent<LayoutTransform>();
        if( canvasTransform )
        {
            auto size = Vector2<real_Num>( 300, 100 );
            canvasTransform->setSize( size );

            auto anchor = Vector2<real_Num>( 0.5, 0.5 );
            canvasTransform->setAnchor( anchor );

            canvasTransform->setVerticalAlignment( VerticalAlignment::CENTER );
            canvasTransform->setHorizontalAlignment( HorizontalAlignment::CENTER );
            canvasTransform->updateAnchorFromAlignment();
        }

        auto text = actor->addComponent<Text>();
        if( text )
        {
            text->setText( label );
            text->setVerticalAlignment( static_cast<u8>( VerticalAlignment::CENTER ) );
            text->setHorizontalAlignment( static_cast<u8>( HorizontalAlignment::CENTER ) );
        }

        return actor;
    }

    SmartPtr<scene::IGameActor> Application::createToggle( const String &label,
                                                           SmartPtr<IBuildDirector> director,
                                                           const String &hint, bool addToScene )
    {
        auto applicationManager = IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();

        auto name = String( "Toggle Button" );
        actor->setName( name );

        auto canvasTransform = actor->addComponent<scene::LayoutTransform>();
        if( canvasTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            canvasTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 100, 100 );
            canvasTransform->setSize( size );
        }

        auto toggle = actor->addComponent<scene::Toggle>();

        return actor;
    }

    SmartPtr<scene::IGameActor> Application::createSlider( const String &label,
                                                           SmartPtr<IBuildDirector> director,
                                                           const String &hint, bool addToScene )
    {
        auto applicationManager = IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();

        static const auto name = String( "Slider" );
        actor->setName( name );

        auto canvasTransform = actor->addComponent<scene::LayoutTransform>();
        if( canvasTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            canvasTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 200, 60 );
            canvasTransform->setSize( size );
        }

        auto slider = actor->addComponent<scene::Slider>();
        slider->setValue( 0.5f );

        return actor;
    }

    SmartPtr<scene::IGameActor> Application::createScrollbar( const String &label,
                                                              SmartPtr<IBuildDirector> director,
                                                              const String &hint, bool addToScene )
    {
        auto applicationManager = IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();

        static const auto name = String( "Slider" );
        actor->setName( name );

        auto canvasTransform = actor->addComponent<scene::LayoutTransform>();
        if( canvasTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            canvasTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 200, 60 );
            canvasTransform->setSize( size );
        }

        auto slider = actor->addComponent<scene::ScrollBar>();
        slider->setScrollValue( 0.5f );

        auto background = createPanel( nullptr, String(), false );
        background->setName( "Background" );
        actor->addChild( background );
        slider->setBackground( background );

        auto backgroundTransform = background->getComponent<scene::LayoutTransform>();
        if( backgroundTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            backgroundTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 200, 10 );
            backgroundTransform->setSize( size );

            backgroundTransform->setAnchor( Vector2<real_Num>( 0.0, 0.5 ) );

            backgroundTransform->setHorizontalAlignment( HorizontalAlignment::LEFT );
            backgroundTransform->setVerticalAlignment( VerticalAlignment::CENTER );
        }

        if( auto imageBackground = background->getComponent<scene::Image>() )
        {
            imageBackground->setColour( ColourF::White * 0.3f );
        }

        auto fill = createPanel( nullptr, String(), false );
        fill->setName( "Fill" );
        actor->addChild( fill );
        //slider->setFill( fill );

        auto fillTransform = fill->getComponent<scene::LayoutTransform>();
        if( fillTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            fillTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 200, 10 );
            fillTransform->setSize( size );

            fillTransform->setAnchor( Vector2<real_Num>( 0.0, 0.5 ) );

            fillTransform->setHorizontalAlignment( HorizontalAlignment::LEFT );
            fillTransform->setVerticalAlignment( VerticalAlignment::CENTER );
        }

        if( auto imageFill = fill->getComponent<scene::Image>() )
        {
            imageFill->setColour( ColourF::Blue );
        }

        auto handle = createPanel( nullptr, String(), false );
        handle->setName( "Handle" );
        actor->addChild( handle );
        slider->setHandleActor( handle );

        auto handleTransform = handle->getComponent<scene::LayoutTransform>();
        if( handleTransform )
        {
            auto pos = Vector2<real_Num>( 0, 0 );
            handleTransform->setPosition( pos );

            auto size = Vector2<real_Num>( 50, 25 );
            handleTransform->setSize( size );

            handleTransform->setAnchor( Vector2<real_Num>( 0.5, 0.5 ) );

            handleTransform->setHorizontalAlignment( HorizontalAlignment::LEFT );
            handleTransform->setVerticalAlignment( VerticalAlignment::CENTER );
        }

        auto imageHandle = handle->getComponent<scene::Image>();
        if( imageHandle )
        {
            imageHandle->setTextureName( "circle_filled_1024.png" );
        }

        return actor;
    }

    SmartPtr<scene::IGameActor> Application::createDefaultSky( bool addToScene )
    {
        auto applicationManager = IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();

        static const auto name = String( "Skybox" );
        actor->setName( name );

        auto skybox = actor->addComponent<scene::Skybox>();

        auto material = actor->addComponent<scene::Material>();
        if( material )
        {
            material->setMaterialPath( "DefaultSkybox.mat" );
        }

        if( addToScene )
        {
            scene->addActor( actor );
        }

        return actor;
    }

    void Application::createRigidStaticMesh( SmartPtr<scene::IGameActor> actor, bool recursive )
    {
        try
        {
            actor->setStatic( true );

            auto meshComponent = actor->getComponent<scene::Mesh>();
            if( meshComponent )
            {
                auto collisionMesh = actor->getComponent<scene::CollisionMesh>();
                if( !collisionMesh )
                {
                    collisionMesh = actor->addComponent<scene::CollisionMesh>();
                }

                if( collisionMesh )
                {
                    auto meshPath = meshComponent->getMeshPath();
                    collisionMesh->setMeshPath( meshPath );
                }

                auto rigidbody = actor->getComponent<scene::Rigidbody>();
                if( !rigidbody )
                {
                    rigidbody = actor->addComponent<scene::Rigidbody>();
                }
            }

            auto children = actor->getChildren();
            for( auto child : children )
            {
                createRigidStaticMesh( child, recursive );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Application::createRigidDynamicMesh( SmartPtr<scene::IGameActor> actor, bool recursive )
    {
        try
        {
            actor->setStatic( false );

            auto meshComponent = actor->getComponent<scene::Mesh>();
            if( meshComponent )
            {
                auto collisionMesh = actor->getComponent<scene::CollisionMesh>();
                if( !collisionMesh )
                {
                    collisionMesh = actor->addComponent<scene::CollisionMesh>();
                }

                if( collisionMesh )
                {
                    auto meshPath = meshComponent->getMeshPath();
                    collisionMesh->setMeshPath( meshPath );
                }

                auto rigidbody = actor->getComponent<scene::Rigidbody>();
                if( !rigidbody )
                {
                    rigidbody = actor->addComponent<scene::Rigidbody>();
                }
            }

            auto children = actor->getChildren();
            for( auto child : children )
            {
                createRigidDynamicMesh( child, recursive );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Application::setCreateFrameStatistics( bool bCreateFrameStatistics )
    {
        ScopedLock lock( this );
        auto newFlags = BitUtil::setFlagValue<u8>( m_applicationFlags, IApplication::frameStatisticsFlag,
                                                   bCreateFrameStatistics );
        setApplicationFlags( newFlags );
    }

    bool Application::getCreateFrameStatistics() const
    {
        ScopedLock lock( this );
        return BitUtil::getFlagValue<u8>( m_applicationFlags, IApplication::frameStatisticsFlag );
    }

    void Application::setPluginsConfigFilePath( const String &pluginsConfigFilePath )
    {
        ScopedLock lock( this );
        m_pluginsConfigFilePath = pluginsConfigFilePath;
    }

    String Application::getPluginsConfigFilePath() const
    {
        ScopedLock lock( this );
        return m_pluginsConfigFilePath;
    }

    bool Application::isDebugMode() const
    {
        ScopedLock lock( this );
        return BitUtil::getFlagValue<u8>( m_applicationFlags, IApplication::debugModeFlag );
    }

    void Application::setDebugMode( bool debugMode )
    {
        ScopedLock lock( this );
        auto newFlags =
            BitUtil::setFlagValue<u8>( m_applicationFlags, IApplication::debugModeFlag, debugMode );
        setApplicationFlags( newFlags );
    }

    void Application::unlock()
    {
        m_applicationMutex.unlock();
    }

    void Application::lock()
    {
        m_applicationMutex.lock();
    }

    bool Application::try_lock()
    {
        return m_applicationMutex.try_lock();
    }

    void Application::setRenderUIHint( const String &renderUIHint )
    {
        m_renderUIHint = renderUIHint;
    }

    String Application::getRenderUIHint() const
    {
        return m_renderUIHint;
    }

    String Application::getMediaPath() const
    {
#if defined WP_PLATFORM_WIN32
        return mediaPathStr;
#elif defined WP_PLATFORM_APPLE
        auto bundleFilePath = macBundlePath();
        if( !StringUtil::isNullOrEmpty( bundleFilePath ) )
        {
            auto mediaPath = Path::lexically_normal( bundleFilePath, mediaPathStrBundle + mediaPathStr );
            return mediaPath;
        }

        return mediaPathStrBundle + mediaPathStr;
#else
        return mediaPathStr;
#endif
    }

    u8 Application::getApplicationFlags() const
    {
        ScopedLock lock( this );
        return m_applicationFlags;
    }

    void Application::setApplicationFlags( u8 applicationFlags )
    {
        ScopedLock lock( this );
        m_applicationFlags = applicationFlags;
    }

    void Application::setViewport( SmartPtr<render::IViewport> viewport )
    {
        m_viewport = viewport;
    }

    SmartPtr<render::IViewport> Application::getViewport() const
    {
        return m_viewport;
    }

    void Application::setCamera( SmartPtr<render::IGraphicsCamera> camera )
    {
        m_camera = camera;
    }

    SmartPtr<render::IGraphicsCamera> Application::getCamera() const
    {
        return m_camera;
    }

    void Application::ApplicationEventListener::setOwner( SmartPtr<Application> owner )
    {
        m_owner = owner;
    }

    SmartPtr<Application> Application::ApplicationEventListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    Parameter Application::ApplicationEventListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( auto owner = getOwner() )
        {
            owner->handleEvent( eventType, eventValue, arguments, sender, object, event );
        }

        return {};
    }

    void Application::ApplicationEventListener::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        m_owner = nullptr;
        setLoadingState( LoadingState::Unloaded );
    }

    Application::ApplicationEventListener::ApplicationEventListener() = default;

    Application::ApplicationEventListener::~ApplicationEventListener() = default;
}  // namespace workphone::core
