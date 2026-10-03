#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    const String TestScenePath = "Tests/ui_test.fbscene";
    const String PhysicsCameraBugScenePath = "Tests/physics_camera_bug.fbscene";

    void drainQueuedWork( TestGuard &guard, u32 iterations = 10 )
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

    void requireUiSceneLoaded( SmartPtr<IGameScene> scene )
    {
        BOOST_REQUIRE( scene );
        BOOST_CHECK( scene->getSceneLoadingState() == IGameScene::SceneLoadingState::Loaded );

        auto actors = scene->getActors();
        BOOST_CHECK( !actors.empty() );

        const auto &updateObjects =
            scene->getRegisteredObjects( Thread::UpdateState::Update, TaskId::Application );
        for( const auto &actor : actors )
        {
            BOOST_CHECK( std::find( updateObjects.begin(), updateObjects.end(), actor ) !=
                         updateObjects.end() );
        }

        auto canvasActor = scene->findActorByName( "Canvas" );
        BOOST_REQUIRE( canvasActor );
        BOOST_CHECK( canvasActor->getScene() == scene );

        auto panelActor = canvasActor->findChildByName( "Panel" );
        BOOST_CHECK( panelActor );

        auto dropdownActor = canvasActor->findChildByName( "Dropdown" );
        BOOST_CHECK( dropdownActor );
    }
}  // namespace

BOOST_AUTO_TEST_SUITE( GameSceneLoadingTests )

BOOST_AUTO_TEST_CASE( gamescene_load_scene_from_file_synchronously_populates_actors )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );

    guard.scene->setState( IGameScene::State::Edit );
    guard.scene->clear( true );

    guard.scene->loadScene( TestScenePath, false );
    drainQueuedWork( guard );

    requireUiSceneLoaded( guard.scene );
    BOOST_CHECK_EQUAL( guard.scene->getLabel(), "ui_test" );
    BOOST_CHECK( guard.scene->getFilePath().find( "ui_test.fbscene" ) != String::npos );
}

BOOST_AUTO_TEST_CASE( gamescene_load_scene_twice_replaces_cleared_scene_contents )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );

    guard.scene->setState( IGameScene::State::Edit );
    guard.scene->clear( true );

    guard.scene->loadScene( TestScenePath, false );
    drainQueuedWork( guard );

    auto firstLoadCount = guard.scene->getActors().size();
    BOOST_REQUIRE_GT( firstLoadCount, 0u );

    guard.scene->clear( true );
    BOOST_CHECK( guard.scene->getActors().empty() );

    guard.scene->loadScene( TestScenePath, false );
    drainQueuedWork( guard );

    requireUiSceneLoaded( guard.scene );
    BOOST_CHECK_EQUAL( guard.scene->getActors().size(), firstLoadCount );
}

BOOST_AUTO_TEST_CASE( gamescene_load_binary_scene_synchronously_populates_actors )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );

    const auto binaryScenePath = Path::getWorkingDirectory() + "/gamescene_loading_test.fbscenebin";
    guard.trackFilesystemPath( binaryScenePath );

    const auto sourceScenePath =
        Path::lexically_normal( guard.applicationManager->getMediaPath(), TestScenePath );
    auto sourceStream = guard.fileSystem->open( sourceScenePath );
    BOOST_REQUIRE( sourceStream );
    auto sourceProperties = make_ptr<Properties>();
    DataUtil::parse( sourceStream->getAsString(), sourceProperties.get(), DataFormat::JSON );
    auto binaryData = PropertiesBinarySerializer::serialize( *sourceProperties );
    guard.fileSystem->writeAllBytes( binaryScenePath, std::move( binaryData ) );

    guard.scene->setState( IGameScene::State::Edit );
    guard.scene->clear( true );
    BOOST_CHECK( guard.scene->getActors().empty() );

    guard.scene->loadScene( binaryScenePath, false );
    drainQueuedWork( guard );

    requireUiSceneLoaded( guard.scene );
    BOOST_CHECK_EQUAL( guard.scene->getLabel(), "gamescene_loading_test" );
}

BOOST_AUTO_TEST_CASE( gamescene_loaded_rigidbodies_are_registered_for_scene_updates )
{
    TestGuard guard( true );
    if( !guard.isAvailable )
    {
        return;
    }

    guard.scene->setState( IGameScene::State::Edit );
    guard.scene->clear( true );

    guard.scene->loadScene( PhysicsCameraBugScenePath, false );
    drainQueuedWork( guard );

    auto rigidbodies = guard.scene->getComponents<Rigidbody>();
    BOOST_REQUIRE( !rigidbodies.empty() );

    const auto &updateObjects =
        guard.scene->getRegisteredObjects( Thread::UpdateState::Update, TaskId::Application );
    for( const auto &rigidbody : rigidbodies )
    {
        BOOST_REQUIRE( rigidbody );
        auto actor = rigidbody->getActor();
        BOOST_REQUIRE( actor );
        BOOST_CHECK( std::find( updateObjects.begin(), updateObjects.end(), actor ) !=
                     updateObjects.end() );
    }
}

BOOST_AUTO_TEST_SUITE_END()
