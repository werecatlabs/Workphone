#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <Workphone/Interface/Physics/IBoxShape3.hpp>
#include <Workphone/Interface/Physics/IMeshShape.hpp>
#include <Workphone/Interface/Physics/IPlaneShape3.hpp>
#include <Workphone/Interface/Physics/ISphereShape3.hpp>
#include <Workphone/Interface/Physics/ITerrainShape.hpp>
#include <Workphone/Interface/Physics/IPhysicsMaterial3.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <Workphone/Interface/Physics/IRaycastHit.hpp>
#include <Workphone/Interface/Physics/IRigidDynamic3.hpp>
#include <Workphone/Interface/Physics/IRigidStatic3.hpp>
#include <Workphone/Scene/Components/CollisionBox.hpp>
#include <Workphone/Scene/Components/Constraint.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    // Tolerance helpers used for all floating-point / vector comparisons in this suite.
    void checkVectorClose( const Vector3<real_Num> &actual, const Vector3<real_Num> &expected,
                           real_Num tolerance = real_Num( 0.001 ) )
    {
        BOOST_CHECK_MESSAGE( MathUtil<real_Num>::equals( actual, expected, tolerance ),
                             "actual=(" << actual.X() << ", " << actual.Y() << ", " << actual.Z()
                                        << ") expected=(" << expected.X() << ", " << expected.Y() << ", "
                                        << expected.Z() << ")" );
    }

    void checkQuaternionClose( const Quaternion<real_Num> &actual, const Quaternion<real_Num> &expected,
                               real_Num tolerance = real_Num( 0.001 ) )
    {
        BOOST_CHECK_MESSAGE( MathUtil<real_Num>::equals( actual.w, expected.w, tolerance ),
                             "quat.w actual=" << actual.w << " expected=" << expected.w );
        BOOST_CHECK_MESSAGE( MathUtil<real_Num>::equals( actual.x, expected.x, tolerance ),
                             "quat.x actual=" << actual.x << " expected=" << expected.x );
        BOOST_CHECK_MESSAGE( MathUtil<real_Num>::equals( actual.y, expected.y, tolerance ),
                             "quat.y actual=" << actual.y << " expected=" << expected.y );
        BOOST_CHECK_MESSAGE( MathUtil<real_Num>::equals( actual.z, expected.z, tolerance ),
                             "quat.z actual=" << actual.z << " expected=" << expected.z );
    }

    SmartPtr<scene::IGameActor> createStaticBox(
        TestGuard &fixture, const Vector3<real_Num> &position = Vector3<real_Num>::zero() )
    {
        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );
        actor->setStatic( true );
        actor->setPosition( position );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );
        collisionBox->setExtents( Vector3<real_Num>( 1.0f, 1.0f, 1.0f ) );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );
        fixture.sceneManager->edit();
        fixture.resetTimer();
        fixture.updatePhysics( 5 );

        return actor;
    }

    SmartPtr<scene::IGameActor> createDynamicBox(
        TestGuard &fixture, const Vector3<real_Num> &position = Vector3<real_Num>::zero() )
    {
        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );
        actor->setStatic( false );
        actor->setPosition( position );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );
        collisionBox->setExtents( Vector3<real_Num>( 1.0f, 1.0f, 1.0f ) );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );
        fixture.sceneManager->play();
        fixture.resetTimer();
        fixture.updatePhysics( 5 );

        return actor;
    }

    SmartPtr<scene::IGameActor> createGround( TestGuard &fixture )
    {
        auto ground = ApplicationUtil::createDefaultGround();
        BOOST_REQUIRE( ground );

        fixture.sceneManager->edit();
        fixture.resetTimer();
        fixture.updatePhysics( 10 );

        return ground;
    }

    void advancePhysics( TestGuard &fixture, u32 iterations = 10 )
    {
        fixture.resetTimer();
        fixture.updatePhysics( iterations );
    }
}  // namespace

