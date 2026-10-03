#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    const String AsyncTestScenePath = "Tests/ui_test.fbscene";

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

BOOST_AUTO_TEST_SUITE_END()
