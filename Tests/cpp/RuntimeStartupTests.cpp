#include <WPRuntime/RuntimeStartup.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/TaskManager.hpp>
#include <Workphone/IO/FileSystem.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/Scene/GameScene.hpp>
#include <Workphone/Scene/CameraManager.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <iostream>
#include <stdexcept>

using namespace workphone;

namespace
{
    void require( bool condition, const char *message )
    {
        if( !condition ) throw std::runtime_error( message );
    }

    // Observe the real GameManager's load and play dispatch without starting a
    // renderer or parsing an external scene. Native ApplicationManager owns the
    // play flag read by project Lua scripts.
    class StartupScene final : public scene::GameScene
    {
    public:
        void clear( bool clearNow = true ) override
        {
            ++clears;
            clearWasSynchronous = clearNow;
        }

        void loadScene( const String &path, bool async ) override
        {
            ++loads;
            loadedPath = path;
            loadWasAsync = async;
            playingWhileLoading = core::IApplicationManager::instance()->isPlaying();
            setFilePath( path );
            setSceneLoadingState( nextLoadState );
        }

        void setState( State state ) override
        {
            observedState = state;
            if( state == State::Play )
            {
                ++plays;
                auto application = core::IApplicationManager::instance();
                playingAtSceneTransition = application->isPlaying();
                pausedAtSceneTransition = application->isPaused();
                editorCameraAtSceneTransition = application->isEditorCamera();
            }
        }

        State getState() const override { return observedState; }

        unsigned clears = 0, loads = 0, plays = 0;
        bool clearWasSynchronous = false, loadWasAsync = true;
        bool playingWhileLoading = true, playingAtSceneTransition = false;
        bool pausedAtSceneTransition = true, editorCameraAtSceneTransition = true;
        String loadedPath;
        State observedState = State::None;
        SceneLoadingState nextLoadState = SceneLoadingState::Loaded;
    };

    class StartupCameraManager final : public scene::CameraManager
    {
    public:
        void reset() override
        {
            ++resets;
            auto application = core::IApplicationManager::instance();
            auto currentScene = application->getGameManager()->getCurrentScene();
            resetAfterScenePlay = currentScene->getState() == scene::IGameScene::State::Play;
            resetWithRuntimeFlags = application->isPlaying() && !application->isPaused() &&
                                    !application->isEditorCamera();
            CameraManager::reset();
        }

        unsigned resets = 0;
        bool resetAfterScenePlay = false, resetWithRuntimeFlags = false;
    };

    struct Fixture
    {
        SmartPtr<core::ApplicationManager> app = make_ptr<core::ApplicationManager>();
        SmartPtr<scene::GameManager> game = make_ptr<scene::GameManager>();
        SmartPtr<StartupScene> scene = make_ptr<StartupScene>();
        SmartPtr<StartupCameraManager> cameras = make_ptr<StartupCameraManager>();

        Fixture()
        {
            core::IApplicationManager::setInstance( app );
            app->setFileSystem( make_ptr<FileSystem>() );
            app->setTaskManager( make_ptr<TaskManager>() );
            app->setGameManager( game );
            app->setCameraManager( cameras );
            game->setCurrentScene( scene );
            game->setLoadingState( LoadingState::Loaded );
            scene->setLoadingState( LoadingState::Loaded );
            cameras->load( nullptr );
        }

        ~Fixture()
        {
            // These observer objects own no loaded scene graph or worker tasks.
            scene->setLoadingState( LoadingState::Unloaded );
            game->setLoadingState( LoadingState::Unloaded );
            game->setCurrentScene( nullptr );
            cameras->unload( nullptr );
            app->setCameraManager( nullptr );
            app->setGameManager( nullptr );
            app->setFileSystem( nullptr );
            app->setTaskManager( nullptr );
            core::IApplicationManager::setInstance( nullptr );
        }
    };

