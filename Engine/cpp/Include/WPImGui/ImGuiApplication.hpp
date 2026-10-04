#ifndef ImGuiApplication_h__
#define ImGuiApplication_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <Workphone/Interface/UI/IUIApplication.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindowListener.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Core/FixedString.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <imgui_internal.h>

namespace workphone
{
    namespace ui
    {

        class ImGuiApplication : public IUIApplication
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

                void setOwner( SmartPtr<ImGuiApplication> owner );
                SmartPtr<ImGuiApplication> getOwner() const;

            protected:
                AtomicWeakPtr<ImGuiApplication> m_owner;
            };

            ImGuiApplication();
            ~ImGuiApplication() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void setCustomStyle();
            void setDarkGreenStyle();
            void setDarkBlueStyle();

            size_t messagePump( SmartPtr<ISharedObject> data );

            void handleWindowEvent( SmartPtr<render::IGraphicsWindowEvent> event ) override;

            bool handleInputEvent( SmartPtr<IInputEvent> event ) override;

            void run();

            void createSubMenus( SmartPtr<IUIMenu> menu );

            void createElement( SmartPtr<IUIElement> element );

            void update() override;

            void *getHWND() const;

            SmartPtr<IUIMenubar> getMenubar() const override;
            void setMenubar( SmartPtr<IUIMenubar> menubar ) override;

            SmartPtr<IUIToolbar> getToolbar() const override;
            void setToolbar( SmartPtr<IUIToolbar> toolbar ) override;

            void showApp( bool *p_open );

            void createMenuItem( SmartPtr<IUIMenu> rootMenu, SmartPtr<IUIElement> menuItemElement );

            Vector2I getWindowSize() const override;
            void setWindowSize( const Vector2I &size ) override;

            void draw( SmartPtr<IUIRenderWindow> renderWindow );

            bool getUseInputEvents() const;
            void setUseInputEvents( bool useInputEvents );

            void *getEmptyTexture() const;

            void setEmptyTexture( void *emptyTexture );

            static ImGuiOverlayOgre *getOverlay();

            ImGuiID m_dockLeftIdLeft = 0;
            ImGuiID m_dockLeftIdRight = 0;
            ImGuiID m_dockLeftIdUp = 0;
            ImGuiID m_dockObjectId = 0;
            ImGuiID m_dockSceneId = 0;
            ImGuiID m_dockProjectId = 0;

            static Array<u8> s_fontData;
            static ImFont *s_fontAwesomeFont;

            WP_CLASS_REGISTER_DECL;

        protected:
            void setupDefaultDockLayout( ImGuiID dockspaceId, const ImVec2 &dockspaceSize );

            void editTransform( float *cameraView, float *cameraProjection, float *matrix,
                                bool editTransformDecomposition );
            void showEditor( bool *p_open );

            void showGuizmo();
            void dockSpaceUI();
            void toolbarUI();
            void dockingToolbar( const char *name, ImGuiAxis *p_toolbar_axis,
                                 SmartPtr<IUIElement> toolbarElement );
            void testDoc();

            void showPlaceholderObject( const char *prefix, int uid );

            SmartPtr<render::IGraphicsWindowListener> m_windowListener;

            AtomicSmartPtr<IUIMenubar> m_menuBar;
            AtomicSmartPtr<IUIToolbar> m_toolbar;

            Ogre::RenderTargetListener *m_renderTargetListener = nullptr;

            void *m_hwnd = nullptr;

            ImguiManagerOgre *m_imguiManagerOgre = nullptr;

            ImGuiApplicationOSX *m_app = nullptr;

            void *m_emptyTexture = nullptr;

            ColourF m_clearColor = ColourF( 0.45f, 0.55f, 0.60f, 1.00f );

            Vector2I m_currentMousePosition;

            Vector2I m_size;

            f32 m_camDistance = 8.f;

            u32 m_childWindowCount = 0;
            s32 m_gizmoCount = 1;
            s32 m_lastUsing = 0;

            bool opt_fullscreen = true;
            bool opt_padding = false;
            ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

            bool m_showDemoWindow = true;
            bool m_showAnotherWindow = false;
            bool m_showDockSpace = true;
            bool m_done = false;

            bool m_useInputEvents = false;
            bool m_showEditor = true;
            bool m_useWindow = true;

            String applicationName = "Workphone";

            FixedString<128> m_iniPath;
            FixedString<128> m_logPath;

            static ImGuiOverlayOgre *m_overlay;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiApplication_h__
