#ifndef WP_MESH_IMPORT_STREAMING_INTEGRATION_TESTS_HPP
#define WP_MESH_IMPORT_STREAMING_INTEGRATION_TESTS_HPP

#include "TestGuard.hpp"
#include <Workphone/ApplicationUtil.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/FileInfo.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Database/ResourceDatabase.hpp>
#include <Workphone/Interface/Graphics/IMeshConverter.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Mesh/IMeshLoader.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Interface/Mesh/ISubMesh.hpp>
#include <Workphone/Interface/Mesh/IVertexBuffer.hpp>
#include <Workphone/Interface/Scene/IGamePrefab.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Scene/GamePrefabManager.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Scene/Components/WorldStreaming.hpp>
#include <Workphone/Scene/Directors/MeshResourceDirector.hpp>
#include <Workphone/Scene/SceneWorld.hpp>
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <array>
#include <set>

namespace mesh_import_streaming_integration_tests
{
    using namespace workphone;
    using namespace workphone::scene;

    inline std::string readBytes( const String &path )
    {
        std::ifstream input( path.c_str(), std::ios::binary );
        BOOST_REQUIRE_MESSAGE( input, "Expected published import document: " << path );
        const std::string text( ( std::istreambuf_iterator<char>( input ) ), std::istreambuf_iterator<char>() );
        return text;
    }
    inline void writeText( const String &path, const String &text )
    {
        std::ofstream output( path.c_str(), std::ios::binary | std::ios::trunc );
        output.write( text.data(), text.size() ); output.close(); BOOST_REQUIRE( output );
    }
    inline void visit( const Properties &data, const std::function<void( const Properties & )> &fn )
    {
        fn( data ); for( const auto &child : data.getChildren() ) visit( *child, fn );
    }
    inline SmartPtr<Properties> streamingComponent( const SmartPtr<Properties> &data )
    {
        BOOST_REQUIRE( data );
        SmartPtr<Properties> result;
        for( auto component : data->getChildrenByName( "component" ) )
        {
            const auto type = component->getProperty( "componentType" );
            if( type == "WorldStreaming" || type == "workphone::scene::WorldStreaming" ) result = component;
        }
        return result;
    }
    inline SmartPtr<Properties> requireWrapper( const SmartPtr<IGamePrefab> &prefab )
    {
        BOOST_REQUIRE_MESSAGE( prefab, "Source import must produce a loadable prefab" );
        auto data = dynamic_pointer_cast<Properties>( prefab->getData() ); BOOST_REQUIRE( data );
        bool imported = false; data->getPropertyValue( "meshImportStreaming", imported ); BOOST_CHECK( imported );
        BOOST_CHECK( data->getChildrenByName( "child" ).empty() );
        BOOST_CHECK( data->getChildrenByName( "children" ).empty() );
        const auto component = streamingComponent( data );
        BOOST_REQUIRE_MESSAGE( component, "Imported prefab must contain a WorldStreaming component" );
        u32 meshes = 0;
        visit( *data, [&]( const Properties &p ) { if( p.hasProperty( "Mesh Path" ) ) ++meshes; } );
        BOOST_CHECK_EQUAL( meshes, 0u );
        return component;
    }

