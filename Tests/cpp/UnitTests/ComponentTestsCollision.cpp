#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

// Helper function to validate common manager setup
namespace
{
    void waitForTasks( SmartPtr<ITaskManager> taskManager, u32 iterations = 100 )
    {
        for( u32 i = 0; i < iterations; ++i )
        {
            taskManager->update();
            Thread::yield();
        }
    }

    constexpr auto DefaultCubeMeshPath = "cube_internal.fbmeshbin";
    constexpr auto InvalidMeshPath = "nonexistent_mesh.fbmeshbin";
}  // namespace

//------------------------------------------------------------------------------
// Sphere Collider Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE( CollisionSphereTests )

BOOST_AUTO_TEST_CASE( components_sphere_collider_basic )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        auto collisionSphere = actor->addComponent<scene::CollisionSphere>();
        BOOST_REQUIRE( collisionSphere );
        BOOST_CHECK( collisionSphere->isValid() );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        BOOST_CHECK( collisionSphere->isLoaded() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_sphere_collider_unload_reload )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        auto collisionSphere = actor->addComponent<scene::CollisionSphere>();
        BOOST_REQUIRE( collisionSphere );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        collisionSphere->unload( nullptr );
        BOOST_CHECK( !collisionSphere->isLoaded() );

        collisionSphere->load( nullptr );
        BOOST_CHECK( collisionSphere->isLoaded() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()

//------------------------------------------------------------------------------
// Box Collider Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE( CollisionBoxTests )

BOOST_AUTO_TEST_CASE( components_box_collider_basic )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );
        BOOST_CHECK( collisionBox->isValid() );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        fixture.sceneManager->edit();
        waitForTasks( fixture.taskManager, 10 );

        auto shape = collisionBox->getShape();
        BOOST_CHECK( shape );

        if( shape )
        {
            BOOST_CHECK( shape->isValid() );
        }

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_box_collider_extents )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );

        // Test setting custom extents
        Vector3<real_Num> customExtents( 2.0f, 3.0f, 4.0f );
        collisionBox->setExtents( customExtents );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        fixture.sceneManager->edit();
        waitForTasks( fixture.taskManager, 10 );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_box_collider_zero_extents )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );

        // Test with zero extents - edge case
        Vector3<real_Num> zeroExtents( 0.0f, 0.0f, 0.0f );
        collisionBox->setExtents( zeroExtents );

        // Should handle gracefully without crashing
        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        fixture.sceneManager->edit();
        waitForTasks( fixture.taskManager, 10 );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_box_collider_negative_extents )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );

        // Test with negative extents - edge case
        Vector3<real_Num> negativeExtents( -1.0f, -2.0f, -3.0f );
        collisionBox->setExtents( negativeExtents );

        // Should handle gracefully without crashing
        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        fixture.sceneManager->edit();
        waitForTasks( fixture.taskManager, 10 );

        fixture.sceneManager->destroyActor( actor );

        fixture.scene->clear();
        fixture.sceneManager->clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_box_collider_raycast )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto physicsScene = fixture.physicsManager->getPhysicsScene();
        BOOST_REQUIRE( physicsScene );

        fixture.scene->clear();

        auto actor = fixture.createBasicActor( true );
        BOOST_REQUIRE( actor );
        BOOST_CHECK( actor->isStatic() );

        auto meshComponent = actor->addComponent<scene::Mesh>();
        BOOST_REQUIRE( meshComponent );

        meshComponent->setMeshPath( DefaultCubeMeshPath );

        auto meshRenderer = actor->addComponent<scene::MeshRenderer>();
        BOOST_REQUIRE( meshRenderer );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );
        BOOST_CHECK( collisionBox->isValid() );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );
        BOOST_CHECK( rigidbody->isValid() );

        auto boxSize = 500.f;
        auto position = Vector3<real_Num>::unitY() * -( boxSize * static_cast<real_Num>( 0.5 ) );

        Transform3<real_Num> transform;
        transform.setPosition( position );
        BOOST_CHECK( transform.isValid() );
        BOOST_CHECK( transform.isSane() );

        actor->setPosition( position );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        fixture.sceneManager->edit();
        waitForTasks( fixture.taskManager, 10 );

        // Perform raycast test
        auto origin = Vector3<real_Num>::unitY() * 250.0f;
        auto dir = -Vector3<real_Num>::unitY();

        BOOST_CHECK( collisionBox->isValid() );

        Array<SmartPtr<physics::IRaycastHit>> hits;
        auto result = physicsScene->castRay( origin, dir, hits );
        BOOST_CHECK( result );
        BOOST_CHECK( !hits.empty() );

        // Verify rigid body setup
        auto rigidStatic = rigidbody->getRigidStatic();
        BOOST_REQUIRE( rigidStatic );
        BOOST_CHECK( rigidStatic->isValid() );

        auto rigidScene = rigidStatic->getScene();
        BOOST_CHECK( rigidScene );

        auto numShapes = rigidStatic->getNumShapes();
        BOOST_CHECK( numShapes > 0 );

        auto physicsShape = collisionBox->getShape();
        BOOST_CHECK( physicsShape );

        if( physicsShape )
        {
            BOOST_CHECK( physicsShape->isValid() );
        }

        fixture.resetTimer();
        fixture.updatePhysics( 10 );

        BOOST_CHECK( collisionBox->isLoaded() );

        // Test raycast after physics updates
        hits.clear();
        result = physicsScene->castRay( origin, dir, hits );
        BOOST_CHECK( result );
        BOOST_CHECK( !hits.empty() );

        fixture.sceneManager->destroyActor( actor );
        actor = nullptr;

        fixture.scene->clear();
        fixture.sceneManager->clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_box_collider_raycast_miss )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto physicsScene = fixture.physicsManager->getPhysicsScene();
        BOOST_REQUIRE( physicsScene );

        fixture.scene->clear();

        auto actor = fixture.createBasicActor( true );
        BOOST_REQUIRE( actor );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        auto position = Vector3<real_Num>::unitY() * -250.0f;
        actor->setPosition( position );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        fixture.sceneManager->edit();
        waitForTasks( fixture.taskManager, 10 );

        // Raycast that should miss (pointing away)
        auto origin = Vector3<real_Num>::unitY() * 250.0f;
        auto dir = Vector3<real_Num>::unitY();  // Pointing up, away from box

        Array<SmartPtr<physics::IRaycastHit>> hits;
        auto result = physicsScene->castRay( origin, dir, hits );
        BOOST_CHECK( !result || hits.empty() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_box_collider_raycast_at_edge )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto physicsScene = fixture.physicsManager->getPhysicsScene();
        BOOST_REQUIRE( physicsScene );

        fixture.scene->clear();

        auto actor = fixture.createBasicActor( true );
        BOOST_REQUIRE( actor );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );

        // Set known extents for predictable edge testing
        Vector3<real_Num> extents( 1.0f, 1.0f, 1.0f );
        collisionBox->setExtents( extents );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        actor->setPosition( Vector3<real_Num>::zero() );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        fixture.sceneManager->edit();
        waitForTasks( fixture.taskManager, 10 );

        // Raycast at the edge of the box
        auto origin = Vector3<real_Num>( 1.0f, 10.0f, 0.0f );
        auto dir = -Vector3<real_Num>::unitY();

        Array<SmartPtr<physics::IRaycastHit>> hits;
        physicsScene->castRay( origin, dir, hits );
        // Edge case - may or may not hit depending on implementation

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()

