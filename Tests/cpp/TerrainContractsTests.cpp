#include <Workphone/Workphone.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Graphics/Terrain.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Scene/Transform.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/System/TimerMT.hpp>
#include <Workphone/System/StateManager.hpp>
#include "TerrainDataContracts.hpp"
#include "TerrainEditingContracts.hpp"
#include <cstdio>
#include <stdexcept>

using namespace workphone;

static void require( bool condition, const char *message )
{
    if( !condition )
        throw std::runtime_error( message );
}

static void persistenceContracts()
{
    auto source = make_ptr<scene::TerrainSystem>();
    render::TerrainData data;
    data.dimensions = { 3, 5 };
    data.spacing = { 2, 3 };
    data.origin = { -2, -6 };
    data.heightScale = -1.25f;
    data.heights = { 0, 1.2f, 4, 3, 9, 2, 0, 4, 1, 8, 2, 7, 0, 4, 5 };
    String error;
    require( source->applyTerrainData( data, error ), "component accepts rectangular data" );
    auto retained = source->getTerrainSnapshot();
    auto json = source->exportTerrainData();
    require( !json.empty(), "sample export is present" );
    auto fresh = make_ptr<scene::TerrainSystem>();
    require( fresh->importTerrainData( json, error ), "sample export imports into fresh component" );
    require( fresh->exportTerrainData() == json,
             "sample export roundtrips exact float values and metadata" );
    auto runtime = make_ptr<render::Terrain>();
    fresh->setTerrain( runtime );
    require( runtime->getTerrainSnapshot()->heights == data.heights &&
                 runtime->getTerrainSnapshot()->origin == data.origin,
             "runtime recreation receives component-owned samples and placement" );
    fresh->setTerrain( nullptr );
    auto secondRuntime = make_ptr<render::Terrain>();
    fresh->setTerrain( secondRuntime );
    require( secondRuntime->getHeightData() == data.heights, "renderer recreation retains terrain" );
    auto current = fresh->getTerrainSnapshot();
    for( const String invalid : { String( "{}" ), String( "[]" ), json + "garbage",
                                  String( "{\"version\":999}" ), String( "null" ) } )
    {
        require( !fresh->importTerrainData( invalid, error ) && !error.empty(),
                 "bad import must report failure" );
        require( fresh->getTerrainSnapshot() == current, "bad import preserves last valid snapshot" );
    }
    auto badCount = json;
    const auto width = badCount.find( "\"width\":3" );
    require( width != String::npos, "fixture width field" );
    badCount.replace( width, 9, "\"width\":4" );
    require( !fresh->importTerrainData( badCount, error ), "mismatched sample count rejected" );
    data.heights[0] = 99;
    require( source->applyTerrainData( data, error, retained->revision ),
             "revision-checked edit commits" );
    require( retained->heights[0] == 0, "published snapshots remain immutable" );
    require( !source->applyTerrainData( *retained, error, retained->revision ), "stale edit rejected" );

    auto properties = static_pointer_cast<Properties>( source->toData() );
    require( properties != nullptr, "unattached component serializes sample properties" );
    const auto sceneJson = DataUtil::toString( properties.get() );
    auto decoded = make_ptr<Properties>();
    DataUtil::parse( sceneJson, decoded.get() );
    auto reopened = make_ptr<scene::TerrainSystem>();
    reopened->setProperties( decoded );
    require( reopened->exportTerrainData() == source->exportTerrainData(),
             "scene properties preserve actual samples" );
    auto inspector = reopened->getProperties();
    require( !inspector->hasProperty( "terrainDataV1" ),
             "sample blobs stay out of inspector properties" );
    inspector->setProperty( scene::TerrainSystem::HeightScaleStr, 3.0f );
    reopened->setProperties( inspector );
    require(
        reopened->getHeightScale() == 3.0f && reopened->getTerrainSnapshot()->heights == data.heights,
        "inspector metadata changes preserve edited samples" );
    auto legacy = make_ptr<scene::TerrainSystem>();
    auto legacyProperties = make_ptr<Properties>();
    legacyProperties->setProperty( scene::TerrainSystem::HeightMapSizeStr, Vector2I( 3, 5 ) );
    legacyProperties->setProperty( scene::TerrainSystem::HeightScaleStr, 2.0f );
    legacy->setProperties( legacyProperties );
    require( legacy->getTerrainSnapshot()->origin == Vector2F( -1.5f, -2.5f ) &&
                 legacy->getTerrainSnapshot()->heights.size() == 15,
             "legacy property migration preserves old render coordinates" );
    legacy->setGeneratedHeightMapWidth( 3 );
    legacy->setGeneratedHeightMapHeight( 5 );
    legacy->generateHeightMap();
    require( legacy->getTerrainSnapshot()->dimensions == Vector2I( 3, 5 ) &&
                 legacy->getTerrainSnapshot()->heights.back() > 0,
             "generator creates rectangular persisted samples without a renderer" );
    std::puts( "Terrain component persistence, migration and publication: PASS" );
}

