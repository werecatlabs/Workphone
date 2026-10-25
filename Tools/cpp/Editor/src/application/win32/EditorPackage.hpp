#pragma once

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <Windows.h>
#include <TlHelp32.h>
#include <ui/ScriptWindow.hpp>
#include <Workphone/Database/ResourceDatabase.hpp>

namespace workphone::editor
{
    // Keep package paths in the Editor adapter; the shared engine's development
    // defaults and the Debug test entry point retain their existing behavior.
    class PackagedEditorApplication : public EditorApplication
    {
    public:
        String mediaDirectory;
        String pluginConfiguration;
        String catalogFile;

        String getMediaPath() const override { return mediaDirectory; }
        String getMediaCatalogPath( const String &mediaRoot ) const override
        {
            return catalogFile.empty() ? EditorApplication::getMediaCatalogPath( mediaRoot ) : catalogFile;
        }

    protected:
        void createPlugins() override
        {
            setPluginsConfigFilePath( pluginConfiguration );
            core::Application::createPlugins();
        }
    };

    struct EditorPackage
    {
        std::filesystem::path root;
        std::filesystem::path workspace;
        bool smoke = false;

        EditorPackage( int argc, char **argv )
        {
            wchar_t executable[32768];
            const auto length = GetModuleFileNameW( nullptr, executable, 32768 );
            if( !length || length == 32768 )
                throw std::runtime_error( "Cannot locate Editor executable" );
            root = std::filesystem::path( executable ).parent_path().parent_path();
            const bool installed = std::filesystem::is_regular_file( root / "editor-package.cfg" );
            bool explicitRoot = false;
            for( int i = 1; i < argc; ++i )
            {
                const std::string argument = argv[i];
                if( argument == "--package-root" || argument == "--workspace" )
                {
                    if( ++i == argc ) throw std::runtime_error( "Missing package path argument" );
                    if( argument == "--package-root" ) { root = argv[i]; explicitRoot = true; }
                    else workspace = argv[i];
                }
                else if( argument == "--package-smoke" ) smoke = true;
            }
            if( !installed && !explicitRoot && !smoke && workspace.empty() )
            {
                // Development plugin/media paths are relative to the executable,
                // including when a desktop launcher supplies a different CWD.
                std::filesystem::current_path( std::filesystem::path( executable ).parent_path() );
                root.clear();
                return;
            }
            root = std::filesystem::canonical( root );
            if( workspace.empty() )
            {
                const auto local = _wgetenv( L"LOCALAPPDATA" );
                if( !local ) throw std::runtime_error( "LOCALAPPDATA is required for the Editor workspace" );
                workspace = std::filesystem::path( local ) / "Workphone" / "Editor";
            }
            workspace = std::filesystem::weakly_canonical( std::filesystem::absolute( workspace ) );
            // A package is immutable. Settings, logs, caches and catalog data belong
            // in a separate workspace, including when launched by double click.
            const auto rootName = root.native();
            const auto workspaceName = workspace.native();
            if( _wcsicmp( rootName.c_str(), workspaceName.c_str() ) == 0 ||
                ( workspaceName.size() > rootName.size() &&
                  _wcsnicmp( rootName.c_str(), workspaceName.c_str(), rootName.size() ) == 0 &&
                  ( workspaceName[rootName.size()] == L'\\' || workspaceName[rootName.size()] == L'/' ) ) )
                throw std::runtime_error( "Editor workspace must be outside the package" );
            if( !std::filesystem::is_regular_file( root / "Media/Scripts/Lua/Editor/Editor.lua" ) )
                throw std::runtime_error( "Editor package media is missing" );
            std::ifstream profile( root / "editor-package.cfg" );
            if( !profile ) throw std::runtime_error( "Editor package plugin profile is missing" );
            std::string configuration, line;
            unsigned plugins = 0;
            while( std::getline( profile, line ) )
            {
                if( !line.empty() && line.back() == '\r' ) line.pop_back();
                if( line.empty() || line.front() == '#' ) continue;
                if( line.compare( 0, 7, "Plugin=" ) != 0 )
                    throw std::runtime_error( "Invalid Editor package plugin profile" );
                const auto name = line.substr( 7 );
                if( name.empty() || name.find_first_not_of( "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_" ) != std::string::npos )
                    throw std::runtime_error( "Invalid Editor package plugin name" );
                const auto dll = root / "bin" / ( name + ".dll" );
                if( !std::filesystem::is_regular_file( dll ) )
                    throw std::runtime_error( "Editor package plugin is missing: " + name );
                configuration += "Plugin=" + dll.generic_string() + "\n";
                ++plugins;
            }
            if( !plugins ) throw std::runtime_error( "Editor package plugin profile is empty" );
            std::filesystem::create_directories( workspace );
            std::ofstream config( workspace / "wp_plugins_package.cfg", std::ios::trunc );
            config << configuration;
            config.close();
            if( !config ) throw std::runtime_error( "Cannot write workspace plugin configuration" );
            Path::setWorkingDirectory( String( workspace.generic_string().c_str() ) );
        }

        void verifyStartup( const SmartPtr<PackagedEditorApplication> &app ) const
        {
            auto manager = core::IApplicationManager::instance();
            if( !app->isLoaded() || !manager || !manager->getGraphicsSystem() ||
                !manager->getResourceDatabase() || !manager->getFileSystem() ||
                !manager->getGameManager() || !manager->getTaskManager() )
                throw std::runtime_error( "Packaged Editor managers failed to initialize" );
            auto stream = manager->getFileSystem()->open( "Editor.lua", true, true, false, true, true );
            if( !stream ) throw std::runtime_error( "Packaged Editor media is not mounted" );
            auto resources = dynamic_pointer_cast<ResourceDatabase>( manager->getResourceDatabase() );
            if( !resources || !resources->getMediaCatalog() ||
                !std::filesystem::is_regular_file( workspace / "Cache/media-assets.db" ) )
                throw std::runtime_error( "Packaged Editor media catalog is unavailable" );
            auto scripts = manager->getScriptManager();
            if( !scripts || !scripts->isLoaded() )
                throw std::runtime_error( "Packaged Editor Lua manager is unavailable" );
            scripts->loadScriptFromString( "class 'PackageStartupProbe'\n"
                "function PackageStartupProbe:__init(owner) owner:setName('package-lua-ready') end" );
            SmartPtr<ISharedObject> owner = make_ptr<ScriptWindow>();
            scripts->createObject( "PackageStartupProbe", owner );
            const bool executed = owner->getName() == "package-lua-ready";
            scripts->destroyObject( owner );
            if( !executed ) throw std::runtime_error( "Packaged Editor Lua execution failed" );

            if( const auto report = _wgetenv( L"WP_EDITOR_MODULE_REPORT" ) )
            {
                std::ofstream output( std::filesystem::path( report ), std::ios::trunc );
                const auto snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, GetCurrentProcessId() );
                if( snapshot == INVALID_HANDLE_VALUE ) throw std::runtime_error( "Cannot enumerate Editor modules" );
                MODULEENTRY32W module = {};
                module.dwSize = sizeof( module );
                bool found = Module32FirstW( snapshot, &module ) != FALSE;
                while( found )
                {
                    output << std::filesystem::path( module.szExePath ).u8string() << '\n';
                    found = Module32NextW( snapshot, &module ) != FALSE;
                }
                CloseHandle( snapshot );
                output.close();
                if( !output ) throw std::runtime_error( "Cannot write Editor module report" );
            }
        }
    };
}
