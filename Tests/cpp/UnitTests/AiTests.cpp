#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <cstdlib>
#include <filesystem>

#if WP_ENABLE_LUA
extern "C" {
#    include <lauxlib.h>
#    include <lualib.h>
}
#endif

namespace
{
    using namespace workphone;

    struct ApplicationStateGuard
    {
        explicit ApplicationStateGuard( core::IApplicationManager *manager ) :
            applicationManager( manager ),
            playing( manager->isPlaying() ),
            paused( manager->isPaused() ),
            running( manager->isRunning() ),
            rendererEnabled( manager->getEnableRenderer() ),
            editor( manager->isEditor() ),
            editorCamera( manager->isEditorCamera() ),
            pauseMenuActive( manager->isPauseMenuActive() ),
            quit( manager->getQuit() )
        {
        }

        ~ApplicationStateGuard()
        {
            applicationManager->setPlaying( playing );
            applicationManager->setPaused( paused );
            applicationManager->setRunning( running );
            applicationManager->setEnableRenderer( rendererEnabled );
            applicationManager->setEditor( editor );
            applicationManager->setEditorCamera( editorCamera );
            applicationManager->setPauseMenuActive( pauseMenuActive );
            applicationManager->setQuit( quit );
        }

        core::IApplicationManager *applicationManager;
        bool playing;
        bool paused;
        bool running;
        bool rendererEnabled;
        bool editor;
        bool editorCamera;
        bool pauseMenuActive;
        bool quit;
    };

    void checkVector( const Vector3<real_Num> &actual, const Vector3<real_Num> &expected )
    {
        constexpr auto tolerance = 0.001;
        BOOST_CHECK_SMALL( static_cast<double>( actual.x - expected.x ), tolerance );
        BOOST_CHECK_SMALL( static_cast<double>( actual.y - expected.y ), tolerance );
        BOOST_CHECK_SMALL( static_cast<double>( actual.z - expected.z ), tolerance );
    }

    struct TemporaryDirectoryGuard
    {
        explicit TemporaryDirectoryGuard( std::filesystem::path path ) : directory( std::move( path ) )
        {
            std::error_code error;
            std::filesystem::remove_all( directory, error );
        }

        ~TemporaryDirectoryGuard()
        {
            std::error_code error;
            std::filesystem::remove_all( directory, error );
        }

        std::filesystem::path directory;
    };

#if WP_ENABLE_LUA
    struct RawLuaStateGuard
    {
        RawLuaStateGuard() : state( luaL_newstate() )
        {
        }

        ~RawLuaStateGuard()
        {
            if( state )
            {
                lua_close( state );
            }
        }

        lua_State *state = nullptr;
    };
#endif

    bool runAiIntegrationTests()
    {
        return std::getenv( "WP_RUN_AI_INTEGRATION_TESTS" ) != nullptr;
    }
}  // namespace

BOOST_AUTO_TEST_SUITE( AiResponseActions )

BOOST_AUTO_TEST_CASE( rejects_malformed_unknown_and_unconfirmed_actions )
{
    using namespace workphone;

    auto applicationManager = core::IApplicationManager::instancePtr();
    BOOST_REQUIRE( applicationManager );

    auto aiManager = applicationManager->getAiManager();
    BOOST_REQUIRE( aiManager );

    ApplicationStateGuard stateGuard( applicationManager );

    BOOST_CHECK( !aiManager->processResponse( "" ) );
    BOOST_CHECK( !aiManager->processResponse( "not json" ) );
    BOOST_CHECK( !aiManager->processResponse( R"({"actions":"invalid"})" ) );
    BOOST_CHECK(
        !aiManager->processResponse( R"({"actions":[{"name":"execute_shell","value":"format"}]})" ) );
    BOOST_CHECK( !aiManager->processResponse( R"({"actions":[{"name":"request_quit"}]})" ) );
    BOOST_CHECK( !applicationManager->getQuit() );
    BOOST_CHECK(
        !aiManager->processResponse( R"({"actions":[{"name":"request_quit","confirm":"true"}]})" ) );
    BOOST_CHECK( !applicationManager->getQuit() );

    BOOST_CHECK(
        aiManager->processResponse( R"({"actions":[{"name":"request_quit","confirm":true}]})" ) );
    BOOST_CHECK( applicationManager->getQuit() );
}

