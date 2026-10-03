#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Graphics/IGraphicsTerrain.hpp>
#include <Workphone/Interface/Physics/IBoxShape3.hpp>
#include <Workphone/Interface/Physics/IRigidDynamic3.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    void checkVectorClose( const Vector3<real_Num> &actual, const Vector3<real_Num> &expected,
                           real_Num tolerance = real_Num( 0.001 ) )
    {
        BOOST_CHECK( MathUtil<real_Num>::equals( actual, expected, tolerance ) );
    }

    template <class T>
    void requirePropertyValue( const SmartPtr<Properties> &properties, const String &name, T &value )
    {
        BOOST_REQUIRE( properties );
        BOOST_REQUIRE_MESSAGE( properties->getPropertyValue( name, value ),
                               "Expected property '" << name << "' to be present" );
    }

    SmartPtr<scene::Rigidbody> createLoadedDynamicBody( TestGuard &fixture,
                                                        SmartPtr<scene::IGameActor> &actor,
                                                        SmartPtr<scene::CollisionBox> &collisionBox )
    {
        actor = fixture.createBasicActor( false );
        BOOST_REQUIRE( actor );
        BOOST_REQUIRE( !actor->isStatic() );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );
        collisionBox->setExtents( Vector3<real_Num>( 1.0f, 1.0f, 1.0f ) );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );
        fixture.sceneManager->play();
        fixture.resetTimer();
        fixture.updatePhysics( 3 );

        BOOST_REQUIRE( rigidbody->isLoaded() );
        BOOST_REQUIRE( rigidbody->getRigidDynamic() );
        BOOST_REQUIRE( !rigidbody->getRigidStatic() );

        return rigidbody;
    }
}  // namespace

BOOST_AUTO_TEST_SUITE( ComponentPhysicsDynamicBodyTests )