    struct Fixture
    {
        Fixture()
        {
            auto app = guard.applicationManager;
            if( auto graphics = app->getGraphicsSystem() )
                if( auto window = graphics->getDefaultWindow() ) window->setVisible( false );
            BOOST_REQUIRE_MESSAGE( app->getGraphicsSystem(), "Mesh import integration requires the graphics plugin" );
            BOOST_REQUIRE_MESSAGE( app->getGraphicsSystem()->getMeshConverter(),
                                   "Mesh import integration requires the real mesh converter" );
            loader = guard.factoryManager->make_object<IMeshLoader>();
            BOOST_REQUIRE_MESSAGE( loader, "Mesh import integration requires an OBJ-capable mesh loader plugin" );
            meshes = make_ptr<MeshManager>(); prefabs = make_ptr<GamePrefabManager>();
            database = make_ptr<ResourceDatabase>();
            const auto id = StringUtil::getUUID();
            directory = std::filesystem::temp_directory_path() /
                ( "wp-import-streaming-" + std::string( id.c_str() ) );
            std::filesystem::create_directories( directory / "Cache" );
            std::filesystem::create_directories( directory / "SettingsCache" );
            guard.trackFilesystemPath( directory.generic_string().c_str() );
            root = directory.generic_string().c_str();
            // Match Editor imports and MeshTest's working OBJ fixture. Absolute
            // short-temp paths may differ from the archive's canonical paths.
            source = "city-" + id + ".obj";

            const auto oldProject = app->getProjectPath(), oldCache = app->getCachePath(), oldSettings = app->getSettingsPath();
            const auto oldRunning = app->isRunning(), oldQuit = app->getQuit();
            const auto oldDirectory = std::filesystem::current_path();
            auto oldDatabase = app->getResourceDatabase(); auto oldLoader = app->getMeshLoader();
            auto oldMeshes = app->getMeshManager(); auto oldPrefabs = app->getPrefabManager();
            auto oldQueue = app->getJobQueue();
            guard.addCleanup( [app, database = database, loader = loader, meshes = meshes, prefabs = prefabs,
                               oldDatabase, oldLoader, oldMeshes, oldPrefabs, oldQueue, oldProject,
                               oldCache, oldSettings, oldRunning, oldQuit, oldDirectory, root = root]() mutable {
                loader->unload( nullptr ); prefabs->unload( nullptr ); meshes->unload( nullptr );
                auto catalog = database->getDatabaseManager();
                database->unload( nullptr ); if( catalog ) catalog->unload( nullptr );
                app->getFileSystem()->removeFileArchive( root + "/Cache" );
                app->getFileSystem()->removeFileArchive( root + "/SettingsCache" );
                app->getFileSystem()->removeFileArchive( root );
                app->setResourceDatabase( oldDatabase ); app->setMeshLoader( oldLoader );
                app->setMeshManager( oldMeshes ); app->setPrefabManager( oldPrefabs );
                app->setJobQueue( oldQueue ); app->setProjectPath( oldProject );
                app->setCachePath( oldCache ); app->setSettingsPath( oldSettings );
                app->setRunning( oldRunning ); app->setQuit( oldQuit );
                std::filesystem::current_path( oldDirectory );
            } );
            app->setProjectPath( root );
            app->setCachePath( ( directory / "Cache" ).generic_string() + "/" );
            app->setSettingsPath( ( directory / "SettingsCache" ).generic_string() + "/" );
            app->setRunning( true ); app->setQuit( false );
            // importFile drains inline without a queue; assertions never race an import worker.
            app->setJobQueue( nullptr );
            app->setResourceDatabase( database ); app->setMeshLoader( loader );
            app->setMeshManager( meshes ); app->setPrefabManager( prefabs );
            std::filesystem::current_path( directory );
            loader->load( nullptr ); prefabs->load( nullptr ); database->load( nullptr );
            auto catalog = dynamic_pointer_cast<AssetDatabaseManager>( database->getDatabaseManager() );
            BOOST_REQUIRE( catalog ); BOOST_REQUIRE( catalog->setProjectRoot( root ) );
            const auto catalogPath = ( directory / "Cache" / "asset.db" ).generic_string();
            database->setFilePath( catalogPath ); catalog->loadFromFile( catalogPath );
            BOOST_REQUIRE_MESSAGE( catalog->getDatabase() && catalog->getDatabase()->isLoaded(),
                                   "Mesh import integration requires a working database plugin" );
            writeObj(); guard.fileSystem->addFolder( root, true );
        }

