#include <WPWxWidgets/WPWxWidgetsPCH.hpp>
#include "WPWxWidgets/WPWxKeyboardState.hpp"

namespace workphone
{
    namespace ui
    {

        //--------------------------------------------
        WxKeyboardState::WxKeyboardState() :
            m_char( 0 ),
            m_keyCode( 0 ),
            m_rawKeyCode( 0 ),
            m_isPressedDown( false ),
            m_isShiftPressed( false ),
            m_isControlPressed( false )
        {
        }

        //--------------------------------------------
        WxKeyboardState::~WxKeyboardState()
        {
        }

        //--------------------------------------------
        u32 WxKeyboardState::getChar() const
        {
            return m_char;
        }

        //--------------------------------------------
        void WxKeyboardState::setChar( u32 char )
        {
            m_char = char;
        }

        //--------------------------------------------
        u32 WxKeyboardState::getKeyCode() const
        {
            return m_keyCode;
        }

        //--------------------------------------------
        void WxKeyboardState::setKeyCode( u32 keyCode )
        {
            m_keyCode = keyCode;
        }

        //--------------------------------------------
        u32 WxKeyboardState::getRawKeyCode() const
        {
            return m_rawKeyCode;
        }

        //--------------------------------------------
        void WxKeyboardState::setRawKeyCode( u32 rawKeyCode )
        {
            m_rawKeyCode = rawKeyCode;
        }

        //--------------------------------------------
        bool WxKeyboardState::isPressedDown() const
        {
            return m_isPressedDown;
        }

        //--------------------------------------------
        void WxKeyboardState::setPressedDown( bool pressedDown )
        {
            m_isPressedDown = pressedDown;
        }

        //--------------------------------------------
        bool WxKeyboardState::isShiftPressed() const
        {
            return m_isShiftPressed;
        }

        //--------------------------------------------
        void WxKeyboardState::setShiftPressed( bool shiftPressed )
        {
            m_isShiftPressed = shiftPressed;
        }

        //--------------------------------------------
        bool WxKeyboardState::isControlPressed() const
        {
            return m_isControlPressed;
        }

        //--------------------------------------------
        void WxKeyboardState::setControlPressed( bool controlPressed )
        {
            m_isControlPressed = controlPressed;
        }

    }  // namespace ui
}  // namespace workphone
