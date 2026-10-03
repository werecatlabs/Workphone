#include <WPWxWidgets/WPWxWidgetsPCH.hpp>
#include "WPWxWidgets/wxImGuiCanvas.hpp"
#include <wx/clipbrd.hpp>
//#include <wx/msw/private.hpp>
#include <wx/wx.hpp>
//#include "GLContext.hpp"

//#include <GL/GL.hpp>

#define wxIMGUI_FPS 60.0f

wxBEGIN_EVENT_TABLE( fb::wxImGuiCanvas, wxWindow ) EVT_SIZE( wxImGuiCanvas::OnSize )
    EVT_PAINT( wxImGuiCanvas::OnPaint ) EVT_CHAR( wxImGuiCanvas::OnChar )
        EVT_KEY_DOWN( wxImGuiCanvas::OnKeyDown ) EVT_KEY_UP( wxImGuiCanvas::OnKeyUp )
            EVT_MOUSE_EVENTS( wxImGuiCanvas::OnMouseEvent )
    //EVT_TIMER(ID_WX_IMGUI, wxImGuiCanvas::OnTimer)
    wxEND_EVENT_TABLE()

        namespace fb
{
    wxImGuiCanvas *wxImGuiCanvas::Create( wxWindow * parent, wxWindowID id, const wxPoint &pos,
                                          const wxSize &size, long style, const wxString &name )
    {
        wxImGuiCanvas *res = 0;

        //if (parent != 0)
        //{
        //	res = new wxImGuiCanvas();
        //	if (res != 0)
        //	{
        //		bool state = res->CreateWindow(parent, id, pos, size, style, name);
        //		if (state == false)
        //		{
        //			delete res;
        //			res = 0;
        //		}
        //	}
        //}

        return res;
    }

    wxImGuiCanvas::wxImGuiCanvas( GLContext * vContext )
    //: m_hDC(0), g_Window(0), g_FontTexture(0)/*, m_timer(this, ID_WX_IMGUI)*/, m_RenderLoopOn(false), m_GLContext(vContext)
    {
    }

