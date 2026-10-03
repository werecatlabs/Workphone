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
     * platform keycode and modifier flags (shift/control). Flags are exposed
     * via convenience accessors for common modifier and press/hold queries.
     */
    class WPCore_API KeyboardState : public IKeyboardState
    {
    public:
        /// Flag indicating a key is currently held down.
        static const u32 isPressedDownFlag;
        /// Flag indicating Shift modifier is active.
        static const u32 isShiftPressedFlag;
        /// Flag indicating Control modifier is active.
        static const u32 isControlPressedFlag;

        /**
         * @brief Construct a new KeyboardState with default values.
         */
        KeyboardState();

        /**
         * @brief Virtual destructor.
         */
        ~KeyboardState() override;

        /**
         * @brief Get the Unicode/character code produced by the key event.
         *
         * @return u32 Character code (Unicode code point)
         */
        u32 getChar() const override;

        /**
         * @brief Set the character code for this keyboard state.
         *
         * @param character Unicode code point produced by the key event
         */
        void setChar( u32 character ) override;

        /**
         * @brief Get the logical/keymap keycode for this event.
         *
         * @return u32 Logical key code
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
         * @return u32 Raw keycode provided by the OS or device
         */
        u32 getRawKeyCode() const override;

        /**
         * @brief Set the raw/platform-specific keycode.
         *
         * @param rawKeyCode Raw platform keycode
         */
        void setRawKeyCode( u32 rawKeyCode ) override;

        /**
         * @brief Query whether the key is currently held down.
         *
         * @return true if the key is pressed down
         */
        bool isPressedDown() const override;

        /**
         * @brief Set the pressed-down flag for this keyboard state.
         *
         * @param pressedDown True if key is held down
         */
        void setPressedDown( bool pressedDown ) override;

        /**
         * @brief Query whether Shift modifier is active for this event.
         *
         * @return true if Shift is pressed
         */
        bool isShiftPressed() const override;

        /**
         * @brief Set or clear the Shift modifier flag.
         *
         * @param shiftPressed True to mark Shift as pressed
         */
        void setShiftPressed( bool shiftPressed ) override;

        /**
         * @brief Query whether Control modifier is active for this event.
         *
         * @return true if Control is pressed
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
         * @brief Check whether the provided keycode is currently pressed in
         *        this state.
         *
         * This overload allows testing by keycode rather than the stored
         * key/char values.
         */
        bool isPressedDown( u32 keycode ) const override;

    protected:
        /// Unicode/character code associated with the key event.
        u32 m_char;

        /// Logical key code (engine keymap) for the event.
        u32 m_keyCode;

        /// Raw platform-specific keycode.
        u32 m_rawKeyCode;

        /// Flags bitfield storing pressed and modifier states.
        u32 m_flags = 0;
    };
}  // namespace workphone

#endif  // FBKeyInput_h__
