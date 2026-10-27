#pragma once
#include "CityInventoryBudgetContracts.hpp"
#include <WPProcedural/CRoadConnection.hpp>

namespace road_network_publication_contracts
{
    using namespace city_inventory_budget_contracts;
    class CountedRoad : public CRoad
    {
    public:
        int unloads = 0;
        void unload( SmartPtr<ISharedObject> data ) override
        {
            ++unloads;
            CRoad::unload( data );
        }
    };

    inline void run()
    {
        Budget budget;
        auto owner = make_ptr<CRoadNetwork>();
        auto stage = make_ptr<CRoadNetwork>();
        auto deniedStage = make_ptr<CRoadNetwork>();
        auto oldRoad = make_ptr<CountedRoad>();
        auto newRoad = make_ptr<CountedRoad>();
        auto oldConnection = make_ptr<CRoadConnection>();
        auto newConnection = make_ptr<CRoadConnection>();
        auto oldNode = make_ptr<CRoadNode>();
        auto neighbor = make_ptr<CRoadNode>();
        oldNode->connect( neighbor );
        Array<SmartPtr<IRoadNode>> nodes;
        nodes.push_back( oldNode );
        owner->setNodes( nodes );
        owner->setMergedNodes( nodes );
        owner->getRoads().reserve( 41 );
        owner->addRoad( oldRoad );
        Array<SmartPtr<IRoadConnection>> connections;
        connections.reserve( 17 ); connections.push_back( oldConnection );
        owner->setRoadConnections( connections );
        RoadSegmentResult payload;
        owner->setRoadSegmentResult( oldRoad, payload );
        const auto oldCached = owner->getRoadSegmentResult( oldRoad );
        check( oldCached != nullptr, "Existing road result is cached before generated publication" );
        const auto oldPayload = oldRoad->getUserData( StringUtil::getHash( "WPRoadSegmentResult" ) );
        stage->addRoad( newRoad ); stage->addRoadConnection( newConnection );
        stage->setRoadSegmentResult( newRoad, payload );
        const auto newPayload = newRoad->getUserData( StringUtil::getHash( "WPRoadSegmentResult" ) );
        auto newConnections = stage->getRoadConnections();
        Array<SmartPtr<IRoad>> registration;
        registration.push_back( newRoad );

        const auto baseline = budget.used();
        budget.set( baseline );
        denied( [&]{ deniedStage->addRoad( newRoad ); } );
        denied( [&]{ deniedStage->addRoads( registration ); } );
        denied( [&]{ deniedStage->addRoadConnection( newConnection ); } );
        check( deniedStage->getRoads().empty() && deniedStage->getRoadConnections().empty() &&
               budget.used() == baseline,
               "Early staging registration propagates denial without publishing inventory" );
        denied( [&]{ owner->appendGenerated( stage->getRoads(), newConnections ); } );
        check( owner->getRoads().size() == 1 && owner->getRoads()[0] == oldRoad &&
               oldNode->getNumConnections() == 1 &&
               owner->getRoadSegmentResult( oldRoad ) == oldCached &&
               oldRoad->getUserData( StringUtil::getHash( "WPRoadSegmentResult" ) ) == oldPayload &&
               budget.used() == baseline,
               "Full-cap generated publication preserves roads, adjacency, cache and payload identities" );

        // Admit the entire road replacement, then deny the connection replacement.
        const auto roadBytes = sourceBudgetStorageBytes(
            owner->getRoads().capacity() * sizeof( SmartPtr<IRoad> ), alignof( SmartPtr<IRoad> ) );
        budget.set( baseline + roadBytes );
        denied( [&]{ owner->appendGenerated( stage->getRoads(), newConnections ); } );
        check( owner->getRoads().size() == 1 && oldNode->getNumConnections() == 1 &&
               owner->getRoadSegmentResult( oldRoad ) == oldCached && budget.used() == baseline,
               "Late connection admission retires prepared road storage and preserves the live network" );
        budget.set( budget.original );
        auto oldConnections = owner->getRoadConnections();
        check( oldConnections.size() == 1 && oldConnections[0] == oldConnection,
               "Denied append leaves the old connection inventory intact" );
        bool unsupported = false;
        try { owner->IRoadNetwork::appendGenerated( stage->getRoads(), newConnections ); }
        catch( const std::logic_error & ) { unsupported = true; }
        check( unsupported && owner->getRoads().size() == 1,
               "Default network publication rejects unsupported ownership before mutation" );

        owner->appendGenerated( stage->getRoads(), newConnections );
        const auto &roads = owner->getRoads();
        auto combinedConnections = owner->getRoadConnections();
        auto retainedNodes = owner->getNodes();
        check( roads.size() == 2 && roads[0] == oldRoad && roads[1] == newRoad &&
               roads.capacity() == 41 && roads.get_allocator().isSourceBudgeted() &&
               combinedConnections.size() == 2 && combinedConnections[0] == oldConnection &&
               combinedConnections[1] == newConnection && combinedConnections.capacity() == 17 &&
               retainedNodes.size() == 1 && retainedNodes[0] == oldNode && oldNode->getNumConnections() == 1 &&
               owner->getRoadSegmentResult( oldRoad ) == oldCached,
               "Retry atomically appends both inventories while preserving spare capacity and old graph/cache owners" );
        budget.set( budget.used() );
        owner->appendGenerated( stage->getRoads(), newConnections ); // Duplicate-only batch needs no admission.
        stage->releaseGenerationReferences();
        stage->unload( nullptr );
        check( stage->getRoads().empty() && stage->getRoadConnections().empty() &&
               newRoad->unloads == 0 && oldRoad->unloads == 0 &&
               newRoad->getUserData( StringUtil::getHash( "WPRoadSegmentResult" ) ) == newPayload &&
               owner->getRoads().size() == 2 && owner->getRoadSegmentResult( oldRoad ) == oldCached,
               "Full-cap staging handoff releases only staging references without unloading committed children" );
        budget.set( budget.original );
        oldNode->clearConnections();
    }
}
