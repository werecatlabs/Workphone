#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Input/InputDeviceManager.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/Interface/Input/IJoystick.hpp>
#include <Workphone/Interface/Input/IMouseState.hpp>
#include <Workphone/Interface/Input/IKeyboardState.hpp>
#include <Workphone/Interface/Input/IGameInput.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindowListener.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Input/InputEvent.hpp>
#include <Workphone/Input/MouseState.hpp>
#include <Workphone/Input/KeyboardState.hpp>
#include <Workphone/Input/GameInput.hpp>
#include <Workphone/Input/Joystick.hpp>
#include <Workphone/Input/JoystickState.hpp>
#include <Workphone/System/WindowMessageData.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <Workphone/Core/LogManager.hpp>

#ifdef WP_PLATFORM_WIN32
#    include <Windows.h>
#    include <WinUser.h>
#    include <mmsystem.h>
#    include <windowsx.h>
#    include <Xinput.h>
#endif

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, InputDeviceManager, IInputDeviceManager );
    WP_CLASS_REGISTER_DERIVED( workphone, InputDeviceManager::WindowListener,
                               render::IGraphicsWindowListener );

    // -------------------------------------------------------------------------
    // InputDeviceManager
    // -------------------------------------------------------------------------

#ifdef WP_PLATFORM_WIN32
    namespace
    {
        using XInputGetStateFn = DWORD( WINAPI * )( DWORD, XINPUT_STATE * );
        using JoyGetNumDevsFn = UINT( WINAPI * )();
        using JoyGetPosExFn = MMRESULT( WINAPI * )( UINT, LPJOYINFOEX );
        using JoyGetDevCapsWFn = MMRESULT( WINAPI * )( UINT_PTR, LPJOYCAPSW, UINT );

        struct WinMmJoystickApi
        {
            JoyGetNumDevsFn getNumDevs = nullptr;
            JoyGetPosExFn getPosEx = nullptr;
            JoyGetDevCapsWFn getDevCaps = nullptr;

            explicit operator bool() const
            {
                return getNumDevs && getPosEx && getDevCaps;
            }
        };

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

        const WinMmJoystickApi &getWinMmJoystickApi()
        {
            static const WinMmJoystickApi api = []() {
                WinMmJoystickApi result;
                if( auto module = LoadLibraryW( L"winmm.dll" ) )
                {
                    result.getNumDevs =
                        reinterpret_cast<JoyGetNumDevsFn>( GetProcAddress( module, "joyGetNumDevs" ) );
                    result.getPosEx =
                        reinterpret_cast<JoyGetPosExFn>( GetProcAddress( module, "joyGetPosEx" ) );
                    result.getDevCaps =
                        reinterpret_cast<JoyGetDevCapsWFn>( GetProcAddress( module, "joyGetDevCapsW" ) );
                }
                return result;
            }();

            return api;
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

        f32 normalizeWinMmAxis( DWORD value, UINT minimum, UINT maximum )
        {
            if( maximum <= minimum )
            {
                return 0.0f;
            }

            const auto range = static_cast<f32>( maximum ) - static_cast<f32>( minimum );
            const auto normalized =
                ( ( static_cast<f32>( value ) - static_cast<f32>( minimum ) ) / range ) * 2.0f - 1.0f;
            return std::max( -1.0f, std::min( 1.0f, normalized ) );
        }

        u32 convertWinMmPov( DWORD pov )
        {
            if( pov == JOY_POVCENTERED )
            {
                return IJoystickState::POV_Centered;
            }

            switch( ( ( pov + 2250u ) % 36000u ) / 4500u )
            {
            case 0:
                return IJoystickState::POV_North;
            case 1:
                return IJoystickState::POV_NorthEast;
            case 2:
                return IJoystickState::POV_East;
            case 3:
                return IJoystickState::POV_SouthEast;
            case 4:
                return IJoystickState::POV_South;
            case 5:
                return IJoystickState::POV_SouthWest;
            case 6:
                return IJoystickState::POV_West;
            case 7:
                return IJoystickState::POV_NorthWest;
            default:
                return IJoystickState::POV_Centered;
            }
        }
    }  // namespace
#endif

    InputDeviceManager::InputDeviceManager() : m_debugEnabled( true )
    {
    }

    InputDeviceManager::~InputDeviceManager()
    {
        auto window = getWindow();
        if( window && m_windowListener )
        {
            window->removeListener( m_windowListener );
        }

        if( m_windowListener )
        {
            m_windowListener->setOwner( nullptr );
        }
    }

    void InputDeviceManager::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            m_currentInputEvent = workphone::make_ptr<InputEvent>();
            m_currentMouseState = workphone::make_ptr<MouseState>();
            m_currentKeyboardState = workphone::make_ptr<KeyboardState>();
            m_mouseScroll = Vector3<real_Num>::zero();
            m_mousePosition = Vector2<real_Num>::zero();
            m_dragStartPosition = Vector2<real_Num>::zero();

            if( m_inputEventQueue.size() != static_cast<u32>( TaskId::Count ) )
            {
                m_inputEventQueue.resize( static_cast<u32>( TaskId::Count ) );
            }

#ifdef WP_PLATFORM_WIN32
            if( auto window = getWindow() )
            {
                // load() may be called again by the hot-reload path.  Detach the old
                // listener before replacing it so each window message is handled once.
                if( m_windowListener )
                {
                    window->removeListener( m_windowListener );
                    m_windowListener = nullptr;
                }

                auto windowListener = workphone::make_ptr<WindowListener>();
                windowListener->setOwner( this );
                m_windowListener = windowListener;
                window->addListener( windowListener );
            }

            if( m_createJoysticks )
            {
                // Joystick instances contain code-owned RTTI and vtables that do
                // not survive a module reload.  Replace every slot with a fresh
                // object when update() discovers a connected native device.
                m_joysticks.clear();
                m_joystickDeviceIds.clear();
                m_joystickPovs.clear();
                m_joystickBackend = 0;
                m_nextJoystickDiscoveryTime = 0;
            }
#endif

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void InputDeviceManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            m_listeners.clear();
            m_gameInputs.clear();

            // Joystick instances contain code-owned RTTI and vtables.  They must not
            // survive a module reload: update() casts slots to the newly loaded
            // Joystick type and would otherwise skip (or call into) stale objects.
            // load()/update() recreate the slots while preserving the manager itself.
            m_joysticks.clear();
            m_joystickDeviceIds.clear();
            m_joystickPovs.clear();
            m_joystickBackend = 0;
            m_nextJoystickDiscoveryTime = 0;

            if( auto window = getWindow() )
            {
                if( m_windowListener )
                {
                    window->removeListener( m_windowListener );
                }
            }

            // Keep m_window intact so a subsequent load()/reload() can reattach
            // the listener without requiring setWindow() to be called again.
            if( m_windowListener )
            {
                m_windowListener->setOwner( nullptr );
            }
            m_windowListener = nullptr;

            m_currentInputEvent = nullptr;
            m_currentMouseState = nullptr;
            m_currentKeyboardState = nullptr;

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void InputDeviceManager::reload( SmartPtr<ISharedObject> data )
    {
        // load() is deliberately idempotent: it replaces only the platform/window
        // binding while preserving externally held device and listener objects.
        load( data );
    }

    void InputDeviceManager::update()
    {
        auto inputTask = getTask();
        auto task = Thread::getCurrentTask();

#ifdef WP_PLATFORM_WIN32
        if( task == inputTask && m_createJoysticks )
        {
            constexpr u32 noJoystickBackend = 0;
            constexpr u32 xInputJoystickBackend = 1;
            constexpr u32 winMmJoystickBackend = 2;
            constexpr u64 joystickDiscoveryIntervalMs = 1000;

            auto updateJoystick = [this]( u32 joystickIndex, const std::array<bool, 32> &buttonStates,
                                          u32 numButtons, const std::array<f32, 6> &axes, u32 numAxes,
                                          u32 pov, const char *name ) {
                while( m_joysticks.size() <= joystickIndex )
                {
                    m_joysticks.push_back( workphone::make_ptr<Joystick>() );
                }

                while( m_joystickPovs.size() <= joystickIndex )
                {
                    m_joystickPovs.push_back( IJoystickState::POV_Centered );
                }

                auto joystick = workphone::dynamic_pointer_cast<Joystick>( m_joysticks[joystickIndex] );
                if( !joystick )
                {
                    m_joysticks[joystickIndex] = workphone::make_ptr<Joystick>();
                    joystick = workphone::dynamic_pointer_cast<Joystick>( m_joysticks[joystickIndex] );
                }

                if( !joystick )
                {
                    return;
                }

                if( name && *name )
                {
                    joystick->setName( name );
                }

                std::array<bool, 32> previousButtonStates = {};
                for( u32 button = 0; button < previousButtonStates.size(); ++button )
                {
                    previousButtonStates[button] = joystick->isButtonDown( static_cast<s32>( button ) );
                    const auto isDown = button < numButtons ? buttonStates[button] : false;
                    joystick->updateButtonState( static_cast<s32>( button ), isDown );
                }

                std::array<f32, 6> previousAxes = {};
                bool axisChanged = false;
                for( u32 axis = 0; axis < previousAxes.size(); ++axis )
                {
                    previousAxes[axis] = joystick->getAxis( static_cast<s32>( axis ) );
                    joystick->updateAxisValue( static_cast<s32>( axis ),
                                               axis < numAxes ? axes[axis] : 0.0f );
                    axisChanged = axisChanged ||
                                  previousAxes[axis] != joystick->getAxis( static_cast<s32>( axis ) );
                }

                const auto previousPov = m_joystickPovs[joystickIndex];
                m_joystickPovs[joystickIndex] = pov;
                joystick->setNumButtons( static_cast<s32>( numButtons ) );
                joystick->setNumAxes( static_cast<s32>( numAxes ) );

                auto createEvent = [this, joystickIndex, &axes, numAxes,
                                    pov]( IJoystickState::Type eventType ) {
                    auto event = workphone::make_ptr<InputEvent>();
                    event->setEventType( IInputEvent::EventType::Joystick );

                    auto joystickState = workphone::make_ptr<JoystickState>();
                    joystickState->setJoystick( joystickIndex );
                    joystickState->setEventType( static_cast<u32>( eventType ) );
                    joystickState->setPOV( pov );
                    for( u32 axis = 0; axis < axes.size(); ++axis )
                    {
                        joystickState->setAxis( axis, axis < numAxes ? axes[axis] : 0.0f );
                    }
                    event->setJoystickState( joystickState );

                    for( const auto &gameInputEntry : m_gameInputs )
                    {
                        if( gameInputEntry.second &&
                            gameInputEntry.second->getJoystickId() == joystickIndex )
                        {
                            event->setGameInputId( gameInputEntry.first );
                            break;
                        }
                    }

                    return std::make_pair( event, joystickState );
                };

                for( u32 button = 0; button < previousButtonStates.size(); ++button )
                {
                    const auto isDown = button < numButtons ? buttonStates[button] : false;
                    if( previousButtonStates[button] == isDown )
                    {
                        continue;
                    }

                    const auto eventType = isDown ? IJoystickState::Type::ButtonPressed
                                                  : IJoystickState::Type::ButtonReleased;
                    auto [event, joystickState] = createEvent( eventType );
                    joystickState->setButtonId( button );
                    joystickState->setPressedDown( isDown );
                    joystickState->setButtonPressed( button, isDown );
                    postEvent( event );
                }

                if( axisChanged )
                {
                    auto [event, joystickState] = createEvent( IJoystickState::Type::AxisMoved );
                    postEvent( event );
                }

                if( previousPov != pov )
                {
                    auto [event, joystickState] = createEvent( IJoystickState::Type::PovMoved );
                    postEvent( event );
                }
            };

            auto discoverXInputDevices = []() {
                Array<u32> deviceIds;
                if( auto xInputGetState = getXInputGetState() )
                {
                    for( u32 userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex )
                    {
                        XINPUT_STATE state = {};
                        if( xInputGetState( userIndex, &state ) == ERROR_SUCCESS )
                        {
                            deviceIds.push_back( userIndex );
                        }
                    }
                }
                return deviceIds;
            };

            auto discoverWinMmDevices = []() {
                Array<u32> deviceIds;
                const auto &winMm = getWinMmJoystickApi();
                if( winMm )
                {
                    const auto numDevices = winMm.getNumDevs();
                    for( u32 deviceId = 0; deviceId < numDevices; ++deviceId )
                    {
                        JOYINFOEX state = {};
                        state.dwSize = sizeof( state );
                        state.dwFlags = JOY_RETURNALL;
                        if( winMm.getPosEx( deviceId, &state ) == JOYERR_NOERROR )
                        {
                            deviceIds.push_back( deviceId );
                        }
                    }
                }
                return deviceIds;
            };

            const auto now = GetTickCount64();
            if( now >= m_nextJoystickDiscoveryTime )
            {
                auto deviceIds = discoverXInputDevices();
                if( !deviceIds.empty() )
                {
                    m_joystickBackend = xInputJoystickBackend;
                    m_joystickDeviceIds = deviceIds;
                }
                else
                {
                    deviceIds = discoverWinMmDevices();
                    m_joystickBackend = deviceIds.empty() ? noJoystickBackend : winMmJoystickBackend;
                    m_joystickDeviceIds = deviceIds;
                }

                m_nextJoystickDiscoveryTime = now + joystickDiscoveryIntervalMs;
            }

            u32 connectedJoystickCount = 0;
            if( m_joystickBackend == xInputJoystickBackend )
            {
                if( auto xInputGetState = getXInputGetState() )
                {
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

                    for( auto deviceId : m_joystickDeviceIds )
                    {
                        XINPUT_STATE state = {};
                        if( xInputGetState( deviceId, &state ) != ERROR_SUCCESS )
                        {
                            continue;
                        }

                        std::array<bool, 32> buttons = {};
                        for( u32 button = 0; button < std::size( buttonMasks ); ++button )
                        {
                            buttons[button] = ( state.Gamepad.wButtons & buttonMasks[button] ) != 0;
                        }

                        const std::array<f32, 6> axes = {
                            normalizeThumbAxis( state.Gamepad.sThumbLX ),
                            normalizeThumbAxis( state.Gamepad.sThumbLY ),
                            normalizeTriggerAxis( state.Gamepad.bLeftTrigger ),
                            normalizeThumbAxis( state.Gamepad.sThumbRX ),
                            normalizeThumbAxis( state.Gamepad.sThumbRY ),
                            normalizeTriggerAxis( state.Gamepad.bRightTrigger )
                        };

                        u32 pov = IJoystickState::POV_Centered;
                        if( buttons[10] )
                            pov |= IJoystickState::POV_North;
                        if( buttons[11] )
                            pov |= IJoystickState::POV_South;
                        if( buttons[12] )
                            pov |= IJoystickState::POV_West;
                        if( buttons[13] )
                            pov |= IJoystickState::POV_East;

                        updateJoystick( connectedJoystickCount, buttons,
                                        static_cast<u32>( std::size( buttonMasks ) ), axes, 6, pov,
                                        "XInput Controller" );
                        ++connectedJoystickCount;
                    }
                }
            }

            if( m_joystickBackend == winMmJoystickBackend )
            {
                const auto &winMm = getWinMmJoystickApi();
                for( auto deviceId : m_joystickDeviceIds )
                {
                    JOYINFOEX state = {};
                    state.dwSize = sizeof( state );
                    state.dwFlags = JOY_RETURNALL;
                    if( !winMm || winMm.getPosEx( deviceId, &state ) != JOYERR_NOERROR )
                    {
                        continue;
                    }

                    JOYCAPSW caps = {};
                    const auto hasCaps =
                        winMm.getDevCaps( deviceId, &caps, sizeof( caps ) ) == JOYERR_NOERROR;

                    std::array<bool, 32> buttons = {};
                    const auto numButtons = std::min<u32>( hasCaps ? caps.wNumButtons : 32, 32 );
                    for( u32 button = 0; button < numButtons; ++button )
                    {
                        buttons[button] = ( state.dwButtons & ( 1u << button ) ) != 0;
                    }

                    std::array<f32, 6> axes = {};
                    u32 numAxes = 6;
                    if( hasCaps )
                    {
                        axes[0] = normalizeWinMmAxis( state.dwXpos, caps.wXmin, caps.wXmax );
                        axes[1] = normalizeWinMmAxis( state.dwYpos, caps.wYmin, caps.wYmax );
                        numAxes = 2;
                        if( caps.wCaps & JOYCAPS_HASZ )
                        {
                            axes[2] = normalizeWinMmAxis( state.dwZpos, caps.wZmin, caps.wZmax );
                            numAxes = 3;
                        }
                        if( caps.wCaps & JOYCAPS_HASR )
                        {
                            axes[3] = normalizeWinMmAxis( state.dwRpos, caps.wRmin, caps.wRmax );
                            numAxes = 4;
                        }
                        if( caps.wCaps & JOYCAPS_HASU )
                        {
                            axes[4] = normalizeWinMmAxis( state.dwUpos, caps.wUmin, caps.wUmax );
                            numAxes = 5;
                        }
                        if( caps.wCaps & JOYCAPS_HASV )
                        {
                            axes[5] = normalizeWinMmAxis( state.dwVpos, caps.wVmin, caps.wVmax );
                            numAxes = 6;
                        }
                    }
                    else
                    {
                        axes[0] = normalizeWinMmAxis( state.dwXpos, 0, 65535 );
                        axes[1] = normalizeWinMmAxis( state.dwYpos, 0, 65535 );
                        axes[2] = normalizeWinMmAxis( state.dwZpos, 0, 65535 );
                        axes[3] = normalizeWinMmAxis( state.dwRpos, 0, 65535 );
                        axes[4] = normalizeWinMmAxis( state.dwUpos, 0, 65535 );
                        axes[5] = normalizeWinMmAxis( state.dwVpos, 0, 65535 );
                    }

                    updateJoystick( connectedJoystickCount, buttons, numButtons, axes, numAxes,
                                    convertWinMmPov( state.dwPOV ), "Windows Joystick" );
                    ++connectedJoystickCount;
                }
            }

            std::array<bool, 32> releasedButtons = {};
            std::array<f32, 6> centeredAxes = {};
            for( u32 joystickIndex = connectedJoystickCount;
                 joystickIndex < static_cast<u32>( m_joysticks.size() ); ++joystickIndex )
            {
                updateJoystick( joystickIndex, releasedButtons, 0, centeredAxes, 0,
                                IJoystickState::POV_Centered, nullptr );
            }

            if( connectedJoystickCount == 0 )
            {
                m_joystickBackend = noJoystickBackend;
                m_joystickDeviceIds.clear();
            }
        }
#endif

        auto taskIdx = static_cast<u32>( task );

        if( taskIdx < static_cast<u32>( m_inputEventQueue.size() ) )
        {
            auto &taskQueue = m_inputEventQueue[taskIdx];
            SmartPtr<IInputEvent> event;
            while( taskQueue.try_pop( event ) )
            {
                if( event )
                {
                    for( auto &listener : m_listeners )
                    {
                        if( listener )
                        {
                            listener->handleEvent( EventType::Input, IEvent::inputEvent,
                                                   Array<Parameter>(), this, this, event );
                        }
                    }
                }
            }
        }
    }

    void InputDeviceManager::triggerEvent( SmartPtr<IInputEvent> inputEvent )
    {
        for( auto &listener : m_listeners )
        {
            if( listener )
            {
                listener->handleEvent( EventType::Input, IEvent::inputEvent, Array<Parameter>(), this,
                                       this, inputEvent );
            }
        }
    }

    void InputDeviceManager::queueEvent( SmartPtr<IInputEvent> event )
    {
        postEvent( event );
    }

    bool InputDeviceManager::postEvent( SmartPtr<IInputEvent> event )
    {
        if( m_inputEventQueue.empty() )
        {
            return false;
        }

        for( auto &queue : m_inputEventQueue )
        {
            queue.push( event );
        }

        return true;
    }

    SmartPtr<IInputEvent> InputDeviceManager::createInputEvent()
    {
        return workphone::make_ptr<InputEvent>();
    }

    SmartPtr<IMouseState> InputDeviceManager::createMouseState()
    {
        return workphone::make_ptr<MouseState>();
    }

    SmartPtr<IKeyboardState> InputDeviceManager::createKeyboardState()
    {
        return workphone::make_ptr<KeyboardState>();
    }

    SmartPtr<IInputEvent> InputDeviceManager::getCurrentInputEvent() const
    {
        return m_currentInputEvent;
    }

    SmartPtr<IMouseState> InputDeviceManager::getCurrentMouseState() const
    {
        return m_currentMouseState;
    }

    SmartPtr<IKeyboardState> InputDeviceManager::getCurrentKeyboardState() const
    {
        return m_currentKeyboardState;
    }

    SmartPtr<IGameInput> InputDeviceManager::addGameInput( hash_type id )
    {
        try
        {
            auto gameInput = workphone::make_ptr<GameInput>();
            gameInput->setId( id );
            m_gameInputs[id] = gameInput;
            return gameInput;
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    SmartPtr<IGameInput> InputDeviceManager::findGameInput( hash_type id ) const
    {
        auto it = m_gameInputs.find( id );
        if( it != m_gameInputs.end() )
        {
            return it->second;
        }

        return nullptr;
    }

    Array<SmartPtr<IGameInput>> InputDeviceManager::getGameInputs() const
    {
        Array<SmartPtr<IGameInput>> result;
        result.reserve( static_cast<u32>( m_gameInputs.size() ) );
        for( auto &pair : m_gameInputs )
        {
            result.push_back( pair.second );
        }
        return result;
    }

    bool InputDeviceManager::isCursorVisible() const
    {
        return m_cursorVisible.load();
    }

    void InputDeviceManager::setCursorVisible( bool visible )
    {
        const auto previous = m_cursorVisible.load();
        m_cursorVisible.store( visible );
#ifdef WP_PLATFORM_WIN32
        if( previous != visible )
        {
            ShowCursor( visible ? TRUE : FALSE );
        }
#endif
    }

    void InputDeviceManager::addListener( SmartPtr<IEventListener> listener )
    {
        m_listeners.push_back( listener );
    }

    void InputDeviceManager::removeListener( SmartPtr<IEventListener> listener )
    {
        auto it = std::find( m_listeners.begin(), m_listeners.end(), listener );
        if( it != m_listeners.end() )
        {
            m_listeners.erase( it );
        }
    }

    void InputDeviceManager::removeListeners()
    {
        m_listeners.clear();
    }

    Vector3<real_Num> InputDeviceManager::getMouseScroll() const
    {
        return m_mouseScroll;
    }

    bool InputDeviceManager::isShiftPressed() const
    {
        return m_shiftPressed.load();
    }

    void InputDeviceManager::setShiftPressed( bool shiftPressed )
    {
        m_shiftPressed.store( shiftPressed );
    }

    bool InputDeviceManager::isMouseButtonDown( u32 button ) const
    {
#ifdef WP_PLATFORM_WIN32
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
#endif
        return false;
    }

    f64 InputDeviceManager::getLastClickTime() const
    {
        return m_lastClickTime;
    }

    void InputDeviceManager::setLastClickTime( f64 lastClickTime )
    {
        m_lastClickTime = lastClickTime;
    }

    f64 InputDeviceManager::getDoubleClickInterval() const
    {
        return m_doubleClickInterval;
    }

    void InputDeviceManager::setDoubleClickInterval( f64 doubleClickInterval )
    {
        m_doubleClickInterval = doubleClickInterval;
    }

    f64 InputDeviceManager::getLastInputTime() const
    {
        return m_lastInputTime;
    }

    void InputDeviceManager::setLastInputTime( f64 lastInputTime )
    {
        m_lastInputTime = lastInputTime;
    }

    f64 InputDeviceManager::getInputTime() const
    {
        return m_inputTime;
    }

    void InputDeviceManager::setInputTime( f64 inputTime )
    {
        m_inputTime = inputTime;
    }

    void InputDeviceManager::addInputTime( f64 inputTime )
    {
        m_inputTime += inputTime;
    }

    bool InputDeviceManager::isKeyPressed( KeyCodes keyCode ) const
    {
#ifdef WP_PLATFORM_WIN32
        auto virtualKey = static_cast<int>( keyCode );
        if( keyCode == KeyCodes::KEY_LALT )
        {
            virtualKey = VK_MENU;
        }

        return ( GetAsyncKeyState( virtualKey ) & 0x8000 ) != 0;
#else
        return false;
#endif
    }

    bool InputDeviceManager::getCreateMouse() const
    {
        return m_createMouse;
    }

    void InputDeviceManager::setCreateMouse( bool createMouse )
    {
        m_createMouse = createMouse;
    }

    bool InputDeviceManager::getCreateKeyboard() const
    {
        return m_createKeyboard;
    }

    void InputDeviceManager::setCreateKeyboard( bool createKeyboard )
    {
        m_createKeyboard = createKeyboard;
    }

    bool InputDeviceManager::getCreateJoysticks() const
    {
        return m_createJoysticks;
    }

    void InputDeviceManager::setCreateJoysticks( bool createJoysticks )
    {
        m_createJoysticks = createJoysticks;
    }

    SmartPtr<render::IGraphicsWindow> InputDeviceManager::getWindow() const
    {
        auto p = m_window.load();
        return p.lock();
    }

    void InputDeviceManager::setWindow( SmartPtr<render::IGraphicsWindow> window )
    {
        auto oldWindow = getWindow();
        if( oldWindow && m_windowListener )
        {
            oldWindow->removeListener( m_windowListener );
        }

        m_window = window;

        if( window )
        {
            if( !m_windowListener )
            {
                auto windowListener = workphone::make_ptr<WindowListener>();
                windowListener->setOwner( this );
                m_windowListener = windowListener;
            }
            window->addListener( m_windowListener );
        }
    }

    Array<SmartPtr<IJoystick>> InputDeviceManager::getJoysticks() const
    {
        Array<SmartPtr<IJoystick>> connectedJoysticks;
        for( const auto &joystick : m_joysticks )
        {
            if( joystick && ( joystick->getNumButtons() > 0 || joystick->getNumAxes() > 0 ) )
            {
                connectedJoysticks.push_back( joystick );
            }
        }
        return connectedJoysticks;
    }

    void InputDeviceManager::setJoysticks( const Array<SmartPtr<IJoystick>> &joysticks )
    {
        m_joysticks = joysticks;
    }

    TaskId InputDeviceManager::getTask() const
    {
        return m_task;
    }

    void InputDeviceManager::setTask( TaskId task )
    {
        m_task = task;
    }

    TaskId InputDeviceManager::getEventTask() const
    {
        return m_eventTask;
    }

    void InputDeviceManager::setEventTask( TaskId eventTask )
    {
        m_eventTask = eventTask;
    }

    void InputDeviceManager::_getObject( void **ppObject )
    {
        *ppObject = this;
    }

    // -------------------------------------------------------------------------
    // WindowListener
    // -------------------------------------------------------------------------

    InputDeviceManager::WindowListener::WindowListener() = default;

    InputDeviceManager::WindowListener::~WindowListener() = default;

    void InputDeviceManager::WindowListener::handleEvent( SmartPtr<render::IGraphicsWindowEvent> event )
    {
#ifdef WP_PLATFORM_WIN32
        auto owner = getOwner();
        if( !owner )
        {
            return;
        }

        auto windowsEvent = workphone::dynamic_pointer_cast<WindowMessageData>( event );
        if( !windowsEvent )
        {
            return;
        }

        auto msg = windowsEvent->getMessage();
        auto wParam = windowsEvent->getWParam();
        auto lParam = windowsEvent->getLParam();

        auto getRelativeMousePosition = [owner]( const Vector2<real_Num> &position ) {
            if( auto window = owner->getWindow() )
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

        auto postMouseEvent = [owner, &getRelativeMousePosition](
                                  IMouseState::Event eventType, const Vector2<real_Num> &position,
                                  size_t mouseFlags, u32 button, bool pressed, bool doubleClick ) {
            auto inputEvent = workphone::make_ptr<InputEvent>();
            inputEvent->setEventType( IInputEvent::EventType::Mouse );

            auto mouseState = workphone::make_ptr<MouseState>();
            inputEvent->setMouseState( mouseState );

            mouseState->setEventType( eventType );
            mouseState->setAbsolutePosition( position );
            mouseState->setRelativePosition( getRelativeMousePosition( position ) );
            mouseState->setDelta( owner->m_mousePosition - position );
            mouseState->setButtonPressed( button, pressed );
            mouseState->setDragging( owner->m_isDragging.load() );
            mouseState->setDragValue( position - owner->m_dragStartPosition );
            applyMouseModifiers( mouseState, mouseFlags );
            mouseState->setDoubleClick( doubleClick );

            owner->m_mousePosition = position;
            owner->m_currentInputEvent = inputEvent;
            owner->m_currentMouseState = mouseState;
            owner->postEvent( inputEvent );
        };

        switch( msg )
        {
        case WM_MOUSEMOVE:
        {
            if( !owner->m_createMouse )
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
            owner->m_mousePosition = currentPos;

            auto inputEvent = workphone::make_ptr<InputEvent>();
            inputEvent->setEventType( IInputEvent::EventType::Mouse );

            auto mouseState = workphone::make_ptr<MouseState>();
            inputEvent->setMouseState( mouseState );

            mouseState->setEventType( IMouseState::Event::Moved );
            mouseState->setAbsolutePosition( currentPos );
            mouseState->setRelativePosition( getRelativeMousePosition( currentPos ) );
            mouseState->setDelta( delta );
            mouseState->setDragValue( currentPos - owner->m_dragStartPosition );
            applyMouseModifiers( mouseState, wParam );

            bool isDragging = owner->m_isDragging.load();
            mouseState->setDragging( isDragging );

            owner->m_currentInputEvent = inputEvent;
            owner->m_currentMouseState = mouseState;
            owner->postEvent( inputEvent );
            break;
        }
        case WM_LBUTTONDBLCLK:
        {
            if( !owner->m_createMouse )
            {
                break;
            }

            auto currentPos = getMousePositionFromLParam( lParam );
            owner->m_isDragging.store( true );
            owner->m_dragStartPosition = currentPos;
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
            if( !owner->m_createMouse )
            {
                break;
            }

            owner->m_isDragging.store( true );
            owner->m_dragStartPosition = getMousePositionFromLParam( lParam );
            if( auto windowHandle = reinterpret_cast<HWND>( windowsEvent->getWindowHandle() ) )
            {
                SetCapture( windowHandle );
            }
            postMouseEvent( IMouseState::Event::LeftPressed, owner->m_dragStartPosition, wParam,
                            IMouseState::MOUSE_LEFT, true, false );

            break;
        }
        case WM_LBUTTONUP:
        {
            if( !owner->m_createMouse )
            {
                break;
            }

            owner->m_isDragging.store( false );
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
            if( !owner->m_createMouse )
            {
                break;
            }
            postMouseEvent( IMouseState::Event::RightPressed, getMousePositionFromLParam( lParam ),
                            wParam, IMouseState::MOUSE_RIGHT, true, false );
            break;
        }
        case WM_RBUTTONUP:
        {
            if( !owner->m_createMouse )
            {
                break;
            }
            postMouseEvent( IMouseState::Event::RightReleased, getMousePositionFromLParam( lParam ),
                            wParam, IMouseState::MOUSE_RIGHT, false, false );
            break;
        }
        case WM_MBUTTONDOWN:
        {
            if( !owner->m_createMouse )
            {
                break;
            }
            postMouseEvent( IMouseState::Event::MiddlePressed, getMousePositionFromLParam( lParam ),
                            wParam, IMouseState::MOUSE_MIDDLE, true, false );
            break;
        }
        case WM_MBUTTONUP:
        {
            if( !owner->m_createMouse )
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
            if( !owner->m_createMouse )
            {
                break;
            }

            auto wheelDelta = static_cast<real_Num>( GET_WHEEL_DELTA_WPARAM( wParam ) );
            const bool horizontal = msg == WM_MOUSEHWHEEL;
            const auto wheelPosition = getMousePositionFromScreen( lParam );

            owner->m_mouseScroll = horizontal ? Vector3<real_Num>( wheelDelta, 0, 0 )
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
            mouseState->setDragging( owner->m_isDragging.load() );
            mouseState->setDragValue( wheelPosition - owner->m_dragStartPosition );
            applyMouseModifiers( mouseState, wParam );

            owner->m_mousePosition = wheelPosition;
            owner->m_currentInputEvent = inputEvent;
            owner->m_currentMouseState = mouseState;
            owner->postEvent( inputEvent );
            break;
        }
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            if( !owner->m_createKeyboard )
            {
                break;
            }

            auto vkCode = mapVirtualKeyCode( static_cast<u32>( wParam ), lParam );
            auto keyCode = static_cast<KeyCodes>( vkCode );

            owner->m_shiftPressed.store( isKeyDown( VK_SHIFT ) );
            owner->m_controlPressed.store( isKeyDown( VK_CONTROL ) );

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

            owner->m_currentInputEvent = inputEvent;
            owner->m_currentKeyboardState = keyboardState;
            owner->postEvent( inputEvent );
            break;
        }
        case WM_SYSKEYUP:
        case WM_KEYUP:
        {
            if( !owner->m_createKeyboard )
            {
                break;
            }

            auto vkCode = mapVirtualKeyCode( static_cast<u32>( wParam ), lParam );
            owner->m_shiftPressed.store( isKeyDown( VK_SHIFT ) );
            owner->m_controlPressed.store( isKeyDown( VK_CONTROL ) );

            auto inputEvent = workphone::make_ptr<InputEvent>();
            inputEvent->setEventType( IInputEvent::EventType::Key );

            auto keyboardState = workphone::make_ptr<KeyboardState>();
            inputEvent->setKeyboardState( keyboardState );

            keyboardState->setPressedDown( false );
            keyboardState->setKeyCode( vkCode );
            keyboardState->setRawKeyCode( static_cast<u32>( wParam ) );
            applyKeyboardModifiers( keyboardState );
            keyboardState->setChar( 0 );

            owner->m_currentInputEvent = inputEvent;
            owner->m_currentKeyboardState = keyboardState;
            owner->postEvent( inputEvent );
            break;
        }
        case WM_CAPTURECHANGED:
        case WM_CANCELMODE:
        case WM_KILLFOCUS:
        {
            owner->m_isDragging.store( false );
            m_hasLastMousePosition = false;
            break;
        }
        default:
            break;
        }
#endif
    }

    Parameter InputDeviceManager::WindowListener::handleEvent( EventType eventType, hash_type eventValue,
                                                               const Array<Parameter> &arguments,
                                                               SmartPtr<ISharedObject> sender,
                                                               SmartPtr<ISharedObject> object,
                                                               SmartPtr<IEvent> event )
    {
        if( eventValue == render::IGraphicsWindowListener::windowClosingHash )
        {
            return Parameter( true );
        }

        return Parameter();
    }

    InputDeviceManager *InputDeviceManager::WindowListener::getOwner() const
    {
        return m_owner;
    }

    void InputDeviceManager::WindowListener::setOwner( InputDeviceManager *owner )
    {
        m_owner = owner;
    }

}  // namespace workphone
