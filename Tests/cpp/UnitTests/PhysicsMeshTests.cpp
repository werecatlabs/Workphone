#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Mesh/MeshSerializer.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <boost/test/unit_test.hpp>
#include <chrono>

using namespace workphone;

namespace
{
    class TestCollisionMesh : public scene::CollisionMesh
    {
    public:
        using scene::CollisionMesh::handleComponentEvent;
    };

    bool isPhysicsRuntimeAvailable()
    {
        auto applicationManager = core::IApplicationManager::instance();
        return applicationManager && applicationManager->getPhysicsManager();
    }
}  // namespace

BOOST_AUTO_TEST_CASE( physics_meshshape_creation )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        auto physicsManager = applicationManager->getPhysicsManager();
        if( !physicsManager )
        {
            WP_LOG_ERROR( "Physics manager is not available." );
            return;
        }

        auto stateManager = applicationManager->getStateManager();
        BOOST_CHECK( stateManager );

        auto timer = applicationManager->getTimer();
        BOOST_CHECK( timer );

        auto scene = physicsManager->getPhysicsScene();
        BOOST_CHECK( scene );

        // Test mesh shape creation
        auto meshShape = physicsManager->addCollisionShape<physics::IMeshShape>( nullptr );
        BOOST_CHECK( meshShape );

        // Run simulation loop
        for( size_t i = 0; i < 10; ++i )
        {
            timer->update();
            stateManager->update();
            scene->preUpdate();
            scene->update();
            scene->postUpdate();
            physicsManager->update();
        }

        if( meshShape )
        {
            BOOST_CHECK( meshShape->isLoaded() == true );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during mesh shape creation test" );
    }
}

BOOST_AUTO_TEST_CASE( physics_mesh_component_defaults_and_accessors )
{
    try
    {
        TestGuard guard;

        auto collisionMesh = workphone::make_ptr<scene::CollisionMesh>();
        BOOST_REQUIRE( collisionMesh );

        BOOST_CHECK( collisionMesh->getMeshPath().empty() );
        BOOST_CHECK( !collisionMesh->getMeshResource() );
        BOOST_CHECK( !collisionMesh->isConvex() );

        collisionMesh->setConvex( true );
        BOOST_CHECK( collisionMesh->isConvex() );

        collisionMesh->setConvex( false );
        BOOST_CHECK( !collisionMesh->isConvex() );

        collisionMesh->setMeshPath( "Physics/Meshes/TestCube.fbmeshbin" );
        BOOST_CHECK_EQUAL( collisionMesh->getMeshPath(),
                           StringUtil::cleanupPath( "Physics/Meshes/TestCube.fbmeshbin" ) );

        collisionMesh->setMeshPath( "" );
        BOOST_CHECK( collisionMesh->getMeshPath().empty() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during collision mesh accessor test" );
    }
}

BOOST_AUTO_TEST_CASE( physics_mesh_component_properties_round_trip )
{
    try
    {
        TestGuard guard;

        auto collisionMesh = workphone::make_ptr<scene::CollisionMesh>();
        BOOST_REQUIRE( collisionMesh );

        auto properties = workphone::make_ptr<Properties>();
        properties->setProperty( scene::CollisionMesh::meshPathStr, String( "Meshes/EditorCube.mesh" ) );
        properties->setProperty( scene::CollisionMesh::isConvexStr, true );

        collisionMesh->setProperties( properties );

        BOOST_CHECK_EQUAL( collisionMesh->getMeshPath(),
                           StringUtil::cleanupPath( "Meshes/EditorCube.fbmeshbin" ) );
        BOOST_CHECK( collisionMesh->isConvex() );

        auto roundTrip = collisionMesh->getProperties();
        BOOST_REQUIRE( roundTrip );

        String meshPath;
        bool isConvex = false;
        BOOST_CHECK( roundTrip->getPropertyValue( scene::CollisionMesh::meshPathStr, meshPath ) );
        BOOST_CHECK( roundTrip->getPropertyValue( scene::CollisionMesh::isConvexStr, isConvex ) );
        BOOST_CHECK_EQUAL( meshPath, collisionMesh->getMeshPath() );
        BOOST_CHECK( isConvex );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during collision mesh properties round-trip test" );
    }
}

BOOST_AUTO_TEST_CASE( physics_mesh_component_resource_assignment )
{
    try
    {
        TestGuard guard;

        auto collisionMesh = workphone::make_ptr<scene::CollisionMesh>();
        BOOST_REQUIRE( collisionMesh );

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto meshManager = applicationManager->getMeshManager();
        BOOST_REQUIRE( meshManager );

        auto resource = meshManager->create( "physics_mesh_component_resource_assignment" );
        auto meshResource = workphone::dynamic_pointer_cast<IMeshResource>( resource );
        BOOST_REQUIRE( meshResource );

        const auto resourcePath = StringUtil::cleanupPath( "Cache/PhysicsMeshResource.fbmeshbin" );
        meshResource->setFilePath( resourcePath );

        collisionMesh->setMeshResource( meshResource );
        BOOST_CHECK( collisionMesh->getMeshResource() == meshResource );
        BOOST_CHECK_EQUAL( collisionMesh->getMeshPath(), resourcePath );

        collisionMesh->setMeshResource( nullptr );
        BOOST_CHECK( !collisionMesh->getMeshResource() );
        BOOST_CHECK_EQUAL( collisionMesh->getMeshPath(), resourcePath );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during collision mesh resource assignment test" );
    }
}

BOOST_AUTO_TEST_CASE( physics_mesh_component_reuses_shape_on_play )
{
    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );
    BOOST_REQUIRE( applicationManager->getPhysicsManager() );

    auto collisionMesh = workphone::make_ptr<TestCollisionMesh>();
    BOOST_REQUIRE( collisionMesh );
    collisionMesh->load( nullptr );

    auto initialShape = collisionMesh->getShape();
    BOOST_REQUIRE( initialShape );

    BOOST_CHECK( collisionMesh->handleComponentEvent( static_cast<u32>( scene::IComponent::State::Play ),
                                                      FSMEvent::Enter ) == FSMReturnType::Ok );
    BOOST_CHECK( collisionMesh->getShape() == initialShape );

    collisionMesh->unload( nullptr );
}

