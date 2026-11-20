#ifndef FBMouseInput_h__
#define FBMouseInput_h__

#include <Workphone/Interface/Input/IMouseState.hpp>

/**
 * @file MouseState.hpp
 * @brief Concrete mouse input state holding positions, button/modifier flags, wheel and drag data.
 */

namespace workphone
{
    /**
     * @brief Concrete implementation of the IMouseState interface for mouse input state management.
     *
     * This class provides a plain in-memory representation of the mouse state used by the
     * input subsystem. It stores cursor positions (relative and absolute), movement and wheel
     * deltas, the drag displacement, a high-level event type, and a single flags bitfield that
     * holds button, modifier, dragging and double-click state. Instances are lightweight and
     * are intended to be passed from input producers to consumers.
     *
     * @note Instances are not thread-safe; synchronize externally if accessed from multiple threads.
     * @note All setters store their value as is; no validation or clamping is performed.
     *
     * @see IMouseState
     */
    class WPCore_API MouseState : public IMouseState
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes all vectors to zero, clears all flags (no buttons pressed, no modifiers,
         * not dragging, no double click), and sets the event type to `Event::Count` (no event).
         */
        MouseState();

        /**
         * @brief Virtual destructor.
         *
         * Ensures correct polymorphic destruction when referenced through `IMouseState`.
         */
        ~MouseState() override;

        /**
         * @brief Retrieves the stored mouse movement delta.
         *
         * The delta is the change in cursor position as recorded by the input producer, commonly
         * used for relative motion (camera rotation, look controls, etc.).
         *
         * @return Movement delta in the same coordinate space as `getRelativePosition()`
         *         (default `(0, 0)`).
         */
        Vector2<real_Num> getDelta() const override;

        /**
         * @brief Stores the mouse movement delta.
         *
         * @param movePosition Delta movement to store (X, Y), in the same units as `getDelta()`.
         */
        void setDelta( const Vector2<real_Num> &movePosition ) override;

        /**
         * @brief Returns the stored mouse position relative to the active window or viewport.
         *
         * The exact coordinate space (pixels or normalized, and its origin) is determined by the
         * input provider that fills this state; this class does not interpret it.
         *
         * @return Relative position (default `(0, 0)`).
         */
        Vector2<real_Num> getRelativePosition() const override;

        /**
         * @brief Sets the mouse relative position.
         *
         * @param position New relative position (X, Y), in whatever coordinate space the input
         *                 provider uses.
         */
        void setRelativePosition( const Vector2<real_Num> &position ) override;

        /**
         * @brief Returns the stored absolute mouse position in global/display coordinates.
         *
         * Absolute coordinates typically reference screen or desktop space (pixels), as supplied
         * by the input provider.
         *
         * @return Absolute position (default `(0, 0)`).
         */
        Vector2<real_Num> getAbsolutePosition() const override;

        /**
         * @brief Sets the absolute mouse position.
         *
         * @param position Absolute position (X, Y), typically in screen pixels.
         */
        void setAbsolutePosition( const Vector2<real_Num> &position ) override;

        /**
         * @brief Retrieves the stored wheel motion.
         *
         * The X component is used for horizontal scrolling (if supported) and the Y component for
         * vertical scrolling. The sign convention is defined by the input provider.
         *
         * @return Wheel delta (default `(0, 0)`).
         */
        Vector2<real_Num> getWheelDelta() const override;

        /**
         * @brief Sets the wheel delta value.
         *
         * @param wheelDelta Wheel motion to store (X horizontal, Y vertical).
         */
        void setWheelDelta( const Vector2<real_Num> &wheelDelta ) override;

        /**
         * @brief Returns whether the Shift modifier flag (`MOUSE_SHIFT`) is set.
         *
         * @return true if Shift is pressed; false otherwise.
         */
        bool isShiftPressed() const override;

        /**
         * @brief Sets or clears the Shift modifier flag (`MOUSE_SHIFT`).
         *
         * @param shiftPressed true to mark Shift as pressed; false to mark as released.
         */
        void setShiftPressed( bool shiftPressed ) override;

        /**
         * @brief Returns whether the Control modifier flag (`MOUSE_CONTROL`) is set.
         *
         * @return true if Control is pressed; false otherwise.
         */
        bool isControlPressed() const override;

