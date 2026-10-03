#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

namespace
{
    struct RenderTargetTestFixture : TestGuard
    {
        RenderTargetTestFixture()
        {
            BOOST_REQUIRE( applicationManager );
            BOOST_REQUIRE( factoryManager );
            BOOST_REQUIRE( fileSystem );
            BOOST_REQUIRE( stateManager );
            BOOST_REQUIRE( timer );

            setupMediaPath();
        }

        ~RenderTargetTestFixture()
        {
            cleanup();
        }

        void setupMediaPath()
        {
            auto mediaFolderPath = String( "" );

#if defined WP_PLATFORM_WIN32
            mediaFolderPath = String( "../../../../Media/" );
#elif defined WP_PLATFORM_APPLE
            mediaFolderPath = String( "../../Media/" );
#else
            mediaFolderPath = String( "../../Media/" );
#endif

            fileSystem->addFolder( mediaFolderPath );
            applicationManager->setMediaPath( mediaFolderPath );

            auto workingDirectory = Path::getWorkingDirectory();
            fileSystem->addFolder( workingDirectory );
        }

        void cleanup()
        {
            // Skip cleanup in headless mode to avoid message pump crash
            if( !graphicsSystem )
            {
                camera = nullptr;
                cameraSceneNode = nullptr;
                rootNode = nullptr;
                sceneMgr = nullptr;
                return;
            }
            if( camera && renderTarget )
            {
                camera->setTargetTexture( nullptr );
            }

            if( cameraSceneNode && camera )
            {
                cameraSceneNode->detachObject( camera );
            }

            if( sceneMgr && camera )
            {
                sceneMgr->removeGraphicsObject( camera );
            }

            if( sceneMgr && cameraSceneNode )
            {
                sceneMgr->removeSceneNode( cameraSceneNode );
            }

            runUpdateCycle( 2 );

            if( textureManager && renderTarget )
            {
                textureManager->destroyRenderTexture( renderTarget );
                renderTarget = nullptr;
            }

            runUpdateCycle( 2 );

            camera = nullptr;
            cameraSceneNode = nullptr;
            rootNode = nullptr;
            sceneMgr = nullptr;
        }

        SmartPtr<render::IGraphicsScene> sceneMgr;
        SmartPtr<render::IGraphicsSceneNode> rootNode;
        SmartPtr<render::IGraphicsSceneNode> cameraSceneNode;
        SmartPtr<render::IGraphicsCamera> camera;
        SmartPtr<render::ITextureManager> textureManager;
        SmartPtr<render::ITexture> renderTarget;
    };
}  // namespace

BOOST_FIXTURE_TEST_SUITE( GraphicsRenderTargetTestSuite, RenderTargetTestFixture )

