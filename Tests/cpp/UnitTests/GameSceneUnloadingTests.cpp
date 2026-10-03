#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    const String UnloadTestScenePath = "Tests/ui_test.fbscene";

    void drainSceneJobs( TestGuard &guard, u32 iterations = 10 )
    {
        auto jobQueue = guard.applicationManager->getJobQueue();
        BOOST_REQUIRE( jobQueue );

        for( u32 i = 0; i < iterations; ++i )
        {
            jobQueue->update();
            guard.taskManager->update();
            guard.runUpdateCycle( 1 );
        }
    }

    void loadUiSceneForUnloadTest( TestGuard &guard )
    {
        guard.scene->setState( IGameScene::State::Edit );
        guard.scene->clear( true );
        guard.scene->loadScene( UnloadTestScenePath, false );
        drainSceneJobs( guard );

        BOOST_REQUIRE( guard.scene->findActorByName( "Canvas" ) );
        BOOST_REQUIRE( !guard.scene->getActors().empty() );
    }
}  // namespace

BOOST_AUTO_TEST_SUITE( GameSceneUnloadingTests )

BOOST_AUTO_TEST_CASE( gamescene_clear_unloads_loaded_actors_immediately )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );

    loadUiSceneForUnloadTest( guard );

    guard.scene->clear( true );

    BOOST_CHECK( guard.scene->getActors().empty() );
    BOOST_CHECK( guard.scene->findActorByName( "Canvas" ) == nullptr );
    BOOST_CHECK_EQUAL( guard.scene->getLabel(), "Untitled" );
}

BOOST_AUTO_TEST_CASE( gamescene_deferred_clear_unloads_after_application_jobs_run )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );

    loadUiSceneForUnloadTest( guard );

    guard.scene->clear( false );
    BOOST_CHECK( !guard.scene->getActors().empty() );

    drainSceneJobs( guard );

    BOOST_CHECK( guard.scene->getActors().empty() );
    BOOST_CHECK( guard.scene->findActorByName( "Canvas" ) == nullptr );
}

BOOST_AUTO_TEST_CASE( gamescene_remove_all_actors_detaches_loaded_scene_contents )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );

    loadUiSceneForUnloadTest( guard );

    guard.scene->removeAllActors();

    BOOST_CHECK( guard.scene->getActors().empty() );
    BOOST_CHECK( guard.scene->findActorByName( "Canvas" ) == nullptr );
}

BOOST_AUTO_TEST_SUITE_END()