    // Replaces wxWindow::Create functionality, since we need to use a different window class
    bool wxImGuiCanvas::CreateWindow( wxWindow * parent, wxWindowID id, const wxPoint &pos,
                                      const wxSize &size, long style, const wxString &name )
    {
        //wxCHECK_MSG(parent, false, wxT("can't create wxWindow without parent"));

        //if (!CreateBase(parent, id, pos, size, style, wxDefaultValidator, name))
        //	return false;

        //parent->AddChild(this);

        ///*
        //A general rule with OpenGL and Win32 is that any window that will have a
        //HGLRC built for it must have two flags:  WS_CLIPCHILDREN & WS_CLIPSIBLINGS.
        //You can find references about this within the knowledge base and most OpenGL
        //books that contain the wgl function descriptions.
        //*/
        //WXDWORD exStyle = 0;
        //DWORD msflags = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

        ////https://github.com/cztomczak/cefpython/issues/152
        ////apparemment si on vire wxTAB_TRAVERSAL, on catch comme il faut le wxEVT_CHAR sinon NON
        //// faudrais trouver un autre moyen d'editer des flags
        //// la j'ai fait au plus simple et virant le style windows herit� et ca marche du feu de dieu
        //msflags |= MSWGetStyle(style, &exStyle);

        //// voila pour enlever : http://stackoverflow.com/questions/3920307/how-can-i-remove-a-flag-in-c
        //msflags &= ~wxTAB_TRAVERSAL;
        //exStyle &= ~WS_EX_CONTROLPARENT;

        //// finalement c'est pas le wxTAB_TRAVERSAL qui merde c'est le WS_EX_CONTROLPARENT
        //// et en fait c'est logique car le wxTAB_TRAVERSAL qui est un flag wxWidget declenche
        //// en fait le WS_EX_CONTROLPARENT qui est un flag windows

        //if (!MSWCreate(wxApp::GetRegisteredClassName(wxT("wxImGuiCanvas"), -1, CS_OWNDC),
        //	NULL, pos, size, msflags, exStyle))
        //	return false;

        ////m_hDC = ::GetDC(GetHWND());
        ////if (!m_hDC)
        ////	return false;

        //if (m_GLContext != 0)
        //{
        //	//if (m_GLContext->ConfigurePixelFormat(m_hDC))
        //	//{
        //	//	if (SetCurrent(m_hDC))
        //	//	{
        //	//		// Setup ImGui binding
        //	//		ImGui_ImplWx_Init(this);

        //	//		// Load Fonts
        //	//		// (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
        //	//		//ImGuiIO& io = ImGui::GetIO();
        //	//		//io.Fonts->AddFontDefault();
        //	//		//io.Fonts->AddFontFromFileTTF("extra_fonts/Cousine-Regular.ttf", 15.0f);
        //	//		//io.Fonts->AddFontFromFileTTF("extra_fonts/DroidSans.ttf", 16.0f);
        //	//		//io.Fonts->AddFontFromFileTTF("extra_fonts/ProggyClean.ttf", 13.0f);
        //	//		//io.Fonts->AddFontFromFileTTF("extra_fonts/ProggyTiny.ttf", 10.0f);
        //	//		//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

        //	//		//m_timer.Start(15.0f, false);    // 6 correspond � 1000/6 = 166 fps max. ca suffira

        //	//		activateRenderLoop(true);
        //	//	}
        //	//}
        //}
        //else
        //{
        //	//if (CreateGLContext(m_hDC))
        //	//{
        //	//	SetCurrent(m_hDC);

        //	//	// Setup ImGui binding
        //	//	ImGui_ImplWx_Init(this);

        //	//	// Load Fonts
        //	//	// (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
        //	//	//ImGuiIO& io = ImGui::GetIO();
        //	//	//io.Fonts->AddFontDefault();
        //	//	//io.Fonts->AddFontFromFileTTF("extra_fonts/Cousine-Regular.ttf", 15.0f);
        //	//	//io.Fonts->AddFontFromFileTTF("extra_fonts/DroidSans.ttf", 16.0f);
        //	//	//io.Fonts->AddFontFromFileTTF("extra_fonts/ProggyClean.ttf", 13.0f);
        //	//	//io.Fonts->AddFontFromFileTTF("extra_fonts/ProggyTiny.ttf", 10.0f);
        //	//	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

        //	//	//m_timer.Start(15.0f, false);    // 6 correspond � 1000/6 = 166 fps max. ca suffira

        //	//	activateRenderLoop(true);
        //	//}
        //}

        return true;
    }

    wxImGuiCanvas::~wxImGuiCanvas()
    {
        m_timer.Stop();

        //if (m_GLContext == 0)
        //{
        //	SetCurrent(m_hDC);
        //	DestroyGLContext(m_hDC);
        //}
    }

    //bool wxImGuiCanvas::CreateGLContext(HDC vHDC)
    //{
    //	// https://www.opengl.org/wiki/Creating_an_OpenGL_Context_(WGL)
    //	PIXELFORMATDESCRIPTOR pfd =
    //	{
    //		sizeof(PIXELFORMATDESCRIPTOR),
    //		1,
    //		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
    //		PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
    //		32,                        //Colordepth of the framebuffer.
    //		0, 0, 0, 0, 0, 0,
    //		0,
    //		0,
    //		0,
    //		0, 0, 0, 0,
    //		24,                        //Number of bits for the depthbuffer
    //		8,                        //Number of bits for the stencilbuffer
    //		0,                        //Number of Aux buffers in the framebuffer.
    //		PFD_MAIN_PLANE,
    //		0,
    //		0, 0, 0
    //	};

    //	int nPixelFormat = ChoosePixelFormat(vHDC, &pfd);

    //	if (nPixelFormat == 0) return false;

    //	BOOL bResult = SetPixelFormat(vHDC, nPixelFormat, &pfd);

    //	if (!bResult) return false;

    //	HGLRC tempContext = wglCreateContext(vHDC);
    //	wglMakeCurrent(vHDC, tempContext);

