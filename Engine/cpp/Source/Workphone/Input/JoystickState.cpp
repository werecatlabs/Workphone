#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Input/JoystickState.hpp>
#include <Workphone/Core/BitUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, JoystickState, IJoystickState );

    const u32 JoystickState::isPressedDownFlag = ( 1 << 0 );
    const u32 JoystickState::isButtonPressedFlag = ( 1 << 1 );

    JoystickState::JoystickState() :
        m_joystick( 0 ),
        m_buttonId( 0 ),
        m_eventType( 0 ),
        m_pov( 0 ),
        m_flags( 0 )
    {
    }

    JoystickState::~JoystickState() = default;

    auto JoystickState::getJoystick() const -> u32
    {
        return m_joystick;
    }

    void JoystickState::setJoystick( u32 id )
    {
        m_joystick = id;
    }

    auto JoystickState::getPOV() const -> u16
    {
        return m_pov;
    }

    void JoystickState::setPOV( u32 position )
    {
        m_pov = position;
    }

    auto JoystickState::getAxis( u32 axisIndex ) const -> f32
    {
        const auto maxAxis = static_cast<u32>( Axis::NUMBER_OF_AXES );
        if( axisIndex >= maxAxis )
        {
            return 0.0f;
        }
        return m_axis[axisIndex];
    }

    void JoystickState::setAxis( u32 axisIndex, f32 position )
    {
        const auto maxAxis = static_cast<u32>( Axis::NUMBER_OF_AXES );
        if( axisIndex < maxAxis )
        {
            m_axis[axisIndex] = position;
        }
    }

    auto JoystickState::getButtonId() const -> u32
    {
        return m_buttonId;
    }

    void JoystickState::setButtonId( u32 buttonId )
    {
        m_buttonId = buttonId;
    }

    auto JoystickState::isPressedDown() const -> bool
    {
        return BitUtil::getFlagValue( m_flags, isPressedDownFlag );
    }

    void JoystickState::setPressedDown( bool pressed )
    {
        m_flags = BitUtil::setFlagValue( m_flags, isPressedDownFlag, pressed );
    }

    auto JoystickState::isButtonPressed( u32 id ) const -> bool
    {
        if( id >= static_cast<u32>( Axis::NUMBER_OF_BUTTONS ) )
        {
            return false;
        }
        return BitUtil::getFlagValue( m_buttonPressedFlags, 1u << id );
    }

    void JoystickState::setButtonPressed( u32 id, bool isPressed )
    {
        if( id >= static_cast<u32>( Axis::NUMBER_OF_BUTTONS ) )
        {
            return;
        }
        m_buttonPressedFlags = BitUtil::setFlagValue( m_buttonPressedFlags, 1u << id, isPressed );
    }

    auto JoystickState::getEventType() const -> u32
    {
        return m_eventType;
    }

    void JoystickState::setEventType( u32 eventType )
    {
        m_eventType = eventType;
    }
}  // namespace workphone