        /**
         * @brief Sets or clears the Control modifier flag (`MOUSE_CONTROL`).
         *
         * @param controlPressed true to mark Control as pressed; false to mark as released.
         */
        void setControlPressed( bool controlPressed ) override;

        /**
         * @brief Tests whether any of the flag bits in `id` is set in the state flags.
         *
         * Pass an `IMouseState::MOUSE_*` constant such as `MOUSE_LEFT`, `MOUSE_RIGHT` or
         * `MOUSE_MIDDLE`. The test is a bitwise "any bit" test against the shared flags word, so
         * the other `MOUSE_*` constants (modifiers, dragging, double click, released) can be
         * queried the same way, and combining several constants returns true if any is set.
         *
         * @param id Button/flag bit mask to test.
         * @return true if at least one bit of `id` is set; false otherwise.
         */
        bool isButtonPressed( u32 id ) const override;

        /**
         * @brief Sets or clears the flag bits of a mouse button in the state flags.
         *
         * @param id      Button/flag bit mask (see the `IMouseState::MOUSE_*` constants); all
         *                bits in the mask are set or cleared together.
         * @param pressed true to set the bits (pressed); false to clear them (released).
         *
         * @note Setting `MOUSE_LEFT` does not automatically clear `MOUSE_LEFT_RELEASED` or vice
         *       versa; callers manage both bits.
         */
        void setButtonPressed( u32 id, bool pressed ) override;

        /**
         * @brief Returns the high-level mouse event type stored in this state.
         *
         * This value describes the kind of event that produced the current state (click,
         * move, wheel, etc.). `Event::Count` indicates no specific event.
         *
         * @return Current event type (default `Event::Count`).
         */
        Event getEventType() const override;

        /**
         * @brief Sets the high-level mouse event type for the current state.
         *
         * @param eventType Event enum value to store.
         */
        void setEventType( Event eventType ) override;

        /**
         * @brief Returns the stored drag displacement.
         *
         * This value is set by the input producer and represents how far the cursor has moved
         * while a drag operation was active; this class does not accumulate it.
         *
         * @return Drag displacement (default `(0, 0)`).
         */
        Vector2<real_Num> getDragValue() const override;

        /**
         * @brief Sets the drag displacement.
         *
         * @param dragValue Total displacement to store (X, Y).
         */
        void setDragValue( const Vector2<real_Num> &dragValue ) override;

        /**
         * @brief Returns whether the dragging flag (`MOUSE_DRAGGING`) is set.
         *
         * @return true if dragging; false otherwise.
         */
        bool isDragging() const override;

        /**
         * @brief Sets or clears the dragging flag (`MOUSE_DRAGGING`).
         *
         * @param dragging true to mark dragging active; false to mark it inactive.
         */
        void setDragging( bool dragging ) override;

        /**
         * @brief Returns whether the double-click flag (`MOUSE_DOUBLE_CLICK`) is set.
         *
         * @return true if the click is flagged as a double click; false otherwise.
         */
        bool isDoubleClick() const override;

        /**
         * @brief Sets or clears the double-click flag (`MOUSE_DOUBLE_CLICK`).
         *
         * @param doubleClick true to flag the click as a double click; false otherwise.
         */
        void setDoubleClick( bool doubleClick ) override;

        /// Registers the class with the Workphone RTTI/class registration system.
        WP_CLASS_REGISTER_DECL;

    protected:
        /** @brief Drag displacement as set by setDragValue(). */
        Vector2<real_Num> m_dragValue;

        /** @brief Movement delta as set by setDelta(). */
        Vector2<real_Num> m_movePosition;

        /** @brief Mouse position relative to the window/viewport, in the provider's coordinate space. */
        Vector2<real_Num> m_relativePosition;

        /** @brief Absolute mouse position in screen/display coordinates. */
        Vector2<real_Num> m_absolutePosition;

        /** @brief Wheel delta (X horizontal, Y vertical). */
        Vector2<real_Num> m_wheelPosition;

        /** @brief Current mouse event type (default: Event::Count = no event). */
        Event m_eventType = Event::Count;

        /**
         * @brief Bit flags that store button states, modifier keys and dragging/double-click state.
         *
         * Use the `IMouseState::MOUSE_*` constants to interpret individual bits.
         */
        u32 m_flags = 0;
    };
}  // namespace workphone

#endif  // FBMouseInput_h__
