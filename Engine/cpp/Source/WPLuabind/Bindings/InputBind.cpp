#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuabind/Bindings/InputBind.hpp"
#include "WPLuabind/WPLuabindTypes.hpp"
#include "WPLuabind/SmartPtrConverter.hpp"
#include "WPLuabind/ParamConverter.hpp"
#include <Workphone/Workphone.hpp>
#include <luabind/luabind.hpp>

namespace workphone
{
    // ---------------------------------------------------------------------------
    // IGameInput helpers
    // ---------------------------------------------------------------------------

    void _setKeyboardAction( IGameInput *gameInput, hash32 id, const char *key0, const char *key1 )
    {
        gameInput->getGameInputMap()->setKeyboardAction( id, key0, key1 );
    }

    // ---------------------------------------------------------------------------
    // IKeyboardState helpers
    // ---------------------------------------------------------------------------

    bool _isPressedDown( const IKeyboardState *keyboardState )
    {
        return keyboardState->isPressedDown();
    }

    bool _isPressedDownByKey( const IKeyboardState *keyboardState, lua_Integer keycode )
    {
        return keyboardState->isPressedDown( static_cast<u32>( keycode ) );
    }

    // ---------------------------------------------------------------------------
    // IGameInputMap helpers
    // ---------------------------------------------------------------------------

    void _setKeyboardActionMap( IGameInputMap *map, lua_Integer id, const String &key0,
                                const String &key1 )
    {
        map->setKeyboardAction( static_cast<u32>( id ), key0, key1 );
    }

    void _getKeyboardAction( IGameInputMap *map, lua_Integer id, String &key0, String &key1 )
    {
        map->getKeyboardAction( static_cast<u32>( id ), key0, key1 );
    }

    void _setJoystickAction( IGameInputMap *map, lua_Integer id, lua_Integer button )
    {
        map->setJoystickAction( static_cast<u32>( id ), static_cast<u32>( button ),
                                IGameInput::UNASSIGNED );
    }

    void _setJoystickAction2( IGameInputMap *map, lua_Integer id, lua_Integer button0,
                              lua_Integer button1 )
    {
        map->setJoystickAction( static_cast<u32>( id ), static_cast<u32>( button0 ),
                                static_cast<u32>( button1 ) );
    }

    void _getJoystickAction( IGameInputMap *map, lua_Integer id, lua_Integer &button0,
                             lua_Integer &button1 )
    {
        u32 btn0 = 0, btn1 = 0;
        map->getJoystickAction( static_cast<u32>( id ), btn0, btn1 );
        button0 = static_cast<lua_Integer>( btn0 );
        button1 = static_cast<lua_Integer>( btn1 );
    }

    hash32 _getActionFromButton( IGameInputMap *map, lua_Integer button )
    {
        return map->getActionFromButton( static_cast<u32>( button ) );
    }

    hash32 _getActionFromKey( IGameInputMap *map, lua_Integer key )
    {
        return map->getActionFromKey( static_cast<u32>( key ) );
    }

    // ---------------------------------------------------------------------------
    // IJoystickState helpers — getEventType returns u32, expose as lua_Integer
    // ---------------------------------------------------------------------------

    lua_Integer _getJoystickEventType( const IJoystickState *joystickState )
    {
        return joystickState->getEventType();
    }

    // ---------------------------------------------------------------------------
    // IMouseState helpers — getEventType/setEventType use IMouseState::Event
    // ---------------------------------------------------------------------------

    lua_Integer _getMouseEventType( const IMouseState *mouseState )
    {
        return static_cast<lua_Integer>( mouseState->getEventType() );
    }

    void _setMouseEventType( IMouseState *mouseState, lua_Integer eventType )
    {
        mouseState->setEventType( static_cast<IMouseState::Event>( eventType ) );
    }

    // ---------------------------------------------------------------------------
    // IGameInputState helpers — hash32 values exposed as lua_Integer
    // ---------------------------------------------------------------------------

    lua_Integer _getGameEventType( const IGameInputState *gameInputEvent )
    {
        return gameInputEvent->getEventType();
    }

