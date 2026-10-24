#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Jobs/ActorLoadJob.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Scene/Components/Camera/CameraFollow.hpp>
#include <Workphone/Scene/Components/Camera/CameraTarget.hpp>
#include <Workphone/System/TaskLock.hpp>
#include <boost/test/unit_test.hpp>
#include <chrono>
#include <thread>
#include <atomic>
#include <fstream>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    SmartPtr<Properties> actorData( const String &name, const String &uuid = StringUtil::getUUID() )
    {
        auto data = make_ptr<Properties>();
        data->setName( ApplicationUtil::actorsStr );
        data->setProperty( GameActorUtil::labelStr, name );
        data->setProperty( GameActorUtil::uuidStr, uuid );
        return data;
    }

    String orderedSceneData()
    {
        auto data = make_ptr<Properties>();
        for( const auto &name : { "First", "Second", "Third" } )
            data->addChild( actorData( name ) );
        return DataUtil::toString( data.get() );
    }

    String createFileScene( TestGuard &guard )
    {
        auto data = make_ptr<Properties>();
        auto canvas = actorData( "Canvas" );
        for( const auto &name : { "Panel", "Dropdown" } )
        {
            auto child = actorData( name );
            child->setName( GameActorUtil::childStr );
            canvas->addChild( child );
        }
        data->addChild( canvas );
        auto path = Path::lexically_normal( Path::getWorkingDirectory(),
                                            "scene-test-" + StringUtil::getUUID() + ".fbscene" );
        guard.trackFilesystemPath( path );
        std::ofstream file( path.c_str(), std::ios::binary );
        BOOST_REQUIRE( file.is_open() );
        auto text = DataUtil::toString( data.get() );
        file.write( text.c_str(), static_cast<std::streamsize>( text.size() ) );
        BOOST_REQUIRE( file.good() );
        return path;
    }

    void drainLoad( TestGuard &guard )
    {
        auto queue = guard.applicationManager->getJobQueue();
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
        do
        {
            queue->update();
            guard.taskManager->update();
            if( !queue->hasJobs() &&
                guard.scene->getSceneLoadingState() != IGameScene::SceneLoadingState::Loading )
                return;
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        } while( std::chrono::steady_clock::now() < deadline );
        BOOST_FAIL( "Scene loading did not drain within five seconds" );
    }

    void resetScene( TestGuard &guard )
    {
        guard.scene->setState( IGameScene::State::Edit );
        guard.scene->clear( true );
    }

    void pumpAsyncSceneLoading( TestGuard &guard, u32 iterations = 20 )
    {
        auto jobQueue = guard.applicationManager->getJobQueue();
        BOOST_REQUIRE( jobQueue );

        for( u32 i = 0; i < iterations; ++i )
        {
            jobQueue->update();
            guard.taskManager->update();
            guard.runUpdateCycle( 1 );

            if( guard.scene->getSceneLoadingState() == IGameScene::SceneLoadingState::Loaded &&
                guard.scene->findActorByName( "Canvas" ) && !jobQueue->hasJobs() )
            {
                return;
            }
        }
    }
}  // namespace

BOOST_AUTO_TEST_SUITE( GameSceneAsyncLoadingTests )

BOOST_AUTO_TEST_CASE( graph_load_preserves_render_pause_after_success_and_rollback )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );
    resetScene( guard );
    auto renderTask = guard.taskManager->getTask( TaskId::Render );
    BOOST_REQUIRE( renderTask );
    BOOST_REQUIRE( renderTask->isLoaded() );
    const auto wasStopped = renderTask->isStopped();
    const auto actorCount = guard.sceneManager->getNumActors();

    auto actors = GameActorUtil::loadSceneActors( { actorData( "Loaded" ) }, guard.scene );
    BOOST_REQUIRE_EQUAL( actors.size(), 1u );
    BOOST_CHECK_EQUAL( renderTask->isStopped(), wasStopped );
    guard.sceneManager->destroyActor( actors.front() );

    auto invalid = actorData( "Rollback" );
    auto component = make_ptr<Properties>();
    component->setName( GameActorUtil::componentStr );
    component->setProperty( GameActorUtil::componentTypeStr, "MissingGraphLoadTestComponent" );
    invalid->addChild( component );
    {
        TaskLock outerPause( renderTask );
        BOOST_CHECK_THROW( GameActorUtil::loadSceneActors( { invalid }, guard.scene ),
                           std::runtime_error );
        BOOST_CHECK( renderTask->isStopped() );
        BOOST_CHECK_EQUAL( guard.sceneManager->getNumActors(), actorCount );
    }
    BOOST_CHECK_EQUAL( renderTask->isStopped(), wasStopped );
}