    //	//GLenum err = glewInit();
    //	//if (GLEW_OK != err)
    //	//{
    //	//	LogStr(_T("GLEW is not initialized!"));
    //	//}

    //	//static const int opengl_4_5[] =
    //	//{
    //	//	WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
    //	//	WGL_CONTEXT_MINOR_VERSION_ARB, 5,
    //	//	WGL_CONTEXT_FLAGS_ARB, 0,
    //	//	0
    //	//};
    //	//static const int opengl_4_4[] =
    //	//{
    //	//	WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
    //	//	WGL_CONTEXT_MINOR_VERSION_ARB, 4,
    //	//	WGL_CONTEXT_FLAGS_ARB, 0,
    //	//	0
    //	//};

    //	/*
    //	static const int opengl_4_3[] =
    //	{
    //		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
    //		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
    //		WGL_CONTEXT_FLAGS_ARB, 0,
    //		0
    //	};
    //	static const int opengl_4_2[] =
    //	{
    //		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
    //		WGL_CONTEXT_MINOR_VERSION_ARB, 2,
    //		WGL_CONTEXT_FLAGS_ARB, 0,
    //		0
    //	};
    //	static const int opengl_4_1[] =
    //	{
    //		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
    //		WGL_CONTEXT_MINOR_VERSION_ARB, 1,
    //		WGL_CONTEXT_FLAGS_ARB, 0,
    //		0
    //	};
    //	static const int opengl_4_0[] =
    //	{
    //		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
    //		WGL_CONTEXT_MINOR_VERSION_ARB, 0,
    //		WGL_CONTEXT_FLAGS_ARB, 0,
    //		0
    //	};
    //	static const int opengl_3_3[] =
    //	{
    //		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    //		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
    //		WGL_CONTEXT_FLAGS_ARB, 0,
    //		0
    //	};
    //	static const int opengl_3_2[] =
    //	{
    //		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    //		WGL_CONTEXT_MINOR_VERSION_ARB, 2,
    //		WGL_CONTEXT_FLAGS_ARB, 0,
    //		0
    //	};
    //	static const int opengl_3_1[] =
    //	{
    //		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    //		WGL_CONTEXT_MINOR_VERSION_ARB, 1,
    //		WGL_CONTEXT_FLAGS_ARB, 0,
    //		0
    //	};
    //	static const int opengl_3_0[] = {
    //		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    //		WGL_CONTEXT_MINOR_VERSION_ARB, 0,
    //		WGL_CONTEXT_FLAGS_ARB, 0,
    //		0,
    //	};
    //	static const int opengl_2_1[] = {
    //		WGL_CONTEXT_MAJOR_VERSION_ARB, 2,
    //		WGL_CONTEXT_MINOR_VERSION_ARB, 1,
    //		WGL_CONTEXT_FLAGS_ARB, 0,
    //		0,
    //	};
    //	static const int opengl_2_0[] = {
    //		WGL_CONTEXT_MAJOR_VERSION_ARB, 2,
    //		WGL_CONTEXT_MINOR_VERSION_ARB, 0,
    //		WGL_CONTEXT_FLAGS_ARB, 0,
    //		0,
    //	};
    //	*/

