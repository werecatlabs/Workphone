#pragma once

#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>
#include <algorithm>
#include <filesystem>
#include <set>
#include <stdexcept>

namespace workphone
{
    inline constexpr auto runtimePackageProjectName = "Runtime.fbproject";

    inline bool isRuntimeSceneExtension( const String &extension )
    {
        return extension == ".fbscene" || extension == ".fbscenexml" ||
               extension == ".fbscenebin";
    }

    /** Validate the startup scene name as a project-relative archive key. */
    inline String runtimePackageScenePath( const String &name )
    {
        namespace fs = std::filesystem;
        const auto relative = fs::u8path( name.c_str() );
        if( relative.has_root_path() || name.find( ':' ) != String::npos ||
            !isRuntimeSceneExtension( StringUtil::make_lower( Path::getFileExtension( name ) ) ) ||
            std::any_of( relative.begin(), relative.end(), []( const fs::path &part ) {
                return part == "." || part == "..";
            } ) )
        {
            throw std::runtime_error( "Invalid startup scene in Runtime.fbproject" );
        }
        return relative.generic_u8string().c_str();
    }

    /** Check all split archives before initializing the game subsystems. */
    inline void validateRuntimePackageArchives( const std::filesystem::path &root,
                                                const Array<String> &archives )
    {
        if( archives.empty() )
        {
            throw std::runtime_error( "Runtime.fbproject lists no package archives" );
        }
        std::set<String> names;
        for( const auto &name : archives )
        {
            const auto relative = std::filesystem::u8path( name.c_str() );
            const auto key = StringUtil::make_lower( name );
            if( relative.has_root_path() || relative.filename() != relative ||
                name.find( ':' ) != String::npos || Path::getFileExtension( key ) != ".fbpak" ||
                !names.insert( key ).second )
            {
                throw std::runtime_error( "Invalid or duplicate package archive: " + name );
            }
            if( !std::filesystem::is_regular_file( root / relative ) )
            {
                throw std::runtime_error( "Missing package archive: " + name );
            }
        }
    }

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
