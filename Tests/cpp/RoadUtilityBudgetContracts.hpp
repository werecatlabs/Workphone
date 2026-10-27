#pragma once
#include "CityInventoryBudgetContracts.hpp"
#include <WPProcedural/RoadGeneratorUtil.hpp>
#include <WPProcedural/CRoadElement.hpp>

namespace road_utility_budget_contracts
{
    using namespace city_inventory_budget_contracts;
    class ExternalRoad : public CRoad
    {
    public:
        int retireCalls = 0;
        void unload( SmartPtr<ISharedObject> data ) override
        {
            ++retireCalls;
            CRoad::unload( data );
        }
    };

    inline void appendSegment( Array<Vector3F> &positions, Array<Vector2F> &uvs )
    {
        RoadGeneratorUtil::createMeshSegment( Vector3F( 0,0,0 ), Vector3F( 10,0,0 ),
            Vector3F( 0,0,1 ), Vector3F( 0,0,1 ), 1, 1, positions, uvs );
    }

    inline void run()
    {
        Budget budget;
        {
            Array<Vector3F> positions;
            Array<Vector2F> uvs;
            positions.reserve( 17 ); positions.push_back( Vector3F( 7,8,9 ) );
            uvs.reserve( 19 ); uvs.push_back( Vector2F( 2,3 ) );
            const auto baseline = budget.used();
            const auto first = sourceBudgetStorageBytes( 17 * sizeof( Vector3F ), alignof( Vector3F ) );
            budget.set( baseline + first );
            denied( [&]{ appendSegment( positions, uvs ); } );
            check( positions.size() == 1 && positions[0] == Vector3F( 7,8,9 ) &&
                   uvs.size() == 1 && uvs[0] == Vector2F( 2,3 ) && budget.used() == baseline,
                   "Late foreign UV admission preserves both caller arrays and releases staged positions" );
            budget.set( budget.original );
            appendSegment( positions, uvs );
            check( positions.size() == 7 && uvs.size() == 7 && positions.capacity() == 17 &&
                   uvs.capacity() == 19 && positions.get_allocator().isSourceBudgeted() &&
                   uvs.get_allocator().isSourceBudgeted(),
                   "Road utility outputs import exact foreign spare capacity and SourceCpu policy" );
        }
        {
            auto positions = makeSourceBudgetSnapshot<Vector3F>();
            auto uvs = makeSourceBudgetSnapshot<Vector2F>();
            const auto baseline = budget.used();
            const auto first = sourceBudgetStorageBytes( 6 * sizeof( Vector3F ), alignof( Vector3F ) );
            budget.set( baseline + first );
            denied( [&]{ appendSegment( positions, uvs ); } );
            check( positions.empty() && uvs.empty() && positions.capacity() == 6 &&
                   budget.used() == baseline + first,
                   "Late admitted UV denial publishes no geometry and retains charged spare position storage" );
            budget.set( budget.original );
            appendSegment( positions, uvs );
            check( positions.size() == 6 && uvs.size() == 6 &&
                   positions[0] == Vector3F( 0,0,-1 ) && positions[2] == Vector3F( 10,0,1 ),
                   "Admitted segment append retries with matching vertex and UV data" );
        }
        {
            auto segment = RoadGeneratorUtil::createRoadSegment( Vector3F( 0,0,0 ), Vector3F( 10,0,0 ),
                Vector3F( 1,0,0 ), Vector3F( 1,0,0 ), 2, 2 );
            check( segment != nullptr, "Road utility segment fixture creates complete endpoint ownership" );
            Array<SmartPtr<IRoadElement>> segments; segments.push_back( segment );
            budget.set( budget.used() );
            denied( [&]{ RoadGeneratorUtil::buildMesh( segments ); } );
            budget.set( budget.original );
            segment->unload( nullptr );
        }
        {
            auto connected = make_ptr<ExternalRoad>();
            connected->setRoadType( "ExternalSentinel" );
            // Warm factory slots and string slabs without retaining temporary graph cycles.
            for( int i = 0; i < 2; ++i )
            {
                auto road = RoadGeneratorUtil::createRoad( connected, Vector3F( 0,0,0 ), Vector3F( 1,0,0 ) );
                check( road && road->getRoadNodes().size() == 2, "Road utility warm fixture creates a complete road" );
                road->unload( nullptr );
            }
            const auto baseline = budget.used();
            bool sawDenial = false;
            for( const u64 headroom : { u64( 0 ), u64( 256 ), u64( 1024 ), u64( 2048 ) } )
            {
                budget.set( baseline + headroom );
                SmartPtr<IRoad> result;
                bool rejected = false;
                try { result = RoadGeneratorUtil::createRoad( connected, Vector3F( 0,0,0 ), Vector3F( 1,0,0 ) ); }
                catch( const std::bad_alloc & ) { rejected = true; sawDenial = true; }
                budget.set( budget.original );
                check( !rejected || !result, "Denied road construction cannot publish a partial road" );
                if( result ) result->unload( nullptr );
                result = nullptr;
                check( connected->retireCalls == 0 && connected->getRoadType() == "ExternalSentinel",
                       "Road rollback never unloads an externally referenced connected road" );
                check( budget.used() == baseline,
                       "Temporary road/node/element cycles retire after construction denial or cleanup" );
            }
            check( sawDenial, "Road construction requires SourceCpu admission" );
            auto road = RoadGeneratorUtil::createRoad( connected, Vector3F( 0,0,0 ), Vector3F( 1,0,0 ) );
            check( road && road->getRoadNodes().size() == 2 && road->getRoadSections().size() == 1,
                   "Road utility retries after complete admission" );
            Array<SmartPtr<IRoad>> roads; roads.push_back( road );
            auto oldNode = road->getFirstNode();
            SmartPtr<IRoadNode> found = oldNode;
            SmartPtr<IRoad> hit = connected;
            budget.set( budget.used() );
            denied( [&]{ RoadGeneratorUtil::isNearExistingRoadNode( roads, nullptr,
                Sphere3F( Vector3F( 0,0,0 ), 5 ), found, hit ); } );
            check( found == oldNode && hit == connected,
                   "Denied road-node query preserves previous caller output pointers" );
            budget.set( budget.original );
            check( RoadGeneratorUtil::isNearExistingRoadNode( roads, nullptr,
                       Sphere3F( Vector3F( 0,0,0 ), 5 ), found, hit ) && hit == road,
                   "Road-node query retries with a complete admitted snapshot" );
            road->unload( nullptr );
        }
        {
            auto manual = make_ptr<ExternalRoad>();
            auto block = make_ptr<CCityBlock>();
            for( const auto &point : { Vector3<real_Num>( 0,0,0 ), Vector3<real_Num>( 100,0,0 ),
                                      Vector3<real_Num>( 100,0,100 ), Vector3<real_Num>( 0,0,100 ) } )
                block->addPoint( point );
            auto buildAndRetire = [&]() {
                Array<SmartPtr<IRoad>> output;
                output.reserve( 33 ); output.push_back( manual );
                const auto before = budget.used();
                RoadGeneratorUtil::createCheckerPattern( block, output );
                check( output.size() == 7 && output[0] == manual && output.capacity() == 33 &&
                       output.get_allocator().isSourceBudgeted(),
                       "Checker output appends six complete roads and retains foreign pointer capacity" );
                const auto span = budget.used() - before;
                for( size_t i = 1; i < output.size(); ++i ) output[i]->unload( nullptr );
                return span;
            };
            buildAndRetire();
            const auto span = buildAndRetire();
            Array<SmartPtr<IRoad>> output;
            output.reserve( 33 ); output.push_back( manual );
            const auto baseline = budget.used();
            budget.set( baseline + span / 2 );
            denied( [&]{ RoadGeneratorUtil::createCheckerPattern( block, output ); } );
            budget.set( budget.original );
            check( output.size() == 1 && output[0] == manual && manual->retireCalls == 0 &&
                   !output.get_allocator().isSourceBudgeted() && budget.used() == baseline,
                   "Late checker-pattern denial rolls back newly owned road cycles and preserves caller output" );
            RoadGeneratorUtil::createCheckerPattern( block, output );
            check( output.size() == 7 && manual->retireCalls == 0,
                   "Checker pattern retries without retiring existing roads" );
            for( size_t i = 1; i < output.size(); ++i ) output[i]->unload( nullptr );
        }
    }
}
