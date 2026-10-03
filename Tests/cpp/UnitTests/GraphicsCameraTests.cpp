#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <cmath>

using namespace workphone;

BOOST_AUTO_TEST_CASE( claw_camera_view_tracks_native_editor_transform )
{
    auto graphicsSystem = core::IApplicationManager::instance()->getGraphicsSystem();
    BOOST_REQUIRE( graphicsSystem );
    auto scene = graphicsSystem->addGraphicsScene( "DefaultScene", "NativeEditorCameraTest" );
    BOOST_REQUIRE( scene );
    auto node = scene->addSceneNode( "NativeEditorCameraNode" );
    auto camera = scene->addGraphicsObjectByType<render::IGraphicsCamera>();
    BOOST_REQUIRE( node );
    BOOST_REQUIRE( camera );
    BOOST_TEST_MESSAGE( "Native editor camera backend: " << typeid( *camera ).name() );
    if( std::string( typeid( *camera ).name() ).find( "ClawCamera" ) == std::string::npos )
    {
        graphicsSystem->removeGraphicsScene( scene );
        graphicsSystem->clearObjectQueues();
        return;
    }
    node->attachObject( camera );

    // The editor pushes world transforms directly into the native scene node.
    // Its C++ transform state need not have been updated when rendering starts.
    for( const auto yaw : { 0.0f, 90.0f, -45.0f } )
    {
        Transform3<real_Num> transform;
        transform.setPosition( Vector3<real_Num>( 7, 3, -9 ) );
        transform.setOrientation( Quaternion<real_Num>::eulerDegrees( 15, yaw, 10 ) );
        node->setWorldTransform( transform );

        Matrix4<real_Num> expected;
        expected.makeViewMatrix( transform.getPosition(), transform.getOrientation(), nullptr );
        const auto actual = camera->getViewMatrix();
        for( size_t i = 0; i < 16; ++i )
        {
            BOOST_CHECK_SMALL( actual.ptr()[i] - expected.ptr()[i], real_Num( 0.0001 ) );
        }
    }

    scene->removeGraphicsObject( camera );
    camera = nullptr;
    node = nullptr;
    graphicsSystem->removeGraphicsScene( scene );
    graphicsSystem->clearObjectQueues();
}

static SmartPtr<render::IGraphicsCamera> createCameraWithScene(
    const String &sceneLabel, const String &nodeName, SmartPtr<render::IGraphicsScene> &outScene,
    SmartPtr<render::IGraphicsSceneNode> &outNode )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return nullptr;
    }

    if( !outScene )
    {
        outScene = graphicsSystem->addGraphicsScene( "DefaultScene", sceneLabel );
        BOOST_CHECK( outScene );
    }

    outNode = outScene->addSceneNode( nodeName );
    BOOST_CHECK( outNode );

    auto camera = outScene->addGraphicsObjectByType<render::IGraphicsCamera>();
    BOOST_CHECK( camera );

    outNode->attachObject( camera );
    return camera;
}

BOOST_AUTO_TEST_CASE( graphics_camera )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        if( graphicsSystem )
        {
            auto scene = graphicsSystem->getGraphicsScenePtr();
            BOOST_CHECK( scene );

            auto cameraNode = scene->addSceneNode( "CameraNode" );
            BOOST_CHECK( cameraNode );

            auto camera = scene->addGraphicsObjectByType<render::IGraphicsCamera>();
            BOOST_CHECK( camera );
        camera->load( nullptr );

            cameraNode->attachObject( camera );

            camera = nullptr;
            cameraNode = nullptr;

            graphicsSystem->clearObjectQueues();
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( graphics_camera_fov )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( graphicsSystem )
    {
        auto scene = graphicsSystem->getGraphicsScenePtr();
        BOOST_CHECK( scene );

        auto camera = scene->addGraphicsObjectByType<render::IGraphicsCamera>();
        BOOST_CHECK( camera );
        camera->load( nullptr );

        // Test setting and getting field of view
        const f32 testFov = 60.0f;
        camera->setFOVy( testFov );
        BOOST_CHECK_CLOSE( camera->getFOVy(), testFov, 0.001f );

        // Test different FOV value
        const f32 wideFov = 90.0f;
        camera->setFOVy( wideFov );
        BOOST_CHECK_CLOSE( camera->getFOVy(), wideFov, 0.001f );

        graphicsSystem->clearObjectQueues();
    }
}

