#include "WPOISInput/Platform/win32/PlatformInputManager.hpp"

#include <windowsx.h>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    PlatformInputManager::PlatformInputManager() = default;
    PlatformInputManager::~PlatformInputManager() = default;

    SmartPtr<render::IGraphicsWindow> PlatformInputManager::getWindow() const
    {
        return m_window;
    }

    void PlatformInputManager::setWindow( SmartPtr<render::IGraphicsWindow> window )
    {
        m_window = window;

        m_listener = workphone::make_ptr<WindowListener>();
        m_window->addListener( m_listener );
    }

    void PlatformInputManager::handleRawInput( HRAWINPUT hRawInput )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto inputManager = applicationManager->getInputDeviceManager();

        UINT dataSize = 0;
        GetRawInputData( hRawInput, RID_INPUT, nullptr, &dataSize, sizeof( RAWINPUTHEADER ) );

        if( dataSize == 0 )
        {
            return;
        }

        std::vector<BYTE> buffer( dataSize );
        if( GetRawInputData( hRawInput, RID_INPUT, buffer.data(), &dataSize,
                             sizeof( RAWINPUTHEADER ) ) != dataSize )
        {
            // Error handling
            return;
        }

        auto rawInput = reinterpret_cast<RAWINPUT *>( buffer.data() );
        if( rawInput->header.dwType == RIM_TYPEMOUSE )
        {
            RAWMOUSE &rawMouse = rawInput->data.mouse;

            // Check if it's a mouse move event
            if( rawMouse.usFlags == MOUSE_MOVE_RELATIVE )
            {
                // Handle mouse move event
                int deltaX = rawMouse.lLastX;
                int deltaY = rawMouse.lLastY;

                auto inputEvent = workphone::make_ptr<InputEvent>();
                inputEvent->setEventType( IInputEvent::EventType::Mouse );

                auto mouseState = workphone::make_ptr<MouseState>();
                inputEvent->setMouseState( mouseState );

                mouseState->setDelta( Vector2F( deltaX, deltaY ) );
                mouseState->setEventType( IMouseState::Event::Moved );

                inputManager->postEvent( inputEvent );
            }
            // Check if it's a mouse button event
            else if( rawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN )
            {
                // Handle left mouse button down event
                auto inputEvent = workphone::make_ptr<InputEvent>();
                inputEvent->setEventType( IInputEvent::EventType::Mouse );

                auto mouseState = workphone::make_ptr<MouseState>();
                inputEvent->setMouseState( mouseState );

                mouseState->setEventType( IMouseState::Event::LeftPressed );

                inputManager->postEvent( inputEvent );
            }
            else if( rawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP )
            {
                // Handle left mouse button down event
                auto inputEvent = workphone::make_ptr<InputEvent>();
                inputEvent->setEventType( IInputEvent::EventType::Mouse );

                auto mouseState = workphone::make_ptr<MouseState>();
                inputEvent->setMouseState( mouseState );

                mouseState->setEventType( IMouseState::Event::LeftReleased );

                inputManager->postEvent( inputEvent );
            }
            else if( rawMouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN )
            {
                // Handle right mouse button down event
                auto inputEvent = workphone::make_ptr<InputEvent>();
                inputEvent->setEventType( IInputEvent::EventType::Mouse );

                auto mouseState = workphone::make_ptr<MouseState>();
                inputEvent->setMouseState( mouseState );

                mouseState->setEventType( IMouseState::Event::RightPressed );

                inputManager->postEvent( inputEvent );
            }
            else if( rawMouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP )
            {
                // Handle right mouse button down event
                auto inputEvent = workphone::make_ptr<InputEvent>();
                inputEvent->setEventType( IInputEvent::EventType::Mouse );

                auto mouseState = workphone::make_ptr<MouseState>();
                inputEvent->setMouseState( mouseState );

                mouseState->setEventType( IMouseState::Event::RightReleased );

                inputManager->postEvent( inputEvent );
            }
            else if( rawMouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN )
            {
                // Handle middle mouse button down event
                auto inputEvent = workphone::make_ptr<InputEvent>();
                inputEvent->setEventType( IInputEvent::EventType::Mouse );

                auto mouseState = workphone::make_ptr<MouseState>();
                inputEvent->setMouseState( mouseState );

                mouseState->setEventType( IMouseState::Event::MiddlePressed );

                inputManager->postEvent( inputEvent );
            }
            else if( rawMouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP )
            {
                // Handle middle mouse button down event
                auto inputEvent = workphone::make_ptr<InputEvent>();
                inputEvent->setEventType( IInputEvent::EventType::Mouse );

                auto mouseState = workphone::make_ptr<MouseState>();
                inputEvent->setMouseState( mouseState );

                mouseState->setEventType( IMouseState::Event::MiddleReleased );

                inputManager->postEvent( inputEvent );
            }
            // Check if it's a mouse wheel event
            else if( rawMouse.usButtonFlags & RI_MOUSE_WHEEL )
            {
                // Handle mouse wheel event
                int wheelDelta = static_cast<SHORT>( rawMouse.usButtonData );

                auto inputEvent = workphone::make_ptr<InputEvent>();
                inputEvent->setEventType( IInputEvent::EventType::Mouse );

                auto mouseState = workphone::make_ptr<MouseState>();
                inputEvent->setMouseState( mouseState );

                mouseState->setWheelDelta( Vector2F( 0, wheelDelta ) );
                mouseState->setEventType( IMouseState::Event::Wheel );

                inputManager->postEvent( inputEvent );
            }
        }
    }

    PlatformInputManager::WindowListener::WindowListener() = default;

    PlatformInputManager::WindowListener::~WindowListener() = default;

    void PlatformInputManager::WindowListener::handleEvent(
        SmartPtr<render::IGraphicsWindowEvent> event )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto inputManager = applicationManager->getInputDeviceManager();

        auto owner = getOwner();

        auto windowsEvent = workphone::dynamic_pointer_cast<WindowMessageData>( event );
        auto msg = windowsEvent->getMessage();
        auto lParam = windowsEvent->getLParam();

        switch( msg )
        {
        case WM_MOUSEMOVE:
        {
            int xPos = GET_X_LPARAM( lParam );
            int yPos = GET_Y_LPARAM( lParam );

            auto inputEvent = workphone::make_ptr<InputEvent>();
            inputEvent->setEventType( IInputEvent::EventType::Mouse );

            auto mouseState = workphone::make_ptr<MouseState>();
            inputEvent->setMouseState( mouseState );

            mouseState->setDelta( Vector2F( xPos, yPos ) );
            mouseState->setEventType( IMouseState::Event::Moved );

            //inputManager->postEvent( inputEvent );
            break;
        }
        case WM_INPUT:
        {
            auto hRawInput = reinterpret_cast<HRAWINPUT>( lParam );
            //owner->handleRawInput( hRawInput );
            break;
        }
        // Handle other messages as needed
        default:
        {
        }
        }
    }

    Parameter PlatformInputManager::WindowListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventValue == windowClosingHash )
        {
            return Parameter( true );
        }

        return Parameter();
    }

    SmartPtr<PlatformInputManager> PlatformInputManager::WindowListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void PlatformInputManager::WindowListener::setOwner( SmartPtr<PlatformInputManager> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone
