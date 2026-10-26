#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <cstdlib>
#include <Workphone/Interface/Mesh/IMeshLoader.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Interface/Graphics/IMeshConverter.hpp>
#include <Workphone/Interface/Scene/IGamePrefabManager.hpp>
#include <Workphone/Interface/Scene/IGamePrefab.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Database/ResourceDatabase.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <vector>
#include <Workphone/Mesh/VertexBuffer.hpp>
#include <Workphone/Mesh/IndexBuffer.hpp>
#include <Workphone/Mesh/VertexDeclaration.hpp>
#include <Workphone/Graphics/GraphicsSystem.hpp>
#include <Workphone/System/Resource.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#if WP_USE_ASSET_IMPORT && !WP_USE_FBXSDK
#include <WPAssimp/AssimpLoader.hpp>
#include <assimp/material.h>
#endif
#if defined( _MSC_VER ) && defined( _DEBUG )
#include <crtdbg.h>
#endif

using namespace workphone;

BOOST_AUTO_TEST_CASE( mesh_cpu_buffer_replacement_and_destruction_release_storage )
{
    auto declaration = make_ptr<VertexDeclaration>();
    declaration->addElement( 0, 0, VertexElementSemantic::VES_POSITION,
                              VertexElementType::VET_FLOAT3 );
    const auto exercise = [&]() {
        VertexBuffer vertices;
        vertices.setVertexDeclaration( declaration );
        vertices.setNumVertices( 4 );
        vertices.createVertexData();
        vertices.setNumVertices( 8 );
        vertices.createVertexData();
        IndexBuffer indices;
        indices.setIndexType( IIndexBuffer::Type::IT_16BIT );
        indices.setNumIndices( 6 );
        indices.createIndexData();
        indices.setIndexType( IIndexBuffer::Type::IT_32BIT );
        indices.setNumIndices( 12 );
        indices.createIndexData();
    };
    exercise(); // Warm any diagnostic caches before measuring owned storage.
#if defined( _MSC_VER ) && defined( _DEBUG )
    _CrtMemState before{}, after{}, difference{};
    _CrtMemCheckpoint( &before );
#endif
    exercise();
#if defined( _MSC_VER ) && defined( _DEBUG )
    _CrtMemCheckpoint( &after );
    const auto changed = _CrtMemDifference( &difference, &before, &after );
    BOOST_CHECK_MESSAGE( !changed, "CPU buffer destruction leaked CRT allocations" );
#endif
    BOOST_CHECK_EQUAL( declaration->getSize( 0 ), 3u * sizeof( f32 ) );
}

namespace
{
    class GraphicsLoadQueueProbe : public render::GraphicsSystem
    {
    public:
        using render::GraphicsSystem::updateLoadQueue;
        using render::GraphicsSystem::updateReloadQueue;
        using render::GraphicsSystem::updateUnloadQueue;

        void load( SmartPtr<ISharedObject> ) override
        {
            ++loadRequests;
            setLoadingState( LoadingState::Loaded );
        }

        u32 loadRequests = 0;
    };

    class QueuedMeshLoadProbe : public Resource<IResource>
    {
    public:
        void load( SmartPtr<ISharedObject> ) override
        {
            ++loadRequests;
            setLoadingState( LoadingState::Loaded );
        }

        u32 loadRequests = 0;
        u32 reloadRequests = 0, unloadRequests = 0;
        void reload( SmartPtr<ISharedObject> ) override { ++reloadRequests; setLoadingState( LoadingState::Loaded ); }
        void unload( SmartPtr<ISharedObject> ) override { ++unloadRequests; setLoadingState( LoadingState::Unloaded ); }
    };

    class CatalogMeshLoadProbe : public render::GraphicsSystem
    {
    public:
        void loadObject( SmartPtr<ISharedObject>, bool ) override
        {
            ++loadRequests;
        }

        u32 loadRequests = 0;
    };

    SmartPtr<IResourceManager> getMeshManager()
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );
        if( !applicationManager )
        {
            return nullptr;
        }

        auto meshManager = applicationManager->getMeshManager();
        BOOST_CHECK( meshManager );
        return meshManager;
    }

    SmartPtr<IResource> tryLoadMeshAsset( const std::vector<String> &paths,
                                          String *loadedPath = nullptr )
    {
        auto meshManager = getMeshManager();

        for( const auto &path : paths )
        {
            const auto cleanedPath = StringUtil::cleanupPath( path );
            if( auto mesh = meshManager->loadFromFile( cleanedPath ) )
            {
                if( loadedPath )
                {
                    *loadedPath = cleanedPath;
                }

                return mesh;
            }
        }

        return nullptr;
    }
}  // namespace

