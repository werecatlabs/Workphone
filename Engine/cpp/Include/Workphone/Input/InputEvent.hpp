#ifndef __WP_INPUT_EVENT__
#define __WP_INPUT_EVENT__

#include <Workphone/Interface/Input/IInputEvent.hpp>

namespace workphone
{
    /**
     * @class InputEvent
     * @brief Default implementation of IInputEvent.
     *
     * Carries the data for a single input event: the event type, a time stamp, the
     * window the event originated from, and the device state snapshots (mouse, keyboard,
     * joystick and game input) relevant to the event. Only the state objects that apply
     * to the event type are normally set; the others remain null.
     */
    class WPCore_API InputEvent : public IInputEvent
    {
    public:
        /** @brief Constructs an empty event of type EventType::None with no states attached. */
        InputEvent();

        /** @brief Destructor. */
        ~InputEvent() override;

        /**
         * @brief Gets the optional property bag attached to this event.
         * @return The event properties, or a null pointer if none were set.
         */
        SmartPtr<Properties> getProperties() const override;

        /**
         * @brief Attaches an optional property bag to this event.
         * @param properties The properties to attach; may be null to clear them.
         */
        void setProperties( SmartPtr<Properties> properties ) override;

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

        /** @copydoc IInputEvent::getGameInputId */
        hash_type getGameInputId() const override;

        /** @copydoc IInputEvent::setGameInputId */
        void setGameInputId( hash_type gameInputId ) override;

        /** @copydoc IInputEvent::getEventType */
        EventType getEventType() const override;

        /** @copydoc IInputEvent::setEventType */
        void setEventType( EventType eventType ) override;

        /**
         * @brief Gets the user data pointer attached to this event.
         * @return The user data pointer, or nullptr if none was set. Ownership is not transferred.
         */
        void *getUserData() const override;

        /**
         * @brief Attaches an opaque user data pointer to this event.
         * @param data Pointer to caller-owned data; the event does not take ownership.
         */
        void setUserData( void *data ) override;

        /** @copydoc IInputEvent::getWindow */
        void *getWindow() const override;

        /** @copydoc IInputEvent::setWindow */
        void setWindow( void *window ) override;

        /**
         * @brief Gets the time stamp of the event.
         * @return The time at which the event occurred.
         */
        time_interval getTime() const override;

        /**
         * @brief Sets the time stamp of the event.
         * @param time The time at which the event occurred.
         */
        void setTime( time_interval time ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /// Mouse state snapshot for mouse events; null otherwise.
        SmartPtr<IMouseState> m_mouseState;

        /// Keyboard state snapshot for key events; null otherwise.
        SmartPtr<IKeyboardState> m_keyboardState;

        /// Joystick state snapshot for joystick events; null otherwise.
        SmartPtr<IJoystickState> m_joystickState;

        /// Game input state snapshot for game input events; null otherwise.
        SmartPtr<IGameInputState> m_gameInputState;

        /// The id (hash) of the game input that triggered this event; 0 if none.
        hash_type m_gameInputId = 0;

        /// The event type.
        EventType m_eventType = EventType::None;

        /// The time stamp of the event.
        time_interval m_time = 0.0;

        /// Opaque, caller-owned user data pointer.
        void *m_userData = nullptr;

        /// Opaque, platform-specific handle of the window the event originated from; not owned.
        void *m_window = nullptr;

        /// Optional property bag attached to the event.
        SmartPtr<Properties> m_eventProperties;
    };
}  // namespace workphone

#endif
