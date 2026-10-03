#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

#if WP_GRAPHICS_SYSTEM_CLAW && defined( WP_PLATFORM_WIN32 )
#    include <WPGraphics/ClawWindow.hpp>
#    include <windows.h>
#endif

using namespace workphone;

#if WP_GRAPHICS_SYSTEM_CLAW && defined( WP_PLATFORM_WIN32 )
BOOST_AUTO_TEST_CASE( claw_window_state_changes_preserve_actual_size )
{
    TestGuard guard;
    auto window = guard.factoryManager->make_ptr<render::ClawWindow>();
    guard.addCleanup( [window]() mutable { window->unload( nullptr ); } );
    window->setSize( Vector2I( 800, 600 ) );
    window->load( nullptr );
    BOOST_REQUIRE( window->isLoaded() );

    auto context = window->getStateContext();
    BOOST_REQUIRE( context );
    auto data = context->getStateDataById<RenderTargetStateData>( window->getId() );
    BOOST_REQUIRE( data );
    SmartPtr<IState> state = guard.factoryManager->make_ptr<State>();
    state->setOwner( window.get() );
    state->setData( SmartPtr<ISharedObject>( data.get() ) );

    // An unrelated state update must not apply the default 128 x 128 size.
    data->size = Vector2I( 128, 128 );
    window->setPriority( 100 );
    BOOST_CHECK( window->handleStateChanged( state ) );
    BOOST_CHECK_EQUAL( window->getSize().X(), 800 );
    BOOST_CHECK_EQUAL( window->getSize().Y(), 600 );
    BOOST_CHECK_EQUAL( data->size.X(), 800 );
    BOOST_CHECK_EQUAL( data->size.Y(), 600 );

    // Simulate an external resize, then deliver another update with stale size data.
    HWND hwnd = nullptr;
    window->getWindowHandle( &hwnd );
    BOOST_REQUIRE( hwnd );
    BOOST_REQUIRE( SetWindowPos( hwnd, nullptr, 0, 0, 940, 710,
                                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE ) );
    BOOST_REQUIRE( window->messagePump() );
    RECT clientRect = {};
    BOOST_REQUIRE( GetClientRect( hwnd, &clientRect ) );
    const auto width = clientRect.right - clientRect.left;
    const auto height = clientRect.bottom - clientRect.top;
    BOOST_REQUIRE( width != 800 || height != 600 );
    window->setAutoUpdated( false );
    BOOST_CHECK( window->handleStateChanged( state ) );
    BOOST_REQUIRE( GetClientRect( hwnd, &clientRect ) );
    BOOST_CHECK_EQUAL( clientRect.right - clientRect.left, width );
    BOOST_CHECK_EQUAL( clientRect.bottom - clientRect.top, height );
    BOOST_CHECK_EQUAL( data->size.X(), width );
    BOOST_CHECK_EQUAL( data->size.Y(), height );

    // Explicit resize requests must still work.
    window->setSize( Vector2I( 1024, 768 ) );
    BOOST_CHECK( window->handleStateChanged( state ) );
    BOOST_CHECK_EQUAL( window->getSize().X(), 1024 );
    BOOST_CHECK_EQUAL( window->getSize().Y(), 768 );
    window->resize( 800, 600 );
    BOOST_CHECK( window->handleStateChanged( state ) );
    BOOST_CHECK_EQUAL( data->size.X(), 800 );
    BOOST_CHECK_EQUAL( data->size.Y(), 600 );
}
#endif

BOOST_AUTO_TEST_CASE( graphics_window_creation )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( !graphicsSystem )
        return;

    auto window = graphicsSystem->createRenderWindow( "TestWindow", 800, 600, false, nullptr );
    BOOST_CHECK( window );

    graphicsSystem->destroyRenderWindow( window );
}

BOOST_AUTO_TEST_CASE( graphics_window_title )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( !graphicsSystem )
        return;

    auto window = graphicsSystem->createRenderWindow( "TestWindowTitle", 800, 600, false, nullptr );
    BOOST_REQUIRE( window );

    // Test getTitle returns the expected value
    BOOST_CHECK_EQUAL( window->getTitle(), "TestWindowTitle" );

    // Test setTitle updates the title
    window->setTitle( "NewTitle" );
    BOOST_CHECK_EQUAL( window->getTitle(), "NewTitle" );

    graphicsSystem->destroyRenderWindow( window );
}

BOOST_AUTO_TEST_CASE( graphics_window_size )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( !graphicsSystem )
        return;

    auto window = graphicsSystem->createRenderWindow( "TestWindowSize", 800, 600, false, nullptr );
    BOOST_REQUIRE( window );

    // Test getSize returns expected dimensions
    auto size = window->getSize();
    BOOST_CHECK_EQUAL( size.X(), 800 );
    BOOST_CHECK_EQUAL( size.Y(), 600 );

    // Test setSize updates the window size
    window->setSize( Vector2I( 1024, 768 ) );
    size = window->getSize();
    BOOST_CHECK_EQUAL( size.X(), 1024 );
    BOOST_CHECK_EQUAL( size.Y(), 768 );

    graphicsSystem->destroyRenderWindow( window );
}

