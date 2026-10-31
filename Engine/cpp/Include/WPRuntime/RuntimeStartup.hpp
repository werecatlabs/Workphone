#pragma once

#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>

namespace workphone
{
    /** Load the selected startup scene synchronously and reject failed loads. */
    inline bool loadRuntimeScene( SmartPtr<core::IApplicationManager> application,
                                  const String &scenePath )
    {
        if( !application || scenePath.empty() ) return false;
        auto manager = application->getGameManager();
        auto currentScene = manager ? manager->getCurrentScene() : nullptr;
        if( !manager || !manager->isLoaded() || !currentScene || !currentScene->isLoaded() ) return false;

        manager->loadScene( scenePath, false );
        const auto loadedScene = manager->getCurrentScene();
        return loadedScene && loadedScene->isLoaded() &&
               loadedScene->getSceneLoadingState() == scene::IGameScene::SceneLoadingState::Loaded;
    }

    /** Publish play mode before scene/Script lifecycle callbacks observe it. */
    inline bool startRuntimePlay( SmartPtr<core::IApplicationManager> application )
    {
        if( !application ) return false;
        auto manager = application->getGameManager();
        auto currentScene = manager ? manager->getCurrentScene() : nullptr;
        if( !manager || !manager->isLoaded() || !currentScene || !currentScene->isLoaded() ||
            currentScene->getSceneLoadingState() != scene::IGameScene::SceneLoadingState::Loaded )
            return false;

        application->setPaused( false );
        application->setEditorCamera( false );
        application->setPlaying( true );
        manager->play();
        if( auto cameraManager = application->getCameraManager() )
        {
            if( cameraManager->getState() == scene::ICameraManager::State::Play )
                cameraManager->reset();
            else
                cameraManager->setState( scene::ICameraManager::State::Play );
        }
        return true;
    }
}