BOOST_AUTO_TEST_CASE( controls_application_lifecycle_and_display_state )
{
    using namespace workphone;

    auto applicationManager = core::IApplicationManager::instancePtr();
    BOOST_REQUIRE( applicationManager );

    auto aiManager = applicationManager->getAiManager();
    BOOST_REQUIRE( aiManager );

    ApplicationStateGuard stateGuard( applicationManager );

    const auto response = R"(
        Here is the requested result:
        ```json
        {
          "message": "Runtime configured.",
          "actions": [
            {"name":"play"},
            {"name":"pause"},
            {"name":"set_running","value":true},
            {"name":"set_renderer_enabled","value":false},
            {"name":"set_editor_mode","value":true},
            {"name":"set_editor_camera","value":true},
            {"name":"set_pause_menu_active","value":true}
          ]
        }
        ```
    )";

    BOOST_CHECK( aiManager->processResponse( response ) );
    BOOST_CHECK( applicationManager->isPlaying() );
    BOOST_CHECK( applicationManager->isPaused() );
    BOOST_CHECK( applicationManager->isRunning() );
    BOOST_CHECK( !applicationManager->getEnableRenderer() );
    BOOST_CHECK( applicationManager->isEditor() );
    BOOST_CHECK( applicationManager->isEditorCamera() );
    BOOST_CHECK( applicationManager->isPauseMenuActive() );

    BOOST_CHECK(
        !aiManager->processResponse( R"({"actions":[{"name":"set_running","value":false}]})" ) );
    BOOST_CHECK( applicationManager->isRunning() );
    BOOST_CHECK( aiManager->processResponse(
        R"({"actions":[{"name":"set_running","value":false,"confirm":true}]})" ) );
    BOOST_CHECK( !applicationManager->isRunning() );
    applicationManager->setRunning( true );

    BOOST_CHECK( aiManager->processResponse( R"({"actions":[{"name":"resume"},{"name":"stop"}]})" ) );
    BOOST_CHECK( !applicationManager->isPaused() );
    BOOST_CHECK( !applicationManager->isPlaying() );
}

BOOST_AUTO_TEST_CASE( controls_timing_and_audio_with_validated_ranges )
{
    using namespace workphone;

    auto applicationManager = core::IApplicationManager::instancePtr();
    BOOST_REQUIRE( applicationManager );

    auto aiManager = applicationManager->getAiManager();
    auto timer = applicationManager->getTimer();
    BOOST_REQUIRE( aiManager );
    BOOST_REQUIRE( timer );

    const auto originalFixedTimestep = timer->getFixedTimeInterval();
    BOOST_CHECK(
        aiManager->processResponse( R"({"actions":[{"name":"set_fixed_timestep","value":0.02}]})" ) );
    BOOST_CHECK_CLOSE( timer->getFixedTimeInterval(), 0.02, 0.001 );
    BOOST_CHECK(
        !aiManager->processResponse( R"({"actions":[{"name":"set_fixed_timestep","value":0}]})" ) );
    timer->setFixedTimeInterval( originalFixedTimestep );

    if( auto soundManager = applicationManager->getSoundManager() )
    {
        const auto originalVolume = soundManager->getVolume();
        BOOST_CHECK(
            aiManager->processResponse( R"({"actions":[{"name":"set_master_volume","value":0.35}]})" ) );
        BOOST_CHECK_CLOSE( soundManager->getVolume(), 0.35f, 0.01 );
        BOOST_CHECK(
            !aiManager->processResponse( R"({"actions":[{"name":"set_master_volume","value":1.5}]})" ) );
        soundManager->setVolume( originalVolume );
    }
}

BOOST_AUTO_TEST_CASE( controls_scene_metadata_and_state )
{
    using namespace workphone;

    auto applicationManager = core::IApplicationManager::instancePtr();
    BOOST_REQUIRE( applicationManager );

    auto aiManager = applicationManager->getAiManager();
    auto gameManager = applicationManager->getGameManager();
    auto scene = gameManager ? gameManager->getCurrentScene() : nullptr;
    BOOST_REQUIRE( aiManager );
    BOOST_REQUIRE( scene );

    const auto originalLabel = scene->getLabel();
    const auto originalState = scene->getState();

    BOOST_CHECK( aiManager->processResponse(
        R"({"actions":[{"name":"set_scene_label","value":"AI Production Scene"},)"
        R"({"name":"set_scene_state","value":"play"}]})" ) );
    BOOST_CHECK_EQUAL( scene->getLabel(), "AI Production Scene" );
    BOOST_CHECK( scene->getState() == scene::IGameScene::State::Play );

    BOOST_CHECK(
        !aiManager->processResponse( R"({"actions":[{"name":"set_scene_state","value":"invalid"}]})" ) );
    BOOST_CHECK( !aiManager->processResponse( R"({"actions":[{"name":"clear_scene"}]})" ) );

    scene->setLabel( originalLabel );
    scene->setState( originalState );
}

