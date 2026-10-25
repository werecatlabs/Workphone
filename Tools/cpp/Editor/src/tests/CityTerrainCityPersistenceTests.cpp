#include <EditorPCH.hpp>
#include <EditorApplication.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Core/Handle.hpp>
#include <Workphone/Scene/GameScene.hpp>
#include <Workphone/Scene/Components/CityRoadAuthoring.hpp>
#include <Workphone/Scene/Components/ProceduralRaceScene.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>

#if WP_EDITOR_TESTS
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>

using namespace workphone;
using namespace workphone::editor;
namespace
{
    struct CityPersistenceDirectory
    {
        std::filesystem::path path = std::filesystem::temp_directory_path() /
            std::string( ( "workphone-terrain-city-persistence-" + StringUtil::getUUID() ).c_str() );
        ~CityPersistenceDirectory() { std::error_code ignored; std::filesystem::remove_all( path, ignored ); }
    };
    struct CityPersistenceEditor
    {
        EditorApplication app;
        ~CityPersistenceEditor()
        {
            if( auto state = core::IApplicationManager::instancePtr() ) { state->setQuit( true ); state->setRunning( false ); }
            app.unload( nullptr );
        }
    };
    struct CityPersistenceTask
    {
        TaskId previous = Thread::getCurrentTask();
        CityPersistenceTask() { Thread::setCurrentTask( TaskId::Application ); }
        ~CityPersistenceTask() { Thread::setCurrentTask( previous ); }
    };
    struct CityPersistenceTaskOwner
    {
        SmartPtr<ITask> task; SmartPtr<ISharedObject> previous;
        explicit CityPersistenceTaskOwner( SmartPtr<ITask> value ) : task( value ), previous( value->getOwner() ) { task->setOwner( nullptr ); }
        ~CityPersistenceTaskOwner() { task->setOwner( previous ); }
    };
    class CityPersistenceBarrier : public Job
    {
    public:
        bool *complete = nullptr;
        void execute() override { *complete = true; }
    };
    String canonicalSource( const procedural::CityRoadSplineSource &source )
    {
        SourceBudgetText json; String error;
        BOOST_REQUIRE_MESSAGE( procedural::serializeCityRoadSplineSource( source, json, error ), error );
        return String( json.data(), json.size() );
    }
}

