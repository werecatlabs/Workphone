#ifndef WPWxInputManager_h__
#define WPWxInputManager_h__

#include <WPWxWidgets/WPWxWidgetsPrerequisites.hpp>
#include <Workphone/Interface/Input/IInputDeviceManager.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace ui
    {

        //--------------------------------------------
        class WxInputManager : public IInputDeviceManager
        {
        public:
            WxInputManager();
            ~WxInputManager();

            void addInputListener( SmartPtr<ISharedObject> inputListener );
            bool removeInputListener( SmartPtr<ISharedObject> inputListener );

            SmartPtr<IGameInput> addGameInput( hash32 id );
            SmartPtr<IGameInput> findGameInput( hash32 id ) const;
            Array<SmartPtr<IGameInput>> getGameInputs() const;

            bool isCursorVisible() const;
            void setCursorVisible( bool visible );

            void triggerEvent( SmartPtr<IInputEvent> inputEvent ) override;

            void queueEvent( SmartPtr<IInputEvent> event ) override;

            bool postEvent( SmartPtr<IInputEvent> event ) override;

            void addListener( SmartPtr<IEventListener> listener ) override;

            void removeListener( SmartPtr<IEventListener> listener ) override;

            void removeListeners() override;

            bool isShiftPressed() const override;

            void setShiftPressed( bool shiftPressed ) override;

            bool isLeftPressed() const;

            void setLeftPressed( bool leftPressed );

            bool isRightPressed() const;

            void setRightPressed( bool rightPressed );

            bool isMiddlePressed() const;

            void setMiddlePressed( bool middlePressed );

            double getLastClickTime() const override;

            void setLastClickTime( double lastClickTime ) override;

            double getDoubleClickInterval() const override;

            void setDoubleClickInterval( double doubleClickInterval ) override;

            double getLastInputTime() const override;

            void setLastInputTime( double lastInputTime ) override;

            double getInputTime() const override;

            void setInputTime( double inputTime ) override;

            void addInputTime( double inputTime ) override;

            bool isKeyPressed( KeyCodes keyCode ) const override;

        protected:
            Array<SmartPtr<ISharedObject>> m_listeners;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // WPWxInputManager_h__
