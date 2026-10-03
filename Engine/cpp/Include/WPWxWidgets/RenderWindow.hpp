#ifndef __WXRenderWindow_h__
#define __WXRenderWindow_h__

#include <WPWxWidgets/WPWxWidgetsPrerequisites.hpp>
#include <WPWxWidgets/WPWxWidgets.hpp>
#include <wx/wx.h>

namespace workphone
{
    namespace ui
    {

        //---------------------------------------------------------------------
        class RenderWindow : public wxScrolledWindow
        {
        public:
            RenderWindow( wxWindow *parent, wxWindowID id );
            ~RenderWindow();

            ui::wxViewWindow *getWxWindow() const;

            SmartPtr<render::ICamera> getCamera() const;
            void setCamera( SmartPtr<render::ICamera> camera );

            SmartPtr<render::ISceneNode> getCameraNode() const;
            void setCameraNode( SmartPtr<render::ISceneNode> cameraNode );

            SmartPtr<render::IWindow> getWindow() const;
            void setWindow( SmartPtr<render::IWindow> window );

        protected:
            void initialiseRenderWindows();

            ui::wxViewWindow *m_viewWindow = nullptr;

            SmartPtr<render::ICamera> m_camera;
            SmartPtr<render::ISceneNode> m_cameraNode;
            SmartPtr<render::IViewport> m_viewport;
            SmartPtr<render::IWindow> m_window;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // GUIRenderWindow_h__
