#include <EditorPCH.hpp>
#include <editor/McpPlaytest.hpp>

#include <EditorApplication.hpp>
#include <editor/EditorManager.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Input/InputDeviceManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

#include <bitset>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <thread>

namespace workphone::editor
{
    namespace
    {
        using MouseEvent = IMouseState::Event;
        constexpr const char *buttonNames[] = { "left", "right", "middle" };

        struct PointerInput
        {
            MouseEvent type;
            Vector2<real_Num> relative;
            int button = -1;
            real_Num wheel = 0;
            bool pressed = false;
        };

        double numberInRange( const boost::json::object &args, const char *field, double low,
                              double high )
        {
            const auto value = args.if_contains( field );
            if( value && value->is_number() )
            {
                const auto n = value->to_number<double>();
                if( std::isfinite( n ) && n >= low && n <= high )
                {
                    return n;
                }
            }
            throw std::invalid_argument( std::string( "Invalid pointer " ) + field );
        }

        PointerInput parsePointer( const boost::json::object &args )
        {
            PointerInput pointer{ MouseEvent::Moved,
                                  { real_Num( numberInRange( args, "x", 0, 1 ) ),
                                    real_Num( numberInRange( args, "y", 0, 1 ) ) } };
            const auto &action = args.at( "action" ).as_string();
            if( action == "press" || action == "release" )
            {
                const auto &button = args.at( "button" ).as_string();
                for( int i = 0; i < 3; ++i )
                {
                    if( button == buttonNames[i] )
                    {
                        pointer.button = i;
                        break;
                    }
                }
                if( pointer.button == -1 )
                {
                    throw std::invalid_argument( "button must be left, right or middle" );
                }
                constexpr MouseEvent press[] = { MouseEvent::LeftPressed, MouseEvent::RightPressed,
                                                  MouseEvent::MiddlePressed };
                constexpr MouseEvent release[] = { MouseEvent::LeftReleased, MouseEvent::RightReleased,
                                                    MouseEvent::MiddleReleased };
                pointer.pressed = action == "press";
                pointer.type = pointer.pressed ? press[pointer.button] : release[pointer.button];
            }
            else if( action == "wheel" )
            {
                pointer.type = MouseEvent::Wheel;
                pointer.wheel = real_Num( numberInRange( args, "wheelDelta", -12000, 12000 ) );
                if( pointer.wheel == 0 )
                {
                    throw std::invalid_argument( "wheelDelta must be nonzero" );
                }
            }
            else if( action != "move" )
            {
                throw std::invalid_argument( "action must be move, press, release or wheel" );
            }
            if( ( args.contains( "button" ) && pointer.button == -1 ) ||
                ( args.contains( "wheelDelta" ) && pointer.type != MouseEvent::Wheel ) )
            {
                throw std::invalid_argument( "Unexpected field for this pointer action" );
            }
            return pointer;
        }

        // Only the MCP host constructs this device. Keep polling and queued events
        // local to the game process, including when the editor has no window focus.
        class McpInputDevice final : public InputDeviceManager
        {
        public:
            bool isKeyPressed( KeyCodes code ) const override
            {
                ScopedLock lock( &m_mutex );
                const auto key = static_cast<u32>( code );
                if( code == KeyCodes::KEY_SHIFT )
                {
                    return m_keys[key] || m_keys[static_cast<u32>( KeyCodes::KEY_LSHIFT )] ||
                           m_keys[static_cast<u32>( KeyCodes::KEY_RSHIFT )];
                }
                if( code == KeyCodes::KEY_CONTROL )
                {
                    return m_keys[key] || m_keys[static_cast<u32>( KeyCodes::KEY_LCONTROL )] ||
                           m_keys[static_cast<u32>( KeyCodes::KEY_RCONTROL )];
                }
                return key < m_keys.size() && m_keys[key];
            }

            bool postKey( u32 key, bool down )
            {
                ScopedLock lock( &m_mutex );
                auto keyboard = createKeyboardState();
                auto event = createInputEvent();
                keyboard->setKeyCode( key );
                keyboard->setRawKeyCode( key );
                keyboard->setPressedDown( down );
                event->setKeyboardState( keyboard );
                event->setEventType( IInputEvent::EventType::Key );
                const bool previous = m_keys[key];
                m_keys[key] = down;
                keyboard->setShiftPressed( isKeyPressed( KeyCodes::KEY_SHIFT ) );
                keyboard->setControlPressed( isKeyPressed( KeyCodes::KEY_CONTROL ) );
                if( !postEvent( event ) )
                {
                    m_keys[key] = previous;
                    return false;
                }
                m_shiftPressed.store( keyboard->isShiftPressed() );
                m_controlPressed.store( keyboard->isControlPressed() );
                m_currentKeyboardState = keyboard;
                m_currentInputEvent = event;
                return true;
            }

            // The editor still pumps window messages. This device owns gameplay pointer
            // state and performs no native cursor movement, capture or visibility changes.
            void setCreateMouse( bool ) override { InputDeviceManager::setCreateMouse( false ); }
            void setCursorVisible( bool visible ) override { m_cursorVisible.store( visible ); }