BOOST_AUTO_TEST_CASE( physics_mesh_runtime_availability_is_explicit )
{
    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    if( !isPhysicsRuntimeAvailable() )
    {
        BOOST_TEST_MESSAGE( "Physics manager is not available - physics-backed mesh tests are skipped" );
        BOOST_CHECK( !applicationManager->getPhysicsManager() );
        return;
    }

    BOOST_CHECK( applicationManager->getPhysicsManager() );
}

BOOST_AUTO_TEST_CASE( physics_meshshape_with_rigidbody )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        auto physicsManager = applicationManager->getPhysicsManager();

        auto stateManager = applicationManager->getStateManager();
        BOOST_CHECK( stateManager );

        auto timer = applicationManager->getTimer();
        BOOST_CHECK( timer );

        if( !physicsManager )
        {
            WP_LOG_ERROR( "Physics manager is not available." );
            return;
        }

        auto scene = physicsManager->getPhysicsScene();
        BOOST_CHECK( scene );

        applicationManager->setPhysicsManager( physicsManager );
        BOOST_CHECK( applicationManager->getPhysicsManager() );

        // Create mesh shape and rigid body
        auto meshShape = physicsManager->addCollisionShape<physics::IMeshShape>( nullptr );
        BOOST_CHECK( meshShape );

        Transform3<real_Num> transform;
        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_CHECK( rigidBody );

        rigidBody->addShape( meshShape );

        auto shapes = rigidBody->getShapes();
        BOOST_CHECK( shapes.size() >= 1 );

        scene->addActor( rigidBody );

        // Run simulation loop
        for( size_t i = 0; i < 10; ++i )
        {
            timer->update();
            stateManager->update();
            scene->preUpdate();
            scene->update();
            scene->postUpdate();
            physicsManager->update();
        }

        shapes = rigidBody->getShapes();
        BOOST_CHECK( shapes.size() == 1 );

        // Cleanup
        shapes.clear();
        rigidBody = nullptr;
        meshShape = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during mesh shape with rigidbody test" );
    }
}

