#ifndef ImGuiManager_h__
#define ImGuiManager_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindowListener.hpp>
#include "ImGuiOverlayOgre.hpp"

namespace workphone
{
    class ImGuiManagerOgre : public ISharedObject
    {
    public:
        class WindowListener : public render::IGraphicsWindowListener
        {
        public:
            WindowListener();
            ~WindowListener() override;

            void unload( SmartPtr<ISharedObject> data ) override;

            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            void handleEvent( SmartPtr<render::IGraphicsWindowEvent> event ) override;

            void setOwner( SmartPtr<ImGuiManagerOgre> owner );
            SmartPtr<ImGuiManagerOgre> getOwner() const;

            WP_CLASS_REGISTER_DECL;

        protected:
            AtomicWeakPtr<ImGuiManagerOgre> m_owner;
        };

        class UIOverlay : public ISharedObject
        {
        public:
            UIOverlay();
            ~UIOverlay() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void setOwner( SmartPtr<ImGuiManagerOgre> owner );

            SmartPtr<ImGuiManagerOgre> getOwner() const;

            WP_CLASS_REGISTER_DECL;

        protected:
            AtomicWeakPtr<ImGuiManagerOgre> m_owner;
        };

        ImGuiManagerOgre();
        ~ImGuiManagerOgre() override;

        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;

        // Returns true once setupImgui() has completed successfully
        bool isInitialised() const;

        // Tears down all ImGui and Ogre resources; safe to call multiple times
        void shutdown();

        SmartPtr<render::IGraphicsWindow> getWindow() const;

        void setWindow( SmartPtr<render::IGraphicsWindow> window );

        WP_CLASS_REGISTER_DECL;

        static ImGuiOverlayOgre *m_overlay;

    protected:
        void setupImgui();

        SmartPtr<render::IGraphicsWindow> m_window;

        Ogre::RenderTargetListener *m_renderTargetListener = nullptr;

        void *m_hwnd = nullptr;

        bool m_initialised = false;

        SmartPtr<UIOverlay> m_uiOverlay;

        SmartPtr<render::IGraphicsWindowListener> m_windowListener;
    };
}  // namespace workphone

#endif  // ImGuiManager_h__
