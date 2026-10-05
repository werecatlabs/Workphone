#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Jobs/ActorLoadJob.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Scene/Components/Camera/CameraFollow.hpp>
#include <Workphone/Scene/Components/Camera/CameraTarget.hpp>
#include <boost/test/unit_test.hpp>
#include <chrono>
#include <thread>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    const String AsyncTestScenePath = "Tests/ui_test.fbscene";

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

    void drainLoad( TestGuard &guard )
    {
        auto queue = guard.applicationManager->getJobQueue();
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
        do
        {
            queue->update();
            guard.taskManager->update();
            if( !queue->hasJobs() )
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

BOOST_AUTO_TEST_CASE( gamescene_async_file_load_completes_through_job_queue )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );

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

BOOST_AUTO_TEST_SUITE_END()
