#pragma once
#include <WPVehiclePhysics/CVehicleManager.hpp>
#include <Workphone/Vehicle/Vehicle.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <array>
#include <atomic>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>

namespace vehicle_manager_budget_contracts
{
    using namespace workphone;
    using Vehicle = vehicle::IVehicle;
    using List = Array<SmartPtr<Vehicle>>;
    using Snapshot = SharedPtr<List>;
    inline void require( bool condition, const char *message )
    { if( !condition ) throw std::runtime_error( message ); }
    struct Budget
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        static constexpr auto source = size_t( StreamMemoryClass::SourceCpu );
        u64 original = scheduler->stats().limits[source];
        u64 used() const { return scheduler->stats().used[source]; }
        void set( u64 value )
        {
            String error;
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, value, error ),
                     "vehicle manager Source budget applies" );
        }
        void restore() { set( original ); }
        ~Budget() { restore(); }
    };
    template<class F> inline void denied( F operation, const char *message )
    {
        bool rejected = false;
        try { operation(); } catch( const std::bad_alloc & ) { rejected = true; }
        require( rejected, message );
    }
    template<class T> inline u64 bytes( size_t count )
    { return count ? sourceBudgetStorageBytes( count * sizeof(T), alignof(T) ) : 0; }
    struct ListOwnerLayout : List, SourceBudgetObject {};
    inline u64 listBody()
    { return sourceBudgetStorageBytes( sizeof(ListOwnerLayout), alignof(std::max_align_t) ); }
    inline u64 listControl()
    {
        return sourceBudgetStorageBytes( sizeof(SharedPtrControlBlockImpl<ListOwnerLayout>),
                                         alignof(std::max_align_t) );
    }
    inline u64 metadata() { return listBody() + listControl(); }
    class Manager : public CVehicleManager
    {
    public:
        Snapshot snapshot() const { return getVehicles(); }
        void replace( Snapshot value ) { setVehicles( std::move( value ) ); }
        void clear() { setVehicles( nullptr ); }
    };
    class VehicleProbe : public vehicle::Vehicle, public SourceBudgetObject
    {
    public:
        using SourceBudgetObject::operator new;
        using SourceBudgetObject::operator delete;
        Manager *owner = nullptr;
        bool *destroyed = nullptr;
        bool *detached = nullptr;
        bool clearOnUpdate = false;
        int pre = 0, updates = 0, post = 0;
        ~VehicleProbe() override
        {
            if( destroyed ) *destroyed = true;
            if( detached ) *detached = owner && !owner->snapshot();
        }
        void preUpdate() override { ++pre; }
        void update() override { ++updates; if( clearOnUpdate && owner ) owner->clear(); }
        void postUpdate() override { ++post; }
    };

    inline void bodies()
    {
        Budget budget;
        // Isolate the list/control and direct-manager body from reusable common
        // IObject/ISharedObject pool slabs, already covered by core contracts.
        std::array<std::optional<Manager>, 4> warm;
        for( auto &manager : warm ) manager.emplace();
        for( auto &manager : warm ) manager.reset();
        const auto baseline = budget.used();
        for( const auto room : { listBody() - 1, listBody(), metadata() - 1 } )
        {
            budget.set( baseline + room );
            denied( [] { Manager candidate; }, "vehicle snapshot body and typed control each require admission" );
            require( budget.used() == baseline, "partial vehicle-manager construction unwinds body and control" );
        }
        budget.set( baseline + metadata() );
        {
            Manager manager;
            require( manager.snapshot() && manager.snapshot()->get_allocator().isSourceBudgeted() &&
                         budget.used() == baseline + metadata(),
                     "empty manager owns exactly its admitted snapshot body and typed Source control" );
            budget.set( budget.used() ); const auto denials = sourceBudgetDenialCount();
            auto retained = manager.snapshot();
            manager.unload( nullptr );
            require( !manager.snapshot() && budget.used() == baseline + metadata(),
                     "unload detaches the manager while a strong snapshot retains its physical owners" );
            manager.preUpdate(); manager.update(); manager.postUpdate();
            retained.reset();
            require( budget.used() == baseline && sourceBudgetDenialCount() == denials,
                     "last empty snapshot release destroys the typed body/control without admission" );
        }
        const auto body = sourceBudgetStorageBytes( sizeof(Manager), alignof(std::max_align_t) );
        budget.set( baseline + body + metadata() - 1 );
        denied( [] { auto manager = std::make_unique<Manager>(); },
                "admitted direct manager body unwinds on inner snapshot construction denial" );
        require( budget.used() == baseline, "failed direct manager construction has no remaining body charge" );
        budget.set( baseline + body + metadata() );
        auto manager = std::make_unique<Manager>();
        require( budget.used() == baseline + body + metadata(), "direct manager heap body participates in Source admission" );
        auto retained = manager->snapshot();
        budget.set( budget.used() ); const auto denials = sourceBudgetDenialCount();
        manager.reset();
        require( budget.used() == baseline + metadata(), "snapshot ownership survives physical manager destruction" );
        retained.reset();
        require( budget.used() == baseline && sourceBudgetDenialCount() == denials,
                 "final manager/snapshot retirement returns every physical body and control charge" );
    }

    inline void copyOnWrite()
    {
        Budget budget;
        Manager manager; manager.clear();
        auto a = make_ptr<VehicleProbe>(), b = make_ptr<VehicleProbe>();
        auto foreign = workphone::make_shared<List>();
        foreign->reserve( 37 ); foreign->push_back( a ); foreign->push_back( b );
        const auto baseline = budget.used();
        const auto buffer = bytes<SmartPtr<Vehicle>>( foreign->capacity() );
        const auto incoming = metadata() + buffer;
        for( const auto room : { u64(0), listBody(), listBody() + buffer, incoming - 1 } )
        {
            budget.set( baseline + room );
            denied( [&] { manager.replace( foreign ); }, "foreign vehicle setter stages admitted body, buffer and control" );
            require( !manager.snapshot() && budget.used() == baseline && foreign->size() == 2,
                     "denied foreign import preserves manager and caller ownership" );
        }
        budget.set( baseline + incoming ); manager.replace( foreign );
        auto original = manager.snapshot();
        require( original && original.get() != foreign.get() && original->capacity() == 37 &&
                     original->get_allocator().isSourceBudgeted() && budget.used() == baseline + incoming,
                 "vehicle setter normalizes foreign body/control/buffer and preserves spare capacity" );
        foreign->front() = b;
        require( original->front() == a, "foreign array mutation cannot change a published vehicle generation" );
        budget.set( budget.used() ); const auto selfDenials = sourceBudgetDenialCount();
        manager.replace( original ); manager.removeVehicle( nullptr );
        require( manager.snapshot() == original && sourceBudgetDenialCount() == selfDenials,
                 "self publication and absent removal need no new allocation at the cap" );
        const auto resident = budget.used();
        for( const auto room : { u64(0), listBody(), listBody() + buffer, incoming - 1 } )
        {
            budget.set( resident + room );
            denied( [&] { manager.addVehicle( a ); }, "vehicle add requires a complete COW replacement generation" );
            require( manager.snapshot() == original && original->size() == 2 && budget.used() == resident,
                     "denied vehicle add preserves the last published identity, order and residency" );
            denied( [&] { manager.removeVehicle( a ); }, "vehicle removal requires a complete COW replacement generation" );
            require( manager.snapshot() == original && original->size() == 2 && budget.used() == resident,
                     "denied vehicle removal preserves the complete published generation" );
        }
        budget.set( resident + incoming ); manager.removeVehicle( a );
        auto removed = manager.snapshot();
        require( removed != original && removed->size() == 1 && removed->front() == b &&
                     removed->capacity() == 37 && original->size() == 2,
                 "middle removal stages directly into one admitted buffer and retains old caller snapshots" );
        budget.restore(); manager.addVehicle( a );
        auto added = manager.snapshot();
        require( added != removed && added->size() == 2 && added->front() == b && added->back() == a &&
                     added->capacity() == 37 && removed->size() == 1,
                 "admitted COW add preserves ordering, retained snapshots and spare capacity" );
        budget.set( budget.used() ); const auto retireDenials = sourceBudgetDenialCount();
        manager.unload( nullptr ); original.reset(); removed.reset(); added.reset();
        require( budget.used() == baseline && sourceBudgetDenialCount() == retireDenials,
                 "COW generations retire complete storage at the cap while foreign owners remain independent" );

        budget.restore();
        *foreign = List( foreign->get_allocator() );
        foreign->reserve( 1 ); foreign->push_back( a );
        manager.replace( foreign ); auto full = manager.snapshot();
        const auto fullResident = budget.used();
        const auto clone = metadata() + bytes<SmartPtr<Vehicle>>( 1 );
        budget.set( fullResident + clone );
        denied( [&] { manager.addVehicle( b ); }, "post-clone append growth obtains overlapping Source admission" );
        require( manager.snapshot() == full && full->size() == 1 && budget.used() == fullResident,
                 "append-growth failure after admitted COW metadata leaves the original generation untouched" );
        budget.restore(); manager.addVehicle( b );
        require( manager.snapshot()->size() == 2 && full->size() == 1,
                 "full-inventory append retries after Source headroom returns" );
        budget.set( budget.used() ); manager.clear(); full.reset();
        require( budget.used() == baseline, "append retry and retained old generation release all manager storage" );
    }

    inline void callbacksAndConcurrency()
    {
        Budget budget;
        Manager manager;
        auto a = make_ptr<VehicleProbe>(), b = make_ptr<VehicleProbe>();
        a->owner = &manager; a->clearOnUpdate = true;
        manager.addVehicle( a ); manager.addVehicle( b );
        auto retained = manager.snapshot();
        budget.set( budget.used() ); const auto denials = sourceBudgetDenialCount();
        manager.update();
        require( !manager.snapshot() && a->updates == 1 && b->updates == 1 && retained->size() == 2 &&
                     sourceBudgetDenialCount() == denials,
                 "callback-driven detach does not invalidate the captured iteration or allocate under the cap" );
        retained.reset();
        budget.restore(); a->clearOnUpdate = false;
        std::atomic<bool> failed{ false };
        auto append = [&] { try { for( size_t i = 0; i < 32; ++i ) manager.addVehicle( a ); }
                           catch(...) { failed.store( true ); } };
        std::thread first( append ), second( append ); first.join(); second.join();
        auto all = manager.snapshot();
        require( !failed.load() && all && all->size() == 64 && all->get_allocator().isSourceBudgeted(),
                 "serialized read-copy-publish mutations retain both concurrent append sequences" );
        manager.clear(); all.reset();

        bool destroyed = false, detached = false;
        auto finalVehicle = make_ptr<VehicleProbe>();
        finalVehicle->owner = &manager; finalVehicle->destroyed = &destroyed; finalVehicle->detached = &detached;
        const auto beforeGeneration = budget.used();
        manager.addVehicle( finalVehicle ); auto finalSnapshot = manager.snapshot();
        finalVehicle = nullptr;
        budget.set( budget.used() ); const auto beforeRelease = sourceBudgetDenialCount();
        manager.clear();
        require( !destroyed && finalSnapshot->size() == 1,
                 "strong caller snapshots retain actual vehicle owners after manager detachment" );
        std::thread lastOwner( [snapshot = std::move( finalSnapshot )]() mutable { snapshot.reset(); } );
        lastOwner.join();
        require( destroyed && detached && sourceBudgetDenialCount() == beforeRelease && budget.used() < beforeGeneration,
                 "cross-thread last snapshot release destroys its Source body/control and vehicle after visible detachment" );
    }
    inline void run() { bodies(); copyOnWrite(); callbacksAndConcurrency(); }
}
