#pragma once
#include "CityInventoryBudgetContracts.hpp"
#include <WPProcedural/CRoadGeneratorGrid.hpp>
#include <WPProcedural/CRoadGeneratorCity.hpp>
#include <WPProcedural/CRoadConnection.hpp>
#include <WPProcedural/CTerrainGenerator.hpp>
#include <WPProcedural/CProceduralScene.hpp>
#include <Workphone/Math/LinearSpline1.hpp>
#include <cmath>

namespace legacy_road_rollback_contracts
{
    using namespace city_inventory_budget_contracts;
    class FinalDeny : public CRoadNetwork
    {
    public:
        bool reject = false;
        void appendGenerated( const Array<SmartPtr<IRoad>> &roads,
                              const Array<SmartPtr<IRoadConnection>> &connections ) override
        {
            if( reject )
            {
                Budget cap;
                cap.set( cap.used() );
                CRoadNetwork::appendGenerated( roads, connections );
                return;
            }
            CRoadNetwork::appendGenerated( roads, connections );
        }
    };
    class SmallGrid : public CRoadGeneratorGrid
    {
    public:
        SmallGrid() { m_gridSize = Vector2I( 2, 2 ); }
    };
    class StageProbe : public CRoadGenerator
    {
    public:
        SmartPtr<CRoad> result = make_ptr<CRoad>();
        SmartPtr<CRoadNode> a = make_ptr<CRoadNode>(), b = make_ptr<CRoadNode>();
        Budget *cap = nullptr;
        bool fail = false, stop = false;
    protected:
        void buildNetwork( SmartPtr<IRoadNetwork> target, SmartPtr<IRoadNetwork> ) override
        {
            target->addRoad( result );
            result->addNode( a ); result->addNode( b );
            a->connect( b ); b->connect( a );
            if( stop ) clear();
            if( fail ) { cap->set( cap->used() ); throw std::bad_alloc(); }
        }
    };
    template<class Generator> inline void actualGenerator()
    {
        Budget budget;
        auto city = make_ptr<CProceduralCity>();
        auto network = make_ptr<FinalDeny>();
        auto oldRoad = make_ptr<CRoad>();
        auto oldNode = make_ptr<CRoadNode>(), neighbor = make_ptr<CRoadNode>();
        oldNode->connect( neighbor );
        Array<SmartPtr<IRoadNode>> oldNodes;
        oldNodes.push_back( oldNode ); network->setNodes( oldNodes ); network->setMergedNodes( oldNodes );
        network->addRoad( oldRoad );
        RoadSegmentResult oldResult;
        network->setRoadSegmentResult( oldRoad, oldResult );
        const auto cached = network->getRoadSegmentResult( oldRoad );
        city->setRoadNetwork( network );
        auto generator = make_ptr<Generator>();
        generator->setCity( city );
        auto options = generator->getOptions();
        options.numMajorRoads = 1; options.generateMinorRoads = false;
        options.generateIntersections = false; options.randomizeSeed = false;
        generator->setOptions( options );
        network->reject = true;
        denied( [&]{ generator->generate(); } );
        check( city->getRoadNetwork() == network && network->getRoads().size() == 1 &&
               network->getRoads()[0] == oldRoad && oldNode->getNumConnections() == 1 &&
               network->getRoadSegmentResult( oldRoad ) == cached && !generator->isFinished(),
               "Late legacy generator publication denial preserves identity, adjacency, cache and status" );
        network->reject = false; generator->generate();
        const auto count = network->getRoads().size();
        check( count > 1 && generator->isFinished() && oldNode->getNumConnections() == 1,
               "Legacy generator retries with one complete append and leaves old adjacency intact" );
        auto generated = static_pointer_cast<CRoad>( network->getRoads()[1] );
        check( generated->getRoadNodeObjects().size() == 2 &&
               generated->getRoadNodeObjects()[0]->getNumConnections() == 1,
               "Committed generated endpoints remain connected after staging retirement" );
        network->reject = true;
        denied( [&]{ generator->generate(); } );
        check( generator->isFinished() && network->getRoads().size() == count &&
               network->getRoadSegmentResult( oldRoad ) == cached,
               "Failed regeneration retains completed status and all published roads" );
        generator->clear(); network->unload( nullptr ); oldNode->unload( nullptr );
    }
    inline void run()
    {
        Budget budget;
        {
            auto city = make_ptr<CProceduralCity>();
            auto owner = make_ptr<CRoadNetwork>(); city->setRoadNetwork( owner );
            auto probe = make_ptr<StageProbe>(); probe->setCity( city ); probe->cap = &budget;
            probe->fail = true;
            denied( [&]{ probe->generate(); } );
            check( owner->getRoads().empty() && probe->result->getRoadNodeObjects().empty() &&
                   probe->a->getNumConnections() == 0 && probe->b->getNumConnections() == 0,
                   "Full-cap staged generation failure cuts temporary endpoint cycles" );
            budget.set( budget.original );
            probe->fail = false; probe->stop = true; probe->generate();
            check( owner->getRoads().empty() && !probe->isFinished() && !probe->getCity() &&
                   probe->result->getRoadNodeObjects().empty(),
                   "Reentrant clear cancels unpublished generation without restoring stale status" );
        }
        actualGenerator<CRoadGenerator>();
        actualGenerator<SmallGrid>();
        actualGenerator<CRoadGeneratorCity>();
        {
            auto terrain = make_ptr<CTerrainGenerator>();
            auto oldCurve = make_ptr<LinearSpline1<real_Num>>();
            Array<real_Num> values{ Allocator<real_Num>( sourceBudget ) };
            values.push_back( 0 ); values.push_back( 1 ); oldCurve->setPoints( values );
            auto options = terrain->getOptions();
            options.width = options.height = 4; options.randomize = false; options.heightCurve = oldCurve;
            terrain->setOptions( options );
            auto properties = make_ptr<Properties>();
            properties->setProperty( "Width", 12 ); properties->setProperty( "HeightCurve", String( "0;0.5;1;" ) );
            budget.set( budget.used() );
            denied( [&]{ terrain->loadOptions( properties ); } );
            check( terrain->getOptions().width == 4 && terrain->getHeightCurve() == oldCurve,
                   "Denied curve-property import preserves the complete old terrain configuration" );
            budget.set( budget.original ); terrain->loadOptions( properties );
            check( terrain->getOptions().width == 12 && terrain->getHeightCurve()->getNumPoints() == 3,
                   "Curve property import retries with coherent admitted spline buffers" );
            auto output = make_ptr<Properties>();
            output->setProperty( "Width", 7 ); output->setProperty( "HeightCurve", String( "previous" ) );
            budget.set( budget.used() );
            denied( [&]{ terrain->saveOptions( output ); } );
            budget.set( budget.original );
            check( output->getPropertyAsInt( "Width", 0 ) == 7 && output->getProperty( "HeightCurve" ) == "previous",
                   "Curve encoding denial occurs before caller-owned property writes" );
            budget.set( budget.original );
            Array<real_Num> constant{ Allocator<real_Num>( sourceBudget ) }; constant.push_back( real_Num( .25 ) );
            auto one = make_ptr<LinearSpline1<real_Num>>(); one->setPoints( constant );
            terrain->setHeightCurve( one ); terrain->saveOptions( output );
            Array<f32> saved{ Allocator<f32>( sourceBudget ) };
            StringUtil::parseArray( output->getProperty( "HeightCurve" ), saved );
            check( saved.size() == 1 && std::isfinite( saved[0] ) && std::abs( saved[0] - .25f ) < .0001f,
                   "Single-point height curves serialize finite values" );
        }
    }
}
