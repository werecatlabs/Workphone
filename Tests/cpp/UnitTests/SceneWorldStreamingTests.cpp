#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/SceneWorld.hpp>
#include <Workphone/Scene/SceneWorldStreamer.hpp>
#include <Workphone/Scene/Transform.hpp>
#include <Workphone/Scene/Components/WorldStreaming.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <Workphone/Graphics/GraphicsMesh.hpp>
#include <Workphone/Graphics/GraphicsSceneNode.hpp>
#include <Workphone/Database/ResourceDatabase.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <chrono>
#include <thread>
#include <fstream>
#include <future>
#include <condition_variable>
#include <mutex>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    class TransformPublicationProbe : public Component
    {
    public:
        void updateTransform() override { ++publications; if( callback ) callback(); }
        u32 publications = 0;
        std::function<void()> callback;
    };
    class StreamPoseNode : public render::GraphicsSceneNode
    {
    public:
        void setTransform( const Transform3<real_Num> &value ) override { pose = value; ++publications; }
        Transform3<real_Num> getTransform() const override { return pose; }
        Transform3<real_Num> pose;
        u32 publications = 0;
    };
    class MeshRetirementProbe : public workphone::Mesh
    {
    public:
        void unload( SmartPtr<ISharedObject> data ) override
        { ++unloads; ++*unloadCounter; if( onUnload ) onUnload(); workphone::Mesh::unload( data ); }
        u32 unloads = 0;
        std::shared_ptr<u32> unloadCounter = std::make_shared<u32>( 0 );
        std::function<void()> onUnload;
    };
    class TransformLifetimeProbe : public Transform
    {
    public:
        ~TransformLifetimeProbe() override { if( onDestroy ) onDestroy(); }
        // Exercise final actor ownership without adding a dirty-queue owner.
        void setDirty( bool, bool ) override {}
        std::function<void()> onDestroy;
    };
    class SnapshotCatalog : public AssetDatabaseManager
    {
    public:
        void loadFromFile( const String & ) override { ++loads; }
        std::atomic<u32> loads{ 0 };
    };
    class RefreshProbe : public ResourceDatabase
    {
    public:
        RefreshProbe() { setLoadingState( LoadingState::Loaded ); }
        void refresh() override { ++calls; if( onRefresh ) onRefresh(); }
        void unload( SmartPtr<ISharedObject> ) override { setLoadingState( LoadingState::Unloaded ); }
        u32 calls = 0;
        std::function<void()> onRefresh;
    };
    class DeferredRendererProbe : public Renderer
    {
    public:
        void unload( SmartPtr<ISharedObject> data ) override
        {
            setGraphicsObject( nullptr ); Component::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }
    };
    class DeferredGraphicsProbe : public render::GraphicsMesh
    {
    public:
        SmartPtr<render::IGraphicsObject> clone( const String & ) const override { return nullptr; }
    };
    class StatisticsReader
    {
    public:
        explicit StatisticsReader( SceneWorldStreamer &stream ) : worker( [&] {
            while( running )
            {
                const auto stats = stream.statistics();
                if( stats.residentActors != stats.residentCells || stats.residentActors > 2 ) valid = false;
                std::this_thread::yield();
            }
        } ) {}
        ~StatisticsReader() { stop(); }
        void stop() { running = false; if( worker.joinable() ) worker.join(); }
        std::atomic<bool> valid{ true };
    private:
        std::atomic<bool> running{ true };
        std::thread worker;
    };
    class PhaseBarrier
    {
    public:
        bool wait()
        {
            std::unique_lock<std::mutex> lock( mutex );
            if( broken ) return false;
            const auto phase = generation;
            if( ++arrivals == 5 )
            { arrivals = 0; ++generation; changed.notify_all(); return true; }
            if( !changed.wait_for( lock, std::chrono::seconds( 5 ), [&] { return broken || generation != phase; } ) )
            { broken = true; changed.notify_all(); }
            return !broken;
        }
    private:
        std::mutex mutex;
        std::condition_variable changed;
        u32 arrivals = 0, generation = 0;
        bool broken = false;
    };
    SmartPtr<Properties> pointActor( const String &label, float x )
    {
        auto actor = make_ptr<Properties>();
        actor->setName( ApplicationUtil::actorsStr );
        actor->setProperty( "label", label ); actor->setProperty( "uuid", StringUtil::getUUID() );
        actor->setProperty( "static", true );
        auto transform = make_ptr<Properties>(); transform->setName( "localTransform" );
        transform->setProperty( "Position", Vector3F( x, 0, 0 ) ); actor->addChild( transform );
        auto mesh = make_ptr<Properties>(); mesh->setName( "component" );
        mesh->setProperty( "componentType", "workphone::scene::Mesh" );
        mesh->setProperty( "uuid", StringUtil::getUUID() ); actor->addChild( mesh );
        return actor;
    }
    String testDirectory( TestGuard &guard )
    {
        auto path = ( std::filesystem::temp_directory_path() /
                      ( "wp-city-test-" + std::string( StringUtil::getUUID().c_str() ) ) ).generic_string();
        guard.trackFilesystemPath( path.c_str() );
        return path.c_str();
    }
    void enablePrimaryJobs( TestGuard &guard )
    {
        auto app = guard.applicationManager;
        const auto running = app->isRunning(), quit = app->getQuit();
        guard.addCleanup( [app, running, quit]() mutable { app->setRunning( running ); app->setQuit( quit ); } );
        app->setRunning( true ); app->setQuit( false );
    }
    void pump( SceneWorldStreamer &stream, const Vector3F &observer, u32 expectedActors )
    {
        const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );
        String error;
        do
        {
            BOOST_REQUIRE_MESSAGE( stream.update( { observer }, error ), error );
            if( stream.statistics().residentActors == expectedActors && !stream.statistics().pendingCells ) return;
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        } while( std::chrono::steady_clock::now() < until );
        BOOST_FAIL( "Cell streaming did not converge" );
    }
}

BOOST_AUTO_TEST_SUITE( SceneWorldStreamingTests )

BOOST_AUTO_TEST_CASE( retirement_releases_cpu_geometry_only_after_all_owners_leave )
{
    TestGuard guard;
    auto manager = make_ptr<MeshManager>();
    auto resource = dynamic_pointer_cast<IMeshResource>( manager->createOrRetrieve(
        StringUtil::getUUID(), "Cache/stream-retirement.fbmeshbin", "Mesh" ).first );
    BOOST_REQUIRE( resource );
    const auto uuid = resource->getHandle()->getUUIDAsString();
    auto geometry = make_ptr<MeshRetirementProbe>();
    const auto unloads = geometry->unloadCounter;
    resource->setMesh( geometry ); resource->setLoadingState( LoadingState::Loaded );
    manager->addMesh( geometry );

    auto gameplayOwner = resource;
    BOOST_CHECK( !manager->releaseUnusedResource( resource ) );
    gameplayOwner = nullptr;
    BOOST_CHECK( !manager->releaseUnusedResource( resource ) ); // Direct mesh owner.
    BOOST_CHECK_EQUAL( geometry->unloads, 0u );

    auto shared = dynamic_pointer_cast<IMeshResource>( manager->createOrRetrieve(
        StringUtil::getUUID(), "Cache/shared-retirement.fbmeshbin", "Mesh" ).first );
    BOOST_REQUIRE( shared );
    shared->setMesh( geometry );
    WeakPtr<MeshRetirementProbe> weak = geometry;
    geometry = nullptr;
    BOOST_CHECK( !manager->releaseUnusedResource( resource ) ); // Shared geometry.
    shared->setMesh( nullptr ); shared = nullptr;
    WeakPtr<IMeshResource> weakResource = resource;
    BOOST_CHECK( !manager->releaseUnusedResource( resource ) );
    weakResource = nullptr;
    BOOST_CHECK( !manager->releaseUnusedResource( resource ) ); // Weak geometry observer.
    weak = nullptr;
    BOOST_REQUIRE( manager->releaseUnusedResource( resource ) );
    BOOST_CHECK( resource->getLoadingState() == LoadingState::Unloaded );
    BOOST_CHECK( !resource->getMesh() );
    BOOST_CHECK_EQUAL( *unloads, 1u );
    BOOST_CHECK( !manager->findMesh( "" ) );
    BOOST_CHECK_EQUAL( resource->getHandle()->getUUIDAsString(), uuid );
    BOOST_CHECK_EQUAL( resource->getFilePath(), "Cache/stream-retirement.fbmeshbin" );
    BOOST_CHECK( manager->getById( uuid ) == resource );
    BOOST_CHECK( manager->releaseUnusedResource( resource ) );
    BOOST_CHECK_EQUAL( *unloads, 1u );

    // Runtime-generated content has no reload source and must never be discarded.
    resource->setFilePath( "__procedural/retained" );
    resource->setLoadingState( LoadingState::Loaded );
    BOOST_CHECK( !manager->releaseUnusedResource( resource ) );
    manager->unload( nullptr );
}

