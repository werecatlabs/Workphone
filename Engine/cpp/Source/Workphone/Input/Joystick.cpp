#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Input/Joystick.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, Joystick, IJoystick );

    Joystick::Joystick() :
        m_numButtons( 0 ),
        m_numAxes( 0 ),
        m_flags( 0 ),
        m_deadZone( 0.1f ),
        m_sensitivity( 1.0f )
    {
        // Initialize button states
        for( auto &button : m_buttonStates )
        {
            button = false;
        }

        for( auto &button : m_prevButtonStates )
        {
            button = false;
        }

        // Initialize axis values
        for( auto &axis : m_axisValues )
        {
            axis = 0.0f;
        }
    }

    Joystick::~Joystick() = default;

    bool Joystick::isButtonDown( s32 button ) const
    {
        if( button < 0 || button >= MAX_BUTTONS )
            return false;

        return m_buttonStates[button].load();
    }

    bool Joystick::isButtonPressed( s32 button ) const
    {
        if( button < 0 || button >= MAX_BUTTONS )
            return false;

        return m_buttonStates[button].load() && !m_prevButtonStates[button].load();
    }

    bool Joystick::isButtonReleased( s32 button ) const
    {
        if( button < 0 || button >= MAX_BUTTONS )
            return false;

        return !m_buttonStates[button].load() && m_prevButtonStates[button].load();
    }

    f32 Joystick::getAxis( s32 axis ) const
    {
        if( axis < 0 || axis >= MAX_AXES )
            return 0.0f;

        f32 value = m_axisValues[axis];

        // Apply dead zone
        if( std::abs( value ) < m_deadZone )
        {
            return 0.0f;
        }

        // Apply sensitivity
        value *= m_sensitivity;

        // Apply inversion based on axis type
        bool shouldInvert = ( m_flags & INVERT_ALL ) != 0;

        switch( axis )
        {
        case 0:  // X axis
            shouldInvert = shouldInvert || ( ( m_flags & INVERT_X ) != 0 );
            break;
        case 1:  // Y axis
            shouldInvert = shouldInvert || ( ( m_flags & INVERT_Y ) != 0 );
            break;
        case 2:  // Z axis
            shouldInvert = shouldInvert || ( ( m_flags & INVERT_Z ) != 0 );
            break;
        case 3:  // RX axis
            shouldInvert = shouldInvert || ( ( m_flags & INVERT_RX ) != 0 );
            break;
        case 4:  // RY axis
            shouldInvert = shouldInvert || ( ( m_flags & INVERT_RY ) != 0 );
            break;
        case 5:  // RZ axis
            shouldInvert = shouldInvert || ( ( m_flags & INVERT_RZ ) != 0 );
            break;
        }

        if( shouldInvert )
        {
            value = -value;
        }

        // Clamp to [-1, 1] range
        return std::max( -1.0f, std::min( 1.0f, value ) );
    }

    s32 Joystick::getNumButtons() const
    {
        return m_numButtons.load();
    }

    s32 Joystick::getNumAxes() const
    {
        return m_numAxes.load();
    }

    String Joystick::getButtonName( s32 button ) const
    {
        if( button < 0 || button >= MAX_BUTTONS )
            return String();

        return StringUtil::toString( "Button " ) + StringUtil::toString( button );
    }

    String Joystick::getAxisName( s32 axis ) const
    {
        if( axis < 0 || axis >= MAX_AXES )
            return String();

        switch( axis )
        {
        case 0:
            return "X Axis";
        case 1:
            return "Y Axis";
        case 2:
            return "Z Axis";
        case 3:
            return "RX Axis";
        case 4:
            return "RY Axis";
        case 5:
            return "RZ Axis";
        default:
            return StringUtil::toString( "Axis " ) + StringUtil::toString( axis );
        }
    }

    void Joystick::setDeadZone( f32 deadZone )
    {
        m_deadZone = std::max( 0.0f, std::min( 1.0f, deadZone ) );
    }

    f32 Joystick::getDeadZone() const
    {
        return m_deadZone;
    }

    void Joystick::setSensitivity( f32 sensitivity )
    {
        m_sensitivity = std::max( 0.1f, std::min( 10.0f, sensitivity ) );
    }

    f32 Joystick::getSensitivity() const
    {
        return m_sensitivity;
    }

    void Joystick::setInvert( bool invert )
    {
        if( invert )
            m_flags |= INVERT_ALL;
        else
            m_flags &= ~INVERT_ALL;
    }

    bool Joystick::getInvert() const
    {
        return ( m_flags & INVERT_ALL ) != 0;
    }

    void Joystick::setInvertX( bool invert )
    {
        if( invert )
            m_flags |= INVERT_X;
        else
            m_flags &= ~INVERT_X;
    }

    bool Joystick::getInvertX() const
    {
        return ( m_flags & INVERT_X ) != 0;
    }

    void Joystick::setInvertY( bool invert )
    {
        if( invert )
            m_flags |= INVERT_Y;
        else
            m_flags &= ~INVERT_Y;
    }

    bool Joystick::getInvertY() const
    {
        return ( m_flags & INVERT_Y ) != 0;
    }

    void Joystick::setInvertZ( bool invert )
    {
        if( invert )
            m_flags |= INVERT_Z;
        else
            m_flags &= ~INVERT_Z;
    }

    bool Joystick::getInvertZ() const
    {
        return ( m_flags & INVERT_Z ) != 0;
    }

    void Joystick::setInvertRx( bool invert )
    {
        if( invert )
            m_flags |= INVERT_RX;
        else
            m_flags &= ~INVERT_RX;
    }

    bool Joystick::getInvertRx() const
    {
        return ( m_flags & INVERT_RX ) != 0;
    }

    void Joystick::setInvertRy( bool invert )
    {
        if( invert )
            m_flags |= INVERT_RY;
        else
            m_flags &= ~INVERT_RY;
    }

    bool Joystick::getInvertRy() const
    {
        return ( m_flags & INVERT_RY ) != 0;
    }

    void Joystick::setInvertRz( bool invert )
    {
        if( invert )
            m_flags |= INVERT_RZ;
        else
            m_flags &= ~INVERT_RZ;
    }

    bool Joystick::getInvertRz() const
    {
        return ( m_flags & INVERT_RZ ) != 0;
    }

    void Joystick::setInvertSlider( bool invert )
    {
        if( invert )
            m_flags |= INVERT_SLIDER;
        else
            m_flags &= ~INVERT_SLIDER;
    }

    bool Joystick::getInvertSlider() const
    {
        return ( m_flags & INVERT_SLIDER ) != 0;
    }

    void Joystick::setInvertDial( bool invert )
    {
        if( invert )
            m_flags |= INVERT_DIAL;
        else
            m_flags &= ~INVERT_DIAL;
    }

    bool Joystick::getInvertDial() const
    {
        return ( m_flags & INVERT_DIAL ) != 0;
    }

    void Joystick::setInvertWheel( bool invert )
    {
        if( invert )
            m_flags |= INVERT_WHEEL;
        else
            m_flags &= ~INVERT_WHEEL;
    }

    bool Joystick::getInvertWheel() const
    {
        return ( m_flags & INVERT_WHEEL ) != 0;
    }

    void Joystick::setInvertPOV( bool invert )
    {
        if( invert )
            m_flags |= INVERT_POV;
        else
            m_flags &= ~INVERT_POV;
    }

    void Joystick::updateButtonState( s32 button, bool pressed )
    {
        if( button >= 0 && button < MAX_BUTTONS )
        {
            m_prevButtonStates[button].store( m_buttonStates[button].load() );
            m_buttonStates[button].store( pressed );
        }
    }

    void Joystick::updateAxisValue( s32 axis, f32 value )
    {
        if( axis >= 0 && axis < MAX_AXES )
        {
            m_axisValues[axis] = value;
        }
    }

    void Joystick::setNumButtons( s32 numButtons )
    {
        m_numButtons = std::max( 0, std::min( MAX_BUTTONS, numButtons ) );
    }

    void Joystick::setNumAxes( s32 numAxes )
    {
        m_numAxes = std::max( 0, std::min( MAX_AXES, numAxes ) );
    }

}  // namespace workphone