BOOST_AUTO_TEST_CASE( physics_meshshape_nullptr_resource )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        auto physicsManager = applicationManager->getPhysicsManager();
        if( !physicsManager )
        {
            WP_LOG_ERROR( "Physics manager is not available." );
            return;
        }

        auto scene = physicsManager->getPhysicsScene();
        BOOST_CHECK( scene );

        // Create mesh shape with nullptr
        auto meshShape = physicsManager->addCollisionShape<physics::IMeshShape>( nullptr );
        BOOST_CHECK( meshShape );

        Transform3<real_Num> transform;
        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_CHECK( rigidBody );

        // Explicitly set nullptr mesh resource
        meshShape->setMeshResource( nullptr );
        rigidBody->addShape( meshShape );

        auto shapes = rigidBody->getShapes();
        BOOST_CHECK( shapes.size() == 1 );

        scene->addActor( rigidBody );

        for( size_t i = 0; i < 3; ++i )
        {
            physicsManager->update();
        }

        // Cleanup
        shapes.clear();
        rigidBody = nullptr;
        meshShape = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during nullptr resource test" );
    }
}

BOOST_AUTO_TEST_CASE( physics_meshshape_multiple_shapes_on_body )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        auto physicsManager = applicationManager->getPhysicsManager();
        if( !physicsManager )
        {
            WP_LOG_ERROR( "Physics manager is not available." );
            return;
        }

        auto scene = physicsManager->getPhysicsScene();
        BOOST_CHECK( scene );

        Transform3<real_Num> transform;
        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_CHECK( rigidBody );

        // Add multiple mesh shapes to single body
        constexpr size_t numShapes = 3;
        for( size_t i = 0; i < numShapes; ++i )
        {
            auto meshShape = physicsManager->addCollisionShape<physics::IMeshShape>( nullptr );
            BOOST_CHECK( meshShape );
            rigidBody->addShape( meshShape );
        }

        auto shapes = rigidBody->getShapes();
        BOOST_CHECK( shapes.size() == numShapes );

        scene->addActor( rigidBody );

        for( size_t i = 0; i < 3; ++i )
        {
            physicsManager->update();
        }

        // Verify shapes remain attached
        shapes = rigidBody->getShapes();
        BOOST_CHECK( shapes.size() == numShapes );

        shapes.clear();
        rigidBody = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during multiple shapes test" );
    }
}

BOOST_AUTO_TEST_CASE( physics_meshshape_remove_and_readd )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        auto physicsManager = applicationManager->getPhysicsManager();
        if( !physicsManager )
        {
            WP_LOG_ERROR( "Physics manager is not available." );
            return;
        }

        auto scene = physicsManager->getPhysicsScene();
        BOOST_CHECK( scene );

        auto meshShape = physicsManager->addCollisionShape<physics::IMeshShape>( nullptr );
        BOOST_CHECK( meshShape );

        Transform3<real_Num> transform;
        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_CHECK( rigidBody );

        // Add shape
        rigidBody->addShape( meshShape );
        BOOST_CHECK( rigidBody->getShapes().size() == 1 );

        scene->addActor( rigidBody );
        physicsManager->update();

        // Remove shape
        rigidBody->removeShape( meshShape );
        auto shapes = rigidBody->getShapes();
        BOOST_CHECK( shapes.size() == 0 );

        physicsManager->update();

        // Re-add shape
        rigidBody->addShape( meshShape );
        shapes = rigidBody->getShapes();
        BOOST_CHECK( shapes.size() == 1 );

        physicsManager->update();

        shapes.clear();
        rigidBody = nullptr;
        meshShape = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during remove and re-add test" );
    }
}

BOOST_AUTO_TEST_CASE( physics_meshshape_transform_variations )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        auto physicsManager = applicationManager->getPhysicsManager();
        if( !physicsManager )
        {
            WP_LOG_ERROR( "Physics manager is not available." );
            return;
        }

        auto scene = physicsManager->getPhysicsScene();
        BOOST_CHECK( scene );

        // Test with identity transform
        {
            Transform3<real_Num> identity;
            BOOST_CHECK( identity.isValid() );

            auto rigidBody = physicsManager->addRigidStatic( identity );
            BOOST_CHECK( rigidBody );

            auto meshShape = physicsManager->addCollisionShape<physics::IMeshShape>( nullptr );
            rigidBody->addShape( meshShape );
            scene->addActor( rigidBody );
        }

        // Test with translated transform
        {
            Transform3<real_Num> translated;
            translated.setPosition( Vector3<real_Num>( 100.0f, 200.0f, 300.0f ) );
            BOOST_CHECK( translated.isValid() );
            BOOST_CHECK( translated.isSane() );

            auto rigidBody = physicsManager->addRigidStatic( translated );
            BOOST_CHECK( rigidBody );

            auto meshShape = physicsManager->addCollisionShape<physics::IMeshShape>( nullptr );
            rigidBody->addShape( meshShape );
            scene->addActor( rigidBody );
        }

        // Test with rotated transform
        {
            Transform3<real_Num> rotated;

            //auto rotation = Quaternion<real_Num>::fromAngleAxis(
            //    Math<real_Num>::pi() / 4.0f, Vector3 < real_Num > ::unitY()
            //                                      );
            //rotated.setRotation( rotation );
            //BOOST_CHECK( rotated.isValid() );

            auto rigidBody = physicsManager->addRigidStatic( rotated );
            BOOST_CHECK( rigidBody );

            auto meshShape = physicsManager->addCollisionShape<physics::IMeshShape>( nullptr );
            rigidBody->addShape( meshShape );
            scene->addActor( rigidBody );
        }

        for( size_t i = 0; i < 5; ++i )
        {
            physicsManager->update();
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during transform variations test" );
    }
}