BOOST_AUTO_TEST_CASE( cache_acquisition_cannot_observe_loaded_data_during_retirement )
{
    TestGuard guard; auto manager = make_ptr<MeshManager>();
    const auto uuid = StringUtil::getUUID();
    auto resource = dynamic_pointer_cast<IMeshResource>( manager->createOrRetrieve(
        uuid, "Cache/concurrent-retirement.fbmeshbin", "Mesh" ).first );
    BOOST_REQUIRE( resource );
    std::promise<void> entered, attempting, finished;
    auto entry = entered.get_future(); auto attempt = attempting.get_future(); auto finish = finished.get_future();
    std::atomic<bool> sawLoaded{ true }; bool blocked = false;
    auto geometry = make_ptr<MeshRetirementProbe>();
    geometry->onUnload = [&] {
        entered.set_value(); attempt.wait();
        blocked = finish.wait_for( std::chrono::milliseconds( 100 ) ) == std::future_status::timeout;
    };
    resource->setMesh( geometry ); geometry = nullptr; resource->setLoadingState( LoadingState::Loaded );
    auto worker = std::async( std::launch::async, [&] {
        entry.wait(); attempting.set_value();
        auto acquired = manager->getById( uuid );
        sawLoaded = acquired && acquired->isLoaded(); finished.set_value();
    } );
    const bool released = manager->releaseUnusedResource( resource );
    if( !released ) entered.set_value();
    worker.get();
    BOOST_CHECK( released ); BOOST_CHECK( blocked ); BOOST_CHECK( !sawLoaded.load() );
    manager->unload( nullptr );
}

BOOST_AUTO_TEST_CASE( large_index_queries_are_local_and_respect_small_shared_queues )
{
    TestGuard guard; guard.scene->clear( true );
    auto data = make_ptr<Properties>();
    for( int i = 0; i < 512; ++i )
        data->addChild( pointActor( "Indexed " + StringUtil::toString( i ), float( i * 1000 ) ) );
    WorldCookOptions cook; cook.cellSize = 64; cook.maximumActorsPerCell = 1;
    String path, error;
    BOOST_REQUIRE_MESSAGE( SceneWorld::cook( *data, testDirectory( guard ), cook, path, error ), error );
    WorldStreamLimits limits; limits.workers = 1; limits.maximumPending = 1; limits.maximumReady = 1;
    auto scheduler = std::make_shared<WorldStreamingScheduler>( limits );
    SceneWorldStreamOptions options;
    options.loadRadius = 1; options.retainRadius = 2; options.minimumResidencySeconds = 0;
    options.maximumResidentActors = 2; options.maximumResidentCells = 2;
    SceneWorldStreamer stream( scheduler );
    BOOST_REQUIRE_MESSAGE( stream.open( path, guard.scene, options, error ), error );
    StatisticsReader reader( stream );
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );
    do
    {
        BOOST_REQUIRE_MESSAGE( stream.update( { Vector3F::zero(), Vector3F( 511000, 0, 0 ) }, error ), error );
        const auto stats = stream.statistics();
        BOOST_CHECK_LT( stats.indexCellsTested, 32u );
        BOOST_CHECK_LE( scheduler->stats().pending, 1u );
        BOOST_CHECK_LE( scheduler->stats().ready, 1u );
        if( stats.residentActors == 2 && !stats.pendingCells ) break;
        std::this_thread::yield();
    } while( std::chrono::steady_clock::now() < until );
    BOOST_CHECK_EQUAL( stream.statistics().residentActors, 2u );
    BOOST_CHECK( guard.scene->findActorByName( "Indexed 0" ) );
    BOOST_CHECK( guard.scene->findActorByName( "Indexed 511" ) );
    pump( stream, Vector3F( 128000, 0, 0 ), 1 );
    BOOST_CHECK( guard.scene->findActorByName( "Indexed 128" ) );
    BOOST_CHECK( !guard.scene->findActorByName( "Indexed 0" ) );
    stream.close();
    reader.stop(); BOOST_CHECK( reader.valid.load() );
    for( auto used : scheduler->stats().used ) BOOST_CHECK_EQUAL( used, 0u );
}

BOOST_AUTO_TEST_CASE( cell_retirement_retries_external_mesh_pins_without_retaining_source_payloads )
{
    TestGuard guard; guard.scene->clear( true );
    auto previous = guard.applicationManager->getMeshManager();
    auto manager = make_ptr<MeshManager>(); guard.applicationManager->setMeshManager( manager );
    guard.addCleanup( [app = guard.applicationManager, previous]() mutable { app->setMeshManager( previous ); } );
    auto data = make_ptr<Properties>();
    data->addChild( pointActor( "Resource owner", 0 ) );
    data->addChild( pointActor( "Next cell", 1000 ) );
    WorldCookOptions cook; cook.cellSize = 64; cook.maximumActorsPerCell = 1;
    String path, error;
    BOOST_REQUIRE( SceneWorld::cook( *data, testDirectory( guard ), cook, path, error ) );
    SceneWorldStreamOptions options;
    options.loadRadius = 1; options.retainRadius = 2; options.minimumResidencySeconds = 0;
    options.maximumResidentActors = 1; options.maximumResidentCells = 1;
    SceneWorldStreamer stream;
    BOOST_REQUIRE_MESSAGE( stream.open( path, guard.scene, options, error ), error );
    pump( stream, Vector3F::zero(), 1 );
    auto resource = dynamic_pointer_cast<IMeshResource>( manager->createOrRetrieve(
        StringUtil::getUUID(), "Cache/retirement-pin.fbmeshbin", "Mesh" ).first );
    BOOST_REQUIRE( resource );
    auto geometry = make_ptr<MeshRetirementProbe>();
    const auto unloads = geometry->unloadCounter;
    resource->setMesh( geometry ); geometry = nullptr;
    resource->setLoadingState( LoadingState::Loaded );
    guard.scene->findActorByName( "Resource owner" )->getComponent<scene::Mesh>()->setMeshResource( resource );
    pump( stream, Vector3F( 1000, 0, 0 ), 1 );
    BOOST_REQUIRE_MESSAGE( stream.update( { Vector3F( 1000, 0, 0 ) }, error ), error );
    BOOST_CHECK( resource->isLoaded() );
    BOOST_CHECK_EQUAL( stream.statistics().releasedMeshResources, 0u );
    BOOST_CHECK_EQUAL( stream.statistics().retiringObjects, 0u );
    resource = nullptr;
    BOOST_REQUIRE_MESSAGE( stream.update( { Vector3F( 1000, 0, 0 ) }, error ), error );
    BOOST_CHECK_EQUAL( stream.statistics().releasedMeshResources, 1u );
    BOOST_CHECK_EQUAL( *unloads, 1u );
    stream.close(); manager->unload( nullptr );
}

