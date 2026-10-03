#ifndef IMouseState_h__
#define IMouseState_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{

    /**
     * @brief Interface that represents the current state of a pointing device (mouse).
     *
     * This interface exposes methods to query and modify the mouse state tracked by the
     * input subsystem: button states, cursor positions (absolute and relative), movement
     * deltas, wheel motion, modifier keys and high-level event type produced by the input
     * system. Implementations are expected to be lightweight shared objects (see
     * `ISharedObject`) used to transport mouse state between input producers and
     * consumers.
     *
     * Thread-safety: callers should assume the implementation is not inherently thread
     * safe unless documented by a concrete implementation.
     */
    class WPCore_API IMouseState : public ISharedObject
    {
    public:
        /// Identifier for left mouse button.
        static const u32 MOUSE_LEFT;
        /// Identifier for right mouse button.
        static const u32 MOUSE_RIGHT;
        /// Identifier for middle mouse button.
        static const u32 MOUSE_MIDDLE;
        /// Identifier for left mouse button released state.
        static const u32 MOUSE_LEFT_RELEASED;
        /// Identifier for right mouse button released state.
        static const u32 MOUSE_RIGHT_RELEASED;
        /// Identifier for middle mouse button released state.
        static const u32 MOUSE_MIDDLE_RELEASED;
        /// Identifier for shift modifier.
        static const u32 MOUSE_SHIFT;
        /// Identifier for control modifier.
        static const u32 MOUSE_CONTROL;
        /// Identifier for dragging state.
        static const u32 MOUSE_DRAGGING;
        /// Identifier for double-click state.
        static const u32 MOUSE_DOUBLE_CLICK;

        /**
         * @brief High-level mouse input events produced by the input system.
         *
         * Use these values to describe the most recent event associated with this
         * mouse state instance. `Count` is a sentinel for the number of event kinds.
         */
        enum class Event : u8
        {
            /// Left mouse button pressed.
            LeftPressed = 0,

            /// Right mouse button pressed.
            RightPressed,

            /// Middle mouse button pressed.
            MiddlePressed,

            /// Left mouse button released.
            LeftReleased,

            /// Right mouse button released.
            RightReleased,

            /// Middle mouse button released.
            MiddleReleased,

            /// Cursor moved. Use `getDelta()` / `getAbsolutePosition()` for details.
            Moved,

            /**
             * Mouse wheel moved. Use `getWheelDelta()` to determine direction and
             * amount. Positive/negative sign convention is implementation dependent
             * (documented by the concrete implementation).
             */
            Wheel,

            /// Number of distinct events in this enumeration (sentinel).
            Count
        };

        /**
         * @brief Virtual destructor.
         *
         * Ensure correct polymorphic destruction for derived implementations.
         */
        ~IMouseState() override;

        /**
         * @brief Retrieves the cursor movement delta since last update.
         *
         * The delta is typically the difference between the current and previous
         * cursor positions in the same coordinate space as `getRelativePosition()`.
         *
         * @return Movement delta as `Vector2<real_Num>`.
         */
        virtual Vector2<real_Num> getDelta() const = 0;

        /**
         * @brief Sets the cursor movement delta.
         *
         * @param move Movement delta to store.
         */
        virtual void setDelta( const Vector2<real_Num> &move ) = 0;

        /**
         * @brief Gets the mouse position relative to the active window or viewport.
         *
         * Relative coordinates are typically in pixels (or UI units) measured from the
         * top-left of the window/viewport. Exact coordinate system depends on the
         * concrete input provider.
         *
         * @return Relative position as `Vector2<real_Num>`.
         */
        virtual Vector2<real_Num> getRelativePosition() const = 0;

        /**
         * @brief Sets the mouse relative position.
         *
         * @param position New relative position to store.
         */
        virtual void setRelativePosition( const Vector2<real_Num> &position ) = 0;

        /**
         * @brief Gets the mouse absolute position in global/system coordinates.
         *
         * Absolute position is generally in screen or OS desktop coordinates. Concrete
         * implementations should document the exact meaning.
         *
         * @return Absolute position as `Vector2<real_Num>`.
         */
        virtual Vector2<real_Num> getAbsolutePosition() const = 0;

        /**
         * @brief Sets the mouse absolute position.
         *
         * @param position New absolute position to store.
         */
        virtual void setAbsolutePosition( const Vector2<real_Num> &position ) = 0;

        /**
         * @brief Retrieves wheel motion since last update.
         *
         * Wheel delta is commonly a 1D value (y-axis) for vertical scroll; a 2D
         * vector is used to allow horizontal wheel data where supported.
         *
         * @return Wheel delta as `Vector2<real_Num>`.
         */
        virtual Vector2<real_Num> getWheelDelta() const = 0;

        /**
         * @brief Sets the wheel delta.
         *
         * @param wheelDelta Wheel motion to store.
         */
        virtual void setWheelDelta( const Vector2<real_Num> &wheelDelta ) = 0;

        /**
         * @brief Returns whether the Shift modifier is currently held.
         *
         * @return True if Shift is pressed; false otherwise.
         */
        virtual bool isShiftPressed() const = 0;

        /**
         * @brief Sets the Shift modifier state.
         *
         * @param shiftPressed True to mark Shift as pressed; false otherwise.
         */
        virtual void setShiftPressed( bool shiftPressed ) = 0;

        /**
         * @brief Returns whether the Control modifier is currently held.
         *
         * @return True if Control is pressed; false otherwise.
         */
        virtual bool isControlPressed() const = 0;

        /**
         * @brief Sets the Control modifier state.
         *
         * @param controlPressed True to mark Control as pressed; false otherwise.
         */
        virtual void setControlPressed( bool controlPressed ) = 0;

        /**
         * @brief Queries the pressed state of a mouse button by identifier.
         *
         * Use the `MOUSE_*` constants to identify buttons.
         *
         * @param id Button identifier (e.g. `MOUSE_LEFT`).
         * @return True if the specified button is currently pressed; false otherwise.
         */
        virtual bool isButtonPressed( u32 id ) const = 0;

        /**
         * @brief Sets the pressed/released state for a mouse button.
         *
         * @param id Button identifier (e.g. `MOUSE_LEFT`).
         * @param pressed True to mark the button as pressed; false to mark released.
         */
        virtual void setButtonPressed( u32 id, bool pressed ) = 0;

        /**
         * @brief Gets the high-level event type most recently generated for this state.
         *
         * @return Event enum value describing the recent mouse event.
         */
        virtual Event getEventType() const = 0;

        /**
         * @brief Sets the high-level event type associated with this state.
         *
         * @param eventType Event enum value to store.
         */
        virtual void setEventType( Event eventType ) = 0;

        /**
         * @brief Returns the accumulated drag value.
         *
         * Drag value is typically the total movement since drag began and can be used
         * by UI components to measure how far the user has dragged.
         *
         * @return Drag value as `Vector2<real_Num>`.
         */
        virtual Vector2<real_Num> getDragValue() const = 0;

        /**
         * @brief Sets the accumulated drag value.
         *
         * @param dragValue Drag vector to store.
         */
        virtual void setDragValue( const Vector2<real_Num> &dragValue ) = 0;

        /**
         * @brief Returns whether a drag operation is currently active.
         *
         * @return True if dragging; false otherwise.
         */
        virtual bool isDragging() const = 0;

        /**
         * @brief Sets the dragging state.
         *
         * @param dragging True to mark dragging active; false to mark it inactive.
         */
        virtual void setDragging( bool dragging ) = 0;

        /**
         * @brief Queries whether the last click was a double click.
         *
         * @return True if a double-click was detected; false otherwise.
         */
        virtual bool isDoubleClick() const = 0;

        /**
         * @brief Marks whether the last click was a double click.
         *
         * @param doubleClick True to mark last click as double click; false otherwise.
         */
        virtual void setDoubleClick( bool doubleClick ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IMouseState_h__
