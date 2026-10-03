#include <Workphone/WorkphonePCH.hpp>
#include "Win32InputBackend.hpp"

#ifdef WP_PLATFORM_WIN32

#    include <Workphone/Input/InputDeviceManager.hpp>
#    include <Workphone/Input/InputEvent.hpp>
#    include <Workphone/Input/InputEvent.hpp>
#    include <Workphone/Input/MouseState.hpp>
#    include <Workphone/Input/KeyboardState.hpp>
#    include <Workphone/Input/Joystick.hpp>
#    include <Workphone/Input/JoystickState.hpp>
#    include <Workphone/Interface/Input/IInputEvent.hpp>
#    include <Workphone/Interface/Input/IJoystick.hpp>
#    include <Workphone/Interface/Input/IJoystickState.hpp>
#    include <Workphone/Interface/Input/IMouseState.hpp>
#    include <Workphone/Interface/Input/IKeyboardState.hpp>
#    include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#    include <Workphone/Interface/Graphics/IGraphicsWindowListener.hpp>
#    include <Workphone/Interface/Graphics/IGraphicsWindowEvent.hpp>
#    include <Workphone/System/WindowMessageData.hpp>
#    include <Workphone/WorkphoneEnums.hpp>

namespace workphone
{
    // -------------------------------------------------------------------------
    // Anonymous-namespace Win32 helpers (moved from InputDeviceManager.cpp)
    // -------------------------------------------------------------------------

    namespace
    {
        using XInputGetStateFn = DWORD( WINAPI * )( DWORD, XINPUT_STATE * );

        XInputGetStateFn getXInputGetState()
        {
            static XInputGetStateFn xInputGetState = []() -> XInputGetStateFn {
                const wchar_t *dllNames[] = { L"xinput1_4.dll", L"xinput9_1_0.dll", L"xinput1_3.dll" };
                for( auto dllName : dllNames )
                {
                    if( auto module = LoadLibraryW( dllName ) )
                    {
                        if( auto proc = GetProcAddress( module, "XInputGetState" ) )
                        {
                            return reinterpret_cast<XInputGetStateFn>( proc );
                        }
                    }
                }

                return nullptr;
            }();

            return xInputGetState;
        }

        bool isKeyDown( int virtualKey )
        {
            return ( GetKeyState( virtualKey ) & 0x8000 ) != 0;
        }

        u32 mapVirtualKeyCode( u32 virtualKey, size_t lParam )
        {
            const auto scanCode = static_cast<UINT>( ( lParam >> 16 ) & 0xFF );
            const auto extended = ( lParam & ( 1ull << 24 ) ) != 0;

            switch( virtualKey )
            {
            case VK_SHIFT:
                return MapVirtualKeyW( scanCode, MAPVK_VSC_TO_VK_EX );
            case VK_CONTROL:
                return extended ? VK_RCONTROL : VK_LCONTROL;
            case VK_MENU:
                return extended ? VK_RMENU : VK_MENU;
            default:
                return virtualKey;
            }
        }

        Vector2<real_Num> getMousePositionFromLParam( size_t lParam )
        {
            return Vector2<real_Num>( static_cast<real_Num>( GET_X_LPARAM( lParam ) ),
                                      static_cast<real_Num>( GET_Y_LPARAM( lParam ) ) );
        }

        void applyMouseModifiers( SmartPtr<MouseState> mouseState, size_t wParam )
        {
            if( !mouseState )
            {
                return;
            }

            mouseState->setButtonPressed( IMouseState::MOUSE_LEFT, ( wParam & MK_LBUTTON ) != 0 );
            mouseState->setButtonPressed( IMouseState::MOUSE_RIGHT, ( wParam & MK_RBUTTON ) != 0 );
            mouseState->setButtonPressed( IMouseState::MOUSE_MIDDLE, ( wParam & MK_MBUTTON ) != 0 );
            mouseState->setShiftPressed( ( wParam & MK_SHIFT ) != 0 );
            mouseState->setControlPressed( ( wParam & MK_CONTROL ) != 0 );
        }

