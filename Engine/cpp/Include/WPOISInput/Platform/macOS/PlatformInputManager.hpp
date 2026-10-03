#pragma once
#include <Workphone/Interface/Graphics/IWindowListener.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{

    class PlatformInputManager : public ISharedObject
    {
    public:
        PlatformInputManager() = default;
        ~PlatformInputManager() = default;

        SmartPtr<render::IWindow> getWindow() const;

        void setWindow( SmartPtr<render::IWindow> window );

    protected:
        class WindowListener : public render::IWindowListener
        {
        public:
            WindowListener() = default;
            ~WindowListener() = default;

            Parameter handleEvent( IEvent::Type eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object,
                                   SmartPtr<IEvent> event )
            {
                if( eventValue == IWindowListener::windowClosingHash )
                {
                    return Parameter( true );
                }

                return Parameter();
            }

            void handleEvent( SmartPtr<render::IWindowEvent> event );

            Vector2F m_lastMousePosition;
        };

        SmartPtr<render::IWindow> m_window;
        SmartPtr<render::IWindowListener> m_listener;
    };

}  // namespace fb