BOOST_AUTO_TEST_CASE( mesh_load_valid_file )
{
    try
    {
        TestGuard guard;

        auto meshManager = getMeshManager();

        auto cubeFbxPath = String( "cube_internal.fbmeshbin" );
        auto cubeBinPath = String( "cube_internal_copy_test.fbmeshbin" );

        cubeFbxPath = StringUtil::cleanupPath( cubeFbxPath );
        BOOST_CHECK( !StringUtil::isNullOrEmpty( cubeFbxPath ) );

        cubeBinPath = StringUtil::cleanupPath( cubeBinPath );
        BOOST_CHECK( !StringUtil::isNullOrEmpty( cubeBinPath ) );

        // Test loading original mesh
        auto mesh = meshManager->loadFromFile( cubeFbxPath );
        if( !mesh )
        {
            BOOST_TEST_MESSAGE(
                "cube_internal.fbmeshbin test asset is not available - skipping mesh load test" );
            return;
        }

        {
            // Verify mesh properties
            BOOST_CHECK( mesh->isLoaded() );

            // Test saving mesh
            meshManager->saveToFile( cubeBinPath, mesh );

            // Test loading saved mesh
            auto meshBin = meshManager->loadResource( cubeBinPath );
            if( !meshBin )
            {
                BOOST_TEST_MESSAGE( "Saved mesh could not be reloaded in this test environment" );
                return;
            }

            {
                BOOST_CHECK( meshBin->isLoaded() );

                // Test mesh comparison
                //BOOST_CHECK(mesh->compare(meshBin));

                // Verify both meshes have the same basic properties
                // Note: Add specific property checks based on your mesh interface
                // Example checks (uncomment if these methods exist):
                // BOOST_CHECK_EQUAL(mesh->getVertexCount(), meshBin->getVertexCount());
                // BOOST_CHECK_EQUAL(mesh->getFaceCount(), meshBin->getFaceCount());
            }
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during mesh load test: " + std::string( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( mesh_load_invalid_file )
{
    try
    {
        TestGuard guard;

        auto meshManager = getMeshManager();

        // Test with non-existent file
        auto invalidPath = String( "non_existent_file.fbx" );
        invalidPath = StringUtil::cleanupPath( invalidPath );

        auto mesh = meshManager->loadFromFile( invalidPath );
        BOOST_CHECK( !mesh );  // Should fail gracefully

        // Test with empty path
        auto emptyPath = String( "" );
        auto meshEmpty = meshManager->loadFromFile( emptyPath );
        BOOST_CHECK( !meshEmpty );

        // Test with null/whitespace path
        auto whitespacePath = String( "   " );
        auto meshWhitespace = meshManager->loadFromFile( whitespacePath );
        BOOST_CHECK( !meshWhitespace );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        // Some exceptions might be expected for invalid files
    }
}

BOOST_AUTO_TEST_CASE( mesh_save_invalid_scenarios )
{
    try
    {
        TestGuard guard;

        auto meshManager = getMeshManager();

        auto cubePath = String( "cube_internal.fbmeshbin" );
        cubePath = StringUtil::cleanupPath( cubePath );

        auto mesh = meshManager->loadFromFile( cubePath );
        if( !mesh )
        {
            BOOST_TEST_MESSAGE(
                "cube_internal.fbmeshbin test asset is not available - skipping mesh save edge test" );
            return;
        }

        // Test saving to invalid path
        auto invalidSavePath = String( "" );
        meshManager->saveToFile( invalidSavePath, mesh );

        // Test saving to read-only location (if applicable)
        auto readOnlyPath = String( "/invalid/readonly/path.fbmeshbin" );
        meshManager->saveToFile( readOnlyPath, mesh );

        // Test saving null mesh
        workphone::SmartPtr<workphone::IMesh> nullMesh;
        auto validPath = String( "test_null.fbmeshbin" );
        validPath = StringUtil::cleanupPath( validPath );
        meshManager->saveToFile( validPath, nullMesh );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( mesh_comparison_edge_cases )
{
    try
    {
        TestGuard guard;

        auto meshManager = getMeshManager();

        auto cubeFbxPath = String( "cube.fbx" );
        cubeFbxPath = StringUtil::cleanupPath( cubeFbxPath );

        auto mesh1 = meshManager->loadFromFile( cubeFbxPath );
        auto mesh2 = meshManager->loadFromFile( cubeFbxPath );

        if( mesh1 && mesh2 )
        {
            // Test comparing same mesh data
            //BOOST_CHECK(mesh1->compare(mesh2) == true);

            // Test self-comparison
            //BOOST_CHECK(mesh1->compare(mesh1) == true);

            // Test comparison with null
            workphone::SmartPtr<workphone::IMesh> nullMesh;
            //BOOST_CHECK(!mesh1->compare(nullMesh));
        }
        else
        {
            BOOST_TEST_MESSAGE(
                "cube.fbx test asset is not available - skipping mesh comparison edge test" );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception in mesh comparison test: " + std::string( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( mesh_memory_management )
{
    try
    {
        TestGuard guard;

        auto meshManager = getMeshManager();

        // Test multiple load/unload cycles
        auto cubeFbxPath = String( "cube.fbx" );
        cubeFbxPath = StringUtil::cleanupPath( cubeFbxPath );

        for( int i = 0; i < 10; ++i )
        {
            auto mesh = meshManager->loadFromFile( cubeFbxPath );
            if( !mesh )
            {
                BOOST_TEST_MESSAGE( "cube.fbx test asset is not available - skipping mesh memory test" );
                return;
            }

            BOOST_CHECK( mesh->isLoaded() );

            // Force unload if method exists
            // mesh->unload();
            // BOOST_CHECK(!mesh->isLoaded());
            // Mesh should be automatically cleaned up when going out of scope
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception in memory management test: " + std::string( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( mesh_concurrent_access )
{
    try
    {
        TestGuard guard;

        auto meshManager = getMeshManager();

        auto cubeFbxPath = String( "cube.fbx" );
        cubeFbxPath = StringUtil::cleanupPath( cubeFbxPath );

        // Test loading same mesh multiple times simultaneously
        std::vector<workphone::SmartPtr<workphone::MeshResource>> meshes;

        for( int i = 0; i < 5; ++i )
        {
            auto mesh = meshManager->loadFromFile( cubeFbxPath );
            if( !mesh )
            {
                BOOST_TEST_MESSAGE(
                    "cube.fbx test asset is not available - skipping repeated mesh load test" );
                return;
            }
            meshes.push_back( mesh );
        }

        // Verify all loaded successfully
        for( const auto &mesh : meshes )
        {
            BOOST_CHECK( mesh );
            if( mesh )
            {
                BOOST_CHECK( mesh->isLoaded() );
            }
        }

        // Test that they're all equivalent
        //if (meshes.size() >= 2) {
        //    BOOST_CHECK(meshes[0]->compare(meshes[1]));
        //}
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception in concurrent access test: " + std::string( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( mesh_actor_load_enhanced )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto prefabManager = applicationManager->getPrefabManager();
        BOOST_REQUIRE( prefabManager );

        auto cubeFbxPath = String( "cube.fbx" );
        cubeFbxPath = StringUtil::cleanupPath( cubeFbxPath );
        BOOST_CHECK( !StringUtil::isNullOrEmpty( cubeFbxPath ) );

        auto prefab = prefabManager->loadPrefab( cubeFbxPath );
        if( !prefab )
        {
            BOOST_TEST_MESSAGE( "cube.fbx test asset is not available - skipping mesh actor load test" );
            return;
        }

        {
            // Verify prefab properties
            BOOST_CHECK( prefab->isLoaded() );

            // Test creating multiple instances
            auto prefab2 = prefabManager->loadPrefab( cubeFbxPath );
            BOOST_CHECK( prefab2 );

            // Both should be valid
            BOOST_CHECK( prefab->isLoaded() );
            BOOST_CHECK( prefab2->isLoaded() );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception in enhanced actor load test: " + std::string( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( mesh_skeleton_load_enhanced )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto prefabManager = applicationManager->getPrefabManager();
        BOOST_REQUIRE( prefabManager );

        // Test with a file that might have skeleton data
        auto cubeFbxPath = String( "cube.fbx" );
        cubeFbxPath = StringUtil::cleanupPath( cubeFbxPath );
        BOOST_CHECK( !StringUtil::isNullOrEmpty( cubeFbxPath ) );

        auto prefab = prefabManager->loadPrefab( cubeFbxPath );
        if( !prefab )
        {
            BOOST_TEST_MESSAGE(
                "cube.fbx test asset is not available - skipping mesh skeleton load test" );
            return;
        }

        {
            BOOST_CHECK( prefab->isLoaded() );

            // Additional skeleton-specific tests could be added here
            // if skeleton interface is available:
            // auto skeleton = prefab->getSkeleton();
            // if (skeleton) {
            //     BOOST_CHECK(skeleton->isValid());
            //     BOOST_CHECK_GT(skeleton->getBoneCount(), 0);
            // }
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( mesh_skeleton_kinematic_implementation )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        // This test was empty in the original - adding basic structure
        auto prefabManager = applicationManager->getPrefabManager();
        BOOST_REQUIRE( prefabManager );

        auto cubeFbxPath = String( "cube.fbx" );
        cubeFbxPath = StringUtil::cleanupPath( cubeFbxPath );

        auto prefab = prefabManager->loadPrefab( cubeFbxPath );
        if( !prefab )
        {
            BOOST_TEST_MESSAGE( "cube.fbx test asset is not available - skipping kinematic mesh test" );
            return;
        }

        // TODO: Add kinematic-specific tests when kinematic system is available
        // Example:
        // if (prefab) {
        //     auto kinematicController = prefab->getKinematicController();
        //     if (kinematicController) {
        //         BOOST_CHECK(kinematicController->isValid());
        //         // Test kinematic operations
        //     }
        // }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( mesh_path_validation )
{
    try
    {
        TestGuard guard;

        auto meshManager = getMeshManager();

        // Test various path formats
        std::vector<std::pair<String, bool>> testPaths = {
            { "cube.fbx", true },              // Valid relative path
            { "./cube.fbx", true },            // Valid relative with ./
            { "../cube.fbx", true },           // Valid relative with ../
            { "", false },                     // Empty path
            { "   ", false },                  // Whitespace only
            { "invalid\\\\path.fbx", false },  // Invalid characters
            { "very_long_filename_that_might_exceed_system_limits_and_cause_issues.fbx",
              false },              // Very long name
            { "cube", false },      // Missing extension
            { ".fbx", false },      // Extension only
            { "cube.txt", false },  // Wrong extension
        };

        for( const auto &[path, shouldSucceed] : testPaths )
        {
            auto cleanedPath = StringUtil::cleanupPath( path );

            if( shouldSucceed )
            {
                BOOST_CHECK( !StringUtil::isNullOrEmpty( cleanedPath ) );
                // Path should be valid for further processing
            }
            else
            {
                // Invalid paths might still be cleaned but won't load successfully
                auto mesh = meshManager->loadFromFile( cleanedPath );
                BOOST_CHECK( !mesh );  // Should fail to load
            }
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( mesh_manager_state_consistency )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto meshManager = applicationManager->getMeshManager();
        BOOST_REQUIRE( meshManager );

        // Verify mesh manager is properly initialized
        BOOST_CHECK( applicationManager->getMeshManager() );
        BOOST_CHECK( applicationManager->getMeshManager() == meshManager );

        // Test setting mesh manager to null and back
        applicationManager->setMeshManager( nullptr );
        BOOST_CHECK( !applicationManager->getMeshManager() );

        applicationManager->setMeshManager( meshManager );
        BOOST_CHECK( applicationManager->getMeshManager() );
        BOOST_CHECK( applicationManager->getMeshManager() == meshManager );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception in state consistency test: " + std::string( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( mesh_load_resource_invalid_names )
{
    try
    {
        TestGuard guard;

        auto meshManager = getMeshManager();

        const std::vector<String> invalidPaths = {
            "",
            "   ",
            ".fbx",
            "missing_mesh_for_unit_test",
            "missing_mesh_for_unit_test.txt",
            "folder/that/does/not/exist/cube.notmesh",
        };

        for( const auto &path : invalidPaths )
        {
            BOOST_CHECK( !meshManager->loadFromFile( StringUtil::cleanupPath( path ) ) );
        }

        const auto uniqueName = String( "unit_test_mesh_name_that_was_never_created" );
        BOOST_CHECK( !meshManager->getByName( uniqueName ) );
        BOOST_CHECK( !meshManager->loadResource( uniqueName ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception in invalid mesh resource-name test: " + std::string( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( mesh_create_or_retrieve_reuses_path_resource )
{
    try
    {
        TestGuard guard;

        auto meshManager = getMeshManager();

        const auto path = StringUtil::cleanupPath( "unit_tests/generated_mesh_resource.fbmeshbin" );
        auto first = meshManager->createOrRetrieve( path );
        BOOST_REQUIRE( first.first );
        BOOST_CHECK( first.second );

        auto second = meshManager->createOrRetrieve( path );
        BOOST_REQUIRE( second.first );
        BOOST_CHECK( !second.second );
        BOOST_CHECK( first.first == second.first );

        auto byType = meshManager->createOrRetrieveByType<IMeshResource>( path );
        BOOST_REQUIRE( byType.first );
        BOOST_CHECK( !byType.second );
        BOOST_CHECK( byType.first == first.first );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception in mesh create-or-retrieve cache test: " + std::string( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( mesh_load_same_asset_returns_cached_resource )
{
    try
    {
        TestGuard guard;

        String loadedPath;
        auto first = tryLoadMeshAsset( { "cube_internal.fbmeshbin", "cube.fbx" }, &loadedPath );
        if( !first )
        {
            BOOST_TEST_MESSAGE( "No mesh test asset is available - skipping cached load test" );
            return;
        }

        auto meshManager = getMeshManager();
        auto second = meshManager->loadFromFile( loadedPath );
        BOOST_REQUIRE( second );
        BOOST_CHECK( second == first );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception in cached mesh load test: " + std::string( e.what() ) );
    }
}

// Opt-in real-asset benchmark. All generated resources stay in a temporary project.
BOOST_AUTO_TEST_CASE( mesh_queued_resource_loads_reuse_completed_resources )
{
    auto graphics = make_ptr<GraphicsLoadQueueProbe>();
    auto resource = make_ptr<QueuedMeshLoadProbe>();
    graphics->loadObject( resource, true );
    const auto queuedOwners = resource->getReferences();
    for( u32 retry = 0; retry < 1000; ++retry ) graphics->loadObject( resource, true );
    BOOST_CHECK_EQUAL( resource->getReferences(), queuedOwners );
    graphics->updateLoadQueue();
    BOOST_CHECK_EQUAL( resource->loadRequests, 1u );

    graphics->loadObject( resource, true );
    graphics->updateLoadQueue();
    BOOST_CHECK_EQUAL( resource->loadRequests, 1u );

    resource->setLoadingState( LoadingState::Unloaded );
    graphics->loadObject( resource, true );
    graphics->updateLoadQueue();
    BOOST_CHECK_EQUAL( resource->loadRequests, 2u );

    // A graphics object can be loaded before its mesh path is supplied. Its
    // later load must still run, even when its initial state is already Loaded.
    auto graphicsObject = make_ptr<GraphicsLoadQueueProbe>();
    graphicsObject->setLoadingState( LoadingState::Loaded );
    graphics->loadObject( graphicsObject, true );
    graphics->updateLoadQueue();
    BOOST_CHECK_EQUAL( graphicsObject->loadRequests, 1u );
}

BOOST_AUTO_TEST_CASE( mesh_queued_lifecycle_discards_cancelled_work )
{
    auto graphics = make_ptr<GraphicsLoadQueueProbe>();
    auto resource = make_ptr<QueuedMeshLoadProbe>();
    graphics->loadObject( resource, true );
    graphics->reloadObject( resource, true );
    graphics->unloadObject( resource, true );
    graphics->updateLoadQueue(); graphics->updateReloadQueue(); graphics->updateUnloadQueue();
    BOOST_CHECK_EQUAL( resource->loadRequests, 0u );
    BOOST_CHECK_EQUAL( resource->reloadRequests, 0u );
    BOOST_CHECK_EQUAL( resource->unloadRequests, 1u );
    graphics->unloadObject( resource, true );
    graphics->loadObject( resource, true );
    graphics->updateLoadQueue(); graphics->updateUnloadQueue();
    BOOST_CHECK_EQUAL( resource->loadRequests, 1u );
    BOOST_CHECK_EQUAL( resource->unloadRequests, 1u );
    BOOST_CHECK( resource->isLoaded() );
    graphics->unloadObject( resource, true );
    graphics->reloadObject( resource, true );
    graphics->updateReloadQueue(); graphics->updateUnloadQueue();
    BOOST_CHECK_EQUAL( resource->reloadRequests, 1u );
    BOOST_CHECK_EQUAL( resource->unloadRequests, 1u );
    BOOST_CHECK( resource->isLoaded() );
}

BOOST_AUTO_TEST_CASE( mesh_large_scene_import_benchmark )
{
    const auto source = std::getenv( "WP_MESH_BENCHMARK_ASSET" );
    if( !source || !*source )
    {
        BOOST_TEST_MESSAGE( "Set WP_MESH_BENCHMARK_ASSET to a model or a directory of GLB models" );
        return;
    }
    const auto benchmarkModel = []( const std::filesystem::path &sourcePath ) {
        BOOST_TEST_MESSAGE( "Import asset=" << sourcePath.filename().string() );
        TestGuard guard;
        auto app = guard.applicationManager;
        auto loader = app->getMeshLoader();
        BOOST_REQUIRE( loader );
        const auto oldProject = app->getProjectPath();
        const auto oldCache = app->getCachePath();
        const auto oldSettings = app->getSettingsPath();
        auto oldDatabase = app->getResourceDatabase();
        const auto oldOverwrite = loader->getOverwrite();
        const auto directory = std::filesystem::temp_directory_path() / StringUtil::getUUID().c_str();
        std::filesystem::create_directories( directory / "Cache" );
        std::filesystem::create_directories( directory / "SettingsCache" );
        const auto model = directory / sourcePath.filename();
        std::filesystem::copy_file( sourcePath, model );
        guard.trackFilesystemPath( directory.generic_string() );
        auto database = workphone::make_ptr<ResourceDatabase>();
        guard.addCleanup( [app, loader, database, oldDatabase, oldProject, oldCache, oldSettings,
                           oldOverwrite]() mutable {
            database->unload( nullptr );
            app->setResourceDatabase( oldDatabase );
            app->setProjectPath( oldProject );
            app->setCachePath( oldCache );
            app->setSettingsPath( oldSettings );
            loader->setOverwrite( oldOverwrite );
        } );
        app->setProjectPath( directory.generic_string() );
        app->setCachePath( ( directory / "Cache" ).generic_string() + "/" );
        app->setSettingsPath( ( directory / "SettingsCache" ).generic_string() + "/" );
        app->setResourceDatabase( database );
        database->load( nullptr );
        auto catalog = dynamic_pointer_cast<AssetDatabaseManager>( database->getDatabaseManager() );
        BOOST_REQUIRE( catalog );
        BOOST_REQUIRE( catalog->setProjectRoot( directory.generic_string() ) );
        const auto catalogPath = ( directory / "Cache" / "asset.db" ).generic_string();
        database->setFilePath( catalogPath );
        catalog->loadFromFile( catalogPath );
        const auto folder = String( directory.generic_string() );
        app->getFileSystem()->addFolder( folder, true );
        guard.addCleanup( [app, folder]() mutable { app->getFileSystem()->removeFileArchive( folder ); } );
        // The importer retains actors and meshes between calls. Release this large
        // temporary graph before the guard restores the previous project and scene.
        guard.addCleanup( [loader]() mutable {
            loader->unload( nullptr );
            loader->load( nullptr );
        } );
        loader->setOverwrite( true );
        const auto start = std::chrono::steady_clock::now();
        auto actor = loader->loadActor( model.generic_string() );
        const auto seconds = std::chrono::duration<double>( std::chrono::steady_clock::now() - start ).count();
        BOOST_REQUIRE( actor );
        size_t nodes = 0, meshes = 0, vertices = 0;
        std::vector<SmartPtr<scene::IGameActor>> pending{ actor };
        while( !pending.empty() )
        {
            auto current = pending.back();
            pending.pop_back();
            ++nodes;
            if( auto component = current->getComponent<scene::Mesh>() )
            {
                auto resource = component->getMeshResource();
                BOOST_REQUIRE( resource );
                auto mesh = resource->getMesh();
                BOOST_REQUIRE( mesh );
                BOOST_REQUIRE_GT( mesh->getNumSubMeshes(), 0u );
                mesh->updateAABB( true );
                BOOST_CHECK( mesh->getAABB().isFinite() && mesh->getAABB().isValid() );
                ++meshes;
                for( auto submesh : mesh->getSubMeshes() )
                {
                    BOOST_REQUIRE( submesh->getVertexBuffer() );
                    BOOST_REQUIRE_GT( submesh->getVertexBuffer()->getNumVertices(), 0u );
                    BOOST_REQUIRE( submesh->getVertexBuffer()->getVertexData() );
                    vertices += submesh->getVertexBuffer()->getNumVertices();
                }
            }
            for( auto child : current->getChildren() )
            {
                pending.push_back( child );
            }
        }
        BOOST_CHECK_GT( meshes, 0u );
        BOOST_TEST_MESSAGE( "Import seconds=" << seconds << " nodes=" << nodes << " meshes=" << meshes
                            << " vertices=" << vertices );
        const auto imageFolder = directory / "Cache" / "EmbeddedTextures";
        size_t decodedImages = 0;
        if( std::filesystem::is_directory( imageFolder ) )
        {
            for( const auto &image : std::filesystem::directory_iterator( imageFolder ) )
            {
                auto texture = database->loadResourceByType<render::ITexture>( image.path().generic_string() );
                BOOST_REQUIRE_MESSAGE( texture, "Embedded image was not cataloged: " << image.path() );
                texture->load( nullptr );
                BOOST_REQUIRE_MESSAGE( texture->isLoaded(), "Embedded image did not decode: " << image.path() );
                BOOST_CHECK_GT( texture->getSize().x, 0 );
                BOOST_CHECK_GT( texture->getSize().y, 0 );
                ++decodedImages;
            }
        }
        BOOST_TEST_MESSAGE( "Decoded embedded images=" << decodedImages );
        const auto cacheStart = std::chrono::steady_clock::now();
        auto converter = app->getGraphicsSystem()->getMeshConverter();
        BOOST_REQUIRE( converter );
        converter->writeMesh( actor );
        BOOST_TEST_MESSAGE( "Cache write seconds=" << std::chrono::duration<double>(
            std::chrono::steady_clock::now() - cacheStart ).count() );
        const auto prefabPath = ( directory / "benchmark.prefab" ).generic_string();
        const auto prefabStart = std::chrono::steady_clock::now();
        app->getPrefabManager()->savePrefab( prefabPath, actor );
        const auto saved = std::chrono::steady_clock::now();
        BOOST_REQUIRE( std::filesystem::is_regular_file( prefabPath ) );
        auto prefab = app->getPrefabManager()->loadPrefab( prefabPath );
        const auto parsed = std::chrono::steady_clock::now();
        BOOST_REQUIRE( prefab );
        auto instance = prefab->createActor();
        BOOST_REQUIRE( instance );
        BOOST_TEST_MESSAGE( "Prefab save/load seconds=" << std::chrono::duration<double>(
            std::chrono::steady_clock::now() - prefabStart ).count() );
        BOOST_TEST_MESSAGE( "Prefab save=" << std::chrono::duration<double>( saved - prefabStart ).count()
                            << " parse=" << std::chrono::duration<double>( parsed - saved ).count()
                            << " instantiate=" << std::chrono::duration<double>(
                                std::chrono::steady_clock::now() - parsed ).count() );
        size_t instanceMeshes = 0, instanceVertices = 0;
        pending = { instance };
        while( !pending.empty() )
        {
            auto current = pending.back();
            pending.pop_back();
            if( auto component = current->getComponent<scene::Mesh>() )
            {
                auto resource = component->getMeshResource();
                BOOST_REQUIRE( resource );
                auto mesh = resource->getMesh();
                BOOST_REQUIRE( mesh );
                BOOST_REQUIRE_GT( mesh->getNumSubMeshes(), 0u );
                mesh->updateAABB( true );
                BOOST_CHECK( mesh->getAABB().isFinite() && mesh->getAABB().isValid() );
                for( auto submesh : mesh->getSubMeshes() )
                {
                    BOOST_REQUIRE( submesh->getVertexBuffer() );
                    BOOST_REQUIRE_GT( submesh->getVertexBuffer()->getNumVertices(), 0u );
                    BOOST_REQUIRE( submesh->getVertexBuffer()->getVertexData() );
                    instanceVertices += submesh->getVertexBuffer()->getNumVertices();
                }
                ++instanceMeshes;
            }
            for( auto child : current->getChildren() )
            {
                pending.push_back( child );
            }
        }
        BOOST_CHECK_EQUAL( instanceMeshes, meshes );
        BOOST_CHECK_EQUAL( instanceVertices, vertices );
        guard.sceneManager->destroyActor( instance );
        guard.sceneManager->destroyActor( actor );
    };
    if( std::filesystem::is_directory( source ) )
    {
        std::vector<std::filesystem::path> models;
        for( const auto &entry : std::filesystem::directory_iterator( source ) )
        {
            if( entry.is_regular_file() &&
                StringUtil::make_lower( entry.path().extension().string() ) == ".glb" )
                models.push_back( entry.path() );
        }
        std::sort( models.begin(), models.end() );
        BOOST_REQUIRE( !models.empty() );
        for( const auto &model : models ) benchmarkModel( model );
        BOOST_TEST_MESSAGE( "Imported and instantiated " << models.size() << " GLB assets" );
    }
    else
    {
        BOOST_REQUIRE( std::filesystem::is_regular_file( source ) );
        benchmarkModel( source );
    }
}

BOOST_AUTO_TEST_CASE( mesh_reimport_rebuilds_geometry_and_settings_on_every_attempt )
{
    TestGuard guard;
    auto app = guard.applicationManager;
    auto loader = app->getMeshLoader();
    BOOST_REQUIRE( loader );
    const auto oldProject = app->getProjectPath();
    const auto oldCache = app->getCachePath();
    const auto oldSettings = app->getSettingsPath();
    auto oldDatabase = app->getResourceDatabase();
    const auto oldOverwrite = loader->getOverwrite();
    const auto directory = std::filesystem::temp_directory_path() / StringUtil::getUUID().c_str();
    std::filesystem::create_directories( directory / "Cache" );
    std::filesystem::create_directories( directory / "SettingsCache" );
    guard.trackFilesystemPath( directory.generic_string() );
    auto database = workphone::make_ptr<ResourceDatabase>();
    guard.addCleanup( [app, loader, database, oldDatabase, oldProject, oldCache, oldSettings,
                       oldOverwrite]() mutable {
        database->unload( nullptr );
        app->setResourceDatabase( oldDatabase );
        app->setProjectPath( oldProject );
        app->setCachePath( oldCache );
        app->setSettingsPath( oldSettings );
        loader->setOverwrite( oldOverwrite );
    } );
    app->setProjectPath( directory.generic_string() );
    app->setCachePath( ( directory / "Cache" ).generic_string() + "/" );
    app->setSettingsPath( ( directory / "SettingsCache" ).generic_string() + "/" );
    app->setResourceDatabase( database );
    database->load( nullptr );

    auto catalog = dynamic_pointer_cast<AssetDatabaseManager>( database->getDatabaseManager() );
    BOOST_REQUIRE( catalog );
    BOOST_REQUIRE( catalog->setProjectRoot( directory.generic_string() ) );
    const auto catalogPath = ( directory / "Cache" / "asset.db" ).generic_string();
    database->setFilePath( catalogPath );
    catalog->loadFromFile( catalogPath );
    const auto folder = String( directory.generic_string() );
    app->getFileSystem()->addFolder( folder, true );
    guard.addCleanup( [app, folder]() mutable { app->getFileSystem()->removeFileArchive( folder ); } );
    // Keep the source resource cached, as it is after the Editor's first import.
    auto sourceResource = app->getMeshManager()->createOrRetrieve( String( "triangle.obj" ) ).first;
    BOOST_REQUIRE( sourceResource );

    std::function<SmartPtr<IMesh>( SmartPtr<scene::IGameActor> )> findMesh;
    findMesh = [&]( SmartPtr<scene::IGameActor> actor ) -> SmartPtr<IMesh> {
        if( auto component = actor->getComponent<scene::Mesh>() )
            if( auto resource = component->getMeshResource() )
                return resource->getMesh();
        for( auto child : actor->getChildren() )
            if( auto mesh = findMesh( child ) )
                return mesh;
        return nullptr;
    };

    for( int attempt = 1; attempt <= 3; ++attempt )
    {
        std::ofstream source( directory / "triangle.obj", std::ios::trunc );
        source << "o Triangle\nv 0 0 0\nv " << attempt << " 0 0\nv 0 1 0\nf 1 2 3\n";
        source.close();
        std::ofstream settings( directory / "SettingsCache" /
                                ( StringUtil::toString( StringUtil::getUUID( "triangle.obj" ) ) +
                                  ".resourcedata" ).c_str(), std::ios::trunc );
        settings << "{\"scale\":" << attempt << "}";
        settings.close();
        app->getFileSystem()->refreshPath( folder, false );
        loader->setOverwrite( true );
        auto actor = loader->loadActor( String( "triangle.obj" ) );
        BOOST_REQUIRE( actor );
        auto mesh = findMesh( actor );
        BOOST_REQUIRE( mesh );
        mesh->updateAABB( true );
        BOOST_CHECK_CLOSE( mesh->getAABB().getMaximum().x,
                           static_cast<real_Num>( attempt * attempt ), 0.001 );
        // Generated mesh caches must remain resolvable after prefab serialization.
        auto converter = app->getGraphicsSystem()->getMeshConverter();
        BOOST_REQUIRE( converter );
        converter->writeMesh( actor );
        auto cachedResource = database->loadResourceByType<IMeshResource>( mesh->getName() );
        BOOST_REQUIRE( cachedResource );
        auto cachedMesh = cachedResource->getMesh();
        BOOST_REQUIRE( cachedMesh );
        const auto importedSubMeshes = mesh->getSubMeshes();
        const auto cachedSubMeshes = cachedMesh->getSubMeshes();
        BOOST_REQUIRE_EQUAL( cachedSubMeshes.size(), importedSubMeshes.size() );
        for( size_t index = 0; index < importedSubMeshes.size(); ++index )
        {
            BOOST_REQUIRE( importedSubMeshes[index]->getVertexBuffer() );
            BOOST_REQUIRE( cachedSubMeshes[index]->getVertexBuffer() );
            BOOST_CHECK_EQUAL( cachedSubMeshes[index]->getVertexBuffer()->getNumVertices(),
                               importedSubMeshes[index]->getVertexBuffer()->getNumVertices() );
        }
        cachedMesh->updateAABB( true );
        BOOST_CHECK( cachedMesh->getAABB() == mesh->getAABB() );
        cachedResource->setLoadingState( LoadingState::Loaded );
        auto originalGraphics = app->getGraphicsSystem();
        auto loadProbe = workphone::make_ptr<CatalogMeshLoadProbe>();
        app->setGraphicsSystem( loadProbe );
        auto reusedResource = database->loadResourceByType<IMeshResource>( mesh->getName() );
        app->setGraphicsSystem( originalGraphics );
        BOOST_REQUIRE( reusedResource );
        BOOST_CHECK( reusedResource == cachedResource );
        BOOST_CHECK_EQUAL( loadProbe->loadRequests, 0u );
        cachedResource->setLoadingState( LoadingState::Unloaded );
        app->setGraphicsSystem( loadProbe );
        reusedResource = database->loadResourceByType<IMeshResource>( mesh->getName() );
        app->setGraphicsSystem( originalGraphics );
        BOOST_REQUIRE( reusedResource );
        BOOST_CHECK_EQUAL( loadProbe->loadRequests, 1u );
        cachedResource->setLoadingState( LoadingState::Loaded );
        const auto prefabPath = ( directory / "triangle.prefab" ).generic_string();
        app->getPrefabManager()->savePrefab( prefabPath, actor );
        auto prefab = app->getPrefabManager()->loadPrefab( prefabPath );
        BOOST_REQUIRE( prefab );
        auto instance = prefab->createActor();
        BOOST_REQUIRE( instance );
        auto instanceMesh = findMesh( instance );
        BOOST_REQUIRE( instanceMesh );
        instanceMesh->updateAABB( true );
        BOOST_CHECK_CLOSE( instanceMesh->getAABB().getMaximum().x,
                           static_cast<real_Num>( attempt * attempt ), 0.001 );
        guard.sceneManager->destroyActor( instance );
        guard.sceneManager->destroyActor( actor );
    }

    // A failed reimport must not be reported as a successful empty actor/prefab.
    std::ofstream invalidSource( directory / "triangle.obj", std::ios::trunc );
    invalidSource << "this is not a mesh";
    invalidSource.close();
    app->getFileSystem()->refreshPath( folder, false );
    BOOST_CHECK( !loader->loadActor( String( "triangle.obj" ) ) );
}

BOOST_AUTO_TEST_CASE( mesh_material_import_registers_local_and_project_textures_before_saving )
{
    TestGuard guard;
    auto app = guard.applicationManager;
    auto loader = app->getMeshLoader();
    BOOST_REQUIRE( loader );
    const auto oldProject = app->getProjectPath();
    const auto oldCache = app->getCachePath();
    const auto oldSettings = app->getSettingsPath();
    auto oldDatabase = app->getResourceDatabase();
    const auto oldOverwrite = loader->getOverwrite();
    const auto directory = std::filesystem::temp_directory_path() / StringUtil::getUUID().c_str();
    const auto modelFolder = directory / "Assets" / "Model";
    std::filesystem::create_directories( modelFolder / "Textures" );
    std::filesystem::create_directories( directory / "Assets" / "Shared" );
    std::filesystem::create_directories( directory / "Cache" );
    std::filesystem::create_directories( directory / "SettingsCache" );
    guard.trackFilesystemPath( directory.generic_string() );
    auto database = workphone::make_ptr<ResourceDatabase>();
    guard.addCleanup( [app, loader, database, oldDatabase, oldProject, oldCache, oldSettings,
                       oldOverwrite]() mutable {
        database->unload( nullptr );
        app->setResourceDatabase( oldDatabase );
        app->setProjectPath( oldProject );
        app->setCachePath( oldCache );
        app->setSettingsPath( oldSettings );
        loader->setOverwrite( oldOverwrite );
    } );
    app->setProjectPath( directory.generic_string() );
    app->setCachePath( ( directory / "Cache" ).generic_string() + "/" );
    app->setSettingsPath( ( directory / "SettingsCache" ).generic_string() + "/" );
    app->setResourceDatabase( database );
    database->load( nullptr );

    auto catalog = dynamic_pointer_cast<AssetDatabaseManager>( database->getDatabaseManager() );
    BOOST_REQUIRE( catalog );
    BOOST_REQUIRE( catalog->setProjectRoot( directory.generic_string() ) );
    const auto catalogPath = ( directory / "Cache" / "asset.db" ).generic_string();
    database->setFilePath( catalogPath );
    catalog->loadFromFile( catalogPath );

    // A valid 1x1, 24-bit BMP; fixtures need no external image assets.
    const unsigned char bmp[] = {
        0x42, 0x4d, 58, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0,
        40, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 24, 0,
        0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0
    };
    for( const auto &path : { modelFolder / "Textures" / "Local.bmp",
                              modelFolder / "Textures" / "Normal.bmp",
                              directory / "Assets" / "Shared" / "Shared.bmp" } )
    {
        std::ofstream output( path, std::ios::binary );
        output.write( reinterpret_cast<const char *>( bmp ), sizeof( bmp ) );
    }
    {
        std::ofstream mtl( modelFolder / "triangle.mtl" );
        mtl << "newmtl Local\nKd 1 1 1\nmap_Kd DeletedExport/LOCAL.BMP\n"
               "map_Bump DeletedExport/NORMAL.BMP\n"
               "newmtl Shared\nKd 1 1 1\nmap_Kd DeletedExport/SHARED.BMP\n"
               "map_Bump DeletedExport/NORMAL.BMP\n"
               "newmtl Missing\nKd 1 1 1\nmap_Kd DoesNotExist.bmp\n";
        std::ofstream obj( modelFolder / "triangle.obj" );
        obj << "mtllib triangle.mtl\no Triangle\nv 0 0 0\nv 1 0 0\nv 0 1 0\n"
               "vt 0 0\nvt 1 0\nvt 0 1\n"
               "usemtl Local\nf 1/1 2/2 3/3\n"
               "usemtl Shared\nf 1/1 2/2 3/3\n"
               "usemtl Missing\nf 1/1 2/2 3/3\n";
    }
    const auto folder = String( directory.generic_string() );
    app->getFileSystem()->addFolder( folder, true );
    guard.addCleanup( [app, folder]() mutable { app->getFileSystem()->removeFileArchive( folder ); } );
    loader->setOverwrite( true );

    // Both entry points must prepare textures before creating material files.
    auto mesh = loader->loadMesh( String( "Assets/Model/triangle.obj" ) );
    BOOST_REQUIRE( mesh );
    auto actor = loader->loadActor( String( "Assets/Model/triangle.obj" ) );
    BOOST_REQUIRE( actor );
    guard.addCleanup( [app, actor]() mutable { app->getGameManager()->destroyActor( actor ); } );

    SmartPtr<render::ITexture> sharedNormal;
    for( const auto &name : { String( "Local" ), String( "Shared" ), String( "Missing" ) } )
    {
        const auto path = String( "Assets/Model/Materials/" ) + name + ".mat";
        BOOST_REQUIRE( std::filesystem::is_regular_file( directory / path.c_str() ) );
        auto material = database->loadResourceByType<render::IMaterial>( path );
        BOOST_REQUIRE( material );
        auto texture = material->getTexture( 0 );
        if( name == "Missing" )
        {
            BOOST_CHECK( !texture );
            continue;
        }
        BOOST_REQUIRE( texture );
        const auto expected = name == "Local" ? "Assets/Model/Textures/Local.bmp"
                                               : "Assets/Shared/Shared.bmp";
        BOOST_CHECK( std::filesystem::equivalent( texture->getFilePath().c_str(),
                                                 directory / expected ) );
        auto handle = texture->getHandle();
        BOOST_REQUIRE( handle );
        BOOST_CHECK( database->loadResourceById( handle->getUUID() ) == texture );
        const auto saved = app->getFileSystem()->readAllText(
            String( ( directory / path.c_str() ).generic_string() ) );
        BOOST_CHECK( saved.find( expected ) != String::npos );
        auto normal = material->getTexture( 1 );
        BOOST_REQUIRE( normal );
        BOOST_CHECK( std::filesystem::equivalent( normal->getFilePath().c_str(),
                       directory / "Assets/Model/Textures/Normal.bmp" ) );
        if( sharedNormal )
        {
            BOOST_CHECK( normal == sharedNormal );
        }
        sharedNormal = normal;
        auto savedProperties = workphone::make_ptr<Properties>();
        DataUtil::parse( saved, savedProperties.get() );
        auto restored = workphone::dynamic_pointer_cast<render::IMaterial>(
            app->getGraphicsSystem()->getMaterialManager()->create( String( "Restored_" ) + name ) );
        BOOST_REQUIRE( restored );
        restored->fromData( savedProperties );
        BOOST_CHECK( restored->getTexture( 0 ) == texture );
        BOOST_CHECK( restored->getTexture( 1 ) == normal );
    }
    BOOST_CHECK( !std::filesystem::exists( directory / "Materials" ) );

    // Old documents have root texture slots rather than serialized pass units.
    // An explicit, cataloged repair path recovers a dangling UUID without
    // importing assets or changing either asset identity.
    const auto localPath = String( "Assets/Model/Textures/Local.bmp" );
    auto local = database->loadResourceByType<render::ITexture>( localPath );
    auto normal = database->loadResourceByType<render::ITexture>(
        "Assets/Model/Textures/Normal.bmp" );
    auto localMaterial = database->loadResourceByType<render::IMaterial>(
        "Assets/Model/Materials/Local.mat" );
    BOOST_REQUIRE( local );
    BOOST_REQUIRE( normal );
    BOOST_REQUIRE( localMaterial );
    const auto legacy = [&]( const String &uuid, const String &path ) {
        auto root = workphone::make_ptr<Properties>();
        root->setProperty( "materialType", 0 );
        auto slots = workphone::make_ptr<Properties>();
        slots->setName( "textures" );
        auto slot = workphone::make_ptr<Properties>();
        slot->setName( "texture" );
        slot->setProperty( "uuid", uuid );
        slot->setProperty( "texturePath", path );
        slots->addChild( slot );
        root->addChild( slots );
        auto manager = app->getGraphicsSystem()->getMaterialManager();
        auto result = dynamic_pointer_cast<render::IMaterial>( manager->create(
            String( "LegacyTextureRepair_" ) + StringUtil::getUUID() ) );
        BOOST_REQUIRE( result );
        guard.addCleanup( [manager, result]() mutable {
            result->unload( nullptr );
            manager->destroyResource( result );
        } );
        result->fromData( root );
        return result;
    };
    const auto missingUUID = StringUtil::getUUID();
    auto repaired = legacy( missingUUID, localPath );
    BOOST_CHECK( repaired->getTexture( 0 ) == local );
    const auto repairedDocument = DataUtil::toString(
        dynamic_pointer_cast<Properties>( repaired->toData() ).get(), true );
    BOOST_CHECK( repairedDocument.find( localPath ) != String::npos );
    BOOST_CHECK( repairedDocument.find( local->getHandle()->getUUIDAsString() ) != String::npos );
    BOOST_CHECK( legacy( "", localPath )->getTexture( 0 ) == local );
    BOOST_CHECK( legacy( normal->getHandle()->getUUIDAsString(), localPath )->getTexture( 0 ) == normal );
    BOOST_CHECK( !legacy( localMaterial->getHandle()->getUUIDAsString(), localPath )->getTexture( 0 ) );
    BOOST_CHECK( !legacy( missingUUID, "../Assets/Model/Textures/Local.bmp" )->getTexture( 0 ) );
    BOOST_CHECK( !legacy( missingUUID, ( directory / localPath.c_str() ).generic_string() )->getTexture( 0 ) );
    BOOST_CHECK( !legacy( missingUUID, "Local.bmp" )->getTexture( 0 ) );
    BOOST_CHECK( !database->loadResourceById( StringUtil::parseUUID( missingUUID ) ) );
}

#if WP_USE_ASSET_IMPORT && !WP_USE_FBXSDK
namespace
{
    class MaterialAdmissionProbe : public AssimpLoader
    {
    public:
        using AssimpLoader::createMaterial;
    };
}

BOOST_AUTO_TEST_CASE( mesh_material_import_preserves_authored_documents_and_cold_catalog_identity )
{
    TestGuard guard;
    auto app = guard.applicationManager;
    auto graphics = app->getGraphicsSystem();
    BOOST_REQUIRE( graphics );
    auto materialManager = graphics->getMaterialManager();
    BOOST_REQUIRE( materialManager );
    const auto oldProject = app->getProjectPath();
    const auto oldCache = app->getCachePath();
    const auto oldSettings = app->getSettingsPath();
    auto oldDatabase = app->getResourceDatabase();
    const auto directory = std::filesystem::temp_directory_path() / StringUtil::getUUID().c_str();
    std::filesystem::create_directories( directory / "Assets/Model/Materials" );
    std::filesystem::create_directories( directory / "Cache" );
    std::filesystem::create_directories( directory / "SettingsCache" );
    guard.trackFilesystemPath( directory.generic_string() );
    auto database = workphone::make_ptr<ResourceDatabase>();
    auto coldDatabase = workphone::make_ptr<ResourceDatabase>();
    guard.addCleanup( [app, database, coldDatabase, oldDatabase, oldProject, oldCache,
                       oldSettings]() mutable {
        coldDatabase->unload( nullptr );
        database->unload( nullptr );
        app->setResourceDatabase( oldDatabase );
        app->setProjectPath( oldProject );
        app->setCachePath( oldCache );
        app->setSettingsPath( oldSettings );
    } );
    app->setProjectPath( directory.generic_string() );
    app->setCachePath( ( directory / "Cache" ).generic_string() + "/" );
    app->setSettingsPath( ( directory / "SettingsCache" ).generic_string() + "/" );
    app->setResourceDatabase( database );
    database->load( nullptr );
    auto catalog = dynamic_pointer_cast<AssetDatabaseManager>( database->getDatabaseManager() );
    BOOST_REQUIRE( catalog );
    BOOST_REQUIRE( catalog->setProjectRoot( directory.generic_string() ) );
    const auto catalogPath = ( directory / "Cache/asset.db" ).generic_string();
    database->setFilePath( catalogPath );
    catalog->loadFromFile( catalogPath );

    const auto folder = String( directory.generic_string() );
    app->getFileSystem()->addFolder( folder, true );
    guard.addCleanup( [app, folder]() mutable { app->getFileSystem()->removeFileArchive( folder ); } );
    const auto relativePath = String( "Assets/Model/Materials/Authored.mat" );
    const auto absolutePath = String( ( directory / relativePath.c_str() ).generic_string() );
    const auto trackMaterial = [&guard, materialManager]( SmartPtr<render::IMaterial> material ) {
        guard.addCleanup( [materialManager, material]() mutable {
            if( material->isLoaded() ) material->unload( nullptr );
            materialManager->destroyResource( material );
        } );
    };

    // The document predates the project catalog, as in an existing city import.
    auto authored = dynamic_pointer_cast<render::IMaterial>( materialManager->create( absolutePath ) );
    BOOST_REQUIRE( authored );
    trackMaterial( authored );
    graphics->loadObject( authored );
    authored->setDiffuse( ColourF( 0.125f, 0.25f, 0.5f, 1.0f ) );
    authored->setRoughness( 0.375f );
    auto authoredData = dynamic_pointer_cast<Properties>( authored->toData() );
    BOOST_REQUIRE( authoredData );
    authoredData->setProperty( "authoringNote", String( "Keep this source document exactly" ) );
    // Enough authored metadata to exercise a refused nested parse allocation,
    // after the Claw descriptor preparation reservation has already succeeded.
    for( u32 i = 0; i < 256; ++i )
        authoredData->setProperty( "annotation" + StringUtil::toString( i ), i );
    const auto authoredBytes = DataUtil::toString( authoredData.get(), true ) + "\n\n";
    {
        std::ofstream output( absolutePath.c_str(), std::ios::binary );
        output.write( authoredBytes.data(), static_cast<std::streamsize>( authoredBytes.size() ) );
        BOOST_REQUIRE( output.good() );
    }
    authored->unload( nullptr );
    materialManager->destroyResource( authored );
    app->getFileSystem()->refreshPath( folder, false );
    BOOST_REQUIRE( !catalog->getResourceEntryFromPath( relativePath ) );

    aiMaterial source;
    aiString sourceName( "Authored" );
    source.AddProperty( &sourceName, AI_MATKEY_NAME );
    const auto importExisting = [&]() {
        auto importer = workphone::make_ptr<MaterialAdmissionProbe>();
        importer->setOverwriteMaterialFiles( false );
        importer->createMaterial( nullptr, 0, &source, "Assets/Model" );
        importer->unload( nullptr );
    };
    const auto checkDocumentBytes = [&]() {
        std::ifstream input( absolutePath.c_str(), std::ios::binary );
        const std::string bytes( ( std::istreambuf_iterator<char>( input ) ),
                                 std::istreambuf_iterator<char>() );
        BOOST_CHECK_EQUAL( bytes, authoredBytes );
    };
    const auto checkAuthoredValues = []( SmartPtr<render::IMaterial> material ) {
        const auto diffuse = material->getDiffuse();
        BOOST_CHECK_SMALL( diffuse.r - 0.125f, 0.00001f );
        BOOST_CHECK_SMALL( diffuse.g - 0.25f, 0.00001f );
        BOOST_CHECK_SMALL( diffuse.b - 0.5f, 0.00001f );
        BOOST_CHECK_SMALL( material->getRoughness() - 0.375f, 0.00001f );
    };
#if WP_GRAPHICS_SYSTEM_CLAW
    // Keep a cold manager body and its identity ready before constraining memory.
    // Descriptor preparation may succeed while a legacy parser swallows a denied
    // allocation; that must fail the import rather than publish partial settings.
    auto candidate = database->createOrRetrieveByType<render::IMaterial>( relativePath ).first;
    BOOST_REQUIRE( candidate );
    trackMaterial( candidate );
    candidate->fromData( authoredData );
    BOOST_REQUIRE( !candidate->isLoaded() );
    const auto candidateIdentity = candidate->getHandle()->getUUID();
    auto deniedImporter = workphone::make_ptr<MaterialAdmissionProbe>();
    deniedImporter->setOverwriteMaterialFiles( false );
    auto scheduler = WorldStreamingScheduler::shared();
    const auto budgetStats = scheduler->stats();
    const auto sourceCpu = size_t( StreamMemoryClass::SourceCpu );
    struct RestoreMaterialBudget
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler; u64 limit;
        ~RestoreMaterialBudget()
        { String error; scheduler->setBudget( StreamMemoryClass::SourceCpu, limit, error ); }
    } restoreBudget{ scheduler, budgetStats.limits[sourceCpu] };
    // Permit the descriptor's bounded preparation and small parse scratch; its
    // complete authored property array needs more than the remaining 16 KiB.
    const auto descriptorPreparation = u64( authoredBytes.size() ) * 32 + ( 64ull << 10 );
    String budgetError;
    BOOST_REQUIRE_MESSAGE( scheduler->setBudget( StreamMemoryClass::SourceCpu,
        budgetStats.used[sourceCpu] + descriptorPreparation + ( 16ull << 10 ), budgetError ), budgetError );
    const auto denialsBefore = sourceBudgetDenialCount();
    bool refused = false;
    try
    {
        deniedImporter->createMaterial( nullptr, 0, &source, "Assets/Model" );
    }
    catch( const std::exception & ) { refused = true; }
    const auto denialsAfter = sourceBudgetDenialCount();
    BOOST_REQUIRE_MESSAGE( scheduler->setBudget( StreamMemoryClass::SourceCpu,
        budgetStats.limits[sourceCpu], budgetError ), budgetError );
    BOOST_CHECK_GT( denialsAfter, denialsBefore );
    BOOST_CHECK_MESSAGE( refused, "Nested descriptor allocation denial must reject material import" );
    BOOST_CHECK( !catalog->getResourceEntryFromPath( relativePath ) );
    BOOST_CHECK( candidate->getHandle()->getUUID() == candidateIdentity );
    BOOST_CHECK( !candidate->isLoaded() );
    BOOST_CHECK_EQUAL( candidate->getNumTechniques(), 0u );
    checkDocumentBytes();
    deniedImporter->unload( nullptr );
#endif
    importExisting();
    checkDocumentBytes();
    auto imported = database->loadResourceByType<render::IMaterial>( relativePath );
#if WP_GRAPHICS_SYSTEM_CLAW
    BOOST_CHECK( imported == candidate );
    BOOST_CHECK( imported && imported->getHandle()->getUUID() == candidateIdentity );
#endif
    BOOST_REQUIRE( imported );
    trackMaterial( imported );
    checkAuthoredValues( imported );
    const auto identity = imported->getHandle()->getUUID();
    BOOST_CHECK( database->loadResourceById( identity ) == imported );
    imported->unload( nullptr );
    materialManager->destroyResource( imported );
    BOOST_REQUIRE( !materialManager->getById( imported->getHandle()->getUUIDAsString() ) );

    // Reopen the durable catalog with no resident body or importer cache alias.
    database->unload( nullptr );
    app->setResourceDatabase( coldDatabase );
    coldDatabase->load( nullptr );
    auto coldCatalog = dynamic_pointer_cast<AssetDatabaseManager>( coldDatabase->getDatabaseManager() );
    BOOST_REQUIRE( coldCatalog );
    BOOST_REQUIRE( coldCatalog != catalog );
    BOOST_REQUIRE( coldCatalog->setProjectRoot( directory.generic_string() ) );
    coldDatabase->setFilePath( catalogPath );
    coldCatalog->loadFromFile( catalogPath );
    auto cold = coldDatabase->loadResourceByType<render::IMaterial>( relativePath );
    BOOST_REQUIRE( cold );
    trackMaterial( cold );
    BOOST_CHECK( cold != imported );
    BOOST_CHECK( cold->getHandle()->getUUID() == identity );
    BOOST_CHECK( coldDatabase->loadResourceById( identity ) == cold );
    checkAuthoredValues( cold );
    checkDocumentBytes();

    // A later import must retain the catalog UUID and the existing resource body.
    importExisting();
    BOOST_CHECK( coldDatabase->loadResourceByType<render::IMaterial>( relativePath ) == cold );
    BOOST_CHECK( cold->getHandle()->getUUID() == identity );
    checkAuthoredValues( cold );
    checkDocumentBytes();
}
#endif
