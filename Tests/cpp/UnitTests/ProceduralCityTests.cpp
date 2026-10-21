// Shared by the complete UnitTests executable and the headless WPProceduralCityTests runner.
// Tests use deterministic inputs and never write to the project or depend on external OSM assets.
#include <Workphone/Workphone.hpp>
#include <WPProcedural/CityGeneratorDefault.hpp>
#include <WPProcedural/CProceduralCity.hpp>
#include <WPProcedural/CProceduralWorld.hpp>
#include <WPProcedural/CProceduralScene.hpp>
#include <WPProcedural/CProceduralTerrain.hpp>
#include <WPProcedural/CBlockGenerator.hpp>
#include <WPProcedural/CCityBlock.hpp>
#include <WPProcedural/CityCenter.hpp>
#include <WPProcedural/CRoadGenerator.hpp>
#include <WPProcedural/CRoadNetwork.hpp>
#include <WPProcedural/CRoadNode.hpp>
#include <WPProcedural/CRoad.hpp>
#include <WPProcedural/CTerrainGeneratorDefault.hpp>
#include <WPProcedural/MeshGeneratorDefault.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/IO/FileSystem.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <boost/test/unit_test.hpp>
#include <atomic>
#include <functional>
#include <limits>
#include <thread>
#include <vector>

namespace
{
    using namespace workphone;
    using namespace workphone::procedural;

    class CityFiles : public FileSystem
    {
    public:
        String text, requestedPath;
        unsigned reads = 0;

        String readAllText( const String &path ) override
        {
            requestedPath = path;
            ++reads;
            return text;
        }
    };

    struct Event
    {
        String stage;
        ISharedObject *target;
    };

    // Collection tests need a center identity; CityCenter's transform/name adapters remain abstract.
    class CollectionCenter : public CityCenter
    {
        String name;

    public:
        String getName() const override
        {
            return name;
        }

        void setName( const String &value ) override
        {
            name = value;
        }

        Transform3<real_Num> getTransform() const override
        {
            return getWorldTransform();
        }

        void setTransform( Transform3<real_Num> value ) override
        {
            setWorldTransform( value );
        }
    };

    // Fault injection at an interface boundary; public transforms correctly reject NaNs themselves.
    class InvalidPositionNode : public CRoadNode
    {
    public:
        Vector3<real_Num> getPosition() const override
        {
            return { std::numeric_limits<real_Num>::quiet_NaN(), 0, 0 };
        }
    };

    class CityRoadStage : public CRoadGenerator
    {
    public:
        std::vector<Event> *events = nullptr;
        std::atomic<bool> ready{ true };
        bool failRun = false, failPoll = false;
        std::function<void()> onRun;
        std::vector<SmartPtr<IProceduralCity>> loadedCities;
        std::vector<SmartPtr<ISharedObject>> loadedData;
        unsigned unloads = 0;

        void generate() override
        {
            events->push_back( { "road", getCity().get() } );
            if( onRun )
            {
                onRun();
            }
            if( failRun )
            {
                throw std::runtime_error( "road stage failed" );
            }
        }

        bool isFinished() const override
        {
            if( failPoll )
            {
                throw std::runtime_error( "road polling failed" );
            }
            return ready.load();
        }

        void load( SmartPtr<ISharedObject> data ) override
        {
            loadedCities.push_back( getCity() );
            loadedData.push_back( data );
        }

        void unload( SmartPtr<ISharedObject> ) override
        {
            ++unloads;
        }
    };

    class CityBlockStage : public CBlockGenerator
    {
    public:
        std::vector<Event> *events = nullptr;
        std::atomic<bool> ready{ true };

        void generate() override
        {
            events->push_back( { "block", getCity().get() } );
        }

        bool isFinished() const override
        {
            return ready.load();
        }
    };

    class CityTerrainStage : public CTerrainGeneratorDefault
    {
    public:
        std::vector<Event> *events = nullptr;
        std::atomic<bool> ready{ true };
        SmartPtr<IProceduralTerrain> output;

        void generate() override
        {
            events->push_back( { "terrain", getProceduralScene().get() } );
        }

        bool isFinished() const override
        {
            return ready.load();
        }

        SmartPtr<IProceduralTerrain> getTerrain() const override
        {
            return output;
        }
    };

    class CityMeshStage : public MeshGeneratorDefault
    {
    public:
        std::vector<Event> *events = nullptr;
        String failAt;
        std::vector<SmartPtr<IMesh>> outputs;

        SmartPtr<IMesh> result( const String &stage, ISharedObject *target )
        {
            events->push_back( { stage, target } );
            if( failAt == stage )
            {
                return nullptr;
            }
            auto mesh = workphone::make_ptr<Mesh>();
            outputs.push_back( mesh );
            return mesh;
        }

        SmartPtr<IMesh> generate( SmartPtr<IRoadNetwork> network ) override
        {
            return result( "road_mesh", network.get() );
        }

        SmartPtr<IMesh> generateFromTerrain( SmartPtr<IProceduralTerrain> terrain ) override
        {
            return result( "terrain_mesh", terrain.get() );
        }

        SmartPtr<IMesh> generateFromObjects( const Array<SmartPtr<IProceduralObject>> &objects ) override
        {
            if( objects.empty() )
            {
                throw std::logic_error( "empty mesh input" );
            }
            return result( "block_mesh", objects.front().get() );
        }
    };

