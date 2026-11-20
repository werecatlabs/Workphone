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
     * states (current and previous sample) and axis values. It provides
     * configurable parameters such as dead zone, sensitivity, and per-axis
     * inversion flags. The helper update* and setNum* methods are intended to
     * be called by the platform input system to feed in new device data.
     *
     * @note Button states, axis values, button/axis counts, dead zone and
     *       sensitivity are stored in atomics and may be read while the input
     *       thread updates them. The inversion flags (m_flags) are a plain
     *       bitfield and are not synchronized.
     * @note Out-of-range button or axis indices are never an error: queries
     *       return false, 0 or an empty string and updates are ignored.
     */
    class WPCore_API Joystick : public IJoystick
    {
    public:
        /**
         * @brief Construct a new Joystick object.
         *
         * All buttons are released, all axes are zero, no buttons or axes are
         * reported, no inversion is set, the dead zone is 0.1 and the
         * sensitivity is 1.0.
         */
        Joystick();

        /**
         * @brief Virtual destructor.
         */
        ~Joystick() override;

        /**
         * @brief Check if the specified button is currently held down.
         *
         * @param button Button index to check, in the range [0, MAX_BUTTONS)
         * @return true if down; false if up or if the index is out of range
         */
        bool isButtonDown( s32 button ) const override;

        /**
         * @brief Check if the specified button was pressed since its previous update.
         *
         * Returns true only if the latest updateButtonState() call for this
         * button was a transition from not-pressed to pressed. The edge is
         * cleared by the next updateButtonState() call for the same button.
         *
         * @param button Button index to check, in the range [0, MAX_BUTTONS)
         * @return true on the not-pressed to pressed transition; false
         *         otherwise or if the index is out of range
         */
        bool isButtonPressed( s32 button ) const override;

        /**
         * @brief Check if the specified button was released since its previous update.
         *
         * Returns true only if the latest updateButtonState() call for this
         * button was a transition from pressed to not-pressed. The edge is
         * cleared by the next updateButtonState() call for the same button.
         *
         * @param button Button index to check, in the range [0, MAX_BUTTONS)
         * @return true on the pressed to not-pressed transition; false
         *         otherwise or if the index is out of range
         */
        bool isButtonReleased( s32 button ) const override;

        /**
         * @brief Get the processed value of the requested axis.
         *
         * The stored raw value is processed in this order: a raw magnitude
         * below the dead zone yields 0; the value is multiplied by the
         * sensitivity; the value is negated if inversion applies (the global
         * invert flag or the per-axis flag for axes 0-5: X, Y, Z, Rx, Ry, Rz);
         * finally the result is clamped to [-1, 1].
         *
         * @param axis Axis index to query, in the range [0, MAX_AXES)
         * @return Processed axis value in [-1, 1], or 0 if the index is out of range
         * @see getRawAxis()
         */
        f32 getAxis( s32 axis ) const override;

        /**
         * @brief Get the stored axis value before dead zone, sensitivity or inversion.
         *
         * @note This non-virtual helper is not part of the IJoystick interface.
         *
         * @param axis Axis index to query, in the range [0, MAX_AXES)
         * @return Value last stored by updateAxisValue(), or 0 if the index is out of range
         * @see getAxis()
         */
        f32 getRawAxis( s32 axis ) const;

        /**
         * @brief Get the number of buttons reported for this joystick.
         *
         * @return s32 Number of buttons, as set by setNumButtons()
         */
        s32 getNumButtons() const override;

        /**
         * @brief Get the number of axes reported for this joystick.
         *
         * @return s32 Number of axes, as set by setNumAxes()
         */
        s32 getNumAxes() const override;

        /**
         * @brief Retrieve a human-readable name for the given button index.
         *
         * @param button Button index, in the range [0, MAX_BUTTONS)
         * @return String of the form "Button N", or an empty string if the
         *         index is out of range
         */
        String getButtonName( s32 button ) const override;

        /**
         * @brief Retrieve a human-readable name for the given axis index.
         *
         * @param axis Axis index, in the range [0, MAX_AXES)
         * @return "X Axis", "Y Axis", "Z Axis", "RX Axis", "RY Axis" or
         *         "RZ Axis" for indices 0-5, "Axis N" for the remaining
         *         valid indices, or an empty string if the index is out of range
         */
        String getAxisName( s32 axis ) const override;

        /**
         * @brief Set the dead zone for axis input.
         *
         * Raw axis values whose magnitude is below the dead zone are reported
         * as zero by getAxis() to prevent drift. The remaining range is not
         * rescaled.
         *
         * @param deadZone Dead zone magnitude; clamped to [0, 1]
         */
        void setDeadZone( f32 deadZone ) override;

        /**
         * @brief Get the current dead zone value.
         *
         * @return f32 Dead zone magnitude in [0, 1] (default 0.1)
         */
        f32 getDeadZone() const override;

        /**
         * @brief Set the axis sensitivity multiplier.
         *
         * This scales the axis value after the dead zone test. The scaled
         * result is still clamped to [-1, 1] by getAxis().
         *
         * @param sensitivity Sensitivity multiplier; clamped to [0.1, 10]
         */
        void setSensitivity( f32 sensitivity ) override;

        /**
         * @brief Get the current axis sensitivity multiplier.
         *
         * @return f32 Sensitivity multiplier in [0.1, 10] (default 1.0)
         */
        f32 getSensitivity() const override;

        /**
         * @brief Set the global inversion flag (INVERT_ALL).
         *
         * When set, every axis is inverted by getAxis() regardless of its
         * individual flag. Clearing it does not change the per-axis flags.
         *
         * @param invert True to invert all axes
         */
        void setInvert( bool invert ) override;

        /**
         * @brief Query the global inversion flag (INVERT_ALL).
         *
         * @return true if the global flag is set
         */
        bool getInvert() const override;

        /**
         * @brief Set the inversion flag for the X axis (axis 0, INVERT_X).
         *
         * @param invert True to invert the axis
         */
        void setInvertX( bool invert ) override;

        /**
         * @brief Query the inversion flag of the X axis (axis 0).
         *
         * @return true if INVERT_X is set
         */
        bool getInvertX() const override;

        /**
         * @brief Set the inversion flag for the Y axis (axis 1, INVERT_Y).
         *
         * @param invert True to invert the axis
         */
        void setInvertY( bool invert ) override;

        /**
         * @brief Query the inversion flag of the Y axis (axis 1).
         *
         * @return true if INVERT_Y is set
         */
        bool getInvertY() const override;

        /**
         * @brief Set the inversion flag for the Z axis (axis 2, INVERT_Z).
         *
         * @param invert True to invert the axis
         */
        void setInvertZ( bool invert ) override;

        /**
         * @brief Query the inversion flag of the Z axis (axis 2).
         *
         * @return true if INVERT_Z is set
         */
        bool getInvertZ() const override;

        /**
         * @brief Set the inversion flag for the Rx axis (axis 3, INVERT_RX).
         *
         * @param invert True to invert the axis
         */
        void setInvertRx( bool invert ) override;

        /**
         * @brief Query the inversion flag of the Rx axis (axis 3).
         *
         * @return true if INVERT_RX is set
         */
        bool getInvertRx() const override;

        /**
         * @brief Set the inversion flag for the Ry axis (axis 4, INVERT_RY).
         *
         * @param invert True to invert the axis
         */
        void setInvertRy( bool invert ) override;

        /**
         * @brief Query the inversion flag of the Ry axis (axis 4).
         *
         * @return true if INVERT_RY is set
         */
        bool getInvertRy() const override;

        /**
         * @brief Set the inversion flag for the Rz axis (axis 5, INVERT_RZ).
         *
         * @param invert True to invert the axis
         */
        void setInvertRz( bool invert ) override;

        /**
         * @brief Query the inversion flag of the Rz axis (axis 5).
         *
         * @return true if INVERT_RZ is set
         */
        bool getInvertRz() const override;

        /**
         * @brief Set the inversion flag for sliders (INVERT_SLIDER).
         *
         * @note The flag is only stored; getAxis() does not consult it.
         *
         * @param invert True to invert sliders
         */
        void setInvertSlider( bool invert ) override;

        /**
         * @brief Query the inversion flag for sliders.
         *
         * @return true if INVERT_SLIDER is set
         */
        bool getInvertSlider() const override;

        /**
         * @brief Set the inversion flag for dials (INVERT_DIAL).
         *
         * @note The flag is only stored; getAxis() does not consult it.
         *
         * @param invert True to invert dials
         */
        void setInvertDial( bool invert ) override;

        /**
         * @brief Query the inversion flag for dials.
         *
         * @return true if INVERT_DIAL is set
         */
        bool getInvertDial() const override;

        /**
         * @brief Set the inversion flag for wheels (INVERT_WHEEL).
         *
         * @note The flag is only stored; getAxis() does not consult it.
         *
         * @param invert True to invert wheels
         */
        void setInvertWheel( bool invert ) override;

        /**
         * @brief Query the inversion flag for wheels.
         *
         * @return true if INVERT_WHEEL is set
         */
        bool getInvertWheel() const override;

        /**
         * @brief Set the inversion flag for POV hats (INVERT_POV).
         *
         * @note The flag is only stored; getAxis() does not consult it, and
         *       there is no corresponding getter.
         *
         * @param invert True to invert POV hats
         */
        void setInvertPOV( bool invert ) override;

        /**
         * @brief Record a new sampled state for a button.
         *
         * The current state is copied to the previous state before the new
         * state is stored, which is what enables isButtonPressed() and
         * isButtonReleased(). Call it once per button per input sample;
         * calling it twice with the same value clears the edge.
         *
         * @param button Button index, in the range [0, MAX_BUTTONS); ignored if out of range
         * @param pressed True if the button is currently down
         */
        void updateButtonState( s32 button, bool pressed );

        /**
         * @brief Store the raw value of an axis.
         *
         * The value is stored unmodified; dead zone, sensitivity and inversion
         * are applied when it is read through getAxis().
         *
         * @param axis Axis index, in the range [0, MAX_AXES); ignored if out of range
         * @param value Normalized raw axis value, typically in [-1, 1]
         */
        void updateAxisValue( s32 axis, f32 value );

        /**
         * @brief Set the number of buttons reported by the device.
         *
         * @param numButtons Button count; clamped to [0, MAX_BUTTONS]
         */
        void setNumButtons( s32 numButtons );

        /**
         * @brief Set the number of axes reported by the device.
         *
         * @param numAxes Axis count; clamped to [0, MAX_AXES]
         */
        void setNumAxes( s32 numAxes );

        WP_CLASS_REGISTER_DECL;

    protected:
        /// Number of buttons currently reported by the device (atomic).
        atomic_u32 m_numButtons;
        /// Number of axes currently reported by the device (atomic).
        atomic_u32 m_numAxes;
        /// Inversion flags, a bitfield of the IJoystick::JoystickFlags values (not atomic).
        u32 m_flags = 0;

        /// Dead zone applied to raw axis values to prevent drift, in [0, 1].
        atomic_f32 m_deadZone;
        /// Axis sensitivity multiplier, in [0.1, 10].
        atomic_f32 m_sensitivity;

        /// Current button states, indexed by button.
        FixedArray<atomic_bool, MAX_BUTTONS> m_buttonStates;
        /// Button states before the most recent update of each button, used for edge detection.
        FixedArray<atomic_bool, MAX_BUTTONS> m_prevButtonStates;

        /// Raw axis values, indexed by axis.
        FixedArray<atomic_f32, MAX_AXES> m_axisValues;
    };

}  // namespace workphone

#endif  // Joystick_h__
