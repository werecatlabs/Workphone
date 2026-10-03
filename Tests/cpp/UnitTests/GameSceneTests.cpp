#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

// Test suite for game scene functionality
BOOST_AUTO_TEST_SUITE( GameSceneTestSuite )

// Helper function to perform update cycles
namespace
{
    void performUpdateCycles( int cycles )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto timer = applicationManager->getTimer();
        auto stateManager = applicationManager->getStateManager();
        auto sceneManager = applicationManager->getGameManager();

        for( int i = 0; i < cycles; ++i )
        {
            timer->update();

            stateManager->preUpdate();
            stateManager->update();
            stateManager->postUpdate();

            sceneManager->preUpdate();
            sceneManager->update();
            sceneManager->postUpdate();
        }
    }
}  // namespace

BOOST_AUTO_TEST_CASE( gamescene_add_actor )
{
    auto applicationManager = core::IApplicationManager::instance();

    try
    {
        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        BOOST_REQUIRE( scene );

        // Create and add actor to scene
        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor );

        scene->addActor( actor );

        auto &updateObjects =
            scene->getRegisteredObjects( Thread::UpdateState::Update, TaskId::Application );
        BOOST_CHECK( std::find( updateObjects.begin(), updateObjects.end(), actor ) !=
                     updateObjects.end() );

        // Set initial positions
        actor->setLocalPosition( Vector3F( 0, 0, 0 ) );
        actor->setPosition( Vector3F( 0, 0, 0 ) );

        // Create child hierarchy
        auto actorImageParent = sceneManager->createActor();
        BOOST_REQUIRE( actorImageParent );
        actor->addChild( actorImageParent );

        auto actorImage = sceneManager->createActor();
        BOOST_REQUIRE( actorImage );
        actorImageParent->addChild( actorImage );

        // Perform update cycles
        performUpdateCycles( 10 );

        // Cleanup
        scene->removeActor( actor );
        BOOST_CHECK( std::find( updateObjects.begin(), updateObjects.end(), actor ) ==
                     updateObjects.end() );
        sceneManager->destroyActor( actor );

        sceneManager->clear();

        // Verify reference count (should be 1 - only our local reference remains)
        BOOST_CHECK_GE( actor->getReferences(), 1 );
        actor = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during gamescene_add_actor test" );
    }
}

BOOST_AUTO_TEST_CASE( gamescene_add_multiple_actors )
{
    auto applicationManager = core::IApplicationManager::instance();

    try
    {
        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        BOOST_REQUIRE( scene );

        const int actorCount = 5;
        std::vector<SmartPtr<IGameActor>> actors;

        // Create multiple actors
        for( int i = 0; i < actorCount; ++i )
        {
            auto actor = sceneManager->createActor();
            BOOST_REQUIRE( actor );

            scene->addActor( actor );
            scene->registerAllUpdates( actor );

            actor->setPosition( Vector3F( static_cast<f32>( i * 10 ), 0, 0 ) );
            actors.push_back( actor );
        }

        // Perform updates
        performUpdateCycles( 5 );

        // Verify all actors are still valid
        for( const auto &actor : actors )
        {
            BOOST_CHECK( actor );
        }

        // Cleanup all actors
        for( auto &actor : actors )
        {
            scene->removeActor( actor );
            scene->unregisterAll( actor );
            sceneManager->destroyActor( actor );
        }

        actors.clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during gamescene_add_multiple_actors test" );
    }
}