    struct CityFixture
    {
        // Restore shared runtime state even after fatal assertions or an exception.
        SmartPtr<core::IApplicationManager> previous = core::IApplicationManager::instancePtr();
        SmartPtr<core::ApplicationManager> app = workphone::make_ptr<core::ApplicationManager>();
        SmartPtr<CityFiles> files = workphone::make_ptr<CityFiles>();
        SmartPtr<CityGeneratorDefault> generator = workphone::make_ptr<CityGeneratorDefault>();
        std::vector<Event> events;
        std::vector<SmartPtr<IRoadNode>> retainedNodes;

        CityFixture()
        {
            app->setFileSystem( files );
            core::IApplicationManager::setInstance( app );
        }

        ~CityFixture()
        {
            generator->unload( nullptr );
            for( auto node : retainedNodes )
            {
                node->clearConnections();
                node->setMergedNode( nullptr );
                node->setMergedNodes( {} );
            }
            app->setMeshManager( nullptr );
            app->setFileSystem( nullptr );
            core::IApplicationManager::setInstance( previous );
        }

        SmartPtr<CRoadNode> point( real_Num x, real_Num z, real_Num y = 0 )
        {
            auto node = workphone::make_ptr<CRoadNode>();
            node->setPosition( { x, y, z } );
            node->setGraphId( 314 );
            retainedNodes.push_back( node );
            return node;
        }

        SmartPtr<CRoadNetwork> square( SmartPtr<CProceduralCity> city, real_Num offset = 0 )
        {
            auto network = workphone::make_ptr<CRoadNetwork>();
            Array<SmartPtr<IRoadNode>> nodes{ point( offset, 0, 2 ), point( offset + 10, 0, 3 ),
                                              point( offset + 10, 10, 4 ), point( offset, 10, 5 ) };
            for( size_t i = 0; i < nodes.size(); ++i )
            {
                nodes[i]->connect( nodes[( i + 1 ) % nodes.size()] );
            }
            network->setNodes( nodes );
            city->setRoadNetwork( network );
            return network;
        }

        SmartPtr<CityRoadStage> roadStage()
        {
            auto stage = workphone::make_ptr<CityRoadStage>();
            stage->events = &events;
            generator->setRoadGenerator( stage );
            return stage;
        }

        SmartPtr<CityBlockStage> blockStage()
        {
            auto stage = workphone::make_ptr<CityBlockStage>();
            stage->events = &events;
            generator->setBlockGenerator( stage );
            return stage;
        }

        SmartPtr<CityTerrainStage> terrainStage()
        {
            auto stage = workphone::make_ptr<CityTerrainStage>();
            stage->events = &events;
            generator->setTerrainGenerator( stage );
            return stage;
        }

        SmartPtr<CityMeshStage> meshStage()
        {
            auto stage = workphone::make_ptr<CityMeshStage>();
            stage->events = &events;
            generator->setMeshGenerator( stage );
            return stage;
        }

        SmartPtr<CProceduralScene> sceneWith( SmartPtr<IProceduralCity> city )
        {
            auto scene = workphone::make_ptr<CProceduralScene>();
            scene->addCity( city );
            auto world = workphone::make_ptr<CProceduralWorld>();
            world->addScene( scene );
            generator->setProceduralWorld( world );
            return scene;
        }

        void expectStages( std::initializer_list<const char *> expected ) const
        {
            BOOST_REQUIRE_EQUAL( events.size(), expected.size() );
            size_t i = 0;
            for( auto stage : expected )
            {
                BOOST_CHECK( events[i++].stage == stage );
            }
        }
    };

    String attribute( SmartPtr<Properties> data, const String &key )
    {
        String result;
        if( !data->getPropertyValue( key, result ) )
        {
            throw std::runtime_error( "Missing XML property" );
        }
        return result;
    }
}  // namespace

BOOST_FIXTURE_TEST_SUITE( ProceduralCityProduction, CityFixture )

BOOST_AUTO_TEST_CASE( city_defaults_and_reset_preserve_content )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    const SCityOptions defaults;
    BOOST_CHECK( city->getSize() == defaults.size );
    BOOST_CHECK_EQUAL( city->getMaxBlocks(), 1024u );
    BOOST_CHECK_EQUAL( city->getMaxCenters(), 8u );
    BOOST_CHECK_EQUAL( city->getDefaultCenterRadius(), real_Num( 100 ) );
    BOOST_CHECK( city->getAutoBuildNetwork() );
    BOOST_CHECK( city->getName().empty() && !city->getRoadNetwork() );
    auto block = workphone::make_ptr<CCityBlock>();
    city->addBlock( block );
    auto network = square( city );
    city->setName( "Changed" );
    city->setSize( { 10, 20 } );
    city->setMaxBlocks( 1 );
    city->resetToDefaults();
    BOOST_CHECK( city->getName().empty() && city->getSize() == defaults.size );
    BOOST_CHECK_EQUAL( city->getMaxBlocks(), defaults.maxBlocks );
    BOOST_REQUIRE_EQUAL( city->getBlocks().size(), 1u );
    BOOST_CHECK( city->getRoadNetwork() == network && city->getBlocks()[0] == block );
}