    lua_Integer _getAction( const IGameInputState *gameInputEvent )
    {
        return gameInputEvent->getAction();
    }

    // ---------------------------------------------------------------------------
    // IInputEvent helpers — getEventType/setEventType use IInputEvent::EventType
    // ---------------------------------------------------------------------------

    lua_Integer _getInputEventType( const IInputEvent *event )
    {
        return static_cast<lua_Integer>( event->getEventType() );
    }

    void _setInputEventType( IInputEvent *event, lua_Integer eventType )
    {
        event->setEventType( static_cast<IInputEvent::EventType>( eventType ) );
    }

    bool _isKeyPressed( const IInputDeviceManager *manager, lua_Integer keyCode )
    {
        return manager->isKeyPressed( static_cast<KeyCodes>( keyCode ) );
    }

    lua_Integer _getInputTask( const IInputDeviceManager *manager )
    {
        return static_cast<lua_Integer>( manager->getTask() );
    }

    void _setInputTask( IInputDeviceManager *manager, lua_Integer task )
    {
        manager->setTask( static_cast<TaskId>( task ) );
    }

    lua_Integer _getInputEventTask( const IInputDeviceManager *manager )
    {
        return static_cast<lua_Integer>( manager->getEventTask() );
    }

    void _setInputEventTask( IInputDeviceManager *manager, lua_Integer task )
    {
        manager->setEventTask( static_cast<TaskId>( task ) );
    }

