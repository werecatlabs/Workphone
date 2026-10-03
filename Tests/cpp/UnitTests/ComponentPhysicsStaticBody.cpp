#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Graphics/IGraphicsTerrain.hpp>
#include <Workphone/Interface/Physics/IBoxShape3.hpp>
#include <Workphone/Interface/Physics/IRigidStatic3.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    void checkVectorClose( const Vector3<real_Num> &actual,
                           const Vector3<real_Num> &expected,
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

    SmartPtr<scene::Rigidbody> createLoadedStaticBody( TestGuard &fixture,
                                                      SmartPtr<scene::IGameActor> &actor,
                                                      SmartPtr<scene::CollisionBox> &collisionBox )
    {
        actor = fixture.createBasicActor( true );
        BOOST_REQUIRE( actor );
        BOOST_REQUIRE( actor->isStatic() );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );
        collisionBox->setExtents( Vector3<real_Num>( 1.0f, 2.0f, 3.0f ) );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );
        fixture.sceneManager->edit();
        fixture.resetTimer();
        fixture.updatePhysics( 3 );

        BOOST_REQUIRE( rigidbody->isLoaded() );
        BOOST_REQUIRE( rigidbody->getRigidStatic() );
        BOOST_REQUIRE( !rigidbody->getRigidDynamic() );

        return rigidbody;
    }
}  // namespace

BOOST_AUTO_TEST_SUITE( ComponentPhysicsStaticBodyTests )