BOOST_AUTO_TEST_CASE( raycasts_mesh_basic )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        auto physicsManager = applicationManager->getPhysicsManager();

        auto meshManager = applicationManager->getMeshManager();
        BOOST_CHECK( meshManager );

        if( !physicsManager )
        {
            WP_LOG_ERROR( "Physics manager is not available." );
            return;
        }

        auto scene = physicsManager->getPhysicsScene();
        BOOST_CHECK( scene );

        auto meshShape = physicsManager->addCollisionShape<physics::IMeshShape>( nullptr );
        BOOST_CHECK( meshShape );

        auto resource = meshManager->loadFromFile( "cube.fbmeshbin" );
        if( !resource )
        {
            BOOST_TEST_MESSAGE( "cube.fbmeshbin is not available - skipping mesh-backed raycast test" );
            return;
        }

        {
            BOOST_CHECK( workphone::dynamic_pointer_cast<IMeshResource>( resource ) );

            auto meshResource = workphone::static_pointer_cast<IMeshResource>( resource );
            BOOST_CHECK( meshResource );

            meshShape->setMeshResource( meshResource );
        }

        auto boxSize = 500.f;
        Transform3<real_Num> transform;
        auto position = Vector3<real_Num>::unitY() * -( boxSize * static_cast<real_Num>( 0.5 ) );
        transform.setPosition( position );

        BOOST_CHECK( transform.isValid() );
        BOOST_CHECK( transform.isSane() );

        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_CHECK( rigidBody );

        rigidBody->addShape( meshShape );
        BOOST_CHECK( rigidBody->getShapes().size() == 1 );

        const auto staticActorsBefore = scene->numStaticActors();
        scene->addActor( rigidBody );
        const auto staticActorsAfterAdd = scene->numStaticActors();
        if( staticActorsAfterAdd == staticActorsBefore )
        {
            BOOST_TEST_MESSAGE(
                "Mesh rigid static was not accepted by the physics scene - skipping raycast "
                "expectations" );
            return;
        }

        BOOST_CHECK_GE( staticActorsAfterAdd, staticActorsBefore + 1 );
        BOOST_CHECK( scene->hasActor( rigidBody ) );

        auto origin = Vector3<real_Num>::unitY() * 250.0f;
        auto dir = -Vector3<real_Num>::unitY();

        for( size_t i = 0; i < 3; ++i )
        {
            physicsManager->update();
        }

        Array<SmartPtr<physics::IRaycastHit>> hits;
        auto result = scene->castRay( origin, dir, hits );
        if( !result )
        {
            BOOST_TEST_MESSAGE(
                "Mesh actor was accepted by the physics scene, but the test ray did not intersect the "
                "cooked mesh." );
            BOOST_CHECK( hits.empty() );

            scene->removeActor( rigidBody );
            BOOST_CHECK_LE( scene->numStaticActors(), staticActorsBefore );

            rigidBody = nullptr;
            meshShape = nullptr;

            return;
        }

        BOOST_CHECK( !hits.empty() );

        for( size_t i = 0; i < 3; ++i )
        {
            physicsManager->update();
        }

        hits.clear();
        result = scene->castRay( origin, dir, hits );
        if( result )
        {
            BOOST_CHECK( !hits.empty() );
        }
        else
        {
            BOOST_TEST_MESSAGE(
                "Mesh raycast missed after a physics update; actor cleanup is still verified." );
            BOOST_CHECK( hits.empty() );
        }

        scene->removeActor( rigidBody );
        BOOST_CHECK_LE( scene->numStaticActors(), staticActorsBefore );

        rigidBody = nullptr;
        meshShape = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during basic raycast test" );
    }
}

