#pragma once

#include <Workphone/Core/Path.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <filesystem>

namespace workphone::editor
{
    inline void initializeEditorWorkspacePaths( SmartPtr<core::IApplicationManager> application,
                                                const String &startupDirectory )
    {
        const auto projectPath = application->getProjectPath();
        const auto workspacePath = projectPath.empty() ? startupDirectory : projectPath;
        if( application->getCachePath().empty() )
        {
            auto cachePath = Path::getAbsolutePath( workspacePath, "Cache" );
            std::filesystem::create_directories( std::filesystem::u8path( cachePath.c_str() ) );
            if( cachePath.back() != '/' ) cachePath += "/";
            application->setCachePath( cachePath );
        }
        if( application->getSettingsPath().empty() )
        {
            auto settingsPath = Path::getAbsolutePath( workspacePath, "SettingsCache" );
            std::filesystem::create_directories( std::filesystem::u8path( settingsPath.c_str() ) );
            if( settingsPath.back() != '/' ) settingsPath += "/";
            application->setSettingsPath( settingsPath );
        }
    }
}
