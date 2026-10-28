#include <Workphone/Workphone.hpp>
#include <WPProcedural/CityGeneratorDefault.hpp>
#include <WPProcedural/CRoadGenerator.hpp>
#include <WPProcedural/CBlockGenerator.hpp>
#include <WPProcedural/CTerrainGeneratorDefault.hpp>
#include <WPProcedural/MeshGeneratorDefault.hpp>
#include <WPProcedural/CProceduralCity.hpp>
#include <WPProcedural/CProceduralWorld.hpp>
#include <WPProcedural/CProceduralScene.hpp>
#include <WPProcedural/CProceduralTerrain.hpp>
#include <WPProcedural/CCityBlock.hpp>
#include <WPProcedural/CRoadNetwork.hpp>
#include <WPProcedural/CRoadNode.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/IO/FileSystem.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <stdexcept>
#include <vector>
#include "CityInventoryBudgetContracts.hpp"
#include "RoadPayloadBudgetContracts.hpp"
#include "ProceduralStorageBudgetContracts.hpp"
#include "SplineHeightBudgetContracts.hpp"
#include "RoadNetworkPublicationContracts.hpp"
#include "RoadUtilityBudgetContracts.hpp"
#include "LegacyRoadRollbackContracts.hpp"
#include "ProceduralWorldBudgetContracts.hpp"
#include "ProceduralTextureStorageContracts.hpp"
#include "BuildingPayloadBudgetContracts.hpp"
#include "ProceduralMeshBuilderBudgetContracts.hpp"
#include "GridStorageBudgetContracts.hpp"
#include "CityCenterStorageBudgetContracts.hpp"
#include "MeshFaceStorageBudgetContracts.hpp"
#include "LSystemStorageBudgetContracts.hpp"
#include "NoiseRoadSeedBudgetContracts.hpp"
#include "ProceduralHelperBodyBudgetContracts.hpp"
#include "SkyBodyBudgetContracts.hpp"
#include "ProceduralObjectBodyBudgetContracts.hpp"

namespace
{
    using namespace workphone;
    using namespace workphone::procedural;

    void require( bool ok )
    {
        if( !ok )
        {
            throw std::runtime_error( "City generator contract failed" );
        }
    }

    std::vector<int> events;

    class Road : public CRoadGenerator
    {
    public:
        bool ready = true, fail = false;
        int loads = 0;
        SmartPtr<CityGeneratorDefault> observer;

        void generate() override
        {
            events.push_back( 1 );
            require( getCity() != nullptr );
            if( observer )
            {
                require( !observer->isFinished() );
            }
            if( fail )
            {
                throw std::runtime_error( "expected failure" );
            }
        }

        bool isFinished() const override
        {
            return ready;
        }

        void load( SmartPtr<ISharedObject> data ) override
        {
            require( data != nullptr );
            ++loads;
        }
    };

    class Block : public CBlockGenerator
    {
    public:
        void generate() override
        {
            require( getCity() != nullptr );
            events.push_back( 2 );
        }
    };

    class Terrain : public CTerrainGeneratorDefault
    {
    public:
        void generate() override
        {
            require( getProceduralScene() != nullptr );
            events.push_back( 0 );
        }

        bool isFinished() const override
        {
            return true;
        }

        SmartPtr<IProceduralTerrain> getTerrain() const override
        {
            return nullptr;
        }
    };

    class MeshSpy : public MeshGeneratorDefault
    {
    public:
        SmartPtr<IMesh> generateFromObjects( const Array<SmartPtr<IProceduralObject>> &objects ) override
        {
            require( !objects.empty() );
            events.push_back( 3 );
            return make_ptr<Mesh>();
        }
    };

    class Files : public FileSystem
    {
    public:
        String text;

        String readAllText( const String & ) override
        {
            return text;
        }
    };
}  // namespace