BOOST_AUTO_TEST_CASE( component_physics_static_body_defaults_are_safe_without_actor )
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
        BOOST_CHECK_EQUAL( rigidbody.getGroupMask(), 0u );
        BOOST_CHECK_EQUAL( rigidbody.getCollisionMask(), 0u );
        BOOST_CHECK( !rigidbody.getRigidStatic() );
        BOOST_CHECK( !rigidbody.getRigidDynamic() );
        checkVectorClose( rigidbody.getLinearVelocity(), Vector3<real_Num>::zero() );
        checkVectorClose( rigidbody.getAngularVelocity(), Vector3<real_Num>::zero() );
        checkVectorClose( rigidbody.getPointVelocity( Vector3<real_Num>( -2.0f, 4.0f, 8.0f ) ),
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

BOOST_AUTO_TEST_CASE( component_physics_static_body_properties_round_trip )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        scene::Rigidbody source;
        const auto expectedMass = 250.0f;
        const auto expectedInertia = Vector3<real_Num>( 10.0f, 20.0f, 30.0f );
        constexpr u32 expectedGroupMask = 0x00000022u;
        constexpr u32 expectedCollisionMask = 0x00000044u;

        source.setMass( expectedMass );
        source.setKinematic( true );
        source.setMassSpaceInertiaTensor( expectedInertia );
        source.setMaxLinearVelocity( 6.0f );
        source.setMaxAngularVelocity( 7.0f );
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
        BOOST_CHECK_CLOSE( maxLinear, 6.0f, 0.001f );
        BOOST_CHECK_CLOSE( maxAngular, 7.0f, 0.001f );
        BOOST_CHECK_EQUAL( groupMask, expectedGroupMask );
        BOOST_CHECK_EQUAL( collisionMask, expectedCollisionMask );

        scene::Rigidbody restored;
        restored.setProperties( properties );

        BOOST_CHECK_CLOSE( restored.getMass(), expectedMass, 0.001f );
        BOOST_CHECK( restored.isKinematic() );
        checkVectorClose( restored.getMassSpaceInertiaTensor(), expectedInertia );
        BOOST_CHECK_CLOSE( restored.getMaxLinearVelocity(), 6.0f, 0.001f );
        BOOST_CHECK_CLOSE( restored.getMaxAngularVelocity(), 7.0f, 0.001f );
        BOOST_CHECK_EQUAL( restored.getGroupMask(), expectedGroupMask );
        BOOST_CHECK_EQUAL( restored.getCollisionMask(), expectedCollisionMask );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_static_body_partial_properties_preserve_defaults )
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
        properties->setProperty( scene::Rigidbody::CollisionMaskStr, 0x0000000Fu );
        properties->setProperty( scene::Rigidbody::KinematicStr, true );

        rigidbody.setProperties( properties );

        BOOST_CHECK( rigidbody.isKinematic() );
        BOOST_CHECK_EQUAL( rigidbody.getCollisionMask(), 0x0000000Fu );
        BOOST_CHECK_CLOSE( rigidbody.getMass(), 1000.0f, 0.001f );
        checkVectorClose( rigidbody.getMassSpaceInertiaTensor(), Vector3<real_Num>::unit() );
        BOOST_CHECK_EQUAL( rigidbody.getGroupMask(), 0u );
        BOOST_CHECK_CLOSE( rigidbody.getMaxLinearVelocity(), 0.0f, 0.001f );
        BOOST_CHECK_CLOSE( rigidbody.getMaxAngularVelocity(), 0.0f, 0.001f );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_static_body_mass_props_update_cache )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        scene::Rigidbody rigidbody;
        const auto expectedInertia = Vector3<real_Num>( 11.0f, 12.0f, 13.0f );

        rigidbody.setMassProps( 500.0f, expectedInertia );

        BOOST_CHECK_CLOSE( rigidbody.getMass(), 500.0f, 0.001f );
        checkVectorClose( rigidbody.getMassSpaceInertiaTensor(), expectedInertia );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_static_body_load_creates_static_actor_only )
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
        auto rigidbody = createLoadedStaticBody( fixture, actor, collisionBox );

        BOOST_CHECK( actor->isStatic() );
        BOOST_CHECK( rigidbody->isStatic() );
        BOOST_CHECK( rigidbody->getRigidStatic() );
        BOOST_CHECK( !rigidbody->getRigidDynamic() );
        BOOST_CHECK( rigidbody->getRigidbodyListener() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_static_body_runtime_properties_propagate_to_static_actor )
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
        auto rigidbody = createLoadedStaticBody( fixture, actor, collisionBox );
        auto rigidStatic = rigidbody->getRigidStatic();
        BOOST_REQUIRE( rigidStatic );

        const auto expectedInertia = Vector3<real_Num>( 3.0f, 6.0f, 9.0f );
        auto properties = workphone::make_ptr<Properties>();
        properties->setProperty( scene::Rigidbody::MassStr, 900.0f );
        properties->setProperty( scene::Rigidbody::MassSpaceInertiaTensorStr, expectedInertia );
        properties->setProperty( scene::Rigidbody::GroupMaskStr, 0x00000018u );
        properties->setProperty( scene::Rigidbody::CollisionMaskStr, 0x00000081u );

        rigidbody->setProperties( properties );

        BOOST_CHECK_CLOSE( rigidbody->getMass(), 900.0f, 0.001f );
        BOOST_CHECK_CLOSE( rigidStatic->getMass(), 900.0f, 0.001f );
        checkVectorClose( rigidbody->getMassSpaceInertiaTensor(), expectedInertia );
        checkVectorClose( rigidStatic->getMassSpaceInertiaTensor(), expectedInertia );
        BOOST_CHECK_EQUAL( rigidStatic->getCollisionType(), 0x00000018u );
        BOOST_CHECK_EQUAL( rigidStatic->getCollisionMask(), 0x00000081u );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_static_body_setters_propagate_to_existing_static_actor )
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
        auto rigidbody = createLoadedStaticBody( fixture, actor, collisionBox );
        auto rigidStatic = rigidbody->getRigidStatic();
        BOOST_REQUIRE( rigidStatic );

        const auto expectedInertia = Vector3<real_Num>( 5.0f, 10.0f, 15.0f );
        rigidbody->setMass( 750.0f );
        rigidbody->setMassSpaceInertiaTensor( expectedInertia );
        rigidbody->setGroupMask( 0x00000006u );
        rigidbody->setCollisionMask( 0x00000060u );

        BOOST_CHECK_CLOSE( rigidStatic->getMass(), 750.0f, 0.001f );
        checkVectorClose( rigidStatic->getMassSpaceInertiaTensor(), expectedInertia );
        BOOST_CHECK_EQUAL( rigidStatic->getCollisionType(), 0x00000006u );
        BOOST_CHECK_EQUAL( rigidStatic->getCollisionMask(), 0x00000060u );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_static_body_initial_transform_applies_to_static_actor )
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
        const auto expectedPosition = Vector3<real_Num>( 25.0f, -5.0f, 12.0f );
        actor->setPosition( expectedPosition );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );
        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        BOOST_REQUIRE( collisionBox );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );
        fixture.sceneManager->edit();
        fixture.updatePhysics( 3 );

        auto rigidStatic = rigidbody->getRigidStatic();
        BOOST_REQUIRE( rigidStatic );
        checkVectorClose( rigidStatic->getTransform().getPosition(), expectedPosition );
        checkVectorClose( rigidbody->getTransform().getPosition(), expectedPosition );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_static_body_actor_attachment_adds_box_shape )
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
        auto rigidbody = createLoadedStaticBody( fixture, actor, collisionBox );
        auto rigidStatic = rigidbody->getRigidStatic();
        BOOST_REQUIRE( rigidStatic );

        BOOST_CHECK( rigidStatic->getNumShapes() >= 1u );
        bool foundBoxShape = false;
        for( auto &shape : rigidStatic->getShapes() )
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

BOOST_AUTO_TEST_CASE( component_physics_static_body_unload_releases_runtime_actor )
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
        auto rigidbody = createLoadedStaticBody( fixture, actor, collisionBox );

        BOOST_REQUIRE( rigidbody->getRigidStatic() );
        BOOST_REQUIRE( rigidbody->getRigidbodyListener() );

        rigidbody->unload( nullptr );
        rigidbody->unload( nullptr );

        BOOST_CHECK( !rigidbody->isLoaded() );
        BOOST_CHECK( !rigidbody->getRigidStatic() );
        BOOST_CHECK( !rigidbody->getRigidDynamic() );
        BOOST_CHECK( !rigidbody->getRigidbodyListener() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()