BOOST_AUTO_TEST_CASE( graphics_camera_clip_distances )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( graphicsSystem )
    {
        auto scene = graphicsSystem->getGraphicsScenePtr();
        BOOST_CHECK( scene );

        auto camera = scene->addGraphicsObjectByType<render::IGraphicsCamera>();
        BOOST_CHECK( camera );
        camera->load( nullptr );

        // Test setting and getting near clip distance
        const f32 nearDist = 0.5f;
        camera->setNearClipDistance( nearDist );
        BOOST_CHECK_CLOSE( camera->getNearClipDistance(), nearDist, 0.001f );

        // Test setting and getting far clip distance
        const f32 farDist = 5000.0f;
        camera->setFarClipDistance( farDist );
        BOOST_CHECK_CLOSE( camera->getFarClipDistance(), farDist, 0.001f );

        graphicsSystem->clearObjectQueues();
    }
}

BOOST_AUTO_TEST_CASE( graphics_camera_aspect_ratio )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( graphicsSystem )
    {
        auto scene = graphicsSystem->getGraphicsScenePtr();
        BOOST_CHECK( scene );

        auto camera = scene->addGraphicsObjectByType<render::IGraphicsCamera>();
        BOOST_CHECK( camera );
        camera->load( nullptr );

        // Test setting and getting aspect ratio
        const f32 aspectRatio = 16.0f / 9.0f;
        camera->setAspectRatio( aspectRatio );
        BOOST_CHECK_CLOSE( camera->getAspectRatio(), aspectRatio, 0.001f );

        // Test 4:3 aspect ratio
        const f32 aspectRatio43 = 4.0f / 3.0f;
        camera->setAspectRatio( aspectRatio43 );
        BOOST_CHECK_CLOSE( camera->getAspectRatio(), aspectRatio43, 0.001f );

        graphicsSystem->clearObjectQueues();
    }
}

BOOST_AUTO_TEST_CASE( graphics_camera_auto_aspect_ratio )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( graphicsSystem )
    {
        auto scene = graphicsSystem->getGraphicsScenePtr();
        BOOST_CHECK( scene );

        auto camera = scene->addGraphicsObjectByType<render::IGraphicsCamera>();
        BOOST_CHECK( camera );
        camera->load( nullptr );

        // Test enabling auto aspect ratio
        camera->setAutoAspectRatio( true );
        BOOST_CHECK_EQUAL( camera->getAutoAspectRatio(), true );

        // Test disabling auto aspect ratio
        camera->setAutoAspectRatio( false );
        BOOST_CHECK_EQUAL( camera->getAutoAspectRatio(), false );

        graphicsSystem->clearObjectQueues();
    }
}

BOOST_AUTO_TEST_CASE( graphics_camera_lod_bias )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( graphicsSystem )
    {
        auto scene = graphicsSystem->getGraphicsScenePtr();
        BOOST_CHECK( scene );

        auto camera = scene->addGraphicsObjectByType<render::IGraphicsCamera>();
        BOOST_CHECK( camera );
        camera->load( nullptr );

        // Test setting and getting LOD bias
        const f32 lodBias = 2.0f;
        camera->setLodBias( lodBias );
        BOOST_CHECK_CLOSE( camera->getLodBias(), lodBias, 0.001f );

        // Test default LOD bias
        camera->setLodBias( 1.0f );
        BOOST_CHECK_CLOSE( camera->getLodBias(), 1.0f, 0.001f );

        graphicsSystem->clearObjectQueues();
    }
}

