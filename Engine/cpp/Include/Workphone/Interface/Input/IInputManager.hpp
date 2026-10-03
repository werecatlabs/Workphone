#ifndef IInput_h__
#define IInput_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @file IInputManager.hpp
     * @brief Interface for a platform-agnostic input manager.
     *
     * This interface provides accessors and control for cursor visibility,
     * axis input values, and input recording/playback flags. Implementations
     * should translate device-specific input into the normalized API used
     * here.
     */

    /**
     * @brief Interface for input management.
     *
     * Implementations of `IInputManager` are responsible for:
     * - tracking cursor visibility,
     * - providing axis values (processed and raw),
     * - controlling input recording/playback,
     * - exposing and mutating input-related flags.
     *
     * The axis methods use integer axis identifiers (platform or project specific).
     */
    class WPCore_API IInputManager : public ISharedObject
    {
    public:
        /** @name Input flags
         *  Bitmask flags used by the input manager. Combine using bitwise OR.
         */
        ///@{
        /** @brief Flag set while the input system is currently assigning bindings. */
        static const u32 isAssigningFlag;
        /** @brief Flag set when the shift modifier is currently pressed. */
        static const u32 isShiftPressedFlag;
        /** @brief Flag indicating override input behavior is enabled. */
        static const u32 useOverrideFlag;
        /** @brief Enable logging of input events. */
        static const u32 enableInputLogFlag;
        /** @brief Run (playback) input from the input log. */
        static const u32 runInputLogFlag;
        /** @brief Buffer keyboard input. */
        static const u32 bufferedKeysFlag;
        /** @brief Buffer mouse input. */
        static const u32 bufferedMouseFlag;
        /** @brief Input capture is currently active (e.g. window has captured mouse). */
        static const u32 inputCaptureFlag;
        /** @brief Left mouse button is pressed. */
        static const u32 isLeftPressedFlag;
        /** @brief Right mouse button is pressed. */
        static const u32 isRightPressedFlag;
        /** @brief Middle mouse button is pressed. */
        static const u32 isMiddlePressedFlag;
        /** @brief Cursor visibility flag. When set, cursor should be visible. */
        static const u32 cursorVisibleFlag;
        ///@}

        /** @brief Virtual destructor. */
        ~IInputManager() override;

        /**
         * @brief Query whether the cursor is currently visible.
         * @return True if the cursor is visible; false otherwise.
         */
        virtual bool isCursorVisible() const = 0;

        /**
         * @brief Set cursor visibility.
         * @param visible True to show the cursor, false to hide it.
         *
         * Implementations should apply changes immediately where possible.
         */
        virtual void setCursorVisible( bool visible ) = 0;

        /**
         * @brief Manually sets a normalized axis value.
         * @param axis Integer identifier for the axis (project-specific).
         * @param value Normalized axis value, typically in the range [-1.0, 1.0].
         *
         * Use to inject or override an axis value (for simulated input, remapping, etc.).
         */
        virtual void setAxisValue( s32 axis, f32 value ) = 0;

        /**
         * @brief Get the processed axis value.
         * @param axis Integer identifier for the axis.
         * @return Axis value in the range [-1.0, 1.0] after any processing
         *         (deadzone, smoothing, sensitivity) performed by the manager.
         */
        virtual f32 getAxisValue( s32 axis ) = 0;

        /**
         * @brief Get the raw axis value from the underlying device.
         * @param axis Integer identifier for the axis.
         * @return Raw axis value as provided by the device; may be outside
         *         the normalized range and typically bypasses smoothing/deadzone.
         */
        virtual f32 getAxisValueRaw( s32 axis ) = 0;

        /**
         * @brief Start playback of a previously recorded input log.
         *
         * Implementations should switch to playback mode and feed input
         * events from the log into the input system.
         */
        virtual void play() = 0;

        /**
         * @brief Start recording input events to a log.
         *
         * Implementations should capture subsequent input events and write
         * them to the configured input log/store.
         */
        virtual void record() = 0;

        /**
         * @brief Stop any current recording or playback.
         *
         * This should cleanly end playback or recording and restore normal input behavior.
         */
        virtual void stop() = 0;

        /**
         * @brief Retrieve the current flags bitmask.
         * @return Bitmask composed of the static flag constants defined above.
         */
        virtual u32 getFlags() const = 0;

        /**
         * @brief Set the flags bitmask.
         * @param flags New flags bitmask. Implementations should store this
         *              and apply behavior changes controlled by the flags.
         */
        virtual void setFlags( u32 flags ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IInputManager_h__
