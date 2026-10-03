#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Input/InputDeviceManager.hpp>
#include <Workphone/Input/InputEvent.hpp>
#include <Workphone/Input/MouseState.hpp>
#include <Workphone/Scene/Components/Camera/SphericalCameraController.hpp>
#include <Workphone/System/Timer.hpp>
#include <Workphone/UI/UIWindow.hpp>
#include <boost/test/unit_test.hpp>

#if defined WP_PLATFORM_WIN32 && WP_GRAPHICS_SYSTEM_CLAW
#    include <WorkphoneCore/workphone_types.h>
#    include <WPGraphics/ClawWindow.hpp>
#    include <WPGraphics/ClawHammerSystem.hpp>
#    include <Windows.h>

using namespace workphone;

namespace
{
    class SceneInputTimer : public Timer
    {
    public:
        f64 getDeltaTime() const override { return 0.01; }
    };

    class SceneInputWindow : public ui::UIWindow
    {
    public:
        void invalidate() override {}
        Vector2F getPosition() const override { return Vector2F::zero(); }
        Vector2F getSize() const override { return { 1280.0f, 720.0f }; }
    };

    class SceneInputListener : public IEventListener
    {
    public:
        Parameter handleEvent( EventType, hash_type, const Array<Parameter> &,
                               SmartPtr<ISharedObject>, SmartPtr<ISharedObject>,
                               SmartPtr<IEvent> event ) override
        {
            auto inputEvent = workphone::dynamic_pointer_cast<IInputEvent>( event );
            if( inputEvent && inputEvent->getMouseState() )
            {
                events.push_back( inputEvent->getMouseState() );
                if( camera )
                {
                    camera->handleEvent( inputEvent );
                }
            }
            return {};
        }

        Array<SmartPtr<IMouseState>> events;
        SmartPtr<scene::SphericalCameraController> camera;
    };
}

BOOST_AUTO_TEST_CASE( scene_camera_native_mouse_messages_preserve_frame_motion )
{
    TestGuard guard;
    const auto oldWindow = guard.applicationManager->getWindow();
    const auto oldInput = guard.applicationManager->getInputDeviceManager();
    const auto oldTimer = guard.applicationManager->getTimer();
    const auto oldGraphics = guard.applicationManager->getGraphicsSystem();
    const auto oldTask = Thread::getCurrentTask();
    guard.addCleanup( [app = guard.applicationManager, oldWindow, oldInput, oldTimer, oldGraphics,
                      oldTask]() mutable {
        app->setWindow( oldWindow );
        app->setInputDeviceManager( oldInput );
        app->setTimer( oldTimer );
        app->setGraphicsSystem( oldGraphics );
        Thread::setCurrentTask( oldTask );
    } );

    guard.applicationManager->setGraphicsSystem( make_ptr<render::ClawHammerSystem>() );
    auto window = make_ptr<render::ClawWindow>();
    window->load( nullptr );
    guard.addCleanup( [window]() mutable { window->unload( nullptr ); } );
    HWND handle = nullptr;
    window->getWindowHandle( &handle );
    BOOST_REQUIRE( handle );
    window->setVisible( false );
    guard.applicationManager->setWindow( window );
    guard.applicationManager->setTimer( make_ptr<SceneInputTimer>() );

    auto input = make_ptr<InputDeviceManager>();
    input->setCreateJoysticks( false );
    input->setWindow( window );
    input->load( nullptr );
    guard.applicationManager->setInputDeviceManager( input );
    guard.addCleanup( [input]() mutable { input->unload( nullptr ); } );

    auto actor = guard.sceneManager->createActor();
    auto camera = make_ptr<scene::SphericalCameraController>();
    actor->addComponentInstance( camera );
    camera->load( nullptr );
    camera->setUiWindow( make_ptr<SceneInputWindow>() );
    camera->setSphericalCoords( Vector3<real_Num>( 10, 1, 1.5 ) );
    guard.addCleanup( [camera, actor]() mutable { camera->unload( nullptr ); actor->unload( nullptr ); } );
    auto listener = make_ptr<SceneInputListener>();
    listener->camera = camera;
    input->addListener( listener );
    Thread::setCurrentTask( TaskId::Application );

    const auto before = camera->getSphericalCoords();
    SendMessageW( handle, WM_MOUSEMOVE, 0, MAKELPARAM( 80, 90 ) );
    SendMessageW( handle, WM_MOUSEMOVE, 0, MAKELPARAM( 100, 100 ) );
    SendMessageW( handle, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM( 100, 100 ) );
    SendMessageW( handle, WM_MOUSEMOVE, MK_RBUTTON, MAKELPARAM( 110, 105 ) );
    SendMessageW( handle, WM_MOUSEMOVE, MK_RBUTTON, MAKELPARAM( 130, 115 ) );
    SendMessageW( handle, WM_MOUSEMOVE, MK_RBUTTON, MAKELPARAM( 130, 115 ) );
    input->update();
    BOOST_REQUIRE_EQUAL( listener->events.size(), 6u );
    BOOST_CHECK( listener->events[3]->getDelta() == Vector2<real_Num>( -10, -5 ) );
    BOOST_CHECK( listener->events[4]->getDelta() == Vector2<real_Num>( -20, -10 ) );
    BOOST_CHECK( listener->events[4]->isButtonPressed( IMouseState::MOUSE_RIGHT ) );
    const auto size = window->getSize();
    BOOST_CHECK_CLOSE( listener->events[4]->getRelativePosition().X(),
                       real_Num( 130 ) / size.X(), 0.001 );
    camera->update();
    const auto after = camera->getSphericalCoords();
    BOOST_CHECK_CLOSE( after.Y() - before.Y(), -30 * camera->getMoveSpeed() * 0.01, 0.001 );
    BOOST_CHECK_CLOSE( after.Z() - before.Z(), 15 * camera->getMoveSpeed() * 0.01, 0.001 );
    camera->update();
    BOOST_CHECK( camera->getSphericalCoords() == after );

    // A button press must establish the baseline even without a preceding move.
    SendMessageW( handle, WM_RBUTTONUP, 0, MAKELPARAM( 130, 115 ) );
    SendMessageW( handle, WM_KILLFOCUS, 0, 0 );
    SendMessageW( handle, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM( 200, 200 ) );
    SendMessageW( handle, WM_MOUSEMOVE, MK_RBUTTON, MAKELPARAM( 210, 205 ) );
    input->update();
    BOOST_CHECK( listener->events.back()->getDelta() == Vector2<real_Num>( -10, -5 ) );
    SendMessageW( handle, WM_RBUTTONUP, 0, MAKELPARAM( 210, 205 ) );
    input->update();
}

