#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/TransformSystem.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

using namespace workphone;

namespace
{
    void setupPrimaryThread()
    {
        Thread::setCurrentThreadId( Thread::ThreadId::Primary );
        Thread::setTaskFlags( std::numeric_limits<u32>::max() );
    }

    void flushTaskUpdates( SmartPtr<ITaskManager> taskManager, size_t iterations = 10 )
    {
        if( !taskManager )
        {
            return;
        }

        for( size_t i = 0; i < iterations; ++i )
        {
            taskManager->update();
            Thread::yield();
        }
    }

    void checkVectorClose( const Vector3<real_Num> &actual, const Vector3<real_Num> &expected )
    {
        BOOST_CHECK_MESSAGE( MathUtil<real_Num>::equals( actual, expected, real_Num( 0.001 ) ),
                             "actual=(" << actual.X() << ", " << actual.Y() << ", " << actual.Z()
                                        << ") expected=(" << expected.X() << ", " << expected.Y() << ", "
                                        << expected.Z() << ")" );
    }

    void checkQuaternionClose( const Quaternion<real_Num> &actual, const Quaternion<real_Num> &expected )
    {
        BOOST_CHECK( MathUtil<real_Num>::equals( actual.w, expected.w, real_Num( 0.001 ) ) );
        BOOST_CHECK( MathUtil<real_Num>::equals( actual.x, expected.x, real_Num( 0.001 ) ) );
        BOOST_CHECK( MathUtil<real_Num>::equals( actual.y, expected.y, real_Num( 0.001 ) ) );
        BOOST_CHECK( MathUtil<real_Num>::equals( actual.z, expected.z, real_Num( 0.001 ) ) );
    }

    void checkTransformClose( const Transform3<real_Num> &actual, const Transform3<real_Num> &expected )
    {
        checkVectorClose( actual.getPosition(), expected.getPosition() );
        checkVectorClose( actual.getScale(), expected.getScale() );
        checkQuaternionClose( actual.getOrientation(), expected.getOrientation() );
    }

    SmartPtr<scene::IGameActor> createSceneActor( TestGuard &guard, const String &name )
    {
        BOOST_CHECK( guard.sceneManager );
        BOOST_CHECK( guard.scene );
        if( !guard.sceneManager || !guard.scene )
        {
            return nullptr;
        }

        auto actor = guard.sceneManager->createActor();
        BOOST_CHECK( actor );
        if( !actor )
        {
            return nullptr;
        }

        actor->setName( name );
        guard.scene->addActor( actor );
        return actor;
    }

    struct ActorTransformFixture : TestGuard
    {
        ActorTransformFixture()
        {
            setupPrimaryThread();
            BOOST_REQUIRE( applicationManager );
            BOOST_REQUIRE( sceneManager );
            BOOST_REQUIRE( scene );

            scene->clear();
            sceneManager->play();
            flushTaskUpdates( taskManager );
        }

        ~ActorTransformFixture()
        {
            if( scene )
            {
                scene->clear();
            }

            flushTaskUpdates( taskManager, 3 );
        }
    };
}  // namespace

BOOST_FIXTURE_TEST_SUITE( ActorTransformTests, ActorTransformFixture )

BOOST_AUTO_TEST_CASE( transform_system_uses_generation_checked_soa_slots )
{
    auto &transformSystem = scene::TransformSystem::instance();
    const auto initialActiveCount = transformSystem.getActiveCount();

    const auto first = transformSystem.createTransform();
    const auto second = transformSystem.createTransform();
    const auto firstPosition = Vector3<real_Num>( 1.0, 2.0, 3.0 );
    const auto secondScale = Vector3<real_Num>( 4.0, 5.0, 6.0 );

    transformSystem.setLocalPosition( first, firstPosition );
    transformSystem.setWorldScale( second, secondScale );

    checkVectorClose( transformSystem.getLocalPosition( first ), firstPosition );
    checkVectorClose( transformSystem.getWorldScale( second ), secondScale );
    transformSystem.readData( [&]( const scene::TransformSystem::ConstDataView &data ) {
        BOOST_REQUIRE( first.index < data.slotCount );
        BOOST_REQUIRE( second.index < data.slotCount );
        BOOST_CHECK( data.activeSlots[first.index] != 0 );
        BOOST_CHECK( data.localPositions != data.worldPositions );
        checkVectorClose( data.localPositions[first.index], firstPosition );
        checkVectorClose( data.worldScales[second.index], secondScale );
    } );
    BOOST_CHECK_EQUAL( transformSystem.getActiveCount(), initialActiveCount + 2 );

    transformSystem.destroyTransform( first );
    BOOST_CHECK( !transformSystem.contains( first ) );

    const auto replacement = transformSystem.createTransform();
    BOOST_CHECK_EQUAL( replacement.index, first.index );
    BOOST_CHECK( replacement.generation != first.generation );
    checkVectorClose( transformSystem.getLocalPosition( replacement ), Vector3<real_Num>::zero() );
    checkVectorClose( transformSystem.getWorldScale( second ), secondScale );

    transformSystem.destroyTransform( replacement );
    transformSystem.destroyTransform( second );
    BOOST_CHECK_EQUAL( transformSystem.getActiveCount(), initialActiveCount );
}

