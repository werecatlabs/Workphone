#pragma once
#include "CityInventoryBudgetContracts.hpp"
#include <WPProcedural/CBlockGenerator.hpp>
#include <WPProcedural/CRoadConnection.hpp>
#include <WPProcedural/CRoadMeshElement.hpp>
#include <WPProcedural/CRoadSection.hpp>
#include <WPProcedural/CRoadGeneratorGrid.hpp>
#include <WPProcedural/CRoadGeneratorCity.hpp>
#include <WPProcedural/PlanarGraph.hpp>
#include <WPProcedural/CTerrainGeneratorDefault.hpp>
#include <WPProcedural/CProceduralScene.hpp>
#include <WPProcedural/CProceduralTerrain.hpp>
#include <Workphone/Math/FalloffGenerator.hpp>
#include <Workphone/Math/PerlinNoiseGenerator.hpp>

namespace procedural_storage_budget_contracts
{
    using namespace city_inventory_budget_contracts;
    class FinalAdmissionCity : public CProceduralCity
    {
    public:
        bool reject = false;
        void setBlocks( const Array<SmartPtr<ICityBlock>> &blocks ) override
        {
            if( reject )
            {
                Budget cap;
                cap.set( cap.used() );
                CProceduralCity::setBlocks( blocks );
                return;
            }
            CProceduralCity::setBlocks( blocks );
        }
    };
    template<class T> class ShadowInventory : public T
    {
    public:
        void reserveCities() { this->m_cities.reserve( 67 ); }
        size_t capacity() const { return this->m_cities.capacity(); }
    };
    inline void run()
    {
        Budget budget;
        {
            auto generator = make_ptr<CRoadGenerator>();
            SRoadGeneratorOptions replacement;
            replacement.patternName = String( 600000, 'x' ); // Above the shared string slab threshold.
            replacement.seed = 99;
            SRoadGeneratorOptions previous;
            previous.patternName = "previous";
            previous.seed = 17;
            generator->setOptions( previous );
            const auto baseline = budget.used();
            budget.set( baseline );
            denied( []{ auto options = std::make_unique<SRoadGeneratorOptions>(); } );
            denied( [&]{ generator->setOptions( replacement ); } );
            check( generator->getOptions().seed == 17 && generator->getOptions().patternName == "previous" &&
                   budget.used() == baseline, "Denied option text/body preserves configuration and leases" );
            budget.set( budget.original );
            generator->setOptions( replacement );
            check( generator->getOptions().seed == 99 && generator->getOptions().patternName.size() == 600000 &&
                   budget.used() > baseline, "Procedural option strings use the shared SourceCpu character pool" );
        }
        {
            auto node = make_ptr<CRoadNode>();
            auto neighbor = make_ptr<CRoadNode>();
            auto road = make_ptr<CRoad>();
            auto section = make_ptr<CRoadSection>();
            Array<SmartPtr<IRoadSection>> sections;
            sections.push_back( section ); road->setRoadSections( sections );
            Array<SmartPtr<IProceduralNode>> merged;
            merged.reserve( 67 ); merged.push_back( neighbor );
            budget.set( budget.used() );
            denied( [&]{ node->addRoad( road ); } );
            denied( [&]{ node->connect( neighbor ); } );
            denied( [&]{ node->setMergedNodes( merged ); } );
            denied( [&]{ road->getNodes(); } );
            check( node->getNumConnections() == 0, "Denied adjacency insertion publishes no edge" );
            budget.set( budget.original );
            node->addRoad( road ); node->connect( neighbor ); node->setMergedNodes( merged );
            auto snapshot = node->getMergedNodes();
            check( snapshot.capacity() == 67 && snapshot.get_allocator().isSourceBudgeted(),
                   "Foreign adjacency imports retain admitted spare capacity in snapshots" );
            auto connection = make_ptr<CRoadConnection>();
            Array<SmartPtr<IRoadNode>> input;
            input.reserve( 41 ); input.push_back( node );
            connection->setRoadNodes( input );
            budget.set( budget.used() );
            denied( [&]{ connection->setRoadNodes( input ); } );
            node->connect( neighbor ); // Duplicate connection is allocation free.
            node->unload( nullptr ); connection->unload( nullptr ); road->unload( nullptr );
            check( node->getConnectedNodes().empty() && node->getMergedNodes().empty() &&
                   node->getRoads().empty() && connection->getRoadNodes().empty() && snapshot[0] == neighbor,
                   "Full-cap adjacency retirement preserves external snapshots" );
            budget.set( budget.original );
        }
        {
            auto grid = make_ptr<ShadowInventory<CRoadGeneratorGrid>>();
            auto cityGenerator = make_ptr<ShadowInventory<CRoadGeneratorCity>>();
            auto city = make_ptr<CProceduralCity>();
            grid->reserveCities(); cityGenerator->reserveCities();
            grid->setCity( city ); cityGenerator->setCity( city );
            budget.set( budget.used() );
            grid->clear(); cityGenerator->unload( nullptr );
            check( grid->capacity() == 0 && cityGenerator->capacity() == 0 &&
                   !grid->getCity() && !cityGenerator->getCity(),
                   "Derived road generator teardown retires shadow inventories at the cap" );
            budget.set( budget.original );
        }
        {
            Array<Vector2<real_Num>> points;
            points.reserve( 41 ); points.push_back( Vector2<real_Num>( 2, 3 ) );
            Polygon2<real_Num> polygon( 0 );
            const auto baseline = budget.used();
            const auto bytes = sourceBudgetStorageBytes( 41 * sizeof( Vector2<real_Num> ), alignof( Vector2<real_Num> ) );
            budget.set( baseline + bytes - 1 );
            denied( [&]{ polygon.setPoints( points ); } );
            check( polygon.getNumPoints() == 0 && budget.used() == baseline, "One-byte-short polygon admission is atomic" );
            budget.set( baseline + bytes ); polygon.setPoints( points );
            check( budget.used() == baseline + bytes, "Polygon spare capacity is charged exactly" );
            budget.set( budget.original );
            auto snapshot = polygon.getPoints();
            check( snapshot.capacity() == 41 && snapshot.get_allocator().isSourceBudgeted(),
                   "Polygon caller snapshots retain SourceCpu policy" );
            Array<Vector2<real_Num>> batch;
            batch.reserve( 2 ); batch.push_back( Vector2<real_Num>( 1,2 ) );
            batch.push_back( Vector2<real_Num>( 3,4 ) );
            Polygon2<real_Num> compact( batch );
            budget.set( budget.used() );
            denied( [&]{ compact.addPoints( batch ); } );
            check( compact.getNumPoints() == 2, "Denied polygon point batch preserves the complete boundary" );
            budget.set( budget.original );
            compact.addPoints( batch );
            check( compact.getNumPoints() == 4 && compact.getPoint( 2 ) == batch[0] &&
                   compact.getPoint( 3 ) == batch[1], "Polygon point batches preserve order across buffer relocation" );
            Polygon3<real_Num> surface;
            for( const auto &point : { Vector3<real_Num>( 0,0,0 ), Vector3<real_Num>( 1,0,0 ),
                                      Vector3<real_Num>( 1,0,1 ), Vector3<real_Num>( 0,0,1 ) } )
                surface.addPoint( point );
            Array<Triangle3<real_Num>> triangles;
            triangles.push_back( Triangle3<real_Num>() );
            Plane3<real_Num> plane( Vector3<real_Num>( 0,0,0 ), Vector3<real_Num>( 0,1,0 ) );
            const auto before = budget.used();
            budget.set( before );
            denied( [&]{ surface.getTriangles( plane, triangles ); } );
            check( triangles.size() == 1 && budget.used() == before, "Denied triangulation preserves caller output" );
            budget.set( budget.original );
            check( surface.getTriangles( plane, triangles ) && triangles.size() == 3 &&
                   triangles.get_allocator().isSourceBudgeted(), "Triangulation retries with admitted output and owned scratch" );
            Polygon3<real_Num> degenerate;
            for( const auto &point : { Vector3<real_Num>( 0,0,0 ), Vector3<real_Num>( 1,0,0 ),
                                      Vector3<real_Num>( 2,0,0 ), Vector3<real_Num>( 3,0,0 ) } )
                degenerate.addPoint( point );
            const auto beforeInvalid = budget.used();
            check( !degenerate.getTriangles( plane, triangles ) && triangles.size() == 3 &&
                   budget.used() == beforeInvalid,
                   "Invalid triangulation retires index scratch and preserves prior caller output" );
            auto mesh = make_ptr<CRoadMeshElement>();
            Array<Polygon3<real_Num>> polygons;
            polygons.reserve( 23 ); polygons.push_back( surface );
            mesh->setPolygons( polygons );
            const auto live = budget.used();
            budget.set( live );
            mesh->setPolygons( polygons ); // Legacy void adapter catches and logs denial.
            check( budget.used() == live, "Denied nested polygon replacement releases staged leases" );
            budget.set( budget.original );
            Array<Vector3F> spine;
            spine.push_back( Vector3F( 0,0,0 ) ); spine.push_back( Vector3F( 1,0,0 ) );
            mesh->setPoints( spine ); mesh->build();
            auto oldVertices = mesh->getVertices();
            auto oldIndices = mesh->getIndices();
            spine.push_back( Vector3F( 2,0,0 ) ); spine.push_back( Vector3F( 3,0,0 ) );
            mesh->setPoints( spine );
            const auto vertexGrowth = sourceBudgetStorageBytes( 8 * sizeof( Vector3<real_Num> ), alignof( Vector3<real_Num> ) );
            budget.set( budget.used() + vertexGrowth );
            mesh->build(); // Admit vertices, then deny index growth before publishing either buffer.
            budget.set( budget.original );
            check( mesh->getVertices() == oldVertices && mesh->getIndices() == oldIndices,
                   "Late road mesh index admission preserves existing geometry" );
            mesh->build();
            check( mesh->getVertices().size() == 8 && mesh->getIndices().size() == 18,
                   "Road mesh rebuild retries after complete output admission" );
            budget.set( budget.used() );
            mesh->unload( nullptr );
            check( mesh->getPolygons().empty() && mesh->Points.capacity() == 0,
                   "Full-cap mesh unload retires nested polygons and spare geometry" );
            budget.set( budget.original );
        }
        {
            using Graph = PlanarGraph<Vector2<real_Num>>;
            Graph graph;
            budget.set( budget.used() );
            denied( [&]{ graph.InsertVertex( Vector2<real_Num>( 0,0 ), 0 ); } );
            budget.set( budget.original );
            graph.InsertVertex( Vector2<real_Num>( 0,0 ), 0 );
            graph.InsertVertex( Vector2<real_Num>( 1,0 ), 1 );
            graph.InsertVertex( Vector2<real_Num>( 1,1 ), 2 );
            graph.InsertVertex( Vector2<real_Num>( 0,1 ), 3 );
            for( u32 i = 0; i < 4; ++i ) graph.InsertEdge( i, ( i + 1 ) % 4 );
            Array<Graph::Primitive *> output;
            const auto baseline = budget.used();
            budget.set( baseline );
            denied( [&]{ graph.ExtractPrimitives( output ); } );
            check( output.empty() && budget.used() == baseline, "Denied planar traversal publishes no primitive or leaked scratch" );
            budget.set( budget.original );
            graph.ExtractPrimitives( output );
            check( output.size() == 1 && output.get_allocator().isSourceBudgeted() &&
                   output[0]->Sequence.get_allocator().isSourceBudgeted(),
                   "Planar extraction admits caller pointers, primitive bodies and nested sequences" );
            for( auto primitive : output ) delete primitive;
        }
        {
            FalloffGenerator<real_Num> falloff;
            falloff.setSize( 4 ); falloff.setFalloffDirection( 3 ); falloff.setFalloffRange( 3 );
            const auto baseline = budget.used();
            const auto outer = sourceBudgetStorageBytes( 4 * sizeof( Array<real_Num> ), alignof( Array<real_Num> ) );
            const auto row = sourceBudgetStorageBytes( 4 * sizeof( real_Num ), alignof( real_Num ) );
            budget.set( baseline + outer + 4 * row - 1 );
            denied( [&]{ falloff.generate(); } );
            check( budget.used() == baseline, "Late height row denial retires outer and already-admitted rows" );
            budget.set( budget.original );
            auto map = falloff.generate();
            check( map.get_allocator().isSourceBudgeted() && map[0].get_allocator().isSourceBudgeted(),
                   "Height scratch accounts both nested container layers" );
            PerlinNoiseGenerator<real_Num> noise;
            noise.setSize( 4 ); noise.setOctaves( 2 );
            auto samples = noise.generate();
            check( samples.get_allocator().isSourceBudgeted() && samples[0].get_allocator().isSourceBudgeted(),
                   "Noise output retains admitted row ownership" );
            auto terrain = make_ptr<CTerrainGeneratorDefault>();
            auto scene = make_ptr<CProceduralScene>();
            terrain->setProceduralScene( scene );
            auto options = terrain->getOptions();
            options.width = options.height = 4; options.randomize = false; options.useFalloffMap = true;
            terrain->setOptions( options ); terrain->generate();
            auto retained = terrain->getTerrain();
            budget.set( budget.used() );
            denied( [&]{ terrain->generate(); } );
            check( terrain->getTerrain() == retained && terrain->isFinished(),
                   "Denied terrain generation preserves the completed output" );
            budget.set( budget.original ); terrain->generate();
            check( terrain->getTerrain() != retained && scene->getTerrains().size() == 2,
                   "Terrain generation retries and publishes once after successful admission" );
            budget.set( budget.used() );
            scene->unload( nullptr ); retained->unload( nullptr );
            check( scene->getTerrains().empty() && retained->getHeightData().empty(),
                   "Full-cap scene and terrain teardown retires inventories and height data" );
            budget.set( budget.original );
        }
        {
            auto city = make_ptr<FinalAdmissionCity>();
            auto network = make_ptr<CRoadNetwork>();
            auto generator = make_ptr<CBlockGenerator>();
            city->setRoadNetwork( network ); generator->setCity( city );
            auto manual = make_ptr<CCityBlock>(); city->addBlock( manual );
            Array<SmartPtr<IRoadNode>> nodes;
            for( const auto &point : { Vector3<real_Num>( 0,0,0 ), Vector3<real_Num>( 10,0,0 ),
                                      Vector3<real_Num>( 10,0,10 ), Vector3<real_Num>( 0,0,10 ) } )
            {
                auto node = make_ptr<CRoadNode>(); node->setPosition( point ); nodes.push_back( node );
            }
            for( size_t i = 0; i < 4; ++i ) nodes[i]->connect( nodes[( i + 1 ) % 4] );
            network->setNodes( nodes ); generator->generate();
            auto previous = city->getBlocks();
            check( previous.size() == 2 && previous[0] == manual, "Block generation preserves manually owned blocks" );
            city->reject = true;
            denied( [&]{ generator->generate(); } );
            auto unchanged = city->getBlocks();
            check( unchanged == previous, "Final city inventory denial preserves all old block identities" );
            city->reject = false; generator->generate();
            auto replacement = city->getBlocks();
            check( replacement.size() == 2 && replacement[0] == manual && replacement[1] != previous[1],
                   "Retry replaces exactly the generator-owned block set" );
            budget.set( budget.used() ); generator->unload( nullptr );
            budget.set( budget.original );
            for( auto node : nodes ) node->clearConnections();
        }
    }
}