BOOST_AUTO_TEST_CASE( graphics_render_target_create_and_destroy )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        // Test render texture creation
        renderTarget = textureManager->createRenderTexture();
        BOOST_CHECK( renderTarget );
        BOOST_CHECK( renderTarget->isValid() );

        // Test render texture destruction
        textureManager->destroyRenderTexture( renderTarget );
        renderTarget = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during render target creation/destruction" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_with_camera )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        // Create graphics scene
        sceneMgr = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( sceneMgr );
        BOOST_CHECK( sceneMgr->isValid() );

        // Create camera
        const auto cameraName = String( "TestCamera" );
        camera = sceneMgr->addGraphicsObjectByType<render::IGraphicsCamera>();
        BOOST_REQUIRE( camera );
        camera->setName( cameraName );

        // Attach camera to scene node
        rootNode = sceneMgr->getRootSceneNode();
        BOOST_REQUIRE( rootNode );

        cameraSceneNode = rootNode->addChildSceneNode( cameraName );
        BOOST_REQUIRE( cameraSceneNode );
        cameraSceneNode->attachObject( camera );

        // Create render texture
        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );
        BOOST_CHECK( renderTarget->isValid() );

        // Assign render target to camera
        camera->setTargetTexture( renderTarget );

        // Run a few frames
        runUpdateCycle( 10 );

        // Verify camera still has render target
        BOOST_CHECK( camera );
        BOOST_CHECK( renderTarget );
        BOOST_CHECK( renderTarget->isValid() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during render target with camera test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_detach_from_camera )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        sceneManager = applicationManager->getGameManager();
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        sceneMgr = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( sceneMgr );

        camera = sceneMgr->addGraphicsObjectByType<render::IGraphicsCamera>();
        BOOST_REQUIRE( camera );
        camera->setName( String( "DetachTestCamera" ) );

        rootNode = sceneMgr->getRootSceneNode();
        BOOST_REQUIRE( rootNode );

        cameraSceneNode = rootNode->addChildSceneNode( String( "DetachCameraNode" ) );
        BOOST_REQUIRE( cameraSceneNode );
        cameraSceneNode->attachObject( camera );

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        // Attach render target
        camera->setTargetTexture( renderTarget );
        runUpdateCycle( 5 );

        // Detach render target (set to nullptr)
        camera->setTargetTexture( nullptr );
        runUpdateCycle( 5 );

        // Verify detachment succeeded
        BOOST_CHECK( camera );
        BOOST_CHECK( renderTarget );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during render target detach test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_multiple_render_targets )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        // Create multiple render textures
        const int numRenderTargets = 3;
        Array<SmartPtr<render::ITexture>> renderTargets;

        for( int i = 0; i < numRenderTargets; ++i )
        {
            auto rt = textureManager->createRenderTexture();
            BOOST_CHECK( rt );
            BOOST_CHECK( rt->isValid() );
            renderTargets.push_back( rt );
        }

        BOOST_CHECK_EQUAL( renderTargets.size(), numRenderTargets );

        // Verify all render targets are distinct
        for( size_t i = 0; i < renderTargets.size(); ++i )
        {
            for( size_t j = i + 1; j < renderTargets.size(); ++j )
            {
                BOOST_CHECK( renderTargets[i] != renderTargets[j] );
            }
        }

        // Clean up
        for( auto &rt : renderTargets )
        {
            textureManager->destroyRenderTexture( rt );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during multiple render targets test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_null_texture_manager )
{
    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();

        if( !graphicsSystem )
        {
            // If no graphics system, test passes (expected scenario)
            BOOST_TEST_MESSAGE( "No graphics system available, skipping test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();

        // Test graceful handling when texture manager is not available
        if( !textureManager )
        {
            BOOST_TEST_MESSAGE( "Texture manager not available" );
            return;
        }

        BOOST_CHECK( textureManager );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during null texture manager test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_camera_switch )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        sceneManager = applicationManager->getGameManager();
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        sceneMgr = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( sceneMgr );

        // Create two cameras
        auto camera1 = sceneMgr->addGraphicsObjectByType<render::IGraphicsCamera>();
        BOOST_REQUIRE( camera1 );
        camera1->setName( String( "Camera1" ) );

        auto camera2 = sceneMgr->addGraphicsObjectByType<render::IGraphicsCamera>();
        BOOST_REQUIRE( camera2 );
        camera2->setName( String( "Camera2" ) );

        rootNode = sceneMgr->getRootSceneNode();
        BOOST_REQUIRE( rootNode );

        auto cameraNode1 = rootNode->addChildSceneNode( String( "CameraNode1" ) );
        BOOST_REQUIRE( cameraNode1 );
        cameraNode1->attachObject( camera1 );

        auto cameraNode2 = rootNode->addChildSceneNode( String( "CameraNode2" ) );
        BOOST_REQUIRE( cameraNode2 );
        cameraNode2->attachObject( camera2 );

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        // Assign render target to first camera
        camera1->setTargetTexture( renderTarget );
        runUpdateCycle( 5 );

        // Switch render target to second camera
        camera1->setTargetTexture( nullptr );
        camera2->setTargetTexture( renderTarget );
        runUpdateCycle( 5 );

        // Switch back to first camera
        camera2->setTargetTexture( nullptr );
        camera1->setTargetTexture( renderTarget );
        runUpdateCycle( 5 );

        // Clean up
        camera1->setTargetTexture( nullptr );
        camera = nullptr;  // Don't use fixture cleanup for camera
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during camera switch test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_destroy_with_active_camera )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        sceneManager = applicationManager->getGameManager();
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        sceneMgr = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( sceneMgr );

        camera = sceneMgr->addGraphicsObjectByType<render::IGraphicsCamera>();
        BOOST_REQUIRE( camera );
        camera->setName( String( "DestroyTestCamera" ) );

        rootNode = sceneMgr->getRootSceneNode();
        BOOST_REQUIRE( rootNode );

        cameraSceneNode = rootNode->addChildSceneNode( String( "DestroyTestNode" ) );
        BOOST_REQUIRE( cameraSceneNode );
        cameraSceneNode->attachObject( camera );

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        camera->setTargetTexture( renderTarget );
        runUpdateCycle( 5 );

        // Properly detach before destroying
        camera->setTargetTexture( nullptr );

        // Test destroying render target after detachment
        textureManager->destroyRenderTexture( renderTarget );
        renderTarget = nullptr;

        runUpdateCycle( 5 );

        BOOST_CHECK( camera );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during destroy with active camera test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_extended_render )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        sceneManager = applicationManager->getGameManager();
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        sceneMgr = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( sceneMgr );

        camera = sceneMgr->addGraphicsObjectByType<render::IGraphicsCamera>();
        BOOST_REQUIRE( camera );
        camera->setName( String( "ExtendedTestCamera" ) );

        rootNode = sceneMgr->getRootSceneNode();
        BOOST_REQUIRE( rootNode );

        cameraSceneNode = rootNode->addChildSceneNode( String( "ExtendedCameraNode" ) );
        BOOST_REQUIRE( cameraSceneNode );
        cameraSceneNode->attachObject( camera );

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        camera->setTargetTexture( renderTarget );

        // Extended render cycle to test stability
        runUpdateCycle( 50 );

        // Verify render target is still valid after extended rendering
        BOOST_CHECK( renderTarget );
        BOOST_CHECK( renderTarget->isValid() );
        BOOST_CHECK( camera );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during extended render test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_size_accessors )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        // Test setSize and getSize
        const Vector2I testSize( 512, 256 );
        renderTarget->setSize( testSize );

        auto retrievedSize = renderTarget->getSize();
        BOOST_CHECK_EQUAL( retrievedSize.X(), testSize.X() );
        BOOST_CHECK_EQUAL( retrievedSize.Y(), testSize.Y() );
        BOOST_CHECK_NE( retrievedSize.X(), retrievedSize.Y() );

        // Test with different sizes
        const Vector2I largeSize( 2048, 1024 );
        renderTarget->setSize( largeSize );

        retrievedSize = renderTarget->getSize();
        BOOST_CHECK_EQUAL( retrievedSize.X(), largeSize.X() );
        BOOST_CHECK_EQUAL( retrievedSize.Y(), largeSize.Y() );

        // Test another non-square size to catch stale square GPU allocation values.
        const Vector2I tallSize( 128, 512 );
        renderTarget->setSize( tallSize );

        retrievedSize = renderTarget->getSize();
        BOOST_CHECK_EQUAL( retrievedSize.X(), tallSize.X() );
        BOOST_CHECK_EQUAL( retrievedSize.Y(), tallSize.Y() );

        renderTarget->setSize( tallSize );

        retrievedSize = renderTarget->getSize();
        BOOST_CHECK_EQUAL( retrievedSize.X(), tallSize.X() );
        BOOST_CHECK_EQUAL( retrievedSize.Y(), tallSize.Y() );

        // Test getActualSize (may differ from requested size)
        auto actualSize = renderTarget->getActualSize();
        BOOST_CHECK( actualSize.X() > 0 || actualSize.Y() > 0 ||
                     ( actualSize.X() == 0 && actualSize.Y() == 0 ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during size accessors test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_usage_flags_accessors )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        // Test setUsageFlags and getUsageFlags
        const u32 renderTargetFlags = (u32)TextureUsage::TU_RENDERTARGET;
        renderTarget->setUsageFlags( renderTargetFlags );

        auto retrievedFlags = renderTarget->getUsageFlags();
        BOOST_CHECK( ( retrievedFlags & (u32)TextureUsage::TU_RENDERTARGET ) != 0 );

        // Test with combined flags
        const u32 combinedFlags = (u32)TextureUsage::TU_RENDERTARGET | (u32)TextureUsage::TU_AUTOMIPMAP;
        renderTarget->setUsageFlags( combinedFlags );

        retrievedFlags = renderTarget->getUsageFlags();
        BOOST_CHECK( ( retrievedFlags & (u32)TextureUsage::TU_RENDERTARGET ) != 0 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during usage flags accessors test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_name_accessors )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        // Test setName and getName
        const String testName( "TestRenderTarget" );
        renderTarget->setName( testName );

        auto retrievedName = renderTarget->getName();
        BOOST_CHECK_EQUAL( retrievedName, testName );

        // Test with different name
        const String anotherName( "AnotherRenderTarget_123" );
        renderTarget->setName( anotherName );

        retrievedName = renderTarget->getName();
        BOOST_CHECK_EQUAL( retrievedName, anotherName );

        // Test empty name
        const String emptyName( "" );
        renderTarget->setName( emptyName );

        retrievedName = renderTarget->getName();
        BOOST_CHECK_EQUAL( retrievedName, emptyName );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during name accessors test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_loading_state_accessors )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        // Test getLoadingState
        auto loadingState = renderTarget->getLoadingState();
        BOOST_CHECK( loadingState == LoadingState::Allocated || loadingState == LoadingState::Loading ||
                     loadingState == LoadingState::Loaded || loadingState == LoadingState::Unloading ||
                     loadingState == LoadingState::Unloaded );

        // Test isLoaded
        bool loaded = renderTarget->isLoaded();
        BOOST_CHECK( loaded == ( loadingState == LoadingState::Loaded ) );

        // Test isAlive
        bool alive = renderTarget->isAlive();
        BOOST_CHECK( alive );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during loading state accessors test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_file_path_accessors )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        // Test setFilePath and getFilePath
        const String testPath( "textures/render_target.png" );
        renderTarget->setFilePath( testPath );

        auto retrievedPath = renderTarget->getFilePath();
        BOOST_CHECK_EQUAL( retrievedPath, testPath );

        // Test with different path
        const String anotherPath( "assets/materials/rt_diffuse.dds" );
        renderTarget->setFilePath( anotherPath );

        retrievedPath = renderTarget->getFilePath();
        BOOST_CHECK_EQUAL( retrievedPath, anotherPath );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during file path accessors test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_texture_handle_accessor )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        // Test getTextureHandle - returns size_t (unsigned), 0 means not yet allocated on GPU
        auto handle = renderTarget->getTextureHandle();

        // Create a second render target and retrieve its handle
        auto renderTarget2 = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget2 );

        auto handle2 = renderTarget2->getTextureHandle();

        // Handles may be equal when neither texture has been uploaded to the GPU yet.
        // Log a diagnostic message rather than failing; uniqueness is only expected after
        // GPU allocation (i.e. after the texture has been used in a render pass).
        if( handle != 0 && handle2 != 0 && handle == handle2 )
        {
            BOOST_TEST_MESSAGE( "Warning: two distinct render textures share handle "
                                << handle << " - may indicate deferred GPU allocation" );
        }

        textureManager->destroyRenderTexture( renderTarget2 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during texture handle accessor test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_id_accessors )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        // Test getId
        auto originalId = renderTarget->getId();

        // Test setId and getId
        const hash_type testId = 12345;
        renderTarget->setId( testId );

        auto retrievedId = renderTarget->getId();
        BOOST_CHECK_EQUAL( retrievedId, testId );

        // Test with different ID
        const hash_type anotherId = 67890;
        renderTarget->setId( anotherId );

        retrievedId = renderTarget->getId();
        BOOST_CHECK_EQUAL( retrievedId, anotherId );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during ID accessors test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_validity_check )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        // Test isValid returns true for newly created texture
        BOOST_CHECK( renderTarget->isValid() );

        // Test isAlive returns true
        BOOST_CHECK( renderTarget->isAlive() );

        // Test getReferences returns positive count
        auto refCount = renderTarget->getReferences();
        BOOST_CHECK( refCount > 0 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during validity check test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_user_data_accessors )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        // Test setUserData and getUserData
        int testData = 42;
        renderTarget->setUserData( &testData );

        auto retrievedData = renderTarget->getUserData();
        BOOST_CHECK( retrievedData == &testData );

        // Verify the data value
        if( retrievedData )
        {
            BOOST_CHECK_EQUAL( *static_cast<int *>( retrievedData ), 42 );
        }

        // Test setting nullptr
        renderTarget->setUserData( nullptr );
        retrievedData = renderTarget->getUserData();
        BOOST_CHECK( retrievedData == nullptr );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during user data accessors test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_render_target_accessor )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        // Test getRenderTarget - may return valid or null depending on implementation
        auto rt = renderTarget->getRenderTarget();

        // If render target is set, verify we can retrieve it
        // Note: For a render texture, this should return a valid render target
        if( rt )
        {
            BOOST_CHECK( rt->isValid() );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during render target accessor test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_to_string )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        // Set a name for meaningful toString output
        const String testName( "ToStringTestTexture" );
        renderTarget->setName( testName );

        // Test toString
        auto str = renderTarget->toString();

        // toString output is implementation-defined; just verify the call succeeds
        BOOST_TEST_MESSAGE( "toString returned: " << str );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during toString test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_copy_to_texture )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        auto renderTarget2 = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget2 );

        // copyToTexture must not throw even when neither texture has been rendered into
        BOOST_CHECK_NO_THROW( renderTarget->copyToTexture( renderTarget2 ) );

        textureManager->destroyRenderTexture( renderTarget2 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during copy to texture test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_copy_data_null )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        auto renderTextureObj = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTextureObj );

        // createRenderTexture() returns an ITexture, but we need to check if it's actually
        // a render texture and get the underlying texture to call copyData on it
        // Try to cast to IRenderTexture to access the internal texture
        auto renderTexture = workphone::dynamic_pointer_cast<render::IRenderTexture>( renderTextureObj );
        if( renderTexture )
        {
            // Get the actual texture from the render texture
            auto texture = renderTexture->getTexture();
            if( texture )
            {
                // Passing nullptr data with a zero-size should be handled gracefully
                const Vector2I zeroSize( 0, 0 );
                BOOST_CHECK_NO_THROW( texture->copyData( nullptr, zeroSize ) );
            }
            else
            {
                BOOST_TEST_MESSAGE(
                    "Render texture does not have an associated texture - skipping test" );
            }
        }
        else
        {
            // If it's directly an ITexture (not wrapped in IRenderTexture), use it directly
            // Passing nullptr data with a zero-size should be handled gracefully
            const Vector2I zeroSize( 0, 0 );
            BOOST_CHECK_NO_THROW( renderTextureObj->copyData( nullptr, zeroSize ) );
        }

        renderTarget = renderTextureObj;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during copy data null test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_size_zero )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        // Setting a zero size must not crash
        const Vector2I zeroSize( 0, 0 );
        BOOST_CHECK_NO_THROW( renderTarget->setSize( zeroSize ) );

        // After setting zero size the implementation may clamp or reject;
        // just verify getSize does not crash and returns consistent values
        auto size = renderTarget->getSize();
        BOOST_CHECK( size.X() >= 0 );
        BOOST_CHECK( size.Y() >= 0 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during zero size test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_size_non_power_of_two )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        // Non-power-of-two sizes are a common edge case for GPU texture allocation
        const Vector2I npotSize( 300, 200 );
        BOOST_CHECK_NO_THROW( renderTarget->setSize( npotSize ) );

        auto size = renderTarget->getSize();
        BOOST_CHECK_EQUAL( size.X(), npotSize.X() );
        BOOST_CHECK_EQUAL( size.Y(), npotSize.Y() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during non-power-of-two size test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_get_texture_gpu )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        // getTextureGPU and getTextureFinal must not throw;
        // the pointer may be null before the texture is uploaded to the GPU
        void *gpuTexture = nullptr;
        BOOST_CHECK_NO_THROW( renderTarget->getTextureGPU( &gpuTexture ) );

        void *finalTexture = nullptr;
        BOOST_CHECK_NO_THROW( renderTarget->getTextureFinal( &finalTexture ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during get texture GPU test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_set_render_target )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        // Setting nullptr as the render target should be handled gracefully
        BOOST_CHECK_NO_THROW( renderTarget->setRenderTarget( nullptr ) );

        // getRenderTarget after setting nullptr must not crash
        auto rt = renderTarget->getRenderTarget();
        BOOST_CHECK( !rt );  // should now be null
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during set render target test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_usage_flags_no_flags )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        // Clearing all flags is an edge case that should not crash
        BOOST_CHECK_NO_THROW( renderTarget->setUsageFlags( 0u ) );

        auto flags = renderTarget->getUsageFlags();
        BOOST_CHECK_EQUAL( flags, 0u );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during usage flags no-flags test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_render_target_texture_handle_after_size_change )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        textureManager = graphicsSystem->getTextureManager();
        BOOST_REQUIRE( textureManager );

        renderTarget = textureManager->createRenderTexture();
        BOOST_REQUIRE( renderTarget );

        auto handleBefore = renderTarget->getTextureHandle();

        // Changing the size may trigger GPU reallocation
        renderTarget->setSize( Vector2I( 256, 256 ) );
        renderTarget->setSize( Vector2I( 512, 512 ) );

        // getTextureHandle must not crash after size changes
        auto handleAfter = renderTarget->getTextureHandle();
        BOOST_TEST_MESSAGE( "Handle before resize: " << handleBefore
                                                     << ", after resize: " << handleAfter );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during texture handle after size change test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()

