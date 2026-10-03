#ifndef FBMouseInput_h__
#define FBMouseInput_h__

#include <Workphone/Interface/Input/IMouseState.hpp>

namespace workphone
{
    /**
     * @brief Concrete implementation of the IMouseState interface for mouse input state management.
     *
     * This class provides a full, in-memory representation of the mouse device state used by the
     * input subsystem. It stores cursor positions (relative and absolute), per-button states,
     * wheel motion, modifier key states and drag-related metrics. Instances are intended to be
     * lightweight and are suitable for being passed between input producers and consumers.
     *
     * @note Implementations are not guaranteed to be thread-safe; synchronize externally if accessed
     *       from multiple threads.
     *
     * @see IMouseState
     */
    class WPCore_API MouseState : public IMouseState
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes all numeric positions to zero, clears all flags (no buttons pressed,
         * no modifiers, not dragging), and sets the event type to `Event::Count` (no event).
         */
        MouseState();

        /**
         * @brief Virtual destructor.
         *
         * Ensures correct polymorphic destruction when referenced through `IMouseState`.
         */
        ~MouseState() override;

        /**
         * @brief Retrieves the mouse movement delta since the last update.
         *
         * The delta represents the change in cursor position and is commonly used for
         * relative motion (camera rotation, look controls, etc.).
         *
         * @return Movement delta as `Vector2<real_Num>` in the same coordinate space as
         *         `getRelativePosition()` (typically pixels or UI units).
         */
        Vector2<real_Num> getDelta() const override;

        /**
         * @brief Stores the mouse movement delta.
         *
         * @param movePosition Delta movement to store (X, Y). Units are the same as returned
         *                     by `getDelta()` (typically pixels).
         */
        void setDelta( const Vector2<real_Num> &movePosition ) override;

        /**
         * @brief Returns the mouse position relative to the active window or viewport.
         *
         * Relative coordinates are typically normalized to the window or viewport. The exact
         * coordinate space is implementation-dependent; callers should consult the concrete input
         * provider if required.
         *
         * @return Relative position as `Vector2<real_Num>` (commonly normalized or pixel-based).
         */
        Vector2<real_Num> getRelativePosition() const override;

        /**
         * @brief Sets the mouse relative position.
         *
         * @param position New relative position (X, Y). If normalized, values are expected
         *                 in the range [0.0, 1.0]; otherwise values are in pixels.
         */
        void setRelativePosition( const Vector2<real_Num> &position ) override;

        /**
         * @brief Returns the absolute mouse position in global/display coordinates.
         *
         * Absolute coordinates typically reference screen or desktop space (pixels).
         *
         * @return Absolute position as `Vector2<real_Num>` (pixels).
         */
        Vector2<real_Num> getAbsolutePosition() const override;

        /**
         * @brief Sets the absolute mouse position.
         *
         * @param position Absolute position (X, Y) in pixels.
         */
        void setAbsolutePosition( const Vector2<real_Num> &position ) override;

        /**
         * @brief Retrieves the wheel motion accumulated since the last update.
         *
         * The X component is used for horizontal scrolling (if supported) and the Y component for
         * vertical scrolling. Positive/negative sign convention is implementation-dependent.
         *
         * @return Wheel delta as `Vector2<real_Num>`.
         */
        Vector2<real_Num> getWheelDelta() const override;

        /**
         * @brief Sets the wheel delta value.
         *
         * @param wheelDelta Wheel motion to store (X horizontal, Y vertical).
         */
        void setWheelDelta( const Vector2<real_Num> &wheelDelta ) override;

        /**
         * @brief Returns whether the Shift modifier is currently held.
         *
         * @return true if Shift is pressed; false otherwise.
         */
        bool isShiftPressed() const override;

        /**
         * @brief Sets the Shift modifier state.
         *
         * @param shiftPressed true to mark Shift as pressed; false to mark as released.
         */
        void setShiftPressed( bool shiftPressed ) override;

        /**
         * @brief Returns whether the Control modifier is currently held.
         *
         * @return true if Control is pressed; false otherwise.
         */
        bool isControlPressed() const override;

        /**
         * @brief Sets the Control modifier state.
         *
         * @param controlPressed true to mark Control as pressed; false to mark as released.
         */
        void setControlPressed( bool controlPressed ) override;

        /**
         * @brief Tests whether a mouse button identified by `id` is currently pressed.
         *
         * Use `IMouseState::MOUSE_LEFT`, `MOUSE_RIGHT`, `MOUSE_MIDDLE` or other defined flags.
         *
         * @param id Button identifier to test.
         * @return true if the requested button is pressed; false otherwise.
         */
        bool isButtonPressed( u32 id ) const override;

        /**
         * @brief Sets the pressed state for a specific mouse button.
         *
         * @param id      Button identifier (see `IMouseState`).
         * @param pressed true to mark the button pressed; false to mark released.
         */
        void setButtonPressed( u32 id, bool pressed ) override;

        /**
         * @brief Returns the most recent high-level mouse event type.
         *
         * This value describes the kind of event that produced the current state (click,
         * move, wheel, etc.). `Event::Count` indicates no specific event.
         *
         * @return Current event type as `Event`.
         */
        Event getEventType() const override;

        /**
         * @brief Sets the high-level mouse event type for the current state.
         *
         * @param eventType Event enum value to store.
         */
        void setEventType( Event eventType ) override;

        /**
         * @brief Returns the accumulated drag displacement since the drag started.
         *
         * This value represents how far the cursor has moved while a drag operation was active.
         *
         * @return Drag displacement as `Vector2<real_Num>` (pixels).
         */
        Vector2<real_Num> getDragValue() const override;

        /**
         * @brief Sets the accumulated drag displacement.
         *
         * @param dragValue Total displacement to store (X, Y) in pixels.
         */
        void setDragValue( const Vector2<real_Num> &dragValue ) override;

        /**
         * @brief Returns whether a drag operation is currently active.
         *
         * @return true if dragging; false otherwise.
         */
        bool isDragging() const override;

        /**
         * @brief Sets the dragging state.
         *
         * @param dragging true to mark dragging active; false to mark it inactive.
         */
        void setDragging( bool dragging ) override;

        /**
         * @brief Returns whether the last click was detected as a double click.
         *
         * @return true if last click is a double click; false otherwise.
         */
        bool isDoubleClick() const override;

        /**
         * @brief Marks whether the last click was a double click.
         *
         * @param doubleClick true to mark last click as a double click; false otherwise.
         */
        void setDoubleClick( bool doubleClick ) override;

        /// Class registration declaration for the WorkPhone reflection/registration system.
        WP_CLASS_REGISTER_DECL;

    protected:
        /** @brief Accumulated drag displacement since drag began (pixels). */
        Vector2<real_Num> m_dragValue;

        /** @brief Delta movement since last update (pixels). */
        Vector2<real_Num> m_movePosition;

        /** @brief Mouse position relative to the window/viewport (normalized or pixel units). */
        Vector2<real_Num> m_relativePosition;

        /** @brief Absolute mouse position in screen/display coordinates (pixels). */
        Vector2<real_Num> m_absolutePosition;

        /** @brief Wheel delta (X horizontal, Y vertical). */
        Vector2<real_Num> m_wheelPosition;

        /** @brief Last recorded mouse event type (default: Event::Count = no event). */
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