BOOST_AUTO_TEST_CASE( graphics_camera_screen_dimensions )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( graphicsSystem )
    {
        auto scene = graphicsSystem->getGraphicsScenePtr();
        BOOST_CHECK( scene );

        auto camera = scene->addGraphicsObjectByType<render::IGraphicsCamera>();
        BOOST_CHECK( camera );
        camera->load( nullptr );

        // Test setting and getting screen width
        const s32 screenWidth = 1920;
        camera->setScreenWidth( screenWidth );
        BOOST_CHECK_EQUAL( camera->getScreenWidth(), screenWidth );

        // Test setting and getting screen height
        const s32 screenHeight = 1080;
        camera->setScreenHeight( screenHeight );
        BOOST_CHECK_EQUAL( camera->getScreenHeight(), screenHeight );

        graphicsSystem->clearObjectQueues();
    }
}

BOOST_AUTO_TEST_CASE( graphics_camera_render_ui )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( graphicsSystem )
    {
        auto scene = graphicsSystem->getGraphicsScenePtr();
        BOOST_CHECK( scene );

        auto camera = scene->addGraphicsObjectByType<render::IGraphicsCamera>();
        BOOST_CHECK( camera );
        camera->load( nullptr );

        // Test enabling render UI
        camera->setRenderUI( true );
        BOOST_CHECK_EQUAL( camera->getRenderUI(), true );

        // Test disabling render UI
        camera->setRenderUI( false );
        BOOST_CHECK_EQUAL( camera->getRenderUI(), false );

        graphicsSystem->clearObjectQueues();
    }
}

BOOST_AUTO_TEST_CASE( graphics_camera_direction_vectors )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( graphicsSystem )
    {
        auto scene = graphicsSystem->getGraphicsScenePtr();
        BOOST_CHECK( scene );

        auto cameraNode = scene->addSceneNode( "DirectionCameraNode" );
        BOOST_CHECK( cameraNode );

        auto camera = scene->addGraphicsObjectByType<render::IGraphicsCamera>();
        BOOST_CHECK( camera );
        camera->load( nullptr );

        cameraNode->attachObject( camera );

        // Test that direction vectors are valid (non-zero length)
        auto direction = camera->getDirection();
        auto up = camera->getUp();
        auto right = camera->getRight();

        // Direction vectors should have unit length (approximately 1.0)
        f32 dirLength = std::sqrt( direction.X() * direction.X() + direction.Y() * direction.Y() +
                                   direction.Z() * direction.Z() );
        f32 upLength = std::sqrt( up.X() * up.X() + up.Y() * up.Y() + up.Z() * up.Z() );
        f32 rightLength =
            std::sqrt( right.X() * right.X() + right.Y() * right.Y() + right.Z() * right.Z() );

        BOOST_CHECK_CLOSE( dirLength, 1.0f, 0.01f );
        BOOST_CHECK_CLOSE( upLength, 1.0f, 0.01f );
        BOOST_CHECK_CLOSE( rightLength, 1.0f, 0.01f );

        cameraNode = nullptr;
        camera = nullptr;

        graphicsSystem->clearObjectQueues();
    }
}

// New tests

BOOST_AUTO_TEST_CASE( graphics_camera_defaults_and_nulls )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( graphicsSystem )
    {
        SmartPtr<render::IGraphicsScene> scene = graphicsSystem->getGraphicsScene();
        SmartPtr<render::IGraphicsSceneNode> node;
        auto camera = createCameraWithScene( "DefaultsScene", "DefaultsNode", scene, node );

        // Initially there should be no target or editor texture and no viewport created
        BOOST_CHECK( camera );
        BOOST_CHECK( camera->getTargetTexture() == nullptr );
        BOOST_CHECK( camera->getViewport() == nullptr );

        // Render UI should be a boolean (check toggle works)
        camera->setRenderUI( true );
        BOOST_CHECK_EQUAL( camera->getRenderUI(), true );
        camera->setRenderUI( false );
        BOOST_CHECK_EQUAL( camera->getRenderUI(), false );

        node = nullptr;
        camera = nullptr;

        graphicsSystem->clearObjectQueues();
    }
}

