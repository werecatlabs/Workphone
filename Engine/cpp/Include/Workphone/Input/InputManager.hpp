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
     * @class InputManager
     * @brief Simple in-memory IInputManager implementation holding axis values and manager flags.
     *
     * Axis values are stored per axis id and returned as set; unknown axes read as 0. The cursor
     * visibility is only recorded in the flag bitmask (IInputManager::cursorVisibleFlag) and does
     * not touch the OS cursor. play(), record() and stop() are currently no-ops, and the stored
     * event list and button bitfields are reserved and not used by this implementation.
     */
    class WPCore_API InputManager : public IInputManager
    {
    public:
        /**
         * @brief Construct a new InputManager instance.
         *
         * All flags and axis values start cleared, so the cursor flag is initially not set.
         */
        InputManager();

        /**
         * @brief Virtual destructor.
         */
        ~InputManager() override;

        /**
         * @brief Query the cursor visibility flag.
         *
         * @return true if IInputManager::cursorVisibleFlag is set in the flags.
         */
        bool isCursorVisible() const override;

        /**
         * @brief Set or clear IInputManager::cursorVisibleFlag.
         *
         * @note Only the flag is changed; the OS cursor is not touched.
         *
         * @param visible True to set the flag, false to clear it.
         */
        void setCursorVisible( bool visible ) override;

        /**
         * @brief Set the value of an axis.
         *
         * The same value is stored as both the processed and the raw value.
         *
         * @param axis Axis identifier.
         * @param value Axis value, typically in [-1, 1]; stored unmodified.
         */
        void setAxisValue( s32 axis, f32 value ) override;

        /**
         * @brief Get the processed value of an axis.
         *
         * @param axis Axis identifier.
         * @return Last value set for the axis, or 0 if it was never set.
         */
        f32 getAxisValue( s32 axis ) override;

        /**
         * @brief Get the raw value of an axis.
         *
         * @param axis Axis identifier.
         * @return Last value set for the axis, or 0 if it was never set.
         */
        f32 getAxisValueRaw( s32 axis ) override;

        /**
         * @brief Start playback of recorded input events.
         *
         * @note Currently a no-op.
         */
        void play() override;

        /**
         * @brief Start recording input events.
         *
         * @note Currently a no-op.
         */
        void record() override;

        /**
         * @brief Stop playback or recording.
         *
         * @note Currently a no-op.
         */
        void stop() override;

        /**
         * @brief Get the flags bitmask.
         *
         * @return Combination of the IInputManager flag constants.
         */
        u32 getFlags() const override;

        /**
         * @brief Replace the flags bitmask.
         *
         * @note This overwrites all flags, including IInputManager::cursorVisibleFlag.
         *
         * @param flags New combination of IInputManager flag constants.
         */
        void setFlags( u32 flags ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /// Reserved list of input events; not used by this implementation.
        Array<SmartPtr<IInputEvent>> m_inputEvents;

        /// Reserved button bitfield of the previous update; not used by this implementation.
        atomic_u32 m_previousButtons;
        /// Reserved button bitfield of the current update; not used by this implementation.
        atomic_u32 m_currentButtons;

        /// Processed axis values keyed by axis identifier.
        Map<s32, f32> m_axisValues;
        /// Raw axis values keyed by axis identifier.
        Map<s32, f32> m_axisValuesRaw;

        /// Flags bitmask built from the IInputManager flag constants.
        u32 m_flags = 0;
    };
}  // namespace workphone

#endif  // InputManager_h__
