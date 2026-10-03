#include <EditorPCH.hpp>
#include <Workphone/Workphone.hpp>
#include <EditorApplication.hpp>
#include <editor/EditorManager.hpp>
#include <commands/ToggleEditorCamera.hpp>
#include <Workphone/System/Job.hpp>

#if WP_EDITOR_TESTS
#    include <boost/test/unit_test.hpp>
#    include <chrono>
#    include <atomic>
#    include <filesystem>
#    include <fstream>
#    include <memory>
#    include <thread>

using namespace workphone;
using namespace workphone::editor;

namespace
{
    // Keep real editor jobs, scenes, cameras and renderer, but isolate plugin
    // discovery from the user's potentially stale editor configuration.
    class PlayModeTestApplication : public EditorApplication
    {
    public:
        String pluginConfiguration;
    private:
        void createPlugins() override
        {
            setPluginsConfigFilePath( pluginConfiguration );
            core::Application::createPlugins();
        }
    };

    class PlayModeBarrierJob : public Job
    {
    public:
        std::shared_ptr<std::atomic_bool> completed;
        void execute() override { completed->store( true ); }
    };

    struct PlayModeFixture
    {
        std::filesystem::path directory = std::filesystem::temp_directory_path() /
            std::string( ( "lioncat-playmode-" + StringUtil::getUUID() ).c_str() );
        PlayModeTestApplication app;
        SmartPtr<core::IApplicationManager> applicationManager;
        SmartPtr<scene::IGameManager> gameManager;
        SmartPtr<scene::IGameScene> scene;
        SmartPtr<scene::ICameraManager> cameraManager;
        SmartPtr<scene::IGameActor> editorActor;
        SmartPtr<scene::Camera> editorCamera;

        explicit PlayModeFixture( u32 workers = 0 )
        {
            std::filesystem::create_directories( directory );
            const auto configPath = directory / "plugins.cfg";
            std::ofstream config( configPath );
            config << "PluginFolder=.\n";
#    if WP_GRAPHICS_SYSTEM_OGRENEXT
            config << "Plugin=WPGraphicsOgreNext\n";
#    elif WP_GRAPHICS_SYSTEM_OGRE
            config << "Plugin=WPGraphicsOgre\n";
#    else
            config << "Plugin=WPGraphics\n";
#    endif
            config << "Plugin=WPSQLite\nPlugin=WPPhysics\nPlugin=WPOISInput\nPlugin=WPAssimp\n";
            config.close();
            app.pluginConfiguration = configPath.generic_string().c_str();
            app.setDebugMode( true );
            app.setActiveThreads( workers );
            app.load( nullptr );
            BOOST_REQUIRE_MESSAGE( app.isLoaded(), "Editor failed to load" );
            applicationManager = core::IApplicationManager::instance();
            BOOST_REQUIRE( applicationManager );
            BOOST_REQUIRE( applicationManager->getJobQueue() );
            BOOST_REQUIRE( applicationManager->getTaskManager() );
            BOOST_REQUIRE( applicationManager->getGraphicsSystem() );
            BOOST_REQUIRE( applicationManager->getResourceDatabase() );
            gameManager = applicationManager->getGameManager();
            BOOST_REQUIRE( gameManager );
            scene = gameManager->getCurrentScene();
            BOOST_REQUIRE( scene );
            cameraManager = applicationManager->getCameraManager();
            BOOST_REQUIRE( cameraManager );
            editorActor = cameraManager->getEditorCamera();
            BOOST_REQUIRE( editorActor );
            editorCamera = editorActor->getComponent<scene::Camera>();
            BOOST_REQUIRE( editorCamera );
            BOOST_REQUIRE( cameraManager->getEditorRTT() );
            // loadDebug disables the manager; test the production selection path.
            cameraManager->setEnabled( true );
            applicationManager->setProjectPath( "" );
            applicationManager->setCachePath( directory.generic_string().c_str() );
            scene->clear( true );
            scene->setFilePath( "" );
            scene->setLabel( "PlayModeTest" );
            scene->setSceneLoadingState( scene::IGameScene::SceneLoadingState::Loaded );
            gameManager->edit();
            settle();
            checkEdit();
        }

