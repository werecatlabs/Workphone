#include <WPOISInput/WPOISInput.hpp>
#include <WPOISInput/WPOISJoystick.hpp>
#include <Workphone/Workphone.hpp>
#include <OIS.h>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, OISJoystick, Joystick );

    OISJoystick::OISJoystick() = default;

    OISJoystick::~OISJoystick() = default;

    bool OISJoystick::isButtonDown( s32 button ) const
    {
        if( m_joystick )
        {
            const auto &state = m_joystick->getJoyStickState();
            return state.mButtons[button];
        }

        return false;
    }

    bool OISJoystick::isButtonPressed( s32 button ) const
    {
        if( m_joystick )
        {
            const auto &state = m_joystick->getJoyStickState();
            return state.mButtons[button];
        }

        return false;
    }

    bool OISJoystick::isButtonReleased( s32 button ) const
    {
        if( m_joystick )
        {
            const auto &state = m_joystick->getJoyStickState();
            return !state.mButtons[button];
        }

        return false;
    }

    f32 OISJoystick::getAxis( s32 axis ) const
    {
        if( m_joystick )
        {
            const auto &state = m_joystick->getJoyStickState();
            return state.mAxes[axis].abs;
        }

        return 0.0f;
    }

    RawPtr<OIS::JoyStick> OISJoystick::getJoystick() const
    {
        return m_joystick;
    }

    void OISJoystick::setJoystick( RawPtr<OIS::JoyStick> joystick )
    {
        m_joystick = joystick;

        if( m_joystick )
        {
            auto name = String( m_joystick->vendor().c_str() );
            setName( name );

            m_numButtons = m_joystick->getNumberOfComponents( OIS::OIS_Button );
            m_numAxes = m_joystick->getNumberOfComponents( OIS::OIS_Axis );
        }
    }
}  // namespace workphone
