#ifndef WPWxInputEvent_h__
#define WPWxInputEvent_h__

#include <WPWxWidgets/WPWxWidgetsPrerequisites.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>

namespace workphone
{
    namespace ui
    {

        //--------------------------------------------
        class WxInputEvent : public IInputEvent
        {
        public:
            WxInputEvent();
            ~WxInputEvent();

            SmartPtr<IMouseState> getMouseState() const;
            void setMouseState( SmartPtr<IMouseState> mouseState );

            SmartPtr<IKeyboardState> getKeyboardState() const;
            void setKeyboardState( SmartPtr<IKeyboardState> keyboardState );

            SmartPtr<IJoystickState> getJoystickState() const;
            void setJoystickState( SmartPtr<IJoystickState> joystickState );

            SmartPtr<IGameInputState> getGameInputState() const;
            void setGameInputState( SmartPtr<IGameInputState> gameInputState );

            hash_type getGameInputId() const;
            void setGameInputId( hash_type gameInputId );

            IInputEvent::EventType getEventType() const;
            void setEventType( IInputEvent::EventType eventType );

            void *getUserData() const;
            void setUserData( void *data );

            void *getWindow() const;
            void setWindow( void *window );

            float getWheel() const;
            void setWheel( float wheel );

            bool isLeftPressed() const;
            bool isRightPressed() const;

            bool isMiddlePressed() const;
            bool isLeftReleased() const;

            bool isRightReleased() const;
            bool isMiddleReleased() const;

        protected:
            ///
            SmartPtr<IMouseState> m_mouseState;

            ///
            SmartPtr<IKeyboardState> m_keyboardState;

            ///
            SmartPtr<IJoystickState> m_joystickState;

            ///
            SmartPtr<IGameInputState> m_gameInputState;

            /// The id of the game input that was used to trigger this event.
            hash32 m_gameInputId = 0;

            /// The event type.
            IInputEvent::EventType m_eventType = IInputEvent::EventType::None;

            /// User data as an unsigned int.
            void *m_userData = nullptr;

            ///
            void *m_window = nullptr;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // WPWxInputEvent_h__