        ~PlayModeFixture()
        {
            // Do not enqueue transitions in cleanup: a failure must stay bounded.
            if( applicationManager )
            {
                applicationManager->setQuit( true );
                applicationManager->setRunning( false );
            }

            // Release fixture-held engine objects while their owning managers are
            // still alive; application teardown unloads and returns them to pools.
            editorCamera = nullptr;
            editorActor = nullptr;
            cameraManager = nullptr;
            scene = nullptr;
            gameManager = nullptr;
            applicationManager = nullptr;
            app.unload( nullptr );
            std::error_code ignored;
            std::filesystem::remove_all( directory, ignored );
        }

        void pump()
        {
            applicationManager->getJobQueue()->update();
            applicationManager->getTaskManager()->update();
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }

        template <class Predicate>
        void waitFor( Predicate predicate, const char *description )
        {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );
            while( !predicate() && std::chrono::steady_clock::now() < deadline )
                pump();
            BOOST_REQUIRE_MESSAGE( predicate(), description );
        }

        void settle()
        {
            waitFor( [this] { return !applicationManager->getJobQueue()->hasJobs(); },
                     "Editor job queue did not drain" );
            // Camera resets have a separate task queue. Barriers observe actual
            // application ticks, including jobs queued by preceding callbacks.
            for( int pass = 0; pass < 3; ++pass )
            {
                auto completed = std::make_shared<std::atomic_bool>( false );
                auto barrier = make_ptr<PlayModeBarrierJob>();
                barrier->completed = completed;
                auto task = applicationManager->getTaskManager()->getTask( TaskId::Application );
                BOOST_REQUIRE( task );
                task->addJob( barrier );
                waitFor( [completed] { return completed->load(); },
                         "Application task did not execute queued jobs" );
            }
            waitFor( [this] { return !applicationManager->getJobQueue()->hasJobs(); },
                     "Editor callbacks did not finish" );
        }

        SmartPtr<scene::IGameActor> addActor( const String &name )
        {
            auto actor = gameManager->createActor();
            BOOST_REQUIRE( actor );
            actor->setName( name );
            scene->addActor( actor );
            actor->setState( scene::IGameActor::State::Edit );
            return actor;
        }

        SmartPtr<scene::Camera> addCamera( const String &name = "GameCamera" )
        {
            auto camera = addActor( name )->addComponent<scene::Camera>();
            BOOST_REQUIRE( camera );
            settle();
            return camera;
        }

        SmartPtr<scene::Camera> gameCamera( const String &name = "GameCamera" )
        {
            auto actor = scene->findActorByName( name );
            BOOST_REQUIRE_MESSAGE( actor, "Game camera actor was lost" );
            auto camera = actor->getComponent<scene::Camera>();
            BOOST_REQUIRE( camera );
            return camera;
        }

        void checkRenderedCamera( SmartPtr<scene::Camera> camera, bool active )
        {
            BOOST_REQUIRE( camera );
            BOOST_CHECK_EQUAL( camera->isActive(), active );
            BOOST_REQUIRE_MESSAGE( camera->getCamera(), "Camera must have a renderer camera" );
            auto actor = camera->getActor();
            const auto description = actor ? String( "Camera viewport missing for " ) + actor->getName() +
                                             " (state=" + StringUtil::toString( static_cast<s32>( camera->getState() ) ) +
                                             ", target=" + StringUtil::toString( static_cast<bool>( camera->getTargetTexture() ) ) + ")"
                                           : String( "Camera viewport missing for unattached camera" );
            if( auto viewport = camera->getViewport() )
            {
                BOOST_CHECK_EQUAL( viewport->isActive(), active );
            }
            else
            {
                BOOST_CHECK_MESSAGE( !active, description );
            }
        }

