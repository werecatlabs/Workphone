#include <WPWxWidgets/WPWxWidgetsPCH.hpp>
#include "WPWxWidgets/WPWxMouseState.hpp"

namespace workphone
{
    namespace ui
    {

        //--------------------------------------------
        WxMouseState::WxMouseState()
        {
            m_buttonMask = 0;
        }

        //--------------------------------------------
        WxMouseState::~WxMouseState()
        {
        }

        Vector2F WxMouseState::getRelativeMove() const
        {
            return m_movePosition;
        }

        void WxMouseState::setRelativeMove( const Vector2F &relativeMove )
        {
            m_movePosition = relativeMove;
        }

        //--------------------------------------------
        Vector2F WxMouseState::getRelativePosition() const
        {
            return m_relativePosition;
        }

        //--------------------------------------------
        void WxMouseState::setRelativePosition( const Vector2F &position )
        {
            m_relativePosition = position;
        }

        //--------------------------------------------
        Vector2F WxMouseState::getAbsolutePosition() const
        {
            return m_absolutePosition;
        }

        //--------------------------------------------
        void WxMouseState::setAbsolutePosition( const Vector2F &position )
        {
            m_absolutePosition = position;
        }

        //--------------------------------------------
        Vector2<real_Num> WxMouseState::getWheelDelta() const
        {
            return m_wheelPosition;
        }

        //--------------------------------------------
        void WxMouseState::setWheelDelta( const Vector2<real_Num> &wheelDelta )
        {
            m_wheelPosition = wheelDelta;
        }

        //--------------------------------------------
        bool WxMouseState::isShiftPressed() const
        {
            return m_isShiftPressed;
        }

        //--------------------------------------------
        void WxMouseState::setShiftPressed( bool shiftPressed )
        {
            m_isShiftPressed = shiftPressed;
        }

        //--------------------------------------------
        bool WxMouseState::isControlPressed() const
        {
            return m_isControlPressed;
        }

        //--------------------------------------------
        void WxMouseState::setControlPressed( bool controlPressed )
        {
            m_isControlPressed = controlPressed;
        }

        //--------------------------------------------
        bool WxMouseState::isButtonPressed( u32 id ) const
        {
            //return BitUtil::getFlagValue(m_buttonMask, id);
            return false;
        }

        //--------------------------------------------
        void WxMouseState::setButtonPressed( u32 id, bool isPressed )
        {
            //m_buttonMask = BitUtil::setFlagValue(m_buttonMask, id, isPressed);
        }

        //--------------------------------------------
        IMouseState::Event WxMouseState::getEventType() const
        {
            return m_eventType;
        }

        //--------------------------------------------
        void WxMouseState::setEventType( IMouseState::Event eventType )
        {
            m_eventType = eventType;
        }

    }  // namespace ui
}  // namespace workphone
