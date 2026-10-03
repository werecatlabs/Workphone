#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    bool skipWhenGraphicsRuntimeUnavailable()
    {
        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
        {
            BOOST_TEST_MESSAGE(
                "Application manager is not available - skipping graphics component test" );
            return true;
        }

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem || !graphicsSystem->getGraphicsScene() )
        {
            BOOST_TEST_MESSAGE( "Graphics runtime is not available - skipping graphics component test" );
            return true;
        }

        return false;
    }
}  // namespace

/// @brief Test basic loading and unloading of graphics renderer components
BOOST_AUTO_TEST_CASE( components_graphics_load )
{
    if( skipWhenGraphicsRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_REQUIRE( factoryManager );

        auto typeManager = TypeManager::instance();
        BOOST_REQUIRE( typeManager );

        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        BOOST_REQUIRE( scene );

        auto factories = factoryManager->getFactories();
        BOOST_CHECK( !factories.empty() );

        for( auto factory : factories )
        {
            if( factory && factory->isObjectDerivedFrom<scene::Renderer>() )
            {
                auto actor = sceneManager->createActor();
                BOOST_REQUIRE( actor );

                auto component = factory->make_ptr<scene::Renderer>();
                if( component )
                {
                    auto componentName = typeManager->getName( component->getTypeInfo() );

                    actor->addComponentInstance( component );
                    component->load( nullptr );
                    BOOST_CHECK( component->isLoaded() );
                    WP_LOG_ERROR( String( "Component failed to load: " ) + componentName );

                    if( !component->isLoaded() )
                    {
                        auto message = String( "Component failed to load: " ) + componentName;
                        WP_LOG( message );
                    }

                    // Explicitly unload before destroying
                    component->unload( nullptr );
                    BOOST_CHECK( !component->isLoaded() );
                }

                sceneManager->destroyActor( actor );
            }
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/// @brief Test double loading and unloading of renderer components
BOOST_AUTO_TEST_CASE( components_graphics_double_load_unload )
{
    if( skipWhenGraphicsRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();
        auto sceneManager = applicationManager->getGameManager();
        auto typeManager = TypeManager::instance();

        auto factories = factoryManager->getFactories();
        for( auto factory : factories )
        {
            if( factory && factory->isObjectDerivedFrom<scene::Renderer>() )
            {
                auto actor = sceneManager->createActor();
                auto component = factory->make_ptr<scene::Renderer>();

                if( component )
                {
                    auto componentName = typeManager->getName( component->getTypeInfo() );
                    actor->addComponentInstance( component );

                    // Test double load
                    component->load( nullptr );
                    BOOST_CHECK( component->isLoaded() );
                    component->load( nullptr );
                    BOOST_CHECK( component->isLoaded() );
                    //,"Component should remain loaded after double load: " + componentName );

                    // Test double unload
                    component->unload( nullptr );
                    BOOST_CHECK( !component->isLoaded() );
                    component->unload( nullptr );
                    BOOST_CHECK( !component->isLoaded() );
                    //"Component should remain unloaded after double unload: " + componentName );
                }

                sceneManager->destroyActor( actor );
                break;  // Test with first available component only
            }
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/// @brief Test unloading component before loading
BOOST_AUTO_TEST_CASE( components_graphics_unload_before_load )
{
    if( skipWhenGraphicsRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();
        auto sceneManager = applicationManager->getGameManager();
        auto typeManager = TypeManager::instance();

        auto factories = factoryManager->getFactories();
        for( auto factory : factories )
        {
            if( factory && factory->isObjectDerivedFrom<scene::Renderer>() )
            {
                auto actor = sceneManager->createActor();
                auto component = factory->make_ptr<scene::Renderer>();

                if( component )
                {
                    auto componentName = typeManager->getName( component->getTypeInfo() );
                    actor->addComponentInstance( component );

                    // Unload without loading first
                    BOOST_CHECK_NO_THROW( component->unload( nullptr ) );
                    BOOST_CHECK( !component->isLoaded() );
                    //"Component should not be loaded: " + componentName );

                    // Now load normally
                    component->load( nullptr );
                    BOOST_CHECK( component->isLoaded() );
                    component->unload( nullptr );
                }

                sceneManager->destroyActor( actor );
                break;  // Test with first available component only
            }
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/// @brief Test multiple renderer components on the same actor
BOOST_AUTO_TEST_CASE( components_graphics_multiple_components )
{
    if( skipWhenGraphicsRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();
        auto sceneManager = applicationManager->getGameManager();
        auto typeManager = TypeManager::instance();

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor );

        SmartPtr<scene::Renderer> components[3];
        int componentCount = 0;

        auto factories = factoryManager->getFactories();
        for( auto factory : factories )
        {
            if( factory && factory->isObjectDerivedFrom<scene::Renderer>() && componentCount < 3 )
            {
                auto component = factory->make_ptr<scene::Renderer>();
                if( component )
                {
                    auto componentName = typeManager->getName( component->getTypeInfo() );

                    actor->addComponentInstance( component );
                    component->load( nullptr );

                    BOOST_CHECK( component->isLoaded() );
                    WP_LOG_ERROR( String( "Component failed to load: " ) + componentName );

                    components[componentCount++] = component;
                }
            }

            if( componentCount >= 3 )
                break;
        }

        // Verify all components are still loaded
        for( int i = 0; i < componentCount; ++i )
        {
            BOOST_CHECK( components[i]->isLoaded() );
        }

        // Unload all components
        for( int i = 0; i < componentCount; ++i )
        {
            components[i]->unload( nullptr );
            BOOST_CHECK( !components[i]->isLoaded() );
        }

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/// @brief Test component state after actor destruction
BOOST_AUTO_TEST_CASE( components_graphics_actor_destruction )
{
    if( skipWhenGraphicsRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();
        auto sceneManager = applicationManager->getGameManager();
        auto typeManager = TypeManager::instance();

        SmartPtr<scene::Renderer> component;

        auto factories = factoryManager->getFactories();
        for( auto factory : factories )
        {
            if( factory && factory->isObjectDerivedFrom<scene::Renderer>() )
            {
                auto actor = sceneManager->createActor();
                component = factory->make_ptr<scene::Renderer>();

                if( component )
                {
                    auto componentName = typeManager->getName( component->getTypeInfo() );

                    actor->addComponentInstance( component );
                    component->load( nullptr );
                    BOOST_CHECK( component->isLoaded() );

                    // Destroy actor while component is loaded
                    sceneManager->destroyActor( actor );

                    // Component should handle cleanup gracefully
                    BOOST_CHECK_NO_THROW( component->unload( nullptr ) );
                }
                break;
            }
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/// @brief Test component lifecycle without actor attachment
BOOST_AUTO_TEST_CASE( components_graphics_no_actor )
{
    if( skipWhenGraphicsRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();
        auto typeManager = TypeManager::instance();

        auto factories = factoryManager->getFactories();
        for( auto factory : factories )
        {
            if( factory && factory->isObjectDerivedFrom<scene::Renderer>() )
            {
                auto component = factory->make_ptr<scene::Renderer>();

                if( component )
                {
                    auto componentName = typeManager->getName( component->getTypeInfo() );

                    // Try to load without attaching to actor
                    BOOST_CHECK_NO_THROW( component->load( nullptr ) );

                    // Component may or may not load successfully without actor
                    // Just ensure it doesn't crash
                    BOOST_CHECK_NO_THROW( component->unload( nullptr ) );
                }
                break;
            }
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/// @brief Test scene clearing with loaded components
BOOST_AUTO_TEST_CASE( components_graphics_scene_clear )
{
    if( skipWhenGraphicsRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();
        auto typeManager = TypeManager::instance();

        SmartPtr<scene::IGameActor> actors[5];
        int actorCount = 0;

        auto factories = factoryManager->getFactories();
        for( auto factory : factories )
        {
            if( factory && factory->isObjectDerivedFrom<scene::Renderer>() && actorCount < 5 )
            {
                auto actor = sceneManager->createActor();
                auto component = factory->make_ptr<scene::Renderer>();

                if( component )
                {
                    actor->addComponentInstance( component );
                    component->load( nullptr );
                    actors[actorCount++] = actor;
                }
            }

            if( actorCount >= 5 )
                break;
        }

        // Clear scene with loaded components - should not crash
        BOOST_CHECK_NO_THROW( scene->clear() );
        BOOST_CHECK_NO_THROW( sceneManager->clear() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( cubemap_update_strategy_configuration )
{
    scene::Cubemap cubemap;

    BOOST_CHECK( cubemap.getRefreshMode() == scene::Cubemap::RefreshMode::OnAwake );
    BOOST_CHECK( cubemap.getUseTimeSlicing() );
    BOOST_CHECK_EQUAL( cubemap.getFaceMask(), scene::Cubemap::FaceAll );

    cubemap.setUseTimeSlicing( false );
    BOOST_CHECK( !cubemap.getUseTimeSlicing() );

    cubemap.setRefreshMode( scene::Cubemap::RefreshMode::Interval );
    cubemap.setUpdateInterval( 2.5f );
    BOOST_CHECK( cubemap.getRefreshMode() == scene::Cubemap::RefreshMode::Interval );
    BOOST_CHECK_CLOSE( cubemap.getUpdateInterval(), 2.5f, 0.001f );

    cubemap.setFaceMask( scene::Cubemap::FacePositiveX | scene::Cubemap::FaceNegativeZ | ( 1u << 12u ) );
    BOOST_CHECK_EQUAL( cubemap.getFaceMask(),
                       scene::Cubemap::FacePositiveX | scene::Cubemap::FaceNegativeZ );
}