BOOST_AUTO_TEST_CASE( graphics_camera_direction_orthogonality )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( graphicsSystem )
    {
        SmartPtr<render::IGraphicsScene> scene = graphicsSystem->getGraphicsScene();
        SmartPtr<render::IGraphicsSceneNode> node;
        auto camera = createCameraWithScene( "OrthoScene", "OrthoNode", scene, node );

        // Get basis vectors
        auto dir = camera->getDirection();
        auto up = camera->getUp();
        auto right = camera->getRight();

        // Helper to compute dot product
        auto dot = []( const Vector3<real_Num> &a, const Vector3<real_Num> &b ) -> f32 {
            return a.X() * b.X() + a.Y() * b.Y() + a.Z() * b.Z();
        };

        // Lengths should be ~1
        auto len = []( const Vector3<real_Num> &a ) -> f32 {
            return std::sqrt( a.X() * a.X() + a.Y() * a.Y() + a.Z() * a.Z() );
        };

        BOOST_CHECK_CLOSE( len( dir ), 1.0f, 0.01f );
        BOOST_CHECK_CLOSE( len( up ), 1.0f, 0.01f );
        BOOST_CHECK_CLOSE( len( right ), 1.0f, 0.01f );

        // Orthogonality: dot products should be approximately zero
        BOOST_CHECK( std::abs( dot( dir, up ) ) < 1e-3f );
        BOOST_CHECK( std::abs( dot( dir, right ) ) < 1e-3f );
        BOOST_CHECK( std::abs( dot( up, right ) ) < 1e-3f );

        node = nullptr;
        camera = nullptr;

        graphicsSystem->clearObjectQueues();
    }
}

BOOST_AUTO_TEST_CASE( graphics_camera_direction_screen_positions )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( graphicsSystem )
    {
        SmartPtr<render::IGraphicsScene> scene = graphicsSystem->getGraphicsScene();
        SmartPtr<render::IGraphicsSceneNode> node;
        auto camera = createCameraWithScene( "ScreenDirScene", "ScreenDirNode", scene, node );

        // center of the screen should give direction similar to camera->getDirection()
        Vector2<real_Num> centerPos( 0.5f, 0.5f );
        auto centerDir = camera->getDirection( centerPos );
        auto forward = camera->getDirection();

        // Normalize both
        auto normalize = []( Vector3<real_Num> v ) {
            f32 l = std::sqrt( v.X() * v.X() + v.Y() * v.Y() + v.Z() * v.Z() );
            if( l > 0 )
            {
                v.X() /= l;
                v.Y() /= l;
                v.Z() /= l;
            }
            return v;
        };

        centerDir = normalize( centerDir );
        forward = normalize( forward );

        // They should be similar (pointing same direction)
        auto diff = Vector3<real_Num>( centerDir.X() - forward.X(), centerDir.Y() - forward.Y(),
                                       centerDir.Z() - forward.Z() );
        f32 dist = std::sqrt( diff.X() * diff.X() + diff.Y() * diff.Y() + diff.Z() * diff.Z() );
        BOOST_CHECK( dist < 1e-2f );

        // Corner positions should produce normalized directions
        Vector2<real_Num> corners[4] = {
            { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f }
        };
        for( auto &c : corners )
        {
            auto d = camera->getDirection( c );
            f32 l = std::sqrt( d.X() * d.X() + d.Y() * d.Y() + d.Z() * d.Z() );
            BOOST_CHECK_CLOSE( l, 1.0f, 0.05f );
        }

        node = nullptr;
        camera = nullptr;

        graphicsSystem->clearObjectQueues();
    }
}