BOOST_AUTO_TEST_CASE( coverage_queries_deduplicate_observers_and_keep_distant_pins )
{
    TestGuard guard; guard.scene->clear( true );
    auto data = make_ptr<Properties>();
    auto spatial = pointActor( "Wide coverage", 0 ); data->addChild( spatial );
    auto persistent = pointActor( "Persistent", 1000000 );
    persistent->setProperty( "smoothMotion", true ); data->addChild( persistent );
    String path, error;
    BOOST_REQUIRE( SceneWorld::cook( *data, testDirectory( guard ), {}, path, error ) );
    // Coverage describes geometry independently of an actor's pivot/grid owner.
    std::ifstream input( path.c_str(), std::ios::binary );
    Array<u8> bytes( ( std::istreambuf_iterator<char>( input ) ), std::istreambuf_iterator<char>() );
    input.close(); auto manifest = make_ptr<Properties>();
    BOOST_REQUIRE( PropertiesBinarySerializer::deserialize( bytes, *manifest, &error ) );
    for( auto cell : manifest->getChildrenByName( "cell" ) )
        if( cell->getProperty( "id" ) != "persistent" )
        {
            cell->setProperty( "minimum", Vector3F( -10000, 0, -10 ) );
            cell->setProperty( "maximum", Vector3F( 10000, 0, 10 ) );
        }
    bytes = PropertiesBinarySerializer::serialize( *manifest );
    std::ofstream output( path.c_str(), std::ios::binary | std::ios::trunc );
    output.write( reinterpret_cast<const char *>( bytes.data() ), bytes.size() ); output.close();
    SceneWorldStreamOptions options;
    options.loadRadius = 1; options.retainRadius = 2; options.minimumResidencySeconds = 0;
    options.maximumResidentActors = 2; options.maximumResidentCells = 2;
    SceneWorldStreamer stream;
    BOOST_REQUIRE_MESSAGE( stream.open( path, guard.scene, options, error ), error );
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );
    do
    {
        BOOST_REQUIRE_MESSAGE( stream.update( { Vector3F( -10001, 0, 0 ), Vector3F( 10001, 0, 0 ) }, error ), error );
        if( stream.statistics().residentActors == 2 && !stream.statistics().pendingCells ) break;
        std::this_thread::yield();
    } while( std::chrono::steady_clock::now() < until );
    BOOST_REQUIRE_EQUAL( stream.statistics().residentActors, 2u );
    BOOST_CHECK( guard.scene->findActorByName( "Wide coverage" ) );
    BOOST_CHECK( guard.scene->findActorByName( "Persistent" ) );
    BOOST_REQUIRE( stream.pinActor( spatial->getProperty( "uuid" ) ) );
    pump( stream, Vector3F( -1000000, 0, 0 ), 2 );
    BOOST_CHECK_EQUAL( stream.statistics().installedCells, 2u );
    BOOST_CHECK_EQUAL( stream.statistics().evictedCells, 0u );
    stream.close();
}

BOOST_AUTO_TEST_CASE( destroying_a_streamer_finishes_retirement_across_both_primary_job_queues )
{
    TestGuard guard; guard.scene->clear( true );
    enablePrimaryJobs( guard );
    auto previousManager = guard.applicationManager->getMeshManager();
    auto previousQueue = guard.applicationManager->getJobQueue();
    guard.addCleanup( [app = guard.applicationManager, previousManager, previousQueue]() mutable {
        app->setMeshManager( previousManager ); app->setJobQueue( previousQueue );
    } );
    const auto thread = Thread::getCurrentThreadId();
    guard.addCleanup( [thread] { Thread::setCurrentThreadId( thread ); } );
    Thread::setCurrentThreadId( Thread::ThreadId::Primary );
    auto data = make_ptr<Properties>(); auto group = pointActor( "Retirement group", 0 );
    for( int i = 1; i < 35; ++i )
    {
        auto child = pointActor( "Retirement child " + StringUtil::toString( i ), 0 );
        child->setName( "child" ); group->addChild( child );
    }
    data->addChild( group );
    String path, error;
    BOOST_REQUIRE( SceneWorld::cook( *data, testDirectory( guard ), {}, path, error ) );
    for( u32 kind = 0; kind < 2; ++kind )
    {
        auto manager = make_ptr<MeshManager>();
        SmartPtr<IJobQueue> queue = kind ? SmartPtr<IJobQueue>( make_ptr<JobQueueMT>() ) :
                                         SmartPtr<IJobQueue>( make_ptr<JobQueue>() );
        queue->load( nullptr ); queue->setRunning( true );
        guard.applicationManager->setMeshManager( manager ); guard.applicationManager->setJobQueue( queue );
        auto scheduler = std::make_shared<WorldStreamingScheduler>();
        auto stream = std::make_unique<SceneWorldStreamer>( scheduler );
        BOOST_REQUIRE_MESSAGE( stream->open( path, guard.scene, {}, error ), error );
        pump( *stream, Vector3F::zero(), 35 );
        Array<std::shared_ptr<u32>> counters;
        std::function<void( SmartPtr<IGameActor> )> assign = [&]( auto actor ) {
            auto resource = dynamic_pointer_cast<IMeshResource>( manager->createOrRetrieve(
                StringUtil::getUUID(), "Cache/close-" + StringUtil::toString( counters.size() ) + ".fbmeshbin", "Mesh" ).first );
            BOOST_REQUIRE( resource ); auto geometry = make_ptr<MeshRetirementProbe>();
            counters.push_back( geometry->unloadCounter ); resource->setMesh( geometry );
            resource->setLoadingState( LoadingState::Loaded );
            actor->getComponent<scene::Mesh>()->setMeshResource( resource );
            for( auto child : actor->getChildren() ) assign( child );
        };
        assign( guard.scene->findActorByName( "Retirement group" ) );
        auto graphics = make_ptr<DeferredGraphicsProbe>(); graphics->setLoadingState( LoadingState::Loaded );
        auto renderer = make_ptr<DeferredRendererProbe>();
        guard.scene->findActorByName( "Retirement group" )->addComponentInstance( renderer );
        renderer->setGraphicsObject( graphics ); renderer->setLoadingState( LoadingState::Loaded );
        stream.reset(); // The coordinator itself must no longer own the work.
        const auto unloaded = [&] { u32 n = 0; for( auto count : counters ) n += *count; return n; };
        BOOST_CHECK_EQUAL( unloaded(), 0u );
        BOOST_CHECK( queue->hasJobs() );
        queue->update(); // Register the coroutine on Primary, without spinning.
        queue->update(); // Still waiting for the render-side unload acknowledgement.
        BOOST_CHECK_EQUAL( unloaded(), 0u );
        BOOST_CHECK_GT( scheduler->stats().used[size_t( StreamMemoryClass::SourceCpu )], 0u );
        graphics->setLoadingState( LoadingState::Unloaded );
        queue->update(); // At most one bounded reclamation step per update.
        BOOST_CHECK_EQUAL( unloaded(), 32u );
        queue->update();
        BOOST_CHECK_EQUAL( unloaded(), 35u );
        BOOST_CHECK( !queue->hasJobs() );
        for( auto used : scheduler->stats().used ) BOOST_CHECK_EQUAL( used, 0u );
        queue->unload( nullptr ); manager->unload( nullptr );
    }
}

