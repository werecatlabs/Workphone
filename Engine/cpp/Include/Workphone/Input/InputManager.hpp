#ifndef InputManager_h__
#define InputManager_h__

#include <Workphone/Interface/Input/IInputManager.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Map.hpp>

/**
 * @file InputManager.hpp
 * @brief High-level input manager that aggregates input events and state.
 */

namespace workphone
{
    /**
     * @brief Central manager for input devices and events.
     *
     * InputManager implements IInputManager and stores a list of input events
     * as well as current/previous button bitfields. It exposes helpers to
     * query and set axis values, control cursor visibility, and manage simple
     * playback/recording states.
     */
    class WPCore_API InputManager : public IInputManager
    {
    public:
        /**
         * @brief Construct a new InputManager instance.
         */
        InputManager();

        /**
         * @brief Virtual destructor.
         */
        ~InputManager() override;

        /**
         * @brief Query whether the OS cursor is currently visible.
         *
         * @return true if the cursor is visible
         */
        bool isCursorVisible() const override;

        /**
         * @brief Show or hide the OS cursor.
         *
         * @param visible True to show the cursor, false to hide it
         */
        void setCursorVisible( bool visible ) override;

        /**
         * @brief Set a normalized axis value (processed) for the given axis.
         *
         * @param axis Axis index
         * @param value Processed axis value (typically [-1,1])
         */
        void setAxisValue( s32 axis, f32 value ) override;

        /**
         * @brief Get the processed axis value for the given axis.
         *
         * @param axis Axis index
         * @return f32 Processed axis value
         */
        f32 getAxisValue( s32 axis ) override;

        /**
         * @brief Get the raw axis value (before processing) for the given
         *        axis.
         *
         * @param axis Axis index
         * @return f32 Raw axis value
         */
        f32 getAxisValueRaw( s32 axis ) override;

        /**
         * @brief Start playback of recorded input events.
         */
        void play() override;

        /**
         * @brief Start recording input events.
         */
        void record() override;

        /**
         * @brief Stop playback or recording.
         */
        void stop() override;

        /**
         * @brief Get internal manager flags bitfield.
         *
         * @return u32 Flags value
         */
        u32 getFlags() const override;

        /**
         * @brief Set internal manager flags bitfield.
         *
         * @param flags Flags to set
         */
        void setFlags( u32 flags ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /// Stored input events queued or recorded by the manager.
        Array<SmartPtr<IInputEvent>> m_inputEvents;

        /// Bitfield of buttons from the previous update (atomic for thread-safety).
        atomic_u32 m_previousButtons;
        /// Bitfield of buttons for the current update (atomic for thread-safety).
        atomic_u32 m_currentButtons;

        /// Stored processed axis values keyed by axis identifier.
        Map<s32, f32> m_axisValues;
        /// Stored raw axis values keyed by axis identifier.
        Map<s32, f32> m_axisValuesRaw;

        /// Internal flags (bitfield) controlling manager behaviour.
        u32 m_flags = 0;
    };
}  // namespace workphone

#endif  // InputManager_h__
