#pragma once
#include <Workphone/Interface/Graphics/IGraphicsWindowListener.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector2.hpp>

#ifdef WP_PLATFORM_WIN32
#    include <Windows.h>
#    include <WinUser.h>
#endif

namespace workphone
{

    class PlatformInputManager : public ISharedObject
    {
    public:
        PlatformInputManager();
        ~PlatformInputManager();

        SmartPtr<render::IGraphicsWindow> getWindow() const;

        void setWindow( SmartPtr<render::IGraphicsWindow> window );

        void handleRawInput( HRAWINPUT hRawInput );

    protected:
        class WindowListener : public render::IGraphicsWindowListener
        {
        public:
            WindowListener();
            ~WindowListener();

            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event );

            void handleEvent( SmartPtr<render::IGraphicsWindowEvent> event );

            SmartPtr<PlatformInputManager> getOwner() const;
            void setOwner( SmartPtr<PlatformInputManager> owner );

            Vector2F m_lastMousePosition;
            AtomicWeakPtr<PlatformInputManager> m_owner;
        };

        SmartPtr<render::IGraphicsWindow> m_window;
        SmartPtr<render::IGraphicsWindowListener> m_listener;
    };

}  // namespace workphone
