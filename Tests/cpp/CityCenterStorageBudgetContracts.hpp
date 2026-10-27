#pragma once
#include "CityInventoryBudgetContracts.hpp"
#include <WPProcedural/CityCenter.hpp>

namespace city_center_storage_budget_contracts
{
    using namespace city_inventory_budget_contracts;

    // CityCenter's name/transform adapters remain abstract; inventory storage is
    // tested without changing those existing interface responsibilities.
    class Center : public CityCenter
    {
    public:
        String getName() const override { return {}; }
        void setName( const String & ) override {}
        Transform3<real_Num> getTransform() const override { return getWorldTransform(); }
        void setTransform( Transform3<real_Num> value ) override { setWorldTransform( value ); }
    };

    class RetiringCenterNode : public CRoadNode
    {
    public:
        Center *center = nullptr;
        int calls = 0;
        bool detached = false;
        void unload( SmartPtr<ISharedObject> ) override
        {
            ++calls;
            detached = center && center->getRoads().empty() && center->getRoads().capacity() == 0 &&
                       center->getRoads().get_allocator().isSourceBudgeted() && center->getNodes().empty() &&
                       !center->getProperties();
            center->unload( nullptr );
            throw std::bad_alloc();
        }
    };

    inline u64 storageBytes( size_t capacity )
    {
        return sourceBudgetStorageBytes( capacity * sizeof( Handle ), alignof( Handle ) );
    }

    inline void setMetadata( Handle &handle, u8 seed )
    {
        workphone::UUID object, definition, file;
        object[0] = seed; object[15] = 1;
        definition[0] = seed; definition[15] = 2;
        file[0] = seed; file[15] = 3;
        handle.setUUID( object ); handle.setClassId( definition ); handle.setFileId( file );
        handle.setInstanceId( 1000 + seed );
    }

    inline bool sameHandle( const Handle &value, const Handle &expected )
    {
        return value.getHash() == expected.getHash() && value.getId() == expected.getId() &&
               value.getInstanceId() == expected.getInstanceId() && value.getUUID() == expected.getUUID() &&
               value.getClassId() == expected.getClassId() && value.getFileId() == expected.getFileId();
    }

    template<class T> inline void bodyAdmission( Budget &budget )
    {
        static_assert( std::is_base_of_v<SourceBudgetObject, T> );
        const auto baseline = budget.used();
        const auto bytes = sourceBudgetStorageBytes( sizeof( T ), alignof( std::max_align_t ) );
        budget.set( baseline + bytes - 1 );
        denied( []{
            auto memory = T::operator new( sizeof( T ) );
            T::operator delete( memory );
        } );
        check( budget.used() == baseline, "Denied city-center body allocation publishes no lease" );
        budget.set( baseline + bytes );
        // Exercise the class allocation policy directly, isolating the body
        // from ISharedObject/Properties construction and borrowed placement slots.
        auto release = []( void *memory ) { T::operator delete( memory ); };
        std::unique_ptr<void, decltype( release )> memory( T::operator new( sizeof( T ) ), release );
        check( budget.used() == baseline + bytes,
               "City-center body allocation requires its exact physical SourceCpu charge" );
        budget.set( budget.used() ); memory.reset();
        check( budget.used() == baseline, "City-center body storage retires at the full cap" );
        budget.set( budget.original );
    }

