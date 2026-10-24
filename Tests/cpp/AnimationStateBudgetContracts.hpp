#pragma once

#include <Workphone/Animation/AnimationInterface.hpp>
#include <Workphone/Animation/Animation.hpp>
#include <Workphone/Animation/AnimationEventTimeline.hpp>
#include <Workphone/Animation/AnimationSyncTrack.hpp>
#include <Workphone/Interface/Animation/IAnimationState.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <array>
#include <memory>
#include <stdexcept>

namespace animation_state_budget_contracts
{
    using namespace workphone;
    using namespace workphone::animation;

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
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, value, error ),
                     "animation-state capacity applies" );
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
    { return capacity ? sourceBudgetStorageBytes( capacity * sizeof(T), alignof(T) ) : 0; }
    template<class T> inline u64 bodyBytes()
    { return sourceBudgetStorageBytes( sizeof(T), alignof(std::max_align_t) ); }

    inline AnimationEvent event( const char *name, f32 time, f32 duration = 0.5f )
    {
        AnimationEvent value;
        value.id = name;
        value.payload = "payload";
        value.startTime = time;
        value.duration = duration;
        value.isSyncMarker = true;
        return value;
    }

    inline void masksAndBodies()
    {
        Capacity budget;
        const auto baseline = budget.used();
        {
            IAnimationState::BoneBlendMask mask;
            budget.set( baseline + arrayBytes<float>(7) - 1 );
            denied( [&] { mask.reserve(7); }, "bone mask admits its physical float buffer" );
            require( mask.empty() && !mask.capacity() && budget.used() == baseline,
                     "denied cold mask has no retained charge" );
            budget.set( baseline + arrayBytes<float>(7) );
            mask.reserve(7);
            mask.assign( 3, 0.25f );
            require( mask.capacity() == 7 && budget.used() == baseline + arrayBytes<float>(7),
                     "bone mask spare slots remain charged" );
            const auto owned = budget.used();
            budget.set( owned + arrayBytes<float>(17) - 1 );
            denied( [&] { mask.reserve(17); }, "bone mask growth includes the old/new overlap" );
            require( mask.size() == 3 && mask.capacity() == 7 && mask[1] == 0.25f &&
                     budget.used() == owned, "denied bone mask growth preserves all state" );
            budget.set( owned );
            denied( [&] { auto copy = mask; }, "bone mask copies keep Source allocation policy" );
            require( mask.size() == 3 && budget.used() == owned,
                     "denied bone mask copy preserves the original buffer" );
            mask.clear();
            require( budget.used() == owned, "clearing a mask retains its physical spare slots" );
        }
        require( budget.used() == baseline, "full-capacity mask destruction retires the buffer" );
        budget.set( baseline );
        denied( [] { auto timeline = std::make_unique<AnimationEventTimeline>(); },
                "dynamic timeline body requires Source admission" );
        denied( [] { auto state = make_ptr<AnimationInterface>(); },
                "animation interface body requires Source admission" );
        budget.set( baseline + bodyBytes<AnimationInterface>() );
        denied( [] { auto state = make_ptr<AnimationInterface>(); },
                "interface empty-map sentinel denial unwinds its admitted body" );
        require( budget.used() == baseline, "partial interface construction retires its body" );
        budget.set( baseline + bodyBytes<AnimationSyncTrack>() );
        denied( [] { auto sync = std::make_unique<AnimationSyncTrack>(); },
                "sync default-marker denial unwinds its admitted body" );
        require( budget.used() == baseline, "partial sync construction leaves no body lease" );
        budget.restore();
        {
            auto timeline = std::make_unique<AnimationEventTimeline>();
            require( budget.used() == baseline + bodyBytes<AnimationEventTimeline>(),
                     "an empty dynamic timeline charges its actual body once" );
            budget.set( budget.used() );
        }
        require( budget.used() == baseline, "timeline body retires at a full cap" );
    }

    inline void timelineImportsAndEdits()
    {
        Capacity budget;
        const auto baseline = budget.used();
        {
            Array<AnimationEvent> raw;
            raw.reserve(7);
            raw.push_back( event("later", 0.4f) );
            raw.push_back( event("earlier", 0.1f) );
            AnimationEventTimeline timeline;
            timeline.addTrack("borrowed");
            timeline.getTrack(0)->events = raw;
            const auto ownerOnly = budget.used();
            const auto outerCapacity = timeline.getTracks().capacity();
            require( !timeline.getTrack(0)->events.get_allocator().isSourceBudgeted(),
                     "fixture begins with an explicitly borrowed raw event inventory" );
            AnimationEventTrack trackCopy;
            budget.set( ownerOnly + arrayBytes<AnimationEvent>(7) - 1 );
            denied( [&] { trackCopy = *timeline.getTrack(0); },
                    "track copy admits borrowed spare capacity before assignment" );
            require( trackCopy.events.empty() && budget.used() == ownerOnly,
                     "denied track copy preserves its destination and source" );
            budget.set( ownerOnly + arrayBytes<AnimationEvent>(7) );
            trackCopy = *timeline.getTrack(0);
            require( trackCopy.events.get_allocator().isSourceBudgeted() &&
                     trackCopy.events.capacity() == 7 && trackCopy.events[0].id == "later" &&
                     budget.used() == ownerOnly + arrayBytes<AnimationEvent>(7),
                     "track copy normalizes borrowed events with the actual spare capacity" );
            trackCopy = AnimationEventTrack{};
            require( budget.used() == ownerOnly, "track replacement physically frees old event capacity" );

            AnimationEventTimeline destination;
            destination.addTrack("unchanged");
            const auto beforeCopy = budget.used();
            const auto copyCost = arrayBytes<AnimationEventTrack>(outerCapacity) +
                                  arrayBytes<AnimationEvent>(7);
            budget.set( beforeCopy + copyCost - 1 );
            denied( [&] { destination = timeline; }, "nested timeline copy is admitted atomically" );
            require( destination.getNumTracks() == 1 && destination.getTrack(0)->name == "unchanged" &&
                     destination.getTrack(0)->events.empty() && budget.used() == beforeCopy,
                     "partial nested copy denial retires the staged outer and inner arrays" );
            budget.set( beforeCopy + copyCost );
            destination = timeline;
            require( destination.getTrack(0)->events.get_allocator().isSourceBudgeted() &&
                     destination.getTrack(0)->events.capacity() == 7,
                     "timeline copy canonicalizes every nested event inventory" );
            destination = AnimationEventTimeline{};
            require( budget.used() == ownerOnly, "timeline assignment retires all previous capacities" );

            const auto first = event("new", 0.25f);
            const auto rawData = timeline.getTrack(0)->events.data();
            budget.set( ownerOnly );
            denied( [&] { timeline.addEvent(0, first); },
                    "an authored edit cannot silently retain an imported raw event array" );
            denied( [&] { timeline.updateEvent(0, 0, first); },
                    "an event update stages raw import admission before replacement" );
            require( timeline.getTrack(0)->events.data() == rawData &&
                     timeline.getTrack(0)->events.size() == 2 &&
                     timeline.getTrack(0)->events[0].id == "later" && budget.used() == ownerOnly,
                     "denied authored edits preserve the borrowed array, order, and charge" );
            budget.set( ownerOnly + arrayBytes<AnimationEvent>(7) );
            require( timeline.addEvent(0, first) == 1, "source import sorts events stably on commit" );
            const auto committed = budget.used();
            require( timeline.getTrack(0)->events.get_allocator().isSourceBudgeted() &&
                     timeline.getTrack(0)->events.capacity() == 7 && raw.size() == 2 &&
                     raw[0].id == "later", "successful edit owns its Source copy without modifying caller storage" );
            budget.set( committed );
            require( timeline.addEvent(0, timeline.getTrack(0)->events[1]) == 1,
                     "aliased event insertion remains valid while stable sorting" );
            require( timeline.removeEvent(0, 1) && timeline.getTrack(0)->events.capacity() == 7 &&
                     budget.used() == committed, "in-place event removal retains physical spare slots at the cap" );
            require( timeline.removeTrack(0) && timeline.getTracks().capacity() == outerCapacity &&
                     budget.used() == ownerOnly, "full-cap track removal frees nested storage without rebuilding outer storage" );
            timeline.clear();
            require( budget.used() == ownerOnly, "timeline clear retains the actual empty outer capacity" );
            budget.set( budget.used() );
        }
        require( budget.used() == baseline, "timeline final destruction frees every retained capacity" );
    }

    inline void sampledSnapshots()
    {
        Capacity budget;
        const auto baseline = budget.used();
        auto retained = makeSourceBudgetSnapshot<SampledAnimationEvent>();
        {
            AnimationEventTimeline timeline;
            timeline.addTrack("events");
            timeline.addEvent(0, event("first", 0.1f));
            timeline.addEvent(0, event("second", 0.3f));
            retained = timeline.getActiveEvents(0.35f);
            require( retained.size() == 2 && retained.get_allocator().isSourceBudgeted(),
                     "active event snapshots carry Source policy" );
            retained.reserve(9);
            AnimationSyncTrack sync;
            const auto before = budget.used();
            const auto pointer = retained.data();
            budget.set( before );
            denied( [&] { retained = timeline.sampleRange(0.0f, 0.9f); },
                    "denied sample scratch leaves caller output unchanged" );
            require( retained.data() == pointer && retained.size() == 2 && retained.capacity() == 9 &&
                     retained[0].event.id == "first" && budget.used() == before,
                     "sample failure preserves retained caller data and spare-capacity charge" );
            denied( [&] { sync = AnimationSyncTrack::fromTimeline(timeline); },
                    "timeline-to-sync conversion admits its detached intermediate snapshots" );
            require( sync.getNumEvents() == 1 && sync.getMarkers()[0].id == "Default" &&
                     budget.used() == before, "denied timeline conversion preserves caller sync output" );
            budget.restore();
            sync = AnimationSyncTrack::fromTimeline(timeline);
            require( sync.getNumEvents() == 2 && sync.getMarkers()[0].id == "first" &&
                     sync.getMarkers().get_allocator().isSourceBudgeted(),
                     "timeline conversion produces an admitted owned sync inventory" );
            auto samples = timeline.sampleRange(0.0f, 0.9f);
            require( samples.get_allocator().isSourceBudgeted() && samples.size() == 4 &&
                     samples[0].event.id == "first" && samples[0].phase == AnimationEventPhase::Started &&
                     samples[2].phase == AnimationEventPhase::Ended,
                     "admitted sample output keeps event order and phases" );
            auto markers = timeline.getSyncMarkers();
            require( markers.get_allocator().isSourceBudgeted() && markers.size() == 2,
                     "sync event snapshots admit their detached array" );
        }
        require( budget.used() == baseline + arrayBytes<SampledAnimationEvent>(9) &&
                 retained[1].event.id == "second", "caller snapshot survives owner retirement with its spare capacity charged" );
        const auto beforeCopy = budget.used();
        budget.set( beforeCopy + arrayBytes<SampledAnimationEvent>(9) - 1 );
        denied( [&] { auto copy = retained; }, "detached snapshot copies retain Source spare-capacity admission" );
        require( budget.used() == beforeCopy, "denied snapshot copy has no residual lease" );
        budget.set( beforeCopy );
        retained = makeSourceBudgetSnapshot<SampledAnimationEvent>();
        require( budget.used() == baseline, "full-cap caller snapshot retirement frees its entire physical capacity" );
    }

    inline void syncImports()
    {
        Capacity budget;
        const auto baseline = budget.used();
        auto retained = makeSourceBudgetSnapshot<AnimationSyncMarker>();
        {
            Array<AnimationSyncMarker> raw;
            raw.reserve(11);
            raw.push_back({"last",0.7f});
            raw.push_back({"first",0.1f});
            raw.push_back({"middle",0.4f});
            AnimationSyncTrack sync;
            sync.setStartEventOffset(0);
            const auto before = budget.used();
            budget.set( before + arrayBytes<AnimationSyncMarker>(11) - 1 );
            denied( [&] { sync.setMarkers(raw); }, "sync imports admit caller spare capacity before commit" );
            require( sync.getMarkers().size() == 1 && sync.getMarkers()[0].id == "Default" &&
                     sync.getStartEventOffset() == 0 && budget.used() == before,
                     "denied sync import preserves the previous marker inventory and offset" );
            budget.set( before + arrayBytes<AnimationSyncMarker>(11) );
            require( sync.setMarkers(raw), "exact sync import overlap admission succeeds" );
            require( sync.getMarkers().get_allocator().isSourceBudgeted() && sync.getMarkers().capacity() == 11 &&
                     sync.getMarkers()[0].id == "first" && raw[0].id == "last" &&
                     budget.used() == before - arrayBytes<AnimationSyncMarker>(1) + arrayBytes<AnimationSyncMarker>(11),
                     "sync import owns sorted Source storage with actual raw spare capacity" );
            sync.setStartEventOffset(2);
            budget.restore();
            Array<AnimationSyncMarker> invalid;
            invalid.push_back({"duplicate-a",0.2f});
            invalid.push_back({"duplicate-b",0.2f});
            const auto committed = budget.used();
            require( !sync.setMarkers(invalid) && sync.getMarkers().size() == 3 &&
                     sync.getStartEventOffset() == 2 && budget.used() == committed,
                     "invalid staged sync import releases its candidate and preserves offset" );
            budget.set( committed + arrayBytes<AnimationSyncMarker>(11) - 1 );
            denied( [&] { auto copy = sync; }, "sync copies retain Source spare-capacity admission" );
            require( budget.used() == committed, "denied sync copy leaves no detached allocation" );
            budget.restore();
            retained = sync.getMarkers();
            require( retained.capacity() == 11 && retained.get_allocator().isSourceBudgeted(),
                     "detached sync snapshots preserve physical spare capacity and Source policy" );
            budget.set( budget.used() );
        }
        require( budget.used() == baseline + arrayBytes<AnimationSyncMarker>(11) &&
                 retained[0].id == "first", "retained sync snapshots survive full-cap owner destruction" );
        budget.set( budget.used() );
        retained = makeSourceBudgetSnapshot<AnimationSyncMarker>();
        require( budget.used() == baseline, "full-cap sync snapshot retirement frees the actual spare capacity" );
    }

    inline void interfaceInventories()
    {
        // Warm shared object metadata before isolating animation bodies and
        // the concrete standard-library map/name inventory allocations.
        { auto warm = make_ptr<AnimationInterface>(); warm->createAnimation("warm",1); }
        Capacity budget;
        u64 secondDelta = 0;
        {
            AnimationInterface probe;
            probe.createAnimation("first",1);
            const auto first = budget.used();
            probe.createAnimation("second",2);
            secondDelta = budget.used() - first;
        }
        require( secondDelta > bodyBytes<Animation>(), "second animation includes body, track-map heads, map node and name growth" );
        const std::array<u64,6> gates{{0, bodyBytes<Animation>() - 1, bodyBytes<Animation>(),
                                      secondDelta - 1, secondDelta,
                                      secondDelta + arrayBytes<String>(1)}};
        size_t failures = 0, successes = 0;
        for( const auto gate : gates )
        {
            budget.restore();
            const auto baseline = budget.used();
            {
                AnimationInterface owner;
                auto retained = owner.createAnimation("first",1);
                const auto before = budget.used();
                budget.set( before + gate );
                try
                {
                    auto added = owner.createAnimation("second",2);
                    ++successes;
                    require( added && owner.getNumAnimations() == 2 && owner.getAnimation(u16(1)) == added &&
                             budget.used() == before + secondDelta,
                             "admitted interface publication updates both inventories together" );
                }
                catch( const std::bad_alloc & )
                {
                    ++failures;
                    require( owner.getNumAnimations() == 1 && !owner.hasAnimation("second") &&
                             owner.getAnimation(u16(0)) == retained && budget.used() == before,
                             "denied interface publication rolls back body, nodes, names and indexing" );
                }
                budget.restore();
                auto replacement = owner.createAnimation("first",3);
                require( owner.getAnimation(u16(0)) == replacement && owner.getNumAnimations() <= 2 &&
                         owner.getAnimation(u16(owner.getNumAnimations())) == nullptr &&
                         retained->getLength() == 1,
                         "replacing a named animation preserves one index and retained old owners" );
                const auto beforeClone = budget.used();
                budget.set( beforeClone );
                denied( [&] { auto copy = owner.clone(); }, "interface clone body denial preserves the original inventory" );
                require( budget.used() == beforeClone, "denied interface clone has no retained storage" );
                owner.removeAnimation("first");
                require( owner.getAnimation(u16(0)) != replacement,
                         "full-cap removal shifts ordered names without new allocation" );
                owner.unload(nullptr);
                require( owner.getNumAnimations() == 0 && !owner.getAnimation(u16(0)) &&
                         retained->getLength() == 1 && replacement->getLength() == 3,
                         "full-cap interface unload retires inventories while retaining external animation owners" );
                budget.set( budget.used() );
            }
            require( budget.used() == baseline, "interface, retained animations and map sentinels retire on final destruction" );
        }
        require( failures >= 4 && successes >= 1,
                 "interface fixture exercises body, nested metadata and final publication gates" );
    }

    inline void run()
    {
        masksAndBodies();
        timelineImportsAndEdits();
        sampledSnapshots();
        syncImports();
        interfaceInventories();
    }
}