BOOST_AUTO_TEST_CASE( gamescene_async_file_load_completes_through_job_queue )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );
    const auto AsyncTestScenePath = createFileScene( guard );

    guard.sceneManager->clear();
    guard.scene->setState( IGameScene::State::Edit );
    guard.scene->clear( true );

    guard.scene->loadScene( AsyncTestScenePath, true );
    pumpAsyncSceneLoading( guard );

    BOOST_CHECK( guard.scene->getSceneLoadingState() == IGameScene::SceneLoadingState::Loaded );

    auto canvasActor = guard.scene->findActorByName( "Canvas" );
    BOOST_REQUIRE( canvasActor );
    BOOST_CHECK( canvasActor->getScene() == guard.scene );
    BOOST_CHECK( canvasActor->findChildByName( "Panel" ) );
    BOOST_CHECK( canvasActor->findChildByName( "Dropdown" ) );
}

BOOST_AUTO_TEST_CASE( gamescene_async_file_load_can_reload_after_clear )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );
    const auto AsyncTestScenePath = createFileScene( guard );

    guard.sceneManager->clear();
    guard.scene->setState( IGameScene::State::Edit );
    guard.scene->clear( true );

    guard.scene->loadScene( AsyncTestScenePath, true );
    pumpAsyncSceneLoading( guard );

    auto firstLoadCount = guard.scene->getActors().size();
    BOOST_REQUIRE_GT( firstLoadCount, 0u );

    guard.scene->clear( true );
    BOOST_CHECK( guard.scene->getActors().empty() );

    guard.scene->loadScene( AsyncTestScenePath, true );
    pumpAsyncSceneLoading( guard );

    BOOST_CHECK_EQUAL( guard.scene->getActors().size(), firstLoadCount );
    BOOST_CHECK( guard.scene->findActorByName( "Canvas" ) );
}

BOOST_AUTO_TEST_CASE( inline_load_preserves_root_order_sync_and_async )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );
    for( bool async : { false, true } )
    {
        resetScene( guard );
        guard.scene->loadSceneDataStr( orderedSceneData(), async );
        drainLoad( guard );
        BOOST_REQUIRE( guard.scene->getSceneLoadingState() == IGameScene::SceneLoadingState::Loaded );
        auto actors = guard.scene->getActors();
        BOOST_REQUIRE_EQUAL( actors.size(), 3u );
        BOOST_CHECK_EQUAL( actors[0]->getName(), "First" );
        BOOST_CHECK_EQUAL( actors[1]->getName(), "Second" );
        BOOST_CHECK_EQUAL( actors[2]->getName(), "Third" );
    }
}