BOOST_AUTO_TEST_CASE( city_options_round_trip_through_json )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    SCityOptions options;
    options.name = "Production City";
    options.size = { 256, 512 };
    options.minLatLong = { 10, -20 };
    options.maxLatLong = { 30, 40 };
    options.defaultCenterRadius = 35;
    options.maxCenters = 3;
    options.maxBlocks = 17;
    options.autoBuildNetwork = false;
    city->setOptions( options );
    auto properties = workphone::make_ptr<Properties>();
    city->saveOptions( properties );
    auto json = DataUtil::toString( properties.get() );
    BOOST_REQUIRE( !json.empty() );
    auto parsed = workphone::make_ptr<Properties>();
    DataUtil::parse( json, parsed.get() );
    auto restored = workphone::make_ptr<CProceduralCity>();
    restored->loadOptions( parsed );
    BOOST_CHECK( restored->getName() == options.name );
    BOOST_CHECK( restored->getSize() == options.size );
    BOOST_CHECK( restored->getMinLatLong() == options.minLatLong );
    BOOST_CHECK( restored->getMaxLatLong() == options.maxLatLong );
    BOOST_CHECK_EQUAL( restored->getDefaultCenterRadius(), options.defaultCenterRadius );
    BOOST_CHECK_EQUAL( restored->getMaxCenters(), options.maxCenters );
    BOOST_CHECK_EQUAL( restored->getMaxBlocks(), options.maxBlocks );
    BOOST_CHECK( !restored->getAutoBuildNetwork() );
}

BOOST_AUTO_TEST_CASE( city_options_validate_minima_and_nullable_properties )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    city->setSize( { -10, 0 } );
    city->setDefaultCenterRadius( -1 );
    BOOST_CHECK( city->getSize() == Vector2<real_Num>( 1, 1 ) );
    BOOST_CHECK_EQUAL( city->getDefaultCenterRadius(), real_Num( 1 ) );
    BOOST_CHECK_NO_THROW( city->saveOptions( nullptr ) );
    BOOST_CHECK_NO_THROW( city->loadOptions( nullptr ) );
    BOOST_CHECK( city->getSize() == Vector2<real_Num>( 1, 1 ) );
    city->loadOptions( workphone::make_ptr<Properties>() );
    BOOST_CHECK( city->getSize() == SCityOptions().size );
}

BOOST_AUTO_TEST_CASE( city_capacities_and_removal_preserve_other_content )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    city->setMaxBlocks( 1 );
    city->setMaxCenters( 1 );
    auto a = workphone::make_ptr<CCityBlock>(), b = workphone::make_ptr<CCityBlock>();
    auto c = workphone::make_ptr<CollectionCenter>(), d = workphone::make_ptr<CollectionCenter>();
    city->addBlock( nullptr );
    city->addCenter( nullptr );
    city->addBlock( a );
    city->addBlock( b );
    city->addCenter( c );
    city->addCenter( d );
    BOOST_REQUIRE_EQUAL( city->getBlocks().size(), 1u );
    BOOST_REQUIRE_EQUAL( city->getCityCenters().size(), 1u );
    BOOST_CHECK( city->getBlocks()[0] == a && city->getCityCenters()[0] == c );
    auto snapshot = city->getBlocks();
    snapshot.clear();
    BOOST_CHECK_EQUAL( city->getBlocks().size(), 1u );
    city->removeBlock( b );
    city->removeCenter( d );
    city->removeBlock( nullptr );
    BOOST_CHECK_EQUAL( city->getBlocks().size(), 1u );
    city->removeBlock( a );
    city->removeCenter( c );
    city->addBlock( b );
    city->addCenter( d );
    BOOST_REQUIRE_EQUAL( city->getBlocks().size(), 1u );
    BOOST_REQUIRE_EQUAL( city->getCityCenters().size(), 1u );
    BOOST_CHECK( city->getBlocks()[0] == b && city->getCityCenters()[0] == d );
}

BOOST_AUTO_TEST_CASE( zero_capacities_disable_new_content )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    city->setMaxBlocks( 0 );
    city->setMaxCenters( 0 );
    city->addBlock( workphone::make_ptr<CCityBlock>() );
    city->addCenter( workphone::make_ptr<CollectionCenter>() );
    BOOST_CHECK( city->getBlocks().empty() && city->getCityCenters().empty() );
}

BOOST_AUTO_TEST_CASE( coordinates_cover_endpoints_midpoint_extrapolation_and_degenerate_bounds )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    city->setSize( { 200, 100 } );
    city->setMinLatLong( { 10, 20 } );
    city->setMaxLatLong( { 20, 40 } );
    BOOST_CHECK( city->getRelativeCoordinates( { 10, 20 } ) == Vector2<real_Num>( 0, 0 ) );
    BOOST_CHECK( city->getRelativeCoordinates( { 20, 40 } ) == Vector2<real_Num>( 200, 100 ) );
    BOOST_CHECK( city->getRelativeCoordinates( { 15, 30 } ) == Vector2<real_Num>( 100, 50 ) );
    BOOST_CHECK( city->getRelativeCoordinates( { 5, 50 } ) == Vector2<real_Num>( -100, 150 ) );
    city->setMaxLatLong( { 10, 40 } );
    BOOST_CHECK( city->getRelativeCoordinates( { 15, 30 } ) == Vector2<real_Num>::zero() );
    city->setMaxLatLong( { 20, 20 } );
    BOOST_CHECK( city->getRelativeCoordinates( { 15, 30 } ) == Vector2<real_Num>::zero() );
}

