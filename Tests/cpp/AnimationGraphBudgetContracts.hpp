#pragma once
#include <Workphone/Animation/Animation.hpp>
#include <Workphone/Animation/AnimationGraph.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <cmath>
#include <memory>
#include <optional>
#include <stdexcept>

namespace animation_graph_budget_contracts
{
    using namespace workphone;
    using namespace workphone::animation;
    inline void require( bool value, const char *message )
    { if( !value ) throw std::runtime_error( message ); }
    struct Capacity
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        static constexpr size_t source = size_t( StreamMemoryClass::SourceCpu );
        u64 original = scheduler->stats().limits[source];
        u64 used() const { return scheduler->stats().used[source]; }
        void set( u64 bytes )
        {
            String error;
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, bytes, error ),
                     "Graph SourceCpu capacity applies" );
        }
        void restore() { set( original ); }
        ~Capacity() { restore(); }
    };
    template<class F> void denied( F &&function, const char *message )
    {
        bool rejected = false;
        try { function(); } catch( const std::bad_alloc & ) { rejected = true; }
        require( rejected, message );
    }
    inline bool closeValue( float a, float b ) { return std::abs( a - b ) < 0.0001f; }
    inline AnimationGraphDefinition flat( const SmartPtr<IAnimation> &animation, bool event = false )
    {
        AnimationGraphDefinition result;
        AnimationGraphClip clip;
        clip.id = "clip";
        clip.animation = animation;
        if( event )
        {
            const auto track = clip.eventTimeline.addTrack( "Gameplay" );
            require( clip.eventTimeline.addEvent( track, { "Step", "left", 0.25f, 0, false } ) !=
                         AnimationEventTimeline::InvalidIndex, "Graph fixture event is valid" );
        }
        require( result.addClip( clip ) && result.addState( { "Idle", "clip", 1 } ) &&
                     result.addState( { "Run", "clip", 1 } ), "Graph fixture is valid" );
        return result;
    }
    inline void definitionPayloads( Capacity &budget, const SmartPtr<IAnimation> &animation )
    {
        auto definition = flat( animation );
        AnimationGraphTransition transition;
        transition.fromState = "Idle";
        transition.toState = "Run";
        Array<AnimationGraphCondition> rawConditions;
        rawConditions.reserve( 11 );
        rawConditions.push_back( { "Moving", AnimationGraphComparison::Equal, 0, true, true } );
        transition.conditions = rawConditions;
        const auto resident = budget.used();
        budget.set( resident );
        denied( [&] { definition.addTransition( transition ); },
                "Borrowed transition conditions require admission before publication" );
        require( definition.getTransitions().empty() && budget.used() == resident,
                 "Denied transition import unwinds nested storage and leaves the definition intact" );
        budget.restore();
        require( definition.addTransition( transition ), "Transition import retries" );
        require( definition.getTransitions().get_allocator().isSourceBudgeted() &&
                     definition.getTransitions()[0].conditions.get_allocator().isSourceBudgeted() &&
                     definition.getTransitions()[0].conditions.capacity() == rawConditions.capacity(),
                 "Transition inventories and imported spare condition slots are SourceCpu owners" );

        AnimationGraphPoseNodeDefinition leaf;
        leaf.id = "leaf";
        leaf.clipId = "clip";
        require( definition.addPoseNode( leaf ), "Graph fixture leaf imports" );
        AnimationGraphPoseNodeDefinition blend;
        blend.id = "blend";
        blend.type = AnimationGraphPoseNodeType::Blend1D;
        blend.parameter = "Speed";
        Array<String> rawChildren;
        rawChildren.reserve( 13 );
        rawChildren.push_back( "leaf" );
        rawChildren.push_back( "leaf" );
        Array<AnimationGraphBlendRange> rawRanges;
        rawRanges.reserve( 9 );
        rawRanges.push_back( { 0, 1, 0, 1 } );
        blend.childNodeIds = rawChildren;
        blend.blendRanges = rawRanges;
        const auto beforeNode = budget.used();
        const auto childBytes = sourceBudgetStorageBytes( rawChildren.capacity() * sizeof( String ),
                                                        alignof( String ) );
        budget.set( beforeNode + childBytes );
        denied( [&] { definition.addPoseNode( blend ); },
                "Second nested pose-node array denial unwinds the already admitted child array" );
        require( budget.used() == beforeNode && !definition.findPoseNode( "blend" ),
                 "A partial pose-node import publishes neither array nor node" );
        budget.restore();
        require( definition.addPoseNode( blend ), "Pose-node import retries" );
        const auto admitted = definition.findPoseNode( "blend" );
        require( admitted && admitted->childNodeIds.get_allocator().isSourceBudgeted() &&
                     admitted->blendRanges.get_allocator().isSourceBudgeted() &&
                     admitted->childNodeIds.capacity() == 13 && admitted->blendRanges.capacity() == 9,
                 "Graph pose imports normalize both borrowed raw arrays including spare capacity" );

        auto previous = flat( animation );
        const auto previousClips = previous.getClips().data();
        u64 copyBytes = 0;
        {
            const auto beforeCopy = budget.used();
            AnimationGraphDefinition copy( definition );
            copyBytes = budget.used() - beforeCopy;
            require( copy.getPoseNodes().capacity() == definition.getPoseNodes().capacity() &&
                         copy.findPoseNode( "blend" )->childNodeIds.capacity() == 13,
                     "Definition snapshots preserve outer and nested admitted spare policy" );
        }
        const auto beforeAssign = budget.used();
        budget.set( beforeAssign + copyBytes - 1 );
        denied( [&] { previous = definition; }, "One-byte-short complete graph replacement is denied" );
        require( budget.used() == beforeAssign && previous.getClips().data() == previousClips &&
                     previous.getPoseNodes().empty() && previous.getTransitions().empty() && previous.isValid(),
                 "Failed deep definition replacement preserves the previous owner and exact residency" );
        budget.restore();
        previous = definition;
        std::optional<Array<AnimationGraphPoseNodeDefinition>> detached( previous.getPoseNodes() );
        const auto beforeClear = budget.used();
        budget.set( beforeClear );
        const auto denials = sourceBudgetDenialCount();
        previous.clear();
        require( sourceBudgetDenialCount() == denials && budget.used() < beforeClear &&
                     previous.getPoseNodes().capacity() == 0 && previous.getClips().capacity() == 0 &&
                     detached->back().childNodeIds.capacity() == 13,
                 "Graph clear releases physical arrays at capacity while detached nested owners survive" );
        const auto retained = budget.used();
        detached.reset();
        require( sourceBudgetDenialCount() == denials && budget.used() < retained,
                 "Final detached definition snapshot retires nested spare capacity at the cap" );
        budget.restore();
    }
    inline void runtimePublications( Capacity &budget, const SmartPtr<IAnimation> &animation )
    {
        const auto definition = flat( animation, true );
        AnimationGraphInstance instance( definition );
        const auto samplePointer = instance.getSamples().data();
        const auto resident = budget.used();
        budget.set( resident );
        denied( [&] { instance.update( 0.3f ); }, "Event sampling scratch must enter SourceCpu admission" );
        require( instance.getCurrentState() == "Idle" && instance.getSamples().data() == samplePointer &&
                     instance.getSamples().size() == 1 && closeValue( instance.getSamples()[0].time, 0 ) &&
                     instance.getSampledEvents().empty() && budget.used() == resident,
                 "Denied update preserves published time, event cursor and output array identity" );
        budget.restore();
        instance.update( 0.3f );
        require( closeValue( instance.getSamples()[0].time, 0.3f ) && instance.getSampledEvents().size() == 1 &&
                     instance.getSampledEvents()[0].sampledEvent.event.id == "Step",
                 "Update retry advances once and emits the event that denial did not consume" );
        require( instance.getSamples().get_allocator().isSourceBudgeted() &&
                     instance.getSampledEvents().get_allocator().isSourceBudgeted(),
                 "Published graph samples and events carry SourceCpu snapshot policy" );
        const auto publishedEvents = instance.getSampledEvents().data();
        const auto beforeWrapped = budget.used();
        budget.set( beforeWrapped );
        denied( [&] { instance.update( 1.0f ); }, "Wrapped event evaluation cannot bypass SourceCpu admission" );
        require( budget.used() == beforeWrapped && closeValue( instance.getSamples()[0].time, 0.3f ) &&
                     instance.getSampledEvents().data() == publishedEvents &&
                     instance.getSampledEvents().size() == 1,
                 "Denied wrapped evaluation preserves previously published events as well as its time cursor" );
        budget.restore();
        auto retainedEvents = instance.getSampledEvents();
        instance.setBoolParameter( "Moving", false );
        instance.setFloatParameter( "Speed", 0.25f );
        const auto warm = budget.used();
        budget.set( warm );
        const auto noAdmission = sourceBudgetDenialCount();
        instance.setBoolParameter( "Moving", true );
        instance.setFloatParameter( "Speed", 0.5f );
        instance.update( 0.01f );
        require( sourceBudgetDenialCount() == noAdmission && budget.used() == warm &&
                     closeValue( instance.getSamples()[0].time, 0.31f ) && instance.getSampledEvents().empty() &&
                     retainedEvents[0].sampledEvent.event.payload == "left",
                 "Warm evaluation and existing parameter writes reuse admission at the cap" );
        denied( [&] { instance.setBoolParameter( "NewBool", true ); }, "New graph bool nodes require SourceCpu" );
        denied( [&] { instance.setFloatParameter( "NewFloat", 1 ); }, "New graph float nodes require SourceCpu" );
        require( !instance.getBoolParameter( "NewBool" ) && instance.getFloatParameter( "NewFloat", -1 ) == -1 &&
                     budget.used() == warm, "Denied parameter nodes preserve maps and release partial nodes" );
        denied( [&] { auto snapshot = instance.getSamples(); }, "Caller sample snapshots remain admitted" );
        budget.restore();

        const auto replacement = flat( animation );
        AnimationGraphInstance cold( definition );
        const auto oldClips = cold.getDefinition().getClips().data();
        const auto oldSamples = cold.getSamples().data();
        const auto beforeForce = budget.used();
        budget.set( beforeForce );
        denied( [&] { cold.forceState( "Run", 0.5f ); }, "Cold forceState output admits before replacing runtime" );
        denied( [&] { cold.reset(); }, "Cold reset output admits before discarding runtime" );
        require( budget.used() == beforeForce && cold.getCurrentState() == "Idle" &&
                     cold.getSamples().data() == oldSamples && closeValue( cold.getSamples()[0].time, 0 ),
                 "Denied forceState and reset retain the complete published runtime" );
        budget.restore();
        u64 replacementBytes;
        {
            const auto before = budget.used();
            AnimationGraphDefinition copy( replacement );
            replacementBytes = budget.used() - before;
        }
        const auto beforeDefinition = budget.used();
        budget.set( beforeDefinition + replacementBytes );
        denied( [&] { cold.setDefinition( replacement ); },
                "Definition replacement rolls back when its subsequent cold runtime rebuild is denied" );
        require( budget.used() == beforeDefinition && cold.getDefinition().getClips().data() == oldClips &&
                     cold.getSamples().data() == oldSamples && cold.getCurrentState() == "Idle" &&
                     cold.getDefinition().getClips()[0].eventTimeline.getTracks().size() == 1,
                 "Failed reset restores both old graph and old runtime rather than publishing a mixed generation" );
        budget.restore();
        require( cold.setDefinition( replacement ) && cold.forceState( "Run", 0.5f ) &&
                     closeValue( cold.getSamples()[0].time, 0.5f ), "Definition and forced-state replacement retry" );

        auto transitioningDefinition = definition;
        AnimationGraphTransition transition;
        transition.fromState = "Idle"; transition.toState = "Run"; transition.duration = 1;
        require( transitioningDefinition.addTransition( transition ), "Transition rollback fixture imports" );
        AnimationGraphInstance transitioning( transitioningDefinition );
        transitioning.update( 0.1f );
        transitioning.update( 0.05f );
        require( transitioning.isTransitioning() && closeValue( transitioning.getTransitionProgress(), 0.05f ) &&
                     transitioning.getSamples().size() == 2, "Transition fixture has both published states" );
        const auto transitionSamples = transitioning.getSamples().data();
        const auto beforeTransition = budget.used();
        budget.set( beforeTransition );
        denied( [&] { transitioning.update( 0.2f ); }, "Transition event scratch denial rolls back both clocks" );
        require( budget.used() == beforeTransition && transitioning.getSamples().data() == transitionSamples &&
                     closeValue( transitioning.getTransitionProgress(), 0.05f ) &&
                     closeValue( transitioning.getSamples()[0].time, 0.15f ) &&
                     closeValue( transitioning.getSamples()[1].time, 0.15f ),
                 "Denied transition update preserves progress, source time, destination time and output owners" );
        budget.restore();
        transitioning.update( 0.2f );
        require( closeValue( transitioning.getTransitionProgress(), 0.25f ) &&
                     transitioning.getSampledEvents().size() == 2 &&
                     closeValue( transitioning.getSamples()[0].time, 0.35f ),
                 "Transition retry advances each state once and emits both previously unconsumed events" );
    }
    inline void bodiesAndPoseTraversal( Capacity &budget, const SmartPtr<IAnimation> &animation )
    {
        const auto baseline = budget.used();
        {
            const auto full = budget.used();
            budget.set( full );
            denied( [] { auto body = std::make_unique<AnimationGraphDefinition>(); },
                    "Heap graph definition bodies require SourceCpu" );
            denied( [] { auto body = std::make_unique<AnimationGraphInstance>(); },
                    "Heap graph runtime bodies require SourceCpu" );
            budget.restore();
        }
        auto definition = std::make_unique<AnimationGraphDefinition>( flat( animation ) );
        AnimationGraphPoseNodeDefinition leaf;
        // Exceed the character pool's largest block so a copied visitation ID
        // cannot be hidden by a warm retained string slab.
        leaf.id.assign( CharacterPoolAllocator<char>::MaxPooledBlockBytes + 1, 'L' );
        leaf.clipId = "clip";
        require( definition->addPoseNode( leaf ), "Pose traversal leaf imports" );
        AnimationGraphPoseNodeDefinition blend;
        blend.id.assign( CharacterPoolAllocator<char>::MaxPooledBlockBytes + 1, 'B' );
        blend.type = AnimationGraphPoseNodeType::Blend1D; blend.parameter = "Speed";
        blend.childNodeIds.push_back( leaf.id ); blend.childNodeIds.push_back( leaf.id );
        blend.blendRanges.push_back( { 0, 1, 0, 1 } );
        require( definition->addPoseNode( blend ) && definition->addState( { "Blend", {}, 1, blend.id } ),
                 "Shared-child pose traversal fixture imports" );
        leaf = AnimationGraphPoseNodeDefinition{};
        blend = AnimationGraphPoseNodeDefinition{};
        definition->setInitialState( "Blend" );
        auto instance = std::make_unique<AnimationGraphInstance>( *definition );
        instance->setFloatParameter( "Speed", 0.5f );
        instance->update( 0.1f );
        instance->update( 0.1f );
        require( instance->getSamples().size() == 2 && closeValue( instance->getSamples()[0].weight, 0.5f ) &&
                     closeValue( instance->getSamples()[1].weight, 0.5f ),
                 "Scoped traversal visits both shared child branches without retaining a false cycle" );
        std::optional<Array<AnimationGraphSample>> retained( instance->getSamples() );
        const auto resident = budget.used();
        budget.set( resident );
        const auto denials = sourceBudgetDenialCount();
        instance->update( 0.1f );
        require( sourceBudgetDenialCount() == denials && budget.used() == resident,
                 "Oversized nonpooled node traversal reuses its borrowed path stack and output workspaces at capacity" );
        instance.reset(); definition.reset();
        require( sourceBudgetDenialCount() == denials && budget.used() > baseline &&
                     retained->size() == 2 && closeValue( retained->front().time, 0.2f ),
                 "Graph body teardown at capacity retains detached sample ownership" );
        retained.reset();
        require( sourceBudgetDenialCount() == denials && budget.used() == baseline,
                 "Final graph output release retires all graph body, runtime and nested allocations" );
        budget.restore();
    }
    inline void run()
    {
        Capacity budget;
        auto animation = make_ptr<Animation>();
        animation->setLength( 1 );
        definitionPayloads( budget, animation );
        runtimePublications( budget, animation );
        bodiesAndPoseTraversal( budget, animation );
    }
}
