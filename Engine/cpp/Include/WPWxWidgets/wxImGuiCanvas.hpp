#ifndef __wxImGuiImpl_H__
#define __wxImGuiImpl_H__

#include <WPWxWidgets/WPWxWidgetsPrerequisites.hpp>

#include "imgui.h"

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#    include <wx/wx.h>
#endif

#include <wx/timer.h>
#include <wx/math.h>
#include <wx/window.h>
#include <wx/event.h>
#include <wx/utils.h>
#include <Workphone/Core/ColourF.h>
#include <Workphone/Math/Vector2.h>

namespace workphone
{

    // Render Loop
    // https://wiki.wxwidgets.org/Making_a_render_loop
    class wxImGuiCanvas : public wxWindow
    {
    public:
        static wxImGuiCanvas *Create( wxWindow *parent, wxWindowID id = wxID_ANY,
                                      const wxPoint &pos = wxDefaultPosition,
                                      const wxSize &size = wxDefaultSize, long style = 0,
                                      const wxString &name = "" );

    public:
        wxImGuiCanvas( GLContext *vContext = 0 );

        virtual ~wxImGuiCanvas();

        //bool CreateGLContext(HDC vHDC);
        //void DestroyGLContext(HDC vHDC);
        //bool SetCurrent(HDC vHDC);

        //// get the HDC used for OpenGL rendering
        //HDC GetHDC() const;

        void activateRenderLoop( bool on );
        void onIdle( wxIdleEvent &evt );

        void OnPaint( wxPaintEvent &event );
        void OnSize( wxSizeEvent &event );
        void OnChar( wxKeyEvent &event );
        void OnMouseEvent( wxMouseEvent &event );
        void OnKeyDown( wxKeyEvent &event );
        void OnKeyUp( wxKeyEvent &event );

        Vector2F GetCanvasSize();

    private:  // https://github.com/ocornut/imgui // based on glfw sample
        wxWindow *g_Window;
        float g_MousePos[2];
        bool g_MousePressed[3];
        float g_MouseWheel;
        u32 g_FontTexture;
        f64 g_Time;

        wxTimer m_timer;  // timer

        bool m_RenderLoopOn;

        GLContext *m_GLContext;

    private:
        // https://github.com/ocornut/imgui // based on glfw sample
        bool ImGui_ImplWx_Init( wxWindow *window );
        void ImGui_ImplWx_Shutdown();
        void ImGui_ImplWx_NewFrame( wxWindow *window );

        // Use if you want to reset your rendering device without losing ImGui state.
        void ImGui_ImplWx_InvalidateDeviceObjects();
        bool ImGui_ImplWx_CreateDeviceObjects();

        void ImGui_ImplWx_MouseButtonCallback( wxWindow *window, int button, int action, int mods );
        void ImGui_ImplWx_ScrollCallback( wxWindow *window, double xoffset, double yoffset );
        void ImGui_ImplWx_KeyCallback( wxWindow *window, int key, int scancode, int action, int mods );
        void ImGui_ImplWx_CharCallback( wxWindow *window, unsigned int c );

        static void ImGui_ImplWx_RenderDrawLists( ImDrawData *draw_data );
        static const char *ImGui_ImplWx_GetClipboardText( void *user_data );
        static void ImGui_ImplWx_SetClipboardText( void *user_data, const char *text );

        virtual void OnDrawImGui();

    public:
        //HDC GetHDC();

        virtual void OnTimer( wxTimerEvent &event );

    private:
        //HGLRC m_hrc;// opengl context
        //HDC m_hDC;// HDC for this window, we keep it all the time

        Vector2F m_ScreenSize;
        ColourF m_ClearColor;

    protected:
        bool CreateWindow( wxWindow *parent, wxWindowID id = wxID_ANY,
                           const wxPoint &pos = wxDefaultPosition, const wxSize &size = wxDefaultSize,
                           long style = 0, const wxString &name = "" );

        wxDECLARE_NO_COPY_CLASS( wxImGuiCanvas );
        wxDECLARE_EVENT_TABLE();
    };

}  // namespace workphone

#endif  //__wxImGuiImpl_H__
