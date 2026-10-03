#ifndef IJoystick_h__
#define IJoystick_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    /** Joystick interface. Provides functionality for
     * a joystick device.
     */
    class WPCore_API IJoystick : public ISharedObject
    {
    public:
        // Bit flags for inversion settings
        enum JoystickFlags : u32
        {
            INVERT_ALL = ( 1 << 0 ),
            INVERT_X = ( 1 << 1 ),
            INVERT_Y = ( 1 << 2 ),
            INVERT_Z = ( 1 << 3 ),
            INVERT_RX = ( 1 << 4 ),
            INVERT_RY = ( 1 << 5 ),
            INVERT_RZ = ( 1 << 6 ),
            INVERT_SLIDER = ( 1 << 7 ),
            INVERT_DIAL = ( 1 << 8 ),
            INVERT_WHEEL = ( 1 << 9 ),
            INVERT_POV = ( 1 << 10 )
        };

        static constexpr s32 MAX_BUTTONS = 32;
        static constexpr s32 MAX_AXES = 8;
        static constexpr s32 MAX_POVS = 4;
        static constexpr s32 MAX_SLIDERS = 2;
        static constexpr s32 MAX_DIALS = 2;
        static constexpr s32 MAX_WHEELS = 2;
        static constexpr s32 MAX_JOYSTICKS = 4;

        /** Destructor.
         */
        ~IJoystick() override;

        /** Gets if a button is down.
         * @param button The button to check.
         * @return True if the button is down, false otherwise.
         */
        virtual bool isButtonDown( s32 button ) const = 0;

        /** Gets if a button is pressed.
         * @param button The button to check.
         * @return True if the button is pressed, false otherwise.
         */
        virtual bool isButtonPressed( s32 button ) const = 0;

        /** Gets if a button is released.
         * @param button The button to check.
         * @return True if the button is released, false otherwise.
         */
        virtual bool isButtonReleased( s32 button ) const = 0;

        /** Gets the value of an axis.
         * @param axis The axis to get the value of.
         * @return The value of the axis.
         */
        virtual f32 getAxis( s32 axis ) const = 0;

        /** Gets the number of buttons on the joystick.
         * @return The number of buttons on the joystick
         */
        virtual s32 getNumButtons() const = 0;

        /** Gets the number of axes on the joystick.
         * @return The number of axes.
         */
        virtual s32 getNumAxes() const = 0;

        /** Gets the button name.
         * @param button The button to get the name of.
         */
        virtual String getButtonName( s32 button ) const = 0;

        /** Gets the axis name.
         * @param axis The axis to get the name of.
         */
        virtual String getAxisName( s32 axis ) const = 0;

        /** Sets the dead zone for the joystick. The dead zone is the
         * area around the center of the joystick where no input is registered.
         * @param deadZone The dead zone value.
         */
        virtual void setDeadZone( f32 deadZone ) = 0;

        /** Gets the dead zone for the joystick. The dead zone is the
         * area around the center of the joystick where no input is registered.
         * @return The dead zone value.
         */
        virtual f32 getDeadZone() const = 0;

        /** Sets the sensitivity for the joystick. The sensitivity is the
         * amount of movement required to register input.
         * @param sensitivity The sensitivity value.
         */
        virtual void setSensitivity( f32 sensitivity ) = 0;

        /** Gets the sensitivity for the joystick. The sensitivity is the
         * amount of movement required to register input.
         * @return The sensitivity value.
         */
        virtual f32 getSensitivity() const = 0;

        /** Sets the axis inversion for the joystick. The axis inversion is
         * used to invert the direction of the joystick movement.
         * @param invert True to invert the axis, false otherwise.
         */
        virtual void setInvert( bool invert ) = 0;

        /** Gets the axis inversion for the joystick. The axis inversion is
         * used to invert the direction of the joystick movement.
         * @return True if the axis is inverted, false otherwise.
         */
        virtual bool getInvert() const = 0;

        /** Sets the axis inversion for the joystick. The axis inversion is
         * used to invert the direction of the joystick movement.
         * @param invert True to invert the axis, false otherwise.
         */
        virtual void setInvertX( bool invert ) = 0;

        /** Gets the axis inversion for the joystick. The axis inversion is
         * used to invert the direction of the joystick movement.
         * @return True if the axis is inverted, false otherwise.
         */
        virtual bool getInvertX() const = 0;

        /** Sets the axis inversion for the joystick. The axis inversion is
         * used to invert the direction of the joystick movement.
         * @param invert True to invert the axis, false otherwise.
         */
        virtual void setInvertY( bool invert ) = 0;

        /** Gets the axis inversion for the joystick. The axis inversion is
         * used to invert the direction of the joystick movement.
         * @return True if the axis is inverted, false otherwise.
         */
        virtual bool getInvertY() const = 0;

        /** Sets the axis inversion for the joystick. The axis inversion is
         * used to invert the direction of the joystick movement.
         * @param invert True to invert the axis, false otherwise.
         */
        virtual void setInvertZ( bool invert ) = 0;

        /** Gets the axis inversion for the joystick. The axis inversion is
         * used to invert the direction of the joystick movement.
         * @return True if the axis is inverted, false otherwise.
         */
        virtual bool getInvertZ() const = 0;

        /** Sets the axis inversion for the joystick. The axis inversion is
         * used to invert the direction of the joystick movement.
         * @param invert True to invert the axis, false otherwise.
         */
        virtual void setInvertRx( bool invert ) = 0;

        /** Gets the axis inversion for the joystick. The axis inversion is
         * used to invert the direction of the joystick movement.
         * @return True if the axis is inverted, false otherwise.
         */
        virtual bool getInvertRx() const = 0;

        /** Sets the axis inversion for the joystick. The axis inversion is
         * used to invert the direction of the joystick movement.
         * @param invert True to invert the axis, false otherwise.
         */
        virtual void setInvertRy( bool invert ) = 0;

        /** Gets the axis inversion for the joystick. The axis inversion is
         * used to invert the direction of the joystick movement.
         * @return True if the axis is inverted, false otherwise.
         */
        virtual bool getInvertRy() const = 0;

        /** Sets the axis inversion for the joystick. The axis inversion is
         * used to invert the direction of the joystick movement.
         * @param invert True to invert the axis, false otherwise.
         */
        virtual void setInvertRz( bool invert ) = 0;

        /** Gets the axis inversion for the joystick. The axis inversion is
         * used to invert the direction of the joystick movement.
         * @return True if the axis is inverted, false otherwise.
         */
        virtual bool getInvertRz() const = 0;

        /** Sets the axis inversion for the joystick. The axis inversion is
         * used to invert the direction of the joystick movement.
         * @param invert True to invert the axis, false otherwise.
         */
        virtual void setInvertSlider( bool invert ) = 0;

        /** Gets if the slider is inverted. The slider inversion is used to invert the
         * direction of the slider movement.
         * @return True if the slider is inverted, false otherwise.
         */
        virtual bool getInvertSlider() const = 0;

        /** Sets if the dial is inverted. The dial inversion is used to invert the
         * direction of the dial movement.
         * @param invert True to invert the dial, false otherwise.
         */
        virtual void setInvertDial( bool invert ) = 0;

        /** Gets if the dial is inverted. The dial inversion is used to invert the
         * direction of the dial movement.
         * @return True if the dial is inverted, false otherwise.
         */
        virtual bool getInvertDial() const = 0;

        /** Sets if the wheel is inverted. The wheel inversion is used to invert the
         * direction of the wheel movement.
         * @param invert True to invert the wheel, false otherwise.
         */
        virtual void setInvertWheel( bool invert ) = 0;

        /** Gets if the wheel is inverted. The wheel inversion is used to invert the
         * direction of the wheel movement.
         * @return True if the wheel is inverted, false otherwise.
         */
        virtual bool getInvertWheel() const = 0;

        /** Sets the axis inversion for the joystick. The axis inversion is
         * used to invert the direction of the joystick movement.
         * @param invert True to invert the axis, false otherwise.
         */
        virtual void setInvertPOV( bool invert ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IJoystick_h__