BOOST_AUTO_TEST_CASE( city_spatial_query_and_named_road_lookup )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    city->setSize( { 100, 100 } );
    BOOST_CHECK( city->isWithin( Sphere3F( { 50, 0, 50 }, 1 ) ) );
    BOOST_CHECK( city->isWithin( Sphere3F( { 100, 0, 100 }, 1 ) ) );
    BOOST_CHECK( !city->isWithin( Sphere3F( { 1000, 0, 1000 }, 1 ) ) );
    auto network = square( city );
    auto road = workphone::make_ptr<CRoad>();
    road->setName( "Main" );
    network->addRoad( road );
    BOOST_CHECK( city->getRoadByName( "Main" ) == road );
    BOOST_CHECK( !city->getRoadByName( "missing" ) );
    auto roads = city->getRoads();
    roads.clear();
    BOOST_CHECK_EQUAL( city->getRoads().size(), 1u );
}

BOOST_AUTO_TEST_CASE( city_unload_is_idempotent_and_retains_configuration )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    auto network = square( city );
    city->setName( "Keep settings" );
    city->addBlock( workphone::make_ptr<CCityBlock>() );
    city->addCenter( workphone::make_ptr<CollectionCenter>() );
    city->unload( nullptr );
    city->unload( nullptr );
    BOOST_CHECK( !city->getRoadNetwork() && city->getBlocks().empty() &&
                 city->getCityCenters().empty() );
    BOOST_CHECK( network->getNodes().empty() );
    BOOST_CHECK( city->getName() == "Keep settings" );
}

BOOST_AUTO_TEST_CASE( coordinator_empty_configuration_and_unique_city_snapshots )
{
    BOOST_CHECK( generator->isFinished() );
    generator->generate();
    BOOST_CHECK( generator->isFinished() );
    auto city = workphone::make_ptr<CProceduralCity>();
    generator->addCity( nullptr );
    generator->addCity( city );
    generator->addCity( city );
    auto copy = generator->getCities();
    BOOST_REQUIRE_EQUAL( copy.size(), 1u );
    copy.clear();
    BOOST_CHECK_EQUAL( generator->getCities().size(), 1u );
    generator->removeCity( nullptr );
    generator->removeCity( workphone::make_ptr<CProceduralCity>() );
    BOOST_CHECK_EQUAL( generator->getCities().size(), 1u );
    generator->removeCity( city );
    generator->removeCity( city );
    BOOST_CHECK( generator->getCities().empty() );
}

BOOST_AUTO_TEST_CASE( ordered_pipeline_deduplicates_world_cities_and_retains_meshes )
{
    auto a = workphone::make_ptr<CProceduralCity>(), b = workphone::make_ptr<CProceduralCity>();
    auto network = square( a );
    network->addRoad( workphone::make_ptr<CRoad>() );
    auto block = workphone::make_ptr<CCityBlock>();
    a->addBlock( block );
    b->addBlock( block );
    auto scene = sceneWith( a );
    scene->addCity( a );
    scene->addCity( b );
    scene->addCity( nullptr );
    generator->getProceduralWorld()->addScene( scene );
    generator->getProceduralWorld()->addScene( nullptr );
    generator->addCity( a );
    auto terrain = terrainStage();
    terrain->output = workphone::make_ptr<CProceduralTerrain>();
    roadStage();
    blockStage();
    auto meshes = meshStage();
    generator->generate();
    BOOST_CHECK( generator->isFinished() );
    expectStages( { "terrain", "terrain_mesh", "road", "block", "road_mesh", "block_mesh", "road",
                    "block", "block_mesh" } );
    BOOST_CHECK( events[0].target == scene.get() );
    BOOST_CHECK( events[2].target == a.get() && events[6].target == b.get() );
    auto results = generator->getGeneratedMeshes();
    BOOST_REQUIRE_EQUAL( results.size(), 4u );
    for( size_t i = 0; i < results.size(); ++i )
    {
        BOOST_CHECK( results[i] == meshes->outputs[i] );
    }
    results.clear();
    BOOST_CHECK_EQUAL( generator->getGeneratedMeshes().size(), 4u );
}

BOOST_AUTO_TEST_CASE( async_terrain_blocks_dependent_stages_until_polled_complete )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    sceneWith( city );
    auto terrain = terrainStage();
    terrain->ready = false;
    roadStage();
    generator->generate();
    BOOST_CHECK( !generator->isFinished() );
    expectStages( { "terrain" } );
    terrain->ready = true;
    BOOST_CHECK( generator->isFinished() );
    expectStages( { "terrain", "road" } );
}

BOOST_AUTO_TEST_CASE( async_road_blocks_blocks_and_the_next_city )
{
    generator->addCity( workphone::make_ptr<CProceduralCity>() );
    generator->addCity( workphone::make_ptr<CProceduralCity>() );
    auto road = roadStage();
    road->ready = false;
    blockStage();
    generator->generate();
    for( int i = 0; i < 5; ++i )
    {
        BOOST_CHECK( !generator->isFinished() );
    }
    expectStages( { "road" } );
    std::thread worker( [&]() { road->ready.store( true ); } );
    worker.join();
    BOOST_CHECK( generator->isFinished() );
    expectStages( { "road", "block", "road", "block" } );
}

BOOST_AUTO_TEST_CASE( async_block_waits_before_mesh_generation )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    city->addBlock( workphone::make_ptr<CCityBlock>() );
    generator->addCity( city );
    auto block = blockStage();
    block->ready = false;
    meshStage();
    generator->generate();
    BOOST_CHECK( !generator->isFinished() );
    expectStages( { "block" } );
    block->ready = true;
    BOOST_CHECK( generator->isFinished() );
    expectStages( { "block", "block_mesh" } );
}

