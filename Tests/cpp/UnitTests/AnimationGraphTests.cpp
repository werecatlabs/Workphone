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

namespace
{
    AnimationGraphDefinition makeStateGraph()
    {
        AnimationGraphClip clip;
        clip.id = "Clip";
        auto animation = workphone::make_ptr<workphone::Animation>();
        animation->setLength( 1.0f );
        clip.animation = animation;

        AnimationGraphDefinition graph;
        BOOST_REQUIRE( graph.addClip( clip ) );
        BOOST_REQUIRE( graph.addState( { "Idle", "Clip", 1.0f } ) );
        BOOST_REQUIRE( graph.addState( { "Walk", "Clip", 1.0f } ) );
        BOOST_REQUIRE( graph.addState( { "Run", "Clip", 1.0f } ) );
        return graph;
    }
}

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

BOOST_AUTO_TEST_CASE( graph_edits_keep_initial_state_links_layout_and_priority )
{
    auto graph = makeStateGraph();
    BOOST_REQUIRE( graph.setStatePosition( "Idle", -140.0f, 290.0f ) );
    AnimationGraphTransition walk;
    walk.fromState = "Idle";
    walk.toState = "Walk";
    BOOST_REQUIRE( graph.addTransition( walk ) );
    auto run = walk;
    run.toState = "Run";
    size_t runIndex = 0;
    BOOST_REQUIRE( graph.addTransition( run, &runIndex ) );
    BOOST_CHECK_EQUAL( runIndex, 1 );

    auto returnToIdle = walk;
    returnToIdle.fromState = "Run";
    returnToIdle.toState = "Idle";
    BOOST_REQUIRE( graph.addTransition( returnToIdle ) );

    run.priority = 5;
    BOOST_REQUIRE( graph.updateTransition( runIndex, run, &runIndex ) );
    BOOST_CHECK_EQUAL( runIndex, 0 );
    BOOST_CHECK_EQUAL( graph.getTransitions().front().toState, "Run" );

    BOOST_CHECK( !graph.renameState( "Idle", "Run" ) );
    BOOST_REQUIRE( graph.renameState( graph.getInitialState(), "Ready" ) );
    BOOST_CHECK_EQUAL( graph.getInitialState(), "Ready" );
    BOOST_CHECK( !graph.findState( "Idle" ) );
    BOOST_CHECK_EQUAL( graph.getTransitions()[0].fromState, "Ready" );
    BOOST_CHECK_EQUAL( graph.getTransitions()[1].fromState, "Ready" );
    BOOST_CHECK_EQUAL( graph.getTransitions()[2].toState, "Ready" );
    const auto *ready = graph.findState( "Ready" );
    BOOST_REQUIRE( ready );
    BOOST_CHECK( ready->hasEditorPosition );
    BOOST_CHECK_EQUAL( ready->editorX, -140.0f );
    BOOST_CHECK_EQUAL( ready->editorY, 290.0f );

    AnimationGraphInstance instance( graph );
    instance.update( 0.01f );
    BOOST_CHECK_EQUAL( instance.getDestinationState(), "Run" );
    BOOST_CHECK( graph.removeTransition( 0 ) );
    BOOST_CHECK_EQUAL( graph.getTransitions().front().toState, "Walk" );
}

BOOST_AUTO_TEST_CASE( authored_parameter_defaults_drive_independent_instances )
{
    auto graph = makeStateGraph();
    BOOST_REQUIRE( graph.addParameter( { "Moving", true, 0.0f, true } ) );
    BOOST_REQUIRE( graph.addParameter( { "Speed", false, 2.0f } ) );
    BOOST_CHECK( !graph.addParameter( { "Speed", false, 3.0f } ) );

    AnimationGraphTransition transition;
    transition.fromState = "Idle";
    transition.toState = "Run";
    transition.conditions.push_back( { "Moving", AnimationGraphComparison::Equal, 0.0f, true, true } );
    transition.conditions.push_back( { "Speed", AnimationGraphComparison::Greater, 1.0f } );
    BOOST_REQUIRE( graph.addTransition( transition ) );

    AnimationGraphInstance first( graph );
    AnimationGraphInstance second( graph );
    first.setBoolParameter( "Moving", false );
    first.setFloatParameter( "Speed", 0.0f );
    first.setFloatParameter( "LegacyInput", 0.5f );
    first.update( 0.1f );
    second.update( 0.1f );
    BOOST_CHECK( first.getDestinationState().empty() );
    BOOST_CHECK_EQUAL( second.getDestinationState(), "Run" );

    first.reset();
    BOOST_CHECK( !first.getBoolParameter( "Moving" ) );
    BOOST_REQUIRE( first.setDefinition( graph ) );
    BOOST_CHECK( first.getBoolParameter( "Moving" ) );
    BOOST_CHECK_EQUAL( first.getFloatParameter( "Speed" ), 2.0f );
    BOOST_CHECK_EQUAL( first.getFloatParameter( "LegacyInput" ), 0.5f );
}

BOOST_AUTO_TEST_CASE( loop_exit_time_can_reach_end_of_clip )
{
    auto graph = makeStateGraph();
    AnimationGraphTransition transition;
    transition.fromState = "Idle";
    transition.toState = "Walk";
    transition.exitTime = 1.0f;
    transition.duration = 0.0f;
    BOOST_REQUIRE( graph.addTransition( transition ) );

    AnimationGraphInstance instance( graph );
    instance.update( 0.8f );
    BOOST_CHECK_EQUAL( instance.getCurrentState(), "Idle" );
    instance.update( 0.3f );
    BOOST_CHECK_EQUAL( instance.getCurrentState(), "Walk" );
}

BOOST_AUTO_TEST_CASE( authored_boolean_defaults_select_the_initial_pose )
{
    auto graph = makeStateGraph();
    auto alternate = workphone::make_ptr<workphone::Animation>();
    alternate->setLength( 2.0f );
    AnimationGraphClip clip;
    clip.id = "AlternateClip";
    clip.animation = alternate;
    BOOST_REQUIRE( graph.addClip( clip ) );
    BOOST_REQUIRE( graph.addParameter( { "Alternate", true, 0.0f, true } ) );

    AnimationGraphPoseNodeDefinition first;
    first.id = "First";
    first.clipId = "Clip";
    BOOST_REQUIRE( graph.addPoseNode( first ) );
    auto second = first;
    second.id = "Second";
    second.clipId = clip.id;
    BOOST_REQUIRE( graph.addPoseNode( second ) );
    AnimationGraphPoseNodeDefinition selector;
    selector.id = "Choose";
    selector.type = AnimationGraphPoseNodeType::Selector;
    selector.parameter = "Alternate";
    selector.childNodeIds = { first.id, second.id };
    BOOST_REQUIRE( graph.addPoseNode( selector ) );
    BOOST_REQUIRE( graph.editState( { "Idle", {}, 1.0f, selector.id } ) );

    AnimationGraphInstance instance( graph );
    BOOST_REQUIRE_EQUAL( instance.getSamples().size(), 1 );
    BOOST_CHECK( instance.getSamples()[0].animation.get() == alternate.get() );
    instance.setBoolParameter( "Alternate", false );
    instance.update( 0.0f );
    BOOST_REQUIRE_EQUAL( instance.getSamples().size(), 1 );
    BOOST_CHECK( instance.getSamples()[0].animation.get() == graph.findClip( "Clip" )->animation.get() );
}

BOOST_AUTO_TEST_SUITE_END()
