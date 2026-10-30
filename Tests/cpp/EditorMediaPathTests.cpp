#include <Workphone/Workphone.hpp>
#include <core/EditorMediaPaths.hpp>
#include <core/EditorWorkspacePaths.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace workphone;
namespace fs = std::filesystem;

namespace
{
    void require( bool condition, const char *message )
    {
        if( !condition ) throw std::runtime_error( message );
    }

    struct TemporaryDirectory
    {
        fs::path original = fs::current_path();
        fs::path path = fs::temp_directory_path() /
                        ( std::string( "workphone_editor_media_" ) + StringUtil::getUUID().c_str() );

        TemporaryDirectory() { fs::create_directories( path ); }
        ~TemporaryDirectory()
        {
            std::error_code ignored;
            fs::current_path( original, ignored );
            if( path.parent_path() == fs::temp_directory_path() &&
                path.filename().string().find( "workphone_editor_media_" ) == 0 )
                fs::remove_all( path, ignored );
        }
    };

    struct ApplicationFixture
    {
        SmartPtr<core::ApplicationManager> app = make_ptr<core::ApplicationManager>();
        SmartPtr<FactoryManager> factories = make_ptr<FactoryManager>();

        ApplicationFixture()
        {
            core::IApplicationManager::setInstance( app );
            factories->load( nullptr );
            app->setFactoryManager( factories );
        }
        ~ApplicationFixture()
        {
            if( auto files = app->getFileSystem() ) files->unload( nullptr );
            app->setFileSystem( nullptr );
            app->setFactoryManager( nullptr );
            factories->unload( nullptr );
            core::IApplicationManager::setInstance( nullptr );
        }
    };

    void verifyMedia( SmartPtr<IFileSystem> files, const fs::path &media )
    {
        struct ExpectedFile { const char *name; const char *relative; const char *contents; };
        for( const auto &expected : {
                 ExpectedFile{ "DefaultUI.mat", "DefaultUI.mat", "bundled material" },
                 ExpectedFile{ "fa-solid-900.ttf", "fa-solid-900.ttf", "bundled font" },
                 ExpectedFile{ "Editor.lua", "Scripts/Lua/Editor/Editor.lua", "bundled editor" },
                 ExpectedFile{ "BaseComponent.lua", "Scripts/Lua/Core/BaseComponent.lua", "bundled base" } } )
        {
            auto stream = files->open( expected.name, true, false, false, true, true );
            require( stream != nullptr, "Bundled file was not found through the real filesystem" );
            require( stream->getAsString() == expected.contents, "Bundled file contents changed" );
            const auto identity = stream->getFileInfo().absolutePath.str();
            require( fs::equivalent( fs::u8path( identity.c_str() ), media / expected.relative ),
                     "Bundled stream physical path was rebased to the project" );
        }
        const auto scripts = files->getFileNamesWithExtension( ".lua" );
        require( scripts.size() == 2, "Nested bundled Lua scripts were not enumerated" );
        for( const auto &script : scripts )
            require( files->open( script, true, false, false, false, false ) != nullptr,
                     "Enumerated Lua script path could not be opened" );
    }

    void mountContract( ApplicationFixture &application, const TemporaryDirectory &temporary,
                        const fs::path &startup, const String &configured )
    {
        const auto media = temporary.path / "bin/Media";
        auto files = make_ptr<FileSystem>();
        files->load( nullptr );
        application.app->setFileSystem( files );
        application.app->setProjectPath( ( temporary.path / "First Project" ).generic_u8string() );
        // Opening the saved project before mounting media must not reinterpret
        // installation-relative paths against that project's directory.
        fs::current_path( startup );
        const auto resolved = editor::mountEditorMedia( files, startup.generic_u8string(), configured );
        require( Path::isPathAbsolute( resolved ), "Editor media root must be absolute" );
        require( fs::equivalent( fs::u8path( resolved.c_str() ), media ),
                 "Editor media root was not resolved against its startup directory" );
        verifyMedia( files, media );
        fs::current_path( temporary.path / "Launcher" );
        verifyMedia( files, media );
        application.app->setProjectPath( ( temporary.path / "Second Project" ).generic_u8string() );
        fs::current_path( temporary.path / "Second Project" );
        verifyMedia( files, media );
        files->refreshAll( false );
        verifyMedia( files, media );
        files->unload( nullptr );
        application.app->setFileSystem( nullptr );
    }

    void workspaceContract( ApplicationFixture &application, const TemporaryDirectory &temporary,
                            const fs::path &startup )
    {
        for( const auto &project : { fs::path{}, temporary.path / "First Project" } )
        {
            application.app->setProjectPath( project.generic_u8string() );
            application.app->setCachePath( "" );
            application.app->setSettingsPath( "" );
            editor::initializeEditorWorkspacePaths( application.app, startup.generic_u8string() );
            const auto expected = project.empty() ? startup : project;
            const auto cache = fs::u8path( application.app->getCachePath().c_str() );
            const auto settings = fs::u8path( application.app->getSettingsPath().c_str() );
            require( fs::equivalent( cache, expected / "Cache" ),
                     "Initial editor cache was not rooted in the active workspace" );
            require( fs::equivalent( settings, expected / "SettingsCache" ),
                     "Initial editor settings cache was not rooted in the active workspace" );
            require( fs::u8path( ( application.app->getCachePath() + "/asset.db" ).c_str() )
                             .lexically_normal() == expected / "Cache/asset.db",
                     "Initial asset database escaped the active workspace" );
        }
        const auto explicitCache = ( temporary.path / "Custom Cache" ).generic_u8string();
        const auto explicitSettings = ( temporary.path / "Custom Settings" ).generic_u8string();
        application.app->setCachePath( explicitCache );
        application.app->setSettingsPath( explicitSettings );
        editor::initializeEditorWorkspacePaths( application.app, startup.generic_u8string() );
        require( application.app->getCachePath() == explicitCache &&
                 application.app->getSettingsPath() == explicitSettings,
                 "Explicit editor cache paths were replaced" );
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
        TemporaryDirectory temporary;
        ApplicationFixture application;
        const auto media = temporary.path / "bin/Media";
        const auto development = temporary.path / "bin/windows/v145/x64/MD/RelWithDebInfo";
        const auto workspace = temporary.path / "Workspace";
        for( const auto &folder : { media / "Scripts/Lua/Editor", media / "Scripts/Lua/Core",
                                    development, workspace, temporary.path / "Launcher",
                                    temporary.path / "First Project", temporary.path / "Second Project" } )
            fs::create_directories( folder );
        std::ofstream( media / "DefaultUI.mat" ) << "bundled material";
        std::ofstream( media / "fa-solid-900.ttf" ) << "bundled font";
        std::ofstream( media / "Scripts/Lua/Editor/Editor.lua" ) << "bundled editor";
        std::ofstream( media / "Scripts/Lua/Core/BaseComponent.lua" ) << "bundled base";
        mountContract( application, temporary, development, "../../../../../Media" );
        mountContract( application, temporary, workspace, media.generic_u8string() );
        mountContract( application, temporary, workspace, "../bin/Media" );
        workspaceContract( application, temporary, workspace );
        std::cout << "PASS: bundled editor material, font and nested Lua paths survive project switches and archive reloads\n";
    }
    catch( const std::exception &error )
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        result = 1;
    }
    types.unload();
    TypeManager::setInstance( nullptr );
    return result;
}