    inline void run()
    {
        Budget budget;
        bodyAdmission<CityCenter>( budget );
        bodyAdmission<CityCenter::CityCenterRecord>( budget );
        Handle first, second, duplicate, match, absent;
        first.setHash( 11 ); first.setId( 101 );
        second.setHash( 22 ); second.setId( 202 );
        duplicate.setHash( 11 ); duplicate.setId( 303 );
        match.setHash( 11 ); match.setId( 999 ); absent.setHash( 33 );
        setMetadata( first, 1 ); setMetadata( second, 2 ); setMetadata( duplicate, 3 );
        setMetadata( match, 4 ); setMetadata( absent, 5 );
        Array<Handle> foreign;
        foreign.reserve( 137 );
        foreign.push_back( first ); foreign.push_back( second ); foreign.push_back( duplicate );
        check( sameHandle( foreign[0], first ) && sameHandle( foreign[1], second ) &&
               sameHandle( foreign[2], duplicate ), "Foreign handle values retain UUID, class, file and instance metadata" );

        {
            const auto baseline = budget.used(); budget.set( baseline );
            Handle copy( first ), assigned;
            assigned = second; assigned = first; assigned = assigned;
            check( sameHandle( copy, first ) && sameHandle( assigned, first ) && budget.used() == baseline,
                   "Fixed-body Handle copy, assignment and self-assignment preserve all metadata at the full cap" );
            budget.set( budget.original );
        }

        {
            Center center;
            check( center.getRadius() == 0 && center.getRoads().get_allocator().isSourceBudgeted(),
                   "City-center radius and empty handle inventory have deterministic defaults" );
            const auto emptyBaseline = budget.used();
            budget.set( emptyBaseline );
            denied( [&]{ center.addRoad( first ); } );
            check( center.getRoads().empty() && budget.used() == emptyBaseline,
                   "City-center append requires admission before publishing a handle" );
            budget.set( budget.original ); center.addRoad( absent );
            const auto oldCapacity = center.getRoads().capacity();
            const auto oldData = center.getRoads().data();
            const auto baseline = budget.used();
            const auto bytes = storageBytes( foreign.capacity() );
            budget.set( baseline + bytes - 1 );
            denied( [&]{ center.setRoads( foreign ); } );
            check( center.getRoads().size() == 1 && center.getRoads()[0].getHash() == 33 &&
                   center.getRoads().data() == oldData && budget.used() == baseline,
                   "One-byte-short foreign handle replacement preserves the previous inventory" );
            budget.set( baseline + bytes ); center.setRoads( foreign );
            check( center.getRoads().capacity() == foreign.capacity() && center.getRoads().size() == 3 &&
                   center.getRoads().get_allocator().isSourceBudgeted() &&
                   sameHandle( center.getRoads()[0], first ) && sameHandle( center.getRoads()[1], second ) &&
                   sameHandle( center.getRoads()[2], duplicate ) &&
                   budget.used() == baseline + bytes - storageBytes( oldCapacity ),
                   "City-center import preserves complete handle metadata and admits foreign spare capacity at the exact cap" );

            budget.set( budget.original );
            auto incoming = makeSourceBudgetSnapshot( foreign );
            const auto incomingData = incoming.data();
            const auto beforeMove = budget.used();
            budget.set( beforeMove ); center.setRoads( std::move( incoming ) );
            check( incoming.empty() && center.getRoads().data() == incomingData &&
                   center.getRoads().capacity() == foreign.capacity() &&
                   sameHandle( center.getRoads()[0], first ) && sameHandle( center.getRoads()[2], duplicate ) &&
                   budget.used() == beforeMove - bytes,
                   "Already admitted city-center handle rvalues transfer their physical buffer at the cap" );

            auto emptyNodes = center.getRoadNodesFromCell( Vector2I( 0,0 ) );
            check( emptyNodes.empty() && emptyNodes.get_allocator().isSourceBudgeted(),
                   "Unimplemented spatial lookup returns an empty SourceCpu caller container" );
            budget.set( budget.used() );
            denied( [&]{ emptyNodes.reserve( 1 ); } );
            budget.set( budget.original );
        }
        {
            Center center;
            center.setRoads( foreign );
            const auto capacity = center.getRoads().capacity();
            const auto bytes = storageBytes( capacity );
            const auto baseline = budget.used();
            const auto oldData = center.getRoads().data();
            budget.set( baseline ); center.removeRoad( absent );
            check( budget.used() == baseline && center.getRoads().data() == oldData,
                   "Removing an absent city-center handle requires no replacement admission" );
            denied( [&]{ center.removeRoad( match ); } );
            check( center.getRoads().size() == 3 && center.getRoads()[0].getId() == 101 &&
                   center.getRoads()[2].getId() == 303 && center.getRoads().data() == oldData &&
                   sameHandle( center.getRoads()[0], first ) && sameHandle( center.getRoads()[1], second ) &&
                   sameHandle( center.getRoads()[2], duplicate ) &&
                   budget.used() == baseline,
                   "Denied city-center handle removal preserves the complete original inventory" );
            budget.set( baseline + bytes ); center.removeRoad( match );
            check( center.getRoads().size() == 2 && center.getRoads()[0].getId() == 202 &&
                   center.getRoads()[1].getId() == 303 && center.getRoads().capacity() == capacity &&
                   sameHandle( center.getRoads()[0], second ) && sameHandle( center.getRoads()[1], duplicate ) &&
                   center.getRoads().get_allocator().isSourceBudgeted() && budget.used() == baseline,
                   "Handle removal uses first hash match while preserving survivor metadata and admitted spare capacity" );
            budget.set( budget.original );
            center.removeRoad( second ); center.removeRoad( match );
            check( center.getRoads().empty() && center.getRoads().capacity() == capacity,
                   "Logical city-center removal retains its paid physical buffer" );
            const auto beforeUnload = budget.used();
            budget.set( beforeUnload ); center.unload( nullptr );
            check( center.getRoads().capacity() == 0 && center.getRoads().get_allocator().isSourceBudgeted() &&
                   budget.used() <= beforeUnload - bytes,
                   "City-center unload physically retires empty spare storage at the full cap" );
            budget.set( budget.original );
        }
        {
            Center center;
            // Explicit mutable-reference allocator replacement remains a caller
            // boundary; the owning setter imports it back into SourceCpu storage.
            center.getRoads() = foreign;
            check( !center.getRoads().get_allocator().isSourceBudgeted(),
                   "Legacy mutable city-center inventory references preserve caller allocator compatibility" );
            const auto baseline = budget.used();
            budget.set( baseline );
            denied( [&]{ center.setRoads( center.getRoads() ); } );
            check( center.getRoads().size() == 3 && !center.getRoads().get_allocator().isSourceBudgeted(),
                   "Denied re-admission preserves explicitly substituted caller storage" );
            budget.set( budget.original ); center.setRoads( center.getRoads() );
            check( center.getRoads().get_allocator().isSourceBudgeted() && center.getRoads().capacity() == 137,
                   "Owning setter restores SourceCpu policy after explicit foreign substitution" );
        }
        {
            Center center;
            center.setRoads( foreign );
            auto snapshot = center.getRoads();
            auto node = make_ptr<RetiringCenterNode>(); node->center = &center; center.addNode( node );
            const auto baseline = budget.used();
            budget.set( baseline ); center.unload( nullptr );
            check( node->calls == 1 && node->detached && center.getRoads().capacity() == 0 &&
                   snapshot.size() == 3 && sameHandle( snapshot[0], first ) &&
                   sameHandle( snapshot[1], second ) && sameHandle( snapshot[2], duplicate ) && budget.used() < baseline,
                   "City-center retirement detaches derived and base inventories before reentrant throwing node callbacks" );
            budget.set( budget.original );
        }
        u64 beforeDestruction = 0;
        {
            Center center; center.setRoads( foreign );
            beforeDestruction = budget.used(); budget.set( beforeDestruction );
        }
        check( budget.used() < beforeDestruction,
               "City-center destruction retires derived handle storage without replacement admission" );
        budget.set( budget.original );
    }
}