    void bindInput( lua_State *L )
    {
        using namespace luabind;

        using JoystickVector = Array<SmartPtr<IJoystick>>;

        module(
            L )[class_<KeyCodes>( "KeyCode" )
                    .enum_( "constants" )[value( "Backspace", static_cast<u32>( KeyCodes::KEY_BACK ) ),
                                          value( "Tab", static_cast<u32>( KeyCodes::KEY_TAB ) ),
                                          value( "Return", static_cast<u32>( KeyCodes::KEY_RETURN ) ),
                                          value( "Shift", static_cast<u32>( KeyCodes::KEY_SHIFT ) ),
                                          value( "Control", static_cast<u32>( KeyCodes::KEY_CONTROL ) ),
                                          value( "Escape", static_cast<u32>( KeyCodes::KEY_ESCAPE ) ),
                                          value( "Space", static_cast<u32>( KeyCodes::KEY_SPACE ) ),
                                          value( "Left", static_cast<u32>( KeyCodes::KEY_LEFT ) ),
                                          value( "Up", static_cast<u32>( KeyCodes::KEY_UP ) ),
                                          value( "Right", static_cast<u32>( KeyCodes::KEY_RIGHT ) ),
                                          value( "Down", static_cast<u32>( KeyCodes::KEY_DOWN ) ),
                                          value( "A", static_cast<u32>( KeyCodes::KEY_KEY_A ) ),
                                          value( "D", static_cast<u32>( KeyCodes::KEY_KEY_D ) ),
                                          value( "E", static_cast<u32>( KeyCodes::KEY_KEY_E ) ),
                                          value( "Q", static_cast<u32>( KeyCodes::KEY_KEY_Q ) ),
                                          value( "R", static_cast<u32>( KeyCodes::KEY_KEY_R ) ),
                                          value( "S", static_cast<u32>( KeyCodes::KEY_KEY_S ) ),
                                          value( "W", static_cast<u32>( KeyCodes::KEY_KEY_W ) )]];

        module( L )[class_<JoystickVector>( "JoystickVector" )
                        .def( "size", &JoystickVector::size )
                        .def( "empty", &JoystickVector::empty )
                        .def( "at", static_cast<SmartPtr<IJoystick> &(JoystickVector::*)( size_t )>(
                                        &JoystickVector::at ) )];

        module( L )[class_<IJoystick, ISharedObject, SmartPtr<IJoystick>>( "IJoystick" )
                        .def( "isButtonDown", &IJoystick::isButtonDown )
                        .def( "isButtonPressed", &IJoystick::isButtonPressed )
                        .def( "isButtonReleased", &IJoystick::isButtonReleased )
                        .def( "getAxis", &IJoystick::getAxis )
                        .def( "getNumButtons", &IJoystick::getNumButtons )
                        .def( "getNumAxes", &IJoystick::getNumAxes )
                        .def( "getButtonName", &IJoystick::getButtonName )
                        .def( "getAxisName", &IJoystick::getAxisName )
                        .def( "setDeadZone", &IJoystick::setDeadZone )
                        .def( "getDeadZone", &IJoystick::getDeadZone )
                        .def( "setSensitivity", &IJoystick::setSensitivity )
                        .def( "getSensitivity", &IJoystick::getSensitivity )
                        .def( "setInvert", &IJoystick::setInvert )
                        .def( "getInvert", &IJoystick::getInvert )
                        .def( "setInvertX", &IJoystick::setInvertX )
                        .def( "getInvertX", &IJoystick::getInvertX )
                        .def( "setInvertY", &IJoystick::setInvertY )
                        .def( "getInvertY", &IJoystick::getInvertY )
                        .def( "setInvertZ", &IJoystick::setInvertZ )
                        .def( "getInvertZ", &IJoystick::getInvertZ )
                        .def( "setInvertRx", &IJoystick::setInvertRx )
                        .def( "getInvertRx", &IJoystick::getInvertRx )
                        .def( "setInvertRy", &IJoystick::setInvertRy )
                        .def( "getInvertRy", &IJoystick::getInvertRy )
                        .def( "setInvertRz", &IJoystick::setInvertRz )
                        .def( "getInvertRz", &IJoystick::getInvertRz )
                        .def( "setInvertSlider", &IJoystick::setInvertSlider )
                        .def( "getInvertSlider", &IJoystick::getInvertSlider )
                        .def( "setInvertDial", &IJoystick::setInvertDial )
                        .def( "getInvertDial", &IJoystick::getInvertDial )
                        .def( "setInvertWheel", &IJoystick::setInvertWheel )
                        .def( "getInvertWheel", &IJoystick::getInvertWheel )
                        .def( "setInvertPOV", &IJoystick::setInvertPOV )
                        .scope[def( "typeInfo", IJoystick::typeInfo )]];

        module( L )[class_<IInputDeviceManager, ISharedObject, SmartPtr<IInputDeviceManager>>(
                        "IInputDeviceManager" )
                        .def( "triggerEvent", &IInputDeviceManager::triggerEvent )
                        .def( "queueEvent", &IInputDeviceManager::queueEvent )
                        .def( "postEvent", &IInputDeviceManager::postEvent )
                        .def( "createInputEvent", &IInputDeviceManager::createInputEvent )
                        .def( "createMouseState", &IInputDeviceManager::createMouseState )
                        .def( "createKeyboardState", &IInputDeviceManager::createKeyboardState )
                        .def( "getCurrentInputEvent", &IInputDeviceManager::getCurrentInputEvent )
                        .def( "getCurrentMouseState", &IInputDeviceManager::getCurrentMouseState )
                        .def( "getCurrentKeyboardState", &IInputDeviceManager::getCurrentKeyboardState )
                        .def( "addGameInput", &IInputDeviceManager::addGameInput )
                        .def( "findGameInput", &IInputDeviceManager::findGameInput )
                        .def( "getGameInputs", &IInputDeviceManager::getGameInputs )
                        .def( "getCreateMouse", &IInputDeviceManager::getCreateMouse )
                        .def( "setCreateMouse", &IInputDeviceManager::setCreateMouse )
                        .def( "getCreateKeyboard", &IInputDeviceManager::getCreateKeyboard )
                        .def( "setCreateKeyboard", &IInputDeviceManager::setCreateKeyboard )
                        .def( "getCreateJoysticks", &IInputDeviceManager::getCreateJoysticks )
                        .def( "setCreateJoysticks", &IInputDeviceManager::setCreateJoysticks )
                        .def( "isCursorVisible", &IInputDeviceManager::isCursorVisible )
                        .def( "setCursorVisible", &IInputDeviceManager::setCursorVisible )
                        .def( "addListener", &IInputDeviceManager::addListener )
                        .def( "removeListener", &IInputDeviceManager::removeListener )
                        .def( "removeListeners", &IInputDeviceManager::removeListeners )
                        .def( "getMouseScroll", &IInputDeviceManager::getMouseScroll )
                        .def( "isShiftPressed", &IInputDeviceManager::isShiftPressed )
                        .def( "setShiftPressed", &IInputDeviceManager::setShiftPressed )
                        .def( "isMouseButtonDown", &IInputDeviceManager::isMouseButtonDown )
                        .def( "getLastClickTime", &IInputDeviceManager::getLastClickTime )
                        .def( "setLastClickTime", &IInputDeviceManager::setLastClickTime )
                        .def( "getDoubleClickInterval", &IInputDeviceManager::getDoubleClickInterval )
                        .def( "setDoubleClickInterval", &IInputDeviceManager::setDoubleClickInterval )
                        .def( "getLastInputTime", &IInputDeviceManager::getLastInputTime )
                        .def( "setLastInputTime", &IInputDeviceManager::setLastInputTime )
                        .def( "getInputTime", &IInputDeviceManager::getInputTime )
                        .def( "setInputTime", &IInputDeviceManager::setInputTime )
                        .def( "addInputTime", &IInputDeviceManager::addInputTime )
                        .def( "isKeyPressed", _isKeyPressed )
                        .def( "getWindow", &IInputDeviceManager::getWindow )
                        .def( "setWindow", &IInputDeviceManager::setWindow )
                        .def( "getJoysticks", &IInputDeviceManager::getJoysticks )
                        .def( "setJoysticks", &IInputDeviceManager::setJoysticks )
                        .def( "getTask", _getInputTask )
                        .def( "setTask", _setInputTask )
                        .def( "getEventTask", _getInputEventTask )
                        .def( "setEventTask", _setInputEventTask )
                        .scope[def( "typeInfo", IInputDeviceManager::typeInfo )]];

        module( L )[class_<IGameInputMap, ISharedObject, SmartPtr<IGameInputMap>>( "IGameInputMap" )
                        .def( "setKeyboardAction", _setKeyboardActionMap )
                        .def( "getKeyboardAction", _getKeyboardAction )
                        .def( "setJoystickAction", _setJoystickAction )
                        .def( "setJoystickAction", _setJoystickAction2 )
                        .def( "getJoystickAction", _getJoystickAction )
                        .def( "getActionFromButton", _getActionFromButton )
                        .def( "getActionFromKey", _getActionFromKey )
                        .scope[def( "typeInfo", IGameInputMap::typeInfo )]];

        module( L )[class_<IGameInput, ISharedObject, SmartPtr<IGameInput>>( "IGameInput" )
                        .def( "isAssigned", &IGameInput::isAssigned )
                        .def( "getGameInputMap", &IGameInput::getGameInputMap )
                        .def( "setKeyboardAction", _setKeyboardAction )
                        .def( "getPlayerIndex", &IGameInput::getPlayerIndex )
                        .def( "setPlayerIndex", &IGameInput::setPlayerIndex )
                        .def( "getJoystickId", &IGameInput::getJoystickId )
                        .def( "setJoystickId", &IGameInput::setJoystickId )
                        .enum_( "constants" )[value(
                            "UNASSIGNED", static_cast<lua_Integer>( IGameInput::UNASSIGNED ) )]
                        .scope[def( "typeInfo", IGameInput::typeInfo )]];

        module(
            L )[class_<IMouseState, ISharedObject, SmartPtr<IMouseState>>( "IMouseState" )
                    .def( "getDelta", &IMouseState::getDelta )
                    .def( "setDelta", &IMouseState::setDelta )
                    .def( "getRelativePosition", &IMouseState::getRelativePosition )
                    .def( "setRelativePosition", &IMouseState::setRelativePosition )
                    .def( "getAbsolutePosition", &IMouseState::getAbsolutePosition )
                    .def( "setAbsolutePosition", &IMouseState::setAbsolutePosition )
                    .def( "getWheelDelta", &IMouseState::getWheelDelta )
                    .def( "setWheelDelta", &IMouseState::setWheelDelta )
                    .def( "isShiftPressed", &IMouseState::isShiftPressed )
                    .def( "setShiftPressed", &IMouseState::setShiftPressed )
                    .def( "isControlPressed", &IMouseState::isControlPressed )
                    .def( "setControlPressed", &IMouseState::setControlPressed )
                    .def( "isButtonPressed", &IMouseState::isButtonPressed )
                    .def( "setButtonPressed", &IMouseState::setButtonPressed )
                    .def( "getEventType", _getMouseEventType )
                    .def( "setEventType", _setMouseEventType )
                    .def( "getDragValue", &IMouseState::getDragValue )
                    .def( "setDragValue", &IMouseState::setDragValue )
                    .def( "isDragging", &IMouseState::isDragging )
                    .def( "setDragging", &IMouseState::setDragging )
                    .def( "isDoubleClick", &IMouseState::isDoubleClick )
                    .def( "setDoubleClick", &IMouseState::setDoubleClick )
                    .enum_( "Event" )
                        [value( "LeftPressed", static_cast<int>( IMouseState::Event::LeftPressed ) ),
                         value( "RightPressed", static_cast<int>( IMouseState::Event::RightPressed ) ),
                         value( "MiddlePressed", static_cast<int>( IMouseState::Event::MiddlePressed ) ),
                         value( "LeftReleased", static_cast<int>( IMouseState::Event::LeftReleased ) ),
                         value( "RightReleased", static_cast<int>( IMouseState::Event::RightReleased ) ),
                         value( "MiddleReleased",
                                static_cast<int>( IMouseState::Event::MiddleReleased ) ),
                         value( "Moved", static_cast<int>( IMouseState::Event::Moved ) ),
                         value( "Wheel", static_cast<int>( IMouseState::Event::Wheel ) ),
                         value( "Count", static_cast<int>( IMouseState::Event::Count ) )]
                    .scope[def( "typeInfo", IMouseState::typeInfo )]];

        module( L )[class_<IKeyboardState, ISharedObject, SmartPtr<IKeyboardState>>( "IKeyboardState" )
                        .def( "getChar", &IKeyboardState::getChar )
                        .def( "setChar", &IKeyboardState::setChar )
                        .def( "getKeyCode", &IKeyboardState::getKeyCode )
                        .def( "setKeyCode", &IKeyboardState::setKeyCode )
                        .def( "getRawKeyCode", &IKeyboardState::getRawKeyCode )
                        .def( "setRawKeyCode", &IKeyboardState::setRawKeyCode )
                        .def( "isPressedDown", _isPressedDown )
                        .def( "isPressedDown", _isPressedDownByKey )
                        .def( "setPressedDown", &IKeyboardState::setPressedDown )
                        .def( "isShiftPressed", &IKeyboardState::isShiftPressed )
                        .def( "setShiftPressed", &IKeyboardState::setShiftPressed )
                        .def( "isControlPressed", &IKeyboardState::isControlPressed )
                        .def( "setControlPressed", &IKeyboardState::setControlPressed )
                        .scope[def( "typeInfo", IKeyboardState::typeInfo )]];

        module(
            L )[class_<IJoystickState, ISharedObject, SmartPtr<IJoystickState>>( "IJoystickState" )
                    .def( "getJoystick", &IJoystickState::getJoystick )
                    .def( "setJoystick", &IJoystickState::setJoystick )
                    .def( "getPOV", &IJoystickState::getPOV )
                    .def( "setPOV", &IJoystickState::setPOV )
                    .def( "getAxis", &IJoystickState::getAxis )
                    .def( "setAxis", &IJoystickState::setAxis )
                    .def( "getButtonId", &IJoystickState::getButtonId )
                    .def( "setButtonId", &IJoystickState::setButtonId )
                    .def( "isPressedDown", &IJoystickState::isPressedDown )
                    .def( "setPressedDown", &IJoystickState::setPressedDown )
                    .def( "isButtonPressed", &IJoystickState::isButtonPressed )
                    .def( "setButtonPressed", &IJoystickState::setButtonPressed )
                    .def( "getEventType", _getJoystickEventType )
                    .def( "setEventType", &IJoystickState::setEventType )
                    .enum_( "Axis" )[value( "X", static_cast<int>( IJoystickState::Axis::AXIS_X ) ),
                                     value( "Y", static_cast<int>( IJoystickState::Axis::AXIS_Y ) ),
                                     value( "Z", static_cast<int>( IJoystickState::Axis::AXIS_Z ) ),
                                     value( "R", static_cast<int>( IJoystickState::Axis::AXIS_R ) ),
                                     value( "U", static_cast<int>( IJoystickState::Axis::AXIS_U ) ),
                                     value( "V", static_cast<int>( IJoystickState::Axis::AXIS_V ) )]
                    .enum_( "Type" )
                        [value( "PovMoved", static_cast<int>( IJoystickState::Type::PovMoved ) ),
                         value( "Vector3Moved", static_cast<int>( IJoystickState::Type::Vector3Moved ) ),
                         value( "ButtonPressed",
                                static_cast<int>( IJoystickState::Type::ButtonPressed ) ),
                         value( "ButtonReleased",
                                static_cast<int>( IJoystickState::Type::ButtonReleased ) ),
                         value( "SliderMoved", static_cast<int>( IJoystickState::Type::SliderMoved ) ),
                         value( "AxisMoved", static_cast<int>( IJoystickState::Type::AxisMoved ) )]
                    .scope[def( "typeInfo", IJoystickState::typeInfo )]];

        module(
            L )[class_<IGameInputState, ISharedObject, SmartPtr<IGameInputState>>( "IGameInputState" )
                    .def( "getEventType", _getGameEventType )
                    .def( "setEventType", &IGameInputState::setEventType )
                    .def( "getAction", _getAction )
                    .def( "setAction", &IGameInputState::setAction )
                    .scope[def( "typeInfo", IGameInputState::typeInfo )]];

        module( L )
            [class_<IInputEvent, IEvent, SmartPtr<IInputEvent>>( "IInputEvent" )
                 .def( "getMouseState", &IInputEvent::getMouseState )
                 .def( "setMouseState", &IInputEvent::setMouseState )
                 .def( "getKeyboardState", &IInputEvent::getKeyboardState )
                 .def( "setKeyboardState", &IInputEvent::setKeyboardState )
                 .def( "getJoystickState", &IInputEvent::getJoystickState )
                 .def( "setJoystickState", &IInputEvent::setJoystickState )
                 .def( "getGameInputState", &IInputEvent::getGameInputState )
                 .def( "setGameInputState", &IInputEvent::setGameInputState )
                 .def( "getGameInputId", &IInputEvent::getGameInputId )
                 .def( "setGameInputId", &IInputEvent::setGameInputId )
                 .def( "getEventType", _getInputEventType )
                 .def( "setEventType", _setInputEventType )
                 .def( "getTime", &IInputEvent::getTime )
                 .def( "setTime", &IInputEvent::setTime )
                 .enum_( "EventType" )
                     [value( "None", static_cast<int>( IInputEvent::EventType::None ) ),
                      value( "Mouse", static_cast<int>( IInputEvent::EventType::Mouse ) ),
                      value( "Key", static_cast<int>( IInputEvent::EventType::Key ) ),
                      value( "Joystick", static_cast<int>( IInputEvent::EventType::Joystick ) ),
                      value( "User", static_cast<int>( IInputEvent::EventType::User ) ),
                      value( "Count", static_cast<int>( IInputEvent::EventType::Count ) )]
                 .enum_( "InputType" )
                     [value( "Reset", static_cast<int>( IInputEvent::InputType::Reset ) ),
                      value( "MouseMoved", static_cast<int>( IInputEvent::InputType::MouseMoved ) ),
                      value( "MousePressed", static_cast<int>( IInputEvent::InputType::MousePressed ) ),
                      value( "MouseRelease", static_cast<int>( IInputEvent::InputType::MouseRelease ) ),
                      value( "KeyUp", static_cast<int>( IInputEvent::InputType::KeyUp ) ),
                      value( "KeyDown", static_cast<int>( IInputEvent::InputType::KeyDown ) ),
                      value( "Count", static_cast<int>( IInputEvent::InputType::Count ) )]
                 .scope[def( "typeInfo", IInputEvent::typeInfo )]];

        module( L )[class_<IInputAction, ISharedObject, SmartPtr<IInputAction>>( "IInputAction" )
                        .def( "getPrimaryAction", &IInputAction::getPrimaryAction )
                        .def( "setPrimaryAction", &IInputAction::setPrimaryAction )
                        .def( "getSecondaryAction", &IInputAction::getSecondaryAction )
                        .def( "setSecondaryAction", &IInputAction::setSecondaryAction )
                        .def( "getActionId", &IInputAction::getActionId )
                        .def( "setActionId", &IInputAction::setActionId )
                        .scope[def( "typeInfo", IInputAction::typeInfo )]];

        module(
            L )[class_<IInputConverter, ISharedObject, SmartPtr<IInputConverter>>( "IInputConverter" )
                    .def( "getInputId", &IInputConverter::getInputId )
                    .def( "getInputName", &IInputConverter::getInputName )
                    .def( "getActionId", &IInputConverter::getActionId )
                    .def( "getActionName", &IInputConverter::getActionName )
                    .scope[def( "typeInfo", IInputConverter::typeInfo )]];

        module( L )[class_<IInputManager, ISharedObject, SmartPtr<IInputManager>>( "IInputManager" )
                        .def( "isCursorVisible", &IInputManager::isCursorVisible )
                        .def( "setCursorVisible", &IInputManager::setCursorVisible )
                        .def( "setAxisValue", &IInputManager::setAxisValue )
                        .def( "getAxisValue", &IInputManager::getAxisValue )
                        .def( "getAxisValueRaw", &IInputManager::getAxisValueRaw )
                        .def( "play", &IInputManager::play )
                        .def( "record", &IInputManager::record )
                        .def( "stop", &IInputManager::stop )
                        .def( "getFlags", &IInputManager::getFlags )
                        .def( "setFlags", &IInputManager::setFlags )
                        .scope[def( "typeInfo", IInputManager::typeInfo )]];

        module( L )[class_<ISequenceDetector, ISharedObject, SmartPtr<ISequenceDetector>>(
                        "ISequenceDetector" )
                        .def( "feedInput", &ISequenceDetector::feedInput )
                        .def( "reset", &ISequenceDetector::reset )
                        .def( "isSequenceDetected", &ISequenceDetector::isSequenceDetected )
                        .def( "isPartialMatch", &ISequenceDetector::isPartialMatch )
                        .def( "setSequence", &ISequenceDetector::setSequence )
                        .def( "getSequence", &ISequenceDetector::getSequence )
                        .scope[def( "typeInfo", ISequenceDetector::typeInfo )]];

        module( L )[class_<ITapDetector, ISharedObject, SmartPtr<ITapDetector>>( "ITapDetector" )
                        .def( "feedInput", &ITapDetector::feedInput )
                        .def( "reset", &ITapDetector::reset )
                        .def( "isTapDetected", &ITapDetector::isTapDetected )
                        .def( "setTapInterval", &ITapDetector::setTapInterval )
                        .def( "getTapInterval", &ITapDetector::getTapInterval )
                        .def( "setTapCount", &ITapDetector::setTapCount )
                        .def( "getTapCount", &ITapDetector::getTapCount )
                        .scope[def( "typeInfo", ITapDetector::typeInfo )]];

        module( L )[class_<IKeymap, ISharedObject, SmartPtr<IKeymap>>( "IKeymap" )
                        .def( "getKeycode", &IKeymap::getKeycode )
                        .def( "getExternalKeycode", &IKeymap::getExternalKeycode )
                        .scope[def( "typeInfo", IKeymap::typeInfo )]];

        module( L )[class_<IChordDetector, ISharedObject, SmartPtr<IChordDetector>>( "IChordDetector" )
                        .def( "addAction", &IChordDetector::addAction )
                        .def( "removeAction", &IChordDetector::removeAction )
                        .def( "clearAction", &IChordDetector::clearAction )
                        .def( "getActions", &IChordDetector::getActions )
                        .scope[def( "typeInfo", IChordDetector::typeInfo )]];
    }
} // namespace workphone
