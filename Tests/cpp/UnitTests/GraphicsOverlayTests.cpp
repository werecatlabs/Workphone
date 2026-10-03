#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

// Helper function to run update loop
namespace
{
    void RunUpdateLoop( SmartPtr<core::IApplicationManager> appManager, int iterations = 10 )
    {
        auto timer = appManager->getTimer();
        auto stateManager = appManager->getStateManager();
        auto sceneManager = appManager->getGameManager();
        auto graphicsSystem = appManager->getGraphicsSystem();

        for( int i = 0; i < iterations; ++i )
        {
            timer->update();

            stateManager->preUpdate();
            stateManager->update();
            stateManager->postUpdate();

            sceneManager->preUpdate();
            sceneManager->update();
            sceneManager->postUpdate();

            graphicsSystem->messagePump();
            graphicsSystem->update();
        }
    }

    // Helper to create a basic UI canvas
    SmartPtr<scene::IGameActor> CreateCanvas( SmartPtr<core::IApplicationManager> appManager,
                                              const Vector2I &referenceSize = Vector2I( 1920, 1080 ) )
    {
        auto sceneManager = appManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();
        scene->addActor( actor );
        scene->registerAllUpdates( actor );

        actor->setLocalPosition( Vector3F( 0, 0, 0 ) );
        actor->setPosition( Vector3F( 0, 0, 0 ) );

        auto canvas = actor->addComponent<scene::Layout>();
        canvas->setReferenceSize( referenceSize );

        return actor;
    }

    // Helper to create image actor with material
    SmartPtr<scene::IGameActor> CreateImageActor( SmartPtr<core::IApplicationManager> appManager,
                                                  SmartPtr<scene::IGameActor> parent,
                                                  const Vector2F &size, const String &materialPath = "" )
    {
        auto sceneManager = appManager->getGameManager();

        auto actorImage = sceneManager->createActor();
        parent->addChild( actorImage );

        auto actorImageCanvasTransform = actorImage->addComponent<scene::LayoutTransform>();
        actorImageCanvasTransform->setSize( size );

        auto image = actorImage->addComponent<scene::Image>();

        if( !materialPath.empty() )
        {
            auto materialComponent = actorImage->addComponent<scene::Material>();
            auto cleanPath = StringUtil::cleanupPath( materialPath );
            materialComponent->setMaterialPath( cleanPath );
        }

        return actorImage;
    }
}  // namespace