BOOST_AUTO_TEST_CASE( creates_and_manipulates_actor )
{
    using namespace workphone;

    auto applicationManager = core::IApplicationManager::instancePtr();
    BOOST_REQUIRE( applicationManager );

    auto aiManager = applicationManager->getAiManager();
    auto gameManager = applicationManager->getGameManager();
    auto scene = gameManager ? gameManager->getCurrentScene() : nullptr;
    BOOST_REQUIRE( aiManager );
    BOOST_REQUIRE( gameManager );
    BOOST_REQUIRE( scene );

    const auto actorName = String( "AiActionProductionActor" );
    const auto renamedActorName = String( "AiActionProductionActorRenamed" );

    if( auto existingActor = scene->findActorByName( actorName ) )
    {
        gameManager->destroyActor( existingActor );
    }
    if( auto existingActor = scene->findActorByName( renamedActorName ) )
    {
        gameManager->destroyActor( existingActor );
    }

    BOOST_CHECK( aiManager->processResponse(
        R"({"actions":[{"name":"create_actor","actor":"AiActionProductionActor",)"
        R"("position":[1,2,3],"rotation":[0.1,0.2,0.3],"scale":[2,3,4],)"
        R"("enabled":true,"visible":true,"static":false,"smooth_motion":true}]})" ) );

    auto actor = scene->findActorByName( actorName );
    BOOST_REQUIRE( actor );
    BOOST_CHECK( actor->isEnabled() );
    BOOST_CHECK( actor->isVisible() );
    BOOST_CHECK( !actor->isStatic() );
    BOOST_CHECK( actor->isSmoothMotion() );

    auto transform = actor->getTransform();
    BOOST_REQUIRE( transform );
    checkVector( transform->getPosition(), Vector3<real_Num>( 1, 2, 3 ) );
    checkVector( transform->getRotation(), Vector3<real_Num>( 0.1, 0.2, 0.3 ) );
    checkVector( transform->getScale(), Vector3<real_Num>( 2, 3, 4 ) );

    BOOST_CHECK( !aiManager->processResponse(
        R"({"actions":[{"name":"create_actor","actor":"AiActionProductionActor"}]})" ) );

    BOOST_CHECK( aiManager->processResponse(
        R"({"actions":[)"
        R"({"name":"translate_actor","actor":"AiActionProductionActor","value":[4,-2,1]},)"
        R"({"name":"rotate_actor","actor":"AiActionProductionActor","value":[0.1,0.1,0.1]},)"
        R"({"name":"set_actor_scale","actor":"AiActionProductionActor","value":{"x":1,"y":1.5,"z":2}},)"
        R"({"name":"set_actor_enabled","actor":"AiActionProductionActor","value":false},)"
        R"({"name":"set_actor_visible","actor":"AiActionProductionActor","value":false},)"
        R"({"name":"set_actor_collision_mask","actor":"AiActionProductionActor","value":42})"
        R"(]})" ) );

    checkVector( transform->getPosition(), Vector3<real_Num>( 5, 0, 4 ) );
    checkVector( transform->getRotation(), Vector3<real_Num>( 0.2, 0.3, 0.4 ) );
    checkVector( transform->getScale(), Vector3<real_Num>( 1, 1.5, 2 ) );
    BOOST_CHECK( !actor->isEnabled() );
    BOOST_CHECK( !actor->isVisible() );
    BOOST_CHECK_EQUAL( actor->getCollisionMask(), 42u );

    BOOST_CHECK( aiManager->processResponse(
        R"({"actions":[{"name":"rename_actor","actor":"AiActionProductionActor",)"
        R"("value":"AiActionProductionActorRenamed"}]})" ) );
    BOOST_CHECK( !scene->findActorByName( actorName ) );
    BOOST_CHECK( scene->findActorByName( renamedActorName ) == actor );

    BOOST_CHECK( !aiManager->processResponse(
        R"({"actions":[{"name":"destroy_actor","actor":"AiActionProductionActorRenamed"}]})" ) );
    BOOST_CHECK( scene->findActorByName( renamedActorName ) == actor );

    BOOST_CHECK( aiManager->processResponse(
        R"({"actions":[{"name":"destroy_actor","actor":"AiActionProductionActorRenamed",)"
        R"("confirm":true}]})" ) );
    BOOST_CHECK( !scene->findActorByName( renamedActorName ) );
}

