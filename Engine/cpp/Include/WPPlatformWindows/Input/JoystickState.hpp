#ifndef FBJoystickState_h__
#define FBJoystickState_h__

#include <Workphone/Interface/Input/IJoystickState.hpp>

/**
 * @file JoystickState.hpp
 * @brief Concrete implementation of IJoystickState holding a single input
 *        event or snapshot for a joystick device.
 */

namespace workphone
{
    /**
     * @brief Represents the state or event for a joystick input.
     *
     * JoystickState stores identification (joystick id, button id), axis
     * positions, POV (hat) position and flags describing transitions such as
     * press/release. It is used to pass joystick events through the input
     * pipeline.
     */
    class WPCore_API JoystickState : public IJoystickState
    {
    public:
        /// Flag bit indicating the button is currently pressed down.
        static const u32 isPressedDownFlag;

        /// Flag bit indicating the button was pressed (edge) event.
        static const u32 isButtonPressedFlag;

        /**
         * @brief Construct a new JoystickState with default values.
         */
        JoystickState();

        /**
         * @brief Virtual destructor.
         */
        ~JoystickState() override;

        /**
         * @brief Get the joystick device id associated with this state.
         *
         * @return u32 Joystick id
         */
        u32 getJoystick() const override;

        /**
         * @brief Set the joystick device id for this state.
         *
         * @param id Joystick id
         */
        void setJoystick( u32 id ) override;

        /**
         * @brief Get the POV (hat) value for this state.
         *
         * @return u16 POV position
         */
        u16 getPOV() const override;

        /**
         * @brief Set the POV (hat) position.
         *
         * @param position POV position value
         */
        void setPOV( u32 position ) override;

        /**
         * @brief Get the axis value for the specified axis index.
         *
         * @param axisIndex Index of the axis
         * @return f32 Axis value
         */
        f32 getAxis( u32 axisIndex ) const override;

        /**
         * @brief Set the axis value for the specified axis index.
         *
         * @param axisIndex Index of the axis
         * @param position Axis value to set
         */
        void setAxis( u32 axisIndex, f32 position ) override;

        /**
         * @brief Get the button id related to this state/event.
         *
         * @return u32 Button id
         */
        u32 getButtonId() const override;

        /**
         * @brief Set the button id for this state/event.
         *
         * @param buttonId Button identifier
         */
        void setButtonId( u32 buttonId ) override;

        /**
         * @brief Query whether the button is currently held down.
         *
         * @return true if held down
         */
        bool isPressedDown() const override;

        /**
         * @brief Set the pressed-down flag for this state.
         *
         * @param pressed True if the button is held down
         */
        void setPressedDown( bool pressed ) override;

        /**
         * @brief Query whether the given button id was pressed (edge) in this
         *        state/event.
         *
         * @param id Button id to test
         * @return true if the button press event occurred
         */
        bool isButtonPressed( u32 id ) const override;

        /**
         * @brief Set or clear the button-pressed (edge) flag for a button id.
         *
         * @param id Button id
         * @param isPressed True to mark button as pressed this event
         */
        void setButtonPressed( u32 id, bool isPressed ) override;

        /**
         * @brief Get the event type bitfield for this joystick state.
         *
         * @return u32 Event type bits
         */
        u32 getEventType() const override;

        /**
         * @brief Set the event type bitfield for this joystick state.
         *
         * @param eventType Event type bits
         */
        void setEventType( u32 eventType ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /// Joystick device identifier.
        u32 m_joystick;

        /// Button identifier associated with this state/event.
        u32 m_buttonId;

        /// Event type bitfield describing the kind of event.
        u32 m_eventType;

        /// POV (point-of-view/hat) value.
        u16 m_pov;

        /// Internal flags bitfield (holds isPressedDown / isButtonPressed bits).
        u32 m_flags = 0;

        /// Per-button pressed (edge) bits. Bit index corresponds to button id.
        u32 m_buttonPressedFlags = 0;

        /// Axis values stored for each axis defined in Axis enum.
        f32 m_axis[static_cast<int>( Axis::NUMBER_OF_AXES )];
    };
}  // namespace workphone

#endif  // FBJoystickState_h__