BOOST_AUTO_TEST_CASE( independent_streamers_do_not_pin_each_others_retirement_tickets )
{
    TestGuard guard; guard.scene->clear( true );
    enablePrimaryJobs( guard );
    auto previousManager = guard.applicationManager->getMeshManager();
    auto previousQueue = guard.applicationManager->getJobQueue();
    guard.addCleanup( [app = guard.applicationManager, previousManager, previousQueue]() mutable {
        app->setMeshManager( previousManager ); app->setJobQueue( previousQueue );
    } );
    auto manager = make_ptr<MeshManager>(); auto queue = make_ptr<JobQueue>();
    queue->load( nullptr ); queue->setRunning( true );
    guard.applicationManager->setMeshManager( manager ); guard.applicationManager->setJobQueue( queue );
    const auto thread = Thread::getCurrentThreadId();
    guard.addCleanup( [thread] { Thread::setCurrentThreadId( thread ); } );
    Thread::setCurrentThreadId( Thread::ThreadId::Primary );
    auto data = make_ptr<Properties>(); data->addChild( pointActor( "First world", 0 ) );
    String firstPath, secondPath, error;
    BOOST_REQUIRE( SceneWorld::cook( *data, testDirectory( guard ), {}, firstPath, error ) );
    data = make_ptr<Properties>(); data->addChild( pointActor( "Second world", 0 ) );
    BOOST_REQUIRE( SceneWorld::cook( *data, testDirectory( guard ), {}, secondPath, error ) );
    SceneWorldStreamer first, second;
    BOOST_REQUIRE( first.open( firstPath, guard.scene, {}, error ) );
    BOOST_REQUIRE( second.open( secondPath, guard.scene, {}, error ) );
    pump( first, Vector3F::zero(), 1 ); pump( second, Vector3F::zero(), 1 );
    auto resource = dynamic_pointer_cast<IMeshResource>( manager->createOrRetrieve(
        StringUtil::getUUID(), "Cache/shared-worlds.fbmeshbin", "Mesh" ).first );
    BOOST_REQUIRE( resource ); auto geometry = make_ptr<MeshRetirementProbe>();
    const auto unloads = geometry->unloadCounter;
    resource->setMesh( geometry ); geometry = nullptr; resource->setLoadingState( LoadingState::Loaded );
    guard.scene->findActorByName( "First world" )->getComponent<scene::Mesh>()->setMeshResource( resource );
    guard.scene->findActorByName( "Second world" )->getComponent<scene::Mesh>()->setMeshResource( resource );
    resource = nullptr;
    first.close(); BOOST_CHECK_EQUAL( *unloads, 0u );
    second.close(); BOOST_CHECK_EQUAL( *unloads, 1u );
    queue->update(); queue->update();
    BOOST_CHECK( !queue->hasJobs() );
    queue->unload( nullptr ); manager->unload( nullptr );
}

BOOST_AUTO_TEST_CASE( cooked_cells_load_independently_and_preserve_ids )
{
    TestGuard guard;
    auto scene = make_ptr<Properties>();
    for( int i = 0; i < 100; ++i ) scene->addChild( pointActor( "Cell " + StringUtil::toString( i ), float( i * 1000 ) ) );
    auto directory = testDirectory( guard );
    WorldCookOptions options; options.cellSize = 64; options.maximumActorsPerCell = 1;
    String manifestPath, error;
    BOOST_REQUIRE_MESSAGE( SceneWorld::cook( *scene, directory, options, manifestPath, error ), error );
    WorldManifest manifest;
    BOOST_REQUIRE_MESSAGE( SceneWorld::readManifest( manifestPath, manifest, error ), error );
    BOOST_REQUIRE_EQUAL( manifest.cells.size(), 100u );
    const auto cell = manifest.cells.front();
    auto payload = SceneWorld::readCell( manifestPath, cell, error );
    BOOST_REQUIRE_MESSAGE( payload, error );
    BOOST_REQUIRE_EQUAL( payload->getChildrenByName( "actors" ).size(), 1u );
    auto actor = payload->getChild( "actors" );
    String uuid; actor->getPropertyValue( "uuid", uuid );
    BOOST_CHECK( !uuid.empty() );
    BOOST_CHECK( cell.bytes < std::filesystem::file_size( manifestPath.c_str() ) );
    // Failure leaves the last published manifest intact.
    options.maximumActorsPerCell = 0;
    String rejected;
    BOOST_CHECK( !SceneWorld::cook( *scene, directory, options, rejected, error ) );
    WorldManifest retained;
    BOOST_REQUIRE( SceneWorld::readManifest( manifestPath, retained, error ) );
    BOOST_CHECK_EQUAL( retained.generation, manifest.generation );
    // Independent cells do not require unrelated payload files to exist.
    std::filesystem::remove( std::filesystem::path( directory.c_str() ) / manifest.cells.back().file.c_str() );
    BOOST_CHECK( SceneWorld::readCell( manifestPath, cell, error ) );
    BOOST_CHECK( !SceneWorld::readCell( manifestPath, manifest.cells.back(), error ) );
}

BOOST_AUTO_TEST_CASE( serialized_streaming_component_installs_and_tears_down_its_world )
{
    TestGuard guard; guard.scene->clear( true );
    auto data = make_ptr<Properties>(); data->addChild( pointActor( "Streamed child", 0 ) );
    String path, error;
    BOOST_REQUIRE( SceneWorld::cook( *data, testDirectory( guard ), {}, path, error ) );
    auto bootstrap = make_ptr<Properties>(); bootstrap->setName( "actors" );
    bootstrap->setProperty( "label", "Streaming owner" ); bootstrap->setProperty( "uuid", StringUtil::getUUID() );
    auto settings = make_ptr<Properties>(); settings->setName( "component" );
    settings->setProperty( "componentType", "workphone::scene::WorldStreaming" );
    settings->setProperty( "uuid", StringUtil::getUUID() ); settings->setProperty( "World Manifest", path );
    bootstrap->addChild( settings );
    auto roots = GameActorUtil::loadSceneActors( { bootstrap }, guard.scene );
    BOOST_REQUIRE_EQUAL( roots.size(), 1u ); guard.scene->addActor( roots.front() );
    auto component = roots.front()->getComponent<WorldStreaming>();
    BOOST_REQUIRE( component ); BOOST_REQUIRE( component->isLoaded() );
    const auto oldTask = Thread::getCurrentTask();
    guard.addCleanup( [oldTask] { Thread::setCurrentTask( oldTask ); } );
    Thread::setCurrentTask( guard.sceneManager->getSceneTask() );
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );
    while( component->statistics().residentActors != 1 && std::chrono::steady_clock::now() < until )
    { component->preUpdate(); std::this_thread::yield(); }
    BOOST_REQUIRE_EQUAL( component->statistics().residentActors, 1u );
    BOOST_CHECK( guard.scene->findActorByName( "Streamed child" ) );
    auto saved = workphone::dynamic_pointer_cast<Properties>( guard.scene->toData() );
    BOOST_REQUIRE( saved );
    BOOST_CHECK_EQUAL( saved->getChildrenByName( "actors" ).size(), 1u );
    guard.sceneManager->destroyActor( roots.front() );
    BOOST_CHECK( !guard.scene->findActorByName( "Streamed child" ) );
}