BOOST_AUTO_TEST_CASE( rejects_invalid_actor_data_without_mutating_actor )
{
    using namespace workphone;

    auto applicationManager = core::IApplicationManager::instancePtr();
    BOOST_REQUIRE( applicationManager );

    auto aiManager = applicationManager->getAiManager();
    auto gameManager = applicationManager->getGameManager();
    auto scene = gameManager ? gameManager->getCurrentScene() : nullptr;
    BOOST_REQUIRE( aiManager );
    BOOST_REQUIRE( gameManager );
    BOOST_REQUIRE( scene );

    const auto actorName = String( "AiActionValidationActor" );
    if( auto existingActor = scene->findActorByName( actorName ) )
    {
        gameManager->destroyActor( existingActor );
    }

    BOOST_CHECK( aiManager->processResponse(
        R"({"actions":[{"name":"create_actor","actor":"AiActionValidationActor"}]})" ) );
    auto actor = scene->findActorByName( actorName );
    BOOST_REQUIRE( actor );
    auto transform = actor->getTransform();
    BOOST_REQUIRE( transform );
    const auto originalPosition = transform->getPosition();

    BOOST_CHECK( !aiManager->processResponse(
        R"({"actions":[{"name":"set_actor_position","actor":"AiActionValidationActor",)"
        R"("value":[1,2]}]})" ) );
    BOOST_CHECK( !aiManager->processResponse(
        R"({"actions":[{"name":"set_actor_scale","actor":"AiActionValidationActor",)"
        R"("value":[0,1,1]}]})" ) );
    BOOST_CHECK( !aiManager->processResponse(
        R"({"actions":[{"name":"set_actor_position","actor":"AiActionValidationActor",)"
        R"("value":[1000001,0,0]}]})" ) );
    checkVector( transform->getPosition(), originalPosition );

    BOOST_CHECK( aiManager->processResponse(
        R"({"actions":[{"name":"destroy_actor","actor":"AiActionValidationActor",)"
        R"("confirm":true}]})" ) );
}

BOOST_AUTO_TEST_CASE( creates_media_folder_and_project )
{
    using namespace workphone;

    auto applicationManager = core::IApplicationManager::instancePtr();
    BOOST_REQUIRE( applicationManager );

    auto aiManager = applicationManager->getAiManager();
    BOOST_REQUIRE( aiManager );

    std::error_code error;
    const auto mediaPath = applicationManager->getMediaPath();
    const auto mediaRoot = std::filesystem::weakly_canonical(
        std::filesystem::absolute(
            std::filesystem::u8path( std::string( mediaPath.data(), mediaPath.size() ) ), error ),
        error );
    BOOST_REQUIRE( !error );
    BOOST_REQUIRE( std::filesystem::is_directory( mediaRoot ) );

    const auto testRoot = mediaRoot / "Tests" / "AiActionFileSystem";
    TemporaryDirectoryGuard directoryGuard( testRoot );

    BOOST_CHECK(
        aiManager->processResponse( R"({"actions":[{"name":"create_folder",)"
                                    R"("path":"Tests/AiActionFileSystem/Content/Levels"}]})" ) );
    BOOST_CHECK( std::filesystem::is_directory( testRoot / "Content" / "Levels" ) );

    BOOST_CHECK( aiManager->processResponse(
        R"({"actions":[{"name":"create_project","path":"Tests/AiActionFileSystem/CatchGame",)"
        R"("product_name":"Catch Game Test","company_name":"Lioncat Tests",)"
        R"("current_scene":"Assets/Scenes/CatchGame.fbscene"}]})" ) );

    const auto projectRoot = testRoot / "CatchGame";
    const auto projectFile = projectRoot / "project.fbproject";
    BOOST_CHECK( std::filesystem::is_regular_file( projectFile ) );
    for( const auto *folder : { "Assets", "Assets/Scenes", "Assets/Scripts", "Cache", "Engine", "Plugin",
                                "Scripts", "SettingsCache" } )
    {
        BOOST_CHECK( std::filesystem::is_directory( projectRoot / folder ) );
    }

    const auto projectText = Path::readAllText( projectFile.string() );
    BOOST_CHECK( projectText.find( "\"productName\":\"Catch Game Test\"" ) != String::npos );
    BOOST_CHECK( projectText.find( "\"applicationType\":\"lua\"" ) != String::npos );

    BOOST_CHECK( aiManager->processResponse( R"({"actions":[{"name":"create_project",)"
                                             R"("path":"Tests/AiActionFileSystem/CatchGame"}]})" ) );

    BOOST_CHECK( aiManager->processResponse(
        R"({"actions":[{"name":"create_project",)"
        R"("path":"Tests/AiActionFileSystem/HelloWorld","product_name":"Hello World",)"
        R"("company_name":"","current_scene":"","template":"hello_world_ui"}]})" ) );
    const auto helloWorldRoot = testRoot / "HelloWorld";
    const auto helloWorldProject =
        Path::readAllText( ( helloWorldRoot / "project.fbproject" ).string() );
    const auto helloWorldScript =
        Path::readAllText( ( helloWorldRoot / "Scripts" / "Main.lua" ).string() );
    BOOST_CHECK( helloWorldProject.find( "\"applicationType\":\"lua\"" ) != String::npos );
    BOOST_CHECK( helloWorldProject.find( "Scripts/Main.lua" ) != String::npos );
    BOOST_CHECK( helloWorldScript.find( "getCurrentScene" ) != String::npos );
    BOOST_CHECK( helloWorldScript.find( "scene:addActor" ) != String::npos );
    BOOST_CHECK( helloWorldScript.find( "addComponent(\"LayoutTransform\")" ) != String::npos );
    BOOST_CHECK( helloWorldScript.find( "addComponent(\"Text\")" ) != String::npos );
    BOOST_CHECK( helloWorldScript.find( "setText(\"Hello World!\")" ) != String::npos );

    BOOST_CHECK( !aiManager->processResponse(
        R"({"actions":[{"name":"create_project",)"
        R"("path":"Tests/AiActionFileSystem/InvalidTemplate","template":"arbitrary_code"}]})" ) );
    BOOST_CHECK( !aiManager->processResponse(
        R"({"actions":[{"name":"create_folder","path":"../OutsideMedia"}]})" ) );
    BOOST_CHECK( !aiManager->processResponse(
        R"({"actions":[{"name":"create_project","path":"C:/OutsideMedia"}]})" ) );
    BOOST_CHECK( !std::filesystem::exists( mediaRoot.parent_path() / "OutsideMedia" ) );
}

