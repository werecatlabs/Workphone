#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/SceneWorld.hpp>
#include <Workphone/Scene/SceneWorldStreamer.hpp>
#include <Workphone/Scene/Transform.hpp>
#include <Workphone/Scene/Components/WorldStreaming.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <chrono>
#include <thread>
#include <fstream>
#include <future>

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
    options.retainRadius = 3;
    BOOST_REQUIRE( stream.open( path, guard.scene, options, error ) );
    BOOST_CHECK( guard.scene->findActorByName( "Edited" ) == actor );
    pump( stream, { 10000, 0, 0 }, 0 );
    actor = nullptr;
    pump( stream, Vector3F::zero(), 1 );
    actor = guard.scene->findActorByName( "Edited" ); BOOST_REQUIRE( actor );
    BOOST_CHECK_EQUAL( actor->getHandle()->getUUIDAsString(), uuid );
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
