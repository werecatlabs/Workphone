#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

namespace
{
    // Returns the graphics system, or nullptr if unavailable.
    SmartPtr<render::IGraphicsSystem> getGraphicsSystem()
    {
        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
            return nullptr;
        return applicationManager->getGraphicsSystem();
    }

    // Runs a fixed number of full engine update cycles.
    void runUpdateCycles( u32 count )
    {
        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
            return;

        auto timer = applicationManager->getTimer();
        auto stateManager = applicationManager->getStateManager();
        auto sceneManager = applicationManager->getGameManager();
        auto graphicsSystem = applicationManager->getGraphicsSystem();

        for( u32 i = 0; i < count; ++i )
        {
            if( timer )
                timer->update();

            if( stateManager )
            {
                stateManager->preUpdate();
                stateManager->update();
                stateManager->postUpdate();
            }

            if( sceneManager )
            {
                sceneManager->preUpdate();
                sceneManager->update();
                sceneManager->postUpdate();
            }

            if( graphicsSystem )
            {
                graphicsSystem->messagePump();
                graphicsSystem->update();
            }
        }
    }
}  // namespace

// Test that the application manager and its core subsystems are available.
BOOST_AUTO_TEST_CASE( graphics_terrain_application_manager )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        BOOST_CHECK( applicationManager->getTimer() );
        BOOST_CHECK( applicationManager->getStateManager() );
        BOOST_CHECK( applicationManager->getGameManager() );
        BOOST_CHECK( applicationManager->getFactoryManager() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during application manager test." );
    }
}

// Test that a terrain actor can be created and added to the scene.
BOOST_AUTO_TEST_CASE( graphics_system_terrain )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            WP_LOG_ERROR( "Graphics system is not initialized." );
            return;
        }

        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto terrain = ApplicationUtil::createDefaultTerrain();
        BOOST_REQUIRE( terrain );

        runUpdateCycles( 10 );

        sceneManager->destroyActor( terrain );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during terrain creation test." );
    }
}

// Test that a terrain actor can be created without being added to the scene.
BOOST_AUTO_TEST_CASE( graphics_terrain_create_without_scene )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            WP_LOG_ERROR( "Graphics system is not initialized." );
            return;
        }

        // addToScene = false: actor should be created but not registered in the scene.
        auto terrain = ApplicationUtil::createDefaultTerrain( false );
        BOOST_REQUIRE( terrain );

        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        // The terrain should NOT appear in the active scene actor list.
        if( scene )
        {
            auto actors = scene->getActors();
            bool found = false;
            for( const auto &actor : actors )
            {
                if( actor == terrain )
                {
                    found = true;
                    break;
                }
            }
            BOOST_CHECK( !found );
        }

        // Explicit destroy (not in scene, so use direct release).
        terrain = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during terrain-without-scene test." );
    }
}

// Test that creating and destroying terrain multiple times does not crash or leak.
BOOST_AUTO_TEST_CASE( graphics_terrain_create_destroy_repeated )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            WP_LOG_ERROR( "Graphics system is not initialized." );
            return;
        }

        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        const u32 iterations = 5;
        for( u32 i = 0; i < iterations; ++i )
        {
            auto terrain = ApplicationUtil::createDefaultTerrain();
            BOOST_REQUIRE( terrain );

            runUpdateCycles( 2 );

            sceneManager->destroyActor( terrain );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during repeated create/destroy test." );
    }
}

// Test that multiple terrain actors can coexist in the scene simultaneously.
BOOST_AUTO_TEST_CASE( graphics_terrain_multiple_actors )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            WP_LOG_ERROR( "Graphics system is not initialized." );
            return;
        }

        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto terrain1 = ApplicationUtil::createDefaultTerrain();
        auto terrain2 = ApplicationUtil::createDefaultTerrain();

        BOOST_REQUIRE( terrain1 );
        BOOST_REQUIRE( terrain2 );
        BOOST_CHECK( terrain1 != terrain2 );

        runUpdateCycles( 5 );

        sceneManager->destroyActor( terrain1 );
        sceneManager->destroyActor( terrain2 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during multiple terrain actors test." );
    }
}

// Test that the engine update loop runs stably with a terrain in the scene.
BOOST_AUTO_TEST_CASE( graphics_terrain_update_loop_stability )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            WP_LOG_ERROR( "Graphics system is not initialized." );
            return;
        }

        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto terrain = ApplicationUtil::createDefaultTerrain();
        BOOST_REQUIRE( terrain );

        // Run a longer update cycle to surface any per-frame issues.
        runUpdateCycles( 30 );

        sceneManager->destroyActor( terrain );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during update loop stability test." );
    }
}

// Test that destroying a terrain actor with a null graphics system does not crash.
BOOST_AUTO_TEST_CASE( graphics_terrain_no_graphics_system )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( graphicsSystem )
        {
            // Graphics is present — test is not applicable; pass silently.
            return;
        }

        // Without a graphics system, createDefaultTerrain should either return
        // nullptr or a valid (non-rendering) actor without crashing.
        auto terrain = ApplicationUtil::createDefaultTerrain();
        auto sceneManager = applicationManager->getGameManager();
        if( terrain && sceneManager )
            sceneManager->destroyActor( terrain );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in no-graphics-system terrain test." );
    }
}

// Test that the current scene is accessible and valid after terrain creation.
BOOST_AUTO_TEST_CASE( graphics_terrain_scene_validity )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            WP_LOG_ERROR( "Graphics system is not initialized." );
            return;
        }

        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto terrain = ApplicationUtil::createDefaultTerrain();
        BOOST_REQUIRE( terrain );

        auto scene = sceneManager->getCurrentScene();
        BOOST_CHECK( scene );

        sceneManager->destroyActor( terrain );

        // Scene should still be valid after the actor is destroyed.
        scene = sceneManager->getCurrentScene();
        BOOST_CHECK( scene );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during scene validity test." );
    }
}
