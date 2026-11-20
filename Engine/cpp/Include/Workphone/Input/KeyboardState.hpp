#ifndef FBKeyInput_h__
#define FBKeyInput_h__

#include <Workphone/Interface/Input/IKeyboardState.hpp>

/**
 * @file KeyboardState.hpp
 * @brief Concrete keyboard input state used to represent key events and
 *        modifier keys.
 */

namespace workphone
{
    /**
     * @brief Stores keyboard event/state information.
     *
     * KeyboardState implements IKeyboardState and carries information about a
     * key event or snapshot: the character code, logical key code, raw
     * platform keycode and a flags bitfield holding the pressed-down state and
     * the Shift/Control modifiers.
     *
     * @note Instances are plain data holders and are not thread-safe.
     */
    class WPCore_API KeyboardState : public IKeyboardState
    {
    public:
        /// Bit (1 << 0) of the flags bitfield: the key is held down.
        static const u32 isPressedDownFlag;
        /// Bit (1 << 1) of the flags bitfield: the Shift modifier is active.
        static const u32 isShiftPressedFlag;
        /// Bit (1 << 2) of the flags bitfield: the Control modifier is active.
        static const u32 isControlPressedFlag;

        /**
         * @brief Construct a new KeyboardState with default values.
         *
         * The character code, key codes and flags are all zero (key up, no modifiers).
         */
        KeyboardState();

        /**
         * @brief Virtual destructor.
         */
        ~KeyboardState() override;

        /**
         * @brief Get the character code produced by the key event.
         *
         * @return u32 Character code as stored by setChar() (default 0)
         */
        u32 getChar() const override;

        /**
         * @brief Set the character code for this keyboard state.
         *
         * @param character Character code produced by the key event. The value
         *                  is stored as is; no encoding is enforced.
         */
        void setChar( u32 character ) override;

        /**
         * @brief Get the logical (engine keymap) keycode for this event.
         *
         * @return u32 Logical key code (default 0)
         * @see isPressedDown( u32 )
         */
        u32 getKeyCode() const override;

        /**
         * @brief Set the logical keycode for this event.
         *
         * @param keyCode Logical key code
         */
        void setKeyCode( u32 keyCode ) override;

        /**
         * @brief Get the raw/platform-specific keycode.
         *
         * @return u32 Raw keycode provided by the OS or device (default 0)
         */
        u32 getRawKeyCode() const override;

        /**
         * @brief Set the raw/platform-specific keycode.
         *
         * @param rawKeyCode Raw platform keycode
         */
        void setRawKeyCode( u32 rawKeyCode ) override;

        /**
         * @brief Query whether the key of this state/event is held down.
         *
         * @return true if the pressed-down flag is set
         */
        bool isPressedDown() const override;

        /**
         * @brief Set or clear the pressed-down flag for this keyboard state.
         *
         * @param pressedDown True if the key is held down
         */
        void setPressedDown( bool pressedDown ) override;

        /**
         * @brief Query whether the Shift modifier is active for this event.
         *
         * @return true if the Shift flag is set
         */
        bool isShiftPressed() const override;

        /**
         * @brief Set or clear the Shift modifier flag.
         *
         * @param shiftPressed True to mark Shift as pressed
         */
        void setShiftPressed( bool shiftPressed ) override;

        /**
         * @brief Query whether the Control modifier is active for this event.
         *
         * @return true if the Control flag is set
         */
        bool isControlPressed() const override;

        /**
         * @brief Set or clear the Control modifier flag.
         *
         * @param controlPressed True to mark Control as pressed
         */
        void setControlPressed( bool controlPressed ) override;

        WP_CLASS_REGISTER_DECL;

        /**
         * @brief Check whether a specific key is held down in this state.
         *
         * Unlike isPressedDown(), this overload also requires that the stored
         * logical key code matches @p keycode.
         *
         * @param keycode Logical key code to test against getKeyCode()
         * @return true if @p keycode equals the stored key code and the
         *         pressed-down flag is set
         */
        bool isPressedDown( u32 keycode ) const override;

    protected:
        /// Character code associated with the key event.
        u32 m_char;

        /// Logical key code (engine keymap) for the event.
        u32 m_keyCode;

        /// Raw platform-specific keycode.
        u32 m_rawKeyCode;

        /// Flags bitfield storing the pressed-down and Shift/Control modifier bits.
        u32 m_flags = 0;
    };
}  // namespace workphone

#endif  // FBKeyInput_h__