        void applyKeyboardModifiers( SmartPtr<KeyboardState> keyboardState )
        {
            if( !keyboardState )
            {
                return;
            }

            keyboardState->setShiftPressed( isKeyDown( VK_SHIFT ) );
            keyboardState->setControlPressed( isKeyDown( VK_CONTROL ) );
        }

        f32 normalizeThumbAxis( SHORT value )
        {
            if( value < 0 )
            {
                return static_cast<f32>( value ) / 32768.0f;
            }

            return static_cast<f32>( value ) / 32767.0f;
        }

        f32 normalizeTriggerAxis( BYTE value )
        {
            return static_cast<f32>( value ) / 255.0f;
        }
    }  // namespace

    // -------------------------------------------------------------------------
    // Win32WindowListener — internal window listener for the Win32 backend
    // -------------------------------------------------------------------------

    /// Internal window listener that forwards graphics window events to the
    /// Win32InputBackend for platform-specific processing.
    class Win32WindowListener : public render::IGraphicsWindowListener
    {
    public:
        Win32WindowListener() = default;
        ~Win32WindowListener() override = default;

        Parameter handleEvent( EventType eventType, hash_type eventValue,
                               const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                               SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override
        {
            if( eventValue == render::IGraphicsWindowListener::windowClosingHash )
            {
                return Parameter( true );
            }

            return Parameter();
        }

        void handleEvent( SmartPtr<render::IGraphicsWindowEvent> event ) override
        {
            if( m_backend )
            {
                m_backend->processWindowEvent( *m_manager, event );
            }
        }

        void setBackend( Win32InputBackend *backend, InputDeviceManager *manager )
        {
            m_backend = backend;
            m_manager = manager;
        }

    private:
        Win32InputBackend *m_backend = nullptr;
        InputDeviceManager *m_manager = nullptr;
    };

    // -------------------------------------------------------------------------
    // Win32InputBackend
    // -------------------------------------------------------------------------

    Win32InputBackend::Win32InputBackend() = default;

    Win32InputBackend::~Win32InputBackend() = default;

    void Win32InputBackend::initialize( InputDeviceManager &manager )
    {
        if( manager.getCreateJoysticks() && getXInputGetState() )
        {
            while( manager.m_joysticks.size() < XUSER_MAX_COUNT )
            {
                auto joystick = workphone::make_ptr<Joystick>();
                manager.m_joysticks.push_back( joystick );
            }
        }
    }

    void Win32InputBackend::shutdown()
    {
        // Nothing platform-specific to release beyond what the manager does.
    }