BOOST_AUTO_TEST_CASE( running_pipeline_rejects_configuration_mutations )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    generator->addCity( city );
    auto road = roadStage();
    road->ready = false;
    generator->generate();
    BOOST_CHECK_THROW( generator->generate(), std::logic_error );
    BOOST_CHECK_THROW( generator->addCity( city ), std::logic_error );
    BOOST_CHECK_THROW( generator->removeCity( city ), std::logic_error );
    BOOST_CHECK_THROW( generator->setFilePath( "change" ), std::logic_error );
    BOOST_CHECK_THROW( generator->setRoadGenerator( nullptr ), std::logic_error );
    BOOST_CHECK_THROW( generator->setBlockGenerator( nullptr ), std::logic_error );
    BOOST_CHECK_THROW( generator->setMeshGenerator( nullptr ), std::logic_error );
    BOOST_CHECK_THROW( generator->setTerrainGenerator( nullptr ), std::logic_error );
    BOOST_CHECK_THROW( generator->setProceduralWorld( nullptr ), std::logic_error );
    BOOST_CHECK_THROW( generator->load( nullptr ), std::logic_error );
    BOOST_CHECK_THROW( generator->loadFromFile( "unused.osm" ), std::logic_error );
    BOOST_CHECK_EQUAL( files->reads, 0u );
    road->ready = true;
    BOOST_CHECK( generator->isFinished() );
    BOOST_CHECK_NO_THROW( generator->setFilePath( "idle" ) );
}

BOOST_AUTO_TEST_CASE( reentrant_completion_query_cannot_start_a_dependent_stage )
{
    generator->addCity( workphone::make_ptr<CProceduralCity>() );
    auto road = roadStage();
    blockStage();
    road->onRun = [&]() {
        BOOST_CHECK( !generator->isFinished() );
        expectStages( { "road" } );
        BOOST_CHECK_THROW( generator->generate(), std::logic_error );
    };
    generator->generate();
    expectStages( { "road", "block" } );
    BOOST_CHECK( generator->isFinished() );
}

BOOST_AUTO_TEST_CASE( generation_exception_stops_downstream_work_and_allows_retry )
{
    generator->addCity( workphone::make_ptr<CProceduralCity>() );
    auto road = roadStage();
    blockStage();
    road->failRun = true;
    BOOST_CHECK_THROW( generator->generate(), std::runtime_error );
    BOOST_CHECK( !generator->isFinished() );
    expectStages( { "road" } );
    road->failRun = false;
    events.clear();
    generator->generate();
    BOOST_CHECK( generator->isFinished() );
    expectStages( { "road", "block" } );
}

BOOST_AUTO_TEST_CASE( polling_exception_stops_downstream_work_and_allows_retry )
{
    generator->addCity( workphone::make_ptr<CProceduralCity>() );
    auto road = roadStage();
    blockStage();
    road->ready = false;
    generator->generate();
    road->failPoll = true;
    BOOST_CHECK_THROW( generator->isFinished(), std::runtime_error );
    BOOST_CHECK( !generator->isFinished() );
    expectStages( { "road" } );
    road->failPoll = false;
    road->ready = true;
    events.clear();
    generator->generate();
    BOOST_CHECK( generator->isFinished() );
    expectStages( { "road", "block" } );
}

BOOST_AUTO_TEST_CASE( null_mesh_results_fail_the_run_and_allow_retry )
{
    for( auto failAt : { "terrain_mesh", "road_mesh", "block_mesh" } )
    {
        BOOST_TEST_CONTEXT( failAt )
        {
            generator->unload( nullptr );
            events.clear();
            auto city = workphone::make_ptr<CProceduralCity>();
            square( city )->addRoad( workphone::make_ptr<CRoad>() );
            city->addBlock( workphone::make_ptr<CCityBlock>() );
            sceneWith( city );
            auto terrain = terrainStage();
            terrain->output = workphone::make_ptr<CProceduralTerrain>();
            auto mesh = meshStage();
            mesh->failAt = failAt;
            BOOST_CHECK_THROW( generator->generate(), std::runtime_error );
            BOOST_CHECK( !generator->isFinished() );
            BOOST_REQUIRE( !events.empty() );
            BOOST_CHECK( events.back().stage == failAt );
            mesh->failAt.clear();
            BOOST_CHECK_NO_THROW( generator->generate() );
            BOOST_CHECK( generator->isFinished() );
        }
    }
}

BOOST_AUTO_TEST_CASE( repeated_runs_replace_mesh_results_and_skip_absent_geometry )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    city->addBlock( workphone::make_ptr<CCityBlock>() );
    generator->addCity( city );
    meshStage();
    generator->generate();
    auto first = generator->getGeneratedMeshes();
    BOOST_REQUIRE_EQUAL( first.size(), 1u );
    generator->generate();
    auto second = generator->getGeneratedMeshes();
    BOOST_REQUIRE_EQUAL( second.size(), 1u );
    BOOST_CHECK( first[0] != second[0] );
    city->removeBlock( city->getBlocks()[0] );
    events.clear();
    generator->generate();
    BOOST_CHECK( generator->getGeneratedMeshes().empty() && events.empty() );
}

