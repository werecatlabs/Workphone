#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Input/InputDeviceManager.hpp>
#include <Workphone/Input/KeyboardState.hpp>
#include <Workphone/Input/InputEvent.hpp>
#include <Workphone/Interface/Input/IGameInputMap.hpp>
#include <Workphone/Interface/Input/IGameInputState.hpp>
#include <Workphone/Sound/SoundManager.hpp>
#include <Workphone/System/InteractionBackendProviders.hpp>
#include <Workphone/Memory/FactoryUtil.hpp>
#include <Workphone/Graphics/GraphicsWindow.hpp>
#include <Workphone/System/WindowMessageData.hpp>
#include <boost/test/unit_test.hpp>
#include <atomic>
#include <functional>
#include <thread>
#include <stdexcept>

#if defined WP_PLATFORM_WIN32
#    include <Windows.h>

namespace workphone
{
    class SettingsProviderSound : public SoundManager
    {
    public:
        SmartPtr<Properties> received;
        void setProperties( SmartPtr<Properties> profile ) override { received = profile; }
        WP_CLASS_REGISTER_DECL;
    };
    class SettingsProviderInput : public InputDeviceManager
    {
    public:
        SmartPtr<Properties> received;
        void setProperties( SmartPtr<Properties> profile ) override { received = profile; }
        WP_CLASS_REGISTER_DECL;
    };
    class SettingsProviderHybrid : public SettingsProviderInput
    {
    public:
        WP_CLASS_REGISTER_DECL;
    };
    WP_CLASS_REGISTER_DERIVED( workphone, SettingsProviderSound, SoundManager );
    WP_CLASS_REGISTER_DERIVED( workphone, SettingsProviderInput, InputDeviceManager );
    WP_CLASS_REGISTER_DERIVED( workphone, SettingsProviderHybrid, SettingsProviderInput );
}

using namespace workphone;

BOOST_AUTO_TEST_CASE( project_backend_providers_dispatch_registered_types_with_full_profile )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.factoryManager );
    FactoryUtil::addFactory<SettingsProviderSound>( guard.factoryManager );
    FactoryUtil::addFactory<SettingsProviderInput>( guard.factoryManager );
    FactoryUtil::addFactory<SettingsProviderHybrid>( guard.factoryManager );
    registerProjectSoundBackend( 1, "workphone::SettingsProviderSound" );
    registerProjectInputBackend( 1, "workphone::SettingsProviderInput" );
    registerProjectInputBackend( 3, "workphone::SettingsProviderHybrid" );
    guard.addCleanup( [] { registerProjectSoundBackend( 1, "" ); registerProjectInputBackend( 1, "" );
        registerProjectInputBackend( 3, "" ); } );
    auto profile = make_ptr<Properties>();
    profile->setProperty( "sound.backend", 1 );
    profile->setProperty( "input.system", 1 );
    profile->setProperty( "sound.sampleRate", 44100 );
    profile->setProperty( "input.touch", true );
    BOOST_CHECK( hasProjectSoundBackend( guard.factoryManager, 1 ) );
    BOOST_CHECK( hasProjectInputBackend( guard.factoryManager, 1 ) );
    auto sound = createProjectSoundBackend( guard.factoryManager, profile );
    auto input = createProjectInputBackend( guard.factoryManager, profile );
    auto soundProvider = dynamic_pointer_cast<SettingsProviderSound>( sound );
    auto inputProvider = dynamic_pointer_cast<SettingsProviderInput>( input );
    BOOST_REQUIRE( soundProvider ); BOOST_REQUIRE( inputProvider );
    BOOST_CHECK( soundProvider->received == profile );
    BOOST_CHECK( inputProvider->received == profile );
    BOOST_CHECK_EQUAL( soundProvider->received->getPropertyAsInt( "sound.sampleRate" ), 44100 );
    BOOST_CHECK( inputProvider->received->getPropertyAsBool( "input.touch" ) );
    BOOST_CHECK( matchesProjectSoundBackend( guard.factoryManager, sound, 1 ) );
    BOOST_CHECK( matchesProjectInputBackend( guard.factoryManager, input, 1 ) );
    profile->setProperty( "input.system", 3 );
    auto hybrid = createProjectInputBackend( guard.factoryManager, profile );
    BOOST_CHECK( hybrid->isExactly<SettingsProviderHybrid>() );
    BOOST_CHECK( matchesProjectInputBackend( guard.factoryManager, hybrid, 3 ) );
    BOOST_CHECK( !matchesProjectInputBackend( guard.factoryManager, input, 3 ) );
    BOOST_CHECK( !matchesProjectSoundBackend( guard.factoryManager, sound, 2 ) );
    registerProjectSoundBackend( 2, "workphone::SettingsProviderInput" );
    guard.addCleanup( [] { registerProjectSoundBackend( 2, "" ); } );
    BOOST_CHECK( !hasProjectSoundBackend( guard.factoryManager, 2 ) );
    profile->setProperty( "sound.backend", 2 );
    BOOST_CHECK_THROW( createProjectSoundBackend( guard.factoryManager, profile ), std::runtime_error );
}