    void Win32InputBackend::updateJoysticks( InputDeviceManager &manager )
    {
        if( !manager.getCreateJoysticks() )
        {
            return;
        }

        auto xInputGetState = getXInputGetState();
        if( !xInputGetState )
        {
            return;
        }

        while( manager.m_joysticks.size() < XUSER_MAX_COUNT )
        {
            manager.m_joysticks.push_back( workphone::make_ptr<Joystick>() );
        }

        static const WORD buttonMasks[] = { XINPUT_GAMEPAD_A,
                                            XINPUT_GAMEPAD_B,
                                            XINPUT_GAMEPAD_X,
                                            XINPUT_GAMEPAD_Y,
                                            XINPUT_GAMEPAD_LEFT_SHOULDER,
                                            XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                            XINPUT_GAMEPAD_BACK,
                                            XINPUT_GAMEPAD_START,
                                            XINPUT_GAMEPAD_LEFT_THUMB,
                                            XINPUT_GAMEPAD_RIGHT_THUMB,
                                            XINPUT_GAMEPAD_DPAD_UP,
                                            XINPUT_GAMEPAD_DPAD_DOWN,
                                            XINPUT_GAMEPAD_DPAD_LEFT,
                                            XINPUT_GAMEPAD_DPAD_RIGHT };

        for( u32 i = 0; i < static_cast<u32>( manager.m_joysticks.size() ) &&
                        i < static_cast<u32>( XUSER_MAX_COUNT );
             ++i )
        {
            auto joystick = workphone::dynamic_pointer_cast<Joystick>( manager.m_joysticks[i] );
            if( !joystick )
            {
                continue;
            }

            XINPUT_STATE state = {};
            const auto result = xInputGetState( i, &state );
            if( result != ERROR_SUCCESS )
            {
                for( u32 button = 0; button < static_cast<u32>( std::size( buttonMasks ) ); ++button )
                {
                    const auto wasDown = joystick->isButtonDown( static_cast<s32>( button ) );
                    joystick->updateButtonState( static_cast<s32>( button ), false );

                    if( wasDown )
                    {
                        auto event = workphone::make_ptr<InputEvent>();
                        event->setEventType( IInputEvent::EventType::Joystick );

                        auto joystickState = workphone::make_ptr<JoystickState>();
                        joystickState->setJoystick( i );
                        joystickState->setEventType(
                            static_cast<u32>( IJoystickState::Type::ButtonReleased ) );
                        joystickState->setButtonId( button );
                        joystickState->setPressedDown( false );
                        joystickState->setButtonPressed( button, false );
                        event->setJoystickState( joystickState );
                        manager.postEvent( event );
                    }
                }

                bool axisChanged = false;
                for( s32 axis = 0; axis < 6; ++axis )
                {
                    axisChanged = axisChanged || joystick->getAxis( axis ) != 0.0f;
                    joystick->updateAxisValue( axis, 0.0f );
                }

                if( axisChanged )
                {
                    auto event = workphone::make_ptr<InputEvent>();
                    event->setEventType( IInputEvent::EventType::Joystick );

                    auto joystickState = workphone::make_ptr<JoystickState>();
                    joystickState->setJoystick( i );
                    joystickState->setEventType( static_cast<u32>( IJoystickState::Type::AxisMoved ) );
                    for( s32 axis = 0; axis < 6; ++axis )
                    {
                        joystickState->setAxis( static_cast<u32>( axis ), 0.0f );
                    }
                    event->setJoystickState( joystickState );
                    manager.postEvent( event );
                }

                // Keep the slot object alive for hot reload, but report no
                // controls so callers can distinguish it from a connected pad.
                joystick->setNumButtons( 0 );
                joystick->setNumAxes( 0 );
                continue;
            }

            joystick->setNumButtons( 14 );
            joystick->setNumAxes( 6 );

            for( u32 button = 0; button < static_cast<u32>( std::size( buttonMasks ) ); ++button )
            {
                const auto wasDown = joystick->isButtonDown( static_cast<s32>( button ) );
                const auto isDown = ( state.Gamepad.wButtons & buttonMasks[button] ) != 0;
                joystick->updateButtonState( static_cast<s32>( button ), isDown );

                if( wasDown != isDown )
                {
                    auto event = workphone::make_ptr<InputEvent>();
                    event->setEventType( IInputEvent::EventType::Joystick );

                    auto joystickState = workphone::make_ptr<JoystickState>();
                    joystickState->setJoystick( i );
                    joystickState->setEventType(
                        static_cast<u32>( isDown ? IJoystickState::Type::ButtonPressed
                                                 : IJoystickState::Type::ButtonReleased ) );
                    joystickState->setButtonId( button );
                    joystickState->setPressedDown( isDown );
                    joystickState->setButtonPressed( button, isDown );
                    event->setJoystickState( joystickState );
                    manager.postEvent( event );
                }
            }

            const f32 axes[] = { normalizeThumbAxis( state.Gamepad.sThumbLX ),
                                 normalizeThumbAxis( state.Gamepad.sThumbLY ),
                                 normalizeTriggerAxis( state.Gamepad.bLeftTrigger ),
                                 normalizeThumbAxis( state.Gamepad.sThumbRX ),
                                 normalizeThumbAxis( state.Gamepad.sThumbRY ),
                                 normalizeTriggerAxis( state.Gamepad.bRightTrigger ) };

            bool axisChanged = false;
            for( u32 axis = 0; axis < static_cast<u32>( std::size( axes ) ); ++axis )
            {
                const auto previousValue = joystick->getAxis( static_cast<s32>( axis ) );
                joystick->updateAxisValue( static_cast<s32>( axis ), axes[axis] );
                axisChanged =
                    axisChanged || previousValue != joystick->getAxis( static_cast<s32>( axis ) );
            }

            if( axisChanged )
            {
                auto event = workphone::make_ptr<InputEvent>();
                event->setEventType( IInputEvent::EventType::Joystick );

                auto joystickState = workphone::make_ptr<JoystickState>();
                joystickState->setJoystick( i );
                joystickState->setEventType( static_cast<u32>( IJoystickState::Type::AxisMoved ) );
                for( u32 axis = 0; axis < static_cast<u32>( std::size( axes ) ); ++axis )
                {
                    joystickState->setAxis( axis, axes[axis] );
                }
                event->setJoystickState( joystickState );
                manager.postEvent( event );
            }
        }
    }

