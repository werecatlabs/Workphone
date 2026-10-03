#include "UnitTests.hpp"
#include <Workphone/Animation/Animation.hpp>
#include <Workphone/Animation/AnimationEventTimeline.hpp>
#include <Workphone/Animation/AnimationFrameTime.hpp>
#include <Workphone/Animation/AnimationGraph.hpp>
#include <Workphone/Animation/AnimationSyncTrack.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

using namespace workphone;
using namespace workphone::animation;

BOOST_AUTO_TEST_SUITE( AnimationGraphTests )

BOOST_AUTO_TEST_CASE( frame_time_converts_between_clip_and_frame_space )
{
    const auto frameTime = AnimationFrameTime::fromNormalizedTime( 0.5f, 11 );
    BOOST_CHECK_EQUAL( frameTime.getFrameIndex(), 5 );
    BOOST_CHECK( frameTime.isExactlyAtKeyFrame() );
    BOOST_CHECK_CLOSE( frameTime.toSeconds( 10.0f ), 0.5f, 0.001f );

    const AnimationFrameTime fractional( 3, 0.75f );
    BOOST_CHECK_EQUAL( fractional.getLowerBoundFrameIndex(), 3 );
    BOOST_CHECK_EQUAL( fractional.getUpperBoundFrameIndex(), 4 );
    BOOST_CHECK_EQUAL( fractional.getNearestFrameIndex(), 4 );
}

BOOST_AUTO_TEST_CASE( event_timeline_samples_linear_wrapped_and_active_events )
{
    AnimationEventTimeline timeline;
    const auto track = timeline.addTrack( "Gameplay" );
    BOOST_REQUIRE_NE( timeline.addEvent( track, { "Footstep", "left", 0.25f, 0.0f, false } ),
                      AnimationEventTimeline::InvalidIndex );
    BOOST_REQUIRE_NE( timeline.addEvent( track, { "Window", {}, 0.8f, 0.3f, false } ),
                      AnimationEventTimeline::InvalidIndex );

    const auto linear = timeline.sampleRange( 0.1f, 0.3f );
    BOOST_REQUIRE_EQUAL( linear.size(), 1 );
    BOOST_CHECK_EQUAL( linear.front().event.id, "Footstep" );
    BOOST_CHECK( linear.front().phase == AnimationEventPhase::Started );

    const auto wrapped = timeline.sampleRange( 0.9f, 0.2f, true );
    BOOST_REQUIRE_EQUAL( wrapped.size(), 1 );
    BOOST_CHECK_EQUAL( wrapped.front().event.id, "Window" );
    BOOST_CHECK( wrapped.front().phase == AnimationEventPhase::Ended );

    const auto active = timeline.getActiveEvents( 0.05f );
    BOOST_REQUIRE_EQUAL( active.size(), 1 );
    BOOST_CHECK_EQUAL( active.front().event.id, "Window" );
}

BOOST_AUTO_TEST_CASE( sync_track_round_trips_non_uniform_markers )
{
    const AnimationSyncTrack syncTrack( { { "Left", 0.0f }, { "Right", 0.4f }, { "Left", 0.75f } } );
    BOOST_REQUIRE( syncTrack.isValid() );

    const auto syncTime = syncTrack.getTime( 0.575f );
    BOOST_CHECK_EQUAL( syncTime.eventIndex, 1 );
    BOOST_CHECK_CLOSE( syncTime.percentageThrough, 0.5f, 0.01f );
    BOOST_CHECK_CLOSE( syncTrack.getNormalizedTime( syncTime ), 0.575f, 0.01f );
    BOOST_CHECK_EQUAL( syncTrack.getClosestEventIndex( syncTime, "Left" ), 2 );
}

BOOST_AUTO_TEST_CASE( graph_evaluates_parameterized_synchronized_transition )
{
    auto idleAnimation = workphone::make_ptr<workphone::Animation>();
    idleAnimation->setLength( 2.0f );
    auto runAnimation = workphone::make_ptr<workphone::Animation>();
    runAnimation->setLength( 1.0f );

    AnimationGraphDefinition definition;
    AnimationGraphClip idleClip;
    idleClip.id = "IdleClip";
    idleClip.animation = idleAnimation;
    AnimationGraphClip runClip;
    runClip.id = "RunClip";
    runClip.animation = runAnimation;

    BOOST_REQUIRE( definition.addClip( idleClip ) );
    BOOST_REQUIRE( definition.addClip( runClip ) );
    BOOST_REQUIRE( definition.addState( { "Idle", "IdleClip", 1.0f } ) );
    BOOST_REQUIRE( definition.addState( { "Run", "RunClip", 1.0f } ) );

    AnimationGraphTransition transition;
    transition.fromState = "Idle";
    transition.toState = "Run";
    transition.duration = 0.5f;
    transition.conditions.push_back( { "Moving", AnimationGraphComparison::Equal, 0.0f, true, true } );
    BOOST_REQUIRE( definition.addTransition( transition ) );
    BOOST_REQUIRE( definition.isValid() );

    AnimationGraphInstance instance( definition );
    BOOST_REQUIRE( instance.isValid() );
    BOOST_CHECK_EQUAL( instance.getCurrentState(), "Idle" );

    instance.setBoolParameter( "Moving", true );
    instance.update( 0.2f );
    BOOST_CHECK( instance.isTransitioning() );

    instance.update( 0.25f );
    BOOST_REQUIRE_EQUAL( instance.getSamples().size(), 2 );
    BOOST_CHECK_CLOSE( instance.getTransitionProgress(), 0.5f, 0.01f );
    BOOST_CHECK_CLOSE( instance.getSamples()[0].weight, 0.5f, 0.01f );
    BOOST_CHECK_CLOSE( instance.getSamples()[1].weight, 0.5f, 0.01f );

    instance.update( 0.25f );
    BOOST_CHECK( !instance.isTransitioning() );
    BOOST_CHECK_EQUAL( instance.getCurrentState(), "Run" );
    BOOST_REQUIRE_EQUAL( instance.getSamples().size(), 1 );
    BOOST_CHECK_CLOSE( instance.getSamples().front().weight, 1.0f, 0.01f );
}

BOOST_AUTO_TEST_SUITE_END()