BOOST_AUTO_TEST_CASE( unload_pending_work_preserves_external_world_and_children )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    auto scene = sceneWith( city );
    auto world = generator->getProceduralWorld();
    auto road = roadStage();
    road->ready = false;
    blockStage();
    generator->generate();
    generator->unload( nullptr );
    generator->unload( nullptr );
    road->ready = true;
    BOOST_CHECK( generator->isFinished() );
    expectStages( { "road" } );
    BOOST_CHECK( !generator->getRoadGenerator() && !generator->getBlockGenerator() &&
                 !generator->getProceduralWorld() );
    BOOST_CHECK( !generator->getInput() && !generator->getOutput() && !generator->getParent() );
    BOOST_CHECK( generator->getCities().empty() && generator->getGeneratedMeshes().empty() );
    BOOST_CHECK_EQUAL( road->unloads, 0u );
    BOOST_CHECK_EQUAL( world->getScenes().size(), 1u );
    BOOST_CHECK_EQUAL( scene->getCities().size(), 1u );
    BOOST_CHECK_NO_THROW( generator->generate() );
}

BOOST_AUTO_TEST_CASE( osm_parser_preserves_ids_order_entities_and_nested_elements )
{
    files->text =
        R"xml(<?xml version="1.0"?><osm version="0.6"><!--ignored--><node id="9223372036854775806" lat="51.123456789" lon="-0.1"/><way id="7"><nd ref="9223372036854775806"/><tag k="name" v="A &amp; B"/></way><relation id="8"><member type="way" ref="7" role="outer"/></relation></osm>)xml";
    auto data = generator->osmDataToProperties( "memory.osm" );
    BOOST_CHECK( attribute( data, "element" ) == "osm" );
    BOOST_CHECK( attribute( data, "version" ) == "0.6" );
    BOOST_REQUIRE_EQUAL( data->getChildren().size(), 3u );
    auto node = data->getChildren()[0], way = data->getChildren()[1], relation = data->getChildren()[2];
    BOOST_CHECK( attribute( node, "element" ) == "node" );
    BOOST_CHECK( attribute( node, "id" ) == "9223372036854775806" );
    BOOST_CHECK( attribute( node, "lat" ) == "51.123456789" );
    BOOST_REQUIRE_EQUAL( way->getChildren().size(), 2u );
    BOOST_CHECK( attribute( way->getChildren()[0], "ref" ) == attribute( node, "id" ) );
    BOOST_CHECK( attribute( way->getChildren()[1], "v" ) == "A & B" );
    BOOST_REQUIRE_EQUAL( relation->getChildren().size(), 1u );
    BOOST_CHECK( attribute( relation->getChildren()[0], "role" ) == "outer" );
    BOOST_CHECK( files->requestedPath == "memory.osm" );
}

BOOST_AUTO_TEST_CASE( invalid_osm_fails_before_dispatch_and_preserves_file_path )
{
    generator->addCity( workphone::make_ptr<CProceduralCity>() );
    auto road = roadStage();
    generator->setFilePath( "previous.osm" );
    BOOST_CHECK_THROW( generator->loadFromFile( "" ), std::invalid_argument );
    for( auto text : { "", "not xml", "<osm>", "<configuration/>" } )
    {
        BOOST_TEST_CONTEXT( text )
        {
            files->text = text;
            BOOST_CHECK_THROW( generator->loadFromFile( "broken.osm" ), std::exception );
            BOOST_CHECK( generator->getFilePath() == "previous.osm" );
            BOOST_CHECK( road->loadedData.empty() );
        }
    }
    files->text = "<osm/>";
    BOOST_CHECK_NO_THROW( generator->loadFromFile( "empty-valid.osm" ) );
    BOOST_CHECK( generator->getFilePath() == "empty-valid.osm" );
    BOOST_REQUIRE_EQUAL( road->loadedData.size(), 1u );
}

BOOST_AUTO_TEST_CASE( missing_services_fail_without_null_dereferences )
{
    app->setFileSystem( nullptr );
    BOOST_CHECK_THROW( generator->osmDataToProperties( "city.osm" ), std::runtime_error );
    core::IApplicationManager::setInstance( nullptr );
    BOOST_CHECK_THROW( generator->osmDataToProperties( "city.osm" ), std::runtime_error );
}

BOOST_AUTO_TEST_CASE( loading_dispatches_once_per_city_and_configured_file_takes_precedence )
{
    auto a = workphone::make_ptr<CProceduralCity>(), b = workphone::make_ptr<CProceduralCity>();
    auto scene = sceneWith( a );
    scene->addCity( b );
    generator->addCity( a );
    auto road = roadStage();
    auto data = workphone::make_ptr<Properties>();
    generator->load( data );
    BOOST_REQUIRE_EQUAL( road->loadedCities.size(), 2u );
    BOOST_CHECK( road->loadedCities[0] == a && road->loadedCities[1] == b );
    BOOST_CHECK( road->loadedData[0] == data && road->loadedData[1] == data );
    files->text = "<osm/>";
    generator->setFilePath( "configured.osm" );
    generator->load( data );
    BOOST_CHECK( files->requestedPath == "configured.osm" );
    BOOST_REQUIRE_EQUAL( road->loadedData.size(), 4u );
    BOOST_CHECK( road->loadedData[2] != data && road->loadedData[2] == road->loadedData[3] );
    BOOST_CHECK( generator->getLoadingState() == LoadingState::Loaded );
}