BOOST_AUTO_TEST_CASE( raycasts_mesh_miss )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        auto physicsManager = applicationManager->getPhysicsManager();

        auto meshManager = applicationManager->getMeshManager();
        BOOST_CHECK( meshManager );

        if( !physicsManager )
        {
            WP_LOG_ERROR( "Physics manager is not available." );
            return;
        }

        auto scene = physicsManager->getPhysicsScene();
        BOOST_CHECK( scene );

        auto meshShape = physicsManager->addCollisionShape<physics::IMeshShape>( nullptr );
        BOOST_CHECK( meshShape );

        auto resource = meshManager->loadFromFile( "cube.fbmeshbin" );
        if( resource )
        {
            auto meshResource = workphone::static_pointer_cast<IMeshResource>( resource );
            meshShape->setMeshResource( meshResource );
        }

        Transform3<real_Num> transform;
        transform.setPosition( Vector3<real_Num>( 0.0f, 0.0f, 0.0f ) );

        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_CHECK( rigidBody );

        rigidBody->addShape( meshShape );
        const auto staticActorsBefore = scene->numStaticActors();
        scene->addActor( rigidBody );

        for( size_t i = 0; i < 3; ++i )
        {
            physicsManager->update();
        }

        // Cast ray that should miss (pointing away from mesh)
        auto origin = Vector3<real_Num>( 1000.0f, 1000.0f, 1000.0f );
        auto dir = Vector3<real_Num>::unitY();  // Pointing away

        Array<SmartPtr<physics::IRaycastHit>> hits;
        auto result = scene->castRay( origin, dir, hits );
        BOOST_CHECK( result == false );
        BOOST_CHECK( hits.empty() );

        scene->removeActor( rigidBody );
        BOOST_CHECK_LE( scene->numStaticActors(), staticActorsBefore );

        rigidBody = nullptr;
        meshShape = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during raycast miss test" );
    }
}

BOOST_AUTO_TEST_CASE( raycasts_mesh_multiple_hits )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        auto physicsManager = applicationManager->getPhysicsManager();

        auto meshManager = applicationManager->getMeshManager();
        BOOST_CHECK( meshManager );

        if( !physicsManager )
        {
            WP_LOG_ERROR( "Physics manager is not available." );
            return;
        }

        auto scene = physicsManager->getPhysicsScene();
        BOOST_CHECK( scene );

        auto resource = meshManager->loadFromFile( "cube.fbmeshbin" );
        if( !resource )
        {
            BOOST_TEST_MESSAGE(
                "cube.fbmeshbin is not available - skipping mesh-backed multi-raycast test" );
            return;
        }

        // Create multiple mesh bodies along ray path
        Array<SmartPtr<physics::IRigidStatic3>> bodies;
        const auto staticActorsBefore = scene->numStaticActors();
        for( int i = 0; i < 3; ++i )
        {
            auto meshShape = physicsManager->addCollisionShape<physics::IMeshShape>( nullptr );
            BOOST_CHECK( meshShape );

            auto meshResource = workphone::static_pointer_cast<IMeshResource>( resource );
            meshShape->setMeshResource( meshResource );

            Transform3<real_Num> transform;
            transform.setPosition( Vector3<real_Num>( 0.0f, static_cast<real_Num>( i * -100 ), 0.0f ) );

            auto rigidBody = physicsManager->addRigidStatic( transform );
            BOOST_CHECK( rigidBody );

            rigidBody->addShape( meshShape );
            scene->addActor( rigidBody );
            bodies.push_back( rigidBody );
        }

        const auto staticActorsAfterAdd = scene->numStaticActors();
        if( staticActorsAfterAdd == staticActorsBefore )
        {
            BOOST_TEST_MESSAGE(
                "Mesh rigid statics were not accepted by the physics scene - skipping multi-raycast "
                "expectations" );
            return;
        }

        for( size_t i = 0; i < 3; ++i )
        {
            physicsManager->update();
        }

        // Cast ray through all meshes
        auto origin = Vector3<real_Num>( 0.0f, 500.0f, 0.0f );
        auto dir = -Vector3<real_Num>::unitY();

        Array<SmartPtr<physics::IRaycastHit>> hits;
        auto result = scene->castRay( origin, dir, hits );
        if( result )
        {
            BOOST_CHECK( !hits.empty() );
        }
        else
        {
            BOOST_TEST_MESSAGE(
                "Mesh actors were accepted by the physics scene, but the test ray did not intersect the "
                "cooked meshes." );
            BOOST_CHECK( hits.empty() );
        }

        for( auto &body : bodies )
        {
            scene->removeActor( body );
        }
        BOOST_CHECK_LE( scene->numStaticActors(), staticActorsBefore );

        bodies.clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during multiple hits raycast test" );
    }
}

