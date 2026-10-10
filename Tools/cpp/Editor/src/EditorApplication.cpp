#include <EditorPCH.hpp>
#include <EditorApplication.hpp>
#include <commands/AddActorCmd.hpp>
#include <commands/AddComponentCmd.hpp>
#include <commands/PromptCmd.hpp>
#include <commands/DuplicateSelectionCmd.hpp>
#include <commands/PasteSelectionCmd.hpp>
#include <commands/RemoveSelectionCmd.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <jobs/JobRendererSetup.hpp>
#include <jobs/PlaymodeJob.hpp>
#include <jobs/SceneDropJob.hpp>
#include <jobs/LeavePlaymodeJob.hpp>
#include <jobs/FileSelectedJob.hpp>
#include <ui/ObjectWindow.hpp>
#include <ui/PropertiesWindow.hpp>
#include <ui/ActorWindow.hpp>
#include <ui/ProjectWindow.hpp>
#include <ui/ProjectTreeData.hpp>
#include <ui/SceneWindow.hpp>
#include <ui/AnimationWindow.hpp>
#include <ui/TransformWindow.hpp>
#include <ui/TerrainWindow.hpp>
#include <ui/UIManager.hpp>
#include <ui/ProjectAssetsWindow.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Graphics/IGraphicsPipeline.hpp>

#ifdef _WP_STATIC_LIB_
#    include <FBRenderUI/FBRenderUI.hpp>
#    include <FBAssimp/FBAssimp.hpp>
#    include <FBOISInput/FBOISInput.hpp>

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
#        include <WPGraphicsOgreNext/WPGraphicsOgreNext.hpp>
#    elif WP_GRAPHICS_SYSTEM_OGRE
#        include <WPGraphicsOgre/WPGraphicsOgre.hpp>
#    endif

#    if WP_BUILD_AUDIO
#        include <WPAudio/WPAudio.hpp>
#    endif

#    if WP_BUILD_PHYSX
#        include <FBPhysx/FBPhysx.hpp>
#    endif

#    if WP_BUILD_IMGUI
#        include <FBImGui/FBImGui.hpp>
#    endif

#    ifdef WP_BUILD_SQLITE
#        include <WPSQLite/WPSQLite.hpp>
#    endif

#    if WP_ENABLE_LUA
#        include <FBLua/FBLua.hpp>
#    elif WP_ENABLE_PYTHON
#        include <FBPython/FBPython.hpp>
#    endif
#endif

namespace workphone::editor
{