BOOST_AUTO_TEST_CASE( actor_has_loaded_transform_after_creation )
{
    auto actor = createSceneActor( *this, "TransformOwner" );
    auto transform = actor->getTransform();

    BOOST_REQUIRE( transform );
    BOOST_CHECK( transform->isLoaded() );
    BOOST_CHECK( transform->getActor() == actor );
    BOOST_CHECK( transform->getActorPtr() == actor.get() );
    checkVectorClose( actor->getLocalPosition(), Vector3<real_Num>::zero() );
    checkVectorClose( actor->getPosition(), Vector3<real_Num>::zero() );
    checkVectorClose( actor->getScale(), Vector3<real_Num>::unit() );
}

BOOST_AUTO_TEST_CASE( actor_world_position_round_trips_and_updates_local_without_parent )
{
    auto actor = createSceneActor( *this, "WorldPositionActor" );
    const auto position = Vector3<real_Num>( 10.0, -20.0, 30.0 );

    actor->setPosition( position );
    flushTaskUpdates( taskManager );

    checkVectorClose( actor->getPosition(), position );
    checkVectorClose( actor->getLocalPosition(), position );
    BOOST_CHECK( !actor->getTransform()->isLocalDirty() );
}

BOOST_AUTO_TEST_CASE( actor_local_position_round_trips_and_updates_world_without_parent )
{
    auto actor = createSceneActor( *this, "LocalPositionActor" );
    const auto localPosition = Vector3<real_Num>( -4.0, 5.0, 6.5 );

    actor->setLocalPosition( localPosition );
    flushTaskUpdates( taskManager );

    checkVectorClose( actor->getLocalPosition(), localPosition );
    checkVectorClose( actor->getPosition(), localPosition );
    BOOST_CHECK( !actor->getTransform()->isDirty() );
}

BOOST_AUTO_TEST_CASE( actor_world_transform_round_trips_position_rotation_and_scale )
{
    auto actor = createSceneActor( *this, "WorldTransformActor" );
    Transform3<real_Num> transform(
        Vector3<real_Num>( 1.0, 2.0, 3.0 ),
        Quaternion<real_Num>::angleAxis( Math<real_Num>::pi() / real_Num( 2.0 ),
                                         Vector3<real_Num>::unitY() ),
        Vector3<real_Num>( 2.0, 3.0, 4.0 ) );

    auto actorTransform = actor->getTransform();
    BOOST_REQUIRE( actorTransform );
    actorTransform->setWorldTransform( transform );
    actorTransform->setLocalDirty( true );
    actor->updateTransform();
    flushTaskUpdates( taskManager );

    checkTransformClose( actor->getWorldTransform(), transform );
    checkVectorClose( actor->getPosition(), transform.getPosition() );
    checkVectorClose( actor->getScale(), transform.getScale() );
    checkQuaternionClose( actor->getOrientation(), transform.getOrientation() );
}

BOOST_AUTO_TEST_CASE( actor_local_transform_round_trips_position_rotation_and_scale )
{
    auto actor = createSceneActor( *this, "LocalTransformActor" );
    Transform3<real_Num> transform(
        Vector3<real_Num>( -3.0, 2.0, 7.0 ),
        Quaternion<real_Num>::angleAxis( Math<real_Num>::pi() / real_Num( 4.0 ),
                                         Vector3<real_Num>::unitZ() ),
        Vector3<real_Num>( 0.5, 1.5, 2.5 ) );

    auto actorTransform = actor->getTransform();
    BOOST_REQUIRE( actorTransform );
    actorTransform->setLocalTransform( transform );
    actorTransform->setDirty( true );
    actor->updateTransform();
    flushTaskUpdates( taskManager );

    checkTransformClose( actor->getLocalTransform(), transform );
    checkTransformClose( actor->getWorldTransform(), transform );
}

