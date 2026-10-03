#ifndef _WPOgreViewWindow_H
#define _WPOgreViewWindow_H

#include <WPWxWidgets/WPWxWidgetsPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <wx/window.h>

namespace workphone
{
    namespace ui
    {

        //--------------------------------------------
        class wxViewWindow : public wxWindow
        {
        public:
            wxViewWindow( wxWindow *parent, int xPosition, int yPosition, int width, int height );
            virtual ~wxViewWindow();

            SmartPtr<render::ICamera> getCamera() const;
            void setCamera( SmartPtr<render::ICamera> camera );

            SmartPtr<render::ISceneNode> getCameraNode() const;
            void setCameraNode( SmartPtr<render::ISceneNode> cameraNode );

            SmartPtr<render::IViewport> getViewport() const;
            void setViewport( SmartPtr<render::IViewport> viewport );

            SmartPtr<render::IWindow> getWindow() const;
            void setWindow( SmartPtr<render::IWindow> window );

            void setupRenderWindow();

            //
            // Events
            //
            void OnMove( wxMoveEvent &event );
            void OnSize( wxSizeEvent &event );
            void OnPaint( wxPaintEvent &event );
            void OnMouse( wxMouseEvent &event );
            void OnKey( wxKeyEvent &event );

        protected:
            ///
            SmartPtr<render::ICamera> m_camera;

            ///
            SmartPtr<render::ISceneNode> m_cameraNode;

            ///
            SmartPtr<render::IViewport> m_viewport;

            /// The renderWindow
            SmartPtr<render::IWindow> m_window;

            ///
            Vector3F m_targetPosition;

            Vector2F m_prevMousePosition;

            DECLARE_EVENT_TABLE()
        };

    }  // namespace ui
}  // namespace workphone

#endif