/**
 * @brief Shared fixture for all ComponentTestsPhysics tests.
 *
 * Derives from TestGuard and requires the physics manager to be available. The
 * fixture clears the scene and scene manager between tests so each case starts from
 * a deterministic empty state.
 */
struct PhysicsComponentFixture : TestGuard
{
    PhysicsComponentFixture() : TestGuard( true )
    {
        if( !isAvailable )
        {
            return;
        }

        if( scene )
        {
            scene->clear();
        }

        if( sceneManager )
        {
            sceneManager->clear();
            sceneManager->play();
        }

        resetTimer();
        updatePhysics( 1 );
    }
};

BOOST_FIXTURE_TEST_SUITE( ComponentTestsPhysics, PhysicsComponentFixture )

//------------------------------------------------------------------------------
// Shape factory tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( physics_shape_factory_creates_and_removes_all_builtin_shape_types )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto typeManager = TypeManager::instance();
        BOOST_REQUIRE( typeManager );

        const Array<hash64> shapeTypes = { typeManager->getHash( physics::IBoxShape3::typeInfo() ),
                                           typeManager->getHash( physics::ISphereShape3::typeInfo() ),
                                           typeManager->getHash( physics::IPlaneShape3::typeInfo() ),
                                           typeManager->getHash( physics::IMeshShape::typeInfo() ),
                                           typeManager->getHash( physics::ITerrainShape::typeInfo() ) };

        for( auto shapeType : shapeTypes )
        {
            auto shape = physicsManager->addCollisionShapeByType( shapeType, nullptr );
            BOOST_REQUIRE_MESSAGE( shape, "Expected shape type " << shapeType << " to be created" );
            BOOST_CHECK( shape->hasShapeData() );
            BOOST_CHECK_MESSAGE( physicsManager->removeCollisionShape( shape ),
                                 "Expected shape type " << shapeType << " to be removed" );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( physics_shape_factory_rejects_unknown_type )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        BOOST_CHECK( !physicsManager->addCollisionShapeByType( 0, nullptr ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( physics_shape_factory_double_remove_reports_failure_without_crashing )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto typeManager = TypeManager::instance();
        BOOST_REQUIRE( typeManager );

        auto shape = physicsManager->addCollisionShapeByType(
            typeManager->getHash( physics::IBoxShape3::typeInfo() ), nullptr );
        BOOST_REQUIRE( shape );

        BOOST_CHECK( physicsManager->removeCollisionShape( shape ) );
        BOOST_CHECK( !physicsManager->removeCollisionShape( shape ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( physics_shape_factory_template_helper_returns_typed_shape )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto boxShape = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
        BOOST_REQUIRE( boxShape );
        BOOST_CHECK( workphone::dynamic_pointer_cast<physics::IBoxShape3>( boxShape ) );
        BOOST_CHECK( physicsManager->removeCollisionShape( boxShape ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

//------------------------------------------------------------------------------
// Physics manager lifecycle tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( physics_manager_material_lifecycle )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto material = physicsManager->addMaterial();
        BOOST_REQUIRE( material );

        material->setRestitution( 0.5f );
        material->setDynamicFriction( 0.3f, 0 );
        material->setStaticFriction( 0.4f, 0 );

        BOOST_CHECK_CLOSE( material->getRestitution(), 0.5f, 0.001f );
        BOOST_CHECK_CLOSE( material->getDynamicFriction( 0 ), 0.3f, 0.001f );
        BOOST_CHECK_CLOSE( material->getStaticFriction( 0 ), 0.4f, 0.001f );

        physicsManager->removeMaterial( material );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( physics_manager_scene_lifecycle )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto newScene = physicsManager->addScene();
        BOOST_REQUIRE( newScene );

        const auto gravity = Vector3<real_Num>( 0.0f, -5.0f, 0.0f );
        const auto size = Vector3<real_Num>( 1000.0f, 1000.0f, 1000.0f );

        newScene->setGravity( gravity );
        newScene->setSize( size );

        checkVectorClose( newScene->getGravity(), gravity );
        checkVectorClose( newScene->getSize(), size );

        physicsManager->removeScene( newScene );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( physics_manager_raycast_hit_data_lifecycle )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto hitData = physicsManager->addRaycastHitData();
        BOOST_REQUIRE( hitData );

        hitData->setDistance( 12.5f );
        BOOST_CHECK_CLOSE( hitData->getDistance(), 12.5f, 0.001f );

        physicsManager->removeRaycastHitData( hitData );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

//------------------------------------------------------------------------------
// Rigidbody integration tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( rigidbody_dynamic_actor_creates_rigid_dynamic )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto actor = createDynamicBox( *this );
        BOOST_REQUIRE( actor );

        auto rigidbody = actor->getComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        BOOST_CHECK( rigidbody->isLoaded() );
        BOOST_CHECK( rigidbody->isValid() );
        BOOST_CHECK( !rigidbody->getRigidStatic() );
        BOOST_REQUIRE( rigidbody->getRigidDynamic() );
        BOOST_CHECK( rigidbody->getRigidDynamic()->isValid() );
        BOOST_CHECK( !actor->isStatic() );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( rigidbody_static_actor_creates_rigid_static )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto actor = createStaticBox( *this );
        BOOST_REQUIRE( actor );

        auto rigidbody = actor->getComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        BOOST_CHECK( rigidbody->isLoaded() );
        BOOST_CHECK( rigidbody->isValid() );
        BOOST_CHECK( !rigidbody->getRigidDynamic() );
        BOOST_REQUIRE( rigidbody->getRigidStatic() );
        BOOST_CHECK( rigidbody->getRigidStatic()->isValid() );
        BOOST_CHECK( actor->isStatic() );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( rigidbody_transform_syncs_to_physics_actor )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        const auto expectedPosition = Vector3<real_Num>( 10.0f, 20.0f, 30.0f );
        auto actor = createStaticBox( *this, expectedPosition );
        BOOST_REQUIRE( actor );

        auto rigidbody = actor->getComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        auto rigidStatic = rigidbody->getRigidStatic();
        BOOST_REQUIRE( rigidStatic );
        checkVectorClose( rigidStatic->getTransform().getPosition(), expectedPosition );
        checkVectorClose( rigidbody->getTransform().getPosition(), expectedPosition );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( rigidbody_mass_and_inertia_sync_to_dynamic_actor )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor );
        actor->setStatic( false );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        const auto expectedMass = 250.0f;
        const auto expectedInertia = Vector3<real_Num>( 2.0f, 3.0f, 4.0f );
        rigidbody->setMass( expectedMass );
        rigidbody->setMassSpaceInertiaTensor( expectedInertia );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );
        collisionBox->setExtents( Vector3<real_Num>( 1.0f, 1.0f, 1.0f ) );

        scene->registerAllUpdates( actor );
        scene->addActor( actor );
        sceneManager->play();
        advancePhysics( *this, 10 );

        BOOST_REQUIRE( rigidbody->getRigidDynamic() );
        BOOST_CHECK_CLOSE( rigidbody->getRigidDynamic()->getMass(), expectedMass, 0.001f );
        checkVectorClose( rigidbody->getRigidDynamic()->getMassSpaceInertiaTensor(), expectedInertia );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( rigidbody_velocity_round_trips_to_dynamic_actor )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto actor = createDynamicBox( *this );
        BOOST_REQUIRE( actor );

        auto rigidbody = actor->getComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        auto rigidDynamic = rigidbody->getRigidDynamic();
        BOOST_REQUIRE( rigidDynamic );

        const auto linearVelocity = Vector3<real_Num>( 1.0f, 2.0f, 3.0f );
        const auto angularVelocity = Vector3<real_Num>( 0.0f, 1.0f, 0.0f );
        rigidbody->setLinearVelocity( linearVelocity );
        rigidbody->setAngularVelocity( angularVelocity );
        advancePhysics( *this, 1 );

        // Compare magnitudes to tolerate a single gravity step; exact vector tests are
        // done by the dedicated component property suites.
        BOOST_CHECK_CLOSE( rigidbody->getLinearVelocity().length(), linearVelocity.length(), 0.001f );
        BOOST_CHECK_CLOSE( rigidbody->getAngularVelocity().length(), angularVelocity.length(), 0.001f );
        BOOST_CHECK_CLOSE( rigidDynamic->getLinearVelocity().length(), linearVelocity.length(), 0.001f );
        BOOST_CHECK_CLOSE( rigidDynamic->getAngularVelocity().length(), angularVelocity.length(),
                           0.001f );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( rigidbody_collision_filtering_propagates_to_dynamic_actor )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto actor = createDynamicBox( *this );
        BOOST_REQUIRE( actor );

        auto rigidbody = actor->getComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        rigidbody->setGroupMask( 0x00000002u );
        rigidbody->setCollisionMask( 0x00000004u );

        auto rigidDynamic = rigidbody->getRigidDynamic();
        BOOST_REQUIRE( rigidDynamic );
        BOOST_CHECK_EQUAL( rigidDynamic->getCollisionType(), 0x00000002u );
        BOOST_CHECK_EQUAL( rigidDynamic->getCollisionMask(), 0x00000004u );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( rigidbody_kinematic_flag_sets_target_on_dynamic_actor )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto actor = createDynamicBox( *this );
        BOOST_REQUIRE( actor );

        auto rigidbody = actor->getComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        auto rigidDynamic = rigidbody->getRigidDynamic();
        BOOST_REQUIRE( rigidDynamic );

        const auto expectedPosition = Vector3<real_Num>( 5.0f, 10.0f, -3.0f );
        actor->setPosition( expectedPosition );
        rigidbody->setKinematic( true );
        rigidbody->updateKinematicState( true );
        rigidbody->preUpdate();

        Transform3<real_Num> target;
        BOOST_CHECK( rigidDynamic->getKinematicTarget( target ) );
        checkVectorClose( target.getPosition(), expectedPosition );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( rigidbody_unload_releases_physics_actor )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto actor = createDynamicBox( *this );
        BOOST_REQUIRE( actor );

        auto rigidbody = actor->getComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        BOOST_REQUIRE( rigidbody->getRigidDynamic() );
        BOOST_REQUIRE( rigidbody->getRigidbodyListener() );

        rigidbody->unload( nullptr );
        rigidbody->unload( nullptr );

        BOOST_CHECK( !rigidbody->isLoaded() );
        BOOST_CHECK( !rigidbody->getRigidDynamic() );
        BOOST_CHECK( !rigidbody->getRigidStatic() );
        BOOST_CHECK( !rigidbody->getRigidbodyListener() );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

//------------------------------------------------------------------------------
// Constraint integration tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( constraint_with_two_dynamic_bodies_is_valid_and_loaded )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto cubeA = ApplicationUtil::createDefaultCube();
        BOOST_REQUIRE( cubeA );

        auto cubeB = ApplicationUtil::createDefaultCube();
        BOOST_REQUIRE( cubeB );

        auto constraintActor = ApplicationUtil::createDefaultConstraint();
        BOOST_REQUIRE( constraintActor );
        constraintActor->setObjectFlag( OBJECT_FLAG_TRACK_REFERENCES, true );

        auto constraint = constraintActor->getComponent<scene::Constraint>();
        BOOST_REQUIRE( constraint );

        auto bodyA = cubeA->getComponent<scene::Rigidbody>();
        BOOST_REQUIRE( bodyA );

        auto bodyB = cubeB->getComponent<scene::Rigidbody>();
        BOOST_REQUIRE( bodyB );

        // Ensure all created actors are in play state so their components load.
        sceneManager->play();
        advancePhysics( *this, 1 );

        constraint->setBodyA( bodyA );
        constraint->setBodyB( bodyB );

        advancePhysics( *this, 10 );

        BOOST_CHECK( constraint->isLoaded() );
        BOOST_CHECK( constraint->isValid() );
        BOOST_CHECK( constraint->getBodyA().get() == bodyA.get() );
        BOOST_CHECK( constraint->getBodyB().get() == bodyB.get() );
        BOOST_CHECK( constraint->getConstraint() );

        sceneManager->destroyActor( cubeA );
        sceneManager->destroyActor( cubeB );
        sceneManager->destroyActor( constraintActor );

        BOOST_CHECK_EQUAL( constraintActor->getReferences(), 1 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( constraint_null_bodies_are_handled_gracefully )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto constraintActor = ApplicationUtil::createDefaultConstraint();
        BOOST_REQUIRE( constraintActor );

        auto constraint = constraintActor->getComponent<scene::Constraint>();
        BOOST_REQUIRE( constraint );

        constraint->setBodyA( nullptr );
        constraint->setBodyB( nullptr );

        BOOST_CHECK( !constraint->getBodyA() );
        BOOST_CHECK( !constraint->getBodyB() );

        advancePhysics( *this, 5 );

        BOOST_CHECK( !constraint->getConstraint() );

        sceneManager->destroyActor( constraintActor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( constraint_motion_axes_and_break_limits_round_trip )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        scene::Constraint source;
        source.setAxisX( physics::D6MotionEnum::eLOCKED );
        source.setAxisY( physics::D6MotionEnum::eLIMITED );
        source.setAxisZ( physics::D6MotionEnum::eFREE );
        source.setSwing1( physics::D6MotionEnum::eLOCKED );
        source.setSwing2( physics::D6MotionEnum::eLIMITED );
        source.setTwist( physics::D6MotionEnum::eFREE );
        source.setBreakForce( 1000.0f );
        source.setBreakTorque( 500.0f );
        source.setType( scene::Constraint::Type::Fixed );

        auto properties = source.getProperties();
        BOOST_REQUIRE( properties );

        scene::Constraint restored;
        restored.setProperties( properties );

        BOOST_CHECK( restored.getAxisX() == physics::D6MotionEnum::eLOCKED );
        BOOST_CHECK( restored.getAxisY() == physics::D6MotionEnum::eLIMITED );
        BOOST_CHECK( restored.getAxisZ() == physics::D6MotionEnum::eFREE );
        BOOST_CHECK( restored.getSwing1() == physics::D6MotionEnum::eLOCKED );
        BOOST_CHECK( restored.getSwing2() == physics::D6MotionEnum::eLIMITED );
        BOOST_CHECK( restored.getTwist() == physics::D6MotionEnum::eFREE );
        BOOST_CHECK_CLOSE( restored.getBreakForce(), 1000.0f, 0.001f );
        BOOST_CHECK_CLOSE( restored.getBreakTorque(), 500.0f, 0.001f );
        BOOST_CHECK( restored.getType() == scene::Constraint::Type::Fixed );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( constraint_unload_releases_physics_constraint )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto cubeA = ApplicationUtil::createDefaultCube();
        BOOST_REQUIRE( cubeA );

        auto cubeB = ApplicationUtil::createDefaultCube();
        BOOST_REQUIRE( cubeB );

        auto constraintActor = ApplicationUtil::createDefaultConstraint();
        BOOST_REQUIRE( constraintActor );

        auto constraint = constraintActor->getComponent<scene::Constraint>();
        BOOST_REQUIRE( constraint );

        auto bodyA = cubeA->getComponent<scene::Rigidbody>();
        BOOST_REQUIRE( bodyA );

        auto bodyB = cubeB->getComponent<scene::Rigidbody>();
        BOOST_REQUIRE( bodyB );

        // Ensure all created actors are in play state so their components load.
        sceneManager->play();
        advancePhysics( *this, 1 );

        constraint->setBodyA( bodyA );
        constraint->setBodyB( bodyB );
        advancePhysics( *this, 10 );

        BOOST_REQUIRE( constraint->getConstraint() );

        constraint->unload( nullptr );
        constraint->unload( nullptr );

        BOOST_CHECK( !constraint->isLoaded() );
        BOOST_CHECK( !constraint->getConstraint() );

        sceneManager->destroyActor( cubeA );
        sceneManager->destroyActor( cubeB );
        sceneManager->destroyActor( constraintActor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

//------------------------------------------------------------------------------
// Transform synchronization tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( actor_position_syncs_to_static_physics_body )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto actor = createStaticBox( *this );
        BOOST_REQUIRE( actor );

        auto rigidbody = actor->getComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        const auto newPosition = Vector3<real_Num>( 50.0f, 75.0f, 100.0f );
        actor->setPosition( newPosition );
        advancePhysics( *this, 3 );

        auto rigidStatic = rigidbody->getRigidStatic();
        BOOST_REQUIRE( rigidStatic );
        checkVectorClose( rigidStatic->getTransform().getPosition(), newPosition );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( actor_orientation_syncs_to_static_physics_body )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto actor = createStaticBox( *this );
        BOOST_REQUIRE( actor );

        auto rigidbody = actor->getComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        const auto rotation = Quaternion<real_Num>::angleAxis( Math<real_Num>::pi() / real_Num( 4.0 ),
                                                               Vector3<real_Num>::unitY() );
        actor->setOrientation( rotation );
        advancePhysics( *this, 3 );

        auto rigidStatic = rigidbody->getRigidStatic();
        BOOST_REQUIRE( rigidStatic );
        checkQuaternionClose( rigidStatic->getTransform().getOrientation(), rotation );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

//------------------------------------------------------------------------------
// Raycast tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( raycast_hits_static_ground_plane )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto ground = createGround( *this );
        BOOST_REQUIRE( ground );

        auto physicsScene = physicsManager->getPhysicsScene();
        BOOST_REQUIRE( physicsScene );

        Array<SmartPtr<physics::IRaycastHit>> hits;
        auto origin = Vector3<real_Num>( 0.0f, 10.0f, 0.0f );
        auto direction = Vector3<real_Num>( 0.0f, -1.0f, 0.0f );
        BOOST_CHECK( physicsScene->castRay( origin, direction, hits ) );
        BOOST_CHECK( !hits.empty() );

        sceneManager->destroyActor( ground );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( raycast_misses_when_no_geometry )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        sceneManager->edit();
        advancePhysics( *this, 5 );

        auto physicsScene = physicsManager->getPhysicsScene();
        BOOST_REQUIRE( physicsScene );

        Array<SmartPtr<physics::IRaycastHit>> hits;
        auto origin = Vector3<real_Num>( 0.0f, 10.0f, 0.0f );
        auto direction = Vector3<real_Num>( 0.0f, -1.0f, 0.0f );
        BOOST_CHECK( !physicsScene->castRay( origin, direction, hits ) );
        BOOST_CHECK( hits.empty() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

//------------------------------------------------------------------------------
// Cleanup / lifecycle edge cases
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( destroying_actor_removes_physics_body_from_scene )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto physicsScene = physicsManager->getPhysicsScene();
        BOOST_REQUIRE( physicsScene );

        const auto initialActorCount = physicsScene->getActors().size();

        auto actor = createStaticBox( *this );
        BOOST_REQUIRE( actor );

        BOOST_CHECK_GT( physicsScene->getActors().size(), initialActorCount );

        sceneManager->destroyActor( actor );
        advancePhysics( *this, 3 );

        BOOST_CHECK_EQUAL( physicsScene->getActors().size(), initialActorCount );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( scene_clear_removes_all_physics_bodies )
{
    try
    {
        if( !isAvailable )
        {
            return;
        }

        auto ground = ApplicationUtil::createDefaultGround();
        BOOST_REQUIRE( ground );

        auto cube = ApplicationUtil::createDefaultCube();
        BOOST_REQUIRE( cube );

        sceneManager->edit();
        advancePhysics( *this, 10 );

        auto physicsScene = physicsManager->getPhysicsScene();
        BOOST_REQUIRE( physicsScene );
        BOOST_CHECK( !physicsScene->getActors().empty() );

        scene->clear();
        advancePhysics( *this, 3 );

        BOOST_CHECK( physicsScene->getActors().empty() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()