        void checkEditorCamera( bool active )
        {
            BOOST_CHECK( cameraManager->getEditorCamera() == editorActor );
            BOOST_CHECK( editorActor->getFlag( scene::IGameActor::ActorFlagIsEditor ) );
            BOOST_CHECK( editorActor->getPerpetual() );
            BOOST_CHECK( editorActor->getComponent<scene::Camera>() == editorCamera );
            const auto cameras = editorActor->getAllComponentsAndInChildren<scene::Camera>();
            BOOST_REQUIRE_EQUAL( cameras.size(), 1u );
            // getComponent() alone misses extra cameras appended during restore.
            for( const auto &camera : cameras )
                checkRenderedCamera( camera, active );
            BOOST_CHECK_EQUAL( cameraManager->isEditorCameraEnabled(), active );
            BOOST_CHECK( editorCamera->getTargetTexture() == cameraManager->getEditorRTT() );
        }

        void checkRegistry()
        {
            const auto cameras = cameraManager->getCameras();
            for( size_t i = 0; i < cameras.size(); ++i )
            {
                BOOST_REQUIRE( cameras[i] );
                BOOST_CHECK( cameras[i]->isLoaded() );
                for( size_t j = i + 1; j < cameras.size(); ++j )
                    BOOST_CHECK( cameras[i] != cameras[j] );
            }
        }

        void checkEdit()
        {
            BOOST_CHECK( !applicationManager->isPlaying() );
            BOOST_CHECK( applicationManager->isEditorCamera() );
            BOOST_CHECK( !applicationManager->isPaused() );
            BOOST_CHECK( scene->getState() == scene::IGameScene::State::Edit );
            for( const auto &actor : scene->getActors() )
                BOOST_CHECK( actor->getState() == scene::IGameActor::State::Edit );
            checkEditorCamera( true );
        }

        void enter( bool hasGameCamera = true )
        {
            app.enterPlayMode();
            waitFor( [this] { return applicationManager->isPlaying(); }, "Play did not complete" );
            settle();
            BOOST_CHECK( !applicationManager->isEditorCamera() );
            BOOST_CHECK( !applicationManager->isPaused() );
            BOOST_CHECK( scene->getState() == scene::IGameScene::State::Play );
            for( const auto &actor : scene->getActors() )
                BOOST_CHECK( actor->getState() == scene::IGameActor::State::Play );
            checkEditorCamera( false );
            if( hasGameCamera )
            {
                auto camera = gameCamera();
                checkRenderedCamera( camera, true );
                BOOST_CHECK( camera->getTargetTexture() == cameraManager->getEditorRTT() );
            }
            checkRegistry();
        }

        void stop()
        {
            app.stopPlayMode();
            waitFor( [this] { return !applicationManager->isPlaying(); }, "Stop did not complete" );
            settle();
            checkEdit();
            for( const auto &camera : scene->getComponents<scene::Camera>() )
                checkRenderedCamera( camera, false );
            checkRegistry();
        }

        static std::string readFile( const std::filesystem::path &path )
        {
            std::ifstream stream( path, std::ios::binary );
            return std::string( std::istreambuf_iterator<char>( stream ), {} );
        }
    };

    struct WorkerPlayModeFixture : PlayModeFixture
    {
        WorkerPlayModeFixture() : PlayModeFixture( 2 ) {}
    };
}

BOOST_FIXTURE_TEST_CASE( play_mode, PlayModeFixture )
{
    addCamera();
    enter();
}

BOOST_FIXTURE_TEST_CASE( play_stop_mode, PlayModeFixture )
{
    auto originalCamera = addCamera();
    auto originalActor = originalCamera->getActor();
    originalActor->setLocalPosition( Vector3F( 1, 2, 3 ) );
    enter();
    gameCamera()->getActor()->setLocalPosition( Vector3F( 9, 8, 7 ) );
    addActor( "RuntimeOnly" );
    stop();
    BOOST_CHECK( !scene->findActorByName( "RuntimeOnly" ) );
    BOOST_CHECK_SMALL( ( gameCamera()->getActor()->getLocalPosition() - Vector3F( 1, 2, 3 ) ).length(), 0.001f );
    BOOST_CHECK( !originalCamera->isLoaded() );
    BOOST_CHECK( !originalActor->isLoaded() );
}

