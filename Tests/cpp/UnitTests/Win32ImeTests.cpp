#include "UnitTests.hpp"
#include <boost/test/unit_test.hpp>

#if defined WP_PLATFORM_WIN32
#    include <WPGraphics/ClawWin32Ime.hpp>
#    include <array>
#    include <chrono>
#    include <future>
#    include <limits>
#    include <memory>
#    include <thread>
#    include <vector>

namespace
{
    namespace ime = workphone::render::detail;

    struct ImeDelivery
    {
        bool visible = false;
        LONG x = 0;
        LONG y = 0;
        DWORD threadId = 0;
        bool handled = false;
    };

    struct ImeWindowState
    {
        std::vector<ImeDelivery> deliveries;
    };

    LRESULT CALLBACK imeTestWindowProc( HWND hwnd, UINT message, WPARAM x, LPARAM y )
    {
        if( message == WM_NCCREATE )
        {
            auto create = reinterpret_cast<CREATESTRUCTW *>( y );
            SetWindowLongPtrW( hwnd, GWLP_USERDATA,
                              reinterpret_cast<LONG_PTR>( create->lpCreateParams ) );
        }

        auto state = reinterpret_cast<ImeWindowState *>( GetWindowLongPtrW( hwnd, GWLP_USERDATA ) );
        if( state && message &&
            ( message == ime::platformImeMessage( true ) ||
              message == ime::platformImeMessage( false ) ) )
        {
            const bool handled = ime::handlePlatformImeMessage( hwnd, message, x, y );
            state->deliveries.push_back( { message == ime::platformImeMessage( true ),
                                          static_cast<LONG>( x ), static_cast<LONG>( y ),
                                          GetCurrentThreadId(), handled } );
            return handled ? 0 : DefWindowProcW( hwnd, message, x, y );
        }
        return DefWindowProcW( hwnd, message, x, y );
    }

    class HiddenImeWindow
    {
    public:
        HiddenImeWindow()
        {
            static const wchar_t className[] = L"Workphone.Win32Ime.Tests";
            static const ATOM registered = []() {
                WNDCLASSW windowClass{};
                windowClass.lpfnWndProc = imeTestWindowProc;
                windowClass.hInstance = GetModuleHandleW( nullptr );
                windowClass.lpszClassName = className;
                return RegisterClassW( &windowClass );
            }();
            if( registered )
            {
                // No visible window, renderer, device, or native input is needed.
                hwnd = CreateWindowExW( 0, className, L"", WS_POPUP, 0, 0, 1, 1,
                                        nullptr, nullptr, GetModuleHandleW( nullptr ), &state );
            }
        }

        ~HiddenImeWindow() { destroy(); }

        void destroy()
        {
            if( hwnd ) DestroyWindow( hwnd );
            hwnd = nullptr;
        }

        void pump()
        {
            MSG message{};
            // Keep recovery bounded even if a broken handler posts repeatedly.
            for( size_t count = 0; count < 128 &&
                 PeekMessageW( &message, hwnd, 0, 0, PM_REMOVE ); ++count )
            {
                TranslateMessage( &message );
                DispatchMessageW( &message );
            }
        }

        ImeWindowState state;
        HWND hwnd = nullptr;
    };
}

BOOST_AUTO_TEST_CASE( win32_ime_coordinates_are_finite_and_preserve_signed_range )
{
    BOOST_CHECK_EQUAL( ime::platformImeCoordinate( -70000.75f ), -70000L );
    BOOST_CHECK_EQUAL( ime::platformImeCoordinate( 90000.5f ), 90000L );
    BOOST_CHECK_EQUAL( ime::platformImeCoordinate( 0.0f ), 0L );
    BOOST_CHECK_EQUAL( ime::platformImeCoordinate( std::numeric_limits<float>::quiet_NaN() ), 0L );
    BOOST_CHECK_EQUAL( ime::platformImeCoordinate( std::numeric_limits<float>::infinity() ), 0L );
    BOOST_CHECK_EQUAL( ime::platformImeCoordinate( -std::numeric_limits<float>::infinity() ), 0L );
    BOOST_CHECK_EQUAL( ime::platformImeCoordinate( ( std::numeric_limits<float>::max )() ),
                       ( std::numeric_limits<LONG>::max )() );
    BOOST_CHECK_EQUAL( ime::platformImeCoordinate( -( std::numeric_limits<float>::max )() ),
                       ( std::numeric_limits<LONG>::min )() );
}

