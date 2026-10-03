#ifndef IJoystickState_h__
#define IJoystickState_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * Interface for a joystick state object.
     */
    class WPCore_API IJoystickState : public ISharedObject
    {
    public:
        /**
         * Enumeration of joystick axes.
         */
        enum class Axis
        {
            NUMBER_OF_BUTTONS = 32,

            AXIS_X = 0,  ///< e.g. analog stick 1 left to right
            AXIS_Y,      ///< e.g. analog stick 1 top to bottom
            AXIS_Z,      ///< e.g. throttle, or analog 2 stick 2 left to right
            AXIS_R,      ///< e.g. rudder, or analog 2 stick 2 top to bottom
            AXIS_U,
            AXIS_V,

            NUMBER_OF_AXES
        };

        /**
         * Enumeration of joystick event types.
         */
        enum class Type
        {
            PovMoved,
            Vector3Moved,
            ButtonPressed,
            ButtonReleased,
            SliderMoved,
            AxisMoved,

            Count
        };

        static const u32 POV_Centered = 0x00000000;
        static const u32 POV_North = 0x00000001;
        static const u32 POV_South = 0x00000010;
        static const u32 POV_East = 0x00000100;
        static const u32 POV_West = 0x00001000;
        static const u32 POV_NorthEast = 0x00000101;
        static const u32 POV_SouthEast = 0x00000110;
        static const u32 POV_NorthWest = 0x00001001;
        static const u32 POV_SouthWest = 0x00001010;

        /** Virtual destructor. */
        ~IJoystickState() override;

        /**
         * Gets the ID of the joystick.
         * @return The joystick ID.
         */
        virtual u32 getJoystick() const = 0;

        /**
         * Sets the ID of the joystick.
         * @param id The ID of the joystick.
         */
        virtual void setJoystick( u32 id ) = 0;

        /**
         * Gets the POV (point of view) direction.
         * @return The POV direction as a u16 value.
         */
        virtual u16 getPOV() const = 0;

        /**
         * Sets the POV direction.
         * @param position The POV direction as a u32 value.
         */
        virtual void setPOV( u32 position ) = 0;

        /**
         * Gets the position of a joystick axis.
         * @param axisIndex The index of the axis to get the position of.
         * @return The position of the axis as an f32 value.
         */
        virtual f32 getAxis( u32 axisIndex ) const = 0;

        /**
         * Sets the position of a joystick axis.
         * @param axisIndex The index of the axis to set the position of.
         * @param position The position to set the axis to as an f32 value.
         */
        virtual void setAxis( u32 axisIndex, f32 position ) = 0;

        /**
         * Gets the ID of a button.
         * @return The ID of the button.
         */
        virtual u32 getButtonId() const = 0;

        /**
         * Sets the ID of a button.
         * @param buttonId The ID of the button.
         */
        virtual void setButtonId( u32 buttonId ) = 0;

        /**
         * @brief Gets if the joystick is pressed down.
         * @return True if the joystick is pressed down, false otherwise.
         */
        virtual bool isPressedDown() const = 0;

        /**
         * @brief Sets if the joystick is pressed down.
         * @param pressed True to set the joystick as pressed down, false otherwise.
         */
        virtual void setPressedDown( bool pressed ) = 0;

        /**
         * @brief Sets if the joystick is pressed down.
         * @param pressed True to set the joystick as pressed down, false otherwise.
         */
        virtual bool isButtonPressed( u32 id ) const = 0;

        /**
         * @brief Sets if the joystick is pressed down.
         * @param id The ID of the button.
         * @param isPressed True to set the joystick as pressed down, false otherwise.
         */
        virtual void setButtonPressed( u32 id, bool isPressed ) = 0;

        /**
         * @brief Gets the event type.
         * @return The event type as a u32 value.
         */
        virtual u32 getEventType() const = 0;

        /**
         * @brief Sets the event type.
         * @param eventType The event type as a u32 value.
         */
        virtual void setEventType( u32 eventType ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IJoystickState_h__