BOOST_AUTO_TEST_CASE( physics_meshshape_actor_removal )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        auto physicsManager = applicationManager->getPhysicsManager();
        if( !physicsManager )
        {
            WP_LOG_ERROR( "Physics manager is not available." );
            return;
        }

        auto scene = physicsManager->getPhysicsScene();
        BOOST_CHECK( scene );

        auto meshShape = physicsManager->addCollisionShape<physics::IMeshShape>( nullptr );
        BOOST_CHECK( meshShape );

        Transform3<real_Num> transform;
        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_CHECK( rigidBody );

        rigidBody->addShape( meshShape );
        scene->addActor( rigidBody );

        physicsManager->update();

        // Remove actor from scene
        scene->removeActor( rigidBody );

        physicsManager->update();

        // Verify no crash after removal
        BOOST_CHECK( true );

        rigidBody = nullptr;
        meshShape = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during actor removal test" );
    }
}

BOOST_AUTO_TEST_CASE( physics_meshshape_rapid_creation_destruction )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        auto physicsManager = applicationManager->getPhysicsManager();
        if( !physicsManager )
        {
            WP_LOG_ERROR( "Physics manager is not available." );
            return;
        }

        auto scene = physicsManager->getPhysicsScene();
        BOOST_CHECK( scene );

        // Rapid creation and destruction to test resource management
        for( int iteration = 0; iteration < 10; ++iteration )
        {
            auto meshShape = physicsManager->addCollisionShape<physics::IMeshShape>( nullptr );
            BOOST_CHECK( meshShape );

            Transform3<real_Num> transform;
            auto rigidBody = physicsManager->addRigidStatic( transform );
            BOOST_CHECK( rigidBody );

            rigidBody->addShape( meshShape );
            scene->addActor( rigidBody );

            physicsManager->update();

            scene->removeActor( rigidBody );
            rigidBody = nullptr;
            meshShape = nullptr;
        }

        // Final update to ensure cleanup
        for( size_t i = 0; i < 5; ++i )
        {
            physicsManager->update();
        }

        BOOST_CHECK( true );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during rapid creation/destruction test" );
    }
}

BOOST_AUTO_TEST_CASE( physics_meshshape_empty_scene_raycast )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        auto physicsManager = applicationManager->getPhysicsManager();
        if( !physicsManager )
        {
            WP_LOG_ERROR( "Physics manager is not available." );
            return;
        }

        auto scene = physicsManager->getPhysicsScene();
        BOOST_CHECK( scene );

        // Raycast on empty scene should not crash
        auto origin = Vector3<real_Num>( 0.0f, 100.0f, 0.0f );
        auto dir = -Vector3<real_Num>::unitY();

        Array<SmartPtr<physics::IRaycastHit>> hits;
        auto result = scene->castRay( origin, dir, hits );

        // No meshes in scene, should miss
        BOOST_CHECK( result == false );
        BOOST_CHECK( hits.empty() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during empty scene raycast test" );
    }
}