BOOST_AUTO_TEST_CASE( actor_scale_accepts_uniform_nonuniform_and_small_values )
{
    auto actor = createSceneActor( *this, "ScaleActor" );
    const Vector3<real_Num> scales[] = {
        Vector3<real_Num>( 2.0, 2.0, 2.0 ),
        Vector3<real_Num>( 1.0, 2.0, 3.0 ),
        Vector3<real_Num>( 0.001, 0.002, 0.003 ),
        Vector3<real_Num>( 100.0, 50.0, 25.0 ),
    };

    for( const auto &scale : scales )
    {
        actor->setScale( scale );
        flushTaskUpdates( taskManager, 3 );
        checkVectorClose( actor->getScale(), scale );
        checkVectorClose( actor->getLocalScale(), scale );
    }
}

BOOST_AUTO_TEST_CASE( actor_orientation_round_trips_quaternion )
{
    auto actor = createSceneActor( *this, "OrientationActor" );
    auto rotation = Quaternion<real_Num>::angleAxis( Math<real_Num>::pi() / real_Num( 2.0 ),
                                                     Vector3<real_Num>::unitY() );

    actor->setOrientation( rotation );
    flushTaskUpdates( taskManager );

    checkQuaternionClose( actor->getOrientation(), rotation );
    checkQuaternionClose( actor->getLocalOrientation(), rotation );
}

BOOST_AUTO_TEST_CASE( actor_rotation_degrees_round_trip )
{
    auto actor = createSceneActor( *this, "RotationActor" );
    const auto rotation = Vector3<real_Num>( 15.0, 30.0, 45.0 );

    actor->setRotation( rotation );
    flushTaskUpdates( taskManager );

    checkVectorClose( actor->getRotation(), rotation );
    checkVectorClose( actor->getLocalRotation(), rotation );
}

BOOST_AUTO_TEST_CASE( child_world_position_is_parent_world_plus_local_position )
{
    auto parent = createSceneActor( *this, "ParentActor" );
    auto child = createSceneActor( *this, "ChildActor" );

    parent->addChild( child );
    parent->setPosition( Vector3<real_Num>( 10.0, 20.0, 30.0 ) );
    child->setLocalPosition( Vector3<real_Num>( 1.0, 2.0, 3.0 ) );
    flushTaskUpdates( taskManager );

    checkVectorClose( child->getPosition(), Vector3<real_Num>( 11.0, 22.0, 33.0 ) );
    checkVectorClose( child->getLocalPosition(), Vector3<real_Num>( 1.0, 2.0, 3.0 ) );
}

BOOST_AUTO_TEST_CASE( setting_child_world_position_recomputes_local_position )
{
    auto parent = createSceneActor( *this, "WorldParentActor" );
    auto child = createSceneActor( *this, "WorldChildActor" );

    parent->addChild( child );
    parent->setPosition( Vector3<real_Num>( 100.0, 0.0, 0.0 ) );
    child->setPosition( Vector3<real_Num>( 125.0, 5.0, -2.0 ) );
    flushTaskUpdates( taskManager );

    checkVectorClose( child->getPosition(), Vector3<real_Num>( 125.0, 5.0, -2.0 ) );
    checkVectorClose( child->getLocalPosition(), Vector3<real_Num>( 25.0, 5.0, -2.0 ) );
}

