#include <WPWxWidgets/WPWxWidgetsPCH.hpp>
#include <WPWxWidgets/WPWxInputManager.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/Interface/Input/IGameInput.hpp>

namespace workphone
{
    namespace ui
    {

        //--------------------------------------------
        WxInputManager::WxInputManager()
        {
        }

        //--------------------------------------------
        WxInputManager::~WxInputManager()
        {
        }

        //--------------------------------------------
        bool WxInputManager::postEvent( SmartPtr<IInputEvent> event )
        {
            //for( auto listener : m_listeners )
            //{
            //    if( listener )
            //    {
            //        if( listener->inputEvent( event ) )
            //        {
            //            return true;
            //        }
            //    }
            //}

            return false;
        }

        //--------------------------------------------
        void WxInputManager::addListener( SmartPtr<IEventListener> listener )
        {
            m_listeners.push_back( listener );
        }

        //--------------------------------------------
        void WxInputManager::removeListener( SmartPtr<IEventListener> listener )
        {
            auto it = std::find( m_listeners.begin(), m_listeners.end(), listener );
            if( it != m_listeners.end() )
            {
                m_listeners.erase( it );
            }
        }

        //--------------------------------------------
        void WxInputManager::removeListeners()
        {
        }

        //--------------------------------------------
        bool WxInputManager::isShiftPressed() const
        {
            return false;
        }

        //--------------------------------------------
        void WxInputManager::setShiftPressed( bool shiftPressed )
        {
        }

        //--------------------------------------------
        bool WxInputManager::isLeftPressed() const
        {
            return false;
        }

        //--------------------------------------------
        void WxInputManager::setLeftPressed( bool leftPressed )
        {
        }

        //--------------------------------------------
        bool WxInputManager::isRightPressed() const
        {
            return false;
        }

        //--------------------------------------------
        void WxInputManager::setRightPressed( bool rightPressed )
        {
        }

        //--------------------------------------------
        bool WxInputManager::isMiddlePressed() const
        {
            return false;
        }

        //--------------------------------------------
        void WxInputManager::setMiddlePressed( bool middlePressed )
        {
        }

        //--------------------------------------------
        double WxInputManager::getLastClickTime() const
        {
            return 0.0;
        }

        //--------------------------------------------
        void WxInputManager::setLastClickTime( double lastClickTime )
        {
        }

        //--------------------------------------------
        double WxInputManager::getDoubleClickInterval() const
        {
            return 0.0;
        }

        //--------------------------------------------
        void WxInputManager::setDoubleClickInterval( double doubleClickInterval )
        {
        }

        //--------------------------------------------
        double WxInputManager::getLastInputTime() const
        {
            return 0.0;
        }

        //--------------------------------------------
        void WxInputManager::setLastInputTime( double lastInputTime )
        {
        }

        //--------------------------------------------
        double WxInputManager::getInputTime() const
        {
            return 0.0;
        }

        //--------------------------------------------
        void WxInputManager::setInputTime( double inputTime )
        {
        }

        //--------------------------------------------
        void WxInputManager::addInputTime( double inputTime )
        {
        }

        //--------------------------------------------
        void WxInputManager::addInputListener( SmartPtr<ISharedObject> inputListener )
        {
            m_listeners.push_back( inputListener );
        }

        //--------------------------------------------
        bool WxInputManager::removeInputListener( SmartPtr<ISharedObject> inputListener )
        {
            return false;
        }

        //--------------------------------------------
        SmartPtr<IGameInput> WxInputManager::addGameInput( hash32 id )
        {
            return nullptr;
        }

        //--------------------------------------------
        SmartPtr<IGameInput> WxInputManager::findGameInput( hash32 id ) const
        {
            return nullptr;
        }

        //--------------------------------------------
        Array<SmartPtr<IGameInput>> WxInputManager::getGameInputs() const
        {
            Array<SmartPtr<IGameInput>> gameInputs;
            return gameInputs;
        }

        //--------------------------------------------
        bool WxInputManager::isCursorVisible() const
        {
            return false;
        }

        //--------------------------------------------
        void WxInputManager::setCursorVisible( bool visible )
        {
        }

        //--------------------------------------------
        void WxInputManager::triggerEvent( SmartPtr<IInputEvent> inputEvent )
        {
        }

        //--------------------------------------------
        void WxInputManager::queueEvent( SmartPtr<IInputEvent> event )
        {
        }

        //--------------------------------------------
        bool WxInputManager::isKeyPressed( KeyCodes keyCode ) const
        {
            return true;
        }

    }  // end namespace ui
}  // namespace workphone
