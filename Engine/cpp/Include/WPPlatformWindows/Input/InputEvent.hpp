#ifndef __WP_INPUT_EVENT__
#define __WP_INPUT_EVENT__

#include <Workphone/Interface/Input/IInputEvent.hpp>

namespace workphone
{
    /** A structure to store input event data. */
    class WPCore_API InputEvent : public IInputEvent
    {
    public:
        /** Constructor. */
        InputEvent();

        /** Destructor. */
        ~InputEvent() override;

        /** @copydoc IInputEvent::getMouseState */
        SmartPtr<IMouseState> getMouseState() const override;

        /** @copydoc IInputEvent::setMouseState */
        void setMouseState( SmartPtr<IMouseState> mouseState ) override;

        /** @copydoc IInputEvent::getKeyboardState */
        SmartPtr<IKeyboardState> getKeyboardState() const override;

        /** @copydoc IInputEvent::setKeyboardState */
        void setKeyboardState( SmartPtr<IKeyboardState> keyboardState ) override;

        /** @copydoc IInputEvent::getJoystickState */
        SmartPtr<IJoystickState> getJoystickState() const override;

        /** @copydoc IInputEvent::setJoystickState */
        void setJoystickState( SmartPtr<IJoystickState> joystickState ) override;

        /** @copydoc IInputEvent::getGameInputState */
        SmartPtr<IGameInputState> getGameInputState() const override;

        /** @copydoc IInputEvent::setGameInputState */
        void setGameInputState( SmartPtr<IGameInputState> gameInputState ) override;

        hash_type getGameInputId() const override;
        void setGameInputId( hash_type gameInputId ) override;

        EventType getEventType() const override;
        void setEventType( EventType eventType ) override;

        void *getUserData() const override;
        void setUserData( void *data ) override;

        void *getWindow() const override;
        void setWindow( void *window ) override;

        time_interval getTime() const override;
        void setTime( time_interval time ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        ///
        SmartPtr<IMouseState> m_mouseState;

        ///
        SmartPtr<IKeyboardState> m_keyboardState;

        ///
        SmartPtr<IJoystickState> m_joystickState;

        ///
        SmartPtr<IGameInputState> m_gameInputState;

        /// The id of the game input that was used to trigger this event.
        hash_type m_gameInputId = 0;

        /// The event type.
        EventType m_eventType = EventType::None;

        /// The time stamp.
        time_interval m_time = 0.0;

        /// User data as an unsigned int.
        void *m_userData = nullptr;

        ///
        void *m_window = nullptr;
    };
}  // namespace workphone

#endif
