#ifndef __WXRenderWindow4_h__
#define __WXRenderWindow4_h__

#include <WPWxWidgets/WPWxWidgetsPrerequisites.hpp>
#include <WPWxWidgets/WPWxWidgets.hpp>
#include <wx/wx.hpp>

namespace workphone
{
    namespace ui
    {

        //---------------------------------------------------------------------
        class RenderWindow4 : public wxScrolledWindow
        {
        public:
            RenderWindow4( wxWindow *parent, wxWindowID id );
            ~RenderWindow4();

            ui::wxViewWindow *getWindow( u32 index ) const;

            SmartPtr<render::ICamera> getCamera() const;
            void setCamera( SmartPtr<render::ICamera> camera );

            SmartPtr<render::ISceneNode> getCameraNode() const;
            void setCameraNode( SmartPtr<render::ISceneNode> cameraNode );

            SmartPtr<render::IWindow> getWindow() const;
            void setWindow( SmartPtr<render::IWindow> window );

        protected:
            void initialiseRenderWindows();

            ui::wxFourWaySplitter *mSplitter;
            ui::wxViewWindow *mViewWindows[4];

            SmartPtr<render::ICamera> m_camera;
            SmartPtr<render::ISceneNode> m_cameraNode;
            SmartPtr<render::IViewport> m_viewport;
            SmartPtr<render::IWindow> m_window;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // __RenderWindow4_h__