BOOST_AUTO_TEST_CASE( simulated_responses_create_cube_catch_game_project_from_scratch )
{
    using namespace workphone;

    auto applicationManager = core::IApplicationManager::instancePtr();
    BOOST_REQUIRE( applicationManager );

    auto aiManager = applicationManager->getAiManager();
    BOOST_REQUIRE( aiManager );

    std::error_code error;
    const auto mediaPath = applicationManager->getMediaPath();
    const auto mediaRoot = std::filesystem::weakly_canonical(
        std::filesystem::absolute(
            std::filesystem::u8path( std::string( mediaPath.data(), mediaPath.size() ) ), error ),
        error );
    BOOST_REQUIRE( !error );
    BOOST_REQUIRE( std::filesystem::is_directory( mediaRoot ) );
    BOOST_REQUIRE( std::filesystem::is_regular_file( mediaRoot / "cube_internal.fbmeshbin" ) );
    BOOST_REQUIRE( std::filesystem::is_regular_file( mediaRoot / "Standard.mat" ) );

    const auto testRoot = mediaRoot / "Tests" / "AiSimulatedCatchGame";
    const auto projectRoot = testRoot / "CubeCatch";
    TemporaryDirectoryGuard directoryGuard( testRoot );

    const auto simulatedPlanningResponse = R"(
        I will prepare a dedicated project location.
        ```json
        {
          "message": "Catch game project location prepared.",
          "actions": [
            {
              "name": "create_folder",
              "path": "Tests/AiSimulatedCatchGame"
            }
          ]
        }
        ```
    )";
    BOOST_REQUIRE( aiManager->processResponse( simulatedPlanningResponse ) );
    BOOST_REQUIRE( std::filesystem::is_directory( testRoot ) );

    const auto simulatedCreationResponse = R"(
        ```json
        {
          "message": "Created a playable Lua cube catch game from scratch.",
          "actions": [
            {
              "name": "create_project",
              "path": "Tests/AiSimulatedCatchGame/CubeCatch",
              "product_name": "Cube Catch",
              "company_name": "Lioncat Tests",
              "current_scene": "",
              "template": "catch_game_3d"
            }
          ]
        }
        ```
    )";
    BOOST_REQUIRE( aiManager->processResponse( simulatedCreationResponse ) );

    const auto projectFile = projectRoot / "project.fbproject";
    const auto scriptFile = projectRoot / "Scripts" / "Main.lua";
    BOOST_REQUIRE( std::filesystem::is_regular_file( projectFile ) );
    BOOST_REQUIRE( std::filesystem::is_regular_file( scriptFile ) );
    for( const auto *folder : { "Assets", "Assets/Scenes", "Assets/Scripts", "Cache", "Engine", "Plugin",
                                "Scripts", "SettingsCache" } )
    {
        BOOST_CHECK( std::filesystem::is_directory( projectRoot / folder ) );
    }

    const auto projectText = Path::readAllText( projectFile.string() );
    BOOST_CHECK( projectText.find( "\"productName\":\"Cube Catch\"" ) != String::npos );
    BOOST_CHECK( projectText.find( "\"companyName\":\"Lioncat Tests\"" ) != String::npos );
    BOOST_CHECK( projectText.find( "\"applicationType\":\"lua\"" ) != String::npos );
    BOOST_CHECK( projectText.find( "\"scriptFilePaths\":[\"Scripts/Main.lua\"]" ) != String::npos );

    const auto scriptText = Path::readAllText( scriptFile.string() );
    for( const auto *requiredText :
         { "class 'CatchGameMain' (BaseComponent)", "addComponent(\"Mesh\")",
           "setMeshPath(\"cube_internal.fbmeshbin\")", "addComponent(\"MeshRenderer\")",
           "setMaterialPath(\"Standard.mat\")", "getAxisValue(0)", "function CatchGameMain:spawnItem",
           "function CatchGameMain:isCaught", "self.score = self.score + 1",
           "self.misses = self.misses + 1", "gameOver", "UserComponent.typeInfo()",
           "scene:addActor(actor)", "launchCatchGame3D()" } )
    {
        BOOST_CHECK_MESSAGE( scriptText.find( requiredText ) != String::npos,
                             "Generated catch game script is missing: " << requiredText );
    }

