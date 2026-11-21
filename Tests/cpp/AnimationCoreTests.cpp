#define BOOST_TEST_MODULE WPAnimationCoreTests
#include <Workphone/WorkphonePrerequisites.hpp>
#include <boost/test/included/unit_test.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Animation/Animation.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Scene/Components/Animator.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>

struct AnimationRuntimeFixture
{
    workphone::TypeManager types;
    workphone::SmartPtr<workphone::core::ApplicationManager> application;
    AnimationRuntimeFixture()
    {
        types.load();
        workphone::TypeManager::setInstance( &types );
        application = workphone::make_ptr<workphone::core::ApplicationManager>();
        workphone::core::IApplicationManager::setInstance( application );
        application->setFactoryManager( workphone::make_ptr<workphone::FactoryManager>() );
    }
    ~AnimationRuntimeFixture()
    {
        application->setFactoryManager( nullptr );
        workphone::core::IApplicationManager::setInstance( nullptr );
        application = nullptr;
        workphone::TypeManager::setInstance( nullptr );
        types.unload();
    }
};

BOOST_GLOBAL_FIXTURE( AnimationRuntimeFixture );

BOOST_AUTO_TEST_CASE( animator_reopens_authored_graph_and_rebinds_instance_clips )
{
    using namespace workphone;
    using namespace workphone::animation;

    auto sourceClip = make_ptr<workphone::Animation>();
    sourceClip->setName( "Imported walk" );
    sourceClip->setLength( 1.0f );

    AnimationGraphDefinition graph;
    AnimationGraphClip binding;
    binding.id = "Walk Motion";
    binding.animation = sourceClip;
    BOOST_REQUIRE( graph.addClip( binding ) );
    BOOST_REQUIRE( graph.addParameter( { "moving", true, 0.0f, false } ) );

    AnimationGraphState idle;
    idle.id = "Idle";
    idle.clipId = binding.id;
    idle.editorX = -125.0f;
    idle.editorY = 180.0f;
    idle.hasEditorPosition = true;
    BOOST_REQUIRE( graph.addState( idle ) );

    AnimationGraphState walk = idle;
    walk.id = "Walk";
    walk.editorX = 150.0f;
    BOOST_REQUIRE( graph.addState( walk ) );
    graph.setInitialState( idle.id );

    AnimationGraphTransition transition;
    transition.fromState = idle.id;
    transition.toState = walk.id;
    transition.duration = 0.0f;
    transition.conditions.push_back( { "moving", AnimationGraphComparison::Equal, 0.0f, true, true } );
    BOOST_REQUIRE( graph.addTransition( transition ) );

    scene::Animator original;
    BOOST_REQUIRE( original.setAuthoredAnimationGraph( graph ) );
    original.play();
    BOOST_REQUIRE( original.isPlaying() );
    original.setGraphBool( "moving", true );
    original.getAnimationGraph()->update( 0.1f );
    BOOST_CHECK_EQUAL( original.getGraphState(), "Walk" );

    scene::Animator reopened;
    reopened.setProperties( static_pointer_cast<Properties>( original.toData() ) );
    BOOST_CHECK( !reopened.hasAnimationGraph() );
    BOOST_CHECK( reopened.isStopped() );
    auto reopenedClip = make_ptr<workphone::Animation>();
    reopenedClip->setName( sourceClip->getName() );
    reopenedClip->setLength( sourceClip->getLength() );
    reopened.addAnimation( static_pointer_cast<IAnimation>( reopenedClip ) );
    BOOST_REQUIRE( reopened.prepareAnimationGraphClips() );
    const auto *reopenedGraph = reopened.getAnimationGraph();
    BOOST_REQUIRE( reopenedGraph );
    BOOST_CHECK_EQUAL( reopened.getGraphState(), "Idle" );
    BOOST_CHECK( !reopened.getGraphBool( "moving" ) );
    const auto *reopenedIdle = reopenedGraph->getDefinition().findState( "Idle" );
    BOOST_REQUIRE( reopenedIdle );
    BOOST_CHECK( reopenedIdle->hasEditorPosition );
    BOOST_CHECK_EQUAL( reopenedIdle->editorX, -125.0f );
    BOOST_CHECK_EQUAL( reopenedIdle->editorY, 180.0f );
    const auto *reopenedBinding = reopenedGraph->getDefinition().findClip( binding.id );
    BOOST_REQUIRE( reopenedBinding );
    BOOST_CHECK( reopenedBinding->animation == reopenedClip );

    reopened.setGraphBool( "moving", true );
    reopened.getAnimationGraph()->update( 0.1f );
    BOOST_CHECK_EQUAL( reopened.getGraphState(), "Walk" );
    reopened.clearAnimationGraph();
    const auto cleared = static_pointer_cast<Properties>( reopened.toData() );
    BOOST_CHECK( !cleared->getChild( "animationGraph" ) );
}

#if WP_ANIMATION_TYPED_RESOURCES
#include "SkeletonResourceContracts.hpp"
#include "AnimationResourceContracts.hpp"
#include "RetargetResourceContracts.hpp"
#include "RigResourceContracts.hpp"
#endif
