#pragma once
#include <WPProcedural/CProceduralCity.hpp>
#include <WPProcedural/CRoadGenerator.hpp>
#include <WPProcedural/CRoadNetwork.hpp>
#include <WPProcedural/CRoadNode.hpp>
#include <WPProcedural/CRoad.hpp>
#include <WPProcedural/CityGeneratorDefault.hpp>
#include <WPProcedural/CCityBlock.hpp>
#include <WPProcedural/MeshGeneratorDefault.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <stdexcept>

namespace city_inventory_budget_contracts
{
    using namespace workphone;
    using namespace workphone::procedural;
    inline void check( bool value, const char *message )
    { if( !value ) throw std::runtime_error( message ); }
    struct Budget
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        u64 original = scheduler->stats().limits[size_t( StreamMemoryClass::SourceCpu )];
        u64 used() const { return scheduler->stats().used[size_t( StreamMemoryClass::SourceCpu )]; }
        void set( u64 bytes ) { String error; check( scheduler->setBudget( StreamMemoryClass::SourceCpu, bytes, error ), "Set city inventory budget" ); }
        ~Budget() { String error; scheduler->setBudget( StreamMemoryClass::SourceCpu, original, error ); }
    };
    template<class F> inline void denied( F f )
    {
        bool rejected = false;
        try { f(); } catch( const std::bad_alloc & ) { rejected = true; }
        check( rejected, "City inventory allocation must require SourceCpu admission" );
    }
    class RetiringNode : public CRoadNode
    {
    public:
        CRoadNetwork *network = nullptr;
        int calls = 0;
        bool detached = false;
        void unload( SmartPtr<ISharedObject> ) override
        {
            ++calls;
            detached = network && network->getNodes().empty() &&
                       network->getMergedNodes().empty() && network->getRoads().empty() &&
                       network->getRoadConnections().empty();
            throw std::bad_alloc();
        }
    };
    class RoadInventory : public CRoadGenerator
    {
    public:
        void reserveCities( size_t count ) { m_cities.reserve( count ); }
        size_t capacity() const { return m_cities.capacity(); }
    };
    class CountingRoad : public CRoadGenerator
    {
    public:
        int calls = 0;
        bool ready = true;
        void generate() override { ++calls; }
        bool isFinished() const override { return ready; }
    };
    class MeshProducer : public MeshGeneratorDefault
    {
    public:
        CityGeneratorDefault *coordinator = nullptr;
        SmartPtr<IMesh> result;
        SmartPtr<IMesh> nextResult;
        SmartPtr<IProceduralCity> restartCity;
        bool stop = false;
        bool restart = false;
        int calls = 0;
        bool isFinished() const override { return true; }
        SmartPtr<IMesh> generateFromObjects( const Array<SmartPtr<IProceduralObject>> & ) override
        {
            ++calls;
            const auto produced = result;
            if( stop ) coordinator->unload( nullptr );
            if( restart )
            {
                restart = false;
                coordinator->unload( nullptr );
                result = nextResult;
                coordinator->addCity( restartCity );
                coordinator->setMeshGenerator( SmartPtr<IMeshGenerator>( this ) );
                coordinator->generate();
            }
            return produced;
        }
    };
    inline void run()
    {
        Budget budget;
        {
            auto network = make_ptr<CRoadNetwork>();
            auto node = make_ptr<RetiringNode>();
            node->network = network.get();
            Array<SmartPtr<IRoadNode>> raw;
            raw.reserve( 257 );
            raw.push_back( node );
            const auto baseline = budget.used();
            const auto bytes = sourceBudgetStorageBytes( raw.capacity() * sizeof( SmartPtr<IRoadNode> ), alignof( SmartPtr<IRoadNode> ) );
            budget.set( baseline + bytes - 1 );
            denied( [&]{ network->setNodes( raw ); } );
            check( budget.used() == baseline && network->getNodes().empty(), "Denied network replacement leaves no partial inventory" );
            budget.set( baseline + bytes );
            network->setNodes( raw );
            check( budget.used() == baseline + bytes, "Network charges complete imported spare capacity" );
            budget.set( budget.original );
            auto snapshot = network->getNodes();
            check( snapshot.capacity() == raw.capacity() && snapshot.get_allocator().isSourceBudgeted() &&
                   snapshot.data() != raw.data(), "Detached network snapshots retain capacity and budget policy" );
            const auto withSnapshot = budget.used();
            budget.set( withSnapshot );
            network->unload( nullptr );
            check( node->calls == 1 && node->detached && snapshot[0] == node &&
                   budget.used() == withSnapshot - bytes,
                   "Full-cap teardown detaches inventories before throwing child callbacks and preserves snapshots" );
            budget.set( budget.original );
        }
        {
            auto coordinator = make_ptr<CityGeneratorDefault>();
            auto city = make_ptr<CProceduralCity>();
            auto road = make_ptr<CountingRoad>();
            road->ready = false;
            coordinator->addCity( city );
            coordinator->setRoadGenerator( road );
            coordinator->generate();
            check( !coordinator->isFinished(), "Asynchronous producer retains a pending generation plan" );
            auto snapshot = coordinator->getCities();
            const auto baseline = budget.used();
            budget.set( baseline );
            coordinator->unload( nullptr );
            check( coordinator->isFinished() && !coordinator->getRoadGenerator() &&
                   coordinator->getCities().empty() && snapshot[0] == city && budget.used() < baseline,
                   "Pending typed steps physically retire at the cap without unloading externally owned producers" );
            budget.set( budget.original );
        }
        {
            auto city = make_ptr<CProceduralCity>();
            auto block = make_ptr<CCityBlock>();
            city->addBlock( block );
            const auto capacity = city->getBlocks().capacity();
            for( size_t i = 1; i < capacity; ++i ) city->addBlock( block );
            auto old = city->getBlocks();
            check( old.get_allocator().isSourceBudgeted(), "City block snapshots participate in SourceCpu" );
            const auto baseline = budget.used();
            budget.set( baseline );
            denied( [&]{ city->addBlock( block ); } );
            check( budget.used() == baseline, "Denied city append preserves prior storage" );
            city->unload( nullptr );
            check( city->getBlocks().empty() && old.size() == capacity, "City retirement releases its inventory independently of escaped snapshots" );
            budget.set( budget.original );
            auto empty = city->getRoads();
            check( empty.get_allocator().isSourceBudgeted(), "Empty road snapshots retain budget policy for later caller growth" );
        }
        {
            RoadInventory generator;
            const auto baseline = budget.used();
            budget.set( baseline );
            denied( [&]{ generator.reserveCities( 127 ); } );
            budget.set( budget.original );
            generator.reserveCities( 127 );
            check( budget.used() > baseline, "Road-generator inventory is admitted" );
            budget.set( budget.used() );
            generator.clear();
            check( generator.capacity() == 0 && budget.used() == baseline, "Road-generator clear physically retires spare inventory at the cap" );
            budget.set( budget.original );
        }
        {
            CProceduralCity city;
            Array<SmartPtr<IProceduralCityCenter>> raw;
            raw.reserve( 131 );
            raw.push_back( nullptr );
            const auto baseline = budget.used();
            budget.set( baseline );
            denied( [&]{ city.setCityCenters( raw ); } );
            check( city.getCityCenters().empty(), "Foreign city-center assignment stages before publication" );
            budget.set( budget.original );
            auto incoming = makeSourceBudgetSnapshot( raw );
            const auto beforeMove = budget.used();
            budget.set( beforeMove );
            city.setCityCenters( std::move( incoming ) );
            check( incoming.empty() && budget.used() == beforeMove,
                   "Already admitted city-center rvalues transfer without allocating a duplicate buffer" );
            budget.set( budget.original );
            auto snapshot = city.getCityCenters();
            check( snapshot.size() == 1 && snapshot.capacity() == 131 &&
                   snapshot.get_allocator().isSourceBudgeted(), "City-center exports retain admitted spare capacity" );
            budget.set( budget.used() );
            city.unload( nullptr );
            check( city.getCityCenters().empty() && snapshot.size() == 1, "Full-cap city-center retirement preserves escaped snapshots" );
            budget.set( budget.original );
        }
        {
            auto coordinator = make_ptr<CityGeneratorDefault>();
            auto city = make_ptr<CProceduralCity>();
            auto road = make_ptr<CountingRoad>();
            coordinator->addCity( city );
            coordinator->setRoadGenerator( road );
            coordinator->generate();
            check( coordinator->isFinished() && road->calls == 1, "Baseline city run completes" );
            const auto baseline = budget.used();
            budget.set( baseline );
            denied( [&]{ coordinator->generate(); } );
            check( road->calls == 1 && coordinator->isFinished() && budget.used() == baseline,
                   "Denied generation plan invokes no producers and preserves completed status" );
            budget.set( budget.original );
            coordinator->generate();
            check( coordinator->isFinished() && road->calls == 2, "Generation retries after plan admission recovers" );
            auto snapshot = coordinator->getCities();
            budget.set( budget.used() );
            coordinator->unload( nullptr );
            check( coordinator->getCities().empty() && snapshot[0] == city,
                   "Coordinator unload at full budget detaches inventory while escaped snapshots remain valid" );
            budget.set( budget.original );
        }
        {
            auto coordinator = make_ptr<CityGeneratorDefault>();
            auto city = make_ptr<CProceduralCity>();
            auto producer = make_ptr<MeshProducer>();
            producer->coordinator = coordinator.get();
            producer->result = make_ptr<Mesh>();
            city->addBlock( make_ptr<CCityBlock>() );
            coordinator->addCity( city );
            coordinator->setMeshGenerator( producer );
            coordinator->generate();
            auto retained = coordinator->getGeneratedMeshes();
            check( retained.size() == 1 && retained[0] == producer->result &&
                   retained.get_allocator().isSourceBudgeted(), "Generated mesh snapshots retain their admitted output" );
            const auto baseline = budget.used();
            budget.set( baseline );
            denied( [&]{ coordinator->generate(); } );
            check( coordinator->isFinished() && producer->calls == 1 && budget.used() == baseline,
                   "Denied replacement plan preserves the completed mesh generation" );
            budget.set( budget.original );
            check( coordinator->getGeneratedMeshes()[0] == retained[0], "Denied plan retains the previous mesh identity" );
            producer->stop = true;
            coordinator->generate();
            check( coordinator->isFinished() && coordinator->getGeneratedMeshes().empty() &&
                   retained[0] == producer->result,
                   "Reentrant unload cannot publish a stale mesh after cancelling the admitted run" );
            producer->stop = false;
            producer->restart = true;
            producer->restartCity = city;
            producer->nextResult = make_ptr<Mesh>();
            coordinator->addCity( city );
            coordinator->setMeshGenerator( producer );
            coordinator->generate();
            auto replacement = coordinator->getGeneratedMeshes();
            check( coordinator->isFinished() && producer->calls == 4 && replacement.size() == 1 &&
                   replacement[0] == producer->nextResult && replacement[0] != retained[0],
                   "Reentrant restart publishes only the new generation, excluding the old producer result" );
        }
    }
}