#if WP_ENABLE_LUA
    RawLuaStateGuard lua;
    BOOST_REQUIRE( lua.state );
    const auto scriptPath = scriptFile.string();
    const auto loadResult = luaL_loadfile( lua.state, scriptPath.c_str() );
    const auto luaError = loadResult == LUA_OK ? nullptr : lua_tostring( lua.state, -1 );
    BOOST_CHECK_MESSAGE( loadResult == LUA_OK, "Generated catch game Lua failed syntax validation: "
                                                   << ( luaError ? luaError : "unknown Lua error" ) );
#endif
}

BOOST_AUTO_TEST_CASE( simulated_responses_create_cube_racing_game_project_from_scratch )
{
    using namespace workphone;

    auto applicationManager = core::IApplicationManager::instancePtr();
    BOOST_REQUIRE( applicationManager );

    auto aiManager = applicationManager->getAiManager();
    BOOST_REQUIRE( aiManager );

    std::error_code error;
    const auto mediaPath = applicationManager->getMediaPath();
    const auto mediaRoot = std::filesystem::weakly_canonical(
        std::filesystem::absolute(
            std::filesystem::u8path( std::string( mediaPath.data(), mediaPath.size() ) ), error ),
        error );
    BOOST_REQUIRE( !error );
    BOOST_REQUIRE( std::filesystem::is_directory( mediaRoot ) );
    BOOST_REQUIRE( std::filesystem::is_regular_file( mediaRoot / "cube_internal.fbmeshbin" ) );
    BOOST_REQUIRE( std::filesystem::is_regular_file( mediaRoot / "Standard.mat" ) );

    const auto testRoot = mediaRoot / "Tests" / "AiSimulatedRacingGame";
    const auto projectRoot = testRoot / "CubeRacer";
    TemporaryDirectoryGuard directoryGuard( testRoot );

    const auto simulatedPlanningResponse = R"(
        The racing game needs a clean project workspace.
        ```json
        {
          "message": "Prepared the racing project workspace.",
          "actions": [
            {
              "name": "create_folder",
              "path": "Tests/AiSimulatedRacingGame"
            }
          ]
        }
        ```
    )";
    BOOST_REQUIRE( aiManager->processResponse( simulatedPlanningResponse ) );
    BOOST_REQUIRE( std::filesystem::is_directory( testRoot ) );

    const auto simulatedCreationResponse = R"(
        ```json
        {
          "message": "Created a complete Lua cube racing game from scratch.",
          "actions": [
            {
              "name": "create_project",
              "path": "Tests/AiSimulatedRacingGame/CubeRacer",
              "product_name": "Cube Racer",
              "company_name": "Lioncat Tests",
              "current_scene": "",
              "template": "racing_game_3d"
            }
          ]
        }
        ```
    )";
    BOOST_REQUIRE( aiManager->processResponse( simulatedCreationResponse ) );

    const auto projectFile = projectRoot / "project.fbproject";
    const auto scriptFile = projectRoot / "Scripts" / "Main.lua";
    BOOST_REQUIRE( std::filesystem::is_regular_file( projectFile ) );
    BOOST_REQUIRE( std::filesystem::is_regular_file( scriptFile ) );
    for( const auto *folder : { "Assets", "Assets/Scenes", "Assets/Scripts", "Cache", "Engine", "Plugin",
                                "Scripts", "SettingsCache" } )
    {
        BOOST_CHECK( std::filesystem::is_directory( projectRoot / folder ) );
    }

    const auto projectText = Path::readAllText( projectFile.string() );
    BOOST_CHECK( projectText.find( "\"productName\":\"Cube Racer\"" ) != String::npos );
    BOOST_CHECK( projectText.find( "\"companyName\":\"Lioncat Tests\"" ) != String::npos );
    BOOST_CHECK( projectText.find( "\"applicationType\":\"lua\"" ) != String::npos );
    BOOST_CHECK( projectText.find( "\"scriptFilePaths\":[\"Scripts/Main.lua\"]" ) != String::npos );

    const auto scriptText = Path::readAllText( scriptFile.string() );
    for( const auto *requiredText : { "class 'RacingGameMain' (BaseComponent)",
                                      "addComponent(\"Mesh\")",
                                      "setMeshPath(\"cube_internal.fbmeshbin\")",
                                      "addComponent(\"MeshRenderer\")",
                                      "setMaterialPath(\"Standard.mat\")",
                                      "function RacingGameMain:createRoad",
                                      "Road.Marker.",
                                      "getAxisValue(0)",
                                      "function RacingGameMain:getSpawnInterval",
                                      "function RacingGameMain:spawnTraffic",
                                      "function RacingGameMain:hasCollision",
                                      "self.distance = self.distance + self.speed * deltaTime",
                                      "self.speed = clamp(",
                                      "self.lives = self.lives - 1",
                                      "self.overtakes = self.overtakes + 1",
                                      "self.gameOver = true",
                                      "function RacingGameMain:reset",
                                      "UserComponent.typeInfo()",
                                      "scene:addActor(actor)",
                                      "launchCubeRacer()" } )
    {
        BOOST_CHECK_MESSAGE( scriptText.find( requiredText ) != String::npos,
                             "Generated racing game script is missing: " << requiredText );
    }

