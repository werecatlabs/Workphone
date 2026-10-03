#ifndef WPOISJoystick_h__
#define WPOISJoystick_h__

#include <WPOISInput/WPOISInputPrerequisites.hpp>
#include <Workphone/Input/Joystick.hpp>

namespace workphone
{
    class WPOISInput_API OISJoystick : public Joystick
    {
    public:
        OISJoystick();
        ~OISJoystick() override;

        bool isButtonDown( s32 button ) const override;

        bool isButtonPressed( s32 button ) const override;

        bool isButtonReleased( s32 button ) const override;

        f32 getAxis( s32 axis ) const override;

        RawPtr<OIS::JoyStick> getJoystick() const;

        void setJoystick( RawPtr<OIS::JoyStick> joystick );

        WP_CLASS_REGISTER_DECL;

    protected:
        RawPtr<OIS::JoyStick> m_joystick;
    };
}  // namespace workphone

#endif  // WPOISJoystick_h__