BOOST_AUTO_TEST_CASE( moving_parent_preserves_child_local_transforms )
{
    auto parent = createSceneActor( *this, "VehicleRoot" );
    auto chassis = sceneManager->createActor();
    auto wheel = sceneManager->createActor();
    auto wheelMesh = sceneManager->createActor();
    parent->addChild( chassis );
    parent->addChild( wheel );
    wheel->addChild( wheelMesh );
    chassis->setLocalScale( Vector3F( 1.6f, 0.6f, 4.405f ) );
    wheel->setLocalPosition( Vector3F( -0.905f, 0.0f, -1.265f ) );
    wheelMesh->setLocalScale( Vector3F( 0.25f, 0.7f, 0.7f ) );
    const auto chassisLocal = chassis->getLocalTransform();
    const auto wheelLocalPosition = wheel->getLocalPosition();
    const auto wheelMeshLocal = wheelMesh->getLocalTransform();

    for( u32 i = 0; i < 8; ++i )
    {
        parent->setPosition( Vector3F( 2.0f * i, 0.5f, -5.0f * i ) );
        parent->setOrientation( QuaternionF::eulerDegrees( 0.0f, 15.0f * i, 0.0f ) );
        wheel->setLocalOrientation( QuaternionF::eulerDegrees( 35.0f * i, 0.0f, 0.0f ) );
        parent->updateTransform();
        checkTransformClose( chassis->getLocalTransform(), chassisLocal );
        checkVectorClose( wheel->getLocalPosition(), wheelLocalPosition );
        checkTransformClose( wheelMesh->getLocalTransform(), wheelMeshLocal );
        Transform3<real_Num> expectedChassis, expectedWheel, expectedWheelMesh;
        expectedChassis.transformFromParent( parent->getWorldTransform(), chassisLocal );
        expectedWheel.transformFromParent( parent->getWorldTransform(), wheel->getLocalTransform() );
        expectedWheelMesh.transformFromParent( expectedWheel, wheelMeshLocal );
        checkTransformClose( chassis->getWorldTransform(), expectedChassis );
        checkTransformClose( wheelMesh->getWorldTransform(), expectedWheelMesh );
    }
}

BOOST_AUTO_TEST_CASE( dirty_flags_cascade_from_parent_transform_changes )
{
    auto parent = createSceneActor( *this, "DirtyParentActor" );
    auto child = createSceneActor( *this, "DirtyChildActor" );

    parent->addChild( child );
    auto parentTransform = parent->getTransform();
    auto childTransform = child->getTransform();
    BOOST_REQUIRE( parentTransform );
    BOOST_REQUIRE( childTransform );

    parentTransform->setDirty( true );

    BOOST_CHECK( parentTransform->isDirty() );
    BOOST_CHECK( childTransform->isDirty() );

    parent->updateTransform();

    BOOST_CHECK( !parentTransform->isDirty() );
    BOOST_CHECK( !childTransform->isDirty() );
}

BOOST_AUTO_TEST_CASE( transform_references_block_direct_mutation_until_released )
{
    auto actor = createSceneActor( *this, "ReferencedTransformActor" );
    auto transform = actor->getTransform();
    BOOST_REQUIRE( transform );

    const auto original = actor->getPosition();
    transform->addTransformReference();
    transform->setPosition( Vector3<real_Num>( 9.0, 9.0, 9.0 ) );
    checkVectorClose( transform->getPosition(), original );

    transform->removeTransformReference();
    transform->setPosition( Vector3<real_Num>( 9.0, 9.0, 9.0 ) );
    checkVectorClose( transform->getPosition(), Vector3<real_Num>( 9.0, 9.0, 9.0 ) );
}

BOOST_AUTO_TEST_CASE( rapid_position_updates_keep_last_value )
{
    auto actor = createSceneActor( *this, "RapidUpdateActor" );

    for( size_t i = 0; i < 100; ++i )
    {
        actor->setPosition( Vector3<real_Num>( static_cast<real_Num>( i ),
                                               static_cast<real_Num>( i * 2 ),
                                               static_cast<real_Num>( i * 3 ) ) );
    }

    flushTaskUpdates( taskManager, 20 );

    checkVectorClose( actor->getPosition(), Vector3<real_Num>( 99.0, 198.0, 297.0 ) );
}

BOOST_AUTO_TEST_CASE( component_removal_preserves_remaining_components )
{
    auto actor = createSceneActor( *this, "ComponentActor" );
    auto meshComponent = actor->addComponent<scene::Mesh>();
    auto meshRenderer = actor->addComponent<scene::MeshRenderer>();

    BOOST_REQUIRE( meshComponent );
    BOOST_REQUIRE( meshRenderer );
    BOOST_CHECK_EQUAL( actor->getComponents().size(), 2 );

    actor->removeComponentInstance( meshComponent );
    flushTaskUpdates( taskManager );

    BOOST_CHECK( !actor->hasComponent<scene::Mesh>() );
    BOOST_CHECK( actor->hasComponent<scene::MeshRenderer>() );
    BOOST_CHECK_EQUAL( actor->getComponents().size(), 1 );
    BOOST_CHECK( meshRenderer->isValid() );
}

BOOST_AUTO_TEST_SUITE_END()