#if WP_ENABLE_LUA
    RawLuaStateGuard lua;
    BOOST_REQUIRE( lua.state );
    const auto scriptPath = scriptFile.string();
    const auto loadResult = luaL_loadfile( lua.state, scriptPath.c_str() );
    const auto luaError = loadResult == LUA_OK ? nullptr : lua_tostring( lua.state, -1 );
    BOOST_CHECK_MESSAGE( loadResult == LUA_OK, "Generated racing game Lua failed syntax validation: "
                                                   << ( luaError ? luaError : "unknown Lua error" ) );
#endif
}

BOOST_AUTO_TEST_CASE( simulated_responses_create_lua_flight_simulator_project_from_scratch )
{
    using namespace workphone;

    auto applicationManager = core::IApplicationManager::instancePtr();
    BOOST_REQUIRE( applicationManager );

    auto aiManager = applicationManager->getAiManager();
    BOOST_REQUIRE( aiManager );

    std::error_code error;
    const auto mediaPath = applicationManager->getMediaPath();
    const auto mediaRoot = std::filesystem::weakly_canonical(
        std::filesystem::absolute(
            std::filesystem::u8path( std::string( mediaPath.data(), mediaPath.size() ) ), error ),
        error );
    BOOST_REQUIRE( !error );
    BOOST_REQUIRE( std::filesystem::is_directory( mediaRoot ) );
    BOOST_REQUIRE( std::filesystem::is_regular_file( mediaRoot / "cube_internal.fbmeshbin" ) );
    BOOST_REQUIRE( std::filesystem::is_regular_file( mediaRoot / "Standard.mat" ) );
    BOOST_REQUIRE( std::filesystem::is_regular_file( mediaRoot / "DefaultUI.mat" ) );

    const auto testRoot = mediaRoot / "Tests" / "AiSimulatedFlightSimulator";
    const auto projectRoot = testRoot / "CubeFlight";
    TemporaryDirectoryGuard directoryGuard( testRoot );

    const auto simulatedPlanningResponse = R"(
        The flight simulator needs its own clean media-relative project workspace.
        ```json
        {
          "message": "Prepared the flight simulator workspace.",
          "actions": [
            {
              "name": "create_folder",
              "path": "Tests/AiSimulatedFlightSimulator"
            }
          ]
        }
        ```
    )";
    BOOST_REQUIRE( aiManager->processResponse( simulatedPlanningResponse ) );
    BOOST_REQUIRE( std::filesystem::is_directory( testRoot ) );

    const auto simulatedCreationResponse = R"(
        ```json
        {
          "message": "Created a non-trivial Lua flight simulator with aerodynamic physics and scene UI.",
          "actions": [
            {
              "name": "create_project",
              "path": "Tests/AiSimulatedFlightSimulator/CubeFlight",
              "product_name": "Cube Flight Simulator",
              "company_name": "Lioncat Tests",
              "current_scene": "",
              "template": "flight_simulator_3d"
            }
          ]
        }
        ```
    )";
    BOOST_REQUIRE( aiManager->processResponse( simulatedCreationResponse ) );

    const auto projectFile = projectRoot / "project.fbproject";
    const auto scriptFile = projectRoot / "Scripts" / "Main.lua";
    BOOST_REQUIRE( std::filesystem::is_regular_file( projectFile ) );
    BOOST_REQUIRE( std::filesystem::is_regular_file( scriptFile ) );
    for( const auto *folder : { "Assets", "Assets/Scenes", "Assets/Scripts", "Cache", "Engine", "Plugin",
                                "Scripts", "SettingsCache" } )
    {
        BOOST_CHECK( std::filesystem::is_directory( projectRoot / folder ) );
    }

    const auto projectText = Path::readAllText( projectFile.string() );
    BOOST_CHECK( projectText.find( "\"productName\":\"Cube Flight Simulator\"" ) != String::npos );
    BOOST_CHECK( projectText.find( "\"companyName\":\"Lioncat Tests\"" ) != String::npos );
    BOOST_CHECK( projectText.find( "\"applicationType\":\"lua\"" ) != String::npos );
    BOOST_CHECK( projectText.find( "\"scriptFilePaths\":[\"Scripts/Main.lua\"]" ) != String::npos );

    const auto scriptText = Path::readAllText( scriptFile.string() );
    for( const auto *requiredText : { "class 'FlightSimulatorMain' (BaseComponent)",
                                      "addComponent(\"Mesh\")",
                                      "setMeshPath(\"cube_internal.fbmeshbin\")",
                                      "setMaterialPath(\"Standard.mat\")",
                                      "addComponent(\"CollisionBox\")",
                                      "addComponent(\"Rigidbody\")",
                                      "addComponent(\"VehicleController\")",
                                      "setVehicleType(1)",
                                      "setAirDensity(",
                                      "setAerodynamicSectionMultiplier(",
                                      "setRollwiseDamping(",
                                      "setAircraftControls(",
                                      "setLinearVelocity(",
                                      "function FlightSimulatorMain:createCourse",
                                      "function FlightSimulatorMain:createAircraft",
                                      "function FlightSimulatorMain:readControls",
                                      "KeyCode and KeyCode.Q",
                                      "KeyCode and KeyCode.E",
                                      "function FlightSimulatorMain:updateCheckpoint",
                                      "self.checkpointsPassed",
                                      "self.laps = self.laps + 1",
                                      "STALL WARNING",
                                      "press R to restart",
                                      "addComponent(\"LayoutTransform\")",
                                      "addComponent(\"Text\")",
                                      "setMaterialPath(\"DefaultUI.mat\")",
                                      "FlightSimulator.HUD",
                                      "scene:addActor(actor)",
                                      "UserComponent.typeInfo()",
                                      "launchFlightSimulator()" } )
    {
        BOOST_CHECK_MESSAGE( scriptText.find( requiredText ) != String::npos,
                             "Generated flight simulator script is missing: " << requiredText );
    }

