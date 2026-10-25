#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Input/InputDeviceManager.hpp>
#include <Workphone/Input/KeyboardState.hpp>
#include <Workphone/Graphics/GraphicsWindow.hpp>
#include <Workphone/System/WindowMessageData.hpp>
#include <boost/test/unit_test.hpp>
#include <atomic>
#include <functional>
#include <thread>

#if defined WP_PLATFORM_WIN32
#    include <Windows.h>

using namespace workphone;

namespace
{
    class SnapshotInput : public InputDeviceManager
    {
    public:
        SnapshotInput()
        {
            // The base window only stores listeners; loading the input manager
            // attaches its real native listener without creating an OS window.
            m_testWindow = make_ptr<render::GraphicsWindow>();
            setWindow( m_testWindow );
            load( nullptr );
            // Event fan-out queues are unrelated to snapshot publication and
            // would retain the previous key event until each task consumes it.
            m_inputEventQueue.clear();
        }

        void installSnapshot( SmartPtr<IKeyboardState> state )
        {
            ScopedLock lock( &m_mutex );
            m_currentKeyboardState = state;
        }

        bool trySnapshotLock()
        {
            if( !m_mutex.try_lock() ) return false;
            m_mutex.unlock();
            return true;
        }

        void keyMessage( u32 message )
        {
            auto event = make_ptr<WindowMessageData>();
            event->setMessage( message );
            event->setWParam( VK_ESCAPE );
            event->setLParam( 0 );
            // Exercise the actual native publication path without sending any
            // Windows input or creating a window/device.
            for( auto listener : m_testWindow->getListeners() ) listener->handleEvent( event );
        }

    private:
        SmartPtr<render::GraphicsWindow> m_testWindow;
    };

    class SnapshotLifetimeProbe : public KeyboardState
    {
    public:
        std::function<void()> onDestroy;
        ~SnapshotLifetimeProbe() override { if( onDestroy ) onDestroy(); }
    };
}

BOOST_AUTO_TEST_CASE( input_keyboard_snapshot_publication_holds_reference_acquisition_lock )
{
    // The old snapshot's final reference is released during assignment. Inspect
    // the publication gate at that precise boundary using nonblocking try_lock;
    // an unprotected old implementation fails without a race, sleep, or hang.
    auto input = make_ptr<SnapshotInput>();
    bool inspected = false;
    bool publicationWasProtected = false;
    auto previous = make_ptr<SnapshotLifetimeProbe>();
    previous->onDestroy = [&]() {
        std::thread reader( [&]() {
            publicationWasProtected = !input->trySnapshotLock();
            inspected = true;
        } );
        reader.join();
    };
    input->installSnapshot( previous );
    previous = nullptr;
    input->keyMessage( WM_KEYUP );
    BOOST_REQUIRE( inspected );
    BOOST_REQUIRE( publicationWasProtected );

    auto held = input->getCurrentKeyboardState();
    BOOST_REQUIRE( held );
    BOOST_CHECK_EQUAL( held->getKeyCode(), static_cast<u32>( KeyCodes::KEY_ESCAPE ) );
    BOOST_CHECK( !held->isPressedDown() );
    const auto id = held->getId();
    WeakPtr<IKeyboardState> retained( held );
    input->keyMessage( WM_KEYDOWN );
    auto replacement = input->getCurrentKeyboardState();
    BOOST_REQUIRE( replacement );
    BOOST_CHECK_NE( replacement->getId(), id );
    BOOST_CHECK( replacement->isPressedDown() );
    BOOST_CHECK_EQUAL( held->getId(), id );
    BOOST_CHECK( !retained.expired() );
    held = nullptr;
    BOOST_CHECK( retained.expired() );
}

BOOST_AUTO_TEST_CASE( input_keyboard_snapshot_concurrent_reads_keep_replaced_events_alive )
{
    auto input = make_ptr<SnapshotInput>();
    input->keyMessage( WM_KEYUP );
    auto retained = input->getCurrentKeyboardState();
    BOOST_REQUIRE( retained );
    const auto id = retained->getId();
    std::atomic_bool start{ false };
    std::atomic_bool valid{ true };
    std::thread publisher( [&]() {
        while( !start.load( std::memory_order_acquire ) ) std::this_thread::yield();
        for( size_t i = 0; i < 2048; ++i ) input->keyMessage( i % 2 ? WM_KEYDOWN : WM_KEYUP );
    } );
    std::thread reader( [&]() {
        while( !start.load( std::memory_order_acquire ) ) std::this_thread::yield();
        for( size_t i = 0; i < 4096; ++i )
        {
            auto snapshot = input->getCurrentKeyboardState();
            if( !snapshot || snapshot->getKeyCode() != static_cast<u32>( KeyCodes::KEY_ESCAPE ) )
                valid.store( false, std::memory_order_relaxed );
        }
    } );
    start.store( true, std::memory_order_release );
    publisher.join();
    reader.join();
    BOOST_CHECK( valid.load() );
    BOOST_CHECK_EQUAL( retained->getId(), id );
    BOOST_CHECK( !retained->isPressedDown() );
    auto latest = input->getCurrentKeyboardState();
    BOOST_REQUIRE( latest );
    BOOST_CHECK_NE( latest->getId(), id );
}
#endif
