#ifndef Joystick_h__
#define Joystick_h__

#include <Workphone/Interface/Input/IJoystick.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/FixedArray.hpp>

/**
 * @file Joystick.hpp
 * @brief Concrete joystick implementation exposing button and axis state.
 */

namespace workphone
{

    /**
     * @brief Joystick class implementing the IJoystick interface.
     *
     * This class stores and exposes the state of a physical joystick: button
     * states (current and previous frame) and axis values. It provides
     * configurable parameters such as dead zone, sensitivity, and per-axis
     * inversion flags. The helper update* methods are intended to be called
     * by the input system to mutate state in a thread-safe manner.
     */
    class WPCore_API Joystick : public IJoystick
    {
    public:
        /**
         * @brief Construct a new Joystick object.
         */
        Joystick();

        /**
         * @brief Virtual destructor.
         */
        ~Joystick() override;

        /**
         * @brief Check if the specified button is currently held down.
         *
         * @param button Button index to check
         * @return true if down, false otherwise
         */
        bool isButtonDown( s32 button ) const override;

        /**
         * @brief Check if the specified button was pressed this frame.
         *
         * Returns true only on the transition from not-pressed to pressed.
         *
         * @param button Button index to check
         * @return true if pressed this frame
         */
        bool isButtonPressed( s32 button ) const override;

        /**
         * @brief Check if the specified button was released this frame.
         *
         * Returns true only on the transition from pressed to not-pressed.
         *
         * @param button Button index to check
         * @return true if released this frame
         */
        bool isButtonReleased( s32 button ) const override;

        /**
         * @brief Get the current value of the requested axis.
         *
         * Axis values are typically normalized in the range [-1, 1], after
         * dead zone and sensitivity adjustments are applied.
         *
         * @param axis Axis index to query
         * @return f32 Axis value
         */
        f32 getAxis( s32 axis ) const override;

        /**
         * @brief Get the number of buttons reported for this joystick.
         *
         * @return s32 Number of buttons
         */
        s32 getNumButtons() const override;

        /**
         * @brief Get the number of axes reported for this joystick.
         *
         * @return s32 Number of axes
         */
        s32 getNumAxes() const override;

        /**
         * @brief Retrieve a human-readable name for the given button index.
         *
         * @param button Button index
         * @return String Button name
         */
        String getButtonName( s32 button ) const override;

        /**
         * @brief Retrieve a human-readable name for the given axis index.
         *
         * @param axis Axis index
         * @return String Axis name
         */
        String getAxisName( s32 axis ) const override;

        /**
         * @brief Set the dead zone for axis input.
         *
         * Values within the dead zone are treated as zero to prevent drift.
         *
         * @param deadZone Dead zone magnitude (typically [0,1])
         */
        void setDeadZone( f32 deadZone ) override;

        /**
         * @brief Get the current dead zone value.
         *
         * @return f32 Dead zone magnitude
         */
        f32 getDeadZone() const override;

        /**
         * @brief Set the axis sensitivity multiplier.
         *
         * This scales axis input after dead zone has been applied.
         *
         * @param sensitivity Sensitivity multiplier
         */
        void setSensitivity( f32 sensitivity ) override;

        /**
         * @brief Get the current axis sensitivity multiplier.
         *
         * @return f32 Sensitivity
         */
        f32 getSensitivity() const override;

        /**
         * @brief Set inversion for all axes at once.
         *
         * @param invert True to invert all axes
         */
        void setInvert( bool invert ) override;

        /**
         * @brief Query whether all axes are inverted.
         *
         * @return true if inverted
         */
        bool getInvert() const override;

        void setInvertX( bool invert ) override;
        bool getInvertX() const override;
        void setInvertY( bool invert ) override;
        bool getInvertY() const override;
        void setInvertZ( bool invert ) override;
        bool getInvertZ() const override;
        void setInvertRx( bool invert ) override;
        bool getInvertRx() const override;
        void setInvertRy( bool invert ) override;
        bool getInvertRy() const override;
        void setInvertRz( bool invert ) override;
        bool getInvertRz() const override;
        void setInvertSlider( bool invert ) override;
        bool getInvertSlider() const override;
        void setInvertDial( bool invert ) override;
        bool getInvertDial() const override;
        void setInvertWheel( bool invert ) override;
        bool getInvertWheel() const override;
        void setInvertPOV( bool invert ) override;

        /**
         * @brief Helper methods for updating joystick state.
         *
         * These methods are intended to be called by the platform input
         * system to mutate the joystick's state in a thread-safe manner.
         */
        void updateButtonState( s32 button, bool pressed );
        void updateAxisValue( s32 axis, f32 value );
        void setNumButtons( s32 numButtons );
        void setNumAxes( s32 numAxes );

        WP_CLASS_REGISTER_DECL;

    protected:
        /// Number of buttons currently reported by the device (atomic).
        atomic_u32 m_numButtons;
        /// Number of axes currently reported by the device (atomic).
        atomic_u32 m_numAxes;
        /// Flags controlling per-device options (bitfield).
        u32 m_flags = 0;

        // Joystick configuration
        /// Dead zone applied to axis values to prevent drift.
        atomic_f32 m_deadZone;
        /// Axis sensitivity multiplier.
        atomic_f32 m_sensitivity;

        // Button states: current and previous frame for edge detection
        FixedArray<atomic_bool, MAX_BUTTONS> m_buttonStates;
        FixedArray<atomic_bool, MAX_BUTTONS> m_prevButtonStates;

        // Current axis values for each axis index
        FixedArray<atomic_f32, MAX_AXES> m_axisValues;
    };

}  // namespace workphone

#endif  // Joystick_h__
