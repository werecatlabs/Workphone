#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <cstdlib>

using namespace workphone;

BOOST_AUTO_TEST_CASE( gamecameramanager_legacy_editor_camera_does_not_cover_game_camera )
{
    TestGuard guard;
    guard.scene->clear( true );
    auto previousManager = guard.applicationManager->getCameraManager();
    const auto wasEditorCamera = guard.applicationManager->isEditorCamera();
    auto cameraManager = make_ptr<scene::CameraManager>();
    cameraManager->load( nullptr );
    guard.applicationManager->setCameraManager( cameraManager );
    guard.addCleanup( [app = guard.applicationManager, previousManager, cameraManager,
                       wasEditorCamera]() mutable {
        app->setEditorCamera( wasEditorCamera );
        cameraManager->unload( nullptr );
        app->setCameraManager( previousManager );
    } );

    auto editorActor = guard.sceneManager->createActor();
    editorActor->setName( "EditorCamera" );
    editorActor->setFlag( scene::IGameActor::ActorFlagIsEditor, true );
    cameraManager->setEditorCamera( editorActor );
    auto editorCamera = editorActor->addComponent<scene::Camera>();
    editorActor->addComponent<scene::SphericalCameraController>();
    editorActor->setState( scene::IGameActor::State::Edit );
    auto legacyEditorData = workphone::static_pointer_cast<Properties>( editorActor->toData() );
    legacyEditorData->setName( ApplicationUtil::actorsStr );

    auto gameActor = guard.sceneManager->createActor();
    gameActor->setName( "GameCamera" );
    gameActor->addComponent<scene::Camera>();
    auto gameData = workphone::static_pointer_cast<Properties>( gameActor->toData() );
    gameData->setName( ApplicationUtil::actorsStr );
    guard.sceneManager->destroyActor( gameActor );

    auto sceneData = make_ptr<Properties>();
    sceneData->addChild( gameData );
    sceneData->addChild( legacyEditorData );
    const auto scenePath = std::getenv( "WP_TEST_CAMERA_SCENE" );
    if( scenePath )
        guard.scene->loadScene( String( scenePath ), false );
    else
        guard.scene->fromData( sceneData );
    editorCamera = editorActor->getComponent<scene::Camera>();
    BOOST_REQUIRE( editorCamera );
    guard.scene->setState( scene::IGameScene::State::Play );
    for( u32 frame = 0; frame < 10; ++frame )
    {
        guard.applicationManager->getJobQueue()->update();
        guard.taskManager->update();
        guard.runUpdateCycle();
    }
    guard.applicationManager->setEditorCamera( false );
    cameraManager->reset();
    auto cameras = guard.scene->getComponents<scene::Camera>();
    BOOST_REQUIRE_EQUAL( cameras.size(), 1u );
    BOOST_CHECK_EQUAL( cameras.front()->getActor()->getName(), scenePath ? "Camera" : "GameCamera" );
    BOOST_CHECK( cameras.front()->isActive() );
    BOOST_CHECK( !editorCamera->isActive() );
    BOOST_CHECK( !guard.scene->findActorByName( "EditorCamera" ) );

    guard.applicationManager->setEditorCamera( true );
    cameraManager->reset();
    BOOST_CHECK( editorCamera->isActive() );
    BOOST_CHECK( !cameras.front()->isActive() );

    // Saving an editor actor in the scene must use the editorCamera entry only.
    const auto gameActorCount = guard.scene->getActors().size();
    guard.scene->addActor( editorActor );
    auto saved = workphone::static_pointer_cast<Properties>( guard.scene->toData() );
    BOOST_CHECK_EQUAL( saved->getChildrenByName( ApplicationUtil::actorsStr ).size(), gameActorCount );
    BOOST_CHECK( saved->getChild( "editorCamera" ) );
    guard.scene->clear( true );
}

// Helper macro for logging test state with context
#define BOOST_TEST_LOG_MESSAGE( msg ) \
    BOOST_TEST_MESSAGE( "[" << boost::unit_test::framework::current_test_case().p_name << "] " << msg )