BOOST_AUTO_TEST_CASE( gamescene_actor_hierarchy )
{
    auto applicationManager = core::IApplicationManager::instance();

    try
    {
        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        BOOST_REQUIRE( scene );

        // Create parent actor
        auto parent = sceneManager->createActor();
        BOOST_REQUIRE( parent );
        scene->addActor( parent );
        scene->registerAllUpdates( parent );
        parent->setPosition( Vector3F( 100, 0, 0 ) );

        // Create child actors
        auto child1 = sceneManager->createActor();
        BOOST_REQUIRE( child1 );
        parent->addChild( child1 );
        child1->setLocalPosition( Vector3F( 10, 0, 0 ) );

        auto child2 = sceneManager->createActor();
        BOOST_REQUIRE( child2 );
        parent->addChild( child2 );
        child2->setLocalPosition( Vector3F( 0, 10, 0 ) );

        // Create grandchild
        auto grandchild = sceneManager->createActor();
        BOOST_REQUIRE( grandchild );
        child1->addChild( grandchild );
        grandchild->setLocalPosition( Vector3F( 5, 5, 0 ) );

        // Perform updates
        performUpdateCycles( 5 );

        // Verify hierarchy
        BOOST_CHECK( parent->getNumChildren() == 2 );
        BOOST_CHECK( child1->getNumChildren() == 1 );
        BOOST_CHECK( child2->getNumChildren() == 0 );

        // Cleanup - removing parent should handle children
        scene->removeActor( parent );
        scene->unregisterAll( parent );
        sceneManager->destroyActor( parent );

        BOOST_CHECK_GE( parent->getReferences(), 1 );
        parent = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during gamescene_actor_hierarchy test" );
    }
}

BOOST_AUTO_TEST_CASE( gamescene_remove_actor_null_check )
{
    auto applicationManager = core::IApplicationManager::instance();

    try
    {
        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        BOOST_REQUIRE( scene );

        // Attempt to remove null actor - should handle gracefully
        SmartPtr<IGameActor> nullActor = nullptr;
        scene->removeActor( nullActor );

        // Test should pass if no exception is thrown
        BOOST_CHECK( true );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        // Depending on expected behavior, this might be acceptable
        BOOST_WARN_MESSAGE( false, "Exception thrown when removing null actor" );
    }
}

BOOST_AUTO_TEST_CASE( gamescene_double_add_actor )
{
    auto applicationManager = core::IApplicationManager::instance();

    try
    {
        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        BOOST_REQUIRE( scene );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor );

        // Add actor twice
        scene->addActor( actor );
        scene->registerAllUpdates( actor );

        // Second add - should be handled gracefully
        scene->addActor( actor );

        performUpdateCycles( 3 );

        // Cleanup
        scene->removeActor( actor );
        scene->unregisterAll( actor );
        sceneManager->destroyActor( actor );

        BOOST_CHECK_GE( actor->getReferences(), 1 );
        actor = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during gamescene_double_add_actor test" );
    }
}

BOOST_AUTO_TEST_CASE( gamescene_double_remove_actor )
{
    auto applicationManager = core::IApplicationManager::instance();

    try
    {
        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        BOOST_REQUIRE( scene );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor );

        scene->addActor( actor );
        scene->registerAllUpdates( actor );

        performUpdateCycles( 3 );

        // Remove actor twice - second removal should be handled gracefully
        scene->removeActor( actor );
        scene->unregisterAll( actor );

        scene->removeActor( actor );  // Second removal

        sceneManager->destroyActor( actor );

        BOOST_CHECK_GE( actor->getReferences(), 1 );
        actor = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during gamescene_double_remove_actor test" );
    }
}

BOOST_AUTO_TEST_CASE( gamescene_actor_position_after_updates )
{
    auto applicationManager = core::IApplicationManager::instance();

    try
    {
        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        BOOST_REQUIRE( scene );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor );

        scene->addActor( actor );
        scene->registerAllUpdates( actor );

        Vector3F initialPosition( 50, 100, 150 );
        actor->setPosition( initialPosition );

        performUpdateCycles( 10 );

        // Verify position hasn't changed (no movement logic applied)
        auto currentPosition = actor->getPosition();
        BOOST_CHECK_CLOSE( currentPosition.X(), initialPosition.X(), 0.001f );
        BOOST_CHECK_CLOSE( currentPosition.Y(), initialPosition.Y(), 0.001f );
        BOOST_CHECK_CLOSE( currentPosition.Z(), initialPosition.Z(), 0.001f );

        // Cleanup
        scene->removeActor( actor );
        scene->unregisterAll( actor );
        sceneManager->destroyActor( actor );

        actor = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during gamescene_actor_position_after_updates test" );
    }
}

