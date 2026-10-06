#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Graphics/GraphicsSceneNode.hpp>
#include <boost/test/unit_test.hpp>
#include <thread>

using namespace workphone;

namespace
{
    void setupPrimaryThread()
    {
        Thread::setCurrentThreadId( Thread::ThreadId::Primary );
        Thread::setTaskFlags( std::numeric_limits<u32>::max() );
    }

    void flushTaskUpdates( SmartPtr<ITaskManager> taskManager, size_t iterations = 5 )
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
        // Allow sub-millimetre floating-point error from timestamp interpolation.
        BOOST_CHECK( MathUtil<real_Num>::equals( actual, expected, real_Num( 0.0001 ) ) );
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

    Transform3<real_Num> makeTransform( const Vector3<real_Num> &position )
    {
        return Transform3<real_Num>( position, Quaternion<real_Num>::identity(),
                                     Vector3<real_Num>::unit() );
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

    class SmoothTransformCapture : public scene::Component
    {
    public:
        void updateTransform( const Transform3<real_Num> &value ) override
        {
            sampled = true;
            pose = value;
        }
        bool sampled = false;
        Transform3<real_Num> pose;
    };

    class PoseNode : public render::GraphicsSceneNode
    {
    public:
        void setTransform( const Transform3<real_Num> &value ) override { pose = value; }
        Transform3<real_Num> getTransform() const override { return pose; }
        Transform3<real_Num> pose;
    };

    class PlayingRenderer : public scene::Renderer
    {
    public:
        State getState() const override { return State::Play; }
    };

    struct ActorSmoothMotionFixture : TestGuard
    {
        ActorSmoothMotionFixture()
        {
            setupPrimaryThread();
            BOOST_REQUIRE( applicationManager );
            BOOST_REQUIRE( sceneManager );
            BOOST_REQUIRE( scene );

            scene->clear();
            sceneManager->play();
            flushTaskUpdates( taskManager );
        }

        ~ActorSmoothMotionFixture()
        {
            if( scene )
            {
                scene->clear();
            }

            flushTaskUpdates( taskManager, 3 );
        }
    };
}  // namespace

BOOST_FIXTURE_TEST_SUITE( ActorSmoothMotionTests, ActorSmoothMotionFixture )

BOOST_AUTO_TEST_CASE( smooth_motion_is_disabled_by_default )
{
    auto actor = createSceneActor( *this, "DefaultSmoothActor" );
    BOOST_REQUIRE( actor );

    auto transform = actor->getTransform();
    BOOST_REQUIRE( transform );

    BOOST_CHECK( !actor->isSmoothMotion() );
    BOOST_CHECK( !transform->getSmoothMotion() );
}

BOOST_AUTO_TEST_CASE( set_smooth_motion_updates_actor_and_transform_flags )
{
    auto actor = createSceneActor( *this, "FlagPropagationActor" );
    BOOST_REQUIRE( actor );
    auto transform = actor->getTransform();
    BOOST_REQUIRE( transform );

    actor->setSmoothMotion( true );

    BOOST_CHECK( actor->isSmoothMotion() );
    BOOST_CHECK( transform->getSmoothMotion() );

    actor->setSmoothMotion( false );

    BOOST_CHECK( !actor->isSmoothMotion() );
    BOOST_CHECK( !transform->getSmoothMotion() );
}

BOOST_AUTO_TEST_CASE( transform_smooth_motion_flag_can_be_toggled_directly )
{
    auto actor = createSceneActor( *this, "TransformToggleActor" );
    BOOST_REQUIRE( actor );
    auto transform = actor->getTransform();
    BOOST_REQUIRE( transform );

    transform->setSmoothMotion( true );
    BOOST_CHECK( transform->getSmoothMotion() );
    BOOST_CHECK( !actor->isSmoothMotion() );

    transform->setSmoothMotion( false );
    BOOST_CHECK( !transform->getSmoothMotion() );
}

BOOST_AUTO_TEST_CASE( smooth_motion_survives_transform_updates )
{
    auto actor = createSceneActor( *this, "MovingSmoothActor" );
    BOOST_REQUIRE( actor );
    auto transform = actor->getTransform();
    BOOST_REQUIRE( transform );

    actor->setSmoothMotion( true );
    actor->setPosition( Vector3<real_Num>( 10.0, 20.0, 30.0 ) );
    actor->setScale( Vector3<real_Num>( 2.0, 3.0, 4.0 ) );
    flushTaskUpdates( taskManager );

    BOOST_CHECK( actor->isSmoothMotion() );
    BOOST_CHECK( transform->getSmoothMotion() );
    checkVectorClose( actor->getPosition(), Vector3<real_Num>( 10.0, 20.0, 30.0 ) );
    checkVectorClose( actor->getScale(), Vector3<real_Num>( 2.0, 3.0, 4.0 ) );
}

BOOST_AUTO_TEST_CASE( transform_frame_time_updates_when_dirty )
{
    auto actor = createSceneActor( *this, "FrameTimeActor" );
    BOOST_REQUIRE( actor );
    auto transform = actor->getTransform();
    BOOST_REQUIRE( transform );

    transform->setFrameTime( time_interval( 0 ) );
    transform->setFrameDeltaTime( time_interval( 0 ) );
    transform->setDirty( true );

    BOOST_CHECK_GE( transform->getFrameTime(), time_interval( 0 ) );
    BOOST_CHECK_GE( transform->getFrameDeltaTime(), time_interval( 0 ) );
    BOOST_CHECK( transform->isDirty() );

    actor->updateTransform();
    BOOST_CHECK( !transform->isDirty() );
}

BOOST_AUTO_TEST_CASE( transform_state_lookup_returns_latest_state )
{
    auto actor = createSceneActor( *this, "LatestStateActor" );
    BOOST_REQUIRE( actor );
    auto handle = actor->getHandle();
    BOOST_REQUIRE( handle );

    const auto id = handle->getInstanceId();
    const auto task = Thread::getCurrentTask();
    const auto expected = makeTransform( Vector3<real_Num>( 1.0, 2.0, 3.0 ) );

    sceneManager->addTransformState( id, time_interval( 10 ), expected );

    Transform3<real_Num> actual;
    BOOST_CHECK( sceneManager->getTransformState( id, time_interval( 10 ), time_interval(1), actual, task ) );
    checkTransformClose( actual, expected );
}

BOOST_AUTO_TEST_CASE( transform_state_lookup_interpolates_between_saved_states )
{
    auto actor = createSceneActor( *this, "InterpolatedStateActor" );
    BOOST_REQUIRE( actor );
    auto handle = actor->getHandle();
    BOOST_REQUIRE( handle );

    const auto id = handle->getInstanceId();
    const auto task = Thread::getCurrentTask();

    sceneManager->addTransformState( id, time_interval( 0 ),
                                     makeTransform( Vector3<real_Num>( 0.0, 0.0, 0.0 ) ) );
    sceneManager->addTransformState( id, time_interval( 10 ),
                                     makeTransform( Vector3<real_Num>( 10.0, 20.0, 30.0 ) ) );

    Transform3<real_Num> actual;
    BOOST_CHECK( sceneManager->getTransformState( id, time_interval( 5 ), time_interval(1), actual, task ) );
    checkVectorClose( actual.getPosition(), Vector3<real_Num>( 5.0, 10.0, 15.0 ) );
}

BOOST_AUTO_TEST_CASE( transform_state_lookup_extrapolates_with_linear_velocity )
{
    auto actor = createSceneActor( *this, "VelocityStateActor" );
    BOOST_REQUIRE( actor );
    auto handle = actor->getHandle();
    BOOST_REQUIRE( handle );

    const auto id = handle->getInstanceId();
    const auto task = Thread::getCurrentTask();

    sceneManager->addTransformState( id, time_interval( 10 ),
                                     makeTransform( Vector3<real_Num>( 1.0, 2.0, 3.0 ) ),
                                     Vector3<real_Num>( 2.0, 0.0, -1.0 ), Vector3<real_Num>::zero() );

    Transform3<real_Num> actual;
    BOOST_CHECK( sceneManager->getTransformState( id, time_interval( 10.05 ), time_interval(1), actual, task ) );
    checkVectorClose( actual.getPosition(), Vector3<real_Num>( 1.1, 2.0, 2.95 ) );
}

BOOST_AUTO_TEST_CASE( transform_state_lookup_returns_false_for_unknown_id )
{
    Transform3<real_Num> actual;

    BOOST_CHECK( !sceneManager->getTransformState( std::numeric_limits<u32>::max(), time_interval( 0 ), time_interval(1),
                                                   actual, Thread::getCurrentTask() ) );
}

BOOST_AUTO_TEST_CASE( transform_state_lookup_extrapolates_latest_of_multiple_physics_states )
{
    auto actor = createSceneActor( *this, "MultipleVelocityStatesActor" );
    BOOST_REQUIRE( actor );
    const auto id = actor->getHandle()->getInstanceId();
    const auto task = Thread::getCurrentTask();
    const auto velocity = Vector3<real_Num>( 2.0, 0.0, -1.0 );
    sceneManager->addTransformState( id, time_interval( 9 ),
                                     makeTransform( Vector3<real_Num>( 1.0, 2.0, 3.0 ) ),
                                     velocity, Vector3<real_Num>::zero() );
    const auto latest = makeTransform( Vector3<real_Num>( 3.0, 2.0, 2.0 ) );
    sceneManager->addTransformState( id, time_interval( 10 ), latest, velocity,
                                     Vector3<real_Num>::zero() );

    Transform3<real_Num> actual;
    BOOST_REQUIRE( sceneManager->getTransformState( id, time_interval( 10 ), time_interval(1), actual, task ) );
    checkTransformClose( actual, latest );
    for( u32 query = 0; query < 2; ++query )
    {
        BOOST_REQUIRE( sceneManager->getTransformState( id, time_interval( 10.05 ), time_interval(1), actual, task ) );
        checkVectorClose( actual.getPosition(), Vector3<real_Num>( 3.1, 2.0, 1.95 ) );
    }
    BOOST_REQUIRE( sceneManager->getTransformState( id, time_interval( 10 ), time_interval(1), actual, task ) );
    checkTransformClose( actual, latest );
}

BOOST_AUTO_TEST_CASE( duplicate_transform_state_time_keeps_first_value )
{
    auto actor = createSceneActor( *this, "DuplicateStateActor" );
    BOOST_REQUIRE( actor );
    auto handle = actor->getHandle();
    BOOST_REQUIRE( handle );

    const auto id = handle->getInstanceId();
    const auto task = Thread::getCurrentTask();
    const auto first = makeTransform( Vector3<real_Num>( 3.0, 4.0, 5.0 ) );

    sceneManager->addTransformState( id, time_interval( 7 ), first );
    sceneManager->addTransformState( id, time_interval( 7 ),
                                     makeTransform( Vector3<real_Num>( 99.0, 99.0, 99.0 ) ) );

    Transform3<real_Num> actual;
    BOOST_CHECK( sceneManager->getTransformState( id, time_interval( 7 ), time_interval(1), actual, task ) );
    checkTransformClose( actual, first );
}

BOOST_AUTO_TEST_CASE( smooth_motion_does_not_break_parent_child_transform_updates )
{
    auto parent = createSceneActor( *this, "SmoothParentActor" );
    auto child = createSceneActor( *this, "SmoothChildActor" );
    BOOST_REQUIRE( parent );
    BOOST_REQUIRE( child );

    parent->addChild( child );
    parent->setSmoothMotion( true );
    child->setSmoothMotion( true );

    parent->setPosition( Vector3<real_Num>( 10.0, 0.0, 0.0 ) );
    child->setLocalPosition( Vector3<real_Num>( 2.0, 3.0, 4.0 ) );
    flushTaskUpdates( taskManager );

    BOOST_CHECK( parent->isSmoothMotion() );
    BOOST_CHECK( child->isSmoothMotion() );
    checkVectorClose( child->getPosition(), Vector3<real_Num>( 12.0, 3.0, 4.0 ) );
}

BOOST_AUTO_TEST_CASE( disabling_smooth_motion_after_updates_preserves_current_transform )
{
    auto actor = createSceneActor( *this, "DisableSmoothActor" );
    BOOST_REQUIRE( actor );
    const auto position = Vector3<real_Num>( -8.0, 4.0, 12.0 );

    actor->setSmoothMotion( true );
    actor->setPosition( position );
    flushTaskUpdates( taskManager );

    actor->setSmoothMotion( false );
    flushTaskUpdates( taskManager );

    BOOST_CHECK( !actor->isSmoothMotion() );
    BOOST_CHECK( !actor->getTransform()->getSmoothMotion() );
    checkVectorClose( actor->getPosition(), position );
}

BOOST_AUTO_TEST_CASE( sampling_is_independent_of_render_delta_and_query_order )
{
    auto actor = createSceneActor( *this, "StatelessSampling" );
    const auto id = actor->getHandle()->getInstanceId();
    const auto task = Thread::getCurrentTask();
    sceneManager->addTransformState( id, 10, makeTransform( Vector3<real_Num>::zero() ) );
    sceneManager->addTransformState( id, 10.1, makeTransform( Vector3<real_Num>( 1, 0, 0 ) ) );
    Transform3<real_Num> actual;
    for( auto dt : { 0.0, 0.001, 0.5 } )
    {
        BOOST_REQUIRE( sceneManager->getTransformState( id, 10.05, dt, actual, task ) );
        checkVectorClose( actual.getPosition(), Vector3<real_Num>( .5, 0, 0 ) );
        BOOST_REQUIRE( sceneManager->getTransformState( id, 10.15, dt, actual, task ) );
        checkVectorClose( actual.getPosition(), Vector3<real_Num>( 1.5, 0, 0 ) );
    }
}

BOOST_AUTO_TEST_CASE( interpolation_preserves_rotation_scale_and_history_boundaries )
{
    auto actor = createSceneActor( *this, "FullPoseSampling" );
    const auto id = actor->getHandle()->getInstanceId();
    const auto task = Thread::getCurrentTask();
    const auto start = makeTransform( Vector3<real_Num>::zero() );
    Quaternion<real_Num> rotation;
    rotation.fromAngleAxis( real_Num( 1 ), Vector3<real_Num>::unitY() );
    const Transform3<real_Num> end( Vector3<real_Num>( 2, 0, 0 ), rotation,
                                   Vector3<real_Num>( 3, 3, 3 ) );
    // Late arrivals must still be sorted by timestamp.
    sceneManager->addTransformState( id, 2, end );
    sceneManager->addTransformState( id, 1, start );
    Transform3<real_Num> actual;
    BOOST_REQUIRE( sceneManager->getTransformState( id, 1.5, .016, actual, task ) );
    checkVectorClose( actual.getPosition(), Vector3<real_Num>( 1, 0, 0 ) );
    checkVectorClose( actual.getScale(), Vector3<real_Num>( 2, 2, 2 ) );
    Quaternion<real_Num> halfway;
    halfway.fromAngleAxis( real_Num( .5 ), Vector3<real_Num>::unitY() );
    checkQuaternionClose( actual.getOrientation(), halfway );
    BOOST_REQUIRE( sceneManager->getTransformState( id, 1, .016, actual, task ) );
    checkTransformClose( actual, start );
    BOOST_REQUIRE( sceneManager->getTransformState( id, 0, .016, actual, task ) );
    checkTransformClose( actual, start );
    BOOST_REQUIRE( sceneManager->getTransformState( id, 2, .016, actual, task ) );
    checkTransformClose( actual, end );
}

BOOST_AUTO_TEST_CASE( prediction_integrates_angular_velocity_and_stops_after_short_gap )
{
    auto actor = createSceneActor( *this, "BoundedPrediction" );
    const auto id = actor->getHandle()->getInstanceId();
    const auto task = Thread::getCurrentTask();
    sceneManager->addTransformState( id, 10, makeTransform( Vector3<real_Num>::zero() ),
                                     Vector3<real_Num>( 2, 0, 0 ), Vector3<real_Num>( 0, 1, 0 ) );
    Transform3<real_Num> actual;
    BOOST_REQUIRE( sceneManager->getTransformState( id, 1000, .016, actual, task ) );
    checkVectorClose( actual.getPosition(), Vector3<real_Num>( .2, 0, 0 ) );
    Quaternion<real_Num> rotation;
    rotation.fromAngleAxis( real_Num( .1 ), Vector3<real_Num>::unitY() );
    checkQuaternionClose( actual.getOrientation(), rotation );
}

BOOST_AUTO_TEST_CASE( scene_lock_does_not_drop_transform_samples )
{
    auto actor = createSceneActor( *this, "IndependentHistoryLock" );
    const auto id = actor->getHandle()->getInstanceId();
    const auto task = Thread::getCurrentTask();
    const auto expected = makeTransform( Vector3<real_Num>( 1, 2, 3 ) );
    sceneManager->addTransformState( id, 10, expected );
    Transform3<real_Num> actual;
    bool sampled = false;
    sceneManager->lock();
    std::thread reader( [&]() {
        sampled = sceneManager->getTransformState( id, 10, .016, actual, task );
    } );
    reader.join();
    sceneManager->unlock();
    BOOST_REQUIRE( sampled );
    checkTransformClose( actual, expected );
}

BOOST_AUTO_TEST_CASE( free_running_producer_and_renderer_preserve_constant_speed )
{
    auto actor = createSceneActor( *this, "IndependentCadences" );
    const auto id = actor->getHandle()->getInstanceId();
    const auto task = Thread::getCurrentTask();
    const Vector3<real_Num> velocity( 20, 0, -10 );
    time_interval producerTime = 0;
    for( size_t frame = 0; frame < 300; ++frame )
    {
        const time_interval renderTime = frame / 144.0;
        // Deliberately vary delivery lag so queries cross interpolation/prediction boundaries.
        const time_interval deliveryTime = renderTime - ( frame % 7 == 0 ? .025 : .003 );
        while( producerTime <= deliveryTime || frame == 0 )
        {
            sceneManager->addTransformState( id, producerTime,
                makeTransform( velocity * static_cast<real_Num>( producerTime ) ),
                velocity, Vector3<real_Num>::zero() );
            producerTime += 1.0 / 120.0;
            if( frame == 0 ) break;
        }
        const auto sampleTime = std::max( time_interval( 0 ), renderTime - scene::IGameManager::smoothMotionDelay );
        Transform3<real_Num> actual;
        BOOST_REQUIRE( sceneManager->getTransformState( id, sampleTime, 1.0 / 144.0, actual, task ) );
        checkVectorClose( actual.getPosition(), velocity * static_cast<real_Num>( sampleTime ) );
    }
}

BOOST_AUTO_TEST_CASE( render_mesh_samples_parent_without_a_parent_renderer )
{
    auto parent = createSceneActor( *this, "PhysicsParentOnly" );
    auto child = createSceneActor( *this, "AttachedRenderMesh" );
    parent->addChild( child );
    child->setLocalPosition( Vector3<real_Num>( 2, 3, 4 ) );
    parent->setSmoothMotion( true, true );
    parent->getTransform()->setTask( TaskId::Primary );
    sceneManager->addTransformState( parent->getHandle()->getInstanceId(), 0,
                                     makeTransform( Vector3<real_Num>( 10, 0, 0 ) ) );
    // Application history on an attached mesh must not override its physics parent.
    child->getTransform()->setTask( TaskId::Application );
    sceneManager->addTransformState( child->getHandle()->getInstanceId(), 0,
                                     makeTransform( Vector3<real_Num>( -100, 0, 0 ) ) );
    auto capture = workphone::make_ptr<SmoothTransformCapture>();
    capture->setActor( child );
    capture->setLoadingState( LoadingState::Loaded );
    sceneManager->registerComponentUpdate( TaskId::Render, Thread::UpdateState::Transform, capture );
    const auto previousTask = Thread::getCurrentTask();
    Thread::setCurrentTask( TaskId::Render );
    sceneManager->preUpdate();
    Thread::setCurrentTask( previousTask );
    sceneManager->unregisterComponentUpdate( TaskId::Render, Thread::UpdateState::Transform, capture );
    BOOST_REQUIRE( capture->sampled );
    checkVectorClose( capture->pose.getPosition(), Vector3<real_Num>( 12, 3, 4 ) );
    capture->setActor( nullptr );
    capture->setLoadingState( LoadingState::Unloaded );
}

BOOST_AUTO_TEST_CASE( single_executor_hierarchy_update_preserves_sampled_render_pose )
{
    BOOST_REQUIRE( taskManager->getNumTasks() > 0 );
    auto pool = applicationManager->getThreadPool();
    BOOST_REQUIRE( pool );
    auto previousThreads = pool->getNumThreads();
    pool->setNumThreads( 0 );
    addCleanup( [pool, previousThreads]() { pool->setNumThreads( previousThreads ); } );
    auto actor = createSceneActor( *this, "SampledRenderPose" );
    actor->setSmoothMotion( true );
    actor->setPosition( Vector3<real_Num>( 100, 0, 0 ) );
    actor->updateTransform();
    auto node = workphone::make_ptr<PoseNode>();
    auto renderer = workphone::make_ptr<PlayingRenderer>();
    renderer->setActor( actor );
    renderer->setGraphicsNode( node );
    const auto sampled = makeTransform( Vector3<real_Num>( 10, 0, 0 ) );
    renderer->updateTransform( sampled );
    renderer->updateTransform();
    checkTransformClose( node->getTransform(), sampled );
    renderer->setGraphicsNode( nullptr );
    renderer->setActor( nullptr );
}

BOOST_AUTO_TEST_SUITE_END()
