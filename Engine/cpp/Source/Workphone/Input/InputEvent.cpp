#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Input/InputEvent.hpp>
#include <Workphone/Interface/Input/IJoystickState.hpp>
#include <Workphone/Interface/Input/IMouseState.hpp>
#include <Workphone/Interface/Input/IKeyboardState.hpp>
#include <Workphone/Interface/Input/IGameInputState.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, InputEvent, IInputEvent );

    InputEvent::InputEvent() = default;

    InputEvent::~InputEvent() = default;

    auto InputEvent::getMouseState() const -> SmartPtr<IMouseState>
    {
        return m_mouseState;
    }

    void InputEvent::setMouseState( SmartPtr<IMouseState> mouseState )
    {
        m_mouseState = mouseState;
    }

    auto InputEvent::getKeyboardState() const -> SmartPtr<IKeyboardState>
    {
        return m_keyboardState;
    }

    void InputEvent::setKeyboardState( SmartPtr<IKeyboardState> keyboardState )
    {
        m_keyboardState = keyboardState;
    }

    auto InputEvent::getJoystickState() const -> SmartPtr<IJoystickState>
    {
        return m_joystickState;
    }

    void InputEvent::setJoystickState( SmartPtr<IJoystickState> joystickState )
    {
        m_joystickState = joystickState;
    }

    auto InputEvent::getGameInputState() const -> SmartPtr<IGameInputState>
    {
        return m_gameInputState;
    }

    void InputEvent::setGameInputState( SmartPtr<IGameInputState> gameInputState )
    {
        m_gameInputState = gameInputState;
    }

    auto InputEvent::getGameInputId() const -> hash_type
    {
        return m_gameInputId;
    }

    void InputEvent::setGameInputId( hash_type gameInputId )
    {
        m_gameInputId = gameInputId;
    }

    auto InputEvent::getEventType() const -> IInputEvent::EventType
    {
        return m_eventType;
    }

    void InputEvent::setEventType( EventType eventType )
    {
        m_eventType = eventType;
    }

    auto InputEvent::getUserData() const -> void *
    {
        return m_userData;
    }

    void InputEvent::setUserData( void *data )
    {
        m_userData = data;
    }

    auto InputEvent::getWindow() const -> void *
    {
        return m_window;
    }

    void InputEvent::setWindow( void *window )
    {
        m_window = window;
    }

    time_interval InputEvent::getTime() const
    {
        return m_time;
    }

    void InputEvent::setTime( time_interval time )
    {
        m_time = time;
    }

}  // namespace workphone