#if WP_ENABLE_LUA
    RawLuaStateGuard lua;
    BOOST_REQUIRE( lua.state );
    const auto scriptPath = scriptFile.string();
    const auto loadResult = luaL_loadfile( lua.state, scriptPath.c_str() );
    const auto luaError = loadResult == LUA_OK ? nullptr : lua_tostring( lua.state, -1 );
    BOOST_CHECK_MESSAGE( loadResult == LUA_OK,
                         "Generated flight simulator Lua failed syntax validation: "
                             << ( luaError ? luaError : "unknown Lua error" ) );
#endif
}

BOOST_AUTO_TEST_CASE( query_ai_creates_lua_hello_world_scene_ui_project )
{
    using namespace workphone;

    if( !runAiIntegrationTests() )
    {
        BOOST_TEST_MESSAGE(
            "Skipping AI query integration test; set WP_RUN_AI_INTEGRATION_TESTS=1 to enable." );
        return;
    }

    auto applicationManager = core::IApplicationManager::instancePtr();
    BOOST_REQUIRE( applicationManager );

    auto aiManager = applicationManager->getAiManager();
    BOOST_REQUIRE( aiManager );

    std::error_code error;
    const auto mediaPath = applicationManager->getMediaPath();
    const auto mediaRoot = std::filesystem::weakly_canonical(
        std::filesystem::absolute(
            std::filesystem::u8path( std::string( mediaPath.data(), mediaPath.size() ) ), error ),
        error );
    BOOST_REQUIRE( !error );

    const auto projectRoot = mediaRoot / "Tests" / "AiHelloWorldQuery";
    TemporaryDirectoryGuard directoryGuard( projectRoot );

    const auto response = aiManager->query(
        "Create a Lua Hello World project at the media-relative path "
        "Tests/AiHelloWorldQuery using the game scene UI. Use exactly one create_project action "
        "with product_name Hello World AI and template hello_world_ui. Do not use any other "
        "action." );
    BOOST_TEST_CONTEXT( "AI response: " << response )
    {
        BOOST_CHECK( !response.empty() );
        BOOST_REQUIRE( std::filesystem::is_regular_file( projectRoot / "project.fbproject" ) );
        BOOST_REQUIRE( std::filesystem::is_regular_file( projectRoot / "Scripts" / "Main.lua" ) );

        const auto projectText = Path::readAllText( ( projectRoot / "project.fbproject" ).string() );
        const auto scriptText = Path::readAllText( ( projectRoot / "Scripts" / "Main.lua" ).string() );
        BOOST_CHECK( projectText.find( "\"productName\":\"Hello World AI\"" ) != String::npos );
        BOOST_CHECK( projectText.find( "\"applicationType\":\"lua\"" ) != String::npos );
        BOOST_CHECK( scriptText.find( "getCurrentScene" ) != String::npos );
        BOOST_CHECK( scriptText.find( "addComponent(\"Text\")" ) != String::npos );
        BOOST_CHECK( scriptText.find( "setText(\"Hello World!\")" ) != String::npos );
    }
}

BOOST_AUTO_TEST_SUITE_END()
