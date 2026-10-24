#pragma once

#include <Workphone/Animation/Animation.hpp>
#include <Workphone/Animation/ActorAnimationTrack.hpp>
#include <Workphone/Animation/AnimationNumericTrack.hpp>
#include <Workphone/Animation/AnimationTimeIndex.hpp>
#include <Workphone/Animation/KeyFrameTransform3.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include "MeshObjectBodyBudgetContracts.hpp"
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <array>
#include <stdexcept>

namespace animation_payload_budget_contracts
{
    using namespace workphone;
    inline void require( bool value, const char *message )
    { if( !value ) throw std::runtime_error( message ); }

    struct Capacity
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        static constexpr auto source = size_t( StreamMemoryClass::SourceCpu );
        u64 original = scheduler->stats().limits[source];
        u64 used() const { return scheduler->stats().used[source]; }
        void set( u64 value )
        {
            String error;
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, value, error ), "animation capacity applies" );
        }
        void restore() { set( original ); }
        ~Capacity() { restore(); }
    };
    template<class F> inline void denied( F operation, const char *message )
    {
        bool refused = false;
        try { operation(); } catch( const std::bad_alloc & ) { refused = true; }
        require( refused, message );
    }
    template<class T> inline u64 arrayBytes( size_t capacity )
    { return capacity ? sourceBudgetStorageBytes( capacity * sizeof( T ), alignof( T ) ) : 0; }
    template<class T> inline u64 bodyBytes()
    { return sourceBudgetStorageBytes( sizeof( T ), alignof( std::max_align_t ) ); }

    class AnimationProbe : public Animation
    {
    public:
        auto &nodes() { return m_ownedNodeTracks; }
        auto &numerics() { return m_ownedNumericTracks; }
        auto &vertices() { return m_ownedVertexTracks; }
    };

    inline void bodies()
    {
        // Warm the shared object metadata pools; actual wrapper allocations are
        // still independently admitted on every new expression.
        { auto a = make_ptr<AnimationKeyFrame>(); auto b = make_ptr<KeyFrameTransform3>();
          auto c = make_ptr<AnimationTimeIndex>(); auto d = make_ptr<AnimationNumericTrack>();
          auto e = make_ptr<ActorAnimationTrack>(); auto f = make_ptr<Animation>(); }
        Capacity budget;
        const auto baseline = budget.used();
        budget.set( baseline );
        denied( [] { auto a = make_ptr<AnimationKeyFrame>(); }, "plain keyframe body admission is mandatory" );
        denied( [] { auto a = make_ptr<KeyFrameTransform3>(); }, "transform keyframe body admission is mandatory" );
        denied( [] { auto a = make_ptr<AnimationTimeIndex>(); }, "time index body admission is mandatory" );
        denied( [] { auto a = make_ptr<AnimationNumericTrack>(); }, "numeric track body admission is mandatory" );
        denied( [] { auto a = make_ptr<ActorAnimationTrack>(); }, "actor track body admission is mandatory" );
        denied( [] { auto a = make_ptr<Animation>(); }, "animation body admission is mandatory" );
        require( budget.used() == baseline, "denied animation wrappers leave no body or payload allocation" );
        budget.restore();
        {
            auto key = make_ptr<KeyFrameTransform3>();
            require( budget.used() == baseline + bodyBytes<KeyFrameTransform3>(),
                     "transform fields are covered by one actual admitted object body" );
            budget.set( budget.used() );
            key->setPosition( Vector3<real_Num>( 2, 4, 6 ) );
            key->setScale( Vector3<real_Num>( 3, 3, 3 ) );
            require( key->getPosition().y == 4 && key->getScale().x == 3,
                     "inline transform edits need no new admission" );
            key = nullptr;
            require( budget.used() == baseline, "full-capacity transform destruction retires its body" );
        }
        budget.restore();
        // Numeric construction has a separate one-key payload allocation after
        // its body. A denial there must invoke normal constructor unwind.
        budget.set( baseline + bodyBytes<AnimationNumericTrack>() +
                    arrayBytes<AnimationNumericTrack::KeyFrame>( 1 ) - 1 );
        denied( [] { auto a = make_ptr<AnimationNumericTrack>(); },
                "numeric constructor payload denial propagates after body admission" );
        require( budget.used() == baseline, "numeric partial construction retires body and earlier metadata" );
    }

    inline void inventories()
    {
        Capacity budget;
        AnimationProbe animation;
        auto previous = animation.addTrack( StringUtil::getHash( "ActorAnimationTrack" ), "old", 0 );
        require( previous != nullptr, "actor track publication succeeds" );
        auto first = make_ptr<ActorAnimationTrack>();
        auto second = make_ptr<ActorAnimationTrack>();
        Array<SmartPtr<IActorAnimationTrack>> input;
        input.reserve( 17 ); input.push_back( first ); input.push_back( second );
        Array<SmartPtr<IActorAnimationTrack>> old;
        old.push_back( dynamic_pointer_cast<IActorAnimationTrack>( previous ) );
        const auto resident = budget.used();
        unsigned refused = 0, accepted = 0;
        for( u64 headroom : { 0ULL, 32ULL, 64ULL, 128ULL, 192ULL, 256ULL, 384ULL, 512ULL, 1024ULL } )
        {
            budget.set( resident + headroom );
            try
            {
                animation.setNodeTracks( input );
                ++accepted;
                require( animation.getNumNodeTracks() == 2 && animation.getNodeTrack( 0 ) == first.get() &&
                             animation.getNodeTrack( 1 ) == second.get() && animation.nodes().capacity() == 17 &&
                             animation.nodes().get_allocator().isSourceBudgeted(),
                         "imported track owners preserve spare capacity under Source policy" );
            }
            catch( const std::bad_alloc & )
            {
                ++refused;
                require( animation.getNumNodeTracks() == 1 && animation.getNodeTrack( 0 ) == previous.get() &&
                             animation.nodes().size() == 1 && budget.used() == resident,
                         "map or owner-array denial preserves both published inventories" );
            }
            budget.restore();
            animation.setNodeTracks( old );
            require( budget.used() == resident, "import retry retires all replaced map and owner capacity" );
        }
        require( refused >= 3 && accepted > 0, "track import sweep covers independent map/list stages and retry" );
        refused = accepted = 0;
        for( u64 headroom : { 0ULL, 128ULL, 256ULL, 512ULL, 768ULL, 1024ULL, 2048ULL, 4096ULL, 16384ULL } )
        {
            budget.set( resident + headroom );
            SmartPtr<IAnimationTrack> added;
            try
            {
                added = animation.addTrack( StringUtil::getHash( "ActorAnimationTrack" ), "new", 9 );
                ++accepted;
                require( added && animation.getNodeTrack( 9 ) == added.get() && animation.nodes().size() == 2,
                         "new track becomes visible in map and owner list together" );
                budget.set( budget.used() );
                animation.destroyNodeTrack( 9 );
                require( !animation.hasNodeTrack( 9 ) && added->getParent() == nullptr,
                         "at-capacity removal drops the index and detaches retained native track parents" );
            }
            catch( const std::bad_alloc & )
            {
                ++refused;
                require( animation.getNumNodeTracks() == 1 && animation.nodes().size() == 1 &&
                             !animation.hasNodeTrack( 9 ) && budget.used() == resident,
                         "partial track construction or publication unwinds without a dangling map entry" );
            }
            added = nullptr;
            budget.restore(); animation.setNodeTracks( old );
            require( budget.used() == resident, "staged actor publication and removal retire all candidate bodies" );
        }
        require( refused >= 3 && accepted > 0, "new-track sweep reaches body/keyframe/map/list failures and success" );
        animation.setNodeTracks( input );
        const auto beforeSnapshot = budget.used();
        auto snapshot = animation.getNodeTracks();
        require( snapshot.capacity() == 17 && snapshot.get_allocator().isSourceBudgeted() &&
                     budget.used() == beforeSnapshot + arrayBytes<SmartPtr<IActorAnimationTrack>>( 17 ),
                 "caller track snapshots admit exact spare capacity and preserve owners" );
        budget.set( budget.used() );
        denied( [&] { auto copied = animation._getNodeTrackList(); }, "track map snapshots preserve Source node admission" );
        denied( [&] { auto copied = animation.getNodeTracks(); }, "track array snapshots preserve Source admission" );
        animation.unload( nullptr );
        require( animation.getNumNodeTracks() == 0 && animation.nodes().capacity() == 0 &&
                     snapshot.size() == 2 && snapshot[0] == first && snapshot[1] == second,
                 "full-capacity animation unload retires owned arrays while escaped snapshots stay valid" );
        budget.restore();
        auto aliased = make_ptr<ActorAnimationTrack>( &animation );
        Array<SmartPtr<IActorAnimationTrack>> duplicates{ aliased, aliased };
        animation.setNodeTracks( duplicates );
        auto alias = aliased.get();
        aliased = nullptr; duplicates.clear();
        budget.set( budget.used() );
        animation.destroyNodeTrack( 0 );
        require( animation.getNumNodeTracks() == 1 && animation.getNodeTrack( 1 ) == alias &&
                     animation.nodes().size() == 1 && alias->getParent() == &animation,
                 "removing one aliased handle keeps its other index backed by a live owner and parent" );
        animation.destroyNodeTrack( 1 );
        require( animation.nodes().empty() && animation.getNumNodeTracks() == 0,
                 "last aliased handle retires its final track owner at capacity" );
    }

    inline void keyframes()
    {
        Capacity budget;
        auto track = make_ptr<ActorAnimationTrack>();
        auto zero = dynamic_pointer_cast<KeyFrameTransform3>( track->getKeyFrame( 0 ) );
        zero->setPosition( Vector3<real_Num>( 0, 0, 0 ) );
        auto last = dynamic_pointer_cast<KeyFrameTransform3>( track->createKeyFrame( 2 ) );
        last->setPosition( Vector3<real_Num>( 4, 0, 0 ) );
        Array<SmartPtr<IAnimationKeyFrame>> input;
        input.reserve( 23 ); input.push_back( last ); input.push_back( zero );
        const auto oldPointer = track->getKeyFrames().data();
        budget.set( budget.used() );
        denied( [&] { track->setKeyFrames( input ); }, "imported keyframe capacity needs independent admission" );
        require( track->getKeyFrames().data() == oldPointer && track->getKeyFrame( 0 ) == zero,
                 "failed keyframe import preserves the sorted old generation" );
        budget.restore(); track->setKeyFrames( input );
        require( track->getKeyFrames().capacity() == 23 && track->getKeyFrames().get_allocator().isSourceBudgeted() &&
                     track->getKeyFrame( 0 ) == zero && track->getKeyFrame( 1 ) == last,
                 "keyframe imports canonicalize caller storage and sort before publication" );
        Array<f32> times; times.reserve( 19 ); times.push_back( 1 );
        const auto timesPointer = times.data();
        budget.set( budget.used() );
        denied( [&] { track->_collectKeyFrameTimes( times ); }, "caller key-time scratch needs admission" );
        require( times.data() == timesPointer && times.size() == 1 && times[0] == 1 &&
                     !times.get_allocator().isSourceBudgeted(), "denied caller scratch preserves external ownership" );
        budget.restore(); track->_collectKeyFrameTimes( times );
        require( times.get_allocator().isSourceBudgeted() && times.capacity() == 19 && times.size() == 3 &&
                     times[0] == 0 && times[1] == 1 && times[2] == 2,
                 "collected key times retain admitted caller capacity and sorted unique values" );
        auto index = make_ptr<AnimationTimeIndex>( 1.f );
        SmartPtr<IAnimationKeyFrame> sampled = zero;
        budget.set( budget.used() );
        denied( [&] { track->getInterpolatedKeyFrame( index, sampled ); }, "interpolated transform body requires admission" );
        require( sampled == zero, "denied interpolation preserves the caller's previous output" );
        budget.restore(); track->getInterpolatedKeyFrame( index, sampled );
        auto transformed = dynamic_pointer_cast<KeyFrameTransform3>( sampled );
        require( transformed && transformed->getPosition().x == 2,
                 "admitted midpoint interpolation preserves transform values" );
        auto snapshot = track->getKeyFrames();
        const auto snapshotBytes = arrayBytes<SmartPtr<IAnimationKeyFrame>>( 23 );
        const auto beforeUnload = budget.used();
        budget.set( beforeUnload );
        const auto epoch = sourceBudgetDenialCount();
        track->removeKeyFrame( 0 );
        track->unload( nullptr );
        require( sourceBudgetDenialCount() == epoch && track->getKeyFrames().capacity() == 0 &&
                     budget.used() == beforeUnload - snapshotBytes && snapshot.size() == 2 && snapshot[0] == zero,
                 "middle removal and full-capacity track unload retain escaped keys and retire owner storage without allocation" );
    }

    inline void numeric()
    {
        Capacity budget;
        auto track = make_ptr<AnimationNumericTrack>();
        Array<AnimationNumericTrack::KeyFrame> input;
        input.reserve( 29 ); input.push_back( {2, 20} ); input.push_back( {0, 0} );
        const auto pointer = track->getKeyFrames().data();
        budget.set( budget.used() );
        denied( [&] { track->setKeyFrames( input ); }, "numeric imported payload requires Source capacity" );
        require( track->getKeyFrames().data() == pointer && track->getNumKeyFrames() == 1,
                 "denied numeric import leaves the previous payload unchanged" );
        budget.restore(); track->setKeyFrames( input );
        require( track->getKeyFrames().capacity() == 29 && track->getKeyFrames().get_allocator().isSourceBudgeted() &&
                     track->getValueAt( 1 ) == 10, "numeric imports retain admitted spare capacity and interpolation" );
        auto snapshot = track->getKeyFrames();
        const auto resident = budget.used();
        budget.set( resident );
        track->addKeyFrame( 2, 30 );
        require( track->getValueAt( 1 ) == 15 && snapshot[1].second == 20,
                 "at-capacity numeric replacement preserves independent escaped payload copies" );
        track->unload( nullptr );
        require( budget.used() == resident - arrayBytes<AnimationNumericTrack::KeyFrame>( 29 ) &&
                     track->getKeyFrames().capacity() == 0 && snapshot.size() == 2,
                 "numeric full-capacity unload physically releases its owner payload" );
    }

    inline void meshClone()
    {
        // Warm manager instance/teardown inventories at the same simultaneous
        // source/clone ownership used by the physical accounting checks.
        {
            auto source = make_ptr<Mesh>();
            source->createAnimation( "budgetclip", 1 );
            auto clone = source->clone();
        }
        Capacity budget;
        const auto baseline = budget.used();
        u64 emptyIndexes = 0;
        {
            IAnimation::NodeTrackList nodes;
            IAnimation::NumericTrackList numerics;
            IAnimation::VertexTrackList vertices;
            emptyIndexes = budget.used() - baseline;
        }
        require( budget.used() == baseline, "empty animation index measurement retires all physical sentinels" );
        auto source = make_ptr<Mesh>();
        source->createAnimation( "budgetclip", 1 );
        const auto resident = budget.used();
        const auto meshBody = mesh_object_body_budget_contracts::managerBytes<Mesh>();
        const auto cloneBytes = meshBody + bodyBytes<Animation>() + emptyIndexes +
                                arrayBytes<SmartPtr<IAnimation>>( 1 );
        unsigned refused = 0, accepted = 0;
        for( u64 headroom : { 0ULL, 32ULL, 128ULL, 256ULL, 512ULL, 1024ULL, 2048ULL, 4096ULL, 16384ULL } )
        {
            budget.set( resident + headroom );
            SmartPtr<IMesh> clone;
            try
            {
                clone = source->clone();
                ++accepted;
                require( clone && clone->getNumAnimations() == 1 && budget.used() == resident + cloneBytes,
                         "mesh animation clone retains exactly its mesh/animation bodies, index sentinels and owner array" );
            }
            catch( const std::bad_alloc & )
            {
                ++refused;
                require( source->getNumAnimations() == 1 && budget.used() == resident,
                         "partial mesh animation clone denial retires every candidate allocation" );
            }
            budget.restore();
            clone = nullptr;
            require( budget.used() == resident, "released mesh animation clone leaves no transferred initial reference" );
        }
        require( refused >= 3 && accepted > 0, "mesh clone sweep covers partial bodies and animation index/copy admission" );
        auto clone = source->clone();
        auto pinned = clone->getAnimation( 0 );
        require( pinned && pinned->getReferences() == 2,
                 "cloned animation has one mesh owner plus the explicit caller pin" );
        pinned = nullptr;
        budget.set( budget.used() );
        clone->unload( nullptr );
        require( budget.used() == resident + meshBody && clone->getNumAnimations() == 0,
                 "full-capacity cloned mesh unload retires its last animation reference and payload metadata" );
        clone = nullptr;
        source->unload( nullptr ); source = nullptr;
        require( budget.used() == baseline, "original and cloned animation graphs physically retire at capacity" );
    }

    inline void run()
    {
        bodies(); inventories(); keyframes(); numeric(); meshClone();
    }
}