void checkCityGeneratorContracts()
{
    city_inventory_budget_contracts::run();
    road_payload_budget_contracts::run();
    procedural_storage_budget_contracts::run();
    spline_height_budget_contracts::run();
    road_network_publication_contracts::run();
    road_utility_budget_contracts::run();
    legacy_road_rollback_contracts::run();
    procedural_world_budget_contracts::run();
    procedural_texture_storage_contracts::run();
    building_payload_budget_contracts::run();
    procedural_mesh_builder_budget_contracts::run();
    grid_storage_budget_contracts::run();
    city_center_storage_budget_contracts::run();
    mesh_face_storage_budget_contracts::run();
    lsystem_storage_budget_contracts::run();
    noise_road_seed_budget_contracts::run();
    procedural_helper_body_budget_contracts::run();
    sky_body_budget_contracts::run();
    procedural_object_body_budget_contracts::run();
    using namespace workphone;
    using namespace workphone::procedural;
    auto coordinator = make_ptr<CityGeneratorDefault>();
    coordinator->generate();
    require( coordinator->isFinished() );
    auto a = make_ptr<CProceduralCity>(), b = make_ptr<CProceduralCity>();
    a->addBlock( make_ptr<CCityBlock>() );
    b->addBlock( make_ptr<CCityBlock>() );
    coordinator->addCity( a );
    coordinator->addCity( a );
    coordinator->addCity( nullptr );
    require( coordinator->getCities().size() == 1 );
    coordinator->removeCity( a );
    require( coordinator->getCities().empty() );
    coordinator->addCity( a );
    auto world = make_ptr<CProceduralWorld>();
    auto scene = make_ptr<CProceduralScene>();
    scene->addCity( a );
    scene->addCity( b );
    world->addScene( scene );
    coordinator->setProceduralWorld( world );
    auto road = make_ptr<Road>();
    auto block = make_ptr<Block>();
    auto terrain = make_ptr<Terrain>();
    auto mesh = make_ptr<MeshSpy>();
    coordinator->setRoadGenerator( road );
    coordinator->setBlockGenerator( block );
    coordinator->setTerrainGenerator( terrain );
    coordinator->setMeshGenerator( mesh );
    road->observer = coordinator;
    road->ready = false;
    events.clear();
    coordinator->generate();
    require( events == std::vector<int>{ 0, 1 } && !coordinator->isFinished() );
    bool rejected = false;
    try
    {
        coordinator->addCity( b );
    }
    catch( const std::logic_error & )
    {
        rejected = true;
    }
    require( rejected );
    road->ready = true;
    require( coordinator->isFinished() );
    require( events == std::vector<int>{ 0, 1, 2, 3, 1, 2, 3 } );
    require( coordinator->getGeneratedMeshes().size() == 2 );
    events.clear();
    coordinator->generate();
    require( coordinator->isFinished() );
    require( coordinator->getGeneratedMeshes().size() == 2 );
    road->fail = true;
    rejected = false;
    try
    {
        coordinator->generate();
    }
    catch( const std::runtime_error & )
    {
        rejected = true;
    }
    require( rejected && !coordinator->isFinished() );
    road->fail = false;
    coordinator->generate();
    require( coordinator->isFinished() );
    road->observer = nullptr;

    auto realCoordinator = make_ptr<CityGeneratorDefault>();
    auto realBlocks = make_ptr<CBlockGenerator>();
    realCoordinator->setBlockGenerator( realBlocks );
    for( auto city : { a, b } )
    {
        auto network = make_ptr<CRoadNetwork>();
        Array<SmartPtr<IRoadNode>> nodes;
        for( auto p : { Vector3<real_Num>( 0, 0, 0 ), Vector3<real_Num>( 10, 0, 0 ),
                        Vector3<real_Num>( 0, 0, 10 ) } )
        {
            auto node = make_ptr<CRoadNode>();
            node->setPosition( p );
            nodes.push_back( node );
        }
        nodes[0]->connect( nodes[1] );
        nodes[1]->connect( nodes[2] );
        nodes[2]->connect( nodes[0] );
        network->setNodes( nodes );
        city->setRoadNetwork( network );
        realCoordinator->addCity( city );
    }
    realCoordinator->generate();
    realCoordinator->generate();
    require( a->getBlocks().size() == 2 && b->getBlocks().size() == 2 );
    for( auto city : { a, b } )
    {
        for( auto node : city->getRoadNetwork()->getNodes() )
        {
            node->clearConnections();
        }
    }
    realCoordinator->unload( nullptr );

    auto previous = core::IApplicationManager::instancePtr();
    auto app = make_ptr<core::ApplicationManager>();
    core::IApplicationManager::setInstance( app );
    auto files = make_ptr<Files>();
    app->setFileSystem( files );
    files->text =
        "<?xml version=\"1.0\"?><osm version=\"0.6\"><node id=\"1234567890123\" lat=\"1\" "
        "lon=\"2\"/><way id=\"9\"><nd ref=\"1234567890123\"/><tag k=\"highway\" "
        "v=\"residential\"/></way></osm>";
    auto properties = coordinator->osmDataToProperties( "test.osm" );
    require( properties->getChildren().size() == 2 );
    String id;
    properties->getChildren()[0]->getPropertyValue( "id", id );
    require( id == "1234567890123" );
    coordinator->loadFromFile( "test.osm" );
    require( coordinator->getFilePath() == "test.osm" && road->loads > 0 );
    files->text = "<osm>";
    rejected = false;
    try
    {
        coordinator->loadFromFile( "bad.osm" );
    }
    catch( const std::exception & )
    {
        rejected = true;
    }
    require( rejected && coordinator->getFilePath() == "test.osm" );
    app->setFileSystem( nullptr );
    core::IApplicationManager::setInstance( previous );
    coordinator->unload( nullptr );
    coordinator->unload( nullptr );
    require( coordinator->getCities().empty() && coordinator->getGeneratedMeshes().empty() );
    require( world->getScenes().size() == 1 && scene->getCities().size() == 2 );
}