BOOST_FIXTURE_TEST_CASE( play_mode_repeated_cycles, PlayModeFixture )
{
    addCamera();
    for( int cycle = 0; cycle < 3; ++cycle )
    {
        BOOST_TEST_CONTEXT( "play/stop cycle " << cycle )
        {
            enter();
            BOOST_CHECK_EQUAL( scene->getComponents<scene::Camera>().size(), 1u );
            stop();
            BOOST_CHECK_EQUAL( scene->getComponents<scene::Camera>().size(), 1u );
            BOOST_CHECK( StringUtil::isNullOrEmpty( scene->getFilePath() ) );
        }
    }
}

BOOST_FIXTURE_TEST_CASE( play_mode_duplicate_requests, PlayModeFixture )
{
    auto camera = addCamera();
    camera->getActor()->setLocalPosition( Vector3F( 1, 2, 3 ) );
    app.enterPlayMode();
    app.enterPlayMode();
    waitFor( [this] { return applicationManager->isPlaying(); }, "Duplicate play did not complete" );
    settle();
    camera = gameCamera();
    camera->getActor()->setLocalPosition( Vector3F( 9, 9, 9 ) );
    app.enterPlayMode();
    settle();
    BOOST_CHECK( gameCamera() == camera );
    checkEditorCamera( false );
    app.stopPlayMode();
    app.stopPlayMode();
    settle();
    checkEdit();
    BOOST_CHECK_SMALL( ( gameCamera()->getActor()->getLocalPosition() - Vector3F( 1, 2, 3 ) ).length(), 0.001f );
    auto restoredCamera = gameCamera();
    app.stopPlayMode();
    settle();
    BOOST_CHECK( gameCamera() == restoredCamera );
    checkRegistry();
}

BOOST_FIXTURE_TEST_CASE( play_mode_rapid_requests, PlayModeFixture )
{
    addCamera();
    app.enterPlayMode();
    app.stopPlayMode();
    app.enterPlayMode();
    app.stopPlayMode();
    settle();
    checkEdit();
    checkRenderedCamera( gameCamera(), false );
    checkRegistry();
    enter();
    stop();
}

BOOST_FIXTURE_TEST_CASE( play_mode_pause_and_restart, PlayModeFixture )
{
    addCamera();
    applicationManager->setPaused( true );
    enter();
    applicationManager->setPaused( true );
    stop();
    enter();
    stop();
}

BOOST_FIXTURE_TEST_CASE( play_mode_camera_eligibility, PlayModeFixture )
{
    addCamera();
    auto disabledComponent = addCamera( "DisabledComponent" );
    disabledComponent->setEnabled( false );
    auto disabledActor = addCamera( "DisabledActor" );
    disabledActor->getActor()->setEnabled( false );
    auto disabledParent = addActor( "DisabledParent" );
    disabledParent->setEnabled( false );
    auto childCamera = addCamera( "DisabledChild" );
    scene->removeActor( childCamera->getActor() );
    disabledParent->addChild( childCamera->getActor() );
    auto editorOnly = addCamera( "EditorOnly" );
    editorOnly->getActor()->setFlag( scene::IGameActor::ActorFlagIsEditor, true );
    enter();
    checkRenderedCamera( disabledComponent, false );
    checkRenderedCamera( disabledActor, false );
    checkRenderedCamera( childCamera, false );
    checkRenderedCamera( editorOnly, false );
}

BOOST_FIXTURE_TEST_CASE( play_mode_no_game_camera, PlayModeFixture )
{
    BOOST_REQUIRE( scene->getComponents<scene::Camera>().empty() );
    enter( false );
    stop();
    BOOST_CHECK( scene->getComponents<scene::Camera>().empty() );
}

BOOST_FIXTURE_TEST_CASE( play_mode_editor_camera_override, PlayModeFixture )
{
    addCamera();
    enter();
    ToggleEditorCamera command;
    command.setToggleValue( true );
    command.execute();
    settle();
    BOOST_CHECK( applicationManager->isPlaying() );
    checkEditorCamera( true );
    checkRenderedCamera( gameCamera(), false );
    command.undo();
    settle();
    checkEditorCamera( false );
    checkRenderedCamera( gameCamera(), true );
    stop();
}