BOOST_AUTO_TEST_CASE( scene_camera_accumulates_wheel_events_before_update )
{
    TestGuard guard;
    const auto oldTimer = guard.applicationManager->getTimer();
    const auto oldInput = guard.applicationManager->getInputDeviceManager();
    const auto oldTask = Thread::getCurrentTask();
    guard.addCleanup( [app = guard.applicationManager, oldTimer, oldInput, oldTask]() mutable {
        app->setTimer( oldTimer );
        app->setInputDeviceManager( oldInput );
        Thread::setCurrentTask( oldTask );
    } );
    guard.applicationManager->setTimer( make_ptr<SceneInputTimer>() );
    guard.applicationManager->setInputDeviceManager( make_ptr<InputDeviceManager>() );
    auto camera = make_ptr<scene::SphericalCameraController>();
    camera->load( nullptr );
    camera->setUiWindow( make_ptr<SceneInputWindow>() );
    guard.addCleanup( [camera]() mutable { camera->unload( nullptr ); } );
    const auto before = camera->getSphericalCoords();
    for( int i = 0; i < 2; ++i )
    {
        auto event = make_ptr<InputEvent>();
        auto mouse = make_ptr<MouseState>();
        mouse->setEventType( IMouseState::Event::Wheel );
        mouse->setRelativePosition( Vector2<real_Num>( 0.1, 0.1 ) );
        mouse->setWheelDelta( Vector2<real_Num>( 0, 1 ) );
        event->setEventType( IInputEvent::EventType::Mouse );
        event->setMouseState( mouse );
        BOOST_CHECK( camera->handleEvent( event ) );
    }
    Thread::setCurrentTask( TaskId::Application );
    camera->update();
    BOOST_CHECK_CLOSE( camera->getSphericalCoords().X(),
                       before.X() - 2 * camera->getZoomSpeed() * 0.01, 0.001 );
}
#endif