BOOST_AUTO_TEST_CASE( streamed_hierarchies_adopt_scene_mode_and_publish_renderer_transforms )
{
    TestGuard guard;
    const auto oldTask = Thread::getCurrentTask();
    const auto oldApplicationFlags = Thread::getTaskFlags( TaskId::Application );
    const auto oldMode = guard.scene->getState();
    guard.addCleanup( [scene = guard.scene, oldTask, oldApplicationFlags, oldMode]() mutable {
        scene->setState( oldMode );
        Thread::setTaskFlags( TaskId::Application, oldApplicationFlags );
        Thread::setCurrentTask( oldTask );
    } );
    Thread::setTaskFlags( TaskId::Application, oldApplicationFlags | Thread::Application_Flag );
    Thread::setCurrentTask( TaskId::Application );
    guard.scene->clear( true );
    guard.scene->setSceneLoadingState( IGameScene::SceneLoadingState::Loaded );
    auto data = make_ptr<Properties>();
    auto root = pointActor( "Activated root", 40 );
    root->getChild( "localTransform" )->setProperty( "Scale", Vector3F( 2, 3, 4 ) );
    auto child = pointActor( "Activated child", 5 ); child->setName( "child" );
    root->addChild( child ); data->addChild( root );
    for( auto actor : { root, child } )
    {
        auto renderer = make_ptr<Properties>(); renderer->setName( "component" );
        renderer->setProperty( "componentType", "workphone::scene::MeshRenderer" );
        renderer->setProperty( "uuid", StringUtil::getUUID() );
        renderer->setProperty( "state", 0 ); // Cooked content does not carry a live scene mode.
        actor->addChild( renderer );
    }
    String path, error;
    BOOST_REQUIRE_MESSAGE( SceneWorld::cook( *data, testDirectory( guard ), {}, path, error ), error );
    for( auto mode : { IGameScene::State::None, IGameScene::State::Edit, IGameScene::State::Play } )
    {
        guard.scene->setState( mode );
        auto scheduler = std::make_shared<WorldStreamingScheduler>();
        SceneWorldStreamer stream( scheduler );
        SceneWorldStreamOptions options;
        options.loadRadius = 1; options.retainRadius = 2; options.minimumResidencySeconds = 0;
        BOOST_REQUIRE_MESSAGE( stream.open( path, guard.scene, options, error ), error );
        const auto manifestSourceBytes = scheduler->stats().used[size_t( StreamMemoryClass::SourceCpu )];
        pump( stream, Vector3F( 40, 0, 0 ), 2 );
        auto loadedRoot = guard.scene->findActorByName( "Activated root" );
        BOOST_REQUIRE( loadedRoot );
        auto children = loadedRoot->getChildren(); BOOST_REQUIRE_EQUAL( children.size(), 1u );
        auto loadedChild = children.front();
        const auto actorMode = mode == IGameScene::State::Edit ? IGameActor::State::Edit :
                               mode == IGameScene::State::Play ? IGameActor::State::Play : IGameActor::State::None;
        const auto componentMode = mode == IGameScene::State::Edit ? IComponent::State::Edit :
                                   mode == IGameScene::State::Play ? IComponent::State::Play : IComponent::State::None;
        for( auto actor : { loadedRoot, loadedChild } )
        {
            BOOST_CHECK( actor->getState() == actorMode );
            auto renderer = actor->getComponent<MeshRenderer>();
            BOOST_REQUIRE( renderer );
            for( const auto &component : actor->getComponents() )
                BOOST_CHECK( component->getState() == componentMode );
            // An empty mesh path performs no asset/device work. Supplying a
            // node exercises the real Renderer::updateTransform mode gate.
            auto node = make_ptr<StreamPoseNode>();
            renderer->setGraphicsNode( node );
            if( mode == IGameScene::State::None )
                BOOST_CHECK_EQUAL( node->publications, 0u );
            else
            {
                BOOST_CHECK_GT( node->publications, 0u );
                const auto expected = Vector3F( actor == loadedRoot ? 40.0f : 50.0f, 0, 0 );
                BOOST_CHECK( node->pose.getPosition().equals( expected, 0.001f ) );
                BOOST_CHECK( node->pose.getScale().equals( Vector3F( 2, 3, 4 ), 0.001f ) );
            }
            renderer->setGraphicsNode( nullptr );
        }
        // Runtime mode changes must not create an edit overlay or retain
        // source memory. This also models a deferred FSM completing later.
        for( auto actor : { loadedRoot, loadedChild } )
            for( auto component : actor->getComponents() ) component->setState( IComponent::State::None );
        pump( stream, Vector3F( 100000, 0, 0 ), 0 );
        BOOST_REQUIRE_MESSAGE( stream.update( { Vector3F( 100000, 0, 0 ) }, error ), error );
        BOOST_CHECK_EQUAL( scheduler->stats().used[size_t( StreamMemoryClass::SourceCpu )], manifestSourceBytes );
        pump( stream, Vector3F( 40, 0, 0 ), 2 );
        loadedRoot = guard.scene->findActorByName( "Activated root" ); BOOST_REQUIRE( loadedRoot );
        children = loadedRoot->getChildren(); BOOST_REQUIRE_EQUAL( children.size(), 1u );
        loadedChild = children.front();
        for( auto actor : { loadedRoot, loadedChild } )
        {
            BOOST_CHECK( actor->getState() == actorMode );
            for( const auto &component : actor->getComponents() )
                BOOST_CHECK( component->getState() == componentMode );
        }
        if( mode != IGameScene::State::None )
        {
            loadedRoot->setName( "Edited active root" );
            loadedRoot->setLocalPosition( Vector3F( 43, 0, 0 ) );
            pump( stream, Vector3F( 100000, 0, 0 ), 0 );
            pump( stream, Vector3F( 40, 0, 0 ), 2 );
            auto edited = guard.scene->findActorByName( "Edited active root" ); BOOST_REQUIRE( edited );
            BOOST_CHECK_EQUAL( edited->getLocalPosition().x, 43.0f );
            BOOST_CHECK( edited->getState() == actorMode );
            for( const auto &component : edited->getComponents() )
                BOOST_CHECK( component->getState() == componentMode );
        }
        stream.close();
        BOOST_CHECK_EQUAL( scheduler->stats().used[size_t( StreamMemoryClass::SourceCpu )], 0u );
        BOOST_CHECK( !guard.scene->findActorByName( "Activated root" ) );
        BOOST_CHECK( !guard.scene->findActorByName( "Edited active root" ) );
    }
}

BOOST_AUTO_TEST_CASE( streaming_traversal_is_bounded_and_releases_reservations )
{
    TestGuard guard;
    guard.scene->clear( true );
    auto data = make_ptr<Properties>();
    for( int i = 0; i < 24; ++i ) data->addChild( pointActor( "Stream " + StringUtil::toString( i ), float( i * 1000 ) ) );
    WorldCookOptions cook; cook.cellSize = 64; cook.maximumActorsPerCell = 1;
    String manifestPath, error;
    BOOST_REQUIRE_MESSAGE( SceneWorld::cook( *data, testDirectory( guard ), cook, manifestPath, error ), error );
    WorldStreamLimits limits; limits.workers = 1;
    auto scheduler = std::make_shared<WorldStreamingScheduler>( limits );
    SceneWorldStreamOptions options;
    options.loadRadius = 1; options.retainRadius = 2;
    options.maximumResidentActors = 1; options.maximumResidentCells = 1;
    options.minimumResidencySeconds = 0;
    SceneWorldStreamer stream( scheduler );
    BOOST_REQUIRE_MESSAGE( stream.open( manifestPath, guard.scene, options, error ), error );
    for( int pass = 0; pass < 2; ++pass )
        for( int i = 0; i < 24; ++i )
        {
            pump( stream, Vector3F( float( i * 1000 ), 0, 0 ), 1 );
            BOOST_CHECK_LE( guard.scene->getActors().size(), 1u );
            BOOST_REQUIRE( guard.scene->findActorByName( "Stream " + StringUtil::toString( i ) ) );
            BOOST_CHECK_LE( stream.statistics().residentSourceBytes, 1ull << 20 );
        }
    const auto reads = stream.statistics().cellReads;
    for( int i = 0; i < 30; ++i ) BOOST_REQUIRE( stream.update( { Vector3F( 23000, 0, 0 ) }, error ) );
    BOOST_CHECK_EQUAL( stream.statistics().cellReads, reads );
    stream.close();
    BOOST_CHECK( guard.scene->getActors().empty() );
    for( auto bytes : scheduler->stats().used ) BOOST_CHECK_EQUAL( bytes, 0u );
}

