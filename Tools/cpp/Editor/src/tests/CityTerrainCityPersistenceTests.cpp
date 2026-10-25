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
#include <Workphone/Scene/Components/CollisionBox.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <Workphone/Interface/Physics/IPhysicsBody3.hpp>
#include <Workphone/System/FSM.hpp>
#include <Workphone/System/FSMManager.hpp>

#if WP_EDITOR_TESTS
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <algorithm>
#include <array>
#include <vector>
#include <cstdint>

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

    void registrationRemovalOrdering( SmartPtr<scene::IGameManager> manager )
    {
        auto registry = make_ptr<scene::GameScene>(); registry->load( nullptr ); BOOST_REQUIRE( registry->isLoaded() );
        using Actor = SmartPtr<scene::IGameActor>;
        std::array<Actor, 5> actors;
        for( auto &actor : actors ) { actor = manager->createActor(); BOOST_REQUIRE( actor ); BOOST_REQUIRE( actor->getHandle() ); }
        std::sort( actors.begin(), actors.end() );
        const Actor removed = actors[1], added = actors[4];
        const auto removedId = removed->getHandle()->getInstanceId();
        constexpr auto phaseCount = static_cast<u32>( Thread::UpdateState::Count );
        constexpr auto taskCount = static_cast<u32>( TaskId::Count );
        using Lists = std::vector<Array<Actor>>;
        const auto snapshot = [&] {
            Lists lists; lists.reserve( phaseCount * taskCount );
            for( u32 phase = 0; phase < phaseCount; ++phase ) for( u32 task = 0; task < taskCount; ++task )
                lists.push_back( registry->getRegisteredObjects( static_cast<Thread::UpdateState>( phase ), static_cast<TaskId>( task ) ).snapshot() );
            return lists;
        };
        const auto checkLists = [&]( const Lists &expected ) {
            const auto actual = snapshot(); BOOST_REQUIRE_EQUAL( actual.size(), expected.size() );
            for( size_t list = 0; list < actual.size(); ++list )
            {
                BOOST_TEST_CONTEXT( "registry phase=" << list / taskCount << " task=" << list % taskCount )
                {
                    BOOST_REQUIRE_EQUAL( actual[list].size(), expected[list].size() );
                    for( size_t actor = 0; actor < actual[list].size(); ++actor ) BOOST_CHECK( actual[list][actor] == expected[list][actor] );
                }
            }
        };
        // The public setter permits an explicit schedule. Make every list
        // deliberately non-sorted so a hidden global re-sort is observable.
        const std::array<Actor, 4> schedule = { actors[3], actors[1], actors[0], actors[2] };
        for( u32 phase = 0; phase < phaseCount; ++phase ) for( u32 task = 0; task < taskCount; ++task )
        {
            ConcurrentArray<Actor> ordered;
            for( size_t actor = 0; actor < schedule.size(); ++actor ) ordered.push_back( schedule[( actor + phase + task ) % schedule.size()] );
            registry->setRegisteredObjects( static_cast<Thread::UpdateState>( phase ), static_cast<TaskId>( task ), ordered );
        }
        auto expected = snapshot();
        const auto registrationTask = TaskId::Application; const auto registrationPhase = Thread::UpdateState::Update;
        const auto registrationList = static_cast<size_t>( registrationPhase ) * taskCount + static_cast<size_t>( registrationTask );
        registry->registerUpdate( registrationTask, registrationPhase, added );
        expected[registrationList].push_back( added ); std::sort( expected[registrationList].begin(), expected[registrationList].end() );
        checkLists( expected );
        registry->registerUpdate( registrationTask, registrationPhase, added ); checkLists( expected ); // Duplicate admission is a no-op.

        const auto removalTask = TaskId::Physics; const auto removalPhase = Thread::UpdateState::PostUpdate;
        const auto removalList = static_cast<size_t>( removalPhase ) * taskCount + static_cast<size_t>( removalTask );
        registry->unregisterUpdate( removalTask, removalPhase, removed );
        auto &single = expected[removalList]; single.erase( std::remove( single.begin(), single.end(), removed ), single.end() );
        checkLists( expected );
        registry->unregisterUpdate( removalTask, removalPhase, removed ); checkLists( expected ); // Repeated removal cannot disturb another schedule.

        registry->unregisterAll( removed );
        for( auto &list : expected ) list.erase( std::remove( list.begin(), list.end(), removed ), list.end() );
        checkLists( expected );
        for( const auto &list : snapshot() ) for( const auto &actor : list )
            BOOST_CHECK( actor->getHandle()->getInstanceId() != removedId );
        registry->unregisterAll( removed ); checkLists( expected );
        registry->unload( nullptr );
        for( const auto &actor : actors ) manager->destroyActor( actor, true );
    }

    void queuedSceneActivation( SmartPtr<scene::IGameManager> manager )
    {
        auto app = core::IApplicationManager::instancePtr(); BOOST_REQUIRE( app ); BOOST_REQUIRE( app->isEditor() );
        BOOST_REQUIRE( Thread::getCurrentTask() == TaskId::Application );
        struct ModeRestore
        {
            u32 flags = Thread::getTaskFlags(); bool playing = core::IApplicationManager::instancePtr()->isPlaying();
            ~ModeRestore() { Thread::setTaskFlags( flags ); core::IApplicationManager::instancePtr()->setPlaying( playing ); }
        } restore;
        Thread::setTaskFlags( restore.flags | Thread::Application_Flag );
        const auto physics = app->getPhysicsManager(); const auto physicsScene = physics ? physics->getPhysicsScene() : nullptr;
        using Actor = SmartPtr<scene::IGameActor>;
        for( bool play : { true, false } )
        {
            auto registry = make_ptr<scene::GameScene>(), other = make_ptr<scene::GameScene>();
            registry->load( nullptr ); other->load( nullptr ); BOOST_REQUIRE( registry->isLoaded() && other->isLoaded() );
            std::array<Actor, 3> actors;
            struct ActorsCleanup
            {
                SmartPtr<scene::IGameManager> manager; SmartPtr<scene::GameScene> registry, other; std::array<Actor, 3> &actors;
                ~ActorsCleanup()
                {
                    for( auto &actor : actors ) if( actor ) manager->destroyActor( actor, true );
                    registry->unload( nullptr ); other->unload( nullptr );
                }
            } cleanup{ manager, registry, other, actors };
            app->setPlaying( play );
            registry->setState( play ? scene::IGameScene::State::Play : scene::IGameScene::State::Edit );
            for( auto &actor : actors )
            {
                actor = manager->createActor(); BOOST_REQUIRE( actor && actor->isLoaded() ); actor->setStatic( true );
                BOOST_REQUIRE( actor->addComponent<scene::CollisionBox>() ); BOOST_REQUIRE( actor->addComponent<scene::Rigidbody>() );
                registry->addActor( actor );
                // Enter through the real public load event, then drain the real
                // deferred queues with preUpdate; no test-only queue mutation.
                registry->handleEvent( EventType::Scene, IEvent::actorLoaded, {}, registry, actor, nullptr );
            }
            const auto removed = actors[0], active = actors[1], transferred = actors[2];
            const auto removedState = removed->getState(), transferredState = transferred->getState();
            auto removedBody = removed->getComponent<scene::Rigidbody>(), activeBody = active->getComponent<scene::Rigidbody>();
            auto transferredBody = transferred->getComponent<scene::Rigidbody>();
            BOOST_REQUIRE( removedBody && activeBody && transferredBody );
            BOOST_REQUIRE( !removedBody->getRigidStatic() && !activeBody->getRigidStatic() && !transferredBody->getRigidStatic() );
            registry->removeActor( removed ); registry->removeActor( transferred ); other->addActor( transferred );
            BOOST_REQUIRE( removed->isLoaded() && !removed->getFlag( scene::IGameActor::ActorFlagInScene ) );
            BOOST_REQUIRE( transferred->getFlag( scene::IGameActor::ActorFlagInScene ) && transferred->getScene() == other );
            registry->preUpdate();
            const auto actorState = play ? scene::IGameActor::State::Play : scene::IGameActor::State::Edit;
            const auto componentState = play ? scene::IComponent::State::Play : scene::IComponent::State::Edit;
            BOOST_CHECK( removed->getState() == removedState && !removedBody->getRigidStatic() );
            BOOST_CHECK( transferred->getState() == transferredState && !transferredBody->getRigidStatic() );
            BOOST_REQUIRE( active->getState() == actorState && activeBody->getState() == componentState );
            const auto body = activeBody->getRigidStatic();
            if( physicsScene ) { BOOST_REQUIRE( body ); BOOST_CHECK( body->getScene() == physicsScene ); BOOST_CHECK( body->getNumShapes() > 0 ); }
            const auto bodyCount = physicsScene ? physicsScene->getActors().size() : 0;
            registry->handleEvent( EventType::Scene, IEvent::actorLoaded, {}, registry, active, nullptr ); registry->preUpdate();
            BOOST_CHECK( active->getState() == actorState && activeBody->getState() == componentState && activeBody->getRigidStatic() == body );
            if( physicsScene ) BOOST_CHECK_EQUAL( physicsScene->getActors().size(), bodyCount );
            BOOST_CHECK( removed->getState() == removedState && transferred->getState() == transferredState );
        }
    }

    class CityPersistenceFSMManager : public FSMManager
    {
    public:
        void growStorage( size_t size ) { resize( size ); }
    };

    void fsmFlagsSurviveStorageGrowth( SmartPtr<IFSMManager> applicationManager )
    {
        BOOST_REQUIRE( applicationManager );
        // Use the real parallel-array implementation, isolated from the Editor's
        // own FSMs. One explicit capacity growth avoids consuming thousands of IDs.
        auto manager = make_ptr<CityPersistenceFSMManager>();
        manager->setGrowSize( applicationManager->getNumFsms() + 64 ); manager->load( nullptr );
        BOOST_REQUIRE( manager->isLoaded() ); auto first = manager->createFSM(); BOOST_REQUIRE( first );
        auto concrete = dynamic_pointer_cast<FSM>( first ); BOOST_REQUIRE( concrete ); BOOST_REQUIRE( first->getHandle() );
        const auto id = first->getHandle()->getInstanceId();
        const u32 beforeFlags = IFSM::autoChangeStateFlag | IFSM::isPendingFlag | IFSM::isLockedFlag;
        manager->setFlags( id, beforeFlags ); BOOST_REQUIRE_EQUAL( first->getFlags(), beforeFlags );
        const auto oldAddress = reinterpret_cast<std::uintptr_t>( manager->getFlagsPtr( id ) );
        manager->growStorage( manager->getSize() * 2 + 17 );
        BOOST_REQUIRE( reinterpret_cast<std::uintptr_t>( manager->getFlagsPtr( id ) ) != oldAddress );
        const u32 liveFlags = 0x5a000000u | IFSM::allowStateChangeFlag | IFSM::isStateChangeCompleteFlag | IFSM::isReadyFlag;
        manager->setFlags( id, liveFlags );
        // This is a value-level regression: the stale implementation reads the
        // old allocation instead of this deliberately different live-slot pattern.
        BOOST_REQUIRE_EQUAL( first->getFlags(), liveFlags );
        BOOST_CHECK( !first->getAutoChangeState() ); BOOST_CHECK( !first->isPending() );
        BOOST_CHECK( concrete->getAllowStateChange() ); BOOST_CHECK( concrete->isStateChangeComplete() );
        first->setAutoChangeState( true ); BOOST_CHECK_EQUAL( manager->getFlags( id ), liveFlags | IFSM::autoChangeStateFlag );
        first->setAutoChangeState( false ); BOOST_CHECK_EQUAL( manager->getFlags( id ), liveFlags );
        concrete->setAllowStateChange( false ); BOOST_CHECK_EQUAL( manager->getFlags( id ), liveFlags & ~IFSM::allowStateChangeFlag );
        concrete->setAllowStateChange( true ); BOOST_CHECK_EQUAL( manager->getFlags( id ), liveFlags );
        concrete->setStateChangeComplete( false ); BOOST_CHECK_EQUAL( manager->getFlags( id ), liveFlags & ~IFSM::isStateChangeCompleteFlag );
        concrete->setStateChangeComplete( true ); BOOST_CHECK_EQUAL( manager->getFlags( id ), liveFlags );
        first->setFlags( liveFlags | IFSM::isPendingFlag );
        BOOST_CHECK( first->isPending() ); BOOST_CHECK_EQUAL( manager->getFlags( id ), liveFlags | IFSM::isPendingFlag );

        first->setNewState( 17, false ); // Leave a real deferred queue reference alive through destruction.
        manager->destroyFSM( first ); BOOST_CHECK( !first->isLoaded() );
        const u32 retiredSlot = 0xa6000000u | IFSM::isReadyFlag;
        manager->setFlags( id, retiredSlot );
        first->setFlags( 0 ); first->setAutoChangeState( true ); concrete->setAllowStateChange( true ); concrete->setStateChangeComplete( true );
        first->update(); first->updateState(); manager->update();
        BOOST_CHECK_EQUAL( first->getFlags(), 0u ); BOOST_CHECK( !first->getAutoChangeState() ); BOOST_CHECK( !first->isPending() );
        BOOST_CHECK( !concrete->getAllowStateChange() ); BOOST_CHECK( !concrete->isStateChangeComplete() );
        BOOST_CHECK_EQUAL( manager->getFlags( id ), retiredSlot );
        manager->unload( nullptr );
        BOOST_CHECK_EQUAL( manager->getFlags( id ), 0u );
        manager->setFlags( id, 0xffffffffu );
        BOOST_CHECK_EQUAL( manager->getFlags( id ), 0u );
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
    registrationRemovalOrdering( manager );
    queuedSceneActivation( manager );
    fsmFlagsSurviveStorageGrowth( app->getFsmManager() );
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
