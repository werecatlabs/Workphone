#ifndef IInputEvent_h__
#define IInputEvent_h__

#include <Workphone/Interface/System/IEvent.hpp>

namespace workphone
{

    /**
     * @brief Interface for input events within the engine.
     *
     * This interface provides a unified way to handle various types of input events
     * including mouse, keyboard, joystick, and custom user events. It extends the
     * base IEvent interface and manages different input device states.
     *
     * @see IEvent
     * @see IMouseState
     * @see IKeyboardState
     * @see IJoystickState
     * @see IGameInputState
     */
    class WPCore_API IInputEvent : public IEvent
    {
    public:
        /**
         * @brief Enumeration of supported event types.
         *
         * Defines the various categories of input events that can be processed
         * by the input system.
         */
        enum class EventType
        {
            None,      ///< No input event (default/null state).
            Mouse,     ///< Mouse input event (movement, clicks, wheel).
            Key,       ///< Keyboard input event (key press/release).
            Joystick,  ///< Joystick/gamepad input event.
            User,      ///< Custom user-defined event with user data.
            Count      ///< Total number of event types (for iteration).
        };

        /**
         * @brief Enumeration of specific input action types.
         *
         * Defines the specific types of input actions that can occur
         * within each event category.
         */
        enum class InputType
        {
            Reset,         ///< Reset the current input state to defaults.
            MouseMoved,    ///< Mouse cursor position has changed.
            MousePressed,  ///< A mouse button has been pressed down.
            MouseRelease,  ///< A mouse button has been released.
            KeyUp,         ///< A keyboard key has been released.
            KeyDown,       ///< A keyboard key has been pressed down.
            Count          ///< Total number of input types (for iteration).
        };

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of derived input event objects.
         */
        ~IInputEvent() override;

        /**
         * @brief Retrieves the current mouse state.
         *
         * @return Smart pointer to the current mouse state object, or nullptr if no mouse state is
         * available.
         * @see IMouseState
         */
        virtual SmartPtr<IMouseState> getMouseState() const = 0;

        /**
         * @brief Sets the current mouse state.
         *
         * @param mouseState Smart pointer to the mouse state to set. Can be nullptr to clear the state.
         * @see IMouseState
         */
        virtual void setMouseState( SmartPtr<IMouseState> mouseState ) = 0;

        /**
         * @brief Retrieves the current keyboard state.
         *
         * @return Smart pointer to the current keyboard state object, or nullptr if no keyboard state is
         * available.
         * @see IKeyboardState
         */
        virtual SmartPtr<IKeyboardState> getKeyboardState() const = 0;

        /**
         * @brief Sets the current keyboard state.
         *
         * @param keyboardState Smart pointer to the keyboard state to set. Can be nullptr to clear the
         * state.
         * @see IKeyboardState
         */
        virtual void setKeyboardState( SmartPtr<IKeyboardState> keyboardState ) = 0;

        /**
         * @brief Retrieves the current joystick state.
         *
         * @return Smart pointer to the current joystick state object, or nullptr if no joystick state is
         * available.
         * @see IJoystickState
         */
        virtual SmartPtr<IJoystickState> getJoystickState() const = 0;

        /**
         * @brief Sets the current joystick state.
         *
         * @param joystickState Smart pointer to the joystick state to set. Can be nullptr to clear the
         * state.
         * @see IJoystickState
         */
        virtual void setJoystickState( SmartPtr<IJoystickState> joystickState ) = 0;

        /**
         * @brief Retrieves the current game input state.
         *
         * Game input state represents high-level game-specific input mappings
         * that can combine multiple input devices.
         *
         * @return Smart pointer to the current game input state object, or nullptr if no game input
         * state is available.
         * @see IGameInputState
         */
        virtual SmartPtr<IGameInputState> getGameInputState() const = 0;

        /**
         * @brief Sets the current game input state.
         *
         * @param gameInputState Smart pointer to the game input state to set. Can be nullptr to clear
         * the state.
         * @see IGameInputState
         */
        virtual void setGameInputState( SmartPtr<IGameInputState> gameInputState ) = 0;

        /**
         * @brief Retrieves the ID of the current game input state.
         *
         * The game input ID provides a unique identifier for the current
         * game input configuration or mapping.
         *
         * @return Hash value representing the game input ID, or 0 if no ID is set.
         */
        virtual hash_type getGameInputId() const = 0;

        /**
         * @brief Sets the ID of the current game input state.
         *
         * @param gameInputId Hash value representing the game input ID to set.
         */
        virtual void setGameInputId( hash_type gameInputId ) = 0;

        /**
         * @brief Retrieves the type of this event.
         *
         * @return The event type indicating what kind of input event this represents.
         * @see EventType
         */
        virtual EventType getEventType() const = 0;

        /**
         * @brief Sets the type of this event.
         *
         * @param eventType The event type to set for this input event.
         * @see EventType
         */
        virtual void setEventType( EventType eventType ) = 0;

        /**
         * @brief Retrieves a pointer to the associated window.
         *
         * Returns a raw pointer to the window or rendering context that
         * this input event is associated with. The actual type depends
         * on the platform and windowing system being used.
         *
         * @return Raw pointer to the window object, or nullptr if no window is associated.
         * @warning The returned pointer should not be deleted by the caller.
         */
        virtual void *getWindow() const = 0;

        /**
         * @brief Sets the associated window pointer.
         *
         * Associates this input event with a specific window or rendering context.
         *
         * @param window Raw pointer to the window object to associate with this event.
         *               Can be nullptr to disassociate from any window.
         * @warning The caller is responsible for ensuring the window pointer remains valid
         *          for the lifetime of this input event.
         */
        virtual void setWindow( void *window ) = 0;

        virtual time_interval getTime() const = 0;
        virtual void setTime( time_interval time ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IInputEvent_h__
