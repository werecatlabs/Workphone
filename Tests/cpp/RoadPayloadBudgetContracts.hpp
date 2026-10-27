#pragma once
#include "CityInventoryBudgetContracts.hpp"
#include <WPProcedural/CRoadConnection.hpp>

namespace road_payload_budget_contracts
{
    using namespace city_inventory_budget_contracts;
    inline void fill( RoadMesh &mesh, real_Num x = 1 )
    {
        RoadVertex vertex{};
        vertex.position = Vector3<real_Num>( x, 0, 0 );
        mesh.vertices.reserve( 17 );
        mesh.indices.reserve( 19 );
        mesh.addVertex( vertex );
        mesh.addTriangle( 0, 0, 0 );
    }
    class RejectingRoad : public CRoad
    {
    public:
        bool reject = false;
        void setUserData( hash_type id, void *value ) override
        {
            if( reject && value ) throw std::bad_alloc();
            CRoad::setUserData( id, value );
        }
    };
    class RejectingConnection : public CRoadConnection
    {
    public:
        bool reject = false;
        void setUserData( hash_type id, void *value ) override
        {
            if( reject && value ) throw std::bad_alloc();
            CRoadConnection::setUserData( id, value );
        }
    };
    inline void run()
    {
        Budget budget;
        const auto initial = budget.used();
        {
            RoadMesh mesh;
            mesh.indices.reserve( 2 );
            budget.set( budget.used() );
            denied( [&]{ mesh.vertices.reserve( 17 ); } );
            denied( [&]{ mesh.addTriangle( 0, 0, 0 ); } );
            check( mesh.vertices.empty() && mesh.indices.empty(), "Denied road buffers publish no geometry" );
            denied( []{ auto body = std::make_unique<RoadMesh>(); } );
            denied( []{ auto body = std::make_unique<RoadSegmentResult>(); } );
            denied( []{ auto body = std::make_unique<IntersectionResult>(); } );
            budget.set( budget.original );
            fill( mesh );
            const auto live = budget.used();
            RoadMesh copy( mesh );
            check( copy.vertices.capacity() == 17 && copy.indices.capacity() == 19 &&
                   copy.vertices.get_allocator().isSourceBudgeted() &&
                   copy.indices.get_allocator().isSourceBudgeted() && budget.used() > live,
                   "Road snapshots admit retained spare capacity" );
            copy.clear();
            const auto retained = budget.used();
            check( copy.vertices.capacity() == 17 && copy.indices.capacity() == 19,
                   "Cleared geometry retains its charged capacity" );
            budget.set( retained );
            denied( [&]{ RoadMesh snapshot( copy ); } );
            copy.addVertex( RoadVertex{} );
            copy.addTriangle( 0, 0, 0 );
            // Vertices already fit, so denial occurs at index admission before mutation.
            mesh.indices = Array<u32>{ Allocator<u32>( sourceBudget ) };
            budget.set( budget.used() );
            denied( [&]{ mesh.append( copy, Vector3<real_Num>( 1, 0, 0 ) ); } );
            check( mesh.vertices.size() == 1 && mesh.indices.empty(),
                   "Late append denial publishes neither vertices nor indices" );
            budget.set( budget.original );
        }
        check( budget.used() == initial, "Road geometry leases retire with physical storage" );
        {
            RoadMesh mesh;
            mesh.vertices.reserve( 1 );
            mesh.indices.reserve( 3 );
            RoadVertex vertex{};
            vertex.position = Vector3<real_Num>( 1, 0, 0 );
            mesh.addVertex( vertex );
            mesh.addTriangle( 0, 0, 0 );
            mesh.append( mesh, Vector3<real_Num>( 2, 0, 0 ) );
            check( mesh.vertices.size() == 2 && mesh.indices.size() == 6 &&
                   mesh.vertices[1].position.X() == 3 && mesh.indices[3] == 1,
                   "Self append preserves source vertices and offsets indices" );
            RoadMesh target;
            fill( target, 9 );
            budget.set( budget.used() + sourceBudgetStorageBytes( mesh.vertices.capacity() * sizeof( RoadVertex ), alignof( RoadVertex ) ) );
            denied( [&]{ target = mesh; } );
            check( target.vertices.size() == 1 && target.vertices[0].position.X() == 9 &&
                   target.indices.size() == 3, "Late index denial preserves the complete mesh" );
            budget.set( budget.original );
            RoadSegmentResult source, destination;
            fill( source.roadSurface, 2 ); fill( source.pedXing, 3 );
            fill( destination.roadSurface, 7 ); fill( destination.pedXing, 8 );
            const auto oneMesh = sourceBudgetStorageBytes( 17 * sizeof( RoadVertex ), alignof( RoadVertex ) ) +
                                 sourceBudgetStorageBytes( 19 * sizeof( u32 ), alignof( u32 ) );
            budget.set( budget.used() + oneMesh );
            denied( [&]{ destination = source; } );
            check( destination.roadSurface.vertices[0].position.X() == 7 &&
                   destination.pedXing.vertices[0].position.X() == 8,
                   "Late road layer denial preserves all destination layers" );
            budget.set( budget.original );
            destination = source;
            IntersectionResult intersection, previous;
            fill( intersection.surface, 2 ); fill( intersection.dressing, 3 );
            fill( previous.surface, 7 ); fill( previous.dressing, 8 );
            budget.set( budget.used() + oneMesh );
            denied( [&]{ previous = intersection; } );
            check( previous.surface.vertices[0].position.X() == 7 &&
                   previous.dressing.vertices[0].position.X() == 8,
                   "Late intersection layer denial preserves all destination layers" );
            budget.set( budget.original );
        }
        check( budget.used() == initial, "Nested result snapshots release all leases" );
        {
            Array<RoadVertex> external;
            external.reserve( 23 );
            external.push_back( RoadVertex{} );
            RoadMesh imported;
            imported.vertices = external; // Existing public mutable-array compatibility boundary.
            const auto baseline = budget.used();
            const auto bytes = sourceBudgetStorageBytes( 23 * sizeof( RoadVertex ), alignof( RoadVertex ) );
            budget.set( baseline + bytes - 1 );
            denied( [&]{ RoadMesh snapshot( imported ); } );
            check( budget.used() == baseline, "Rejected foreign mesh snapshot releases its partial lease" );
            budget.set( baseline + bytes );
            {
                RoadMesh snapshot( imported );
                check( snapshot.vertices.capacity() == 23 && snapshot.vertices.get_allocator().isSourceBudgeted() &&
                       budget.used() == baseline + bytes,
                       "Foreign mesh snapshots admit exact retained capacity and restore SourceCpu policy" );
            }
            budget.set( budget.original );
        }
        {
            auto road = make_ptr<RejectingRoad>();
            auto connection = make_ptr<RejectingConnection>();
            auto network = make_ptr<CRoadNetwork>();
            const auto roadId = StringUtil::getHash( "WPRoadSegmentResult" );
            const auto connectionId = StringUtil::getHash( "WPIntersectionResult" );
            RoadSegmentResult empty;
            const auto firstBaseline = budget.used();
            budget.set( firstBaseline + sourceBudgetStorageBytes( sizeof( RoadSegmentResult ), alignof( std::max_align_t ) ) );
            network->setRoadSegmentResult( road, empty );
            check( !road->getUserData( roadId ) && budget.used() == firstBaseline,
                   "Cache node denial retires the already admitted result body" );
            budget.set( budget.original );
            auto owned = std::make_unique<RoadSegmentResult>();
            road->setUserData( roadId, owned.get() ); owned.release();
            auto ownedIntersection = std::make_unique<IntersectionResult>();
            connection->setUserData( connectionId, ownedIntersection.get() ); ownedIntersection.release();
            const auto baseline = budget.used();
            budget.set( baseline );
            check( !network->getRoadSegmentResult( road ) && !network->getIntersectionResult( connection ) &&
                   budget.used() == baseline, "Empty result cache nodes also require SourceCpu admission" );
            budget.set( budget.original );
            const auto cached = network->getRoadSegmentResult( road );
            const auto cachedIntersection = network->getIntersectionResult( connection );
            check( cached && cachedIntersection && budget.used() > baseline, "Denied cache population retries successfully" );
            budget.set( budget.used() );
            check( cached == network->getRoadSegmentResult( road ) &&
                   cachedIntersection == network->getIntersectionResult( connection ),
                   "Warm cache reads remain allocation free at the cap" );
            budget.set( budget.original );
            RoadSegmentResult replacement;
            fill( replacement.roadSurface, 4 );
            road->reject = true;
            auto coldRoad = make_ptr<RejectingRoad>();
            coldRoad->reject = true;
            const auto coldBaseline = budget.used();
            network->setRoadSegmentResult( coldRoad, replacement );
            check( !coldRoad->getUserData( roadId ) && !network->getRoadSegmentResult( coldRoad ) &&
                   budget.used() == coldBaseline, "Failed first publication removes its staged cache node" );
            const auto before = budget.used();
            const auto previous = road->getUserData( roadId );
            network->setRoadSegmentResult( road, replacement );
            check( road->getUserData( roadId ) == previous && cached->roadSurface.vertices.empty() &&
                   budget.used() == before, "Failed owned publication preserves cache and userdata without leaking staged payloads" );
            road->reject = false;
            network->setRoadSegmentResult( road, replacement );
            check( network->getRoadSegmentResult( road ) == cached && cached->roadSurface.vertices.size() == 1,
                   "Successful cache replacement preserves the returned entry address" );
            IntersectionResult replacementIntersection;
            fill( replacementIntersection.dressing, 5 );
            connection->reject = true;
            const auto oldIntersection = connection->getUserData( connectionId );
            const auto intersectionBaseline = budget.used();
            network->setIntersectionResult( connection, replacementIntersection );
            check( connection->getUserData( connectionId ) == oldIntersection &&
                   cachedIntersection->dressing.vertices.empty() && budget.used() == intersectionBaseline,
                   "Intersection publication failure preserves both old copies" );
            connection->reject = false;
            network->setIntersectionResult( connection, replacementIntersection );
            check( network->getIntersectionResult( connection ) == cachedIntersection &&
                   cachedIntersection->dressing.vertices.size() == 1,
                   "Intersection replacement retries without moving the cache entry" );
            budget.set( budget.used() );
            network->unload( nullptr );
            road->unload( nullptr ); connection->unload( nullptr );
            budget.set( budget.original );
        }
        check( budget.used() == initial, "Result bodies, cache metadata and geometry physically retire" );
    }
}