            bool isMouseButtonDown( u32 button ) const override
            {
                ScopedLock lock( &m_mutex );
                return button < m_buttons.size() && m_buttons[button];
            }

            boost::json::object postPointer( const PointerInput &input, const Vector2I &size )
            {
                if( size.x <= 0 || size.y <= 0 )
                {
                    throw std::runtime_error( "The editor client has no drawable area" );
                }
                ScopedLock lock( &m_mutex );
                const Vector2<real_Num> dimensions( real_Num( size.x ), real_Num( size.y ) );
                // Native events use client pixels, with relative coordinates divided
                // by the full client size. Clamp the inclusive far edge to its last pixel.
                const Vector2<real_Num> pixels(
                    std::min( std::floor( input.relative.x * dimensions.x ), dimensions.x - 1 ),
                    std::min( std::floor( input.relative.y * dimensions.y ), dimensions.y - 1 ) );
                auto buttons = m_buttons;
                if( input.button >= 0 )
                {
                    buttons.set( input.button, input.pressed );
                }
                const auto origin = input.type == MouseEvent::LeftPressed ? pixels : m_dragStartPosition;
                auto mouse = createMouseState();
                mouse->setEventType( input.type );
                mouse->setRelativePosition( pixels / dimensions );
                mouse->setAbsolutePosition( pixels );
                mouse->setDelta( m_hasPointer ? m_mousePosition - pixels : Vector2<real_Num>::zero() );
                mouse->setDragging( buttons[0] );
                mouse->setDragValue( pixels - origin );
                mouse->setWheelDelta( Vector2<real_Num>( 0, input.wheel ) );
                mouse->setShiftPressed( isKeyPressed( KeyCodes::KEY_SHIFT ) );
                mouse->setControlPressed( isKeyPressed( KeyCodes::KEY_CONTROL ) );
                const u32 flags[] = { IMouseState::MOUSE_LEFT, IMouseState::MOUSE_RIGHT,
                                      IMouseState::MOUSE_MIDDLE };
                boost::json::array held;
                for( u32 i = 0; i < buttons.size(); ++i )
                {
                    mouse->setButtonPressed( flags[i], buttons[i] );
                    if( buttons[i] )
                    {
                        held.emplace_back( buttonNames[i] );
                    }
                }
                auto event = createInputEvent();
                event->setEventType( IInputEvent::EventType::Mouse );
                event->setMouseState( mouse );
                if( !postEvent( event ) )
                {
                    throw std::runtime_error( "The input backend did not accept the pointer event" );
                }
                m_buttons = buttons;
                m_hasPointer = true;
                m_mousePosition = pixels;
                m_isDragging.store( buttons[0] );
                m_dragStartPosition = origin;
                if( input.type == MouseEvent::Wheel )
                {
                    m_mouseScroll = Vector3<real_Num>( 0, input.wheel, 0 );
                }
                m_currentMouseState = mouse;
                m_currentInputEvent = event;
                return { { "pixelX", static_cast<int>( pixels.x ) },
                         { "pixelY", static_cast<int>( pixels.y ) }, { "buttons", std::move( held ) } };
            }

            void resetInput()
            {
                ScopedLock lock( &m_mutex );
                // A release or click can be queued immediately before Stop. Do not
                // deliver this session's pending actions to restored editor actors.
                for( auto &queue : m_inputEventQueue )
                {
                    queue.clear();
                }
                m_keys.reset();
                m_buttons.reset();
                m_hasPointer = false;
                m_isDragging.store( false );
                m_mousePosition = Vector2<real_Num>::zero();
                m_mouseScroll = Vector3<real_Num>::zero();
                m_dragStartPosition = Vector2<real_Num>::zero();
                m_currentMouseState = createMouseState();
                m_shiftPressed.store( false );
                m_controlPressed.store( false );
                m_currentKeyboardState = createKeyboardState();
                m_currentInputEvent = nullptr;
            }

            WP_CLASS_REGISTER_DECL;

        private:
            std::bitset<256> m_keys;
            std::bitset<3> m_buttons;
            bool m_hasPointer = false;
        };

        WP_CLASS_REGISTER_DERIVED( workphone::editor, McpInputDevice, InputDeviceManager );

        u32 boundedInteger( const boost::json::object &args, const char *key, u32 maximum )
        {
            auto value = args.if_contains( key );
            if( value && ( value->is_uint64() || ( value->is_int64() && value->as_int64() > 0 ) ) )
            {
                const auto n = value->is_uint64() ? value->as_uint64() :
                                                    static_cast<uint64_t>( value->as_int64() );
                if( n > 0 && n <= maximum )
                {
                    return static_cast<u32>( n );
                }
            }
            throw std::invalid_argument( std::string( key ) + " must be an integer from 1 to " +
                                         std::to_string( maximum ) );
        }