BOOST_AUTO_TEST_CASE( file_load_repairs_copied_root_ids_and_internal_references )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );
    auto data = make_ptr<Properties>();
    const auto rootUUID = StringUtil::getUUID();
    const auto childUUID = StringUtil::getUUID();
    const auto componentUUID = StringUtil::getUUID();
    for( const auto &name : { "Original", "Copy" } )
    {
        auto root = actorData( name, rootUUID );
        auto child = actorData( "Target", childUUID );
        child->setName( GameActorUtil::childStr );
        root->addChild( child );
        auto follow = make_ptr<Properties>();
        follow->setName( GameActorUtil::componentStr );
        follow->setProperty( GameActorUtil::componentTypeStr, "CameraFollow" );
        follow->setProperty( GameActorUtil::uuidStr, componentUUID );
        follow->setProperty( CameraFollow::followObjectStr, childUUID );
        root->addChild( follow );
        data->addChild( root );
    }
    const auto path = Path::lexically_normal( Path::getWorkingDirectory(),
                                             "copied-scene-" + StringUtil::getUUID() + ".fbscene" );
    guard.trackFilesystemPath( path );
    const auto text = DataUtil::toString( data.get() );
    {
        std::ofstream file( path.c_str(), std::ios::binary );
        BOOST_REQUIRE( file.is_open() );
        file.write( text.c_str(), static_cast<std::streamsize>( text.size() ) );
        BOOST_REQUIRE( file.good() );
    }
    const auto originalProjectPath = guard.applicationManager->getProjectPath();
    guard.addCleanup( [app = guard.applicationManager, originalProjectPath]() mutable {
        app->setProjectPath( originalProjectPath );
    } );
    // Absolute asset paths must remain absolute even with a project directory configured.
    guard.applicationManager->setProjectPath( Path::getWorkingDirectory() );
    for( bool async : { false, true } )
    {
        resetScene( guard );
        guard.sceneManager->loadScene( path, async );
        drainLoad( guard );
        BOOST_REQUIRE( guard.scene->getSceneLoadingState() == IGameScene::SceneLoadingState::Loaded );
        auto roots = guard.scene->getActors();
        BOOST_REQUIRE_EQUAL( roots.size(), 2u );
        BOOST_CHECK_EQUAL( roots[0]->getHandle()->getUUIDAsString(), rootUUID );
        BOOST_CHECK( roots[1]->getHandle()->getUUIDAsString() != rootUUID );
        for( const auto &root : roots )
        {
            auto follow = root->getComponent<CameraFollow>();
            BOOST_REQUIRE( follow );
            BOOST_CHECK( follow->getFollowObject() == root->findChildByName( "Target" ) );
        }
        BOOST_CHECK( roots[0]->getComponent<CameraFollow>()->getHandle()->getUUIDAsString() !=
                     roots[1]->getComponent<CameraFollow>()->getHandle()->getUUIDAsString() );
    }
    BOOST_CHECK_EQUAL( Path::readAllText( path ), text );
}

BOOST_AUTO_TEST_CASE( components_resolve_forward_actor_and_component_references )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );
    resetScene( guard );
    const auto laterUUID = StringUtil::getUUID();
    const auto targetUUID = StringUtil::getUUID();
    auto data = make_ptr<Properties>();
    auto first = actorData( "Follower" );
    auto follow = make_ptr<Properties>();
    follow->setName( GameActorUtil::componentStr );
    follow->setProperty( GameActorUtil::componentTypeStr, "CameraFollow" );
    follow->setProperty( CameraFollow::followObjectStr, laterUUID );
    follow->setProperty( CameraFollow::targetStr, targetUUID );
    first->addChild( follow );
    auto later = actorData( "Later", laterUUID );
    auto target = make_ptr<Properties>();
    target->setName( GameActorUtil::componentStr );
    target->setProperty( GameActorUtil::componentTypeStr, "CameraTarget" );
    target->setProperty( GameActorUtil::uuidStr, targetUUID );
    later->addChild( target );
    data->addChild( first );
    data->addChild( later );
    guard.scene->loadSceneDataStr( DataUtil::toString( data.get() ), true );
    drainLoad( guard );
    auto actors = guard.scene->getActors();
    BOOST_REQUIRE_EQUAL( actors.size(), 2u );
    auto component = actors[0]->getComponent<CameraFollow>();
    BOOST_REQUIRE( component );
    BOOST_CHECK( component->getFollowObject() == actors[1] );
    BOOST_CHECK( component->getTarget() == actors[1]->getComponent<CameraTarget>() );
}

BOOST_AUTO_TEST_CASE( actor_job_loads_children_once_in_serialized_order )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );
    resetScene( guard );
    auto data = actorData( "Parent" );
    for( const auto &name : { "ChildA", "ChildB" } )
    {
        auto child = actorData( name );
        child->setName( GameActorUtil::childStr );
        data->addChild( child );
    }
    auto job = make_ptr<ActorLoadJob>();
    job->setScene( guard.scene );
    job->setProperties( data );
    job->setCreateChildJobs( true );
    guard.applicationManager->getJobQueue()->addJob( job );
    drainLoad( guard );
    BOOST_REQUIRE( job->getActor() );
    auto children = job->getActor()->getChildren();
    BOOST_REQUIRE_EQUAL( children.size(), 2u );
    BOOST_CHECK_EQUAL( children[0]->getName(), "ChildA" );
    BOOST_CHECK_EQUAL( children[1]->getName(), "ChildB" );
    BOOST_CHECK( job->getChildJobs().empty() );
}

