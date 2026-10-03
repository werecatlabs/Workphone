#ifndef WPWxMouseState_h__
#define WPWxMouseState_h__

#include <WPWxWidgets/WPWxWidgetsPrerequisites.hpp>
#include <Workphone/Interface/Input/IMouseState.hpp>

namespace workphone
{
    namespace ui
    {

        //--------------------------------------------
        class WxMouseState : public IMouseState
        {
        public:
            WxMouseState();
            ~WxMouseState();

            Vector2F getRelativeMove() const;
            void setRelativeMove( const Vector2F &relativeMove );

            Vector2F getRelativePosition() const;
            void setRelativePosition( const Vector2F &position );

            Vector2F getAbsolutePosition() const;
            void setAbsolutePosition( const Vector2F &position );

            Vector2<real_Num> getWheelDelta() const;
            void setWheelDelta( const Vector2<real_Num> &wheelDelta );

            bool isShiftPressed() const;
            void setShiftPressed( bool shiftPressed );

            bool isControlPressed() const;
            void setControlPressed( bool controlPressed );

            bool isButtonPressed( u32 id ) const;
            void setButtonPressed( u32 id, bool isPressed );

            IMouseState::Event getEventType() const;
            void setEventType( IMouseState::Event eventType );

        protected:
            Vector2F m_movePosition;
            Vector2F m_relativePosition;
            Vector2F m_absolutePosition;
            Vector2<real_Num> m_wheelPosition;
            IMouseState::Event m_eventType;
            u32 m_buttonMask;
            bool m_isShiftPressed;
            bool m_isControlPressed;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // WPWxMouseState_h__
