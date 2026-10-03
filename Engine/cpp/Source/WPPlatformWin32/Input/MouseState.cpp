#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Input/MouseState.hpp>
#include <Workphone/Core/BitUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, MouseState, IMouseState );

    MouseState::MouseState() = default;

    MouseState::~MouseState() = default;

    Vector2<real_Num> MouseState::getDelta() const
    {
        return m_movePosition;
    }

    void MouseState::setDelta( const Vector2<real_Num> &movePosition )
    {
        m_movePosition = movePosition;
    }

    Vector2<real_Num> MouseState::getRelativePosition() const
    {
        return m_relativePosition;
    }

    void MouseState::setRelativePosition( const Vector2<real_Num> &position )
    {
        m_relativePosition = position;
    }

    Vector2<real_Num> MouseState::getAbsolutePosition() const
    {
        return m_absolutePosition;
    }

    void MouseState::setAbsolutePosition( const Vector2<real_Num> &position )
    {
        m_absolutePosition = position;
    }

    Vector2<real_Num> MouseState::getWheelDelta() const
    {
        return m_wheelPosition;
    }

    void MouseState::setWheelDelta( const Vector2<real_Num> &wheelDelta )
    {
        m_wheelPosition = wheelDelta;
    }

    bool MouseState::isShiftPressed() const
    {
        return BitUtil::getFlagValue( m_flags, MOUSE_SHIFT );
    }

    void MouseState::setShiftPressed( bool shiftPressed )
    {
        m_flags = BitUtil::setFlagValue( m_flags, MOUSE_SHIFT, shiftPressed );
    }

    bool MouseState::isControlPressed() const
    {
        return BitUtil::getFlagValue( m_flags, MOUSE_CONTROL );
    }

    void MouseState::setControlPressed( bool controlPressed )
    {
        m_flags = BitUtil::setFlagValue( m_flags, MOUSE_CONTROL, controlPressed );
    }

    bool MouseState::isButtonPressed( u32 id ) const
    {
        return BitUtil::getFlagValue( m_flags, id );
    }

    void MouseState::setButtonPressed( u32 id, bool pressed )
    {
        m_flags = BitUtil::setFlagValue( m_flags, id, pressed );
    }

    IMouseState::Event MouseState::getEventType() const
    {
        return m_eventType;
    }

    void MouseState::setEventType( Event eventType )
    {
        m_eventType = eventType;
    }

    Vector2<real_Num> MouseState::getDragValue() const
    {
        return m_dragValue;
    }

    void MouseState::setDragValue( const Vector2<real_Num> &dragValue )
    {
        m_dragValue = dragValue;
    }

    bool MouseState::isDragging() const
    {
        return BitUtil::getFlagValue( m_flags, MOUSE_DRAGGING );
    }

    void MouseState::setDragging( bool dragging )
    {
        m_flags = BitUtil::setFlagValue( m_flags, MOUSE_DRAGGING, dragging );
    }

    bool MouseState::isDoubleClick() const
    {
        return BitUtil::getFlagValue( m_flags, MOUSE_DOUBLE_CLICK );
    }

    void MouseState::setDoubleClick( bool doubleClick )
    {
        m_flags = BitUtil::setFlagValue( m_flags, MOUSE_DOUBLE_CLICK, doubleClick );
    }

}  // namespace workphone