    bool Win32InputBackend::isKeyPressed( KeyCodes keyCode ) const
    {
        auto virtualKey = static_cast<int>( keyCode );
        if( keyCode == KeyCodes::KEY_LALT )
        {
            virtualKey = VK_MENU;
        }

        return ( GetAsyncKeyState( virtualKey ) & 0x8000 ) != 0;
    }

    bool Win32InputBackend::isMouseButtonDown( u32 button ) const
    {
        switch( button )
        {
        case 0:
            return ( GetAsyncKeyState( VK_LBUTTON ) & 0x8000 ) != 0;
        case 1:
            return ( GetAsyncKeyState( VK_RBUTTON ) & 0x8000 ) != 0;
        case 2:
            return ( GetAsyncKeyState( VK_MBUTTON ) & 0x8000 ) != 0;
        case 3:
            return ( GetAsyncKeyState( VK_XBUTTON1 ) & 0x8000 ) != 0;
        case 4:
            return ( GetAsyncKeyState( VK_XBUTTON2 ) & 0x8000 ) != 0;
        default:
            break;
        }
        return false;
    }

    void Win32InputBackend::setCursorVisible( bool visible )
    {
        ShowCursor( visible ? TRUE : FALSE );
    }

    SmartPtr<render::IGraphicsWindowListener> Win32InputBackend::createWindowListener(
        InputDeviceManager &manager )
    {
        auto listener = workphone::make_ptr<Win32WindowListener>();
        listener->setBackend( this, &manager );
        return listener;
    }