        void writeObj()
        {
            std::ofstream output( source.c_str(), std::ios::trunc );
            const std::array<unsigned,3> faces[] = { {1,2,3}, {1,3,4}, {5,7,6}, {5,8,7},
                {1,5,6}, {1,6,2}, {4,3,7}, {4,7,8}, {1,4,8}, {1,8,5}, {2,6,7}, {2,7,3} };
            for( unsigned object = 0; object < 2; ++object )
            {
                output << "o Building" << object << '\n';
                const auto x = object * 1000;
                for( const auto &v : { Vector3F(0,0,0), Vector3F(8,0,0), Vector3F(8,12,0), Vector3F(0,12,0),
                                       Vector3F(0,0,8), Vector3F(8,0,8), Vector3F(8,12,8), Vector3F(0,12,8) } )
                    output << "v " << x + v.x << ' ' << v.y << ' ' << v.z << '\n';
                const auto first = object * 8;
                for( const auto &face : faces )
                    output << "f " << first + face[0] << ' ' << first + face[1] << ' ' << first + face[2] << '\n';
            }
            output.close(); BOOST_REQUIRE( output );
        }
        void saveDirector( bool enabled, bool enableHlod = false )
        {
            MeshResourceDirector director; director.setResourcePath( source );
            auto options = director.getStreamingOptions(); options.enabled = enabled;
            options.cellSize = 32; options.maxActorsPerCell = 8;
            options.loadRadius = 96; options.retainRadius = 128;
            options.enableHlod = enableHlod; options.previewEditorCamera = true; director.setStreamingOptions( options );
            const auto path = directory / "SettingsCache" /
                ( StringUtil::toString( StringUtil::getUUID( source ) ) + ".resourcedata" ).c_str();
            director.setFilePath( path.generic_string().c_str() );
            const auto data = director.getProperties();
            writeText( path.generic_string().c_str(), DataUtil::toString( data.get(), true, DataFormat::JSON ) );
            // Recursive addFolder creates one archive per directory. Refresh
            // this child archive after creating the settings file; refreshing
            // only the project root leaves findFileInfo's settings index stale.
            guard.fileSystem->refreshPath( ( directory / "SettingsCache" ).generic_string().c_str(), false );
            FileInfo settingsInfo;
            const auto relativeSettings = path.lexically_relative( directory ).generic_string();
            BOOST_REQUIRE_MESSAGE( guard.fileSystem->findFileInfo( relativeSettings.c_str(), settingsInfo ),
                                   "Saved import options must be indexed under their project-relative path" );
        }
        void import()
        {
            auto app = guard.applicationManager;
            BOOST_REQUIRE( database->isLoaded() ); BOOST_REQUIRE( app->isRunning() ); BOOST_REQUIRE( !app->getQuit() );
            BOOST_REQUIRE( app->getResourceDatabase() == database );
            guard.fileSystem->refreshPath( root, false );
            FileInfo sourceInfo;
            BOOST_REQUIRE_MESSAGE( guard.fileSystem->findFileInfo( source, sourceInfo ),
                                   "Source OBJ must be indexed before ResourceDatabase import" );
            database->importFile( source, true );
            BOOST_REQUIRE_MESSAGE( std::filesystem::is_regular_file( ApplicationUtil::getPrefabPath( source ).c_str() ),
                                   "Actual ResourceDatabase import did not publish its generated prefab" );
            guard.fileSystem->refreshPath( root, false );
        }
        SmartPtr<IGamePrefab> cached()
        { return dynamic_pointer_cast<IGamePrefab>( prefabs->loadResource( source ) ); }
        String manifestPath( const SmartPtr<Properties> &component ) const
        {
            const auto path = std::filesystem::path( component->getProperty( "World Manifest" ).c_str() );
            BOOST_REQUIRE_MESSAGE( !path.empty(), "Streaming wrapper must reference a world manifest" );
            BOOST_CHECK( !path.is_absolute() );
            return ( directory / path ).lexically_normal().generic_string().c_str();
        }
        void requireNativeWrapper( SmartPtr<IGamePrefab> prefab )
        {
            auto actor = prefab->createActor(); BOOST_REQUIRE( actor );
            guard.addCleanup( [manager = guard.sceneManager, actor]() mutable { manager->destroyActor( actor ); } );
            BOOST_REQUIRE( actor->getComponent<WorldStreaming>() );
            BOOST_CHECK( !actor->getComponent<scene::Mesh>() ); BOOST_CHECK( actor->getChildren().empty() );
        }

