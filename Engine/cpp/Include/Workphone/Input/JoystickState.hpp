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
     * positions, the POV (hat) position, an event type and flags describing
     * the pressed state. It is used to pass joystick events through the input
     * pipeline.
     *
     * @note Instances are plain data holders and are not thread-safe.
     * @note The axis values are not initialized by the constructor; set an
     *       axis before relying on its value.
     */
    class WPCore_API JoystickState : public IJoystickState
    {
    public:
        /// Bit (1 << 0) of the flags bitfield used by isPressedDown() / setPressedDown().
        static const u32 isPressedDownFlag;

        /// Bit (1 << 1) reserved for a button-pressed event; not used by isButtonPressed(), which has its own per-button bits.
        static const u32 isButtonPressedFlag;

        /**
         * @brief Construct a new JoystickState with default values.
         *
         * Joystick id, button id, event type, POV and flags are zero.
         */
        JoystickState();

        /**
         * @brief Virtual destructor.
         */
        ~JoystickState() override;

        /**
         * @brief Get the joystick device id associated with this state.
         *
         * @return u32 Joystick id (default 0)
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
         * @return u16 POV position, typically one of the IJoystickState::POV_*
         *         constants (default IJoystickState::POV_Centered)
         */
        u16 getPOV() const override;

        /**
         * @brief Set the POV (hat) position.
         *
         * @param position POV position, typically one of the IJoystickState::POV_*
         *                 constants. The value is narrowed to 16 bits when stored.
         */
        void setPOV( u32 position ) override;

        /**
         * @brief Get the axis value for the specified axis index.
         *
         * @param axisIndex Index of the axis, in the range [0, Axis::NUMBER_OF_AXES)
         * @return f32 Axis value, or 0 if the index is out of range
         */
        f32 getAxis( u32 axisIndex ) const override;

        /**
         * @brief Set the axis value for the specified axis index.
         *
         * @param axisIndex Index of the axis, in the range [0, Axis::NUMBER_OF_AXES);
         *                  ignored if out of range
         * @param position Axis value to set
         */
        void setAxis( u32 axisIndex, f32 position ) override;

        /**
         * @brief Get the button id related to this state/event.
         *
         * @return u32 Button id (default 0)
         */
        u32 getButtonId() const override;

        /**
         * @brief Set the button id for this state/event.
         *
         * @param buttonId Button identifier
         */
        void setButtonId( u32 buttonId ) override;

        /**
         * @brief Query whether the button of this state/event is held down.
         *
         * @return true if the pressed-down flag is set
         */
        bool isPressedDown() const override;

        /**
         * @brief Set the pressed-down flag for this state.
         *
         * @param pressed True if the button is held down
         */
        void setPressedDown( bool pressed ) override;

        /**
         * @brief Query whether the given button is marked as pressed in this
         *        state/event.
         *
         * Each button has its own bit, independent of isPressedDown().
         *
         * @param id Button id to test, in the range [0, Axis::NUMBER_OF_BUTTONS)
         * @return true if the button is marked pressed; false if it is not or
         *         if the id is out of range
         */
        bool isButtonPressed( u32 id ) const override;

        /**
         * @brief Set or clear the pressed mark for a button id.
         *
         * @param id Button id, in the range [0, Axis::NUMBER_OF_BUTTONS);
         *           ignored if out of range
         * @param isPressed True to mark the button as pressed, false to clear it
         */
        void setButtonPressed( u32 id, bool isPressed ) override;

        /**
         * @brief Get the event type for this joystick state.
         *
         * @return u32 Event type value, normally derived from IJoystickState::Type
         *         (default 0)
         */
        u32 getEventType() const override;

        /**
         * @brief Set the event type for this joystick state.
         *
         * @param eventType Event type value, normally derived from IJoystickState::Type
         */
        void setEventType( u32 eventType ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /// Joystick device identifier.
        u32 m_joystick;

        /// Button identifier associated with this state/event.
        u32 m_buttonId;

        /// Event type value describing the kind of event.
        u32 m_eventType;

        /// POV (point-of-view/hat) value.
        u16 m_pov;

        /// Flags bitfield; currently holds the isPressedDownFlag bit.
        u32 m_flags = 0;

        /// Per-button pressed marks. Bit index corresponds to button id.
        u32 m_buttonPressedFlags = 0;

        /// Axis values, indexed by the axes in the Axis enum (not zero-initialized).
        f32 m_axis[static_cast<int>( Axis::NUMBER_OF_AXES )];
    };
}  // namespace workphone

#endif  // FBJoystickState_h__