    WP_CLASS_REGISTER_DERIVED( workphone::editor, EditorApplication, core::Application );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, EditorApplication::ApplicationListener,
                               IEventListener );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, EditorApplication::ApplicationStateListener,
                               IStateListener );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, EditorApplication::ApplicationFSMListener,
                               FSMListener );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, EditorApplication::EventListener, IEventListener );

    const String EditorApplication::scriptsFolderStr = String( "/Scripts" );

    EditorApplication::EditorApplication()
    {
        static const String applicationName = "LioncatEditor";
        setName( applicationName );

#if WP_EDITOR_TESTS
        auto flags = getApplicationFlags();
        setApplicationFlags( flags | core::IApplication::developerModeFlag );
#endif
    }

    EditorApplication::~EditorApplication()
    {
        try
        {
            unload( nullptr );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void EditorApplication::loadDebug( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto currentThreadId = Thread::ThreadId::Primary;
            Thread::setCurrentThreadId( currentThreadId );

            auto task = TaskId::Primary;
            Thread::setCurrentTask( task );

            auto taskFlags = std::numeric_limits<u32>::max();
            Thread::setTaskFlags( taskFlags );

            auto applicationManager = workphone::make_ptr<core::ApplicationManager>();
            core::ApplicationManager::setInstance( applicationManager );
            m_applicationManager = applicationManager;

            Application::load( data );

            ApplicationUtil::createDefaultFont();
            ApplicationUtil::createDefaultMaterials();

            auto meshManager = workphone::make_ptr<MeshManager>();
            applicationManager->setMeshManager( meshManager );

            auto commandMgr = workphone::make_ptr<CommandManagerMT>();
            applicationManager->setCommandManager( commandMgr );

            auto prefabManager = workphone::make_ptr<scene::GamePrefabManager>();
            applicationManager->setPrefabManager( prefabManager );

            auto selectionManager = workphone::make_ptr<SelectionManager>();
            applicationManager->setSelectionManager( selectionManager );

            applicationManager->setEditor( true );
            applicationManager->setEditorCamera( true );

            if( auto cameraManager = applicationManager->getCameraManager() )
            {
                cameraManager->setEnabled( false );
            }

            loadSettings();

            auto packageManager = workphone::make_ptr<PackageManager>();
            applicationManager->setPackageManager( packageManager );

            m_editorManager = workphone::make_ptr<EditorManager>();
            m_editorManager->load( nullptr );
            EditorManager::setSingletonPtr( m_editorManager );
            applicationManager->setEditorManager( m_editorManager );

            auto projectManager = workphone::make_ptr<ProjectManager>();
            projectManager->load( nullptr );
            m_editorManager->setProjectManager( projectManager );

            auto pProject = workphone::make_ptr<Project>();
            pProject->load( nullptr );
            m_editorManager->setProject( pProject );

            auto editorUiManager = workphone::make_ptr<UIManager>();
            m_editorManager->setUI( editorUiManager );
            editorUiManager->load( nullptr );

            if( !m_editorApplicationListener )
            {
                m_editorApplicationListener = workphone::make_ptr<ApplicationListener>( this );
            }
            applicationManager->addObjectListener( m_editorApplicationListener );

            createDefaultSky();
            //createDirectionalLight();
            // ApplicationUtil::createDefaultCube();
            //ApplicationUtil::createDefaultTerrain();
            //  ApplicationUtil::createDefaultPlane();
            //  ApplicationUtil::createDefaultVehicle();
            //  ApplicationUtil::createProceduralTest();
            //  ApplicationUtil::createOverlayPanelTest();

            auto gameManager = applicationManager->getGameManager();
            gameManager->edit();

            auto taskManager = applicationManager->getTaskManager();
            taskManager->setState( ITaskManager::State::FreeStep );

            auto threadPool = applicationManager->getThreadPool();
            threadPool->setState( IThreadPool::State::Start );

            if( auto camera = m_camera )
            {
                camera->setRenderUI( true );
            }

            if( auto vp = m_viewport )
            {
                vp->setEnableUI( true );
                vp->setEnableSceneRender( false );
            }

            if( m_cameraActor )
            {
                if( auto camera = m_cameraActor->getComponent<scene::Camera>() )
                {
                    camera->setActive( true );
                }
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void EditorApplication::loadEditor( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto currentThreadId = Thread::ThreadId::Primary;
            Thread::setCurrentThreadId( currentThreadId );

            auto task = TaskId::Primary;
            Thread::setCurrentTask( task );

            auto taskFlags = Thread::Primary_Flag;
            Thread::setTaskFlags( taskFlags );

            auto applicationManager = workphone::make_ptr<core::ApplicationManager>();

#ifdef WP_PLATFORM_APPLE
            auto macBundlePath = Path::macBundlePath();
            auto path = Path::getAbsolutePath( macBundlePath, "../" );
            Path::setWorkingDirectory( path );
#endif

            core::ApplicationManager::setInstance( applicationManager );
            m_applicationManager = applicationManager;
            applicationManager->setApplication( this );
            applicationManager->load( nullptr );

            Application::load( data );

            applicationManager->setEditor( true );
            applicationManager->setEditorCamera( true );

            if( auto cameraManager = applicationManager->getCameraManager() )
            {
                cameraManager->setEnabled( true );
            }

            auto meshManager = workphone::make_ptr<MeshManager>();
            applicationManager->setMeshManager( meshManager );

            auto commandMgr = workphone::make_ptr<CommandManagerMT>();
            applicationManager->setCommandManager( commandMgr );

            auto prefabManager = workphone::make_ptr<scene::GamePrefabManager>();
            applicationManager->setPrefabManager( prefabManager );

            auto selectionManager = workphone::make_ptr<SelectionManager>();
            applicationManager->setSelectionManager( selectionManager );

            ApplicationUtil::createDefaultFont();
            ApplicationUtil::createDefaultMaterials();

            loadSettings();

            auto packageManager = workphone::make_ptr<PackageManager>();
            applicationManager->setPackageManager( packageManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto stateManager = applicationManager->getStateManager();
            WP_ASSERT( stateManager );

            m_stateContext = stateManager->addStateContext();

            auto applicationStateListener = factoryManager->make_ptr<ApplicationStateListener>();
            applicationStateListener->setOwner( this );
            m_stateListener = applicationStateListener;
            m_stateContext->addStateListener( m_stateListener );

            m_editorManager = workphone::make_ptr<EditorManager>();
            m_editorManager->load( nullptr );
            EditorManager::setSingletonPtr( m_editorManager );
            applicationManager->setEditorManager( m_editorManager );

            auto projectManager = workphone::make_ptr<ProjectManager>();
            projectManager->load( nullptr );
            m_editorManager->setProjectManager( projectManager );

            auto pProject = workphone::make_ptr<Project>();
            pProject->load( nullptr );
            m_editorManager->setProject( pProject );

            auto editorUiManager = workphone::make_ptr<UIManager>();
            m_editorManager->setUI( editorUiManager );
            editorUiManager->load( nullptr );

            // ImGuiApplication uses these objects as the editor's active transform-mode state.
            // Their legacy scene debug rendering remains hidden; ImGuizmo supplies the visuals.
            auto translateManipulator = factoryManager->make_ptr<TranslateManipulator>();
            translateManipulator->load( nullptr );
            translateManipulator->setVisible( false );
            translateManipulator->setEnabled( false );
            m_editorManager->setTranslateManipulator( translateManipulator );

            auto rotateManipulator = factoryManager->make_ptr<RotateManipulator>();
            rotateManipulator->load( nullptr );
            rotateManipulator->setVisible( false );
            rotateManipulator->setEnabled( false );
            m_editorManager->setRotateManipulator( rotateManipulator );

            auto scaleManipulator = factoryManager->make_ptr<ScaleManipulator>();
            scaleManipulator->load( nullptr );
            scaleManipulator->setVisible( false );
            scaleManipulator->setEnabled( false );
            m_editorManager->setScaleManipulator( scaleManipulator );

            createDefaultSky();
            createDirectionalLight();
            // ApplicationUtil::createDefaultCube();
            //ApplicationUtil::createDefaultTerrain();
            //  ApplicationUtil::createDefaultPlane();
            //  ApplicationUtil::createDefaultVehicle();
            //  ApplicationUtil::createProceduralTest();
            //  ApplicationUtil::createOverlayPanelTest();

            //if( m_cameraActor )
            //{
            //    if( auto camera = m_cameraActor->getComponent<scene::Camera>() )
            //    {
            //        camera->setActive( true );
            //    }
            //}

            auto taskManager = applicationManager->getTaskManager();
            taskManager->setState( ITaskManager::State::FreeStep );

            auto threadPool = applicationManager->getThreadPool();
            threadPool->setState( IThreadPool::State::Start );

            auto projectWindow = editorUiManager->getProjectWindow();
            if( projectWindow )
            {
                projectWindow->buildTree();
            }

            if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
            {
                auto window = graphicsSystem->getDefaultWindow();
                if( window )
                {
                    const auto windowName = String( "Workphone" );
                    window->setTitle( windowName );
                    window->maximize();
                }
            }

            if( m_cameraActor )
            {
#if 1
                auto sphericalCamera = m_cameraActor->addComponent<scene::SphericalCameraController>();
                if( sphericalCamera )
                {
                    sphericalCamera->setSphericalCoords( Vector3<real_Num>( 5.0, 0.0, 2.0 ) );
                    m_sphericalCamera = sphericalCamera;
                    sphericalCamera->setUiWindow( m_renderWindow );
                }
#else
                auto sphericalCamera = m_cameraActor->addComponent<scene::EditorCameraController>();
                if( sphericalCamera )
                {
                    sphericalCamera->setUiWindow( m_renderWindow );
                }
#endif

                if( auto camera = m_cameraActor->getComponent<scene::Camera>() )
                {
                    camera->setEnableSceneRender( true );
                    camera->setEnableUI( true );
                    camera->setActive( true );
                }
            }

            if( auto uiManager = applicationManager->getUI() )
            {
                uiManager->setMainWindow( m_renderWindow );
            }

            applicationManager->setSceneRenderWindow( m_renderWindow );

            if( auto gameManager = applicationManager->getGameManager() )
            {
                gameManager->edit();
            }

            if( auto vp = m_viewport )
            {
                vp->setEnableUI( true );
                vp->setEnableSceneRender( false );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void EditorApplication::createDebugText()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto debug = graphicsSystem->getDebug();

        debug->drawText( 0, Vector2F::unit() * 0.5f, "Hello world!", 0 );
    }

    void EditorApplication::load( SmartPtr<ISharedObject> data )
    {
        if( isDebugMode() )
        {
            loadDebug( data );
        }
        else
        {
            loadEditor( data );
        }
    }

    void EditorApplication::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                if( m_editorApplicationListener )
                {
                    applicationManager->removeObjectListener( m_editorApplicationListener );
                    if( auto inputManager = applicationManager->getInputDeviceManager() )
                    {
                        inputManager->removeListener( m_editorApplicationListener );
                    }
                    m_editorApplicationListener = nullptr;
                }

                if( auto jobQueue = applicationManager->getJobQueue() )
                {
                    jobQueue->shutdown();
                }

                if( auto threadPool = applicationManager->getThreadPool() )
                {
                    threadPool->stop();
                }

                if( auto taskManager = applicationManager->getTaskManager() )
                {
                    taskManager->shutdown();
                }

                // Event jobs can retain listeners and UI objects. Release them before the
                // editor manager tears those objects down; clearing them later during the base
                // application teardown would dereference stale UI state.
                applicationManager->clearAllEvents();

                if( auto manipulator = m_editorManager->getTranslateManipulator() )
                {
                    manipulator->unload( nullptr );
                    m_editorManager->setTranslateManipulator( nullptr );
                }

                if( auto manipulator = m_editorManager->getRotateManipulator() )
                {
                    manipulator->unload( nullptr );
                    m_editorManager->setRotateManipulator( nullptr );
                }

                if( auto manipulator = m_editorManager->getScaleManipulator() )
                {
                    manipulator->unload( nullptr );
                    m_editorManager->setScaleManipulator( nullptr );
                }

                applicationManager->setWindow( nullptr );

                if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
                {
                    if( auto textureManager = graphicsSystem->getTextureManager() )
                    {
                        if( m_renderTarget )
                        {
                            /*
                            if( auto renderTarget = m_renderTarget->getRenderTarget() )
                            {
                                if( auto vp = getRttViewport() )
                                {
                                    vp->setCamera( nullptr );

                                    renderTarget->removeViewport( vp );
                                    setRttViewport( nullptr );
                                }
                            }*/
                        }
                    }
                }

                if( auto ui = applicationManager->getUI() )
                {
                    if( m_application )
                    {
                        ui->removeApplication( m_application );
                        m_application = nullptr;
                    }

                    if( m_renderWindow )
                    {
                        m_renderWindow->setWindow( nullptr );
                        m_renderWindow->setRenderTexture( nullptr );

                        ui->removeElement( m_renderWindow );
                        m_renderWindow = nullptr;
                    }
                }

                const auto editorSettings = applicationManager->getEditorSettings();

                const auto dataStr = DataUtil::toString( editorSettings.get(), true );

                static String settingsFileName = "LioncatEditor.settings";

#if defined WP_PLATFORM_APPLE
                auto settingsPath = macGetConfigFilePath();
                auto settingsFilePath = settingsPath + "/" + settingsFileName;
#else
                auto settingsFilePath = settingsFileName;
#endif
                Path::writeAllText( settingsFilePath, dataStr );

                if( auto cameraManager = applicationManager->getCameraManager() )
                {
                    cameraManager->unload( nullptr );
                    applicationManager->setCameraManager( nullptr );
                }

                if( m_sphericalCamera )
                {
                    m_sphericalCamera->unload( nullptr );
                    m_sphericalCamera = nullptr;
                }

                if( auto stateContext = getStateContext() )
                {
                    if( auto stateListener = getStateListener() )
                    {
                        stateContext->removeStateListener( stateListener );
                        setStateListener( nullptr );
                    }

                    if( auto stateManager = applicationManager->getStateManager() )
                    {
                        stateManager->removeStateContext( stateContext );
                    }

                    setStateContext( nullptr );
                }

                if( auto sceneManager = applicationManager->getGameManager() )
                {
                    if( m_cameraActor )
                    {
                        sceneManager->destroyActor( m_cameraActor );
                        m_cameraActor = nullptr;
                    }
                }

                if( auto camera = getCamera() )
                {
                    if( auto owner = camera->getOwner() )
                    {
                        owner->detachObject( camera );
                    }
                }

                if( auto node = getCameraSceneNode() )
                {
                    if( auto creator = node->getCreator() )
                    {
                        creator->removeSceneNode( node );
                    }
                }

                if( auto sceneManager = applicationManager->getGameManager() )
                {
                    if( auto currentScene = sceneManager->getCurrentScene() )
                    {
                        currentScene->clear();
                    }

                    if( sceneManager )
                    {
                        sceneManager->unload( nullptr );
                    }
                }

                if( m_editorManager )
                {
                    m_editorManager->unload( nullptr );
                    m_editorManager = nullptr;
                }

                EditorManager::setSingletonPtr( nullptr );

                m_camera = nullptr;
                m_cameraSceneNode = nullptr;

                if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
                {
                    if( m_viewport )
                    {
                        if( auto window = graphicsSystem->getDefaultWindow() )
                        {
                            m_viewport->setCamera( nullptr );
                            window->removeViewport( m_viewport );
                        }

                        m_viewport = nullptr;
                    }
                }

                if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
                {
                    if( auto textureManager = graphicsSystem->getTextureManager() )
                    {
                        if( m_renderTarget )
                        {
                            textureManager->destroyRenderTexture( m_renderTarget );
                            m_renderTarget = nullptr;
                        }
                    }
                }

                Application::unload( nullptr );

                core::IApplicationManager::setInstance( nullptr );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void EditorApplication::run()
    {
        auto currentThreadId = Thread::ThreadId::Primary;
        Thread::setCurrentThreadId( currentThreadId );

        auto task = TaskId::Primary;
        Thread::setCurrentTask( task );

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto timer = applicationManager->getTimerPtr();
        WP_ASSERT( timer );

        timer->reset();

        auto targetRate = 1.0 / 240.0;

        while( applicationManager->isRunning() && !applicationManager->getQuit() )
        {
            WP_ASSERT( timer );

            auto startTime = timer->now();

            iterate();

            auto endTime = timer->now();
            auto timeTaken = endTime - startTime;
            auto sleepTime = targetRate - timeTaken;
            if( sleepTime > 0.0 && sleepTime < 1.0 )
            {
                Thread::sleep( sleepTime );
            }
        }
    }

    void EditorApplication::iterate()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto fsmManager = applicationManager->getFsmManagerPtr();
        WP_ASSERT( fsmManager );

        auto taskManager = applicationManager->getTaskManagerPtr();
        WP_ASSERT( taskManager );

        try
        {
            if( fsmManager )
            {
                fsmManager->update();
            }

            if( taskManager )
            {
                taskManager->update();
            }

            Thread::yield();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void EditorApplication::update()
    {
        if( isLoaded() )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto timer = applicationManager->getTimerPtr();
            WP_ASSERT( timer );

            auto stateManager = applicationManager->getStateManagerPtr();
            WP_ASSERT( stateManager );

            auto soundManager = applicationManager->getSoundManagerPtr();
            WP_ASSERT( stateManager );

            auto cameraManager = applicationManager->getCameraManager();

            auto sceneManager = applicationManager->getGameManager();

            auto applicationFSM = getFSM();

            auto task = Thread::getCurrentTask();
            auto t = timer->getTime();
            auto dt = timer->getDeltaTime();

            auto editorManager = EditorManager::getSingletonPtr();

            if( sceneManager )
            {
                sceneManager->preUpdate();
            }

            auto inputManager = applicationManager->getInputDeviceManager();
            if( inputManager )
            {
                inputManager->preUpdate();
            }

            if( stateManager )
            {
                stateManager->preUpdate();
            }

            if( soundManager )
            {
                soundManager->preUpdate();
            }

            if( cameraManager )
            {
                cameraManager->update();
            }

            if( stateManager )
            {
                stateManager->update();
            }

            if( sceneManager )
            {
                sceneManager->update();
            }

            if( auto scriptManager = applicationManager->getScriptManager() )
            {
                scriptManager->update();
            }

            if( auto inputManager = applicationManager->getInputDeviceManager() )
            {
                inputManager->update();
            }

            if( soundManager )
            {
                soundManager->update();
            }

            switch( task )
            {
            case TaskId::GarbageCollect:
            {
            }
            break;
            case TaskId::Primary:
            {
                // Scripts can request a return to edit mode through the shared
                // application interface. Complete the same snapshot restoration
                // used by the toolbar before accepting another Play request.
                if( !applicationManager->isPlaying() && editorManager &&
                    editorManager->getPlayModeSceneData() )
                {
                    auto job = workphone::make_ptr<LeavePlaymodeJob>();
                    job->execute();
                }

                auto jobQueue = applicationManager->getJobQueue();
                if( jobQueue )
                {
                    jobQueue->preUpdate();
                }

                if( jobQueue )
                {
                    jobQueue->update();
                }

                if( t > 3.0 )
                {
                    if( applicationFSM->getState<State>() != State::Editor )
                    {
                        applicationFSM->setState( State::Editor );
                    }
                }

                if( jobQueue )
                {
                    jobQueue->postUpdate();
                }

                if( editorManager )
                {
                    if( auto ui = editorManager->getUI() )
                    {
                        if( auto toggle = ui->getPlaymodeToggle() )
                        {
                            toggle->setValue( applicationManager->isPlaying() );
                        }
                        if( auto toggle = ui->getEditorCameraToggle() )
                        {
                            toggle->setValue( applicationManager->isEditorCamera() );
                        }
                    }
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
                    if( graphicsSystem )
                    {
                        if( auto window = graphicsSystem->getDefaultWindow() )
                        {
                            if( !window->isClosed() )
                            {
                                if( stateManager )
                                {
                                    stateManager->update();
                                }

                                if( graphicsSystem )
                                {
                                    graphicsSystem->update();
                                }
                            }
                        }
                    }

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
            case TaskId::Physics:
            {
                if( timer->getTimeSinceSceneLoad() > 0.1 )
                {
                    if( auto physicsManager = applicationManager->getPhysicsManager() )
                    {
                        physicsManager->preUpdate();
                        physicsManager->update();
                        physicsManager->postUpdate();

                        if( auto physicsScene = physicsManager->getPhysicsScene() )
                        {
                            physicsScene->update();
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
            break;
            case TaskId::Application:
            {
                if( editorManager )
                {
                    // The input panel is a ScriptWindow, so its Lua refresh must
                    // run explicitly on the application task, including in edit mode.
                    if( auto editorUI = editorManager->getUI() )
                    {
                        if( auto actorWindow = editorUI->getActorWindow() )
                        {
                            auto propertiesWindow = actorWindow->getPropertiesWindow();
                            if( propertiesWindow && propertiesWindow->isLoaded() )
                                propertiesWindow->update();
                        }
                        if( auto propertiesWindow = editorUI->getPropertiesWindow() )
                        {
                            if( propertiesWindow->isLoaded() )
                                propertiesWindow->update();
                        }
                        auto inputWindow = editorUI->getInputManagerWindow();
                        if( inputWindow && inputWindow->isLoaded() && inputWindow->isWindowVisible() )
                        {
                            if( auto invoker = inputWindow->getInvoker() )
                            {
                                invoker->callObjectMember( "update" );
                            }
                        }
                    }

                    if( auto translateManipulator = editorManager->getTranslateManipulator() )
                    {
                        translateManipulator->preUpdate();
                        translateManipulator->update();
                        translateManipulator->postUpdate();
                    }

                    if( auto rotateManipulator = editorManager->getRotateManipulator() )
                    {
                        rotateManipulator->preUpdate();
                        rotateManipulator->update();
                        rotateManipulator->postUpdate();
                    }

                    if( auto scaleManipulator = editorManager->getScaleManipulator() )
                    {
                        scaleManipulator->preUpdate();
                        scaleManipulator->update();
                        scaleManipulator->postUpdate();
                    }
                }
            }
            break;
            default:
            {
            }
            }

            if( sceneManager )
            {
                sceneManager->postUpdate();
            }

            if( soundManager )
            {
                soundManager->postUpdate();
            }
        }
    }

    bool EditorApplication::inputEvent( SmartPtr<IInputEvent> event )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto ui = applicationManager->getUI();

        auto editorManager = EditorManager::getSingletonPtr();

        if( m_application )
        {
            m_application->handleInputEvent( event );
        }

        if( auto translateManipulator = editorManager->getTranslateManipulator() )
        {
            if( translateManipulator->handleEvent( event ) )
            {
                return true;
            }
        }

        if( auto rotateManipulator = editorManager->getRotateManipulator() )
        {
            if( rotateManipulator->handleEvent( event ) )
            {
                return true;
            }
        }

        if( auto scaleManipulator = editorManager->getScaleManipulator() )
        {
            if( scaleManipulator->handleEvent( event ) )
            {
                return true;
            }
        }

        if( m_sphericalCamera )
        {
            if( m_sphericalCamera->handleEvent( event ) )
            {
                return true;
            }
        }

        return false;
    }

    FSMReturnType EditorApplication::handleEvent( u32 state, FSMEvent eventType )
    {
        switch( eventType )
        {
        case FSMEvent::Change:
        {
        }
        break;
        case FSMEvent::Enter:
        {
            switch( auto eState = static_cast<State>( state ) )
            {
            case State::Editor:
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto editorSettings = applicationManager->getEditorSettings();

                auto editorManager = EditorManager::getSingletonPtr();

                if( editorSettings )
                {
                    auto projectFilePath = String();
                    if( editorSettings->getPropertyValue( "Project Path", projectFilePath ) )
                    {
                        if( !StringUtil::isNullOrEmpty( projectFilePath ) )
                        {
                            if( Path::isExistingFile( projectFilePath ) )
                            {
                                editorManager->loadProject( projectFilePath );
                            }
                        }
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
        case FSMEvent::Leave:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Editor:
            {
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

    size_t EditorApplication::getWindowHandle() const
    {
        return m_windowHandle;
    }

    void EditorApplication::setWindowHandle( size_t windowHandle )
    {
        m_windowHandle = windowHandle;
    }

    void EditorApplication::enterPlayMode()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto jobQueue = applicationManager->getJobQueue();

        auto job = workphone::make_ptr<PlaymodeJob>();
        jobQueue->addJob( job );
    }

    void EditorApplication::stopPlayMode()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto jobQueue = applicationManager->getJobQueue();

        auto job = workphone::make_ptr<LeavePlaymodeJob>();
        jobQueue->addJob( job );
    }

    void EditorApplication::createLogManager()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto logManager = workphone::make_ptr<LogManagerDefault>();
        applicationManager->setLogManager( logManager );

        String logPath;

#if defined WP_PLATFORM_APPLE
        logPath = macGetLogFilePath();
#endif

        static const String logFileName = "LioncatEditor.log";
        String logFilePath;

        if( !logPath.empty() )
            logFilePath = logPath + "/" + logFileName;
        else
            logFilePath = logFileName;

        logManager->open( logFilePath );

        WP_ASSERT( logManager->isValid() );
        WP_ASSERT( applicationManager->isValid() );
    }

    void EditorApplication::createFactoryManager()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = workphone::make_ptr<FactoryManager>();
        factoryManager->load( nullptr );
        applicationManager->setFactoryManager( factoryManager );
        WP_ASSERT( applicationManager->getFactoryManager() );

        //Util::createFactories();
        FactoryUtil::addFactory<AddActorCmd>();
        FactoryUtil::addFactory<AddComponentCmd>();
        FactoryUtil::addFactory<PromptCmd>();
        FactoryUtil::addFactory<RemoveSelectionCmd>();
        FactoryUtil::addFactory<DuplicateSelectionCmd>();
        FactoryUtil::addFactory<PasteSelectionCmd>();

        FactoryUtil::addFactory<FileSelectedJob>();
        FactoryUtil::addFactory<SceneDropJob>();
        FactoryUtil::addFactory<ProjectTreeData>();

        FactoryUtil::addFactory<ObjectWindow>();
        FactoryUtil::addFactory<AnimationWindow>();
        FactoryUtil::addFactory<AnimationWindow::WindowListener>();
        FactoryUtil::addFactory<ProjectAssetsWindow>();
        FactoryUtil::addFactory<SceneWindow>();
        FactoryUtil::addFactory<TerrainWindow>();
        FactoryUtil::addFactory<TransformWindow>();
        FactoryUtil::addFactory<TransformWindow::VectorListener>();

        FactoryUtil::addFactory<ProjectAssetsWindow::BuildTreeJob>();

        const auto messagePoolSize = 4096;

        factoryManager->setPoolSizeByType<AddActorCmd>( 4 );
        factoryManager->setPoolSizeByType<AddComponentCmd>( 4 );
        factoryManager->setPoolSizeByType<PromptCmd>( 4 );
        factoryManager->setPoolSizeByType<RemoveSelectionCmd>( 4 );
        factoryManager->setPoolSizeByType<DuplicateSelectionCmd>( 4 );
        factoryManager->setPoolSizeByType<PasteSelectionCmd>( 4 );

        factoryManager->setPoolSizeByType<FileSelectedJob>( 4 );
        factoryManager->setPoolSizeByType<SceneDropJob>( 4 );

        factoryManager->setPoolSizeByType<ObjectWindow>( 1 );
        factoryManager->setPoolSizeByType<AnimationWindow>( 1 );
        factoryManager->setPoolSizeByType<AnimationWindow::WindowListener>( 1 );
        factoryManager->setPoolSizeByType<ProjectAssetsWindow>( 1 );
        factoryManager->setPoolSizeByType<SceneWindow>( 1 );
        factoryManager->setPoolSizeByType<TerrainWindow>( 1 );
        factoryManager->setPoolSizeByType<TransformWindow>( 1 );
        factoryManager->setPoolSizeByType<TransformWindow::VectorListener>( 3 );

        factoryManager->setPoolSizeByType<ProjectTreeData>( messagePoolSize );
        //factoryManager->setPoolSizeByType<ProjectAssetsWindow::BuildTreeJob>( 2 );
    }

    void EditorApplication::createPlugins()
    {
#ifndef _WP_STATIC_LIB_
        const String pluginsConfigFilePath = "wp_plugins_editor.cfg";
        setPluginsConfigFilePath( pluginsConfigFilePath );

        Application::createPlugins();
#else
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto corePlugin = workphone::make_ptr<WPCore>();
        applicationManager->addPlugin( corePlugin );

        auto databasePlugin = workphone::make_ptr<SQLitePlugin>();
        applicationManager->addPlugin( databasePlugin );

        auto inputPlugin = workphone::make_ptr<OISInput>();
        applicationManager->addPlugin( inputPlugin );

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
        auto graphicsPlugin = workphone::make_ptr<render::WPGraphicsOgreNext>();
        applicationManager->addPlugin( graphicsPlugin );
#    elif WP_GRAPHICS_SYSTEM_OGRE
        auto graphicsPlugin = workphone::make_ptr<render::WPGraphicsOgre>();
        applicationManager->addPlugin( graphicsPlugin );
#    endif

        auto imguiPlugin = workphone::make_ptr<ui::FBImGui>();
        applicationManager->addPlugin( imguiPlugin );

#    if WP_BUILD_AUDIO
        auto audioPlugin = workphone::make_ptr<WPAudio>();
        applicationManager->addPlugin( audioPlugin );
#    endif

        auto meshLoaderPlugin = workphone::make_ptr<FBAssimp>();
        applicationManager->addPlugin( meshLoaderPlugin );

#    if WP_ENABLE_LUA
        auto lua = workphone::make_ptr<FBLua>();
        applicationManager->addPlugin( lua );
#    elif WP_ENABLE_PYTHON
        auto python = workphone::make_ptr<FBPython>();
        applicationManager->addPlugin( python );
#    endif

        const String pluginsConfigFilePath = "wp_plugins_editor_static.cfg";
        setPluginsConfigFilePath( pluginsConfigFilePath );

        Application::createPlugins();
#endif
    }

    void EditorApplication::createTimer()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto timer = workphone::make_ptr<TimerMT>();
        timer->load( nullptr );
        applicationManager->setTimer( timer );
    }

    void EditorApplication::createFsmManager()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );
        WP_ASSERT( applicationManager->isValid() );

        auto fsmManager = workphone::make_ptr<FSMManager>();
        fsmManager->setGrowSize( 32 );
        fsmManager->load( nullptr );
        WP_ASSERT( fsmManager->isLoaded() );
        WP_ASSERT( fsmManager->isValid() );

        applicationManager->setFsmManager( fsmManager );
        WP_ASSERT( applicationManager->isValid() );
    }

    void EditorApplication::createFsm()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto fsmManager = applicationManager->getFsmManager();
        WP_ASSERT( fsmManager );

        auto fsm = fsmManager->createFSM();
        setFSM( fsm );

        auto applicationFSMListener = workphone::make_ptr<ApplicationFSMListener>();
        applicationFSMListener->setOwner( this );
        fsm->addListener( applicationFSMListener );
    }

    void EditorApplication::createTaskManager()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto taskManager = workphone::make_ptr<TaskManager>();
        applicationManager->setTaskManager( taskManager );

        taskManager->load( nullptr );
    }

    void EditorApplication::createThreadPool()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto numThreads = getActiveThreads();

        auto maxThreads = Thread::hardware_concurrency();
        if( numThreads > maxThreads )
        {
            numThreads = maxThreads;
        }

        auto threadPool = workphone::make_ptr<ThreadPool>();
        applicationManager->setThreadPool( threadPool );

        threadPool->setNumThreads( static_cast<u32>( numThreads ) );
        threadPool->load( nullptr );

        for( u32 i = 0; i < numThreads; ++i )
        {
            auto workerThread = threadPool->getThread( i );
            if( workerThread )
            {
                auto framesPerSecond = 75.0;
                workerThread->setTargetFPS( framesPerSecond );
            }
        }
    }

    void EditorApplication::createStateManager()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto stateManager = workphone::make_ptr<StateManager>();
        stateManager->load( nullptr );
        applicationManager->setStateManager( stateManager );
    }

    void EditorApplication::createSceneManager()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto sceneManager = factoryManager->make_ptr<scene::GameManager>();
        applicationManager->setGameManager( sceneManager );
        sceneManager->load( nullptr );

        auto scene = factoryManager->make_ptr<scene::GameScene>();
        scene->load( nullptr );
        scene->setLabel( "Untitled" );
        sceneManager->setCurrentScene( scene );
    }

    void EditorApplication::createFileSystem()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto fileSystem = factoryManager->make_object<IFileSystem>();
            WP_ASSERT( fileSystem );

            fileSystem->load( nullptr );

            applicationManager->setFileSystem( fileSystem );

            auto workingDirectory = Path::getWorkingDirectory();

#ifdef WP_PLATFORM_APPLE
            auto bundlePath = Path::macBundlePath();
#endif

            auto projectPath = applicationManager->getProjectPath();
            fileSystem->addFolder( projectPath );

            fileSystem->addFolder( workingDirectory );

            auto cachePath = applicationManager->getCachePath();
            auto cacheAbsolutePath = Path::getAbsolutePath( workingDirectory, cachePath );
            fileSystem->addFolder( cacheAbsolutePath );

            auto settingsCachePath = applicationManager->getSettingsPath();
            auto settingsCacheAbsolutePath =
                Path::getAbsolutePath( workingDirectory, settingsCachePath );
            fileSystem->addFolder( settingsCacheAbsolutePath );

            // add archives
            Array<String> fileNames;
            fileSystem->getFileNamesInFolder( String( "./" ), fileNames );
            for( const auto &fileName : fileNames )
            {
                auto ext = Path::getFileExtension( fileName );
                if( ext == ".fba" )
                {
                    fileSystem->addFileArchive( fileName, true, true,
                                                IFileSystem::ArchiveType::ObfuscatedZip, "" );
                }
            }

            auto applicationFlags = getApplicationFlags();
            if( BitUtil::getFlagValue<u8>( applicationFlags, IApplication::developerModeFlag ) )
            {
#if defined WP_PLATFORM_WIN32
                auto mediaPath = String( "../../../../../Media" );
                auto scriptsPath = String( "../../../../../Media/Scripts/" );
#elif defined WP_PLATFORM_APPLE
                auto mediaPath = String( "../../Media" );
                auto scriptsPath = String( "../../Media/Scripts/" );
#else
                auto mediaPath = String( "../../Media" );
                auto scriptsPath = String( "../../Media/Scripts/" );
#endif

                //auto absoluteMediaPath = Path::lexically_normal( workingDirectory, mediaPath );
                applicationManager->setMediaPath( mediaPath );

                auto absoluteScriptsPath = Path::lexically_normal( workingDirectory, scriptsPath );
                fileSystem->addFolder( scriptsPath, true );
                fileSystem->addFolder( mediaPath, false );
            }
            else
            {
                auto mediaPath = getMediaPath();
                auto scriptsPath = mediaPath + scriptsFolderStr;

                applicationManager->setMediaPath( mediaPath );
                fileSystem->addFolder( scriptsPath, true );
                fileSystem->addFolder( mediaPath, false );

                //fileSystem->addFileArchive( "Media.zip", false, false, IFileSystem::ArchiveType::Zip );
            }

            fileSystem->addFileArchive( ".FBCache", true, true, IFileSystem::ArchiveType::Folder );
            fileSystem->addFileArchive( "./", true, true, IFileSystem::ArchiveType::Folder );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void EditorApplication::createUI()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto factoryManager = applicationManager->getFactoryManagerPtr();

        auto uiManager = factoryManager->make_object<ui::IUIManager>( "ImGui" );
        //auto uiManager = factoryManager->make_object<ui::IUIManager>( "ClawUIManager" );
        if( uiManager )
        {
            uiManager->load( nullptr );
            applicationManager->setUI( uiManager );

            auto application = uiManager->addApplication();
            uiManager->setApplication( application );
            m_application = application;
        }
    }

    void EditorApplication::setRendererType( render::IGraphicsSystem::RenderApi type )
    {
        m_rendererType = type;
        m_rendererTypeExplicit = true;
    }

    SmartPtr<Properties> EditorApplication::getProperties() const
    {
        auto manager = core::IApplicationManager::instancePtr();
        auto graphics = manager ? manager->getGraphicsSystem() : nullptr;
        auto pipeline = graphics ? graphics->getGraphicsPipeline() : nullptr;
        auto properties = pipeline ? pipeline->getProperties() : workphone::make_ptr<Properties>();
        properties->setProperty( "Applies to", "Editor only", true );
        properties->setProperty( "API change", "Restart the editor to apply", true );
        properties->setProperty( "Pipeline support", "Software main window only", true );
        const s32 api = m_rendererType == render::IGraphicsSystem::RenderApi::Software ? 0 : 1;
        properties->setPropertyAsEnum( "Render API", api, { "Software", "DirectX 11" } );
        return properties;
    }

    void EditorApplication::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
            return;
        auto manager = core::IApplicationManager::instancePtr();
        if( !manager )
            return;
        auto settings = manager->getEditorSettings();
        if( !settings )
        {
            settings = workphone::make_ptr<Properties>();
            manager->setEditorSettings( settings );
        }
        s32 api = 1;
        if( properties->getPropertyValue( "Render API", api ) && api >= 0 && api <= 1 )
        {
            using Api = render::IGraphicsSystem::RenderApi;
            m_rendererType = api == 0 ? Api::Software : Api::DX11;
            settings->setProperty( "Editor Render API", api );
        }
        auto graphics = manager->getGraphicsSystem();
        if( auto pipeline = graphics ? graphics->getGraphicsPipeline() : nullptr )
        {
            pipeline->setProperties( properties );
            auto saved = pipeline->getProperties();
            saved->setName( "Editor Pipeline" );
            settings->removeChild( "Editor Pipeline" );
            settings->addChild( saved );
        }
    }

    bool EditorApplication::createGraphicsSystem()
    {
        loadEditorPreferences();
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto graphicsSystem = factoryManager->make_object<render::IGraphicsSystem>();
        if( !graphicsSystem )
        {
            return false;
        }

        applicationManager->setGraphicsSystem( graphicsSystem );

        graphicsSystem->load( nullptr );

        auto configurationData = graphicsSystem->createConfiguration();
        auto configuration =
            workphone::dynamic_pointer_cast<render::GraphicsSettings>( configurationData );
        WP_ASSERT( configuration );

        configuration->setCreateWindow( false );
        // configuration->setCreateWindow( true );

        graphicsSystem->configure( configuration );
        graphicsSystem->setRendererType( m_rendererType );

        if( auto settings = applicationManager->getEditorSettings() )
        {
            if( auto saved = settings->getChild( "Editor Pipeline" ) )
            {
                if( auto pipeline = graphicsSystem->getGraphicsPipeline() )
                {
                    // Restore the preset first, then the saved per-effect overrides.
                    pipeline->setProperties( saved );
                    pipeline->setProperties( saved );
                }
            }
        }

        auto properties = factoryManager->make_ptr<Properties>();

        // Set renderer type based on user selection
        switch( m_rendererType )
        {
        case render::IGraphicsSystem::RenderApi::Software:
            properties->setProperty( "RendererType", "software" );
            break;
        case render::IGraphicsSystem::RenderApi::DX11:
            properties->setProperty( "RendererType", "dx11" );
            break;
        case render::IGraphicsSystem::RenderApi::DX12:
            properties->setProperty( "RendererType", "dx12" );
            break;
        default:
            properties->setProperty( "RendererType", "dx11" );
            break;
        }

        if( m_windowHandle != 0 )
        {
            properties->setProperty( "WindowHandle", m_windowHandle );
        }

        auto window = graphicsSystem->createRenderWindow( "Editor", 400, 400, false, properties );
        applicationManager->setWindow( window );

        auto resourceGroupManager = graphicsSystem->getResourceGroupManager();
        WP_ASSERT( resourceGroupManager );
        resourceGroupManager->load( nullptr );

        auto resourceDatabase = factoryManager->make_ptr<ResourceDatabase>();
        applicationManager->setResourceDatabase( resourceDatabase );
        resourceDatabase->load( nullptr );

        return true;
    }

    void EditorApplication::createRenderWindow()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto ui = applicationManager->getUIPtr();
        if( ui )
        {
            m_renderWindow = ui->addElementByType<ui::IUIRenderWindow>();

            if( m_renderWindow )
            {
                if( m_renderTarget )
                {
                    m_renderWindow->setRenderTexture( m_renderTarget );
                }
            }
        }
    }

    void EditorApplication::createCamera()
    {
        Application::createCamera();

        if( auto camera = m_camera )
        {
            camera->setRenderUI( true );
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        applicationManager->setEditor( true );
        applicationManager->setEditorCamera( true );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto sceneManager = applicationManager->getGameManagerPtr();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScenePtr();
        WP_ASSERT( scene );

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        if( graphicsSystem )
        {
            auto textureManager = graphicsSystem->getTextureManager();
            WP_ASSERT( textureManager );

            auto cameraMgr = factoryManager->make_ptr<scene::CameraManager>();
            cameraMgr->load( nullptr );
            applicationManager->setCameraManager( cameraMgr );

            m_cameraActor = sceneManager->createActor();
            m_cameraActor->setName( "EditorCamera" );
            m_cameraActor->setFlag( scene::IGameActor::ActorFlagIsEditor, true );
            m_cameraActor->setPerpetual( true );
            cameraMgr->setEditorCamera( m_cameraActor );

            auto cameraComponent = m_cameraActor->addComponent<scene::Camera>();
            cameraComponent->setEnableShadows( true );
            cameraComponent->setEnableSceneRender( true );
            cameraComponent->setEnableUI( true );
            cameraComponent->setActive( true );

            m_renderTarget = textureManager->createRenderTexture();
            WP_ASSERT( m_renderTarget );

            cameraMgr->setEditorRTT( m_renderTarget );

            WP_ASSERT( cameraComponent );
            cameraComponent->setTargetTexture( m_renderTarget );

            //auto gameScene = sceneManager->getCurrentScene();
            //gameScene->addActor( m_cameraActor );
        }
    }

    void EditorApplication::createViewports()
    {
        Application::createViewports();

        if( auto vp = m_viewport )
        {
            vp->setEnableUI( true );
            vp->setEnableSceneRender( false );
        }
    }

    void EditorApplication::createRenderInitJob()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto taskManager = applicationManager->getTaskManager();
        auto jobQueue = applicationManager->getJobQueue();

        auto jobRendererSetup = workphone::make_ptr<JobRendererSetup>();

        auto renderTask = taskManager->getTask( TaskId::Render );
        if( renderTask )
        {
            renderTask->addJob( jobRendererSetup );
        }
        else
        {
            jobQueue->addJob( jobRendererSetup );
        }
    }

    void EditorApplication::loadGraphicsResources()
    {
        //auto applicationManager = core::IApplicationManager::instance();
        //WP_ASSERT( applicationManager );

        //auto graphicsSystem = applicationManager->getGraphicsSystem();
        //WP_ASSERT( graphicsSystem );

        //auto resourceGroupManager = graphicsSystem->getResourceGroupManager();
        //WP_ASSERT( resourceGroupManager );
        //// resourceGroupManager->load( nullptr );
        //graphicsSystem->loadObject( resourceGroupManager, true );
    }

    void EditorApplication::createPhysics()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();

            auto properties = factoryManager->make_ptr<Properties>();

            auto physicsManager = factoryManager->make_object<physics::IPhysicsManager>();
            if( physicsManager )
            {
                physicsManager->load( nullptr );
                applicationManager->setPhysicsManager( physicsManager );

                auto physicsScene = physicsManager->addScene();
                physicsManager->setPhysicsScene( physicsScene );

                auto vehicleManager = factoryManager->make_object<vehicle::IVehicleManager>();
                applicationManager->setVehicleManager( vehicleManager );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void EditorApplication::createInputSystem()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            if( graphicsSystem )
            {
                if( auto window = graphicsSystem->getDefaultWindow() )
                {
                    auto inputManager = factoryManager->make_object<IInputDeviceManager>();
                    if( inputManager )
                    {
                        applicationManager->setInputDeviceManager( inputManager );

                        inputManager->setWindow( window );

#if defined WP_PLATFORM_WIN32
                        inputManager->setCreateMouse( true );
                        inputManager->setCreateKeyboard( true );
#elif defined WP_PLATFORM_APPLE
                        inputManager->setCreateMouse( false );
                        inputManager->setCreateKeyboard( false );
#endif

                        inputManager->load( nullptr );

                        auto hash = StringUtil::getHash( "gameInput0" );
                        auto gameInput0 = inputManager->addGameInput( hash );
                        gameInput0->setPlayerIndex( 0 );

                        auto gameInputMap0 = gameInput0->getGameInputMap();

                        ////Keyboard
                        // gameInputMap0->setKeyboardAction(Types::INPUT_UP, "UP", "");
                        // gameInputMap0->setKeyboardAction(Types::INPUT_DOWN, "DOWN", "");
                        // gameInputMap0->setKeyboardAction(Types::INPUT_RIGHT, "RIGHT", "");
                        // gameInputMap0->setKeyboardAction(Types::INPUT_LEFT, "LEFT", "");

                        if( !m_editorApplicationListener )
                        {
                            m_editorApplicationListener =
                                workphone::make_ptr<ApplicationListener>( this );
                        }
                        applicationManager->addObjectListener( m_editorApplicationListener );

                        if( auto inputManager = applicationManager->getInputDeviceManager() )
                        {
                            inputManager->addListener( m_editorApplicationListener );
                        }
                    }
                }
                else
                {
                    WP_LOG( "Could not create input system. No window found." );
                }
            }
            else
            {
                WP_LOG( "Could not create input system. No window found." );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void EditorApplication::createGraphicsScene()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( graphicsSystem )
            {
                auto smgr = graphicsSystem->addGraphicsScene( "DefaultSceneManager", "ViewSM" );

                auto color = ColourF::White * 0.5f;
                smgr->setAmbientLight( color );

                m_sceneMgr = smgr;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void EditorApplication::createTasks()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto taskManager = applicationManager->getTaskManagerPtr();
            WP_ASSERT( taskManager );

            auto profiler = applicationManager->getProfilerPtr();
            WP_ASSERT( profiler );

#if 1
            if( auto primaryTask = taskManager->getTaskPtr( TaskId::Primary ) )
            {
                primaryTask->setTask( TaskId::Primary );
                primaryTask->setThreadTaskFlags( Thread::Primary_Flag );
                primaryTask->setPrimary( true );
                primaryTask->setEnabled( true );
                primaryTask->setOwner( this );
                primaryTask->setTargetFPS( 60.0 );

                auto profile = profiler->addProfile();
                profile->setLabel( "Primary" );
                primaryTask->setProfile( profile );
            }

            if( auto applicationTask = taskManager->getTaskPtr( TaskId::Application ) )
            {
                applicationTask->setTask( TaskId::Application );
                applicationTask->setThreadTaskFlags( Thread::Application_Flag );

                applicationTask->setPrimary( false );
                applicationTask->setEnabled( true );
                applicationTask->setOwner( this );
                applicationTask->setTargetFPS( 60.0 );
                applicationTask->setAffinity( 6 );

                auto profile = profiler->addProfile();
                profile->setLabel( "Application" );
                applicationTask->setProfile( profile );
            }

            if( auto renderTask = taskManager->getTaskPtr( TaskId::Render ) )
            {
                renderTask->setTask( TaskId::Render );
                renderTask->setThreadTaskFlags( Thread::Render_Flag );

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
#        ifdef WP_PLATFORM_WIN32
                renderTask->setPrimary( false );
#        else
                renderTask->setPrimary( true );
#        endif
#    elif WP_GRAPHICS_SYSTEM_OGRE
                renderTask->setPrimary( true );
#    else
                renderTask->setPrimary( false );
#    endif

                renderTask->setEnabled( true );
                renderTask->setOwner( this );
                renderTask->setTargetFPS( 60.0 );
                renderTask->setAffinity( 1 );

                auto profile = profiler->addProfile();
                profile->setLabel( "Render" );
                renderTask->setProfile( profile );
            }

            if( auto physicsTask = taskManager->getTaskPtr( TaskId::Physics ) )
            {
                physicsTask->setTask( TaskId::Physics );
                physicsTask->setThreadTaskFlags( Thread::Physics_Flag );

                physicsTask->setPrimary( false );
                physicsTask->setEnabled( true );
                physicsTask->setOwner( this );
                physicsTask->setTargetFPS( 120.0 );
                physicsTask->setAffinity( 3 );

                auto profile = profiler->addProfile();
                profile->setLabel( "Physics" );
                physicsTask->setProfile( profile );
            }

            if( auto inputTask = taskManager->getTaskPtr( TaskId::Input ) )
            {
                inputTask->setTask( TaskId::Input );
                inputTask->setThreadTaskFlags( Thread::Input_Flag );

                inputTask->setPrimary( false );
                inputTask->setEnabled( true );
                inputTask->setOwner( this );
                inputTask->setTargetFPS( 120.0 );
                inputTask->setAffinity( 4 );

                auto profile = profiler->addProfile();
                profile->setLabel( "Input" );
                inputTask->setProfile( profile );
            }

            if( auto garbageCollectTask = taskManager->getTaskPtr( TaskId::GarbageCollect ) )
            {
                garbageCollectTask->setTask( TaskId::GarbageCollect );
                garbageCollectTask->setThreadTaskFlags( Thread::GarbageCollect_Flag );

                garbageCollectTask->setPrimary( false );
                garbageCollectTask->setEnabled( true );
                garbageCollectTask->setOwner( this );
                garbageCollectTask->setTargetFPS( 10.0 );
                garbageCollectTask->setAffinity( 5 );

                auto profile = profiler->addProfile();
                profile->setLabel( "Garbage Collect" );
                garbageCollectTask->setProfile( profile );
            }
#else
            if( auto primaryTask = taskManager->getTaskPtr( TaskId::Primary ) )
            {
                primaryTask->setTask( TaskId::Primary );
                primaryTask->setThreadTaskFlags( Thread::Primary_Flag );
                primaryTask->setPrimary( true );
                primaryTask->setEnabled( true );
                primaryTask->setOwner( this );
                primaryTask->setTargetFPS( 60.0 );

                auto profile = profiler->addProfile();
                profile->setLabel( "Primary" );
                primaryTask->setProfile( profile );
            }

            if( auto applicationTask = taskManager->getTaskPtr( TaskId::Application ) )
            {
                applicationTask->setTask( TaskId::Application );
                applicationTask->setThreadTaskFlags( Thread::Application_Flag );

                applicationTask->setPrimary( false );
                applicationTask->setEnabled( true );
                applicationTask->setOwner( this );
                applicationTask->setTargetFPS( 60.0 );
                applicationTask->setAffinity( 1 );

                auto profile = profiler->addProfile();
                profile->setLabel( "Application" );
                applicationTask->setProfile( profile );
            }

            if( auto renderTask = taskManager->getTaskPtr( TaskId::Render ) )
            {
                renderTask->setTask( TaskId::Render );
                renderTask->setThreadTaskFlags( Thread::Render_Flag );

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
#        ifdef WP_PLATFORM_WIN32
                renderTask->setPrimary( false );
                //renderTask->setPrimary( true );
#        else
                renderTask->setPrimary( true );
#        endif
#    elif WP_GRAPHICS_SYSTEM_OGRE
                renderTask->setPrimary( true );
#    endif

                renderTask->setEnabled( true );
                renderTask->setOwner( this );
                renderTask->setTargetFPS( 60.0 );
                renderTask->setAffinity( 1 );

                auto profile = profiler->addProfile();
                profile->setLabel( "Render" );
                renderTask->setProfile( profile );
            }

            if( auto physicsTask = taskManager->getTaskPtr( TaskId::Physics ) )
            {
                physicsTask->setTask( TaskId::Physics );
                physicsTask->setThreadTaskFlags( Thread::Physics_Flag );

                physicsTask->setPrimary( false );
                physicsTask->setEnabled( true );
                physicsTask->setOwner( this );
                physicsTask->setTargetFPS( 120.0 );
                physicsTask->setAffinity( 1 );

                auto profile = profiler->addProfile();
                profile->setLabel( "Physics" );
                physicsTask->setProfile( profile );
            }

            if( auto inputTask = taskManager->getTaskPtr( TaskId::Input ) )
            {
                inputTask->setTask( TaskId::Input );
                inputTask->setThreadTaskFlags( Thread::Input_Flag );

                inputTask->setPrimary( false );
                inputTask->setEnabled( true );
                inputTask->setOwner( this );
                inputTask->setTargetFPS( 30.0 );
                inputTask->setAffinity( 2 );

                auto profile = profiler->addProfile();
                profile->setLabel( "Input" );
                inputTask->setProfile( profile );
            }

            if( auto garbageCollectTask = taskManager->getTaskPtr( TaskId::GarbageCollect ) )
            {
                garbageCollectTask->setTask( TaskId::GarbageCollect );
                garbageCollectTask->setThreadTaskFlags( Thread::GarbageCollect_Flag );

                garbageCollectTask->setPrimary( false );
                garbageCollectTask->setEnabled( true );
                garbageCollectTask->setOwner( this );
                garbageCollectTask->setTargetFPS( 10.0 );
                garbageCollectTask->setAffinity( 3 );

                auto profile = profiler->addProfile();
                profile->setLabel( "Garbage Collect" );
                garbageCollectTask->setProfile( profile );
            }
#endif
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    /*
    SmartPtr<render::IGraphicsCamera> EditorApplication::getUiCamera() const
    {
        return m_uiCamera;
    }

    void EditorApplication::setUiCamera( SmartPtr<render::IGraphicsCamera> camera )
    {
        m_uiCamera = camera;
    }

    SmartPtr<render::IGraphicsSceneNode> EditorApplication::getUiCameraSceneNode() const
    {
        return m_uiCameraSceneNode;
    }

    void EditorApplication::setUiCameraSceneNode( SmartPtr<render::IGraphicsSceneNode> sceneNode )
    {
        m_uiCameraSceneNode = sceneNode;
    }
    */

    SmartPtr<render::IGraphicsCamera> EditorApplication::getCamera() const
    {
        return m_camera;
    }

    void EditorApplication::setCamera( SmartPtr<render::IGraphicsCamera> camera )
    {
        m_camera = camera;
    }

    SmartPtr<render::IGraphicsSceneNode> EditorApplication::getCameraSceneNode() const
    {
        return m_cameraSceneNode;
    }

    void EditorApplication::setCameraSceneNode( SmartPtr<render::IGraphicsSceneNode> sceneNode )
    {
        m_cameraSceneNode = sceneNode;
    }

    SmartPtr<IStateContext> EditorApplication::getStateContext() const
    {
        return m_stateContext;
    }

    void EditorApplication::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        m_stateContext = stateContext;
    }

    SmartPtr<IStateListener> EditorApplication::getStateListener() const
    {
        return m_stateListener;
    }

    void EditorApplication::setStateListener( SmartPtr<IStateListener> stateListener )
    {
        m_stateListener = stateListener;
    }

    void EditorApplication::loadEditorPreferences()
    {
        auto applicationManager = core::IApplicationManager::instance();

        static String settingsFileName = "LioncatEditor.settings";

#if defined WP_PLATFORM_APPLE
        auto settingsPath = macGetConfigFilePath();
        auto settingsFilePath = settingsPath + "/" + settingsFileName;
#else
        auto settingsFilePath = settingsFileName;
#endif

        auto editorSettingsDataStr = Path::readAllText( settingsFilePath );
        if( !StringUtil::isNullOrEmpty( editorSettingsDataStr ) )
        {
            auto editorSettings = workphone::make_ptr<Properties>();
            DataUtil::parse( editorSettingsDataStr, editorSettings.get() );
            applicationManager->setEditorSettings( editorSettings );
            s32 api = 1;
            if( !m_rendererTypeExplicit &&
                editorSettings->getPropertyValue( "Editor Render API", api ) && api >= 0 && api <= 1 )
            {
                using Api = render::IGraphicsSystem::RenderApi;
                m_rendererType = api == 0 ? Api::Software : Api::DX11;
            }
        }
    }

    void EditorApplication::loadSettings()
    {
        auto applicationManager = core::IApplicationManager::instance();

        auto playerSettings = workphone::make_ptr<Properties>();
        applicationManager->setPlayerSettings( playerSettings );

        loadEditorPreferences();

        auto fileSystem = applicationManager->getFileSystem();
        auto gameManager = applicationManager->getGameManager();

        static const auto ignorelistFilePath = String( "ignorelist.settings" );
        auto ignoreListStr = Path::readAllText( ignorelistFilePath );
        auto ignoreList = Array<String>();
        StringUtil::parseArray( ignoreListStr, ignoreList );
        gameManager->setComponentFactoryIgnoreList( ignoreList );

        static const auto componentmapFilePath = String( "componentmap.settings" );
        auto componentmapDataStr = Path::readAllText( componentmapFilePath );
        if( !StringUtil::isNullOrEmpty( componentmapDataStr ) )
        {
            auto componentmapSettings = workphone::make_ptr<Properties>();
            DataUtil::parse( componentmapDataStr, componentmapSettings.get() );

            Map<String, String> componentMap;

            auto properties = componentmapSettings->getPropertiesAsArray();
            for( auto &property : properties )
            {
                auto key = property.getName();
                auto value = property.getValue();

                componentMap[key] = value;
            }

            gameManager->setComponentFactoryMap( componentMap );
        }
    }

    /*
    SmartPtr<render::IViewport> EditorApplication::getRttViewport() const
    {
        return m_rttViewport;
    }

    void EditorApplication::setRttViewport( SmartPtr<render::IViewport> rttViewport )
    {
        m_rttViewport = rttViewport;
    }
    */

    EditorApplication::ApplicationListener::ApplicationListener( EditorApplication *app ) :
        m_application( app )
    {
    }

    EditorApplication::ApplicationListener::ApplicationListener() = default;

    EditorApplication::ApplicationListener::~ApplicationListener() = default;

    //--------------------------------------------
    // void FileListener::handleFileAction( FW::WatchID watchid, const FW::String& dirStr, const
    // FW::String& filenameStr, FW::Action action )
    //{
    //	auto applicationManager = core::IApplicationManager::instance();

    //	String fileName = filenameStr.c_str();
    //	String ext = FileSystem::getFileExtension(fileName);
    //	if(ext==(".fdb"))
    //	{
    //		/*Sleep(200);
    //		for(u32 i=0; i<DATABASE_COUNT; ++i)
    //		{
    //			DatabaseMgrPtr database;// =
    //IApplicationManager::instance()->getGameSceneManager()->getDatabase(i);
    //			database->handleResourceModified(fileName);
    //		}*/
    //	}
    //	else if(ext==(".lua"))
    //	{
    //		Thread::sleep(200);
    //		reloadLuaScripts();

    //	}
    //	else if(ext==(".gui"))
    //	{
    //		//RecursiveMutex::ScopedLock lock(OgreMutex);
    //		//CGUIManager::getSingletonPtr()->reloadCurrentLayout();
    //	}
    //	else if(ext==(".cg") || ext==(".hlsl") || ext==(".glsl"))
    //	{
    //		//RecursiveMutex::ScopedLock lock(OgreMutex);
    //		////IApplicationManager::instance()->getGraphicsSystem()->reloadResources("material");

    //		//reloadAResourceGroupWithoutDestroyingIt("gpuProgram");

    //		//parseScripts(Ogre::MaterialManager::getSingletonPtr(), ".program", "gpuProgram");

    //		//ResourceGroupHelper resGrpHelper;
    //		//resGrpHelper.updateOnEveryRenderable();
    //	}
    //	else if(ext==(".material"))
    //	{
    //		//RecursiveMutex::ScopedLock lock(OgreMutex);
    //		////IApplicationManager::instance()->getGraphicsSystem()->reloadResources("material");

    //		//reloadAResourceGroupWithoutDestroyingIt("material");

    //		//parseScripts(Ogre::MaterialManager::getSingletonPtr(), ".material", "material");

    //		//ResourceGroupHelper resGrpHelper;
    //		//resGrpHelper.updateOnEveryRenderable();
    //	}
    //	else if(ext==(".pu"))
    //	{
    //		reloadParticles();
    //	}
    //	else if(ext==(".dds") ||
    //			ext==(".tga") ||
    //			ext==(".png") ||
    //			ext==(".jpg") ||
    //			ext==(".bmp") )
    //	{
    //		Thread::sleep(200);
    //		reloadTexture(fileName);
    //	}

    //	SmartPtr<IEntityManager>& entityManager = engine->getEntityManager();
    //	Array<SmartPtr<IGameActor>> entities;
    //	entityManager->getEntities(entities);
    //	for(u32 i=0; i<entities.size(); ++i)
    //	{
    //		SmartPtr<IGameActor>& entity = entities[i];
    //		//entity->handleResourceModified(fileName);
    //	}
    //}

    Parameter EditorApplication::ApplicationListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        auto applicationManager = core::IApplicationManager::instance();

        if( eventValue == IEvent::addActor )
        {
        }
        else if( eventValue == IEvent::inputEvent )
        {
            inputEvent( event );
        }

        return {};
    }

    bool EditorApplication::ApplicationListener::inputEvent( SmartPtr<IInputEvent> event )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        if( m_application->inputEvent( event ) )
        {
            return true;
        }

        switch( event->getEventType() )
        {
        case IInputEvent::EventType::Mouse:
        {
            if( auto mouseState = event->getMouseState() )
            {
                auto window = static_cast<render::IGraphicsWindow *>( event->getWindow() );
                if( !window )
                {
                    return false;
                }

                auto windowSize = window->getSize();
                Vector2F mousePosition = mouseState->getAbsolutePosition();
                //Vector2F mouseCoords = mousePosition / windowSize;

                if( mouseState->getEventType() == IMouseState::Event::LeftPressed )
                {
                    //auto smgr = graphicsSystem->getGraphicsScene( "ViewSM" );
                    //WP_ASSERT( smgr );

                    // RoadManagerPtr roadManager = applicationManager->getRoadManager();

                    // if ( roadManager )
                    //{
                    //	if ( roadManager->getCreateRoadNode() )
                    //	{
                    //		//SmartPtr<ISceneView> meshSceneView =
                    // applicationManager->getMeshSceneView();
                    //		//SmartPtr<render::IGraphicsSceneManager> smgr =
                    // meshSceneView->getSceneManager();

                    //		// create road node
                    //		SmartPtr<IGraphicsSceneNode> boxNode;
                    //		GraphicsMeshPtr boxMesh;

                    //		boxNode = smgr->getRootSceneNode()->addChildSceneNode();
                    //		boxMesh = smgr->addMesh("4x4chassis.mesh");
                    //		boxMesh->setMaterialName("Box/SphereMappedRustySteel");
                    //		boxNode->attachObject(boxMesh);
                    //		boxNode->setPosition(Vector3F(0, 0, 0));
                    //		boxNode->setScale(Vector3F::UNIT * 2.0f);

                    //		m_app->boxNode = boxNode;
                    //		m_app->boxMesh = boxMesh;

                    //		SphericalCameraPtr sphericalCamera = m_app->m_sphericalCamera;
                    //		Array<SmartPtr<IGraphicsCamera>> cameras = sphericalCamera->getCameras();
                    //		for ( u32 i = 0; i < cameras.size(); ++i )
                    //		{
                    //			SmartPtr<IGraphicsCamera> camera = cameras[i];

                    //			Ray3F ray = camera->getRay(mouseCoords.X(), mouseCoords.Y());

                    //			Vector3F intersectionPoint = Vector3F::ZERO;

                    //			Plane3F groundPlane(Vector3F::ZERO, Vector3F::UNIT_Y);
                    //			if ( groundPlane.getIntersectionWithLine(ray.getOrigin(),
                    // ray.getDirection(), intersectionPoint) )
                    //			{
                    //				boxNode->setPosition(intersectionPoint);
                    //			}
                    //		}
                    //	}
                    //}
                }

                // auto viewWindow = static_cast<ui::wxViewWindow*>(event->getUserData());
                // auto camera = viewWindow->getCamera();
                // if (!camera)
                //{
                //	return false;
                // }

                // Ray3F ray = camera->getRay(mouseCoords.X(), mouseCoords.Y());

                // if (mouseState->getEventType() == IMouseState::MOUSE_EVENT_LEFT_BUTTON_PRESSED)
                //{
                //	//SmartPtr<ISceneView> meshSceneView = applicationManager->getMeshSceneView();

                //	//Vector3F meshHitPosition;

                //	//SmartPtr<render::IGraphicsSceneManager> sceneManager =
                // meshSceneView->getSceneManager();
                //	//if (sceneManager->castRay(ray, meshHitPosition))
                //	//{
                //	//	m_application->m_sphericalCamera->setTargetPosition(meshHitPosition);
                //	//}
                //}

                //						SmartPtr<IGraphicsTerrain> terrain =
                // applicationManager->getTerrain(); 						if ( terrain &&
                // terrain->isLoaded()
                // )
                //						{
                //							TerrainRayResultPtr result = terrain->intersects(ray);
                //							if ( result && result->hasIntersected() )
                //							{
                //								Vector3F hitPosition = result->getPosition();
                //
                //								SmartPtr<IDecalCursor> decalCursor =
                // applicationManager->getDecalCursor(); 								if (
                // decalCursor
                // )
                //								{
                //									decalCursor->setPosition(hitPosition);
                //								}
                //
                //								if(mouseState->getEventType() ==
                // IMouseState::MOUSE_EVENT_LEFT_BUTTON_PRESSED)
                //								{
                //									if(applicationManager->getEditFoliage())
                //									{
                //#if 0
                //										auto applicationManager =
                // ApplicationManager::instance();
                // SmartPtr<IGraphicsSystem> graphicsSystem = engine->getGraphicsSystem();
                // SmartPtr<render::IGraphicsSceneManager> smgr = graphicsSystem->getSceneManager("ViewSM");
                //
                //										GraphicsMeshPtr treeMesh =
                // smgr->addMesh("Palm.mesh"); SmartPtr<IGraphicsSceneNode> sceneNode =
                // smgr->getRootSceneNode()->addChildSceneNode(); sceneNode->attachObject(treeMesh);
                // sceneNode->setPosition(hitPosition);
                //
                //										QuaternionF orientation;
                //										orientation.fromAngleAxis(MathF::DegToRad(-90.0f),
                // Vector3F::UNIT_X);
                // sceneNode->setOrientation(orientation); #else
                //
                //										ApplicationManager* editorManager =
                // ApplicationManager::instance(); FoliageManagerPtr foliageMgr =
                // editorManager->getFoliageManager();
                //
                //										Vector3F terrrainSpacePosition =
                // terrain->getTerrainSpacePosition(result->getPosition());
                //										foliageMgr->add(terrrainSpacePosition);
                //
                //#endif
                //								}
                //									else
                //									{
                //										Vector3F terrrainSpacePosition =
                // terrain->getTerrainSpacePosition(result->getPosition());
                // TerrainBlendMapPtr
                // blendMap = terrain->getBlendMap(1); 										if (
                // blendMap
                // )
                //										{
                //											f32 blendMapSize =
                // terrain->getLayerBlendMapSize(); 											u32
                // brushSize = 64; Vector3F blendMapCoords(terrrainSpacePosition.X() * blendMapSize,
                // terrrainSpacePosition.Y()
                //* blendMapSize, 0.0f);
                //
                //											Vector2I minCoord(blendMapCoords.X() -
                // brushSize, blendMapCoords.Y() - brushSize);
                // Vector2I maxCoord(blendMapCoords.X() + brushSize, blendMapCoords.Y() + brushSize);
                //
                //											for ( u32 y = minCoord.Y(); y <
                // maxCoord.Y();
                //++y
                //)
                //											{
                //												for ( u32 x = minCoord.X(); x <
                // maxCoord.X();
                //++x
                //)
                //												{
                //													blendMap->setBlendValue(x, y,
                // MathF::RangedRandom(0.0, 1.0));
                //												}
                //											}
                //
                //											blendMap->updateModifications();
                //										}
                //									}
                //							}
                //								else if ( mouseState->getEventType() ==
                // IMouseState::MOUSE_EVENT_RIGHT_BUTTON_PRESSED )
                //								{
                //
                //								}
                //						}
                //					}
            }
        }
        break;
        default:
        {
        }
        }

        return false;
    }

    bool EditorApplication::ApplicationListener::updateEvent( const SmartPtr<IInputEvent> &event )
    {
        return false;
    }

    void EditorApplication::ApplicationListener::setPriority( s32 priority )
    {
    }

    s32 EditorApplication::ApplicationListener::getPriority() const
    {
        return 0;
    }

    EditorApplication::ApplicationStateListener::~ApplicationStateListener()
    {
        setOwner( nullptr );
    }

    bool EditorApplication::ApplicationStateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        return false;
    }

    bool EditorApplication::ApplicationStateListener::handleStateMessage(
        const SmartPtr<IStateMessage> &message )
    {
        // if (message->isExactly<StateMessage>())
        //{
        //	static const auto RENDERER_READY_HASH = StringUtil::getHash("RendererReady");

        //	auto type = message->getType();
        //	if (type == RENDERER_READY_HASH)
        //	{
        //		auto editorManager = EditorManager::getSingletonPtr();
        //		auto uiManager = editorManager->getUI();
        //		auto renderWindow = uiManager->getRenderWindow();
        //		renderWindow->setRenderer();

        //		auto applicationManager = core::IApplicationManager::instance();
        //		auto graphicsSystem = applicationManager->getGraphicsSystem();
        //		graphicsSystem->loadResources();
        //	}
        //}

        return false;
    }

    EditorApplication *EditorApplication::ApplicationStateListener::getOwner() const
    {
        return m_owner;
    }

    void EditorApplication::ApplicationStateListener::setOwner( EditorApplication *owner )
    {
        m_owner = owner;
    }

    EditorApplication::ApplicationStateListener::ApplicationStateListener() = default;

    FSMReturnType EditorApplication::ApplicationFSMListener::handleEvent( u32 state, FSMEvent eventType )
    {
        return m_owner->handleEvent( state, eventType );
    }

    EditorApplication *EditorApplication::ApplicationFSMListener::getOwner() const
    {
        return m_owner;
    }

    void EditorApplication::ApplicationFSMListener::setOwner( EditorApplication *owner )
    {
        m_owner = owner;
    }

    EditorApplication::ApplicationFSMListener::ApplicationFSMListener() = default;

    EditorApplication::ApplicationFSMListener::~ApplicationFSMListener() = default;

    Parameter EditorApplication::EventListener::handleEvent( EventType eventType, hash_type eventValue,
                                                             const Array<Parameter> &arguments,
                                                             SmartPtr<ISharedObject> sender,
                                                             SmartPtr<ISharedObject> object,
                                                             SmartPtr<IEvent> event )
    {
        return {};
    }

    EditorApplication::EventListener::EventListener() = default;

    EditorApplication::EventListener::~EventListener() = default;
}  // namespace workphone::editor
