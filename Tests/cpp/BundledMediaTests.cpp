#include <Workphone/Workphone.hpp>
#include <Workphone/Graphics/GraphicsSystem.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/Scene/ScenePicking.hpp>
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
        TaskId previousTask = Thread::getCurrentTask();
        ApplicationFixture()
        {
            core::IApplicationManager::setInstance( app );
            Thread::setCurrentTask( TaskId::Primary );
            app->setLogManager( make_ptr<LogManagerDefault>() );
            factories->load( nullptr );
            app->setFactoryManager( factories );
            app->setGameManager( make_ptr<scene::GameManager>() );
            app->setGraphicsSystem( make_ptr<render::GraphicsSystem>() );
            app->setMeshManager( make_ptr<MeshManager>() );
            sqlite->load( nullptr );
        }
        ~ApplicationFixture()
        {
            app->setGameManager( nullptr );
            if( auto resources = app->getResourceDatabase() ) resources->unload( nullptr );
            app->setResourceDatabase( nullptr );
            if( auto meshes = app->getMeshManager() ) meshes->unload( nullptr );
            app->setMeshManager( nullptr );
            app->setGraphicsSystem( nullptr );
            if( auto files = app->getFileSystem() ) files->unload( nullptr );
            app->setFileSystem( nullptr );
            sqlite->unload( nullptr );
            app->setFactoryManager( nullptr );
            factories->unload( nullptr );
            core::IApplicationManager::setInstance( nullptr );
            Thread::setCurrentTask( previousTask );
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

    void verifyScenePicking( AssetDatabaseManager &catalog )
    {
        const String meshPath = "cube_internal.fbmeshbin";
        const String staleUuid = "c08aa9a6-9c6e-4120-b423-d8e6952e4ecb";
        AssetDatabaseManager::EntrySnapshot entry;
        require( !catalog.tryGetEntry( staleUuid, entry ), "The saved mesh UUID must be stale" );

        auto actor = make_ptr<scene::GameActor>();
        actor->setEnabled( true );
        actor->setVisible( true );

        // Older scenes save the media-relative path alongside a UUID that may
        // no longer exist in the current installation's catalog.
        auto saved = make_ptr<Properties>();
        saved->setProperty( "Mesh Path", meshPath );
        saved->setProperty( "Mesh Resource", staleUuid, Properties::resourceStr );
        auto mesh = make_ptr<scene::Mesh>();
        mesh->fromData( saved );
        const auto resource = mesh->getMeshResource();
        require( resource && catalog.tryGetEntry( resource->getHandle()->getUUIDAsString(), entry ) &&
                     entry.path == meshPath,
                 "The restored mesh must use the bundled cube's catalog identity" );
        mesh->setEnabled( true );
        actor->addComponentInstance( mesh );

        // The CPU picker uses the authoring components and mesh data without a
        // GPU scene. Attach the renderer after restoring the mesh properties.
        auto renderer = make_ptr<scene::MeshRenderer>();
        renderer->setEnabled( true );
        actor->addComponentInstance( renderer );

        const Ray3<real_Num> ray( { .2f, .1f, 20 }, { 0, 0, -1 } );
        const Array<SmartPtr<scene::IGameActor>> actors{ actor };
        require( scene::pickSceneActor( actors, ray, 40 ) == actor,
                 "Scene picking must hit a bundled cube restored from a stale mesh UUID" );
        actor->setVisible( false );
        require( !scene::pickSceneActor( actors, ray, 40 ), "Scene picking must skip a hidden cube" );
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
        fs::copy_file( fs::u8path( WP_TEST_SOURCE_ROOT ) / "bin/Media/cube_internal.fbmeshbin",
                       media / "cube_internal.fbmeshbin" );
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
        verifyScenePicking( *catalog );
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
        std::cout << "PASS: bundled media discovery, physical mounts, catalog UUIDs and scene picking\n";
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