        TestGuard guard;
        std::filesystem::path directory;
        String root, source;
        SmartPtr<ResourceDatabase> database;
        SmartPtr<IMeshLoader> loader;
        SmartPtr<MeshManager> meshes;
        SmartPtr<GamePrefabManager> prefabs;
    };
}

BOOST_AUTO_TEST_SUITE( MeshImportStreamingIntegrationTests )

BOOST_AUTO_TEST_CASE( saved_default_off_import_keeps_the_detailed_prefab )
{
    using namespace mesh_import_streaming_integration_tests;
    Fixture fixture;
    BOOST_CHECK( !MeshResourceDirector::StreamingOptions{}.enabled );
    fixture.saveDirector( false ); fixture.import();
    auto prefab = fixture.cached(); BOOST_REQUIRE( prefab );
    auto data = dynamic_pointer_cast<Properties>( prefab->getData() ); BOOST_REQUIRE( data );
    BOOST_CHECK( !streamingComponent( data ) );
    bool streaming = false; data->getPropertyValue( "meshImportStreaming", streaming ); BOOST_CHECK( !streaming );
    u32 meshCount = 0;
    visit( *data, [&]( const Properties &p ) { if( p.hasProperty( "Mesh Path" ) ) ++meshCount; } );
    BOOST_CHECK_GT( meshCount, 0u );
    BOOST_CHECK( !std::filesystem::exists( fixture.directory / "Cache" / "Worlds" ) );
}

BOOST_AUTO_TEST_CASE( saved_opt_in_import_cooks_geometry_and_both_load_paths_use_the_thin_prefab )
{
    using namespace mesh_import_streaming_integration_tests;
    Fixture fixture; fixture.saveDirector( true ); fixture.import();
    auto cached = fixture.cached(); const auto component = requireWrapper( cached );
    float load = 0, retain = 0; component->getPropertyValue( "Load Radius", load );
    component->getPropertyValue( "Retain Radius", retain ); BOOST_CHECK_EQUAL( load, 96 ); BOOST_CHECK_EQUAL( retain, 128 );
    const auto path = fixture.manifestPath( component ); WorldManifest manifest; String error;
    BOOST_REQUIRE_MESSAGE( SceneWorld::readManifest( path, manifest, error ), error );
    BOOST_CHECK_EQUAL( manifest.cellSize, 32 );
    u32 spatial = 0, meshFiles = 0;
    for( const auto &cell : manifest.cells )
    {
        spatial += cell.id != "persistent" && !cell.detailKey;
        auto payload = SceneWorld::readCell( path, cell, error ); BOOST_REQUIRE_MESSAGE( payload, error );
        visit( *payload, [&]( const Properties &p ) {
            if( !p.hasProperty( "Mesh Path" ) ) return;
            const auto file = fixture.directory / p.getProperty( "Mesh Path" ).c_str();
            BOOST_REQUIRE_MESSAGE( std::filesystem::is_regular_file( file ), "Cooked geometry must exist: " << file.string() );
            BOOST_CHECK_GT( std::filesystem::file_size( file ), 0u ); ++meshFiles;
        } );
    }
    BOOST_CHECK_GE( spatial, 2u ); BOOST_CHECK_GE( meshFiles, 2u );
    // A direct source preview must still load the published wrapper when reparsing the source would fail.
    writeText( fixture.source, "invalid source after successful import\n" );
    fixture.guard.fileSystem->refreshPath( fixture.root, false );
    cached = fixture.cached(); requireWrapper( cached ); fixture.requireNativeWrapper( cached );
    auto direct = fixture.prefabs->loadPrefab( fixture.source );
    const auto directComponent = requireWrapper( direct );
    BOOST_CHECK_EQUAL( directComponent->getProperty( "World Manifest" ), component->getProperty( "World Manifest" ) );
    fixture.requireNativeWrapper( direct );
}