BOOST_AUTO_TEST_CASE( graphics_window_visibility )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( !graphicsSystem )
        return;

    auto window = graphicsSystem->createRenderWindow( "TestWindowVisibility", 800, 600, false, nullptr );
    BOOST_REQUIRE( window );

    // Test visibility state
    window->setVisible( true );
    BOOST_CHECK( window->isVisible() );

    window->setVisible( false );
    BOOST_CHECK( !window->isVisible() );

    graphicsSystem->destroyRenderWindow( window );
}

BOOST_AUTO_TEST_CASE( graphics_window_fullscreen )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( !graphicsSystem )
        return;

    // Create window in windowed mode
    auto window = graphicsSystem->createRenderWindow( "TestWindowFullscreen", 800, 600, false, nullptr );
    BOOST_REQUIRE( window );

    // Window should not be in fullscreen mode initially
    BOOST_CHECK( !window->isFullScreen() );

    graphicsSystem->destroyRenderWindow( window );
}

BOOST_AUTO_TEST_CASE( graphics_window_active_state )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( !graphicsSystem )
        return;

    auto window = graphicsSystem->createRenderWindow( "TestWindowActive", 800, 600, false, nullptr );
    BOOST_REQUIRE( window );

    // Test active state
    window->setActive( true );
    BOOST_CHECK( window->isActive() );

    window->setActive( false );
    BOOST_CHECK( !window->isActive() );

    graphicsSystem->destroyRenderWindow( window );
}

BOOST_AUTO_TEST_CASE( graphics_window_auto_updated )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( !graphicsSystem )
        return;

    auto window = graphicsSystem->createRenderWindow( "TestWindowAutoUpdate", 800, 600, false, nullptr );
    BOOST_REQUIRE( window );

    // Test auto-update setting
    window->setAutoUpdated( true );
    BOOST_CHECK( window->isAutoUpdated() );

    window->setAutoUpdated( false );
    BOOST_CHECK( !window->isAutoUpdated() );

    graphicsSystem->destroyRenderWindow( window );
}

BOOST_AUTO_TEST_CASE( graphics_window_priority )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( !graphicsSystem )
        return;

    auto window = graphicsSystem->createRenderWindow( "TestWindowPriority", 800, 600, false, nullptr );
    BOOST_REQUIRE( window );

    // Test priority setting
    window->setPriority( 100 );
    BOOST_CHECK_EQUAL( window->getPriority(), 100 );

    window->setPriority( 200 );
    BOOST_CHECK_EQUAL( window->getPriority(), 200 );

    graphicsSystem->destroyRenderWindow( window );
}

BOOST_AUTO_TEST_CASE( graphics_window_closed_state )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( !graphicsSystem )
        return;

    auto window = graphicsSystem->createRenderWindow( "TestWindowClosed", 800, 600, false, nullptr );
    BOOST_REQUIRE( window );

    // Window should not be closed after creation
    BOOST_CHECK( !window->isClosed() );

    graphicsSystem->destroyRenderWindow( window );
}

BOOST_AUTO_TEST_CASE( graphics_window_colour_depth )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( !graphicsSystem )
        return;

    auto window =
        graphicsSystem->createRenderWindow( "TestWindowColourDepth", 800, 600, false, nullptr );
    BOOST_REQUIRE( window );

    // Test colour depth accessor (typically 32-bit for modern displays)
    auto colourDepth = window->getColourDepth();
    BOOST_CHECK( colourDepth > 0 );

    graphicsSystem->destroyRenderWindow( window );
}

BOOST_AUTO_TEST_CASE( graphics_window_viewports )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( !graphicsSystem )
        return;

    auto window = graphicsSystem->createRenderWindow( "TestWindowViewports", 800, 600, false, nullptr );
    BOOST_REQUIRE( window );

    // Initially should have no viewports
    BOOST_CHECK_EQUAL( window->getNumViewports(), 0 );

    // Get viewports array should be empty
    auto viewports = window->getViewports();
    BOOST_CHECK( viewports.empty() );

    graphicsSystem->destroyRenderWindow( window );
}

BOOST_AUTO_TEST_CASE( graphics_window_deactivate_on_focus_change )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( !graphicsSystem )
        return;

    auto window = graphicsSystem->createRenderWindow( "TestWindowFocus", 800, 600, false, nullptr );
    BOOST_REQUIRE( window );

    // Test deactivate on focus change setting
    window->setDeactivateOnFocusChange( true );
    BOOST_CHECK( window->isDeactivatedOnFocusChange() );

    window->setDeactivateOnFocusChange( false );
    BOOST_CHECK( !window->isDeactivatedOnFocusChange() );

    graphicsSystem->destroyRenderWindow( window );
}