    //	//if (wglewIsSupported("WGL_ARB_create_context") == 1)
    //	//{
    //	//	//m_hrc = wglCreateContextAttribsARB(vHDC, 0, opengl_4_5);
    //	//	//if (m_hrc == 0)
    //	//	//	m_hrc = wglCreateContextAttribsARB(vHDC, 0, opengl_4_4);
    //	//	//if (m_hrc == 0)
    //	//	//	m_hrc = wglCreateContextAttribsARB(vHDC, 0, opengl_4_3);
    //	//	//if (m_hrc == 0)
    //	//	//	m_hrc = wglCreateContextAttribsARB(vHDC, 0, opengl_4_2);
    //	//	//if (m_hrc == 0)
    //	//	//	m_hrc = wglCreateContextAttribsARB(vHDC, 0, opengl_4_1);
    //	//	//if (m_hrc == 0)
    //	//	//	m_hrc = wglCreateContextAttribsARB(vHDC, 0, opengl_4_0);
    //	//	//if (m_hrc == 0)
    //	//	//	m_hrc = wglCreateContextAttribsARB(vHDC, 0, opengl_3_3);
    //	//	//if (m_hrc == 0)
    //	//	//	m_hrc = wglCreateContextAttribsARB(vHDC, 0, opengl_3_2);
    //	//	//if (m_hrc == 0)
    //	//	m_hrc = wglCreateContextAttribsARB(vHDC, 0, opengl_3_1);
    //	//	if (m_hrc == 0)
    //	//		m_hrc = wglCreateContextAttribsARB(vHDC, 0, opengl_3_0);
    //	//	if (m_hrc == 0)
    //	//		m_hrc = wglCreateContextAttribsARB(vHDC, 0, opengl_2_1);
    //	//	if (m_hrc == 0)
    //	//		m_hrc = wglCreateContextAttribsARB(vHDC, 0, opengl_2_0);

    //	//	wglMakeCurrent(NULL, NULL);
    //	//	wglDeleteContext(tempContext);
    //	//	wglMakeCurrent(vHDC, m_hrc);
    //	//}
    //	//else
    //	//{	//It's not possible to make a GL 3.x context. Use the old style context (GL 2.1 and before)
    //	//	m_hrc = tempContext;
    //	//}

    //	//if (!m_hrc)
    //	//{
    //	//	LogStr("Context Opengl FAIL !");
    //	//	return false;
    //	//}

    //	return true;
    //}

    //void wxImGuiCanvas::DestroyGLContext(HDC vHDC)
    //{
    //	if (m_hrc)
    //	{
    //		wglMakeCurrent(vHDC, m_hrc);

    //		wglDeleteContext(m_hrc);
    //		m_hrc = NULL;

    //		wglMakeCurrent(vHDC, NULL);
    //		wglMakeCurrent(NULL, NULL);
    //	}
    //}

    //bool wxImGuiCanvas::SetCurrent(HDC vHDC)
    //{
    //	bool res = true;

    //	//if (m_GLContext != 0)
    //	//{
    //	//	m_GLContext->SetCurrent(vHDC);
    //	//}
    //	//else
    //	//{
    //	//	// making gl context current
    //	//	if (wglMakeCurrent(vHDC, m_hrc) == false)
    //	//	{
    //	//		string str = string("wglMakeCurrent Failed %x\r\n", GetLastError());
    //	//		LogStr(str);
    //	//		res = false;
    //	//	}
    //	//}

    //	return res;
    //}

    void wxImGuiCanvas::activateRenderLoop( bool vOn )
    {
        if( vOn && !m_RenderLoopOn )
        {
            Connect( wxID_ANY, wxEVT_IDLE, wxIdleEventHandler( wxImGuiCanvas::onIdle ) );
            m_RenderLoopOn = true;
        }
        else if( !vOn && m_RenderLoopOn )
        {
            Disconnect( wxEVT_IDLE, wxIdleEventHandler( wxImGuiCanvas::onIdle ) );
            m_RenderLoopOn = false;
        }
    }

    void wxImGuiCanvas::onIdle( wxIdleEvent & evt )
    {
        if( m_RenderLoopOn )
        {
            Refresh( false, 0 );  // raffraichit la vue � la frequence du timer
            evt.RequestMore();    // render continuously, not only once on idle
        }
    }

    void wxImGuiCanvas::OnPaint( wxPaintEvent & /*event*/ )
    {
        //SetCurrent(m_hDC);

        // Setup display size (every frame to accommodate for window resizing)
        //io.DisplaySize = viewSize.ToImVec2();
        // Setup time step
        //io.DeltaTime = dt;

        ImGui::GetIO().MouseDrawCursor = false;

        // Start the frame
        ImGui_ImplWx_NewFrame( this );

        OnDrawImGui();

        //// Rendering
        //glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        //glClearColor(0, 0, 0, 1);
        //glClear(GL_COLOR_BUFFER_BIT);

        ImGui::Render();

        //glFlush();
        //SwapBuffers(m_hDC);
    }

