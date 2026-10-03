#include <WPWxWidgets/WPWxWidgetsPCH.hpp>
#include "WPWxWidgets/WPWxInputEvent.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace ui
    {

        //--------------------------------------------
        WxInputEvent::WxInputEvent() : m_userData( 0 ), m_gameInputId( 0 ), m_window( nullptr )
        {
        }

        //--------------------------------------------
        WxInputEvent::~WxInputEvent()
        {
        }

        //--------------------------------------------
        SmartPtr<IMouseState> WxInputEvent::getMouseState() const
        {
            return m_mouseState;
        }

        //--------------------------------------------
        void WxInputEvent::setMouseState( SmartPtr<IMouseState> mouseState )
        {
            m_mouseState = mouseState;
        }

        //--------------------------------------------
        SmartPtr<IKeyboardState> WxInputEvent::getKeyboardState() const
        {
            return m_keyboardState;
        }

        //--------------------------------------------
        void WxInputEvent::setKeyboardState( SmartPtr<IKeyboardState> keyboardState )
        {
            m_keyboardState = keyboardState;
        }

        //--------------------------------------------
        SmartPtr<IJoystickState> WxInputEvent::getJoystickState() const
        {
            return m_joystickState;
        }

        //--------------------------------------------
        void WxInputEvent::setJoystickState( SmartPtr<IJoystickState> joystickState )
        {
            m_joystickState = joystickState;
        }

        //--------------------------------------------
        SmartPtr<IGameInputState> WxInputEvent::getGameInputState() const
        {
            return m_gameInputState;
        }

        //--------------------------------------------
        void WxInputEvent::setGameInputState( SmartPtr<IGameInputState> gameInputState )
        {
            m_gameInputState = gameInputState;
        }

        //--------------------------------------------
        hash_type WxInputEvent::getGameInputId() const
        {
            return m_gameInputId;
        }

        //--------------------------------------------
        void WxInputEvent::setGameInputId( hash_type gameInputId )
        {
            m_gameInputId = gameInputId;
        }

        //--------------------------------------------
        IInputEvent::EventType WxInputEvent::getEventType() const
        {
            return m_eventType;
        }

        //--------------------------------------------
        void WxInputEvent::setEventType( IInputEvent::EventType eventType )
        {
            m_eventType = eventType;
        }

        //--------------------------------------------
        void *WxInputEvent::getUserData() const
        {
            return m_userData;
        }

        //--------------------------------------------
        void WxInputEvent::setUserData( void *data )
        {
            m_userData = data;
        }

        //--------------------------------------------
        void *WxInputEvent::getWindow() const
        {
            return m_window;
        }

        //--------------------------------------------
        void WxInputEvent::setWindow( void *window )
        {
            m_window = window;
        }

        float WxInputEvent::getWheel() const
        {
            return 0.0f;
        }

        void WxInputEvent::setWheel( float wheel )
        {
        }

        bool WxInputEvent::isLeftPressed() const
        {
            return false;
        }

        bool WxInputEvent::isRightPressed() const
        {
            return false;
        }

        bool WxInputEvent::isMiddlePressed() const
        {
            return false;
        }

        bool WxInputEvent::isLeftReleased() const
        {
            return false;
        }

        bool WxInputEvent::isRightReleased() const
        {
            return false;
        }

        bool WxInputEvent::isMiddleReleased() const
        {
            return false;
        }

    }  // namespace ui
}  // namespace workphone