BOOST_AUTO_TEST_CASE( terrain_city_source_anchor_roundtrip_test )
{
    CityPersistenceDirectory temporary;
    Project project; project.create( temporary.path.generic_u8string() ); project.setMediaPaths( {} ); project.save();
    CityPersistenceEditor guard; guard.app.setDebugMode( true ); guard.app.setActiveThreads( 0 ); guard.app.load( nullptr );
    BOOST_REQUIRE( guard.app.isLoaded() );
    auto app = core::IApplicationManager::instancePtr(); auto editor = EditorManager::getSingletonPtr();
    BOOST_REQUIRE( app ); BOOST_REQUIRE( editor ); editor->loadProject( project.getFilePath() );
    CityPersistenceTask application;
    auto manager = app->getGameManager(); BOOST_REQUIRE( manager );
    auto originalScene = manager->getCurrentScene(); BOOST_REQUIRE( originalScene );
    auto task = app->getTaskManager()->getTask( TaskId::Application ); BOOST_REQUIRE( task );
    BOOST_REQUIRE( task->isLoaded() ); BOOST_REQUIRE( !task->isStopped() ); CityPersistenceTaskOwner owner( task );
    const auto flush = [&] {
        bool complete = false; auto barrier = make_ptr<CityPersistenceBarrier>(); barrier->complete = &complete; task->addJob( barrier );
        for( unsigned pass = 0; pass < 8 && !complete; ++pass )
        { task->setState( ITask::State::Idle ); task->setNextUpdateTime( -1 ); task->update(); }
        BOOST_REQUIRE_MESSAGE( complete, "Application queue did not execute the scene persistence barrier" );
        auto queue = app->getJobQueue();
        for( unsigned pass = 0; pass < 100 && queue->hasJobs(); ++pass ) queue->update();
        BOOST_REQUIRE_MESSAGE( !queue->hasJobs(), "Scene persistence callbacks did not drain" );
    };
    flush();
    auto terrainActor = manager->createActor(); BOOST_REQUIRE( terrainActor ); terrainActor->setName( "Terrain city persisted terrain" );
    auto terrain = terrainActor->addComponent<scene::TerrainSystem>(); BOOST_REQUIRE( terrain ); originalScene->addActor( terrainActor );
    if( auto renderer = dynamic_pointer_cast<render::Terrain>( terrain->getTerrain() ) ) renderer->setDeferredTerrainPublication( false );
    render::TerrainData data; data.dimensions = { 33, 33 }; data.spacing = { 8, 8 }; data.origin = { -128, -128 };
    data.heights.resize( 33 * 33 );
    for( int z = 0; z < 33; ++z ) for( int x = 0; x < 33; ++x ) data.heights[size_t( z ) * 33 + x] = 2 + ( -128 + x * 8 ) * .01f;
    String error; BOOST_REQUIRE_MESSAGE( terrain->applyTerrainData( data, error ), error );
    auto sourceActor = manager->createActor(); BOOST_REQUIRE( sourceActor ); sourceActor->setName( "Terrain city persisted source" );
    auto source = sourceActor->addComponent<scene::CityRoadAuthoring>(); BOOST_REQUIRE( source ); originalScene->addActor( sourceActor );
    unsigned publications = 0;
    const auto publisher = [&]( const auto &, const auto &, auto, const auto &, String &diagnostic ) {
        BOOST_CHECK( Thread::getCurrentTask() == TaskId::Application ); ++publications; diagnostic.clear(); return true;
    };
    source->setPublishCallback( publisher ); BOOST_REQUIRE_MESSAGE( source->bindTerrain( terrain, error ), error );
    auto authored = source->getSource();
    procedural::CityRoadSpline road; road.id = "30000000-0000-4000-8000-000000000001";
    road.width = 10.25f; road.heightOffset = .025f; road.maxGrade = .08f;
    road.knots = {
        { "40000000-0000-4000-8000-000000000001", { -30, 0, -16 }, {}, { 18.25f, 0, -3.5f } },
        { "40000000-0000-4000-8000-000000000002", { 32.5f, 0, -21.25f }, { -17.75f, 0, 5.5f }, {} } };
    authored.roads.push_back( road );
    BOOST_REQUIRE_MESSAGE( source->setSourceJSON( canonicalSource( authored ), source->getSourceRevision(), error ), error );
    // Preserve a subsequent manual control edit, rather than only an untouched generated curve.
    authored = source->getSource(); authored.roads[0].knots[0].outHandle.x = 20.5f;
    authored.roads[0].knots[1].position.z = -24.75f;
    BOOST_REQUIRE_MESSAGE( source->setSourceJSON( canonicalSource( authored ), source->getSourceRevision(), error ), error );
    const auto expected = source->getSource();
    BOOST_REQUIRE( source->getHandle() ); BOOST_REQUIRE( terrain->getHandle() );
    const String sourceUuid = source->getHandle()->getUUIDAsString(), terrainUuid = terrain->getHandle()->getUUIDAsString();
    BOOST_REQUIRE( !sourceUuid.empty() ); BOOST_REQUIRE( !terrainUuid.empty() ); BOOST_REQUIRE( sourceUuid != terrainUuid );

    auto vehicleActor = manager->createActor(); BOOST_REQUIRE( vehicleActor ); vehicleActor->setName( "Terrain city persisted vehicle" );
    auto vehicle = vehicleActor->addComponent<scene::ProceduralRaceScene>(); BOOST_REQUIRE( vehicle ); originalScene->addActor( vehicleActor );
    auto properties = vehicle->getProperties(); properties->setProperty( "City Terrain Roads", true );
    properties->setProperty( "City Road Authoring UUID", sourceUuid ); properties->setProperty( "Audio Enabled", false );
    properties->setProperty( "Effects Enabled", false ); vehicle->setProperties( properties );
    vehicle->setEnabled( false ); // This gate saves/restores anchors; it does not regenerate a WPVehicle.
    flush();
    const auto path = temporary.path / "Assets" / "TerrainCityAnchors.scene";
    BOOST_REQUIRE_MESSAGE(source->getSource().roads.size()==1u,"Manual source was lost before scene save: "+source->getError());
    originalScene->saveScene( path.generic_u8string() ); BOOST_REQUIRE( std::filesystem::is_regular_file( path ) );
    std::ifstream saved( path, std::ios::binary );
    const std::string bytes( ( std::istreambuf_iterator<char>( saved ) ), std::istreambuf_iterator<char>() );
    BOOST_REQUIRE( !bytes.empty() ); BOOST_CHECK( bytes.find( sourceUuid.c_str() ) != std::string::npos );
    BOOST_REQUIRE_MESSAGE(bytes.find("30000000-0000-4000-8000-000000000001")!=std::string::npos,"Scene serialization omitted the authored road");
    BOOST_CHECK( bytes.find( terrainUuid.c_str() ) != std::string::npos );

    if( auto selection = app->getSelectionManager() ) selection->clearSelection();
    if( auto history = app->getCommandManager() ) history->clearAll();
    originalScene->clear( true );
    BOOST_CHECK( originalScene->getComponents<scene::CityRoadAuthoring>().empty() );
    BOOST_CHECK( !source->getActor() ); BOOST_CHECK( !terrain->getActor() );
    auto reopenedScene = app->getFactoryManager()->make_ptr<scene::GameScene>(); BOOST_REQUIRE( reopenedScene );
    reopenedScene->load( nullptr ); manager->setCurrentScene( reopenedScene );
    BOOST_REQUIRE( manager->getCurrentScene() != originalScene );
    // Production SceneLoadJob/GameActorUtil reconstruction, not a component-properties clone.
    reopenedScene->loadScene( path.generic_u8string(), false );
    BOOST_REQUIRE_MESSAGE( reopenedScene->getSceneLoadingState() == scene::IGameScene::SceneLoadingState::Loaded,
        "Actual saved terrain-city scene failed to reopen" );
    auto restoredSourceActor = reopenedScene->findActorByName( "Terrain city persisted source" );
    auto restoredTerrainActor = reopenedScene->findActorByName( "Terrain city persisted terrain" );
    auto restoredVehicleActor = reopenedScene->findActorByName( "Terrain city persisted vehicle" );
    BOOST_REQUIRE( restoredSourceActor ); BOOST_REQUIRE( restoredTerrainActor ); BOOST_REQUIRE( restoredVehicleActor );
    auto restoredSource = restoredSourceActor->getComponent<scene::CityRoadAuthoring>();
    auto restoredTerrain = restoredTerrainActor->getComponent<scene::TerrainSystem>();
    auto restoredVehicle = restoredVehicleActor->getComponent<scene::ProceduralRaceScene>();
    BOOST_REQUIRE( restoredSource ); BOOST_REQUIRE( restoredTerrain ); BOOST_REQUIRE( restoredVehicle );
    BOOST_CHECK( restoredSource != source ); BOOST_CHECK( restoredTerrain != terrain ); BOOST_CHECK( restoredVehicle != vehicle );
    restoredSource->setPublishCallback( publisher );
    if( auto renderer = dynamic_pointer_cast<render::Terrain>( restoredTerrain->getTerrain() ) ) renderer->setDeferredTerrainPublication( false );
    flush(); restoredSource->update();
    BOOST_REQUIRE_MESSAGE(restoredSource->getSource().roads.size()==1u,"Source lost during actual scene restore: "+restoredSource->getError()+"; JSON="+restoredSource->getSourceJSON());
    BOOST_REQUIRE_MESSAGE( restoredSource->rebuild( error ), error );
    BOOST_REQUIRE_EQUAL( restoredSource->getSource().roads.size(), 1u );
    const auto &actual = restoredSource->getSource();
    const auto &actualRoad = actual.roads[0], &expectedRoad = expected.roads[0];
    BOOST_CHECK( actual.id == expected.id ); BOOST_CHECK( actualRoad.id == expectedRoad.id );
    BOOST_CHECK_EQUAL( actual.version, 2u );
    BOOST_CHECK_EQUAL( actualRoad.width, expectedRoad.width ); BOOST_CHECK_EQUAL( actualRoad.heightOffset, expectedRoad.heightOffset );
    BOOST_REQUIRE_EQUAL( actualRoad.knots.size(), expectedRoad.knots.size() );
    for( size_t knot = 0; knot < expectedRoad.knots.size(); ++knot )
    {
        BOOST_CHECK( actualRoad.knots[knot].id == expectedRoad.knots[knot].id );
        BOOST_CHECK( actualRoad.knots[knot].position == expectedRoad.knots[knot].position );
        BOOST_CHECK( actualRoad.knots[knot].inHandle == expectedRoad.knots[knot].inHandle );
        BOOST_CHECK( actualRoad.knots[knot].outHandle == expectedRoad.knots[knot].outHandle );
    }
    BOOST_CHECK_EQUAL( restoredSource->getHandle()->getUUIDAsString(), sourceUuid );
    BOOST_CHECK_EQUAL( restoredTerrain->getHandle()->getUUIDAsString(), terrainUuid );
    BOOST_CHECK_EQUAL( restoredSource->getTerrainBindingUUID(), restoredTerrain->getHandle()->getUUIDAsString() );
    BOOST_CHECK_EQUAL( String( actual.terrainResourceUuid.data(), actual.terrainResourceUuid.size() ), terrainUuid );
    BOOST_CHECK( restoredSource->getTerrain() == restoredTerrain );
    BOOST_CHECK( restoredSource->getActor()->getScene() == manager->getCurrentScene() );
    BOOST_CHECK( restoredTerrain->getActor()->getScene() == manager->getCurrentScene() );
    BOOST_REQUIRE( restoredTerrain->getTerrainSnapshot() );
    BOOST_CHECK( restoredTerrain->getTerrainSnapshot()->heights == data.heights );
    bool terrainRoads = false; String vehicleSourceUuid;
    BOOST_REQUIRE( restoredVehicle->getProperties()->getPropertyValue( "City Terrain Roads", terrainRoads ) );
    BOOST_REQUIRE( restoredVehicle->getProperties()->getPropertyValue( "City Road Authoring UUID", vehicleSourceUuid ) );
    BOOST_CHECK( terrainRoads ); BOOST_CHECK_EQUAL( vehicleSourceUuid, restoredSource->getHandle()->getUUIDAsString() );
    BOOST_CHECK( !restoredVehicle->isGenerated() );

    // Assert these identities belong to the current live manager/scene registry,
    // with exactly one terrain/source/vehicle anchor and no constructor-ID clone.
    std::set<String> liveIds; unsigned liveSources = 0, liveTerrains = 0, liveVehicles = 0;
    for( auto actor : manager->getActors() ) if( actor && actor->getScene() == reopenedScene )
    {
        BOOST_REQUIRE( actor->getHandle() ); BOOST_CHECK( liveIds.insert( actor->getHandle()->getUUIDAsString() ).second );
        for( auto component : actor->getComponents() )
        { BOOST_REQUIRE( component->getHandle() ); BOOST_CHECK( liveIds.insert( component->getHandle()->getUUIDAsString() ).second ); }
        if( actor->getComponent<scene::CityRoadAuthoring>() ) ++liveSources;
        if( actor->getComponent<scene::TerrainSystem>() ) ++liveTerrains;
        if( actor->getComponent<scene::ProceduralRaceScene>() ) ++liveVehicles;
    }
    BOOST_CHECK_EQUAL( liveSources, 1u ); BOOST_CHECK_EQUAL( liveTerrains, 1u ); BOOST_CHECK_EQUAL( liveVehicles, 1u );
    BOOST_CHECK( liveIds.count( sourceUuid ) == 1 ); BOOST_CHECK( liveIds.count( terrainUuid ) == 1 );
    BOOST_CHECK( publications > 0 );
    reopenedScene->clear( true );
}
#endif