    void wxImGuiCanvas::OnSize( wxSizeEvent & event )
    {
        if( !IsShownOnScreen() )
            return;

        wxSize size = event.GetSize();
        m_ScreenSize = Vector2F( size.x, size.y );
    }

    void wxImGuiCanvas::OnKeyDown( wxKeyEvent & event )
    {
        ImGuiIO &io = ImGui::GetIO();
        io.KeysDown[event.GetKeyCode()] = true;

        io.KeyCtrl = event.ControlDown();
        io.KeyShift = event.ShiftDown();
        io.KeyAlt = event.AltDown();

        event.Skip();
    }

    void wxImGuiCanvas::OnKeyUp( wxKeyEvent & event )
    {
        ImGuiIO &io = ImGui::GetIO();
        io.KeysDown[event.GetKeyCode()] = false;

        io.KeyCtrl = event.ControlDown();
        io.KeyShift = event.ShiftDown();
        io.KeyAlt = event.AltDown();

        event.Skip();
    }

    void wxImGuiCanvas::OnChar( wxKeyEvent & event )
    {
        wxChar uc = event.GetUnicodeKey();
        if( uc != WXK_NONE )
        {
            ImGui::GetIO().AddInputCharacter( uc );
        }
        else  // No Unicode equivalent.
        {
            unsigned int c = event.GetRawKeyCode();
            if( c > 0 && c < 1000 )
                ImGui::GetIO().AddInputCharacter( c );
        }

        event.DoAllowNextEvent();

        event.Skip();
    }

    void wxImGuiCanvas::OnMouseEvent( wxMouseEvent & event )
    {
        if( event.LeftIsDown() )
            g_MousePressed[0] = event.LeftIsDown();
        if( event.MiddleIsDown() )
            g_MousePressed[1] = event.MiddleIsDown();
        if( event.RightIsDown() )
            g_MousePressed[2] = event.RightIsDown();

        if( event.GetWheelAxis() == wxMOUSE_WHEEL_VERTICAL )
            g_MouseWheel += event.GetWheelDelta() * event.GetWheelRotation();

        g_MousePos[0] = event.GetX();
        g_MousePos[1] = event.GetY();

        event.Skip();
    }

    void wxImGuiCanvas::OnTimer( wxTimerEvent & /*event*/ )
    {
        Refresh( false, 0 );  // raffraichit la vue � la frequence du timer
    }

    // https://github.com/ocornut/imgui // based on glfw sample
    bool wxImGuiCanvas::ImGui_ImplWx_Init( wxWindow * window )
    {
        g_Window = window;

        ImGuiIO &io = ImGui::GetIO();
        io.KeyMap[ImGuiKey_Tab] =
            WXK_TAB;  // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
        io.KeyMap[ImGuiKey_LeftArrow] = WXK_LEFT;
        io.KeyMap[ImGuiKey_RightArrow] = WXK_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow] = WXK_UP;
        io.KeyMap[ImGuiKey_DownArrow] = WXK_DOWN;
        io.KeyMap[ImGuiKey_PageUp] = WXK_PAGEUP;
        io.KeyMap[ImGuiKey_PageDown] = WXK_PAGEDOWN;
        io.KeyMap[ImGuiKey_Home] = WXK_HOME;
        io.KeyMap[ImGuiKey_End] = WXK_END;
        io.KeyMap[ImGuiKey_Delete] = WXK_DELETE;
        io.KeyMap[ImGuiKey_Backspace] = WXK_BACK;
        io.KeyMap[ImGuiKey_Enter] = WXK_RETURN;
        io.KeyMap[ImGuiKey_Escape] = WXK_ESCAPE;
        io.KeyMap[ImGuiKey_A] = WXK_CONTROL_A;
        io.KeyMap[ImGuiKey_C] = WXK_CONTROL_C;
        io.KeyMap[ImGuiKey_V] = WXK_CONTROL_V;
        io.KeyMap[ImGuiKey_X] = WXK_CONTROL_X;
        io.KeyMap[ImGuiKey_Y] = WXK_CONTROL_Y;
        io.KeyMap[ImGuiKey_Z] = WXK_CONTROL_Z;

