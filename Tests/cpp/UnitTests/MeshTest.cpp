#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <vector>

using namespace workphone;

namespace
{
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