BOOST_AUTO_TEST_CASE( failed_source_reimport_preserves_the_published_streaming_generation )
{
    using namespace mesh_import_streaming_integration_tests;
    Fixture fixture; fixture.saveDirector( true ); fixture.import();
    const auto component = requireWrapper( fixture.cached() );
    const auto prefabPath = ApplicationUtil::getPrefabPath( fixture.source );
    const auto manifestPath = fixture.manifestPath( component );
    const auto prefabBefore = readBytes( prefabPath ), manifestBefore = readBytes( manifestPath );
    WorldManifest before; String error; BOOST_REQUIRE( SceneWorld::readManifest( manifestPath, before, error ) );
    writeText( fixture.source, "this is not a mesh\n" ); fixture.import();
    BOOST_CHECK_EQUAL( readBytes( prefabPath ), prefabBefore );
    BOOST_CHECK_EQUAL( readBytes( manifestPath ), manifestBefore );
    const auto retained = requireWrapper( fixture.prefabs->loadPrefab( fixture.source ) );
    BOOST_CHECK_EQUAL( fixture.manifestPath( retained ), manifestPath );
    WorldManifest after; BOOST_REQUIRE_MESSAGE( SceneWorld::readManifest( manifestPath, after, error ), error );
    BOOST_CHECK_EQUAL( after.generation, before.generation );
    for( const auto &cell : after.cells ) BOOST_CHECK_MESSAGE( SceneWorld::readCell( manifestPath, cell, error ), error );
}

BOOST_AUTO_TEST_CASE( automatic_hlod_import_catalogs_generated_proxy_meshes_before_prefab_publication )
{
    using namespace mesh_import_streaming_integration_tests;
    Fixture fixture; fixture.saveDirector( true, true );
    BOOST_REQUIRE( !fixture.guard.applicationManager->getJobQueue() ); fixture.import();
    const auto component = requireWrapper( fixture.cached() );
    const auto path = fixture.manifestPath( component ); WorldManifest manifest; String error;
    BOOST_REQUIRE_MESSAGE( SceneWorld::readManifest( path, manifest, error ), error );
    auto catalog = dynamic_pointer_cast<AssetDatabaseManager>( fixture.database->getDatabaseManager() );
    BOOST_REQUIRE( catalog ); u32 proxies = 0, meshes = 0; std::set<String> folders;
    for( const auto &cell : manifest.cells )
    {
        if( !cell.detailKey ) continue;
        ++proxies;
        auto payload = SceneWorld::readCell( path, cell, error ); BOOST_REQUIRE_MESSAGE( payload, error );
        visit( *payload, [&]( const Properties &properties ) {
            if( !properties.hasProperty( "Mesh Path" ) ) return;
            const auto meshPath = properties.getPropertyObject( "Mesh Path" ).getValue(); ++meshes;
            const auto folder = String( ( fixture.directory / meshPath.c_str() ).parent_path().generic_string().c_str() );
            if( folders.insert( folder ).second )
                fixture.guard.addCleanup( [files = fixture.guard.fileSystem, folder]() mutable {
                    files->removeFileArchive( folder );
                } );
            FileInfo meshInfo;
            BOOST_REQUIRE_MESSAGE( fixture.guard.fileSystem->findFileInfo( meshPath, meshInfo ),
                                   "Generated HLOD mesh must be indexed by its project-relative path: " << meshPath );
            BOOST_REQUIRE_MESSAGE( catalog->getResourceEntryFromPath( meshPath ),
                                   "Generated HLOD mesh must already have a catalog identity: " << meshPath );
            auto mesh = fixture.database->loadResourceByType<IMeshResource>( meshPath );
            BOOST_REQUIRE_MESSAGE( mesh, "Normal resource lookup must resolve generated HLOD mesh: " << meshPath );
            BOOST_CHECK( mesh->isLoaded() ); auto body = mesh->getMesh(); BOOST_REQUIRE( body );
            const auto subMeshes = body->getSubMeshes(); BOOST_REQUIRE( !subMeshes.empty() );
            auto vertices = subMeshes.front()->getVertexBuffer(); BOOST_REQUIRE( vertices );
            BOOST_CHECK_GT( vertices->getNumVertices(), 0u );
        } );
    }
    BOOST_CHECK_GT( proxies, 0u ); BOOST_CHECK_GE( meshes, proxies );
}

BOOST_AUTO_TEST_SUITE_END()
#endif