BOOST_AUTO_TEST_CASE( oversized_groups_and_unsupported_manifests_fail_without_publication )
{
    TestGuard guard;
    auto data = make_ptr<Properties>(); auto actor = pointActor( "Animated", 0 );
    actor->setProperty( "smoothMotion", true );
    auto child = pointActor( "Child", 10 ); child->setName( "child" ); actor->addChild( child );
    data->addChild( actor );
    WorldCookOptions options; options.maximumActorsPerCell = 1; options.convertRenderOnlyLeaves = true;
    String path, error;
    const auto directory = testDirectory( guard );
    BOOST_CHECK( !SceneWorld::cook( *data, directory, options, path, error ) );
    BOOST_CHECK( !std::filesystem::exists( std::filesystem::path( directory.c_str() ) / "world.wpworld" ) );
    options.maximumActorsPerCell = 2;
    BOOST_REQUIRE_MESSAGE( SceneWorld::cook( *data, directory, options, path, error ), error );
    WorldManifest manifest;
    BOOST_REQUIRE( SceneWorld::readManifest( path, manifest, error ) );
    BOOST_REQUIRE_EQUAL( manifest.cells.size(), 1u );
    BOOST_CHECK_EQUAL( manifest.cells.front().id, "persistent" );
    auto invalid = make_ptr<Properties>(); invalid->setProperty( "version", WorldManifest::version + 1 );
    const auto bytes = PropertiesBinarySerializer::serialize( *invalid );
    std::ofstream file( path.c_str(), std::ios::binary | std::ios::trunc );
    file.write( reinterpret_cast<const char *>( bytes.data() ), bytes.size() ); file.close();
    BOOST_CHECK( !SceneWorld::readManifest( path, manifest, error ) );
    BOOST_CHECK( !error.empty() );
}

BOOST_AUTO_TEST_CASE( cancelled_scene_cannot_publish_late_cells )
{
    TestGuard guard;
    guard.scene->clear( true );
    auto data = make_ptr<Properties>(); data->addChild( pointActor( "Late", 0 ) );
    String path, error;
    BOOST_REQUIRE( SceneWorld::cook( *data, testDirectory( guard ), {}, path, error ) );
    auto scheduler = std::make_shared<WorldStreamingScheduler>();
    SceneWorldStreamer stream( scheduler );
    BOOST_REQUIRE( stream.open( path, guard.scene, {}, error ) );
    BOOST_REQUIRE( stream.update( { Vector3F::zero() }, error ) );
    guard.scene->clear( true );
    BOOST_CHECK( !stream.update( { Vector3F::zero() }, error ) );
    BOOST_CHECK( !stream.isOpen() );
    BOOST_CHECK( guard.scene->getActors().empty() );
    for( auto bytes : scheduler->stats().used ) BOOST_CHECK_EQUAL( bytes, 0u );
}

BOOST_AUTO_TEST_CASE( demand_larger_than_request_batch_converges_without_rereads )
{
    TestGuard guard; guard.scene->clear( true );
    auto data = make_ptr<Properties>();
    for( int i = 0; i < 20; ++i ) data->addChild( pointActor( "Batch " + StringUtil::toString( i ), float( i * 100 ) ) );
    WorldCookOptions cook; cook.cellSize = 64; cook.maximumActorsPerCell = 1;
    String path, error;
    BOOST_REQUIRE( SceneWorld::cook( *data, testDirectory( guard ), cook, path, error ) );
    SceneWorldStreamOptions options; options.loadRadius = 3000; options.retainRadius = 4000;
    options.maximumPendingCells = 3; options.maximumResidentCells = 20; options.maximumResidentActors = 20;
    auto scheduler = std::make_shared<WorldStreamingScheduler>();
    SceneWorldStreamer stream( scheduler );
    BOOST_REQUIRE( stream.open( path, guard.scene, options, error ) );
    pump( stream, Vector3F::zero(), 20 );
    BOOST_CHECK_EQUAL( stream.statistics().cellReads, 20u );
    BOOST_CHECK_EQUAL( guard.scene->getActors().size(), 20u );
    stream.close();
    for( auto bytes : scheduler->stats().used ) BOOST_CHECK_EQUAL( bytes, 0u );
}

BOOST_AUTO_TEST_CASE( edits_survive_eviction_and_dynamic_actors_are_promoted )
{
    TestGuard guard; guard.scene->clear( true );
    auto data = make_ptr<Properties>(); data->addChild( pointActor( "Editable", 0 ) );
    String path, error;
    BOOST_REQUIRE( SceneWorld::cook( *data, testDirectory( guard ), {}, path, error ) );
    SceneWorldStreamOptions options; options.loadRadius = 1; options.retainRadius = 2;
    options.minimumResidencySeconds = 0;
    SceneWorldStreamer stream;
    BOOST_REQUIRE( stream.open( path, guard.scene, options, error ) );
    pump( stream, Vector3F::zero(), 1 );
    auto actor = guard.scene->findActorByName( "Editable" ); BOOST_REQUIRE( actor );
    const auto uuid = actor->getHandle()->getUUIDAsString();
    actor->setName( "Edited" ); actor->setLocalPosition( { 3, 0, 0 } );
    actor->setLayer( "EditedLayer" );
    options.retainRadius = 3;
    BOOST_REQUIRE( stream.open( path, guard.scene, options, error ) );
    BOOST_CHECK( guard.scene->findActorByName( "Edited" ) == actor );
    pump( stream, { 10000, 0, 0 }, 0 );
    actor = nullptr;
    pump( stream, Vector3F::zero(), 1 );
    actor = guard.scene->findActorByName( "Edited" ); BOOST_REQUIRE( actor );
    BOOST_CHECK_EQUAL( actor->getHandle()->getUUIDAsString(), uuid );
    BOOST_CHECK_EQUAL( actor->getLocalPosition().x, 3.0f );
    WorldManifest manifest;
    BOOST_REQUIRE( SceneWorld::readManifest( path, manifest, error ) );
    auto constrained = options;
    constrained.maximumDecodedCellBytes = manifest.cells.front().decodedBytes;
    BOOST_CHECK( !stream.open( path, guard.scene, constrained, error ) );
    BOOST_CHECK( error.find( "overlay" ) != String::npos );
    BOOST_CHECK( stream.isOpen() );
    BOOST_CHECK( guard.scene->findActorByName( "Edited" ) == actor );
    // Rejection preserves the old decoded limit as well as live ownership.
    pump( stream, { 10000, 0, 0 }, 0 ); actor = nullptr;
    pump( stream, Vector3F::zero(), 1 );
    actor = guard.scene->findActorByName( "Edited" ); BOOST_REQUIRE( actor );
    BOOST_CHECK_EQUAL( actor->getLayer(), "EditedLayer" );
    BOOST_CHECK_EQUAL( actor->getLocalPosition().x, 3.0f );
    actor->setStatic( false );
    BOOST_REQUIRE( stream.update( { Vector3F( 10000, 0, 0 ) }, error ) );
    BOOST_CHECK_EQUAL( stream.statistics().residentActors, 1u );
    BOOST_CHECK( actor->isLoaded() );
}

BOOST_AUTO_TEST_CASE( missing_cell_failure_remains_visible_and_releases_budgets )
{
    TestGuard guard; guard.scene->clear( true );
    auto data = make_ptr<Properties>(); data->addChild( pointActor( "Missing", 0 ) );
    String path, error;
    BOOST_REQUIRE( SceneWorld::cook( *data, testDirectory( guard ), {}, path, error ) );
    WorldManifest manifest; BOOST_REQUIRE( SceneWorld::readManifest( path, manifest, error ) );
    std::filesystem::remove( std::filesystem::path( path.c_str() ).parent_path() / manifest.cells.front().file.c_str() );
    auto scheduler = std::make_shared<WorldStreamingScheduler>();
    SceneWorldStreamer stream( scheduler ); BOOST_REQUIRE( stream.open( path, guard.scene, {}, error ) );
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );
    while( stream.update( { Vector3F::zero() }, error ) && std::chrono::steady_clock::now() < until )
        std::this_thread::yield();
    BOOST_REQUIRE( !error.empty() );
    BOOST_CHECK( !stream.update( { Vector3F::zero() }, error ) );
    BOOST_CHECK( !error.empty() ); BOOST_CHECK( guard.scene->getActors().empty() );
    stream.close();
    for( auto bytes : scheduler->stats().used ) BOOST_CHECK_EQUAL( bytes, 0u );
}