BOOST_AUTO_TEST_CASE( real_blocks_preserve_elevations_ids_and_manual_content_across_cities )
{
    auto a = workphone::make_ptr<CProceduralCity>(), b = workphone::make_ptr<CProceduralCity>();
    square( a );
    square( b, 50 );
    auto manual = workphone::make_ptr<CCityBlock>();
    a->addBlock( manual );
    generator->addCity( a );
    generator->addCity( b );
    generator->setBlockGenerator( workphone::make_ptr<CBlockGenerator>() );
    for( int run = 0; run < 4; ++run )
    {
        generator->generate();
        BOOST_REQUIRE( generator->isFinished() );
        BOOST_REQUIRE_EQUAL( a->getBlocks().size(), 2u );
        BOOST_REQUIRE_EQUAL( b->getBlocks().size(), 1u );
        BOOST_CHECK( a->getBlocks()[0] == manual );
        const auto points = a->getBlocks()[1]->getPoints();
        BOOST_REQUIRE_EQUAL( points.size(), 4u );
        for( const auto &p : points )
        {
            BOOST_CHECK( p.Y() >= 2 && p.Y() <= 5 );
        }
    }
    for( auto node : retainedNodes )
    {
        BOOST_CHECK_EQUAL( node->getGraphId(), 314 );
    }
}

BOOST_AUTO_TEST_CASE( invalid_embeddings_preserve_published_blocks )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    auto network = square( city );
    auto blocks = workphone::make_ptr<CBlockGenerator>();
    blocks->setCity( city );
    blocks->generate();
    auto published = city->getBlocks();
    BOOST_REQUIRE_EQUAL( published.size(), 1u );
    auto nodes = network->getNodes();
    nodes[0]->connect( nodes[2] );
    nodes[1]->connect( nodes[3] );
    BOOST_CHECK_THROW( blocks->generate(), std::invalid_argument );
    BOOST_REQUIRE_EQUAL( city->getBlocks().size(), 1u );
    BOOST_CHECK( city->getBlocks()[0] == published[0] );
    for( auto node : nodes )
    {
        node->clearConnections();
    }
    nodes[0] = workphone::make_ptr<InvalidPositionNode>();
    network->setNodes( nodes );
    BOOST_CHECK_THROW( blocks->generate(), std::invalid_argument );
    BOOST_REQUIRE_EQUAL( city->getBlocks().size(), 1u );
    BOOST_CHECK( city->getBlocks()[0] == published[0] );
}

BOOST_AUTO_TEST_CASE( open_chains_and_isolated_nodes_never_become_blocks )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    auto network = workphone::make_ptr<CRoadNetwork>();
    Array<SmartPtr<IRoadNode>> nodes{ point( 0, 0 ), point( 10, 0 ), point( 10, 10 ), point( 30, 30 ) };
    nodes[0]->connect( nodes[1] );
    nodes[1]->connect( nodes[2] );
    network->setNodes( nodes );
    city->setRoadNetwork( network );
    generator->addCity( city );
    generator->setBlockGenerator( workphone::make_ptr<CBlockGenerator>() );
    generator->generate();
    BOOST_CHECK( generator->isFinished() && city->getBlocks().empty() );
}

BOOST_AUTO_TEST_CASE( grid_blocks_have_expected_count_area_and_stable_regeneration )
{
    constexpr unsigned width = 8, height = 7;
    auto city = workphone::make_ptr<CProceduralCity>();
    auto network = workphone::make_ptr<CRoadNetwork>();
    Array<SmartPtr<IRoadNode>> nodes;
    for( unsigned y = 0; y < height; ++y )
    {
        for( unsigned x = 0; x < width; ++x )
        {
            nodes.push_back( point( x * 10, y * 10 ) );
        }
    }
    for( unsigned y = 0; y < height; ++y )
    {
        for( unsigned x = 0; x < width; ++x )
        {
            if( x + 1 < width )
            {
                nodes[y * width + x]->connect( nodes[y * width + x + 1] );
            }
            if( y + 1 < height )
            {
                nodes[y * width + x]->connect( nodes[( y + 1 ) * width + x] );
            }
        }
    }
    network->setNodes( nodes );
    city->setRoadNetwork( network );
    generator->addCity( city );
    generator->setBlockGenerator( workphone::make_ptr<CBlockGenerator>() );
    for( int run = 0; run < 5; ++run )
    {
        generator->generate();
        const auto blocks = city->getBlocks();
        BOOST_REQUIRE_EQUAL( blocks.size(), ( width - 1 ) * ( height - 1 ) );
        real_Num totalArea = 0;
        for( auto block : blocks )
        {
            const auto points = block->getPoints();
            BOOST_REQUIRE_EQUAL( points.size(), 4u );
            real_Num twiceArea = 0;
            for( size_t i = 0; i < points.size(); ++i )
            {
                const auto &a = points[i], &b = points[( i + 1 ) % points.size()];
                twiceArea += a.X() * b.Z() - a.Z() * b.X();
            }
            BOOST_CHECK_EQUAL( twiceArea, real_Num( 200 ) );
            totalArea += twiceArea / 2;
        }
        BOOST_CHECK_EQUAL( totalArea, real_Num( 4200 ) );
    }
}

BOOST_AUTO_TEST_CASE( real_road_mesh_pipeline_produces_geometry_without_graphics )
{
    auto factories = workphone::make_ptr<FactoryManager>();
    factories->load( nullptr );
    app->setFactoryManager( factories );
    app->setMeshManager( workphone::make_ptr<MeshManager>() );
    auto city = workphone::make_ptr<CProceduralCity>();
    auto network = workphone::make_ptr<CRoadNetwork>();
    auto road = workphone::make_ptr<CRoad>();
    road->addNode( point( 0, 0 ) );
    road->addNode( point( 10, 0 ) );
    network->addRoad( road );
    city->setRoadNetwork( network );
    generator->addCity( city );
    generator->setMeshGenerator( workphone::make_ptr<MeshGeneratorDefault>() );
    generator->generate();
    BOOST_REQUIRE( generator->isFinished() );
    const auto meshes = generator->getGeneratedMeshes();
    BOOST_REQUIRE_EQUAL( meshes.size(), 1u );
    BOOST_REQUIRE( meshes[0] );
    BOOST_REQUIRE_EQUAL( meshes[0]->getNumSubMeshes(), 1u );
    auto submesh = meshes[0]->getSubMesh( 0 );
    BOOST_REQUIRE( submesh );
    BOOST_REQUIRE( submesh->getVertexBuffer() && submesh->getIndexBuffer() );
    BOOST_CHECK_GE( submesh->getVertexBuffer()->getNumVertices(), 6u );
    BOOST_CHECK_GE( submesh->getIndexBuffer()->getNumIndices(), 6u );
}