BOOST_FIXTURE_TEST_CASE( play_mode_saved_scene_preserves_edits, PlayModeFixture )
{
    auto camera = addCamera();
    const auto path = directory / "saved.fbscene";
    scene->saveScene( path.generic_string().c_str() );
    const auto diskBefore = readFile( path );
    BOOST_REQUIRE( !diskBefore.empty() );
    const auto originalPath = scene->getFilePath();
    const auto originalLabel = scene->getLabel();
    camera->getActor()->setLocalPosition( Vector3F( 3, 4, 5 ) );
    camera->setFOV( 73.0f );
    auto parent = addActor( "UnsavedParent" );
    auto child = addActor( "UnsavedChild" );
    scene->removeActor( child );
    parent->addChild( child );
    child->setLocalPosition( Vector3F( 6, 7, 8 ) );
    EditorManager::getSingletonPtr()->setFileSaved( false );
    enter();
    BOOST_CHECK_SMALL( ( gameCamera()->getActor()->getLocalPosition() - Vector3F( 3, 4, 5 ) ).length(), 0.001f );
    BOOST_REQUIRE( scene->findActorByName( "UnsavedParent" ) );
    gameCamera()->setFOV( 40.0f );
    gameCamera()->getActor()->setLocalPosition( Vector3F( 10, 10, 10 ) );
    stop();
    BOOST_CHECK_EQUAL( scene->getFilePath(), originalPath );
    BOOST_CHECK_EQUAL( scene->getLabel(), originalLabel );
    BOOST_CHECK( !EditorManager::getSingletonPtr()->getFileSaved() );
    BOOST_CHECK_EQUAL( readFile( path ), diskBefore );
    BOOST_CHECK_CLOSE( gameCamera()->getFOV(), 73.0f, 0.001f );
    BOOST_CHECK_SMALL( ( gameCamera()->getActor()->getLocalPosition() - Vector3F( 3, 4, 5 ) ).length(), 0.001f );
    parent = scene->findActorByName( "UnsavedParent" );
    BOOST_REQUIRE( parent );
    BOOST_REQUIRE_EQUAL( parent->getChildren().size(), 1u );
    BOOST_CHECK_EQUAL( parent->getChildren()[0]->getName(), "UnsavedChild" );
    BOOST_CHECK_SMALL( ( parent->getChildren()[0]->getLocalPosition() - Vector3F( 6, 7, 8 ) ).length(), 0.001f );
}

BOOST_FIXTURE_TEST_CASE( play_mode_unsaved_scene_cache_paths, PlayModeFixture )
{
    addCamera();
    for( const auto &cachePath : Array<String>{ String(), directory.generic_string().c_str() } )
    {
        applicationManager->setCachePath( cachePath );
        scene->setFilePath( "" );
        scene->setLabel( "Unsaved scene" );
        EditorManager::getSingletonPtr()->setFileSaved( false );
        enter();
        addActor( "RuntimeOnly" );
        stop();
        BOOST_CHECK( !scene->findActorByName( "RuntimeOnly" ) );
        BOOST_CHECK( StringUtil::isNullOrEmpty( scene->getFilePath() ) );
        BOOST_CHECK_EQUAL( scene->getLabel(), "Unsaved scene" );
        BOOST_CHECK( !EditorManager::getSingletonPtr()->getFileSaved() );
        BOOST_CHECK( !std::filesystem::exists( directory / "tmp.fbscene" ) );
    }
}

BOOST_FIXTURE_TEST_CASE( play_mode_runtime_scene_switch, PlayModeFixture )
{
    addCamera();
    const auto originalScene = scene;
    enter();
    auto runtimeScene = make_ptr<scene::GameScene>();
    runtimeScene->load( nullptr );
    runtimeScene->setSceneLoadingState( scene::IGameScene::SceneLoadingState::Loaded );
    gameManager->setCurrentScene( runtimeScene );
    auto runtimeActor = gameManager->createActor();
    runtimeActor->setName( "RuntimeSceneCamera" );
    auto runtimeCamera = runtimeActor->addComponent<scene::Camera>();
    runtimeScene->addActor( runtimeActor );
    gameManager->play();
    settle();
    checkRenderedCamera( runtimeCamera, true );
    stop();
    BOOST_CHECK( gameManager->getCurrentScene() == originalScene );
    BOOST_REQUIRE( originalScene->findActorByName( "GameCamera" ) );
    BOOST_CHECK( runtimeScene->getActors().empty() );
    BOOST_CHECK( !runtimeActor->isLoaded() );
    BOOST_CHECK( !runtimeCamera->isLoaded() );
    runtimeScene->unload( nullptr );
}