BOOST_AUTO_TEST_CASE( static_hierarchy_reuses_transforms_and_tracks_dirty_children )
{
    TestGuard guard;
    guard.scene->clear( true );
    auto parent = guard.sceneManager->createActor();
    auto child = guard.sceneManager->createActor();
    guard.scene->addActor( parent ); parent->addChild( child );
    parent->setState( IGameActor::State::Edit ); child->setState( IGameActor::State::Edit );
    parent->setStatic( true, true );
    auto publications = make_ptr<TransformPublicationProbe>();
    child->addComponentInstance( publications );
    parent->setLocalPosition( Vector3F( 10, 0, 0 ) );
    child->setLocalPosition( Vector3F( 2, 0, 0 ) );
    for( int i = 0; i < 3; ++i ) parent->updateTransform();
    auto &storage = TransformSystem::instance(); storage.resetStatistics();
    const auto warmedPublications = publications->publications;
    for( int i = 0; i < 100; ++i )
    {
        parent->updateTransform();
        parent->setLocalPosition( Vector3F( 10, 0, 0 ) );
    }
    BOOST_CHECK_EQUAL( storage.statistics().worldBuilds, 0u );
    BOOST_CHECK_EQUAL( publications->publications, warmedPublications );
    child->getTransform()->setLocalPosition( Vector3F( 7, 0, 0 ) );
    child->getTransform()->setDirty( true );
    parent->updateTransform();
    BOOST_CHECK_EQUAL( child->getPosition().x, 17.0f );
    BOOST_CHECK_EQUAL( storage.statistics().worldBuilds, 1u );
    BOOST_CHECK_EQUAL( publications->publications, warmedPublications + 1 );
    parent->setLocalPosition( Vector3F( 20, 0, 0 ) );
    BOOST_CHECK_EQUAL( child->getPosition().x, 27.0f );
    child->setStatic( false );
    child->setLocalPosition( Vector3F( 9, 0, 0 ) );
    BOOST_CHECK_EQUAL( child->getPosition().x, 29.0f );
}

BOOST_AUTO_TEST_CASE( transform_handle_reuse_does_not_reuse_parent_or_revisions )
{
    auto &storage = TransformSystem::instance();
    auto parent = storage.createTransform();
    auto child = storage.createTransform();
    storage.setParent( child, parent );
    const auto before = storage.revisions( parent ).subtree;
    storage.setLocalPosition( child, Vector3F( 1, 2, 3 ) );
    BOOST_CHECK_GT( storage.revisions( parent ).subtree, before );
    BOOST_CHECK_THROW( storage.setParent( parent, child ), std::logic_error );
    storage.destroyTransform( child );
    auto replacement = storage.createTransform();
    BOOST_CHECK( replacement.index != child.index || replacement.generation != child.generation );
    BOOST_CHECK_EQUAL( storage.revisions( child ).world, 0u );
    const auto detached = storage.revisions( parent ).subtree;
    storage.setLocalPosition( replacement, Vector3F( 9, 9, 9 ) );
    BOOST_CHECK_EQUAL( storage.revisions( parent ).subtree, detached );
    storage.destroyTransform( replacement ); storage.destroyTransform( parent );
}

BOOST_AUTO_TEST_CASE( dirty_transform_batch_limit_keeps_the_unprocessed_request )
{
    TestGuard guard; guard.scene->clear( true );
    guard.scene->setSceneLoadingState( IGameScene::SceneLoadingState::Loaded );
    const auto oldTask = Thread::getCurrentTask();
    guard.addCleanup( [oldTask] { Thread::setCurrentTask( oldTask ); } );
    Thread::setCurrentTask( guard.sceneManager->getStateTask() );
    for( int i = 0; i < 4; ++i ) guard.sceneManager->preUpdate();
    Array<SmartPtr<Transform>> transforms;
    for( u32 i = 0; i < 2049; ++i )
    {
        auto transform = make_ptr<Transform>(); transform->load( nullptr );
        transform->setLocalPosition( { float( i + 1 ), 0, 0 } );
        transform->setDirty( true, false ); transforms.push_back( transform );
    }
    guard.sceneManager->preUpdate();
    BOOST_CHECK( !transforms[2047]->isDirty() );
    BOOST_CHECK( transforms[2048]->isDirty() );
    guard.sceneManager->preUpdate();
    BOOST_CHECK( !transforms[2048]->isDirty() );
    BOOST_CHECK_EQUAL( transforms[2048]->getPosition().x, 2049.0f );
    for( auto &transform : transforms ) transform->unload( nullptr );
}

BOOST_AUTO_TEST_CASE( concurrent_transform_registration_preserves_slots_and_queued_owners )
{
    TestGuard guard; guard.scene->clear( true );
    guard.scene->setSceneLoadingState( IGameScene::SceneLoadingState::Loaded );
    auto manager = dynamic_pointer_cast<GameManager>( guard.sceneManager ); BOOST_REQUIRE( manager );
    const auto task = Thread::getCurrentTask();
    guard.addCleanup( [task] { Thread::setCurrentTask( task ); } );
    Thread::setCurrentTask( manager->getStateTask() );
    struct Entry { SmartPtr<Transform> transform; u32 id = 0; };
    std::array<Entry, 4> entries;
    PhaseBarrier barrier;
    Array<std::future<void>> workers;
    for( u32 i = 0; i < entries.size(); ++i )
        workers.push_back( std::async( std::launch::async, [&, i] {
            for( u32 round = 0; round < 64; ++round )
            {
                auto transform = dynamic_pointer_cast<Transform>( manager->createTransform() );
                entries[i].id = transform->getHandle()->getInstanceId();
                entries[i].transform = transform;
                transform->setLocalPosition( Vector3F( float( round + i ), 0, 0 ) );
                transform->setDirty( true, false );
                if( !barrier.wait() || !barrier.wait() ) return;
                manager->destroyTransform( transform ); entries[i].transform = nullptr;
                if( !barrier.wait() ) return;
            }
        } ) );
    bool synchronized = true;
    for( u32 round = 0; round < 64; ++round )
    {
        if( !barrier.wait() ) { synchronized = false; break; }
        for( u32 i = 0; i < entries.size(); ++i )
        {
            BOOST_CHECK( manager->getTransform( entries[i].id ) == entries[i].transform );
            for( u32 j = 0; j < i; ++j ) BOOST_CHECK_NE( entries[i].id, entries[j].id );
        }
        if( !barrier.wait() ) { synchronized = false; break; }
        manager->preUpdate(); // Consume retained queue entries during concurrent removal.
        if( !barrier.wait() ) { synchronized = false; break; }
    }
    for( auto &worker : workers ) worker.get();
    BOOST_CHECK( synchronized );
    manager->preUpdate();
}