        //io.RenderDrawListsFn = ImGui_ImplWx_RenderDrawLists;      // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
        //io.SetClipboardTextFn = ImGui_ImplWx_SetClipboardText;
        //io.GetClipboardTextFn = ImGui_ImplWx_GetClipboardText;

#ifdef _WIN32
        io.ImeWindowHandle = window->GetHWND();
#endif

        /*ImFontConfig config;
        config.OversampleH = 2;
        config.OversampleV = 2;
        config.GlyphExtraSpacing.x = 1.0f;
        io.Fonts->AddFontFromFileTTF("comicbd.ttf", 17, &config);*/

        ImGuiStyle &style = ImGui::GetStyle();
        style.Colors[ImGuiCol_Text] = ImVec4( 0.85f, 0.85f, 0.85f, 1.00f );
        style.Colors[ImGuiCol_WindowBg] = ImVec4( 0.15f, 0.16f, 0.17f, 1.00f );
        //style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.15f, 0.16f, 0.17f, 1.00f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4( 0.15f, 0.16f, 0.17f, 1.00f );
        style.Colors[ImGuiCol_Border] = ImVec4( 1.00f, 1.00f, 1.00f, 0.65f );
        style.Colors[ImGuiCol_BorderShadow] = ImVec4( 1.00f, 1.00f, 1.00f, 0.56f );
        style.Colors[ImGuiCol_FrameBg] = ImVec4( 0.21f, 0.29f, 0.36f, 1.00f );
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4( 0.13f, 0.52f, 0.94f, 0.20f );
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4( 0.13f, 0.52f, 0.88f, 1.00f );
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4( 0.13f, 0.52f, 0.78f, 1.00f );
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4( 0.13f, 0.52f, 1.00f, 0.40f );
        //style.Colors[ImGuiCol_ComboBg] = ImVec4(0.21f, 0.29f, 0.36f, 1.00f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4( 1.00f, 0.60f, 0.00f, 1.00f );
        style.Colors[ImGuiCol_Button] = ImVec4( 1.00f, 0.60f, 0.00f, 0.8f );
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4( 1.00f, 0.48f, 0.00f, 0.8f );
        style.Colors[ImGuiCol_ButtonActive] = ImVec4( 1.00f, 0.40f, 0.00f, 0.8f );
        style.Colors[ImGuiCol_Header] = ImVec4( 0.13f, 0.52f, 0.94f, 0.78f );
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4( 0.13f, 0.52f, 0.94f, 1.00f );
        style.Colors[ImGuiCol_HeaderActive] = ImVec4( 0.13f, 0.52f, 0.94f, 0.59f );
        style.Colors[ImGuiCol_TitleBg] = ImVec4( 0.13f, 0.52f, 0.94f, 0.78f );
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4( 0.13f, 0.52f, 0.94f, 1.00f );
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4( 0.13f, 0.52f, 0.94f, 0.59f );
        //style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.13f);