BOOST_AUTO_TEST_CASE( cleared_pending_load_cannot_repopulate_scene )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );
    resetScene( guard );
    guard.scene->loadSceneDataStr( orderedSceneData(), true );
    guard.scene->clear( true );
    drainLoad( guard );
    BOOST_CHECK( guard.scene->getActors().empty() );
    BOOST_CHECK( guard.scene->getSceneLoadingState() == IGameScene::SceneLoadingState::Cancelled );
}

BOOST_AUTO_TEST_CASE( duplicate_uuid_load_fails_without_leaking_registered_actors )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );
    resetScene( guard );
    const auto count = guard.sceneManager->getNumActors();
    const auto uuid = StringUtil::getUUID();
    auto data = make_ptr<Properties>();
    data->addChild( actorData( "DuplicateA", uuid ) );
    data->addChild( actorData( "DuplicateB", uuid ) );
    guard.scene->loadSceneDataStr( DataUtil::toString( data.get() ), false );
    BOOST_CHECK( guard.scene->getSceneLoadingState() == IGameScene::SceneLoadingState::Failed );
    BOOST_CHECK( guard.scene->getActors().empty() );
    BOOST_CHECK_EQUAL( guard.sceneManager->getNumActors(), count );
}

BOOST_AUTO_TEST_CASE( cleared_pending_actor_job_cannot_attach_its_actor )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );
    resetScene( guard );
    auto job = make_ptr<ActorLoadJob>();
    job->setScene( guard.scene );
    job->setProperties( actorData( "CancelledActor" ) );
    guard.applicationManager->getJobQueue()->addJob( job );
    guard.scene->clear( true );
    drainLoad( guard );
    BOOST_CHECK( !job->getActor() );
    BOOST_CHECK( guard.scene->getActors().empty() );
}

BOOST_AUTO_TEST_CASE( malformed_inline_scene_reports_failure )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );
    resetScene( guard );
    guard.scene->loadSceneDataStr( "{invalid-json", true );
    drainLoad( guard );
    BOOST_CHECK( guard.scene->getSceneLoadingState() == IGameScene::SceneLoadingState::Failed );
    BOOST_CHECK( guard.scene->getActors().empty() );
}

BOOST_AUTO_TEST_CASE( ordered_async_load_with_zero_one_and_multiple_job_workers )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );
    auto originalPool = guard.applicationManager->getThreadPool();
    auto pool = make_ptr<ThreadPool>();
    guard.applicationManager->setThreadPool( pool );
    struct RestorePool
    {
        SmartPtr<core::IApplicationManager> application;
        SmartPtr<IThreadPool> original;
        ~RestorePool()
        {
            application->setThreadPool( original );
        }
    } restore{ guard.applicationManager, originalPool };
    auto queue = guard.applicationManager->getJobQueue();
    for( u32 count : { 0u, 1u, 3u } )
    {
        resetScene( guard );
        pool->setNumThreads( count );
        std::atomic<bool> stop{ false };
        std::vector<std::thread> workers;
        struct JoinWorkers
        {
            std::atomic<bool> &stop;
            std::vector<std::thread> &workers;
            ~JoinWorkers()
            {
                stop = true;
                for( auto &worker : workers )
                    worker.join();
            }
        } join{ stop, workers };
        for( u32 i = 0; i < count; ++i )
            workers.emplace_back( [&, i] {
                Thread::setCurrentThreadId( static_cast<Thread::ThreadId>( i ) );
                while( !stop.load() )
                {
                    queue->update();
                    std::this_thread::yield();
                }
            } );
        guard.scene->loadSceneDataStr( orderedSceneData(), true );
        drainLoad( guard );
        BOOST_REQUIRE( guard.scene->getSceneLoadingState() == IGameScene::SceneLoadingState::Loaded );
        auto actors = guard.scene->getActors();
        BOOST_REQUIRE_EQUAL( actors.size(), 3u );
        BOOST_CHECK_EQUAL( actors[0]->getName(), "First" );
        BOOST_CHECK_EQUAL( actors[1]->getName(), "Second" );
        BOOST_CHECK_EQUAL( actors[2]->getName(), "Third" );
    }
}