BOOST_AUTO_TEST_CASE( graphics_overlay_basic )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        BOOST_REQUIRE( fileSystem );

        auto workingDirectory = Path::getWorkingDirectory();
        fileSystem->addFolder( workingDirectory );
        BOOST_CHECK( fileSystem->isValid() );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }
        BOOST_CHECK( graphicsSystem->getLoadingState() == LoadingState::Loaded );

        auto timer = applicationManager->getTimer();
        BOOST_REQUIRE( timer );

        auto stateManager = applicationManager->getStateManager();
        BOOST_REQUIRE( stateManager );

        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        BOOST_REQUIRE( scene );

        if( graphicsSystem )
        {
            auto window = graphicsSystem->getDefaultWindow();
            BOOST_REQUIRE( window );

            auto resourceGroupManager = graphicsSystem->getResourceGroupManager();
            BOOST_REQUIRE( resourceGroupManager );

            auto renderUI = applicationManager->getRenderUI();
            BOOST_REQUIRE( renderUI );

            auto canvas = CreateCanvas( applicationManager );
            BOOST_REQUIRE( canvas );

            auto actorImageParent = sceneManager->createActor();
            canvas->addChild( actorImageParent );

            auto actorImageParentCanvasTransform =
                actorImageParent->addComponent<scene::LayoutTransform>();
            actorImageParentCanvasTransform->setSize( Vector2F( 1920.0f, 1080.0f ) );

            auto materialPath = applicationManager->getMediaPath() + "/DefaultUI.mat";
            auto actorImage = CreateImageActor( applicationManager, actorImageParent,
                                                Vector2F( 1280.0f, 720.0f ), materialPath );
            BOOST_REQUIRE( actorImage );

            RunUpdateLoop( applicationManager, 10 );

            scene->removeActor( canvas );
            scene->unregisterAll( canvas );

            sceneManager->destroyActor( canvas );

            if( canvas->getReferences() != 1 )
            {
#if WP_TRACK_REFERENCES
                auto &objectTracker = SharedObjectTracker::instance();
                auto filePath = "canvas.log";
                objectTracker.dumpReport( canvas.get(), filePath );
#endif
            }

            scene->clear();

            BOOST_CHECK_EQUAL( canvas->getReferences(), 1 );
            canvas = nullptr;
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( graphics_overlay_material_switch )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        BOOST_REQUIRE( fileSystem );

        auto workingDirectory = Path::getWorkingDirectory();
        BOOST_REQUIRE( !StringUtil::isNullOrEmpty( workingDirectory ) );
        fileSystem->addFolder( workingDirectory, true );

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

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }
        BOOST_CHECK( graphicsSystem->getLoadingState() == LoadingState::Loaded );

        if( graphicsSystem )
        {
            auto window = graphicsSystem->getDefaultWindow();
            BOOST_REQUIRE( window );

            auto canvas = CreateCanvas( applicationManager );
            BOOST_REQUIRE( canvas );

            auto actorImageParent = sceneManager->createActor();
            canvas->addChild( actorImageParent );

            auto actorImageParentCanvasTransform =
                actorImageParent->addComponent<scene::LayoutTransform>();
            actorImageParentCanvasTransform->setSize( Vector2F( 1920.0f, 1080.0f ) );

            auto materialPath = mediaFolderPath + "/DefaultUI.mat";
            auto actorImage = CreateImageActor( applicationManager, actorImageParent,
                                                Vector2F( 1280.0f, 720.0f ), materialPath );
            BOOST_REQUIRE( actorImage );

            auto materialComponent = actorImage->getComponent<scene::Material>();
            BOOST_REQUIRE( materialComponent );

            auto image = actorImage->getComponent<scene::Image>();
            BOOST_REQUIRE( image );

            RunUpdateLoop( applicationManager, 10 );

            // Switch to background material
            auto bgMaterialPath = mediaFolderPath + "/BackgroundUI.mat";
            bgMaterialPath = StringUtil::cleanupPath( bgMaterialPath );
            materialComponent->setMaterialPath( bgMaterialPath );

            RunUpdateLoop( applicationManager, 10 );

            BOOST_CHECK( image->isValid() );
            BOOST_CHECK( materialComponent->isValid() );

            sceneManager->destroyActor( canvas );

            scene->clear();
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( graphics_overlay_load_unload_cycle )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        BOOST_REQUIRE( fileSystem );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto stateManager = applicationManager->getStateManager();
        auto timer = applicationManager->getTimer();

        if( graphicsSystem )
        {
            BOOST_CHECK( graphicsSystem->getLoadingState() == LoadingState::Loaded );

            auto window = graphicsSystem->getDefaultWindow();
            BOOST_REQUIRE( window );

            auto canvas = CreateCanvas( applicationManager );
            BOOST_REQUIRE( canvas );

            auto actorImageParent = sceneManager->createActor();
            canvas->addChild( actorImageParent );

            auto actorImageParentCanvasTransform =
                actorImageParent->addComponent<scene::LayoutTransform>();
            actorImageParentCanvasTransform->setSize( Vector2F( 1920.0f, 1080.0f ) );

            auto materialPath = applicationManager->getMediaPath() + "/DefaultUI.mat";
            auto actorImage = CreateImageActor( applicationManager, actorImageParent,
                                                Vector2F( 1280.0f, 720.0f ), materialPath );
            BOOST_REQUIRE( actorImage );

            auto image = actorImage->getComponent<scene::Image>();
            BOOST_REQUIRE( image );

            auto materialComponent = actorImage->getComponent<scene::Material>();
            BOOST_REQUIRE( materialComponent );

            RunUpdateLoop( applicationManager, 10 );

            // Switch material
            auto bgMaterialPath = applicationManager->getMediaPath() + "/BackgroundUI.mat";
            bgMaterialPath = StringUtil::cleanupPath( bgMaterialPath );
            materialComponent->setMaterialPath( bgMaterialPath );

            RunUpdateLoop( applicationManager, 10 );

            BOOST_CHECK( image );
            BOOST_CHECK( image->isValid() );
            BOOST_CHECK_EQUAL( (s32)image->getLoadingState(), (s32)LoadingState::Loaded );

            // Unload/reload the image component directly. GameActor::unload() is destructive and
            // clears child/component ownership, so it is not a valid actor reload path.
            image->unload( nullptr );
            BOOST_CHECK_EQUAL( (s32)image->getLoadingState(), (s32)LoadingState::Unloaded );
            BOOST_CHECK( image->isValid() );

            // Reload test
            image->load( nullptr );
            RunUpdateLoop( applicationManager, 5 );
            BOOST_CHECK_EQUAL( (s32)image->getLoadingState(), (s32)LoadingState::Loaded );

            scene->clear();
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( graphics_overlay_create_destroy_cycle )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();
        auto graphicsSystem = applicationManager->getGraphicsSystem();

        if( graphicsSystem )
        {
            BOOST_CHECK( graphicsSystem->getLoadingState() == LoadingState::Loaded );

            auto window = graphicsSystem->getDefaultWindow();
            BOOST_REQUIRE( window );

            // Create and destroy cycle
            for( int i = 0; i < 3; ++i )
            {
                auto canvas = ApplicationUtil::createOverlayPanelTest();
                BOOST_REQUIRE( canvas );

                RunUpdateLoop( applicationManager, 10 );

                sceneManager->destroyActor( canvas );

                RunUpdateLoop( applicationManager, 5 );
            }

            // Final verification
            auto canvas = ApplicationUtil::createOverlayPanelTest();
            BOOST_REQUIRE( canvas );

            RunUpdateLoop( applicationManager, 10 );

            sceneManager->destroyActor( canvas );
            scene->clear();
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( graphics_overlay_reparenting )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_REQUIRE( factoryManager );

        auto fileSystem = applicationManager->getFileSystem();
        BOOST_REQUIRE( fileSystem );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto stateManager = applicationManager->getStateManager();
        auto timer = applicationManager->getTimer();

        if( graphicsSystem )
        {
            BOOST_CHECK( graphicsSystem->getLoadingState() == LoadingState::Loaded );

            auto window = graphicsSystem->getDefaultWindow();
            BOOST_REQUIRE( window );

            auto canvas1 = CreateCanvas( applicationManager );
            BOOST_REQUIRE( canvas1 );

            auto canvas2 = CreateCanvas( applicationManager );
            BOOST_REQUIRE( canvas2 );

            auto actorImageParent = sceneManager->createActor();
            canvas1->addChild( actorImageParent );

            auto actorImageParentCanvasTransform =
                actorImageParent->addComponent<scene::LayoutTransform>();
            actorImageParentCanvasTransform->setSize( Vector2F( 1920.0f, 1080.0f ) );

            auto materialPath = applicationManager->getMediaPath() + "/DefaultUI.mat";
            auto actorImage = CreateImageActor( applicationManager, actorImageParent,
                                                Vector2F( 1280.0f, 720.0f ), materialPath );
            BOOST_REQUIRE( actorImage );

            auto image = actorImage->getComponent<scene::Image>();
            BOOST_REQUIRE( image );

            RunUpdateLoop( applicationManager, 10 );

            BOOST_CHECK_EQUAL( (s32)image->getLoadingState(), (s32)LoadingState::Loaded );

            // Reparent to canvas2
            canvas2->addChild( actorImageParent );

            RunUpdateLoop( applicationManager, 10 );

            BOOST_CHECK( image->isValid() );
            BOOST_CHECK_EQUAL( (s32)image->getLoadingState(), (s32)LoadingState::Loaded );

            // Unload canvas1 (should not affect actorImage now)
            canvas1->unload( nullptr );
            BOOST_CHECK_EQUAL( (s32)image->getLoadingState(), (s32)LoadingState::Loaded );

            // Unload canvas2 (should affect actorImage)
            canvas2->unload( nullptr );
            BOOST_CHECK_EQUAL( (s32)image->getLoadingState(), (s32)LoadingState::Unloaded );

            sceneManager->destroyActor( canvas1 );
            sceneManager->destroyActor( canvas2 );

            scene->clear();
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( graphics_overlay_null_material )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();
        auto graphicsSystem = applicationManager->getGraphicsSystem();

        if( graphicsSystem )
        {
            auto canvas = CreateCanvas( applicationManager );
            BOOST_REQUIRE( canvas );

            auto actorImageParent = sceneManager->createActor();
            canvas->addChild( actorImageParent );

            // Create image without material
            auto actorImage =
                CreateImageActor( applicationManager, actorImageParent, Vector2F( 640.0f, 480.0f ), "" );
            BOOST_REQUIRE( actorImage );

            auto image = actorImage->getComponent<scene::Image>();
            BOOST_REQUIRE( image );

            RunUpdateLoop( applicationManager, 10 );

            // Should still be valid even without material
            BOOST_CHECK( image->isValid() );

            sceneManager->destroyActor( canvas );
            scene->clear();
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( graphics_overlay_various_sizes )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();
        auto graphicsSystem = applicationManager->getGraphicsSystem();

        if( graphicsSystem )
        {
            // Test various canvas sizes
            std::vector<Vector2I> testSizes = { Vector2I( 1920, 1080 ), Vector2I( 1280, 720 ),
                                                Vector2I( 800, 600 ), Vector2I( 3840, 2160 ),  // 4K
                                                Vector2I( 640, 480 ) };

            for( const auto &size : testSizes )
            {
                auto canvas = CreateCanvas( applicationManager, size );
                BOOST_REQUIRE( canvas );

                auto layoutComponent = canvas->getComponent<scene::Layout>();
                BOOST_REQUIRE( layoutComponent );

                RunUpdateLoop( applicationManager, 5 );

                sceneManager->destroyActor( canvas );
                scene->clear();
            }
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( graphics_overlay_multiple_images )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();
        auto graphicsSystem = applicationManager->getGraphicsSystem();

        if( graphicsSystem )
        {
            auto canvas = CreateCanvas( applicationManager );
            BOOST_REQUIRE( canvas );

            auto actorImageParent = sceneManager->createActor();
            canvas->addChild( actorImageParent );

            auto actorImageParentCanvasTransform =
                actorImageParent->addComponent<scene::LayoutTransform>();
            actorImageParentCanvasTransform->setSize( Vector2F( 1920.0f, 1080.0f ) );

            // Create multiple images
            std::vector<SmartPtr<scene::IGameActor>> images;
            std::vector<Vector2F> sizes = { Vector2F( 400.0f, 300.0f ), Vector2F( 640.0f, 480.0f ),
                                            Vector2F( 800.0f, 600.0f ) };

            auto materialPath = applicationManager->getMediaPath() + "/DefaultUI.mat";

            for( const auto &size : sizes )
            {
                auto actorImage =
                    CreateImageActor( applicationManager, actorImageParent, size, materialPath );
                BOOST_REQUIRE( actorImage );
                images.push_back( actorImage );
            }

            RunUpdateLoop( applicationManager, 10 );

            // Verify all images are valid
            for( const auto &imageActor : images )
            {
                auto image = imageActor->getComponent<scene::Image>();
                BOOST_CHECK( image );
                BOOST_CHECK( image->isValid() );
            }

            sceneManager->destroyActor( canvas );
            scene->clear();
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( graphics_overlay_reference_counting )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();
        auto graphicsSystem = applicationManager->getGraphicsSystem();

        if( graphicsSystem )
        {
            SmartPtr<scene::IGameActor> canvas;

            {
                canvas = CreateCanvas( applicationManager );
                BOOST_REQUIRE( canvas );

                RunUpdateLoop( applicationManager, 5 );

                // Should have multiple references at this point
                BOOST_CHECK_GT( canvas->getReferences(), 1 );
            }

            scene->removeActor( canvas );
            scene->unregisterAll( canvas );

            sceneManager->destroyActor( canvas );

            // Should drop to 1 reference after cleanup
            if( canvas->getReferences() != 1 )
            {
#if WP_TRACK_REFERENCES
                auto &objectTracker = SharedObjectTracker::instance();
                auto filePath = "canvas_refcount.log";
                objectTracker.dumpReport( canvas.get(), filePath );
#endif
                BOOST_WARN_MESSAGE( false, "Reference count is not 1: " +
                                               StringUtil::toString( canvas->getReferences() ) );
            }

            canvas = nullptr;

            scene->clear();
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( graphics_overlay_edge_case_zero_size )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();
        auto graphicsSystem = applicationManager->getGraphicsSystem();

        if( graphicsSystem )
        {
            auto canvas = CreateCanvas( applicationManager );
            BOOST_REQUIRE( canvas );

            auto actorImageParent = sceneManager->createActor();
            canvas->addChild( actorImageParent );

            // Test zero size - edge case
            auto actorImage =
                CreateImageActor( applicationManager, actorImageParent, Vector2F( 0.0f, 0.0f ), "" );
            BOOST_REQUIRE( actorImage );

            RunUpdateLoop( applicationManager, 5 );

            auto image = actorImage->getComponent<scene::Image>();
            BOOST_CHECK( image );

            sceneManager->destroyActor( canvas );
            scene->clear();
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( graphics_overlay_negative_position )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();
        auto graphicsSystem = applicationManager->getGraphicsSystem();

        if( graphicsSystem )
        {
            auto canvas = CreateCanvas( applicationManager );
            BOOST_REQUIRE( canvas );

            // Test negative position - edge case
            canvas->setLocalPosition( Vector3F( -100.0f, -100.0f, -100.0f ) );
            canvas->setPosition( Vector3F( -100.0f, -100.0f, -100.0f ) );

            RunUpdateLoop( applicationManager, 5 );

            BOOST_CHECK( canvas->isValid() );

            sceneManager->destroyActor( canvas );
            scene->clear();
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}