    void Win32InputBackend::processWindowEvent( InputDeviceManager &manager,
                                                SmartPtr<render::IGraphicsWindowEvent> event )
    {
        auto windowsEvent = workphone::dynamic_pointer_cast<WindowMessageData>( event );
        if( !windowsEvent )
        {
            return;
        }

        auto msg = windowsEvent->getMessage();
        auto wParam = windowsEvent->getWParam();
        auto lParam = windowsEvent->getLParam();

        auto getRelativeMousePosition = [&manager]( const Vector2<real_Num> &position ) {
            if( auto window = manager.getWindow() )
            {
                const auto size = window->getSize();
                if( size.X() > 0 && size.Y() > 0 )
                {
                    return position / Vector2<real_Num>( static_cast<real_Num>( size.X() ),
                                                         static_cast<real_Num>( size.Y() ) );
                }
            }

            return Vector2<real_Num>::zero();
        };

        auto getMousePositionFromScreen = [windowsEvent]( size_t messageLParam ) {
            POINT point = { GET_X_LPARAM( messageLParam ), GET_Y_LPARAM( messageLParam ) };
            auto windowHandle = reinterpret_cast<HWND>( windowsEvent->getWindowHandle() );
            if( windowHandle )
            {
                ScreenToClient( windowHandle, &point );
            }

            return Vector2<real_Num>( static_cast<real_Num>( point.x ),
                                      static_cast<real_Num>( point.y ) );
        };

        auto postMouseEvent = [&manager, &getRelativeMousePosition](
                                  IMouseState::Event eventType, const Vector2<real_Num> &position,
                                  size_t mouseFlags, u32 button, bool pressed, bool doubleClick ) {
            auto inputEvent = workphone::make_ptr<InputEvent>();
            inputEvent->setEventType( IInputEvent::EventType::Mouse );

            auto mouseState = workphone::make_ptr<MouseState>();
            inputEvent->setMouseState( mouseState );

            mouseState->setEventType( eventType );
            mouseState->setAbsolutePosition( position );
            mouseState->setRelativePosition( getRelativeMousePosition( position ) );
            mouseState->setDelta( manager.m_mousePosition - position );
            mouseState->setButtonPressed( button, pressed );
            mouseState->setDragging( manager.m_isDragging.load() );
            mouseState->setDragValue( position - manager.m_dragStartPosition );
            applyMouseModifiers( mouseState, mouseFlags );
            mouseState->setDoubleClick( doubleClick );

            manager.m_mousePosition = position;
            manager.m_currentInputEvent = inputEvent;
            manager.m_currentMouseState = mouseState;
            manager.postEvent( inputEvent );
        };

        switch( msg )
        {
        case WM_MOUSEMOVE:
        {
            if( !manager.m_createMouse )
            {
                break;
            }

            auto currentPos = getMousePositionFromLParam( lParam );
            auto delta = Vector2<real_Num>::zero();
            if( m_hasLastMousePosition )
            {
                // Preserve the established OIS convention used by existing consumers.
                delta = m_lastMousePosition - currentPos;
            }
            m_lastMousePosition = currentPos;
            m_hasLastMousePosition = true;
            manager.m_mousePosition = currentPos;

            auto inputEvent = workphone::make_ptr<InputEvent>();
            inputEvent->setEventType( IInputEvent::EventType::Mouse );

            auto mouseState = workphone::make_ptr<MouseState>();
            inputEvent->setMouseState( mouseState );

            mouseState->setEventType( IMouseState::Event::Moved );
            mouseState->setAbsolutePosition( currentPos );
            mouseState->setRelativePosition( getRelativeMousePosition( currentPos ) );
            mouseState->setDelta( delta );
            mouseState->setDragValue( currentPos - manager.m_dragStartPosition );
            applyMouseModifiers( mouseState, wParam );

            bool isDragging = manager.m_isDragging.load();
            mouseState->setDragging( isDragging );

            manager.m_currentInputEvent = inputEvent;
            manager.m_currentMouseState = mouseState;
            manager.postEvent( inputEvent );
            break;
        }
        case WM_LBUTTONDBLCLK:
        {
            if( !manager.m_createMouse )
            {
                break;
            }

            auto currentPos = getMousePositionFromLParam( lParam );
            manager.m_isDragging.store( true );
            manager.m_dragStartPosition = currentPos;
            if( auto windowHandle = reinterpret_cast<HWND>( windowsEvent->getWindowHandle() ) )
            {
                SetCapture( windowHandle );
            }
            postMouseEvent( IMouseState::Event::LeftPressed, currentPos, wParam, IMouseState::MOUSE_LEFT,
                            true, true );
            break;
        }
        case WM_LBUTTONDOWN:
        {
            if( !manager.m_createMouse )
            {
                break;
            }

            manager.m_isDragging.store( true );
            manager.m_dragStartPosition = getMousePositionFromLParam( lParam );
            if( auto windowHandle = reinterpret_cast<HWND>( windowsEvent->getWindowHandle() ) )
            {
                SetCapture( windowHandle );
            }
            postMouseEvent( IMouseState::Event::LeftPressed, manager.m_dragStartPosition, wParam,
                            IMouseState::MOUSE_LEFT, true, false );
            break;
        }
        case WM_LBUTTONUP:
        {
            if( !manager.m_createMouse )
            {
                break;
            }

            manager.m_isDragging.store( false );
            postMouseEvent( IMouseState::Event::LeftReleased, getMousePositionFromLParam( lParam ),
                            wParam, IMouseState::MOUSE_LEFT, false, false );
            if( auto windowHandle = reinterpret_cast<HWND>( windowsEvent->getWindowHandle() );
                windowHandle && GetCapture() == windowHandle )
            {
                ReleaseCapture();
            }
            break;
        }
        case WM_RBUTTONDOWN:
        {
            if( !manager.m_createMouse )
            {
                break;
            }
            postMouseEvent( IMouseState::Event::RightPressed, getMousePositionFromLParam( lParam ),
                            wParam, IMouseState::MOUSE_RIGHT, true, false );
            break;
        }
        case WM_RBUTTONUP:
        {
            if( !manager.m_createMouse )
            {
                break;
            }
            postMouseEvent( IMouseState::Event::RightReleased, getMousePositionFromLParam( lParam ),
                            wParam, IMouseState::MOUSE_RIGHT, false, false );
            break;
        }
        case WM_MBUTTONDOWN:
        {
            if( !manager.m_createMouse )
            {
                break;
            }
            postMouseEvent( IMouseState::Event::MiddlePressed, getMousePositionFromLParam( lParam ),
                            wParam, IMouseState::MOUSE_MIDDLE, true, false );
            break;
        }
        case WM_MBUTTONUP:
        {
            if( !manager.m_createMouse )
            {
                break;
            }
            postMouseEvent( IMouseState::Event::MiddleReleased, getMousePositionFromLParam( lParam ),
                            wParam, IMouseState::MOUSE_MIDDLE, false, false );
            break;
        }
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        {
            if( !manager.m_createMouse )
            {
                break;
            }

            auto wheelDelta = static_cast<real_Num>( GET_WHEEL_DELTA_WPARAM( wParam ) );
            const bool horizontal = msg == WM_MOUSEHWHEEL;
            const auto wheelPosition = getMousePositionFromScreen( lParam );

            manager.m_mouseScroll = horizontal ? Vector3<real_Num>( wheelDelta, 0, 0 )
                                               : Vector3<real_Num>( 0, wheelDelta, 0 );

            auto inputEvent = workphone::make_ptr<InputEvent>();
            inputEvent->setEventType( IInputEvent::EventType::Mouse );

            auto mouseState = workphone::make_ptr<MouseState>();
            inputEvent->setMouseState( mouseState );

            mouseState->setEventType( IMouseState::Event::Wheel );
            mouseState->setWheelDelta( horizontal ? Vector2<real_Num>( wheelDelta, 0 )
                                                  : Vector2<real_Num>( 0, wheelDelta ) );
            mouseState->setAbsolutePosition( wheelPosition );
            mouseState->setRelativePosition( getRelativeMousePosition( wheelPosition ) );
            mouseState->setDragging( manager.m_isDragging.load() );
            mouseState->setDragValue( wheelPosition - manager.m_dragStartPosition );
            applyMouseModifiers( mouseState, wParam );

            manager.m_mousePosition = wheelPosition;
            manager.m_currentInputEvent = inputEvent;
            manager.m_currentMouseState = mouseState;
            manager.postEvent( inputEvent );
            break;
        }
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            if( !manager.m_createKeyboard )
            {
                break;
            }

            auto vkCode = mapVirtualKeyCode( static_cast<u32>( wParam ), lParam );
            auto keyCode = static_cast<KeyCodes>( vkCode );

            manager.m_shiftPressed.store( isKeyDown( VK_SHIFT ) );
            manager.m_controlPressed.store( isKeyDown( VK_CONTROL ) );

            auto inputEvent = workphone::make_ptr<InputEvent>();
            inputEvent->setEventType( IInputEvent::EventType::Key );

            auto keyboardState = workphone::make_ptr<KeyboardState>();
            inputEvent->setKeyboardState( keyboardState );

            keyboardState->setPressedDown( true );
            keyboardState->setKeyCode( vkCode );
            keyboardState->setRawKeyCode( static_cast<u32>( wParam ) );
            applyKeyboardModifiers( keyboardState );

            BYTE keyboardStateArr[256] = {};
            GetKeyboardState( keyboardStateArr );
            WCHAR charBuf[4] = {};
            if( ToUnicode( static_cast<UINT>( wParam ), static_cast<u32>( ( lParam >> 16 ) & 0xFF ),
                           keyboardStateArr, charBuf, 4, 0 ) > 0 )
            {
                keyboardState->setChar( static_cast<u32>( charBuf[0] ) );
            }
            else
            {
                keyboardState->setChar( 0 );
            }

            manager.m_currentInputEvent = inputEvent;
            manager.m_currentKeyboardState = keyboardState;
            manager.postEvent( inputEvent );
            break;
        }
        case WM_SYSKEYUP:
        case WM_KEYUP:
        {
            if( !manager.m_createKeyboard )
            {
                break;
            }

            auto vkCode = mapVirtualKeyCode( static_cast<u32>( wParam ), lParam );
            manager.m_shiftPressed.store( isKeyDown( VK_SHIFT ) );
            manager.m_controlPressed.store( isKeyDown( VK_CONTROL ) );

            auto inputEvent = workphone::make_ptr<InputEvent>();
            inputEvent->setEventType( IInputEvent::EventType::Key );

            auto keyboardState = workphone::make_ptr<KeyboardState>();
            inputEvent->setKeyboardState( keyboardState );

            keyboardState->setPressedDown( false );
            keyboardState->setKeyCode( vkCode );
            keyboardState->setRawKeyCode( static_cast<u32>( wParam ) );
            applyKeyboardModifiers( keyboardState );
            keyboardState->setChar( 0 );

            manager.m_currentInputEvent = inputEvent;
            manager.m_currentKeyboardState = keyboardState;
            manager.postEvent( inputEvent );
            break;
        }
        case WM_CAPTURECHANGED:
        case WM_CANCELMODE:
        case WM_KILLFOCUS:
        {
            manager.m_isDragging.store( false );
            m_hasLastMousePosition = false;
            break;
        }
        default:
            break;
        }
    }

}  // namespace workphone

#else  // !WP_PLATFORM_WIN32 — stub for non-Win32 platforms

namespace workphone
{
    Win32InputBackend::Win32InputBackend() = default;
    Win32InputBackend::~Win32InputBackend() = default;

    void Win32InputBackend::initialize( InputDeviceManager & )
    {
    }
    void Win32InputBackend::shutdown()
    {
    }
    void Win32InputBackend::updateJoysticks( InputDeviceManager & )
    {
    }
    bool Win32InputBackend::isKeyPressed( KeyCodes ) const
    {
        return false;
    }
    bool Win32InputBackend::isMouseButtonDown( u32 ) const
    {
        return false;
    }
    void Win32InputBackend::setCursorVisible( bool )
    {
    }

    SmartPtr<render::IGraphicsWindowListener> Win32InputBackend::createWindowListener(
        InputDeviceManager & )
    {
        return nullptr;
    }

    void Win32InputBackend::processWindowEvent( InputDeviceManager &,
                                                SmartPtr<render::IGraphicsWindowEvent> )
    {
    }

}  // namespace workphone

#endif  // WP_PLATFORM_WIN32
