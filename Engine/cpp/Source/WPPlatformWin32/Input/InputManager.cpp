#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Input/InputManager.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, InputManager, IInputManager );

    InputManager::InputManager() = default;

    InputManager::~InputManager() = default;

    bool InputManager::isCursorVisible() const
    {
        return ( m_flags & cursorVisibleFlag ) != 0;
    }

    void InputManager::setCursorVisible( bool visible )
    {
        if( visible )
        {
            m_flags |= cursorVisibleFlag;
        }
        else
        {
            m_flags &= ~cursorVisibleFlag;
        }
    }

    void InputManager::setAxisValue( s32 axis, f32 value )
    {
        m_axisValues[axis] = value;
        m_axisValuesRaw[axis] = value;
    }

    f32 InputManager::getAxisValue( s32 axis )
    {
        auto it = m_axisValues.find( axis );
        if( it != m_axisValues.end() )
        {
            return it->second;
        }
        return 0.0f;
    }

    f32 InputManager::getAxisValueRaw( s32 axis )
    {
        auto it = m_axisValuesRaw.find( axis );
        if( it != m_axisValuesRaw.end() )
        {
            return it->second;
        }
        return 0.0f;
    }

    void InputManager::play()
    {
    }

    void InputManager::record()
    {
    }

    void InputManager::stop()
    {
    }

    u32 InputManager::getFlags() const
    {
        return m_flags;
    }

    void InputManager::setFlags( u32 flags )
    {
        m_flags = flags;
    }

}  // namespace workphone