BOOST_AUTO_TEST_CASE( transform_acquisition_retains_ownership_during_actor_detachment )
{
    TestGuard guard;
    auto actor = make_ptr<GameActor>();
    PhaseBarrier barrier;
    std::atomic<bool> valid{ true };
    std::atomic<u32> destroyed{ 0 }, reads{ 0 };
    Array<std::future<void>> workers;
    for( u32 worker = 0; worker < 4; ++worker )
        workers.push_back( std::async( std::launch::async, [&] {
            for( u32 round = 0; round < 64; ++round )
            {
                if( !barrier.wait() ) { valid = false; return; }
                for( u32 i = 0; i < 512; ++i )
                    if( auto transform = actor->getTransform() )
                    {
                        ++reads;
                        if( !transform->isAlive() || transform->getReferences() <= 0 ) valid = false;
                        transform->getLocalPosition();
                    }
                if( !barrier.wait() ) { valid = false; return; }
            }
        } ) );
    for( u32 round = 0; round < 64; ++round )
    {
        if( !barrier.wait() ) { valid = false; break; }
        for( u32 i = 0; i < 512; ++i )
        {
            auto transform = make_ptr<TransformLifetimeProbe>();
            transform->onDestroy = [&] {
                // Final release must run after the acquisition gate is released.
                auto current = actor->getTransform();
                ++destroyed;
            };
            actor->setTransform( transform );
            if( i % 16 == 0 ) actor->setTransform( nullptr );
        }
        if( !barrier.wait() ) { valid = false; break; }
    }
    for( auto &worker : workers ) worker.get();
    actor->setTransform( nullptr );
    BOOST_CHECK( valid.load() );
    BOOST_CHECK_GT( reads.load(), 0u );
    BOOST_CHECK_EQUAL( destroyed.load(), 64u * 512u );
}

BOOST_AUTO_TEST_CASE( concurrent_scene_resource_refresh_preserves_transform_owners_and_complete_lookups )
{
    TestGuard guard; guard.scene->clear( true );
    auto database = make_ptr<ResourceDatabase>();
    auto catalog = make_ptr<SnapshotCatalog>();
    database->setDatabaseManager( catalog );
    Array<SmartPtr<ITransform>> transforms;
    for( u32 i = 0; i < 32; ++i )
    {
        auto actor = guard.sceneManager->createActor(); guard.scene->addActor( actor );
        auto transform = actor->getTransform();
        transform->getHandle()->setUUID( StringUtil::getUUID() );
        transforms.push_back( transform );
    }
    database->refresh();
    PhaseBarrier barrier;
    std::atomic<bool> valid{ true };
    Array<std::future<void>> workers;
    for( u32 worker = 0; worker < 4; ++worker )
        workers.push_back( std::async( std::launch::async, [&] {
            if( !barrier.wait() ) { valid = false; return; }
            for( u32 round = 0; round < 64; ++round )
            {
                database->refresh();
                for( const auto &transform : transforms )
                    if( database->getObject( transform->getHandle()->getUUID() ) != transform ||
                        !transform->isAlive() || !transform->isLoaded() ) valid = false;
            }
        } ) );
    BOOST_CHECK( barrier.wait() );
    for( auto &worker : workers ) worker.get();
    BOOST_CHECK( valid.load() );
    BOOST_CHECK_EQUAL( catalog->loads.load(), 257u );
    database->setDatabaseManager( nullptr ); database = nullptr;
    for( auto &transform : transforms )
    {
        BOOST_CHECK( transform->isAlive() );
        BOOST_CHECK_GE( transform->getReferences(), 3 ); // Registry, actor and this snapshot.
    }
}

BOOST_AUTO_TEST_CASE( scene_membership_refreshes_coalesce_without_reentrant_primary_queue_spins )
{
    TestGuard guard;
    enablePrimaryJobs( guard );
    auto previousDatabase = guard.applicationManager->getResourceDatabase();
    auto previousQueue = guard.applicationManager->getJobQueue();
    const auto task = Thread::getCurrentTask();
    const auto thread = Thread::getCurrentThreadId();
    guard.addCleanup( [app = guard.applicationManager, previousDatabase, previousQueue, task, thread]() mutable {
        app->setResourceDatabase( previousDatabase ); app->setJobQueue( previousQueue );
        Thread::setCurrentTask( task ); Thread::setCurrentThreadId( thread );
    } );
    Thread::setCurrentTask( TaskId::Primary ); Thread::setCurrentThreadId( Thread::ThreadId::Primary );
    auto manager = dynamic_pointer_cast<GameManager>( guard.sceneManager ); BOOST_REQUIRE( manager );
    auto request = [&] { manager->handleEvent( EventType::Scene, IEvent::addActor, {}, nullptr, nullptr, nullptr ); };
    for( u32 kind = 0; kind < 2; ++kind )
    {
        auto database = make_ptr<RefreshProbe>();
        SmartPtr<IJobQueue> queue = kind ? SmartPtr<IJobQueue>( make_ptr<JobQueueMT>() ) :
                                         SmartPtr<IJobQueue>( make_ptr<JobQueue>() );
        queue->load( nullptr ); queue->setRunning( true );
        guard.applicationManager->setJobQueue( queue ); guard.applicationManager->setResourceDatabase( database );
        bool reentered = false;
        database->onRefresh = [&] { if( !reentered ) { reentered = true; request(); } };
        for( u32 i = 0; i < 128; ++i ) request();
        BOOST_CHECK_EQUAL( database->calls, 0u );
        queue->update();
        BOOST_CHECK_EQUAL( database->calls, 1u );
        BOOST_CHECK( !queue->hasJobs() );
        request(); queue->update();
        BOOST_CHECK_EQUAL( database->calls, 2u );
        bool changedDuringScan = false;
        database->onRefresh = [&] {
            if( changedDuringScan ) return;
            changedDuringScan = true;
            auto worker = std::async( std::launch::async, request ); worker.get();
        };
        request(); queue->update();
        BOOST_CHECK_EQUAL( database->calls, 3u );
        BOOST_CHECK( queue->hasJobs() );
        queue->update();
        BOOST_CHECK_EQUAL( database->calls, 4u );
        BOOST_CHECK( !queue->hasJobs() );
        database->onRefresh = {};

        // A queued strong reference preserves storage, not an open catalog.
        request(); database->unload( nullptr ); queue->update();
        BOOST_CHECK_EQUAL( database->calls, 4u );
        BOOST_CHECK( !queue->hasJobs() );
        database->setLoadingState( LoadingState::Loaded );
        request(); queue->update();
        BOOST_CHECK_EQUAL( database->calls, 5u );
        BOOST_CHECK( !queue->hasJobs() );

        // Replacing the active database must invalidate the old queued scan
        // without suppressing work for the newly published database.
        request();
        auto replacement = make_ptr<RefreshProbe>();
        guard.applicationManager->setResourceDatabase( replacement );
        request(); queue->update(); queue->update();
        BOOST_CHECK_EQUAL( database->calls, 5u );
        BOOST_CHECK_EQUAL( replacement->calls, 1u );
        BOOST_CHECK( !queue->hasJobs() );
        guard.applicationManager->setResourceDatabase( database );
        request(); queue->update();
        BOOST_CHECK_EQUAL( database->calls, 6u );
        BOOST_CHECK( !queue->hasJobs() );

        queue->setRunning( false ); request();
        BOOST_CHECK_EQUAL( database->calls, 7u );
        queue->unload( nullptr );
    }
}

BOOST_AUTO_TEST_CASE( transform_callbacks_allow_concurrent_hierarchy_capture )
{
    TestGuard guard; guard.scene->clear( true );
    auto parent = guard.sceneManager->createActor(); auto child = guard.sceneManager->createActor();
    guard.scene->addActor( parent ); parent->addChild( child );
    parent->setState( IGameActor::State::Edit ); child->setState( IGameActor::State::Edit );
    auto probe = make_ptr<TransformPublicationProbe>(); child->addComponentInstance( probe );
    std::promise<void> start, captured;
    auto trigger = start.get_future(); auto complete = captured.get_future();
    std::thread capture( [&] {
        trigger.wait();
        auto children = parent->getChildren(); auto components = child->getComponents();
        captured.set_value();
    } );
    bool completedInsideCallback = false;
    probe->callback = [&] {
        start.set_value();
        completedInsideCallback = complete.wait_for( std::chrono::seconds( 2 ) ) == std::future_status::ready;
    };
    parent->updateTransform();
    probe->callback = {};
    capture.join();
    BOOST_CHECK_MESSAGE( completedInsideCallback,
                        "Render capture must not wait on hierarchy locks held across component callbacks" );
}
BOOST_AUTO_TEST_SUITE_END()
