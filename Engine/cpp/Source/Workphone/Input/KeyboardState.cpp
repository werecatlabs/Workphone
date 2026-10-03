#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Input/KeyboardState.hpp>
#include <Workphone/Core/BitUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, KeyboardState, IKeyboardState );

    const u32 KeyboardState::isPressedDownFlag = ( 1 << 0 );
    const u32 KeyboardState::isShiftPressedFlag = ( 1 << 1 );
    const u32 KeyboardState::isControlPressedFlag = ( 1 << 2 );

    KeyboardState::KeyboardState() : m_char( 0 ), m_keyCode( 0 ), m_rawKeyCode( 0 )
    {
    }

    KeyboardState::~KeyboardState() = default;

    auto KeyboardState::getChar() const -> u32
    {
        return m_char;
    }

    void KeyboardState::setChar( u32 c )
    {
        m_char = c;
    }

    auto KeyboardState::getKeyCode() const -> u32
    {
        return m_keyCode;
    }

    void KeyboardState::setKeyCode( u32 keyCode )
    {
        m_keyCode = keyCode;
    }

    auto KeyboardState::getRawKeyCode() const -> u32
    {
        return m_rawKeyCode;
    }

    void KeyboardState::setRawKeyCode( u32 rawKeyCode )
    {
        m_rawKeyCode = rawKeyCode;
    }

    auto KeyboardState::isPressedDown() const -> bool
    {
        return BitUtil::getFlagValue( m_flags, isPressedDownFlag );
    }

    bool KeyboardState::isPressedDown( u32 keycode ) const
    {
        return keycode == m_keyCode && isPressedDown();
    }

    void KeyboardState::setPressedDown( bool pressedDown )
    {
        m_flags = BitUtil::setFlagValue( m_flags, isPressedDownFlag, pressedDown );
    }

    auto KeyboardState::isShiftPressed() const -> bool
    {
        return BitUtil::getFlagValue( m_flags, isShiftPressedFlag );
    }

    void KeyboardState::setShiftPressed( bool shiftPressed )
    {
        m_flags = BitUtil::setFlagValue( m_flags, isShiftPressedFlag, shiftPressed );
    }

    auto KeyboardState::isControlPressed() const -> bool
    {
        return BitUtil::getFlagValue( m_flags, isControlPressedFlag );
    }

    void KeyboardState::setControlPressed( bool controlPressed )
    {
        m_flags = BitUtil::setFlagValue( m_flags, isControlPressedFlag, controlPressed );
    }
}  // namespace workphone
