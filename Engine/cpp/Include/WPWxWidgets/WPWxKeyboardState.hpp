#ifndef WPWxKeyboardState_h__
#define WPWxKeyboardState_h__

#include <WPWxWidgets/WPWxWidgetsPrerequisites.hpp>
#include <Workphone/Interface/Input/IKeyboardState.hpp>

namespace workphone
{
    namespace ui
    {

        //--------------------------------------------
        class WxKeyboardState : public IKeyboardState
        {
        public:
            WxKeyboardState();
            ~WxKeyboardState();

            u32 getChar() const;
            void setChar( u32 char );

            u32 getKeyCode() const;
            void setKeyCode( u32 keyCode );

            u32 getRawKeyCode() const;
            void setRawKeyCode( u32 rawKeyCode );

            bool isPressedDown() const;
            void setPressedDown( bool pressedDown );

            bool isShiftPressed() const;
            void setShiftPressed( bool shiftPressed );

            bool isControlPressed() const;
            void setControlPressed( bool controlPressed );

        protected:
            u32 m_char;
            u32 m_keyCode;
            u32 m_rawKeyCode;
            bool m_isPressedDown;
            bool m_isShiftPressed;
            bool m_isControlPressed;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // WPWxKeyboardState_h__