BOOST_AUTO_TEST_CASE( concurrent_city_registration_is_unique_and_snapshots_are_consistent )
{
    std::vector<SmartPtr<IProceduralCity>> cities;
    for( unsigned i = 0; i < 8; ++i )
    {
        cities.push_back( workphone::make_ptr<CProceduralCity>() );
    }
    std::atomic<bool> valid{ true };
    std::vector<std::thread> workers;
    for( unsigned worker = 0; worker < 4; ++worker )
    {
        workers.emplace_back( [&]() {
            for( unsigned repeat = 0; repeat < 100; ++repeat )
            {
                for( auto city : cities )
                {
                    generator->addCity( city );
                }
                if( generator->getCities().size() > cities.size() )
                {
                    valid = false;
                }
            }
        } );
    }
    for( auto &worker : workers )
    {
        worker.join();
    }
    BOOST_CHECK( valid.load() );
    BOOST_CHECK_EQUAL( generator->getCities().size(), cities.size() );
    generator->generate();
    BOOST_CHECK( generator->isFinished() );
}

BOOST_AUTO_TEST_CASE( serialized_negative_capacities_do_not_wrap_to_unbounded_limits )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    auto options = workphone::make_ptr<Properties>();
    options->setProperty( "MaxCenters", s32( -1 ) );
    options->setProperty( "MaxBlocks", std::numeric_limits<s32>::min() );
    city->loadOptions( options );
    BOOST_CHECK_EQUAL( city->getMaxCenters(), 0u );
    BOOST_CHECK_EQUAL( city->getMaxBlocks(), 0u );
    city->addBlock( workphone::make_ptr<CCityBlock>() );
    city->addCenter( workphone::make_ptr<CollectionCenter>() );
    BOOST_CHECK( city->getBlocks().empty() && city->getCityCenters().empty() );
}

BOOST_AUTO_TEST_CASE( merged_source_aliases_close_the_boundary_without_reassigning_ids )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    auto network = square( city );
    auto nodes = network->getNodes();
    auto alias = point( 0, 0, 2 );
    alias->setMergedNode( nodes[0] );
    nodes[3]->disconnect( nodes[0] );
    nodes[3]->connect( alias );
    nodes.push_back( alias );
    nodes.push_back( nullptr );
    network->setNodes( nodes );
    network->setMergedNodes( { nodes[0] } );
    auto blocks = workphone::make_ptr<CBlockGenerator>();
    blocks->setCity( city );
    blocks->generate();
    BOOST_REQUIRE_EQUAL( city->getBlocks().size(), 1u );
    BOOST_CHECK_EQUAL( city->getBlocks()[0]->getPoints().size(), 4u );
    BOOST_CHECK_EQUAL( alias->getGraphId(), 314 );
}

BOOST_AUTO_TEST_CASE( cyclic_merge_references_fail_without_publishing_partial_blocks )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    auto network = square( city );
    auto nodes = network->getNodes();
    nodes[0]->setMergedNode( nodes[1] );
    nodes[1]->setMergedNode( nodes[0] );
    auto blocks = workphone::make_ptr<CBlockGenerator>();
    blocks->setCity( city );
    BOOST_CHECK_THROW( blocks->generate(), std::invalid_argument );
    BOOST_CHECK( city->getBlocks().empty() );
}

BOOST_AUTO_TEST_CASE( unsplit_junctions_and_vertical_overlaps_fail_without_publication )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    auto network = square( city );
    auto nodes = network->getNodes();
    nodes.push_back( point( 5, 0, 2 ) );
    network->setNodes( nodes );
    auto blocks = workphone::make_ptr<CBlockGenerator>();
    blocks->setCity( city );
    BOOST_CHECK_THROW( blocks->generate(), std::invalid_argument );
    BOOST_CHECK( city->getBlocks().empty() );
    nodes.back() = point( 0, 0, 200 );
    network->setNodes( nodes );
    BOOST_CHECK_THROW( blocks->generate(), std::invalid_argument );
    BOOST_CHECK( city->getBlocks().empty() );
}

BOOST_AUTO_TEST_CASE( clearing_the_network_removes_generated_blocks_but_keeps_authored_content )
{
    auto city = workphone::make_ptr<CProceduralCity>();
    auto network = square( city );
    auto manual = workphone::make_ptr<CCityBlock>();
    city->addBlock( manual );
    generator->addCity( city );
    generator->setBlockGenerator( workphone::make_ptr<CBlockGenerator>() );
    generator->generate();
    BOOST_REQUIRE_EQUAL( city->getBlocks().size(), 2u );
    network->setNodes( {} );
    generator->generate();
    BOOST_REQUIRE_EQUAL( city->getBlocks().size(), 1u );
    BOOST_CHECK( city->getBlocks()[0] == manual );
}

BOOST_AUTO_TEST_SUITE_END()