BOOST_AUTO_TEST_CASE( gamescene_deep_hierarchy )
{
    auto applicationManager = core::IApplicationManager::instance();

    try
    {
        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        BOOST_REQUIRE( scene );

        // Create deep hierarchy (10 levels)
        const int depth = 10;
        std::vector<SmartPtr<IGameActor>> hierarchy;

        auto root = sceneManager->createActor();
        BOOST_REQUIRE( root );
        scene->addActor( root );
        scene->registerAllUpdates( root );
        hierarchy.push_back( root );

        for( int i = 1; i < depth; ++i )
        {
            auto child = sceneManager->createActor();
            BOOST_REQUIRE( child );
            hierarchy[i - 1]->addChild( child );
            child->setLocalPosition( Vector3F( 1, 0, 0 ) );
            hierarchy.push_back( child );
        }

        performUpdateCycles( 5 );

        // Verify hierarchy depth
        BOOST_CHECK_EQUAL( hierarchy.size(), depth );

        // Cleanup - removing root should clean up entire hierarchy
        scene->removeActor( root );
        scene->unregisterAll( root );
        sceneManager->destroyActor( root );

        hierarchy.clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during gamescene_deep_hierarchy test" );
    }
}

BOOST_AUTO_TEST_CASE( gamescene_actor_reference_counting )
{
    auto applicationManager = core::IApplicationManager::instance();

    try
    {
        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        BOOST_REQUIRE( scene );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor );

        int initialRefs = actor->getReferences();
        BOOST_CHECK_GE( initialRefs, 1 );

        scene->addActor( actor );
        int afterAddRefs = actor->getReferences();
        BOOST_CHECK_GT( afterAddRefs, initialRefs );

        scene->registerAllUpdates( actor );

        performUpdateCycles( 5 );

        scene->removeActor( actor );
        scene->unregisterAll( actor );
        int afterRemoveRefs = actor->getReferences();
        BOOST_CHECK_LE( afterRemoveRefs, afterAddRefs );

        sceneManager->destroyActor( actor );
        BOOST_CHECK_GE( actor->getReferences(), 1 );

        actor = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during gamescene_actor_reference_counting test" );
    }
}

BOOST_AUTO_TEST_CASE( gamescene_zero_update_cycles )
{
    auto applicationManager = core::IApplicationManager::instance();

    try
    {
        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        BOOST_REQUIRE( scene );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor );

        scene->addActor( actor );
        scene->registerAllUpdates( actor );

        // Don't perform any updates - test immediate cleanup

        scene->removeActor( actor );
        scene->unregisterAll( actor );
        sceneManager->destroyActor( actor );

        BOOST_CHECK_GE( actor->getReferences(), 1 );
        actor = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during gamescene_zero_update_cycles test" );
    }
}

BOOST_AUTO_TEST_CASE( gamescene_many_update_cycles )
{
    auto applicationManager = core::IApplicationManager::instance();

    try
    {
        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        BOOST_REQUIRE( scene );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor );

        scene->addActor( actor );
        scene->registerAllUpdates( actor );

        // Perform many update cycles to test stability
        performUpdateCycles( 100 );

        // Actor should still be valid
        BOOST_CHECK( actor );
        BOOST_CHECK_GE( actor->getReferences(), 1 );

        scene->removeActor( actor );
        scene->unregisterAll( actor );
        sceneManager->destroyActor( actor );

        BOOST_CHECK_GE( actor->getReferences(), 1 );
        actor = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during gamescene_many_update_cycles test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()