static void attachmentContracts()
{
    auto component = make_ptr<scene::TerrainSystem>();
    render::TerrainData data;
    data.dimensions = { 3, 5 };
    data.spacing = { 2, 3 };
    data.origin = { -2, -6 };
    data.heightScale = -1.25f;
    data.heights.assign( 15, 4.0f );
    String error;
    require( component->applyTerrainData( data, error ), "attachment fixture source validates" );
    const auto source = component->getTerrainSnapshot();
    auto actor = make_ptr<scene::GameActor>();
    auto actorTransform = make_ptr<scene::Transform>();
    Transform3<real_Num> placement;
    placement.setPosition( { 10, 7, 20 } );
    placement.setScale( { 2, 3, 4 } );
    actorTransform->setWorldTransform( placement );
    actor->setTransform( actorTransform );
    component->setActor( actor );

    auto first = make_ptr<render::Terrain>();
    component->setTerrain( first );
    require( component->getTerrain() == first && first->getWorldTransform() == placement &&
                 first->getHeightAtWorldPosition( { 10, 0, 20 } ) == -8,
             "runtime attachment receives an already-transformed actor's placement and world height" );
    component->setTerrain( nullptr );
    auto second = make_ptr<render::Terrain>();
    component->setTerrain( second );
    require( component->getTerrain() == second && second->getWorldTransform() == placement &&
                 second->getHeightAtWorldPosition( { 10, 0, 20 } ) == -8 &&
                 second->getHeightData() == data.heights && component->getTerrainSnapshot() == source,
             "runtime recreation preserves actor placement without a new transform event" );

    auto invalidPlacement = placement;
    invalidPlacement.setScale( { 0, 3, 4 } );
    actorTransform->setWorldTransform( invalidPlacement );
    auto rejected = make_ptr<render::Terrain>();
    const auto rejectedBefore = rejected->getTerrainSnapshot();
    const auto retained = second->getTerrainSnapshot();
    component->setTerrain( rejected );
    require(
        component->getTerrain() == second && second->getWorldTransform() == placement &&
            second->getTerrainSnapshot() == retained &&
            rejected->getTerrainSnapshot() == rejectedBefore &&
            component->getTerrainSnapshot() == source,
        "invalid actor placement rejects attachment before changing the prior runtime or candidate" );
    actorTransform->setWorldTransform( placement );
    component->setActor( nullptr );
    std::puts( "Terrain runtime actor placement and reattachment: PASS" );
}

int main()
{
    TypeManager types;
    types.load();
    TypeManager::setInstance( &types );
    auto application = make_ptr<core::ApplicationManager>();
    core::IApplicationManager::setInstance( application );
    application->setFactoryManager( make_ptr<FactoryManager>() );
    application->setMeshManager( make_ptr<MeshManager>() );
    auto timer = make_ptr<TimerMT>();
    timer->load( nullptr );
    application->setTimer( timer );
    application->setStateManager( make_ptr<StateManager>() );
    int result = 0;
    try
    {
        terrain_data_contracts::run();
        persistenceContracts();
        attachmentContracts();
        terrain_editing_contracts::run();
    }
    catch( const std::exception &exception )
    {
        std::fprintf( stderr, "Terrain contracts failed: %s\n", exception.what() );
        result = 1;
    }
    application->setStateManager( nullptr );
    application->setMeshManager( nullptr );
    application->setTimer( nullptr );
    timer->unload( nullptr );
    timer = nullptr;
    application->setFactoryManager( nullptr );
    core::IApplicationManager::setInstance( nullptr );
    application = nullptr;
    TypeManager::setInstance( nullptr );
    types.unload();
    return result;
}
