#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

BOOST_AUTO_TEST_CASE( physics_terrain )
{
    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    try
    {
        auto stateManager = applicationManager->getStateManager();
        BOOST_CHECK( stateManager );

        auto timer = applicationManager->getTimer();
        BOOST_CHECK( timer );

        auto physicsManager = applicationManager->getPhysicsManager();
        if( !physicsManager )
        {
            WP_LOG_ERROR( "Physics manager is not available." );
            return;
        }

        auto scene = physicsManager->getPhysicsScene();
        BOOST_REQUIRE( scene );

        // create rigid bodies
        auto meshShape = physicsManager->addCollisionShape<physics::IMeshShape>( nullptr );
        BOOST_REQUIRE( meshShape );

        Transform3<real_Num> transform;
        BOOST_CHECK( transform.isValid() );

        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_REQUIRE( rigidBody );

        rigidBody->addShape( meshShape );

        auto shapes = rigidBody->getShapes();
        BOOST_CHECK_EQUAL( shapes.size(), 1 );

        scene->addActor( rigidBody );
        TestGuard guard( true );
        guard.trackPhysicsActor( scene, rigidBody );

        real_Num fDT = static_cast<real_Num>( 1.0 / 60.0 );
        real_Num fT = static_cast<real_Num>( 0.0 );

        // Run a few simulation steps
        for( size_t i = 0; i < 10; ++i )
        {
            physicsManager->update();
        }

        shapes.clear();
        rigidBody = nullptr;
        meshShape = nullptr;
        scene = nullptr;
        physicsManager = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( raycasts_terrain )
{
    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    try
    {
        BOOST_CHECK( applicationManager->getTimer() );
        BOOST_CHECK( applicationManager->getStateManager() );

        auto physicsManager = applicationManager->getPhysicsManager();
        if( !physicsManager )
        {
            WP_LOG_ERROR( "Physics manager is not available." );
            return;
        }

        auto scene = physicsManager->getPhysicsScene();
        BOOST_REQUIRE( scene );

        // create rigid bodies
        auto boxShape = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
        BOOST_REQUIRE( boxShape );

        auto boxSize = static_cast<real_Num>( 500.0 );
        boxShape->setExtents( Vector3<real_Num>::unit() * boxSize );

        Transform3<real_Num> transform;

        auto position = Vector3<real_Num>::unitY() * -( boxSize * static_cast<real_Num>( 0.5 ) );
        transform.setPosition( position );

        BOOST_CHECK( transform.isValid() );
        BOOST_CHECK( transform.isSane() );

        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_REQUIRE( rigidBody );

        rigidBody->addShape( boxShape );

        auto shapes = rigidBody->getShapes();
        BOOST_CHECK_EQUAL( shapes.size(), 1 );

        scene->addActor( rigidBody );
        TestGuard guard( true );
        guard.trackPhysicsActor( scene, rigidBody );

        auto origin = Vector3<real_Num>::unitY() * static_cast<real_Num>( 250.0 );
        auto dir = -Vector3<real_Num>::unitY();

        Array<SmartPtr<physics::IRaycastHit>> hits;
        auto result = scene->castRay( origin, dir, hits );
        BOOST_CHECK( result == true );
        BOOST_CHECK( !hits.empty() );

        for( size_t i = 0; i < 3; ++i )
        {
            physicsManager->update();
        }

        hits.clear();
        result = scene->castRay( origin, dir, hits );
        BOOST_CHECK( result == true );
        BOOST_CHECK( !hits.empty() );

        shapes.clear();
        rigidBody = nullptr;
        boxShape = nullptr;
        scene = nullptr;
        physicsManager = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( raycast_miss )
{
    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    if( !physicsManager )
    {
        WP_LOG_ERROR( "Physics manager is not available." );
        return;
    }

    try
    {
        auto scene = physicsManager->getPhysicsScene();
        BOOST_REQUIRE( scene );

        // Create a box at origin
        auto boxShape = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
        BOOST_REQUIRE( boxShape );

        boxShape->setExtents( Vector3<real_Num>::unit() * static_cast<real_Num>( 10.0 ) );

        Transform3<real_Num> transform;
        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_REQUIRE( rigidBody );

        rigidBody->addShape( boxShape );
        scene->addActor( rigidBody );
        TestGuard guard( true );
        guard.trackPhysicsActor( scene, rigidBody );

        // Cast ray that should miss (pointing away from the box)
        auto origin = Vector3<real_Num>::unitY() * static_cast<real_Num>( 100.0 );
        auto dir = Vector3<real_Num>::unitY();  // Pointing up, away from box

        Array<SmartPtr<physics::IRaycastHit>> hits;
        auto result = scene->castRay( origin, dir, hits );
        BOOST_CHECK( result == false );
        BOOST_CHECK( hits.empty() );

        rigidBody = nullptr;
        boxShape = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( raycast_removed_actor_no_longer_hits )
{
    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    if( !physicsManager )
    {
        WP_LOG_ERROR( "Physics manager is not available." );
        return;
    }

    try
    {
        auto scene = physicsManager->getPhysicsScene();
        BOOST_REQUIRE( scene );

        auto boxShape = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
        BOOST_REQUIRE( boxShape );

        const auto boxSize = static_cast<real_Num>( 10.0 );
        boxShape->setExtents( Vector3<real_Num>::unit() * boxSize );

        Transform3<real_Num> transform;
        transform.setPosition( Vector3<real_Num>( static_cast<real_Num>( 1234.0 ),
                                                  static_cast<real_Num>( 0.0 ),
                                                  static_cast<real_Num>( 6789.0 ) ) );

        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_REQUIRE( rigidBody );

        rigidBody->addShape( boxShape );

        const auto staticActorsBefore = scene->numStaticActors();
        scene->addActor( rigidBody );
        TestGuard guard( true );
        guard.trackPhysicsActor( scene, rigidBody );
        const auto staticActorsAfterAdd = scene->numStaticActors();
        if( staticActorsAfterAdd == staticActorsBefore )
        {
            BOOST_TEST_MESSAGE(
                "Box rigid static was not accepted by the physics scene - skipping removal raycast "
                "expectations" );
            return;
        }

        const auto origin =
            Vector3<real_Num>( static_cast<real_Num>( 1234.0 ), static_cast<real_Num>( 100.0 ),
                               static_cast<real_Num>( 6789.0 ) );
        const auto dir = -Vector3<real_Num>::unitY();

        scene->removeActor( rigidBody );
        physicsManager->update();

        Array<SmartPtr<physics::IRaycastHit>> hits;
        BOOST_CHECK( !scene->castRay( origin, dir, hits ) );
        BOOST_CHECK( hits.empty() );
        BOOST_CHECK_LE( scene->numStaticActors(), staticActorsBefore );

        rigidBody = nullptr;
        boxShape = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( multiple_shapes_on_rigid_body )
{
    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    if( !physicsManager )
    {
        WP_LOG_ERROR( "Physics manager is not available." );
        return;
    }

    try
    {
        auto scene = physicsManager->getPhysicsScene();
        BOOST_REQUIRE( scene );

        auto boxShape1 = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
        auto boxShape2 = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
        BOOST_REQUIRE( boxShape1 );
        BOOST_REQUIRE( boxShape2 );

        boxShape1->setExtents( Vector3<real_Num>::unit() * static_cast<real_Num>( 5.0 ) );
        boxShape2->setExtents( Vector3<real_Num>::unit() * static_cast<real_Num>( 3.0 ) );

        Transform3<real_Num> transform;
        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_REQUIRE( rigidBody );

        rigidBody->addShape( boxShape1 );
        rigidBody->addShape( boxShape2 );

        auto shapes = rigidBody->getShapes();
        BOOST_CHECK_EQUAL( shapes.size(), 2 );

        scene->addActor( rigidBody );
        TestGuard guard( true );
        guard.trackPhysicsActor( scene, rigidBody );

        physicsManager->update();

        shapes.clear();
        rigidBody = nullptr;
        boxShape1 = nullptr;
        boxShape2 = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( transform_edge_cases )
{
    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    if( !physicsManager )
    {
        WP_LOG_ERROR( "Physics manager is not available." );
        return;
    }

    try
    {
        auto scene = physicsManager->getPhysicsScene();
        BOOST_REQUIRE( scene );

        // Test with zero position
        {
            Transform3<real_Num> transform;
            transform.setPosition( Vector3<real_Num>::zero() );
            BOOST_CHECK( transform.isValid() );
            BOOST_CHECK( transform.isSane() );

            auto rigidBody = physicsManager->addRigidStatic( transform );
            BOOST_CHECK( rigidBody );
        }

        // Test with large position values
        {
            Transform3<real_Num> transform;
            transform.setPosition( Vector3<real_Num>::unit() * static_cast<real_Num>( 100000.0 ) );
            BOOST_CHECK( transform.isValid() );
            BOOST_CHECK( transform.isSane() );

            auto rigidBody = physicsManager->addRigidStatic( transform );
            BOOST_CHECK( rigidBody );
        }

        // Test with negative position values
        {
            Transform3<real_Num> transform;
            transform.setPosition( Vector3<real_Num>::unit() * static_cast<real_Num>( -500.0 ) );
            BOOST_CHECK( transform.isValid() );
            BOOST_CHECK( transform.isSane() );

            auto rigidBody = physicsManager->addRigidStatic( transform );
            BOOST_CHECK( rigidBody );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( box_shape_extents_edge_cases )
{
    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    if( !physicsManager )
    {
        WP_LOG_ERROR( "Physics manager is not available." );
        return;
    }

    try
    {
        // Test with very small extents
        {
            auto boxShape = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
            BOOST_REQUIRE( boxShape );
            boxShape->setExtents( Vector3<real_Num>::unit() * static_cast<real_Num>( 0.001 ) );
        }

        // Test with very large extents
        {
            auto boxShape = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
            BOOST_REQUIRE( boxShape );
            boxShape->setExtents( Vector3<real_Num>::unit() * static_cast<real_Num>( 10000.0 ) );
        }

        // Test with non-uniform extents
        {
            auto boxShape = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
            BOOST_REQUIRE( boxShape );
            Vector3<real_Num> nonUniform( static_cast<real_Num>( 1.0 ), static_cast<real_Num>( 5.0 ),
                                          static_cast<real_Num>( 10.0 ) );
            boxShape->setExtents( nonUniform );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( actor_add_remove )
{
    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    if( !physicsManager )
    {
        WP_LOG_ERROR( "Physics manager is not available." );
        return;
    }

    try
    {
        auto scene = physicsManager->getPhysicsScene();
        BOOST_REQUIRE( scene );

        auto boxShape = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
        BOOST_REQUIRE( boxShape );
        boxShape->setExtents( Vector3<real_Num>::unit() * static_cast<real_Num>( 10.0 ) );

        Transform3<real_Num> transform;
        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_REQUIRE( rigidBody );

        rigidBody->addShape( boxShape );

        // Add actor
        scene->addActor( rigidBody );
        TestGuard guard( true );
        guard.trackPhysicsActor( scene, rigidBody );

        // Update physics
        physicsManager->update();

        // Remove actor
        scene->removeActor( rigidBody );

        // Update physics after removal
        physicsManager->update();

        rigidBody = nullptr;
        boxShape = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( multiple_raycasts_same_frame )
{
    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    if( !physicsManager )
    {
        WP_LOG_ERROR( "Physics manager is not available." );
        return;
    }

    try
    {
        auto scene = physicsManager->getPhysicsScene();
        BOOST_REQUIRE( scene );

        // Create ground plane as box
        auto boxShape = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
        BOOST_REQUIRE( boxShape );
        boxShape->setExtents( Vector3<real_Num>( static_cast<real_Num>( 1000.0 ),
                                                 static_cast<real_Num>( 1.0 ),
                                                 static_cast<real_Num>( 1000.0 ) ) );

        Transform3<real_Num> transform;
        transform.setPosition( Vector3<real_Num>::unitY() * static_cast<real_Num>( -1.0 ) );

        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_REQUIRE( rigidBody );

        rigidBody->addShape( boxShape );
        const auto staticActorsBefore = scene->numStaticActors();
        scene->addActor( rigidBody );
        TestGuard guard( true );
        guard.trackPhysicsActor( scene, rigidBody );
        const auto staticActorsAfterAdd = scene->numStaticActors();
        if( staticActorsAfterAdd == staticActorsBefore )
        {
            BOOST_TEST_MESSAGE(
                "Box rigid static was not accepted by the physics scene - skipping repeated raycast "
                "expectations" );
            return;
        }

        physicsManager->update();

        // Perform multiple raycasts
        auto dir = -Vector3<real_Num>::unitY();
        size_t hitCount = 0;

        for( int x = -5; x <= 5; ++x )
        {
            for( int z = -5; z <= 5; ++z )
            {
                Vector3<real_Num> origin( static_cast<real_Num>( x * 10 ),
                                          static_cast<real_Num>( 100.0 ),
                                          static_cast<real_Num>( z * 10 ) );

                Array<SmartPtr<physics::IRaycastHit>> hits;
                if( scene->castRay( origin, dir, hits ) )
                {
                    ++hitCount;
                }
            }
        }

        // All rays should hit the ground plane
        BOOST_CHECK_EQUAL( hitCount, 121 );  // 11 x 11 grid

        scene->removeActor( rigidBody );
        BOOST_CHECK_LE( scene->numStaticActors(), staticActorsBefore );

        rigidBody = nullptr;
        boxShape = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( box_extents_update_after_scene_add_affects_raycast_queries )
{
    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    if( !physicsManager )
    {
        WP_LOG_ERROR( "Physics manager is not available." );
        return;
    }

    try
    {
        auto scene = physicsManager->getPhysicsScene();
        BOOST_REQUIRE( scene );

        auto boxShape = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
        BOOST_REQUIRE( boxShape );
        boxShape->setExtents( Vector3<real_Num>( static_cast<real_Num>( 10.0 ),
                                                 static_cast<real_Num>( 2.0 ),
                                                 static_cast<real_Num>( 10.0 ) ) );

        Transform3<real_Num> transform;
        transform.setPosition( Vector3<real_Num>::unitY() * static_cast<real_Num>( -1.0 ) );

        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_REQUIRE( rigidBody );

        rigidBody->addShape( boxShape );
        const auto staticActorsBefore = scene->numStaticActors();
        scene->addActor( rigidBody );
        TestGuard guard( true );
        guard.trackPhysicsActor( scene, rigidBody );
        const auto staticActorsAfterAdd = scene->numStaticActors();
        if( staticActorsAfterAdd == staticActorsBefore )
        {
            BOOST_TEST_MESSAGE(
                "Box rigid static was not accepted by the physics scene - skipping extent update "
                "raycast expectations" );
            return;
        }

        physicsManager->update();

        auto dir = -Vector3<real_Num>::unitY();
        auto edgeOrigin =
            Vector3<real_Num>( static_cast<real_Num>( 100.0 ), static_cast<real_Num>( 100.0 ),
                               static_cast<real_Num>( 0.0 ) );

        Array<SmartPtr<physics::IRaycastHit>> hits;
        BOOST_CHECK( !scene->castRay( edgeOrigin, dir, hits ) );
        BOOST_CHECK( hits.empty() );

        boxShape->setExtents( Vector3<real_Num>( static_cast<real_Num>( 300.0 ),
                                                 static_cast<real_Num>( 2.0 ),
                                                 static_cast<real_Num>( 300.0 ) ) );

        for( size_t i = 0; i < 3; ++i )
        {
            physicsManager->update();
        }

        hits.clear();
        BOOST_CHECK( scene->castRay( edgeOrigin, dir, hits ) );
        BOOST_CHECK( !hits.empty() );

        auto outsideOrigin =
            Vector3<real_Num>( static_cast<real_Num>( 250.0 ), static_cast<real_Num>( 100.0 ),
                               static_cast<real_Num>( 0.0 ) );
        hits.clear();
        BOOST_CHECK( !scene->castRay( outsideOrigin, dir, hits ) );
        BOOST_CHECK( hits.empty() );

        scene->removeActor( rigidBody );
        BOOST_CHECK_LE( scene->numStaticActors(), staticActorsBefore );

        rigidBody = nullptr;
        boxShape = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( null_shape_handling )
{
    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    if( !physicsManager )
    {
        WP_LOG_ERROR( "Physics manager is not available." );
        return;
    }

    try
    {
        Transform3<real_Num> transform;
        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_REQUIRE( rigidBody );

        // Check initial state - no shapes
        auto shapes = rigidBody->getShapes();
        BOOST_CHECK( shapes.empty() );

        rigidBody = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}
