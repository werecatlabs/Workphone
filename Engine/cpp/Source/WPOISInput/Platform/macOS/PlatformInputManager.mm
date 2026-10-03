#include <WPOISInput/Platform/macOS/PlatformInputManager.hpp>
#include <Workphone/WPCore.hpp>

#import <Foundation/Foundation.h>

#if TARGET_OS_OSX
#    import <Cocoa/Cocoa.h>
#else
#    import <UIKit/UIKit.h>
#endif

namespace workphone
{

    SmartPtr<render::IWindow> PlatformInputManager::getWindow() const
    {
        return m_window;
    }

    void PlatformInputManager::setWindow( SmartPtr<render::IWindow> window )
    {
        m_window = window;

        m_listener = workphone::make_ptr<WindowListener>();
        m_window->addListener( m_listener );
    }

    void PlatformInputManager::WindowListener::handleEvent( SmartPtr<render::IWindowEvent> pEvent )
    {
        auto applicationManager = core::ApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();

        auto inputManager = applicationManager->getInputDeviceManager();

        auto windowMessage = workphone::static_pointer_cast<WindowMessageData>( pEvent );
        auto event = (NSEvent *)windowMessage->getEvent();
        auto view = (NSView *)windowMessage->getSelf();

        auto contentScalingFactor =
            ( [view respondsToSelector:@selector( wantsBestResolutionOpenGLSurface )] &&
              [(id)view wantsBestResolutionOpenGLSurface] )
                ? ( view.window.screen ?: [NSScreen mainScreen] ).backingScaleFactor
                : 1.0f;

        if( event.type == NSEventTypeLeftMouseDown || event.type == NSEventTypeRightMouseDown ||
            event.type == NSEventTypeOtherMouseDown )
        {
            int button = (int)[event buttonNumber];

            SmartPtr<IInputEvent> inputEvent( new InputEvent );
            inputEvent->setEventType( IInputEvent::EventType::Mouse );

            SmartPtr<IMouseState> mouseState( new MouseState );
            inputEvent->setMouseState( mouseState );

            if( event.type == NSEventTypeLeftMouseDown )
            {
                mouseState->setEventType( IMouseState::Event::LeftPressed );
            }
            else if( event.type == NSEventTypeRightMouseDown )
            {
                mouseState->setEventType( IMouseState::Event::RightPressed );
            }

            inputManager->postEvent( inputEvent );
        }

        if( event.type == NSEventTypeLeftMouseUp || event.type == NSEventTypeRightMouseUp ||
            event.type == NSEventTypeOtherMouseUp )
        {
            int button = (int)[event buttonNumber];

            SmartPtr<IInputEvent> inputEvent( new InputEvent );
            inputEvent->setEventType( IInputEvent::EventType::Mouse );

            SmartPtr<IMouseState> mouseState( new MouseState );
            inputEvent->setMouseState( mouseState );

            if( event.type == NSEventTypeLeftMouseUp )
            {
                mouseState->setEventType( IMouseState::Event::LeftReleased );
            }
            else if( event.type == NSEventTypeRightMouseUp )
            {
                mouseState->setEventType( IMouseState::Event::RightReleased );
            }

            inputManager->postEvent( inputEvent );
        }

        if( event.type == NSEventTypeMouseMoved || event.type == NSEventTypeLeftMouseDragged ||
            event.type == NSEventTypeRightMouseDragged || event.type == NSEventTypeOtherMouseDragged )
        {
            NSPoint mousePoint = event.locationInWindow;
            mousePoint = [view convertPoint:mousePoint fromView:nil];
            mousePoint = NSMakePoint( mousePoint.x, view.bounds.size.height - mousePoint.y );

            auto vSize = NSMakePoint(view.bounds.size.width, view.bounds.size.height);
            //io.AddMousePosEvent((float)mousePoint.x * contentScalingFactor,  (float)mousePoint.y * contentScalingFactor);

            auto inputEvent = factoryManager->make_ptr<InputEvent>();
            inputEvent->setEventType( IInputEvent::EventType::Mouse );

            auto mouseState = factoryManager->make_ptr<MouseState>();
            inputEvent->setMouseState( mouseState );

            auto size = Vector2F(vSize.x, vSize.y);
            auto absolutePosition = Vector2F( (f32)mousePoint.x / contentScalingFactor,
                                              (f32)mousePoint.y / contentScalingFactor );

            if ( !MathUtil<f32>::equals( absolutePosition, m_lastMousePosition) )
            {
                //auto relativeMove = absolutePosition - m_lastMousePosition;
                auto relativeMove = m_lastMousePosition - absolutePosition;
                auto relativePosition = absolutePosition / size;

                mouseState->setDelta( relativeMove );
                mouseState->setRelativePosition( relativePosition );
                mouseState->setAbsolutePosition( absolutePosition );

                mouseState->setEventType( IMouseState::Event::Moved );

                m_lastMousePosition = absolutePosition;
                inputManager->postEvent( inputEvent );
            }
        }

        if (event.type == NSEventTypeScrollWheel) {
            CGFloat deltaX = [event scrollingDeltaX];
            CGFloat deltaY = [event scrollingDeltaY];

            auto inputEvent = factoryManager->make_ptr<InputEvent>();
            inputEvent->setEventType(IInputEvent::EventType::Mouse);

            auto mouseState = factoryManager->make_ptr<MouseState>();
            inputEvent->setMouseState(mouseState);

            mouseState->setWheelDelta(Vector2F(deltaY, deltaX));
            mouseState->setEventType(IMouseState::Event::Wheel);

            inputManager->postEvent(inputEvent);
        }

    }

}  // namespace fb