BOOST_AUTO_TEST_CASE( win32_ime_publication_returns_before_owner_pumps_and_delivers_values_in_order )
{
    HiddenImeWindow window;
    BOOST_REQUIRE( window.hwnd );
    BOOST_REQUIRE( ime::platformImeMessage( true ) );
    BOOST_REQUIRE( ime::platformImeMessage( false ) );
    BOOST_REQUIRE_NE( ime::platformImeMessage( true ), ime::platformImeMessage( false ) );

    struct PublicationResult
    {
        bool posted = true;
        bool rejectedWrongThread = false;
        DWORD threadId = 0;
    };
    const std::array<ImeDelivery, 3> expected = {{
        { true, -70000L, 90000L },
        { false, ( std::numeric_limits<LONG>::min )(), ( std::numeric_limits<LONG>::max )() },
        { true, ( std::numeric_limits<LONG>::max )(), ( std::numeric_limits<LONG>::min )() }
    }};
    auto completion = std::make_shared<std::promise<PublicationResult>>();
    auto completed = completion->get_future();
    const auto hwnd = window.hwnd;
    std::thread publisher( [hwnd, expected, completion]() {
        PublicationResult result;
        result.threadId = GetCurrentThreadId();
        result.rejectedWrongThread = !ime::handlePlatformImeMessage(
            hwnd, ime::platformImeMessage( true ), 0, 0 );
        auto sourceValues = expected;
        for( const auto &update : sourceValues )
            result.posted = ime::postPlatformImeUpdate(
                hwnd, update.visible, update.x, update.y ) && result.posted;
        // Only values travel through the native queue. Mutating and destroying
        // producer storage must not change a later window-thread delivery.
        sourceValues = {};
        completion->set_value( result );
    } );

    // Models scene teardown waiting for the renderer: this thread intentionally
    // does not process HWND messages while the render-side publisher finishes.
    const bool finishedWithoutOwnerPump = completed.wait_for( std::chrono::seconds( 2 ) ) ==
                                          std::future_status::ready;
    BOOST_CHECK( finishedWithoutOwnerPump );
    BOOST_CHECK( window.state.deliveries.empty() );

    // If publication regresses to a synchronous SendMessage, allow it to unwind
    // after the bounded failure above instead of hanging the test's thread join.
    const auto recoveryDeadline = std::chrono::steady_clock::now() + std::chrono::seconds( 2 );
    while( completed.wait_for( std::chrono::milliseconds( 0 ) ) != std::future_status::ready &&
           std::chrono::steady_clock::now() < recoveryDeadline )
    {
        window.pump();
        std::this_thread::yield();
    }
    const bool recovered = completed.wait_for( std::chrono::milliseconds( 0 ) ) ==
                           std::future_status::ready;
    if( recovered ) publisher.join();
    else publisher.detach(); // Captures only copied HWND/value/shared promise.
    BOOST_REQUIRE_MESSAGE( recovered, "IME publisher did not complete after bounded recovery" );
    const auto result = completed.get();
    BOOST_CHECK( result.posted );
    BOOST_CHECK( result.rejectedWrongThread );
    BOOST_CHECK_NE( result.threadId, GetCurrentThreadId() );

    window.pump();
    BOOST_REQUIRE_EQUAL( window.state.deliveries.size(), expected.size() );
    for( size_t i = 0; i < expected.size(); ++i )
    {
        const auto &actual = window.state.deliveries[i];
        BOOST_CHECK_EQUAL( actual.visible, expected[i].visible );
        BOOST_CHECK_EQUAL( actual.x, expected[i].x );
        BOOST_CHECK_EQUAL( actual.y, expected[i].y );
        BOOST_CHECK_EQUAL( actual.threadId, GetCurrentThreadId() );
        BOOST_CHECK( actual.handled );
    }
}

BOOST_AUTO_TEST_CASE( win32_ime_transport_rejects_missing_windows_and_unrelated_messages )
{
    BOOST_CHECK( !ime::postPlatformImeUpdate( nullptr, true, 1, 2 ) );
    BOOST_CHECK( !ime::handlePlatformImeMessage( nullptr, WM_NULL, 0, 0 ) );
    HiddenImeWindow window;
    BOOST_REQUIRE( window.hwnd );
    BOOST_CHECK( !ime::handlePlatformImeMessage( window.hwnd, WM_NULL, 0, 0 ) );
    const auto destroyed = window.hwnd;
    window.destroy();
    BOOST_CHECK( !ime::postPlatformImeUpdate( destroyed, false, 3, 4 ) );
}
#endif