BOOST_AUTO_TEST_CASE( component_physics_dynamic_body_defaults_are_safe_without_actor )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        scene::Rigidbody rigidbody;

        BOOST_CHECK( !rigidbody.isStatic() );
        BOOST_CHECK( !rigidbody.isKinematic() );
        BOOST_CHECK_CLOSE( rigidbody.getMass(), 1000.0f, 0.001f );
        checkVectorClose( rigidbody.getMassSpaceInertiaTensor(), Vector3<real_Num>::unit() );
        BOOST_CHECK_CLOSE( rigidbody.getMaxLinearVelocity(), 0.0f, 0.001f );
        BOOST_CHECK_CLOSE( rigidbody.getMaxAngularVelocity(), 0.0f, 0.001f );
        BOOST_CHECK_CLOSE( rigidbody.getLinearDamping(), 0.0f, 0.001f );
        BOOST_CHECK_CLOSE( rigidbody.getAngularDamping(), 0.05f, 0.001f );
        BOOST_CHECK( rigidbody.getUseGravity() );
        BOOST_CHECK( !rigidbody.getContinuousCollisionDetection() );
        BOOST_CHECK_CLOSE( rigidbody.getSleepThreshold(), 0.005f, 0.001f );
        BOOST_CHECK_CLOSE( rigidbody.getStabilizationThreshold(), 0.0f, 0.001f );
        BOOST_CHECK_EQUAL( rigidbody.getSolverPositionIterations(), 4u );
        BOOST_CHECK_EQUAL( rigidbody.getSolverVelocityIterations(), 1u );
        BOOST_CHECK_CLOSE( rigidbody.getContactReportThreshold(), 0.0f, 0.001f );
        BOOST_CHECK( !rigidbody.isSleeping() );

        rigidbody.setSolverPositionIterations( 0u );
        rigidbody.setSolverVelocityIterations( 1000u );
        BOOST_CHECK_EQUAL( rigidbody.getSolverPositionIterations(), 1u );
        BOOST_CHECK_EQUAL( rigidbody.getSolverVelocityIterations(), 255u );

        BOOST_CHECK_EQUAL( rigidbody.getGroupMask(), 0u );
        BOOST_CHECK_EQUAL( rigidbody.getCollisionMask(), 0u );
        BOOST_CHECK( !rigidbody.getRigidDynamic() );
        BOOST_CHECK( !rigidbody.getRigidStatic() );
        checkVectorClose( rigidbody.getLinearVelocity(), Vector3<real_Num>::zero() );
        checkVectorClose( rigidbody.getAngularVelocity(), Vector3<real_Num>::zero() );
        checkVectorClose( rigidbody.getPointVelocity( Vector3<real_Num>( 3.0f, 2.0f, 1.0f ) ),
                          Vector3<real_Num>::zero() );
        checkVectorClose( rigidbody.getLocalLinearVelocity(), Vector3<real_Num>::zero() );
        checkVectorClose( rigidbody.getLocalAngularVelocity(), Vector3<real_Num>::zero() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_dynamic_body_properties_round_trip )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        scene::Rigidbody source;
        const auto expectedMass = 42.0f;
        const auto expectedInertia = Vector3<real_Num>( 2.0f, 3.0f, 4.0f );
        const auto expectedMaxLinear = 18.0f;
        const auto expectedMaxAngular = 9.0f;
        constexpr u32 expectedGroupMask = 0x00000005u;
        constexpr u32 expectedCollisionMask = 0x0000000Au;

        source.setMass( expectedMass );
        source.setKinematic( true );
        source.setMassSpaceInertiaTensor( expectedInertia );
        source.setMaxLinearVelocity( expectedMaxLinear );
        source.setMaxAngularVelocity( expectedMaxAngular );
        source.setLinearDamping( 0.2f );
        source.setAngularDamping( 0.4f );
        source.setUseGravity( false );
        source.setContinuousCollisionDetection( true );
        source.setSleepThreshold( 0.01f );
        source.setStabilizationThreshold( 0.02f );
        source.setSolverPositionIterations( 8u );
        source.setSolverVelocityIterations( 3u );
        source.setContactReportThreshold( 2.5f );
        source.setGroupMask( expectedGroupMask );
        source.setCollisionMask( expectedCollisionMask );

        auto properties = source.getProperties();
        BOOST_REQUIRE( properties );

        f32 mass = 0.0f;
        bool kinematic = false;
        Vector3<real_Num> inertia;
        f32 maxLinear = 0.0f;
        f32 maxAngular = 0.0f;
        u32 groupMask = 0u;
        u32 collisionMask = 0u;

        requirePropertyValue( properties, scene::Rigidbody::MassStr, mass );
        requirePropertyValue( properties, scene::Rigidbody::KinematicStr, kinematic );
        requirePropertyValue( properties, scene::Rigidbody::MassSpaceInertiaTensorStr, inertia );
        requirePropertyValue( properties, scene::Rigidbody::MaxLinearVelocityStr, maxLinear );
        requirePropertyValue( properties, scene::Rigidbody::MaxAngularVelocityStr, maxAngular );
        requirePropertyValue( properties, scene::Rigidbody::GroupMaskStr, groupMask );
        requirePropertyValue( properties, scene::Rigidbody::CollisionMaskStr, collisionMask );

        BOOST_CHECK_CLOSE( mass, expectedMass, 0.001f );
        BOOST_CHECK( kinematic );
        checkVectorClose( inertia, expectedInertia );
        BOOST_CHECK_CLOSE( maxLinear, expectedMaxLinear, 0.001f );
        BOOST_CHECK_CLOSE( maxAngular, expectedMaxAngular, 0.001f );
        BOOST_CHECK_EQUAL( groupMask, expectedGroupMask );
        BOOST_CHECK_EQUAL( collisionMask, expectedCollisionMask );

        auto editorProperty = properties->getPropertyObject( scene::Rigidbody::LinearDampingStr );
        BOOST_CHECK_EQUAL( editorProperty.getAttribute( "category" ), "Damping & Limits" );
        BOOST_CHECK_EQUAL( editorProperty.getAttribute( "min" ), "0" );

        scene::Rigidbody restored;
        restored.setProperties( properties );

        BOOST_CHECK_CLOSE( restored.getMass(), expectedMass, 0.001f );
        BOOST_CHECK( restored.isKinematic() );
        checkVectorClose( restored.getMassSpaceInertiaTensor(), expectedInertia );
        BOOST_CHECK_CLOSE( restored.getMaxLinearVelocity(), expectedMaxLinear, 0.001f );
        BOOST_CHECK_CLOSE( restored.getMaxAngularVelocity(), expectedMaxAngular, 0.001f );
        BOOST_CHECK_CLOSE( restored.getLinearDamping(), 0.2f, 0.001f );
        BOOST_CHECK_CLOSE( restored.getAngularDamping(), 0.4f, 0.001f );
        BOOST_CHECK( !restored.getUseGravity() );
        BOOST_CHECK( restored.getContinuousCollisionDetection() );
        BOOST_CHECK_CLOSE( restored.getSleepThreshold(), 0.01f, 0.001f );
        BOOST_CHECK_CLOSE( restored.getStabilizationThreshold(), 0.02f, 0.001f );
        BOOST_CHECK_EQUAL( restored.getSolverPositionIterations(), 8u );
        BOOST_CHECK_EQUAL( restored.getSolverVelocityIterations(), 3u );
        BOOST_CHECK_CLOSE( restored.getContactReportThreshold(), 2.5f, 0.001f );
        BOOST_CHECK_EQUAL( restored.getGroupMask(), expectedGroupMask );
        BOOST_CHECK_EQUAL( restored.getCollisionMask(), expectedCollisionMask );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_dynamic_body_partial_properties_preserve_defaults )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        scene::Rigidbody rigidbody;
        auto properties = workphone::make_ptr<Properties>();
        properties->setProperty( scene::Rigidbody::MassStr, 12.5f );
        properties->setProperty( scene::Rigidbody::GroupMaskStr, 0x00000003u );

        rigidbody.setProperties( properties );

        BOOST_CHECK_CLOSE( rigidbody.getMass(), 12.5f, 0.001f );
        BOOST_CHECK_EQUAL( rigidbody.getGroupMask(), 0x00000003u );
        BOOST_CHECK( !rigidbody.isKinematic() );
        checkVectorClose( rigidbody.getMassSpaceInertiaTensor(), Vector3<real_Num>::unit() );
        BOOST_CHECK_CLOSE( rigidbody.getMaxLinearVelocity(), 0.0f, 0.001f );
        BOOST_CHECK_CLOSE( rigidbody.getMaxAngularVelocity(), 0.0f, 0.001f );
        BOOST_CHECK_EQUAL( rigidbody.getCollisionMask(), 0u );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_dynamic_body_mass_props_update_cache )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        scene::Rigidbody rigidbody;
        const auto expectedInertia = Vector3<real_Num>( 7.0f, 8.0f, 9.0f );

        rigidbody.setMassProps( 21.0f, expectedInertia );

        BOOST_CHECK_CLOSE( rigidbody.getMass(), 21.0f, 0.001f );
        checkVectorClose( rigidbody.getMassSpaceInertiaTensor(), expectedInertia );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_dynamic_body_load_creates_dynamic_actor_with_cached_properties )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = SmartPtr<scene::IGameActor>();
        auto collisionBox = SmartPtr<scene::CollisionBox>();
        auto rigidbody = createLoadedDynamicBody( fixture, actor, collisionBox );
        auto rigidDynamic = rigidbody->getRigidDynamic();
        BOOST_REQUIRE( rigidDynamic );

        const auto expectedInertia = Vector3<real_Num>( 4.0f, 5.0f, 6.0f );
        auto properties = workphone::make_ptr<Properties>();
        properties->setProperty( scene::Rigidbody::MassStr, 55.0f );
        properties->setProperty( scene::Rigidbody::KinematicStr, true );
        properties->setProperty( scene::Rigidbody::MassSpaceInertiaTensorStr, expectedInertia );
        properties->setProperty( scene::Rigidbody::MaxLinearVelocityStr, 22.0f );
        properties->setProperty( scene::Rigidbody::MaxAngularVelocityStr, 11.0f );
        properties->setProperty( scene::Rigidbody::LinearDampingStr, 0.15f );
        properties->setProperty( scene::Rigidbody::AngularDampingStr, 0.35f );
        properties->setProperty( scene::Rigidbody::UseGravityStr, false );
        properties->setProperty( scene::Rigidbody::ContinuousCollisionDetectionStr, true );
        properties->setProperty( scene::Rigidbody::SleepThresholdStr, 0.02f );
        properties->setProperty( scene::Rigidbody::StabilizationThresholdStr, 0.03f );
        properties->setProperty( scene::Rigidbody::SolverPositionIterationsStr, 7u );
        properties->setProperty( scene::Rigidbody::SolverVelocityIterationsStr, 2u );
        properties->setProperty( scene::Rigidbody::ContactReportThresholdStr, 1.25f );
        properties->setProperty( scene::Rigidbody::GroupMaskStr, 0x00000012u );
        properties->setProperty( scene::Rigidbody::CollisionMaskStr, 0x00000034u );

        rigidbody->setProperties( properties );
        rigidDynamic = rigidbody->getRigidDynamic();
        BOOST_REQUIRE( rigidDynamic );

        BOOST_CHECK_CLOSE( rigidbody->getMass(), 55.0f, 0.001f );
        BOOST_CHECK_CLOSE( rigidDynamic->getMass(), 55.0f, 0.001f );
        BOOST_CHECK( rigidbody->isKinematic() );
        BOOST_CHECK( rigidDynamic->isKinematic() );
        checkVectorClose( rigidbody->getMassSpaceInertiaTensor(), expectedInertia );
        checkVectorClose( rigidDynamic->getMassSpaceInertiaTensor(), expectedInertia );
        BOOST_CHECK_CLOSE( rigidbody->getMaxLinearVelocity(), 22.0f, 0.001f );
        BOOST_CHECK_CLOSE( rigidbody->getMaxAngularVelocity(), 11.0f, 0.001f );
        BOOST_CHECK_CLOSE( rigidDynamic->getMaxAngularVelocity(), 11.0f, 0.001f );
        BOOST_CHECK_CLOSE( rigidDynamic->getLinearDamping(), 0.15f, 0.001f );
        BOOST_CHECK_CLOSE( rigidDynamic->getAngularDamping(), 0.35f, 0.001f );
        BOOST_CHECK_CLOSE( rigidDynamic->getSleepThreshold(), 0.02f, 0.001f );
        BOOST_CHECK_CLOSE( rigidDynamic->getStabilizationThreshold(), 0.03f, 0.001f );
        u32 positionIterations = 0;
        u32 velocityIterations = 0;
        rigidDynamic->getSolverIterationCounts( positionIterations, velocityIterations );
        BOOST_CHECK_EQUAL( positionIterations, 7u );
        BOOST_CHECK_EQUAL( velocityIterations, 2u );
        BOOST_CHECK_CLOSE( rigidDynamic->getContactReportThreshold(), 1.25f, 0.001f );
        BOOST_CHECK( !rigidbody->getUseGravity() );
        BOOST_CHECK( rigidbody->getContinuousCollisionDetection() );
        BOOST_CHECK_EQUAL( rigidDynamic->getCollisionType(), 0x00000012u );
        BOOST_CHECK_EQUAL( rigidDynamic->getCollisionMask(), 0x00000034u );
        BOOST_CHECK( rigidbody->getRigidbodyListener() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_dynamic_body_setters_propagate_to_existing_dynamic_actor )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = SmartPtr<scene::IGameActor>();
        auto collisionBox = SmartPtr<scene::CollisionBox>();
        auto rigidbody = createLoadedDynamicBody( fixture, actor, collisionBox );
        auto rigidDynamic = rigidbody->getRigidDynamic();
        BOOST_REQUIRE( rigidDynamic );

        const auto expectedInertia = Vector3<real_Num>( 1.5f, 2.5f, 3.5f );
        rigidbody->setMass( 64.0f );
        rigidbody->setMassSpaceInertiaTensor( expectedInertia );
        rigidbody->setMaxAngularVelocity( 13.0f );
        rigidbody->setLinearDamping( 0.25f );
        rigidbody->setAngularDamping( 0.45f );
        rigidbody->setUseGravity( false );
        rigidbody->setContinuousCollisionDetection( true );
        rigidbody->setSleepThreshold( 0.04f );
        rigidbody->setStabilizationThreshold( 0.06f );
        rigidbody->setSolverPositionIterations( 9u );
        rigidbody->setSolverVelocityIterations( 4u );
        rigidbody->setContactReportThreshold( 3.0f );
        rigidbody->setGroupMask( 0x00000008u );
        rigidbody->setCollisionMask( 0x00000010u );
        rigidbody->updateKinematicState( true );

        BOOST_CHECK_CLOSE( rigidDynamic->getMass(), 64.0f, 0.001f );
        checkVectorClose( rigidDynamic->getMassSpaceInertiaTensor(), expectedInertia );
        BOOST_CHECK_CLOSE( rigidDynamic->getMaxAngularVelocity(), 13.0f, 0.001f );
        BOOST_CHECK_CLOSE( rigidDynamic->getLinearDamping(), 0.25f, 0.001f );
        BOOST_CHECK_CLOSE( rigidDynamic->getAngularDamping(), 0.45f, 0.001f );
        BOOST_CHECK_CLOSE( rigidDynamic->getSleepThreshold(), 0.04f, 0.001f );
        BOOST_CHECK_CLOSE( rigidDynamic->getStabilizationThreshold(), 0.06f, 0.001f );
        u32 positionIterations = 0;
        u32 velocityIterations = 0;
        rigidDynamic->getSolverIterationCounts( positionIterations, velocityIterations );
        BOOST_CHECK_EQUAL( positionIterations, 9u );
        BOOST_CHECK_EQUAL( velocityIterations, 4u );
        BOOST_CHECK_CLOSE( rigidDynamic->getContactReportThreshold(), 3.0f, 0.001f );
        BOOST_CHECK_EQUAL( rigidDynamic->getCollisionType(), 0x00000008u );
        BOOST_CHECK_EQUAL( rigidDynamic->getCollisionMask(), 0x00000010u );
        BOOST_CHECK( rigidDynamic->isKinematic() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_dynamic_body_velocity_accessors_use_dynamic_actor )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = SmartPtr<scene::IGameActor>();
        auto collisionBox = SmartPtr<scene::CollisionBox>();
        auto rigidbody = createLoadedDynamicBody( fixture, actor, collisionBox );

        const auto linearVelocity = Vector3<real_Num>( 3.0f, 4.0f, 5.0f );
        const auto angularVelocity = Vector3<real_Num>( 0.0f, 2.0f, 0.0f );
        rigidbody->setLinearVelocity( linearVelocity );
        rigidbody->setAngularVelocity( angularVelocity );

        checkVectorClose( rigidbody->getLinearVelocity(), linearVelocity );
        checkVectorClose( rigidbody->getAngularVelocity(), angularVelocity );
        checkVectorClose( rigidbody->getLocalLinearVelocity(), linearVelocity );
        checkVectorClose( rigidbody->getLocalAngularVelocity(), angularVelocity );

        const auto point =
            rigidbody->getTransform().getPosition() + Vector3<real_Num>( 1.0f, 0.0f, 0.0f );
        const auto expectedPointVelocity =
            linearVelocity + angularVelocity.crossProduct( Vector3<real_Num>( 1.0f, 0.0f, 0.0f ) );
        checkVectorClose( rigidbody->getPointVelocity( point ), expectedPointVelocity );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_dynamic_body_preupdate_clamps_velocity_limits )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = SmartPtr<scene::IGameActor>();
        auto collisionBox = SmartPtr<scene::CollisionBox>();
        auto rigidbody = createLoadedDynamicBody( fixture, actor, collisionBox );

        rigidbody->setLinearVelocity( Vector3<real_Num>( 10.0f, 0.0f, 0.0f ) );
        rigidbody->setAngularVelocity( Vector3<real_Num>( 0.0f, 12.0f, 0.0f ) );
        rigidbody->setMaxLinearVelocity( 2.0f );
        rigidbody->setMaxAngularVelocity( 3.0f );
        rigidbody->preUpdate();

        BOOST_CHECK_CLOSE( rigidbody->getLinearVelocity().length(), 2.0f, 0.001f );
        BOOST_CHECK_CLOSE( rigidbody->getAngularVelocity().length(), 3.0f, 0.001f );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_dynamic_body_kinematic_preupdate_sets_target )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = SmartPtr<scene::IGameActor>();
        auto collisionBox = SmartPtr<scene::CollisionBox>();
        auto rigidbody = createLoadedDynamicBody( fixture, actor, collisionBox );
        auto rigidDynamic = rigidbody->getRigidDynamic();
        BOOST_REQUIRE( rigidDynamic );

        const auto expectedPosition = Vector3<real_Num>( 12.0f, 5.0f, -7.0f );
        actor->setPosition( expectedPosition );
        rigidbody->setKinematic( true );
        rigidbody->updateKinematicState( true );
        rigidbody->preUpdate();

        Transform3<real_Num> target;
        BOOST_CHECK( rigidDynamic->getKinematicTarget( target ) );
        checkVectorClose( target.getPosition(), expectedPosition );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_dynamic_body_unload_releases_runtime_actor )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = SmartPtr<scene::IGameActor>();
        auto collisionBox = SmartPtr<scene::CollisionBox>();
        auto rigidbody = createLoadedDynamicBody( fixture, actor, collisionBox );

        BOOST_REQUIRE( rigidbody->getRigidDynamic() );
        BOOST_REQUIRE( rigidbody->getRigidbodyListener() );

        rigidbody->unload( nullptr );
        rigidbody->unload( nullptr );

        BOOST_CHECK( !rigidbody->isLoaded() );
        BOOST_CHECK( !rigidbody->getRigidDynamic() );
        BOOST_CHECK( !rigidbody->getRigidStatic() );
        BOOST_CHECK( !rigidbody->getRigidbodyListener() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_dynamic_body_actor_attachment_adds_box_shape )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = SmartPtr<scene::IGameActor>();
        auto collisionBox = SmartPtr<scene::CollisionBox>();
        auto rigidbody = createLoadedDynamicBody( fixture, actor, collisionBox );
        auto rigidDynamic = rigidbody->getRigidDynamic();
        BOOST_REQUIRE( rigidDynamic );

        BOOST_CHECK( rigidDynamic->getNumShapes() >= 1u );
        bool foundBoxShape = false;
        for( auto &shape : rigidDynamic->getShapes() )
        {
            if( workphone::dynamic_pointer_cast<physics::IBoxShape3>( shape ) )
            {
                foundBoxShape = true;
                break;
            }
        }

        BOOST_CHECK( foundBoxShape );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()