BOOST_FIXTURE_TEST_CASE( play_mode_failed_load_does_not_enter, PlayModeFixture )
{
    addCamera();
    scene->setFilePath( "missing-playmode-test.fbscene" );
    scene->setSceneLoadingState( scene::IGameScene::SceneLoadingState::None );
    app.enterPlayMode();
    settle();
    checkEdit();
    BOOST_REQUIRE( scene->findActorByName( "GameCamera" ) );
}

BOOST_FIXTURE_TEST_CASE( play_mode_without_scene, PlayModeFixture )
{
    gameManager->setCurrentScene( nullptr );
    app.enterPlayMode();
    // Execute only the rejected transition while no scene is installed. Other
    // editor tasks assume a current scene during their unrelated UI updates.
    applicationManager->getJobQueue()->update();
    BOOST_CHECK( !applicationManager->isPlaying() );
    BOOST_CHECK( applicationManager->isEditorCamera() );
    BOOST_CHECK( !gameManager->getCurrentScene() );
    gameManager->setCurrentScene( scene );
    settle();
    checkEdit();
}

BOOST_FIXTURE_TEST_CASE( play_mode_scene_data_preserves_editor_camera, PlayModeFixture )
{
    addCamera();
    if( !editorActor->getComponent<scene::SphericalCameraController>() )
        editorActor->addComponent<scene::SphericalCameraController>();
    const auto controller = editorActor->getComponent<scene::SphericalCameraController>();
    editorCamera->setFOV( 67.0f );
    const auto data = scene->toData();
    for( int reload = 0; reload < 3; ++reload )
    {
        scene->clear( true );
        scene->fromData( data );
        gameManager->edit();
        settle();
        checkEditorCamera( true );
        const auto controllers = editorActor->getComponentsByType<scene::SphericalCameraController>();
        BOOST_REQUIRE_EQUAL( controllers.size(), 1u );
        BOOST_CHECK( editorActor->getComponent<scene::SphericalCameraController>() == controller );
        BOOST_CHECK_CLOSE( editorCamera->getFOV(), 67.0f, 0.001f );
        BOOST_CHECK_EQUAL( scene->getComponents<scene::Camera>().size(), 1u );
        checkRegistry();
    }
    enter();
    stop();
}

BOOST_FIXTURE_TEST_CASE( play_mode_legacy_editor_camera_migration, PlayModeFixture )
{
    addCamera();
    if( !editorActor->getComponent<scene::SphericalCameraController>() )
        editorActor->addComponent<scene::SphericalCameraController>();
    auto data = workphone::static_pointer_cast<Properties>( scene->toData() );
    auto legacy = workphone::static_pointer_cast<Properties>( editorActor->toData() );
    legacy->setName( ApplicationUtil::actorsStr );
    data->removeChild( "editorCamera" );
    data->addChild( legacy );
    scene->clear( true );
    scene->fromData( data );
    gameManager->edit();
    settle();
    BOOST_CHECK( !scene->findActorByName( "EditorCamera" ) );
    BOOST_CHECK_EQUAL( scene->getComponents<scene::Camera>().size(), 1u );
    checkEditorCamera( true );
    enter();
    stop();
}