// Helper macro for checking with detailed logging
#define BOOST_CHECK_WITH_LOG( condition, message )           \
    do                                                       \
    {                                                        \
        if( !( condition ) )                                 \
        {                                                    \
            BOOST_TEST_LOG_MESSAGE( "FAILED: " << message ); \
        }                                                    \
        else                                                 \
        {                                                    \
            BOOST_TEST_LOG_MESSAGE( "PASSED: " << message ); \
        }                                                    \
        BOOST_CHECK( condition );                            \
    } while( 0 )

BOOST_AUTO_TEST_CASE( gamecameramanager_reset )
{
    BOOST_TEST_LOG_MESSAGE( "Starting camera manager reset test" );

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto cameraManager = applicationManager->getCameraManager();
        if( !cameraManager )
        {
            BOOST_TEST_MESSAGE( "Camera manager is not available - skipping test" );
            return;
        }

        BOOST_CHECK_NO_THROW( cameraManager->reset() );
        BOOST_TEST_LOG_MESSAGE( "Camera manager reset completed successfully" );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamecameramanager_reset test: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( gamecameramanager_get_cameras )
{
    BOOST_TEST_LOG_MESSAGE( "Starting get cameras test" );

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto cameraManager = applicationManager->getCameraManager();
        if( !cameraManager )
        {
            BOOST_TEST_MESSAGE( "Camera manager is not available - skipping test" );
            return;
        }

        auto cameras = cameraManager->getCameras();
        BOOST_TEST_LOG_MESSAGE( "Camera count: " << cameras.size() );
        BOOST_CHECK_WITH_LOG( true, "getCameras() returned without error" );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamecameramanager_get_cameras test: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( gamecameramanager_find_camera )
{
    BOOST_TEST_LOG_MESSAGE( "Starting find camera test" );

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto cameraManager = applicationManager->getCameraManager();
        if( !cameraManager )
        {
            BOOST_TEST_MESSAGE( "Camera manager is not available - skipping test" );
            return;
        }

        // Finding a camera by a non-existent name should return null without throwing
        auto result = cameraManager->findCamera( "NonExistentCamera" );
        BOOST_CHECK_WITH_LOG( !result, "findCamera() should return null for non-existent camera name" );

        // If cameras exist, verify we can find one by its name
        auto cameras = cameraManager->getCameras();
        if( !cameras.empty() && cameras[0] )
        {
            auto name = cameras[0]->getName();
            auto found = cameraManager->findCamera( name );
            BOOST_CHECK_WITH_LOG( found != nullptr,
                                  "findCamera() should find an existing camera by name" );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamecameramanager_find_camera test: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( gamecameramanager_editor_camera )
{
    BOOST_TEST_LOG_MESSAGE( "Starting editor camera accessor test" );

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto cameraManager = applicationManager->getCameraManager();
        if( !cameraManager )
        {
            BOOST_TEST_MESSAGE( "Camera manager is not available - skipping test" );
            return;
        }

        auto editorCamera = cameraManager->getEditorCamera();
        auto editorCameraPtr = cameraManager->getEditorCameraPtr();
        BOOST_TEST_LOG_MESSAGE( "Editor camera is " << ( editorCamera ? "set" : "null" ) );

        // Both accessors should agree on whether the editor camera is set
        BOOST_CHECK_WITH_LOG( ( editorCamera != nullptr ) == ( editorCameraPtr != nullptr ),
                              "getEditorCamera() and getEditorCameraPtr() should be consistent" );

        // Round-trip: set the editor camera to itself and verify retrieval
        if( editorCamera )
        {
            cameraManager->setEditorCamera( editorCamera );
            auto retrieved = cameraManager->getEditorCamera();
            BOOST_CHECK_WITH_LOG(
                retrieved == editorCamera,
                "setEditorCamera()/getEditorCamera() round-trip should return the same camera" );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamecameramanager_editor_camera test: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( gamecameramanager_editor_camera_enabled )
{
    BOOST_TEST_LOG_MESSAGE( "Starting editor camera enabled accessor test" );

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto cameraManager = applicationManager->getCameraManager();
        if( !cameraManager )
        {
            BOOST_TEST_MESSAGE( "Camera manager is not available - skipping test" );
            return;
        }

        // isEditorCameraEnabled should not throw
        bool enabled = false;
        BOOST_CHECK_NO_THROW( enabled = cameraManager->isEditorCameraEnabled() );
        BOOST_TEST_LOG_MESSAGE( "Editor camera enabled: " << enabled );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamecameramanager_editor_camera_enabled test: " +
                    String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( gamecameramanager_enabled )
{
    BOOST_TEST_LOG_MESSAGE( "Starting camera manager enabled accessor test" );

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto cameraManager = applicationManager->getCameraManager();
        if( !cameraManager )
        {
            BOOST_TEST_MESSAGE( "Camera manager is not available - skipping test" );
            return;
        }

        // Enable and verify
        cameraManager->setEnabled( true );
        BOOST_CHECK_WITH_LOG( cameraManager->isEnabled(),
                              "Camera manager should be enabled after setEnabled(true)" );

        // Disable and verify
        cameraManager->setEnabled( false );
        BOOST_CHECK_WITH_LOG( !cameraManager->isEnabled(),
                              "Camera manager should be disabled after setEnabled(false)" );

        // Restore to enabled
        cameraManager->setEnabled( true );
        BOOST_CHECK_WITH_LOG( cameraManager->isEnabled(),
                              "Camera manager should be enabled after restoring setEnabled(true)" );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamecameramanager_enabled test: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( gamecameramanager_state )
{
    BOOST_TEST_LOG_MESSAGE( "Starting camera manager state accessor test" );

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto cameraManager = applicationManager->getCameraManager();
        if( !cameraManager )
        {
            BOOST_TEST_MESSAGE( "Camera manager is not available - skipping test" );
            return;
        }

        using State = scene::ICameraManager::State;

        // Round-trip each meaningful state
        cameraManager->setState( State::Edit );
        BOOST_CHECK_WITH_LOG( cameraManager->getState() == State::Edit,
                              "getState() should return Edit after setState(Edit)" );

        cameraManager->setState( State::Play );
        BOOST_CHECK_WITH_LOG( cameraManager->getState() == State::Play,
                              "getState() should return Play after setState(Play)" );

        cameraManager->setState( State::Reset );
        BOOST_CHECK_WITH_LOG( cameraManager->getState() == State::Reset,
                              "getState() should return Reset after setState(Reset)" );

        cameraManager->setState( State::None );
        BOOST_CHECK_WITH_LOG( cameraManager->getState() == State::None,
                              "getState() should return None after setState(None)" );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamecameramanager_state test: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( gamecameramanager_add_remove_camera )
{
    BOOST_TEST_LOG_MESSAGE( "Starting add/remove camera test" );

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto cameraManager = applicationManager->getCameraManager();
        if( !cameraManager )
        {
            BOOST_TEST_MESSAGE( "Camera manager is not available - skipping test" );
            return;
        }

        auto gameManager = applicationManager->getGameManager();
        if( !gameManager )
        {
            BOOST_TEST_MESSAGE( "Game manager is not available - skipping test" );
            return;
        }

        auto scene = gameManager->getCurrentScene();
        if( !scene )
        {
            BOOST_TEST_MESSAGE( "No current scene available - skipping test" );
            return;
        }

        auto countBefore = cameraManager->getCameras().size();

        // Create a new camera actor and add it
        auto cameraActor = gameManager->createActor();
        if( !cameraActor )
        {
            BOOST_TEST_MESSAGE( "Could not create camera actor - skipping add/remove test" );
            return;
        }

        auto camera = cameraActor->addComponent<scene::Camera>();
        cameraManager->addCamera( cameraActor );
        auto countAfter = cameraManager->getCameras().size();
        BOOST_CHECK_WITH_LOG( countAfter == countBefore + 1,
                              "Camera count should increase by 1 after addCamera()" );

        // Remove it and verify the count returns to the original
        bool removed = cameraManager->removeCamera( cameraActor );
        BOOST_CHECK_WITH_LOG( removed, "removeCamera() should return true for a known camera" );
        BOOST_CHECK_WITH_LOG( cameraManager->getCameras().size() == countBefore,
                              "Camera count should return to original after removeCamera()" );

        // Removing a camera that is no longer managed should return false
        bool removedAgain = cameraManager->removeCamera( cameraActor );
        BOOST_CHECK_WITH_LOG( !removedAgain,
                              "removeCamera() should return false for a camera that is not managed" );

        gameManager->destroyActor( cameraActor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamecameramanager_add_remove_camera test: " +
                    String( e.what() ) );
    }
}