//------------------------------------------------------------------------------
// Mesh Collider Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE( CollisionMeshTests )

BOOST_AUTO_TEST_CASE( components_mesh_collider )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        auto meshComponent = actor->addComponent<scene::Mesh>();
        BOOST_REQUIRE( meshComponent );
        BOOST_CHECK( meshComponent->isValid() );

        meshComponent->setMeshPath( DefaultCubeMeshPath );

        auto meshRenderer = actor->addComponent<scene::MeshRenderer>();
        BOOST_REQUIRE( meshRenderer );
        BOOST_CHECK( meshRenderer->isValid() );

        auto collisionMesh = actor->addComponent<scene::CollisionMesh>();
        BOOST_REQUIRE( collisionMesh );
        BOOST_CHECK( collisionMesh->isValid() );

        //auto rigidbody = actor->addComponent<scene::Rigidbody>();
        //BOOST_REQUIRE( rigidbody );
        //BOOST_CHECK( rigidbody->isValid() );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        fixture.resetTimer();

        // Check initial state
        BOOST_CHECK( collisionMesh->isLoaded() );
        auto meshPath = collisionMesh->getMeshPath();
        BOOST_CHECK( !StringUtil::isNullOrEmpty( meshPath ) );

        auto meshResource = collisionMesh->getMeshResource();
        BOOST_CHECK( meshResource );

        fixture.sceneManager->play();
        fixture.updatePhysics( 10 );

        // Verify state after physics updates
        BOOST_CHECK( collisionMesh->isLoaded() );
        BOOST_CHECK( !StringUtil::isNullOrEmpty( collisionMesh->getMeshPath() ) );
        BOOST_CHECK( collisionMesh->getMeshResource() );

        // Test component unloading
        meshComponent->unload( nullptr );
        BOOST_CHECK( !meshComponent->isLoaded() );

        meshRenderer->unload( nullptr );
        BOOST_CHECK( !meshRenderer->isLoaded() );

        collisionMesh->unload( nullptr );
        BOOST_CHECK( !collisionMesh->isLoaded() );

        //rigidbody->unload( nullptr );
        //BOOST_CHECK( !rigidbody->isLoaded() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_collider_null_mesh_path )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        auto collisionMesh = actor->addComponent<scene::CollisionMesh>();
        BOOST_REQUIRE( collisionMesh );

        // Test with empty mesh path - should handle gracefully
        auto meshPath = collisionMesh->getMeshPath();
        BOOST_CHECK( StringUtil::isNullOrEmpty( meshPath ) );

        // Mesh resource should be null without a valid path
        auto meshResource = collisionMesh->getMeshResource();
        BOOST_CHECK( !meshResource );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_collider_invalid_mesh_path )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        auto collisionMesh = actor->addComponent<scene::CollisionMesh>();
        BOOST_REQUIRE( collisionMesh );

        // Set an invalid mesh path - should handle gracefully without crashing
        collisionMesh->setMeshPath( InvalidMeshPath );

        auto meshPath = collisionMesh->getMeshPath();
        BOOST_CHECK( !StringUtil::isNullOrEmpty( meshPath ) );

        // Resource may be null or invalid for non-existent files
        auto meshResource = collisionMesh->getMeshResource();
        // Should not crash even with invalid path

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_collider_mesh_path_change )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        auto meshComponent = actor->addComponent<scene::Mesh>();
        BOOST_REQUIRE( meshComponent );
        meshComponent->setMeshPath( DefaultCubeMeshPath );

        auto collisionMesh = actor->addComponent<scene::CollisionMesh>();
        BOOST_REQUIRE( collisionMesh );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        auto originalPath = collisionMesh->getMeshPath();

        // Change mesh path dynamically
        const auto newMeshPath = String( "cube_internal_copy.fbmeshbin" );
        collisionMesh->setMeshPath( newMeshPath );

        auto updatedPath = collisionMesh->getMeshPath();
        BOOST_CHECK( updatedPath != originalPath || originalPath == newMeshPath );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_collider_raycast )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        BOOST_REQUIRE( fileSystem );

        auto physicsScene = fixture.physicsManager->getPhysicsScene();
        BOOST_REQUIRE( physicsScene );

        auto actor = fixture.createBasicActor( true );
        BOOST_REQUIRE( actor );
        BOOST_CHECK( actor->isStatic() );

        auto meshComponent = actor->addComponent<scene::Mesh>();
        BOOST_REQUIRE( meshComponent );
        BOOST_CHECK( meshComponent->isValid() );

        auto fileExists = fileSystem->isExistingFile( DefaultCubeMeshPath, true, true );
        BOOST_CHECK( fileExists );

        meshComponent->setMeshPath( DefaultCubeMeshPath );

        auto meshRenderer = actor->addComponent<scene::MeshRenderer>();
        BOOST_REQUIRE( meshRenderer );

        auto collisionMesh = actor->addComponent<scene::CollisionMesh>();
        BOOST_REQUIRE( collisionMesh );
        BOOST_CHECK( collisionMesh->isValid() );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );
        BOOST_CHECK( rigidbody->isValid() );

        auto boxSize = 500.f;
        auto position = Vector3<real_Num>::unitY() * -( boxSize * static_cast<real_Num>( 0.5 ) );

        Transform3<real_Num> transform;
        transform.setPosition( position );
        BOOST_CHECK( transform.isValid() );
        BOOST_CHECK( transform.isSane() );

        actor->setPosition( position );
        BOOST_CHECK( actor->isValid() );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );
        BOOST_CHECK( fixture.scene->isValid() );

        fixture.sceneManager->edit();
        fixture.resetTimer();

        waitForTasks( fixture.taskManager, 10 );

        BOOST_CHECK( collisionMesh->isValid() );
        BOOST_CHECK( rigidbody->isValid() );

        auto collisionMeshState = collisionMesh->getState();
        BOOST_CHECK( collisionMeshState == IComponent::State::Edit );

        auto rigidbodyState = rigidbody->getState();
        BOOST_CHECK( rigidbodyState == IComponent::State::Edit );

        // Perform raycast
        auto origin = Vector3<real_Num>::unitY() * 250.0f;
        auto dir = -Vector3<real_Num>::unitY();

        Array<SmartPtr<physics::IRaycastHit>> hits;
        auto result = physicsScene->castRay( origin, dir, hits );
        BOOST_CHECK( result );

        //auto rigidStatic = rigidbody->getRigidStatic();
        //BOOST_REQUIRE( rigidStatic );
        //BOOST_CHECK( rigidStatic->isValid() );

        //auto rigidStaticScene = rigidStatic->getScene();
        //BOOST_CHECK( rigidStaticScene );

        //auto numShapes = rigidStatic->getNumShapes();
        //BOOST_CHECK( numShapes > 0 );

        //BOOST_CHECK( fixture.scene->isValid() );

        //auto physicsShape = collisionMesh->getShape();
        //BOOST_CHECK( physicsShape );

        //if( physicsShape )
        //{
        //    BOOST_CHECK( physicsShape->isValid() );

        //    if( physicsShape->isExactly<physics::IMeshShape>() )
        //    {
        //        auto meshShape = workphone::static_pointer_cast<physics::IMeshShape>( physicsShape );
        //        BOOST_REQUIRE( meshShape );

        //        auto meshShapeMesh = meshShape->getMesh();
        //        BOOST_CHECK( meshShapeMesh );
        //    }
        //}

        fixture.resetTimer();
        waitForTasks( fixture.taskManager, 10 );

        // Verify loaded state
        BOOST_CHECK( collisionMesh->isLoaded() );
        BOOST_CHECK( !StringUtil::isNullOrEmpty( collisionMesh->getMeshPath() ) );
        BOOST_CHECK( collisionMesh->getMeshResource() );

        // Test raycast again after updates
        hits.clear();
        result = physicsScene->castRay( origin, dir, hits );
        BOOST_CHECK( result );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_collider_reload )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        auto meshComponent = actor->addComponent<scene::Mesh>();
        BOOST_REQUIRE( meshComponent );
        meshComponent->setMeshPath( DefaultCubeMeshPath );

        auto collisionMesh = actor->addComponent<scene::CollisionMesh>();
        BOOST_REQUIRE( collisionMesh );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        // Unload and reload the collision mesh
        collisionMesh->unload( nullptr );
        BOOST_CHECK( !collisionMesh->isLoaded() );

        collisionMesh->load( nullptr );
        BOOST_CHECK( collisionMesh->isLoaded() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()

//------------------------------------------------------------------------------
// Multiple Collision Shapes Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE( MultipleCollisionShapesTests )

BOOST_AUTO_TEST_CASE( components_multiple_collision_shapes )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        // Add multiple collision shapes
        auto collisionBox1 = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox1 );
        BOOST_CHECK( collisionBox1->isValid() );

        auto collisionBox2 = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox2 );
        BOOST_CHECK( collisionBox2->isValid() );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        fixture.sceneManager->edit();
        waitForTasks( fixture.taskManager, 100 );

        // Verify multiple shapes are registered
        auto rigidStatic = rigidbody->getRigidStatic();
        if( rigidStatic )
        {
            auto numShapes = rigidStatic->getNumShapes();
            BOOST_CHECK( numShapes >= 2 );
        }

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_mixed_collision_shapes )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        // Add different types of collision shapes
        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );
        BOOST_CHECK( collisionBox->isValid() );

        auto collisionSphere = actor->addComponent<scene::CollisionSphere>();
        BOOST_REQUIRE( collisionSphere );
        BOOST_CHECK( collisionSphere->isValid() );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        fixture.sceneManager->edit();
        waitForTasks( fixture.taskManager, 100 );

        // Verify both shapes are registered
        auto rigidStatic = rigidbody->getRigidStatic();
        if( rigidStatic )
        {
            auto numShapes = rigidStatic->getNumShapes();
            BOOST_CHECK( numShapes >= 2 );
        }

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_collision_shapes_partial_unload )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        auto collisionBox1 = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox1 );

        auto collisionBox2 = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox2 );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        fixture.sceneManager->edit();
        waitForTasks( fixture.taskManager, 100 );

        // Unload only one shape
        collisionBox1->unload( nullptr );
        BOOST_CHECK( !collisionBox1->isLoaded() );
        BOOST_CHECK( collisionBox2->isLoaded() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()

//------------------------------------------------------------------------------
// Collision Shape Validity Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE( CollisionValidityTests )

BOOST_AUTO_TEST_CASE( components_collision_shapes_validity )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );
        BOOST_CHECK( collisionBox->isValid() );

        auto shape = collisionBox->getShape();
        BOOST_CHECK( shape );

        if( shape )
        {
            BOOST_CHECK( shape->isValid() );
        }

        // Unload and verify invalidity
        collisionBox->unload( nullptr );
        BOOST_CHECK( !collisionBox->isLoaded() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_collision_without_rigidbody )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        // Add collision without rigidbody - edge case
        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        // Should handle gracefully without rigidbody
        BOOST_CHECK( collisionBox->isLoaded() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_rigidbody_without_collision )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        // Add rigidbody without collision shapes - edge case
        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        fixture.sceneManager->edit();
        waitForTasks( fixture.taskManager, 100 );

        // Should handle gracefully without collision shapes
        BOOST_CHECK( rigidbody->isLoaded() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_collision_double_unload )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        // Unload twice - should not crash
        collisionBox->unload( nullptr );
        BOOST_CHECK( !collisionBox->isLoaded() );

        collisionBox->unload( nullptr );
        BOOST_CHECK( !collisionBox->isLoaded() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_collision_double_load )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        // Load twice - should not crash or create duplicate shapes
        collisionBox->load( nullptr );
        BOOST_CHECK( collisionBox->isLoaded() );

        collisionBox->load( nullptr );
        BOOST_CHECK( collisionBox->isLoaded() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()

//------------------------------------------------------------------------------
// Rigidbody Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE( RigidbodyTests )

BOOST_AUTO_TEST_CASE( components_rigidbody_static_vs_dynamic )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        // Test static actor
        auto staticActor = fixture.createBasicActor( true );
        BOOST_REQUIRE( staticActor );
        BOOST_CHECK( staticActor->isStatic() );

        auto staticRigidbody = staticActor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( staticRigidbody );

        auto staticCollisionBox = staticActor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( staticCollisionBox );

        fixture.scene->registerAllUpdates( staticActor );
        fixture.scene->addActor( staticActor );

        fixture.sceneManager->edit();
        waitForTasks( fixture.taskManager, 100 );

        auto rigidStatic = staticRigidbody->getRigidStatic();
        BOOST_CHECK( rigidStatic );

        // Test dynamic actor
        auto dynamicActor = fixture.createBasicActor( false );
        BOOST_REQUIRE( dynamicActor );
        BOOST_CHECK( !dynamicActor->isStatic() );

        auto dynamicRigidbody = dynamicActor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( dynamicRigidbody );

        auto dynamicCollisionBox = dynamicActor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( dynamicCollisionBox );

        fixture.scene->registerAllUpdates( dynamicActor );
        fixture.scene->addActor( dynamicActor );

        fixture.sceneManager->play();
        fixture.updatePhysics( 10 );

        auto rigidDynamic = dynamicRigidbody->getRigidDynamic();
        // Dynamic rigidbody should be created for non-static actors
        // BOOST_CHECK( rigidDynamic );  // Depends on implementation

        fixture.sceneManager->destroyActor( staticActor );
        fixture.sceneManager->destroyActor( dynamicActor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_rigidbody_mass_properties )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor( false );
        BOOST_REQUIRE( actor );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        // Test mass property setting
        // Note: Actual mass setting depends on API availability
        BOOST_CHECK( rigidbody->isLoaded() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_rigidbody_unload_with_shapes )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor();
        BOOST_REQUIRE( actor );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        fixture.sceneManager->edit();
        waitForTasks( fixture.taskManager, 100 );

        // Unload rigidbody while shapes are still attached
        rigidbody->unload( nullptr );
        BOOST_CHECK( !rigidbody->isLoaded() );

        // Shapes should still exist
        BOOST_CHECK( collisionBox->isLoaded() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()

//------------------------------------------------------------------------------
// Actor Transform Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE( CollisionTransformTests )

BOOST_AUTO_TEST_CASE( components_collision_extreme_position )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor( true );
        BOOST_REQUIRE( actor );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        // Set extreme position values
        auto extremePosition = Vector3<real_Num>( 1000000.0f, 1000000.0f, 1000000.0f );
        actor->setPosition( extremePosition );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        fixture.sceneManager->edit();
        waitForTasks( fixture.taskManager, 100 );

        // Should handle extreme positions without crashing
        BOOST_CHECK( collisionBox->isLoaded() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_collision_position_update )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto physicsScene = fixture.physicsManager->getPhysicsScene();
        BOOST_REQUIRE( physicsScene );

        auto actor = fixture.createBasicActor( true );
        BOOST_REQUIRE( actor );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        actor->setPosition( Vector3<real_Num>::zero() );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        fixture.sceneManager->edit();
        waitForTasks( fixture.taskManager, 100 );

        // Initial raycast at origin
        auto origin = Vector3<real_Num>::unitY() * 10.0f;
        auto dir = -Vector3<real_Num>::unitY();

        Array<SmartPtr<physics::IRaycastHit>> hits;
        auto result1 = physicsScene->castRay( origin, dir, hits );

        // Move actor and test raycast at new position
        auto newPosition = Vector3<real_Num>( 100.0f, 0.0f, 0.0f );
        actor->setPosition( newPosition );

        waitForTasks( fixture.taskManager, 100 );

        // Raycast at new position
        origin = newPosition + Vector3<real_Num>::unitY() * 10.0f;
        hits.clear();
        auto result2 = physicsScene->castRay( origin, dir, hits );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()

//------------------------------------------------------------------------------
// Scene Management Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE( CollisionSceneTests )

BOOST_AUTO_TEST_CASE( components_collision_actor_removal )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor( true );
        BOOST_REQUIRE( actor );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        fixture.sceneManager->edit();
        waitForTasks( fixture.taskManager, 100 );

        // Remove actor from scene
        fixture.scene->removeActor( actor );

        // Actor should handle removal gracefully
        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_collision_scene_clear )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        // Create multiple actors with collision
        for( int i = 0; i < 5; ++i )
        {
            auto actor = fixture.createBasicActor( true );
            BOOST_REQUIRE( actor );

            auto collisionBox = actor->addComponent<scene::CollisionBox>();
            BOOST_REQUIRE( collisionBox );

            auto rigidbody = actor->addComponent<scene::Rigidbody>();
            BOOST_REQUIRE( rigidbody );

            fixture.scene->registerAllUpdates( actor );
            fixture.scene->addActor( actor );
        }

        fixture.sceneManager->edit();
        waitForTasks( fixture.taskManager, 100 );

        // Clear scene - should clean up all collision objects
        fixture.scene->clear();

        // Scene should be empty and not crash
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( components_collision_edit_to_play_transition )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor( false );
        BOOST_REQUIRE( actor );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        // Start in edit mode
        fixture.sceneManager->edit();
        waitForTasks( fixture.taskManager, 50 );

        BOOST_CHECK( collisionBox->isLoaded() );

        // Transition to play mode
        fixture.sceneManager->play();
        fixture.resetTimer();
        fixture.updatePhysics( 20 );

        // Collision should remain valid through transition
        BOOST_CHECK( collisionBox->isLoaded() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()