BOOST_FIXTURE_TEST_CASE( play_mode_file_camera_migration, PlayModeFixture )
{
    addCamera();
    if( !editorActor->getComponent<scene::SphericalCameraController>() )
        editorActor->addComponent<scene::SphericalCameraController>();
    for( bool legacyFormat : { false, true } )
    {
        auto data = workphone::static_pointer_cast<Properties>( scene->toData() );
        if( legacyFormat )
        {
            auto legacy = workphone::static_pointer_cast<Properties>( editorActor->toData() );
            legacy->setName( ApplicationUtil::actorsStr );
            data->removeChild( "editorCamera" );
            data->addChild( legacy );
        }
        const auto path = ( directory / ( legacyFormat ? "legacy.fbscene" : "modern.fbscene" ) ).generic_string();
        applicationManager->getFileSystem()->writeAllText( path.c_str(), DataUtil::toString( data.get(), true, DataFormat::JSON ) );
        scene->clear( true );
        scene->loadScene( path.c_str(), false );
        settle();
        BOOST_REQUIRE( scene->getSceneLoadingState() == scene::IGameScene::SceneLoadingState::Loaded );
        BOOST_CHECK( !scene->findActorByName( "EditorCamera" ) );
        BOOST_CHECK_EQUAL( scene->getComponents<scene::Camera>().size(), 1u );
        checkEditorCamera( true );
        enter();
        stop();
    }
}

BOOST_FIXTURE_TEST_CASE( play_mode_worker_transitions, WorkerPlayModeFixture )
{
    BOOST_REQUIRE_EQUAL( applicationManager->getThreadPool()->getNumThreads(), 2u );
    addCamera();
    for( int cycle = 0; cycle < 3; ++cycle )
    {
        enter();
        stop();
    }
    app.enterPlayMode();
    app.stopPlayMode();
    app.enterPlayMode();
    app.stopPlayMode();
    settle();
    checkEdit();
    checkRenderedCamera( gameCamera(), false );
    checkRegistry();
}

BOOST_FIXTURE_TEST_CASE( play_mode_worker_latest_scene_request, WorkerPlayModeFixture )
{
    auto pool = applicationManager->getThreadPool();
    for( u32 index = 0; index < pool->getNumThreads(); ++index )
        BOOST_CHECK_EQUAL( static_cast<s32>( pool->getThread( index )->getThreadId() ),
                           static_cast<s32>( index ) );
    auto camera = addCamera();
    {
        auto lock = applicationManager->getTaskManager()->lockTask( TaskId::Application );
        gameManager->play();
        gameManager->edit();
    }
    settle();
    checkEdit();
    BOOST_CHECK( camera->getState() == scene::IComponent::State::Edit );
    checkRenderedCamera( camera, false );
}

BOOST_FIXTURE_TEST_CASE( play_mode_editor_camera_data_precedence, PlayModeFixture )
{
    addCamera();
    addCamera( "EditorCamera" );
    if( !editorActor->getComponent<scene::SphericalCameraController>() )
        editorActor->addComponent<scene::SphericalCameraController>();
    const auto controller = editorActor->getComponent<scene::SphericalCameraController>();
    editorCamera->setFOV( 67.0f );
    auto data = workphone::static_pointer_cast<Properties>( scene->toData() );
    editorCamera->setFOV( 91.0f );
    auto legacy = workphone::static_pointer_cast<Properties>( editorActor->toData() );
    legacy->setName( ApplicationUtil::actorsStr );
    auto wrapper = make_ptr<Properties>();
    wrapper->setName( "components" );
    const auto components = legacy->getChildrenByName( "component" );
    legacy->removeChild( "component" );
    for( auto component : components )
    {
        String type;
        component->getPropertyValue( "componentType", type );
        if( type == "workphone::scene::SphericalCameraController" )
            component->setProperty( "componentType", String( "fb::scene::SphericalCameraController" ) );
        wrapper->addChild( component );
    }
    legacy->addChild( wrapper );
    data->addChild( legacy );
    scene->clear( true );
    scene->fromData( data );
    gameManager->edit();
    settle();
    auto namedGameCamera = scene->findActorByName( "EditorCamera" );
    BOOST_REQUIRE( namedGameCamera );
    BOOST_CHECK( namedGameCamera != editorActor );
    BOOST_CHECK_EQUAL( scene->getComponents<scene::Camera>().size(), 2u );
    BOOST_CHECK_CLOSE( editorCamera->getFOV(), 67.0f, 0.001f );
    BOOST_CHECK( editorActor->getComponent<scene::SphericalCameraController>() == controller );
    checkEditorCamera( true );
    enter();
    checkRenderedCamera( gameCamera( "EditorCamera" ), true );
    stop();
}

#endif
