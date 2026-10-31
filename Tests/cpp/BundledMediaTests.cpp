#include <Workphone/Workphone.hpp>
#include <Workphone/System/BundledMediaPaths.hpp>
#include <Workphone/Script/ScriptAsset.hpp>
#include <WPSQLite/WPSQLite.hpp>
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
                        ( std::string( "workphone_bundled_media_" ) + StringUtil::getUUID().c_str() );
        TemporaryDirectory() { fs::create_directories( path ); }
        ~TemporaryDirectory()
        {
            std::error_code ignored;
            fs::current_path( original, ignored );
            if( path.parent_path() == fs::temp_directory_path() &&
                path.filename().string().find( "workphone_bundled_media_" ) == 0 )
                fs::remove_all( path, ignored );
        }
    };

    struct ApplicationFixture
    {
        SmartPtr<core::ApplicationManager> app = make_ptr<core::ApplicationManager>();
        SmartPtr<FactoryManager> factories = make_ptr<FactoryManager>();
        SmartPtr<SQLitePlugin> sqlite = make_ptr<SQLitePlugin>();
        ApplicationFixture()
        {
            core::IApplicationManager::setInstance( app );
            app->setLogManager( make_ptr<LogManagerDefault>() );
            factories->load( nullptr );
            app->setFactoryManager( factories );
            sqlite->load( nullptr );
        }
        ~ApplicationFixture()
        {
            if( auto resources = app->getResourceDatabase() ) resources->unload( nullptr );
            app->setResourceDatabase( nullptr );
            if( auto files = app->getFileSystem() ) files->unload( nullptr );
            app->setFileSystem( nullptr );
            sqlite->unload( nullptr );
            app->setFactoryManager( nullptr );
            factories->unload( nullptr );
            core::IApplicationManager::setInstance( nullptr );
        }
    };

    void write( const fs::path &file, const char *contents )
    {
        fs::create_directories( file.parent_path() );
        std::ofstream stream( file, std::ios::binary );
        stream << contents;
        require( stream.good(), "Cannot write bundled-media fixture" );
    }

    void verifyStreams( SmartPtr<IFileSystem> files, const fs::path &media )
    {
        for( const auto &item : {
                 std::pair<const char *, const char *>{ "DefaultUI.mat", "material" },
                 { "fa-solid-900.ttf", "font" }, { "BaseComponent.lua", "return {}" } } )
        {
            auto stream = files->open( item.first, true, false, false, true, true );
            require( stream && stream->getAsString() == item.second,
                     "Real filesystem must open the installed bundled bytes" );
            const auto relative = std::string( item.first ) == "BaseComponent.lua"
                                      ? "Scripts/Lua/Core/BaseComponent.lua" : item.first;
            require( fs::equivalent( fs::u8path( stream->getFileInfo().absolutePath.str().c_str() ),
                                     media / relative ),
                     "Bundled streams must retain the installation source root" );
        }
        const auto scripts = files->getFileNamesWithExtension( ".lua" );
        require( scripts.size() == 1, "Nested bundled scripts must be enumerated once" );
    }

    void runContracts()
    {
        TemporaryDirectory temporary;
        ApplicationFixture application;
        const auto checkout = temporary.path / "Checkout";
        const auto media = checkout / "bin/Media";
        const auto executable = checkout / "bin/windows/v145/x64/MD/RelWithDebInfo";
        const auto launcher = temporary.path / "Launcher";
        const auto projectA = temporary.path / "Project A";
        const auto projectB = temporary.path / "Project B";
        for( const auto &directory : { executable, launcher, projectA / "Cache", projectB } )
            fs::create_directories( directory );
        write( media / "DefaultUI.mat", "material" );
        write( media / "fa-solid-900.ttf", "font" );
        write( media / "Scripts/Lua/Core/BaseComponent.lua", "return {}" );
        write( projectA / "Media/DefaultUI.mat", "project decoy" );

        const auto resolve = [&]( const fs::path &startup, const String &configured,
                                   const fs::path &module ) {
            const auto path = resolveBundledMediaPath( startup.generic_u8string().c_str(), configured,
                                                        module.generic_u8string().c_str() );
            require( Path::isPathAbsolute( path ) && fs::equivalent( fs::u8path( path.c_str() ), media ),
                     "Bundled media must resolve from launcher, project and source-root starts" );
        };
        resolve( launcher, "Media", executable );
        resolve( projectA, "Media", executable );
        resolve( checkout, "Media", {} );
        resolve( executable, "../../../../../Media", executable );
        resolve( launcher, media.generic_u8string().c_str(), {} );
        const auto installed = temporary.path / "Installed Application";
        fs::create_directories( installed / "Media" );
        const auto installedRoot = resolveBundledMediaPath( launcher.generic_u8string().c_str(),
                                                            "Media", installed.generic_u8string().c_str() );
        require( fs::equivalent( fs::u8path( installedRoot.c_str() ), installed / "Media" ),
                 "Packaged applications must resolve Media beside the executable" );

        application.app->setProjectPath( projectA.generic_u8string().c_str() );
        auto files = make_ptr<FileSystem>();
        files->load( nullptr );
        application.app->setFileSystem( files );
        fs::current_path( projectA );
        const auto mounted = mountBundledMedia( files, projectA.generic_u8string().c_str(), "Media",
                                                executable.generic_u8string().c_str() );
        application.app->setMediaPath( mounted );
        verifyStreams( files, media );

        auto resources = make_ptr<ResourceDatabase>();
        application.app->setResourceDatabase( resources );
        resources->load( nullptr );
        auto projectCatalog = dynamic_pointer_cast<AssetDatabaseManager>( resources->getDatabaseManager() );
        require( projectCatalog && projectCatalog->setProjectRoot( projectA.generic_u8string().c_str() ),
                 "Project catalog must capture its project root" );
        projectCatalog->loadFromFile( ( projectA / "Cache/asset.db" ).generic_u8string().c_str() );
        String error;
        const auto mountedCatalog = mountBundledMediaCatalog( resources, mounted, error );
        require( mountedCatalog, error.c_str() );
        auto catalog = resources->getMediaCatalog();
        require( catalog && catalog->isReadOnly() &&
                     fs::equivalent( fs::u8path( catalog->getProjectRoot().c_str() ), media ),
                 "Runtime catalog must mount read-only using the installation source root" );
        auto script = dynamic_pointer_cast<ScriptAsset>(
            resources->loadMediaResource( "Scripts/Lua/Core/BaseComponent.lua" ) );
        require( script && !script->getHandle()->getUUID().is_nil(),
                 "Runtime bundled scripts must resolve through their catalog UUID" );
        require( fs::equivalent( fs::u8path( script->getFilePath().c_str() ),
                                 media / "Scripts/Lua/Core/BaseComponent.lua" ),
                 "Runtime UUID resolution must retain the installed source path" );
        const auto uuid = script->getHandle()->getUUID();
        script = nullptr;
        application.app->setProjectPath( projectB.generic_u8string().c_str() );
        fs::current_path( projectB );
        files->refreshAll( false );
        verifyStreams( files, media );
        auto switchedScript = resources->loadMediaResource( "Scripts/Lua/Core/BaseComponent.lua" );
        require( switchedScript && switchedScript->getHandle()->getUUID() == uuid,
                 "Project changes must not alter bundled catalog identities" );
        require( !resources->loadMediaResource( "../outside.lua" ),
                 "Bundled catalog requests must reject escaped source paths" );
        resources->unload( nullptr );
        application.app->setResourceDatabase( nullptr );
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
        runContracts();
        std::cout << "PASS: runtime bundled media discovery, physical mounts, read-only catalog and UUID resolution\n";
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