namespace
{
    class ProjectSettingsInput : public InputDeviceManager
    {
    public:
        ProjectSettingsInput() { m_inputEventQueue.resize( static_cast<u32>( TaskId::Count ) ); }
    };
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

BOOST_AUTO_TEST_CASE( input_project_settings_gate_events_mapping_and_rebinding )
{
    auto input = make_ptr<ProjectSettingsInput>();
    auto settings = make_ptr<Properties>();
    settings->setProperty( "input.gamepad", false );
    settings->setProperty( "input.touch", false );
    settings->setProperty( "input.gestures", false );
    settings->setProperty( "input.mouseCapture", false );
    settings->setProperty( "input.rebinds", true );
    input->setProperties( settings );
    auto player = input->addGameInput( 91 );
    BOOST_REQUIRE( player );
    auto map = player->getGameInputMap();
    BOOST_REQUIRE( map );
    map->setKeyboardAction( 42, "A", "" );
    BOOST_CHECK_EQUAL( map->getActionFromKey( 65 ), 42u );
    auto key = make_ptr<KeyboardState>();
    key->setKeyCode( 65 ); key->setPressedDown( true );
    auto event = make_ptr<InputEvent>();
    event->setEventType( IInputEvent::EventType::Key ); event->setKeyboardState( key );
    BOOST_CHECK( input->postEvent( event ) );
    BOOST_REQUIRE( event->getGameInputState() );
    BOOST_CHECK_EQUAL( event->getGameInputState()->getAction(), 42u );
    BOOST_CHECK_EQUAL( event->getGameInputId(), 91u );
    settings->setProperty( "input.rebinds", false ); input->setProperties( settings );
    map->setKeyboardAction( 42, "B", "" );
    BOOST_CHECK_EQUAL( map->getActionFromKey( 65 ), 42u );
    BOOST_CHECK_EQUAL( map->getActionFromKey( 66 ), IGameInput::UNASSIGNED );
    settings->setProperty( "input.rebinds", true ); input->setProperties( settings );
    map->setKeyboardAction( 42, "B", "" );
    BOOST_CHECK_EQUAL( map->getActionFromKey( 66 ), 42u );
    auto joystick = make_ptr<InputEvent>(); joystick->setEventType( IInputEvent::EventType::Joystick );
    BOOST_CHECK( !input->postEvent( joystick ) );
    auto touch = make_ptr<InputEvent>(); touch->setEventType( IInputEvent::EventType::User );
    auto touchValues = make_ptr<Properties>(); touchValues->setProperty( "input.kind", "touch" );
    touch->setProperties( touchValues );
    BOOST_CHECK( !input->postEvent( touch ) );
    settings->setProperty( "input.touch", true ); input->setProperties( settings );
    BOOST_CHECK( input->postEvent( touch ) );
    settings->setProperty( "input.system", 2 ); input->setProperties( settings );
    auto raw = make_ptr<InputEvent>(); raw->setEventType( IInputEvent::EventType::Key );
    raw->setKeyboardState( key );
    BOOST_CHECK( input->postEvent( raw ) );
    BOOST_CHECK( !raw->getGameInputState() );
    BOOST_CHECK( !input->getProperties()->getPropertyAsBool( "input.mouseCapture" ) );
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