    void selectedSceneAndPlayContract()
    {
        Fixture fixture;
        const String selected = "Assets/Scenes/ConfiguredStartup.fbscenebin";
        require( !fixture.app->isPlaying(), "Runtime fixture must begin outside play mode" );
        require( loadRuntimeScene( fixture.app, selected ), "Selected startup scene failed to load" );
        require( fixture.scene->loads == 1 && fixture.scene->loadedPath == selected,
                 "Runtime replaced the selected scene with Application.fbscene" );
        require( fixture.scene->clears == 1 && fixture.scene->clearWasSynchronous &&
                     !fixture.scene->loadWasAsync,
                 "Startup must use the real GameManager synchronous scene handoff" );
        require( !fixture.scene->playingWhileLoading && !fixture.app->isPlaying(),
                 "Runtime entered play mode before the scene finished loading" );
        fixture.app->setPaused( true );
        fixture.app->setEditorCamera( true );
        require( startRuntimePlay( fixture.app ), "Loaded startup scene did not enter play mode" );
        require( fixture.app->isPlaying() && fixture.scene->plays == 1 &&
                     fixture.scene->getState() == scene::IGameScene::State::Play &&
                     fixture.scene->playingAtSceneTransition,
                 "Scene lifecycle callbacks observed application play mode as false" );
        require( !fixture.scene->pausedAtSceneTransition &&
                     !fixture.scene->editorCameraAtSceneTransition,
                 "Scene lifecycle callbacks observed paused or editor-camera mode" );
        require( fixture.cameras->getState() == scene::ICameraManager::State::Play &&
                     fixture.cameras->resets == 1 && fixture.cameras->resetAfterScenePlay &&
                     fixture.cameras->resetWithRuntimeFlags,
                 "Runtime camera manager did not reset in Play after the scene transition" );

        // A manager already in Play still needs to select cameras for a newly
        // loaded scene; CameraManager::setState(Play) alone would skip reset.
        fixture.app->setPaused( true );
        fixture.app->setEditorCamera( true );
        require( startRuntimePlay( fixture.app ) && fixture.cameras->resets == 2 &&
                     fixture.cameras->resetAfterScenePlay && fixture.cameras->resetWithRuntimeFlags,
                 "Runtime failed to refresh a camera manager already in Play" );
    }

    void failedAndPendingSceneContract()
    {
        Fixture fixture;
        using State = scene::IGameScene::SceneLoadingState;
        for( const auto state : { State::Failed, State::Cancelled, State::Loading, State::Unloaded } )
        {
            fixture.scene->nextLoadState = state;
            require( !loadRuntimeScene( fixture.app, "Assets/Chosen.fbscene" ),
                     "Runtime reported an unsuccessful scene load as ready" );
            require( !startRuntimePlay( fixture.app ) && !fixture.app->isPlaying() &&
                         fixture.scene->plays == 0,
                     "Runtime played a failed or pending startup scene" );
        }
        fixture.scene->nextLoadState = State::Loaded;
        require( loadRuntimeScene( fixture.app, "Assets/Chosen.fbscene" ) &&
                     startRuntimePlay( fixture.app ),
                 "Runtime could not start after a successful scene load" );
    }

    void missingStartupDependenciesContract()
    {
        require( !loadRuntimeScene( nullptr, "Chosen.fbscene" ) && !startRuntimePlay( nullptr ),
                 "Missing application was accepted" );
        Fixture fixture;
        require( !loadRuntimeScene( fixture.app, "" ) && fixture.scene->loads == 0 &&
                     fixture.scene->clears == 0,
                 "An empty startup path cleared the current scene" );
        fixture.scene->setLoadingState( LoadingState::Unloaded );
        require( !loadRuntimeScene( fixture.app, "Chosen.fbscene" ) &&
                     !startRuntimePlay( fixture.app ),
                 "An uninitialized scene was accepted" );
        fixture.scene->setLoadingState( LoadingState::Loaded );
        fixture.game->setLoadingState( LoadingState::Unloaded );
        require( !loadRuntimeScene( fixture.app, "Chosen.fbscene" ) &&
                     !startRuntimePlay( fixture.app ),
                 "An uninitialized game manager was accepted" );
        fixture.game->setLoadingState( LoadingState::Loaded );
        fixture.game->setCurrentScene( nullptr );
        require( !loadRuntimeScene( fixture.app, "Chosen.fbscene" ) &&
                     !startRuntimePlay( fixture.app ),
                 "A missing current scene was accepted" );
        fixture.app->setGameManager( nullptr );
        require( !loadRuntimeScene( fixture.app, "Chosen.fbscene" ) &&
                     !startRuntimePlay( fixture.app ) && !fixture.app->isPlaying(),
                 "A missing game manager was accepted" );
    }
}

int main()
{
    TypeManager types;
    types.load();
    TypeManager::setInstance( &types );
    int result = 0;
    try
    {
        selectedSceneAndPlayContract();
        failedAndPendingSceneContract();
        missingStartupDependenciesContract();
        std::cout << "WPRuntime startup contracts passed (selected scene, play ordering, camera state/reset, failed loads, missing dependencies).\n";
    }
    catch( const std::exception &error )
    {
        std::cerr << "WPRuntime startup contract failed: " << error.what() << '\n';
        result = 1;
    }
    types.unload();
    TypeManager::setInstance( nullptr );
    return result;
}