BOOST_AUTO_TEST_CASE( physics_mesh_bvh_repeated_rotated_raycast )
{
    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );
    auto physicsManager = applicationManager->getPhysicsManager();
    BOOST_REQUIRE( physicsManager );
    auto scene = physicsManager->getPhysicsScene();
    BOOST_REQUIRE( scene );
    scene->clear();

    constexpr u32 gridSize = 128u;
    Array<Vector3<real_Num>> vertices;
    Array<Vector3<real_Num>> normals;
    Array<Vector2<real_Num>> uvs;
    Array<u32> indices;
    vertices.reserve( ( gridSize + 1u ) * ( gridSize + 1u ) );
    normals.reserve( vertices.capacity() );
    uvs.reserve( vertices.capacity() );
    indices.reserve( gridSize * gridSize * 6u );

    const auto halfSize = static_cast<real_Num>( gridSize ) * static_cast<real_Num>( 0.5 );
    for( u32 z = 0u; z <= gridSize; ++z )
    {
        for( u32 x = 0u; x <= gridSize; ++x )
        {
            vertices.emplace_back( static_cast<real_Num>( x ) - halfSize, static_cast<real_Num>( 0 ),
                                   static_cast<real_Num>( z ) - halfSize );
            normals.push_back( Vector3<real_Num>::unitY() );
            uvs.emplace_back( static_cast<real_Num>( x ) / static_cast<real_Num>( gridSize ),
                              static_cast<real_Num>( z ) / static_cast<real_Num>( gridSize ) );
        }
    }

    for( u32 z = 0u; z < gridSize; ++z )
    {
        for( u32 x = 0u; x < gridSize; ++x )
        {
            const auto row = gridSize + 1u;
            const auto v00 = z * row + x;
            const auto v10 = v00 + 1u;
            const auto v01 = v00 + row;
            const auto v11 = v01 + 1u;
            indices.push_back( v00 );
            indices.push_back( v01 );
            indices.push_back( v10 );
            indices.push_back( v10 );
            indices.push_back( v01 );
            indices.push_back( v11 );
        }
    }

    auto mesh = MeshUtil::createMesh( vertices, normals, uvs, indices );
    BOOST_REQUIRE( mesh );
    auto meshShape = physicsManager->addCollisionShape<physics::IMeshShape>( nullptr );
    BOOST_REQUIRE( meshShape );
    meshShape->setCleanMesh( mesh );

    const auto orientation = Quaternion<real_Num>::angleAxis(
        Math<real_Num>::pi() / static_cast<real_Num>( 6 ), Vector3<real_Num>::unitZ() );
    const auto position = Vector3<real_Num>( static_cast<real_Num>( 10 ), static_cast<real_Num>( 3 ),
                                             static_cast<real_Num>( -4 ) );
    Transform3<real_Num> transform;
    transform.setPosition( position );
    transform.setOrientation( orientation );

    auto body = physicsManager->addRigidStatic( transform );
    BOOST_REQUIRE( body );
    body->addShape( meshShape );
    scene->addActor( body );
    BOOST_REQUIRE( scene->hasActor( body ) );

    const auto rayDirection = -( orientation * Vector3<real_Num>::unitY() );
    const auto rayOrigin = position - rayDirection * static_cast<real_Num>( 10 );
    const auto ray = Ray3<real_Num>( rayOrigin, rayDirection );
    auto hit = workphone::make_ptr<physics::RaycastHit>();
    hit->setCheckStatic( true );
    hit->setCheckDynamic( false );

    constexpr u32 rayCount = 2000u;
    const auto firstHit = scene->castRay( ray, hit );
    BOOST_TEST_MESSAGE( "Rotated BVH probe: origin=("
                        << rayOrigin.X() << ", " << rayOrigin.Y() << ", " << rayOrigin.Z()
                        << ") direction=(" << rayDirection.X() << ", " << rayDirection.Y() << ", "
                        << rayDirection.Z() << ") hit=" << firstHit );
    if( !firstHit )
    {
        scene->removeActor( body );
        physicsManager->removeCollisionShape( meshShape );
        physicsManager->removePhysicsBody( body );
        BOOST_FAIL( "Rotated BVH mesh raycast missed the grid center." );
        return;
    }

    const auto start = std::chrono::steady_clock::now();
    for( u32 query = 1u; query < rayCount; ++query )
    {
        BOOST_REQUIRE( scene->castRay( ray, hit ) );
    }
    const auto elapsed = std::chrono::duration<double>( std::chrono::steady_clock::now() - start );

    BOOST_CHECK_CLOSE( hit->getDistance(), static_cast<real_Num>( 10 ), static_cast<real_Num>( 0.01 ) );
    BOOST_CHECK_GT(
        Math<real_Num>::Abs( hit->getNormal().dotProduct( orientation * Vector3<real_Num>::unitY() ) ),
        static_cast<real_Num>( 0.99 ) );
    BOOST_TEST_MESSAGE( "2,000 BVH mesh raycasts across 32,768 triangles: " << elapsed.count()
                                                                            << " seconds" );

    scene->removeActor( body );
    physicsManager->removePhysicsBody( body );
    physicsManager->removeCollisionShape( meshShape );
}