BOOST_AUTO_TEST_CASE( deferred_play_scene_load_retains_perpetual_root_hierarchy )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );
    const auto wasPlaying = guard.applicationManager->isPlaying();
    guard.applicationManager->setPlaying( false );
    guard.addCleanup( [&guard, wasPlaying] {
        guard.applicationManager->setPlaying( false );
        guard.scene->setState( IGameScene::State::Edit );
        guard.scene->clear( true );
        guard.applicationManager->setPlaying( wasPlaying );
    } );
    resetScene( guard );

    auto retained = guard.createBasicActor();
    auto child = guard.createBasicActor();
    auto grandchild = guard.createBasicActor();
    auto ordinary = guard.createBasicActor();
    BOOST_REQUIRE( retained && child && grandchild && ordinary );
    retained->setName( "PersistentRig" );
    child->setName( "PersistentChild" );
    grandchild->setName( "PersistentGrandchild" );
    retained->addChild( child );
    child->addChild( grandchild );
    retained->setPerpetual( true, false );
    guard.scene->addActor( retained );
    guard.scene->addActor( ordinary );
    const auto retainedUUID = retained->getHandle()->getUUIDAsString();
    guard.applicationManager->setPlaying( true );
    guard.scene->setState( IGameScene::State::Play );

    auto data = make_ptr<Properties>();
    data->addChild( actorData( "NewTerrain" ) );
    guard.scene->clear( false );
    guard.scene->loadSceneDataStr( DataUtil::toString( data.get() ), true );
    drainLoad( guard );

    BOOST_REQUIRE( guard.scene->getSceneLoadingState() == IGameScene::SceneLoadingState::Loaded );
    BOOST_CHECK_EQUAL( guard.scene->getActors().size(), 2u );
    BOOST_CHECK( guard.scene->findActorByName( "PersistentRig" ) == retained );
    BOOST_CHECK_EQUAL( retained->getHandle()->getUUIDAsString(), retainedUUID );
    BOOST_REQUIRE_EQUAL( retained->getChildren().size(), 1u );
    BOOST_CHECK( retained->getChildren()[0] == child );
    BOOST_REQUIRE_EQUAL( child->getChildren().size(), 1u );
    BOOST_CHECK( child->getChildren()[0] == grandchild );
    BOOST_CHECK( child->getParent() == retained );
    BOOST_CHECK( grandchild->getParent() == child );
    BOOST_CHECK( retained->isLoaded() && child->isLoaded() && grandchild->isLoaded() );
    BOOST_CHECK( retained->getScene() == guard.scene );
    BOOST_CHECK( guard.scene->findActorByName( "NewTerrain" ) );
    BOOST_CHECK( ordinary->getLoadingState() == LoadingState::Unloaded );
}