        style.WindowPadding = ImVec2( 7, 7 );
        style.WindowRounding = 7.0f;
        //style.WindowMinSize = ImVec2(100,100);
        //style.ChildWindowRounding = 0.0f;
        style.FramePadding = ImVec2( 5, 4 );  // Padding within a framed rectangle (used by most widgets)
        style.FrameRounding =
            3.0f;  // Radius of frame corners rounding. Set to 0.0f to have rectangular frames (used by most widgets).
        //style.ItemSpacing = ImVec2(8, 4);      // Horizontal and vertical spacing between widgets/lines
        style.ItemInnerSpacing = ImVec2(
            6,
            4 );  // Horizontal and vertical spacing between within elements of a composed widget (e.g. a slider and its label)
        //style.TouchExtraPadding = ImVec2(0, 0);      // Expand reactive bounding box for touch-based system where touch position is not accurate enough. Unfortunately we don't sort widgets so priority on overlap will always be given to the first widget. So don't grow this too much!
        style.IndentSpacing =
            12.0f;  // Horizontal spacing when e.g. entering a tree node. Generally == (FontSize + FramePadding.x*2).
        //style.ColumnsMinSpacing = 6.0f;             // Minimum horizontal spacing between two columns
        style.ScrollbarSize =
            20.0f;  // Width of the vertical scrollbar, Height of the horizontal scrollbar
        //style.ScrollbarRounding = 9.0f;             // Radius of grab corners rounding for scrollbar
        style.GrabMinSize = 9.0f;  // Minimum width/height of a grab box for slider/scrollbar
        style.GrabRounding =
            16.0f;  // Radius of grabs corners rounding. Set to 0.0f to have rectangular slider grabs.
        //style.DisplayWindowPadding = ImVec2(22, 22);    // Window positions are clamped to be visible within the display area by at least this amount. Only covers regular windows.
        //style.DisplaySafeAreaPadding = ImVec2(4, 4);      // If you cannot see the edge of your screen (e.g. on a TV) increase the safe area padding. Covers popups/tooltips as well regular windows.
        style.AntiAliasedLines =
            true;  // Enable anti-aliasing on lines/borders. Disable if you are really short on CPU/GPU.
        //style.AntiAliasedShapes = true;             // Enable anti-aliasing on filled shapes (rounded rectangles, circles, etc.)
        //style.CurveTessellationTol = 1.25f;
        return true;
    }

    void wxImGuiCanvas::ImGui_ImplWx_Shutdown()
    {
        ImGui_ImplWx_InvalidateDeviceObjects();
        //ImGui::Shutdown();
    }

    void wxImGuiCanvas::ImGui_ImplWx_NewFrame( wxWindow * window )
    {
        //cAssert(window != 0, "Erreur : window = 0");

        if( !g_FontTexture )
            ImGui_ImplWx_CreateDeviceObjects();

        ImGuiIO &io = ImGui::GetIO();

        // Setup display size (every frame to accommodate for window resizing)
        int w, h;
        int display_w, display_h;
        wxRect rc = window->GetClientRect();
        w = rc.width;
        display_w = w;
        h = rc.height;
        display_h = h;
        io.DisplaySize = ImVec2( (float)w, (float)h );
        io.DisplayFramebufferScale =
            ImVec2( w > 0 ? ( (float)display_w / w ) : 0, h > 0 ? ( (float)display_h / h ) : 0 );

        // Setup time step
        double time = 01;  // g_Time.get() / 1000.0f;
        io.DeltaTime = time > 0.0 ? (float)( time ) : (float)( 1.0f / wxIMGUI_FPS );
        //g_Time.Fix();

        if( window->HasFocus() )
            io.MousePos = ImVec2( g_MousePos[0], g_MousePos[1] );
        else
            io.MousePos = ImVec2( -1, -1 );

        wxMouseState mouseStat = wxGetMouseState();
        io.MouseDown[0] = g_MousePressed[0] || mouseStat.LeftIsDown();
        io.MouseDown[1] = g_MousePressed[1] || mouseStat.RightIsDown();
        io.MouseDown[2] = g_MousePressed[2] || mouseStat.MiddleIsDown();
        g_MousePressed[0] = g_MousePressed[1] = g_MousePressed[2] = false;

        io.MouseWheel = g_MouseWheel;
        g_MouseWheel = 0.0f;

        //io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        //io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        //io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
        //io.KeySuper = false;

        //// Hide OS mouse cursor if ImGui is drawing it
        //ShowCursor(io.MouseDrawCursor ? 0 : 1);

        // Start the frame
        ImGui::NewFrame();
    }

    // Use if you want to reset your rendering device without losing ImGui state.
    void wxImGuiCanvas::ImGui_ImplWx_InvalidateDeviceObjects()
    {
        //if (g_FontTexture)
        //{
        //	glDeleteTextures(1, &g_FontTexture);
        //	ImGui::GetIO().Fonts->TexID = 0;
        //	g_FontTexture = 0;
        //}
    }
    bool wxImGuiCanvas::ImGui_ImplWx_CreateDeviceObjects()
    {
        //// Build texture atlas
        //ImGuiIO& io = ImGui::GetIO();
        //unsigned char* pixels;
        //int width, height;
        //io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

        //// Upload texture to graphics system
        //GLint last_texture;
        //glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
        //glGenTextures(1, &g_FontTexture);
        //glBindTexture(GL_TEXTURE_2D, g_FontTexture);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        //glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);

        //// Store our identifier
        //io.Fonts->TexID = (void*)(intptr_t)g_FontTexture;

        //// Restore state
        //glBindTexture(GL_TEXTURE_2D, last_texture);

        return true;
    }

    // This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
    // If text or lines are blurry when integrating ImGui in your engine:
    // - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
    void wxImGuiCanvas::ImGui_ImplWx_RenderDrawLists( ImDrawData * draw_data )
    {
        /*
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        ImGuiIO& io = ImGui::GetIO();
        int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
        int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
        if (fb_width == 0 || fb_height == 0)
            return;
        draw_data->ScaleClipRects(io.DisplayFramebufferScale);

        // We are using the OpenGL fixed pipeline to make the example code simpler to read!
        // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers.
        GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
        GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
        GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
        glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_SCISSOR_TEST);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glEnable(GL_TEXTURE_2D);
        //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context

        // Setup viewport, orthographic projection matrix
        glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, +1.0f);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // Render command lists
#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
            const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;
            glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + OFFSETOF(ImDrawVert, pos)));
            glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + OFFSETOF(ImDrawVert, uv)));
            glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + OFFSETOF(ImDrawVert, col)));

            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback)
                {
                    pcmd->UserCallback(cmd_list, pcmd);
                }
                else
                {
                    glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                    glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                    glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer);
                }
                idx_buffer += pcmd->ElemCount;
            }
        }
#undef OFFSETOF

        // Restore modified state
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glBindTexture(GL_TEXTURE_2D, (GLuint)last_texture);
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glPopAttrib();
        glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
        glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
        */
    }

    const char *wxImGuiCanvas::ImGui_ImplWx_GetClipboardText( void *user_data )
    {
        wxTextDataObject data;

        // Read some text
        if( wxTheClipboard->Open() )
        {
            if( wxTheClipboard->IsSupported( wxDF_TEXT ) )
            {
                wxTheClipboard->GetData( data );
            }
            wxTheClipboard->Close();
        }

        return data.GetText().c_str();
    }

    void wxImGuiCanvas::ImGui_ImplWx_SetClipboardText( void *user_data, const char *text )
    {
        // Write some text to the clipboard
        if( wxTheClipboard->Open() )
        {
            // This data objects are held by the clipboard,
            // so do not delete them in the app.
            wxTheClipboard->SetData( new wxTextDataObject( text ) );
            wxTheClipboard->Close();
        }
    }

    void wxImGuiCanvas::OnDrawImGui()
    {
        ImGui::Begin( "MainToolBox", (bool *)0,
                      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                          ImGuiWindowFlags_NoSavedSettings );

        auto size = m_ScreenSize;

        ImGui::SetWindowSize( ImVec2( 0.0f, size.Y() ) );
        ImGui::SetWindowPos( ImVec2( 0.0f, 0.0f ) );

        ImGui::Separator();
        ImGui::Text( "Add Item :" );

        ImGui::Button( "toto va au zoo" );

        ImGui::End();
    }

    //HDC wxImGuiCanvas::GetHDC()
    //{
    //	return m_hDC;
    //}

    Vector2F wxImGuiCanvas::GetCanvasSize()
    {
        return m_ScreenSize;
    }

    //HDC wxImGuiCanvas::GetHDC() const
    //{
    //	return m_hDC;
    //}
}