        template <class Predicate>
        void pumpUntil( EditorApplication &app, Predicate done, const char *failure )
        {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 15 );
            while( !done() )
            {
                if( core::IApplicationManager::instance()->getQuit() ||
                    std::chrono::steady_clock::now() >= deadline )
                {
                    throw std::runtime_error( failure );
                }
                app.iterate();
                if( !done() )
                {
                    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                }
            }
        }
    }

    SmartPtr<IInputDeviceManager> createMcpInputDevice( SmartPtr<Properties> settings )
    {
        auto device = workphone::make_ptr<McpInputDevice>();
        device->setProperties( settings );
        return device;
    }

    boost::json::object mcpPlaytest( const std::string &name, const boost::json::object &args,
                                     EditorApplication &app )
    {
        auto manager = core::IApplicationManager::instance();
        auto editor = EditorManager::getSingletonPtr();
        const auto game = manager ? manager->getGameManager() : nullptr;
        auto timer = manager ? manager->getTimer() : nullptr;
        if( !app.isLoaded() || !game || !timer || !editor || !manager->getJobQueue() )
        {
            throw std::runtime_error( "The editor game systems are unavailable" );
        }
        auto device = dynamic_pointer_cast<McpInputDevice>( manager->getInputDeviceManager() );
        boost::json::object result;
        if( name == "play_start" )
        {
            if( manager->isPlaying() || editor->getPlayModeSceneData() )
            {
                throw std::runtime_error( "A play session is already active" );
            }
            if( !game->getCurrentScene() )
            {
                throw std::runtime_error( "Open a scene before starting a playtest" );
            }
            if( device )
            {
                device->resetInput();
            }
            app.enterPlayMode();
            pumpUntil( app, [&] {
                return manager->isPlaying() || !manager->getJobQueue()->hasJobs();
            }, "Timed out entering Play mode" );
            const auto scene = game->getCurrentScene();
            if( !manager->isPlaying() || !scene || !editor->getPlayModeSceneData() ||
                scene->getState() != scene::IGameScene::State::Play )
            {
                throw std::runtime_error( "The editor could not enter Play mode. Check the editor log." );
            }
        }
        else if( name == "play_step" )
        {
            const auto frames = args.contains( "frames" ) ? boundedInteger( args, "frames", 120 ) : 1u;
            if( !manager->isPlaying() || manager->isPaused() )
            {
                throw std::runtime_error( "Start an unpaused play session before stepping" );
            }
            const auto first = timer->getTickCount( TaskId::Application );
            // iterate() also pumps render, input, jobs, physics and the scene. Frame-rate
            // limits can leave the application task idle during individual iterations.
            pumpUntil( app, [&] {
                return timer->getTickCount( TaskId::Application ) - first >= frames ||
                       !manager->isPlaying() || manager->isPaused();
            }, "Timed out waiting for gameplay frames" );
            result["frames"] = timer->getTickCount( TaskId::Application ) - first;
        }
        else if( name == "play_input" )
        {
            const auto key = boundedInteger( args, "key", 255 );
            auto down = args.if_contains( "down" );
            if( !down || !down->is_bool() )
            {
                throw std::invalid_argument( "down must be a boolean" );
            }
            if( !manager->isPlaying() || manager->isPaused() )
            {
                throw std::runtime_error( "Start an unpaused play session before sending input" );
            }
            if( !device || !device->postKey( key, down->as_bool() ) )
            {
                throw std::runtime_error( "The input backend did not accept the virtual key event" );
            }
            result["key"] = key;
            result["down"] = device->isKeyPressed( static_cast<KeyCodes>( key ) );
        }
        else if( name == "play_pointer" )
        {
            const auto pointer = parsePointer( args );
            if( !manager->isPlaying() || manager->isPaused() )
            {
                throw std::runtime_error( "Start an unpaused play session before sending input" );
            }
            auto window = device ? device->getWindow() : nullptr;
            if( !window )
            {
                throw std::runtime_error( "The editor input backend has no virtual pointer window" );
            }
            result = device->postPointer( pointer, window->getSize() );
        }
        else if( name == "play_stop" )
        {
            const auto saved = editor->getPlayModeScene();
            if( !saved || !editor->getPlayModeSceneData() )
            {
                throw std::runtime_error( "There is no captured play session to stop" );
            }
            if( device )
            {
                device->resetInput();
            }
            app.stopPlayMode();
            pumpUntil( app, [&] {
                return !manager->isPlaying() && !editor->getPlayModeSceneData();
            }, "Timed out restoring the editor scene" );
            if( game->getCurrentScene() != saved || saved->getState() != scene::IGameScene::State::Edit )
            {
                throw std::runtime_error( "The editor did not restore the original scene" );
            }
        }
        else if( name != "play_status" )
        {
            throw std::invalid_argument( "Unknown playtest command" );
        }

        const auto scene = game->getCurrentScene();
        result["playing"] = manager->isPlaying();
        result["paused"] = manager->isPaused();
        result["keyboardAvailable"] = static_cast<bool>( device );
        result["pointerAvailable"] = static_cast<bool>( device );
        result["applicationTick"] = timer->getTickCount( TaskId::Application );
        result["scene"] = scene ? scene->getLabel().c_str() : "";
        return result;
    }
}