BOOST_AUTO_TEST_CASE( deferred_play_load_reuses_retained_uuid_and_resolves_references )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );
    const auto wasPlaying = guard.applicationManager->isPlaying();
    guard.applicationManager->setPlaying( false );
    guard.addCleanup( [&guard, wasPlaying] {
        guard.applicationManager->setPlaying( false );
        guard.scene->setState( IGameScene::State::Edit );
        guard.scene->clear( true );
        guard.applicationManager->setPlaying( wasPlaying );
    } );
    resetScene( guard );

    auto retained = guard.createBasicActor();
    auto child = guard.createBasicActor();
    auto ordinary = guard.createBasicActor();
    BOOST_REQUIRE( retained && child && ordinary );
    retained->setName( "RuntimeRig" );
    child->setName( "RuntimeTarget" );
    retained->addChild( child );
    auto target = child->addComponent<CameraTarget>();
    BOOST_REQUIRE( target );
    target->getHandle()->setUUID( StringUtil::getUUID() );
    retained->setPerpetual( true, false );
    guard.scene->addActor( retained );
    guard.scene->addActor( ordinary );

    auto duplicate = workphone::dynamic_pointer_cast<Properties>( retained->toData() );
    BOOST_REQUIRE( duplicate );
    duplicate->setName( ApplicationUtil::actorsStr );
    duplicate->setProperty( GameActorUtil::labelStr, "DiskCopyMustNotReplaceRuntime" );
    duplicate->setProperty( "perpetual", false );
    auto follower = actorData( "NewFollower" );
    auto followData = make_ptr<Properties>();
    followData->setName( GameActorUtil::componentStr );
    followData->setProperty( GameActorUtil::componentTypeStr, "CameraFollow" );
    followData->setProperty( CameraFollow::followObjectStr, child->getHandle()->getUUIDAsString() );
    followData->setProperty( CameraFollow::targetStr, target->getHandle()->getUUIDAsString() );
    follower->addChild( followData );
    auto data = make_ptr<Properties>();
    data->addChild( duplicate );
    data->addChild( follower );

    guard.applicationManager->setPlaying( true );
    guard.scene->setState( IGameScene::State::Play );
    guard.scene->clear( false );
    guard.scene->loadSceneDataStr( DataUtil::toString( data.get() ), true );
    drainLoad( guard );

    BOOST_REQUIRE( guard.scene->getSceneLoadingState() == IGameScene::SceneLoadingState::Loaded );
    BOOST_CHECK_EQUAL( guard.scene->getActors().size(), 2u );
    BOOST_CHECK( guard.scene->findActorByName( "RuntimeRig" ) == retained );
    BOOST_CHECK( !guard.scene->findActorByName( "DiskCopyMustNotReplaceRuntime" ) );
    BOOST_CHECK( retained->getPerpetual() );
    BOOST_REQUIRE_EQUAL( retained->getChildren().size(), 1u );
    BOOST_CHECK( retained->getChildren()[0] == child );
    BOOST_CHECK( child->getComponent<CameraTarget>() == target );
    auto newFollower = guard.scene->findActorByName( "NewFollower" );
    BOOST_REQUIRE( newFollower );
    auto follow = newFollower->getComponent<CameraFollow>();
    BOOST_REQUIRE( follow );
    BOOST_CHECK( follow->getFollowObject() == child );
    BOOST_CHECK( follow->getTarget() == target );
    BOOST_CHECK( ordinary->getLoadingState() == LoadingState::Unloaded );
}

BOOST_AUTO_TEST_CASE( deferred_edit_scene_load_still_replaces_perpetual_roots )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );
    const auto wasPlaying = guard.applicationManager->isPlaying();
    guard.applicationManager->setPlaying( false );
    guard.addCleanup( [&guard, wasPlaying] {
        guard.applicationManager->setPlaying( false );
        guard.scene->setState( IGameScene::State::Edit );
        guard.scene->clear( true );
        guard.applicationManager->setPlaying( wasPlaying );
    } );
    resetScene( guard );
    auto old = guard.createBasicActor();
    BOOST_REQUIRE( old );
    old->setName( "OldEditRig" );
    old->setPerpetual( true, false );
    guard.scene->addActor( old );
    auto data = make_ptr<Properties>();
    data->addChild( actorData( "NewEditTerrain", old->getHandle()->getUUIDAsString() ) );
    guard.scene->clear( false );
    guard.scene->loadSceneDataStr( DataUtil::toString( data.get() ), true );
    drainLoad( guard );

    BOOST_REQUIRE( guard.scene->getSceneLoadingState() == IGameScene::SceneLoadingState::Loaded );
    BOOST_REQUIRE_EQUAL( guard.scene->getActors().size(), 1u );
    BOOST_CHECK( guard.scene->getActors()[0] != old );
    BOOST_CHECK_EQUAL( guard.scene->getActors()[0]->getName(), "NewEditTerrain" );
    BOOST_CHECK( old->getLoadingState() == LoadingState::Unloaded );
    BOOST_CHECK( !guard.scene->getActors()[0]->getPerpetual() );
}

BOOST_AUTO_TEST_SUITE_END()