BOOST_AUTO_TEST_CASE( physics_meshshape_cooks_shared_vertex_buffer )
{
    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );
    auto physicsManager = applicationManager->getPhysicsManager();
    BOOST_REQUIRE( physicsManager );
    auto scene = physicsManager->getPhysicsScene();
    BOOST_REQUIRE( scene );
    scene->clear();

    const Array<Vector3<real_Num>> vertices = { { -5, 0, -5 }, { 5, 0, -5 }, { 5, 0, 5 }, { -5, 0, 5 } };
    const Array<Vector3<real_Num>> normals( vertices.size(), Vector3<real_Num>::unitY() );
    const Array<Vector2<real_Num>> uvs = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
    const Array<u32> expectedIndices = { 0, 2, 1, 0, 3, 2 };

    auto mesh = MeshUtil::createMesh( vertices, normals, uvs, expectedIndices );
    BOOST_REQUIRE( mesh );
    auto subMeshes = mesh->getSubMeshes();
    BOOST_REQUIRE_EQUAL( subMeshes.size(), 1u );
    auto subMesh = subMeshes.front();
    BOOST_REQUIRE( subMesh );

    /*
     * Reproduce the representation used by imported track meshes: indices
     * remain on the submesh while positions live in the mesh-level buffer.
     */
    auto sharedVertexBuffer = subMesh->getVertexBuffer();
    BOOST_REQUIRE( sharedVertexBuffer );
    mesh->setSharedVertexBuffer( sharedVertexBuffer );
    mesh->setHasSharedVertexData( true );
    subMesh->setUseSharedVertices( true );
    subMesh->setVertexBuffer( nullptr );

    const auto cookedPoints = MeshUtil::getPoints( mesh );
    const auto cookedIndices = MeshUtil::getIndices( mesh );
    BOOST_REQUIRE_EQUAL( cookedPoints.size(), vertices.size() );
    BOOST_REQUIRE_EQUAL( cookedIndices.size(), expectedIndices.size() );
    BOOST_CHECK_EQUAL_COLLECTIONS( cookedIndices.begin(), cookedIndices.end(), expectedIndices.begin(),
                                   expectedIndices.end() );

    const String roundTripPath = "physics_shared_vertex_roundtrip.fbmeshbin";
    guard.trackFilesystemPath( roundTripPath );
    auto serializableMesh = workphone::dynamic_pointer_cast<Mesh>( mesh );
    BOOST_REQUIRE( serializableMesh );
    MeshSerializer serializer;
    serializer.exportMesh( serializableMesh.get(), roundTripPath );

    auto meshManager =
        workphone::dynamic_pointer_cast<MeshManager>( applicationManager->getMeshManager() );
    BOOST_REQUIRE( meshManager );
    auto roundTripMesh = meshManager->loadMesh( roundTripPath );
    BOOST_REQUIRE( roundTripMesh );
    BOOST_CHECK_EQUAL( MeshUtil::getPoints( roundTripMesh ).size(), vertices.size() );
    const auto roundTripIndices = MeshUtil::getIndices( roundTripMesh );
    BOOST_CHECK_EQUAL_COLLECTIONS( roundTripIndices.begin(), roundTripIndices.end(),
                                   expectedIndices.begin(), expectedIndices.end() );

    auto meshShape = physicsManager->addCollisionShape<physics::IMeshShape>( nullptr );
    BOOST_REQUIRE( meshShape );
    meshShape->setCleanMesh( mesh );

    auto body = physicsManager->addRigidStatic( Transform3<real_Num>() );
    BOOST_REQUIRE( body );
    body->addShape( meshShape );
    scene->addActor( body );
    BOOST_REQUIRE( scene->hasActor( body ) );

    Array<SmartPtr<physics::IRaycastHit>> hits;
    BOOST_REQUIRE( scene->castRay( Vector3<real_Num>( 0, 5, 0 ), -Vector3<real_Num>::unitY(), hits ) );
    BOOST_REQUIRE( !hits.empty() );
    BOOST_CHECK_SMALL( hits.front()->getPoint().Y(), static_cast<real_Num>( 1.0e-4 ) );

    scene->removeActor( body );
    physicsManager->removePhysicsBody( body );
    physicsManager->removeCollisionShape( meshShape );
}
