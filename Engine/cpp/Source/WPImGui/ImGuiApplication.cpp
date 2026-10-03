#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiApplication.hpp>
#include <WPImGui/WPImGuiStyle.hpp>
#include <WPImGui/ImGuiMenuItem.hpp>
#include <WPImGui/ImGuiMenuBar.hpp>
#include <WPImGui/ImGuiMenu.hpp>
#include <Workphone/Workphone.hpp>
#include "ImGuizmo.hpp"
#include "ImSequencer.hpp"
//#include "ImZoomSlider.hpp"
#include "ImCurveEdit.hpp"
#include "GraphEditor.hpp"
#include "ImGuiManager.hpp"
#include "ImGuiPropertyGrid.hpp"
#include "ImGuiTreeCtrl.hpp"
#include "ImGuiTreeNode.hpp"
#include "ImGuiUtil.hpp"
#include "imgui_internal.h"
#include <cmath>
#include <vector>
#include <algorithm>

#if defined WP_PLATFORM_APPLE
#    include <WPImGui/Apple/ImGuiApplicationOSX.hpp>
#endif

extern void ShowExampleAppDockSpace( bool *p_open );

#if defined WP_PLATFORM_WIN32
// Copy this line into your .cpp file to forward declare the function.
extern LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
#endif

// extern void ImGui_ImplDX11_SetWindowSize(ImGuiViewport* viewport, ImVec2 size);

#ifdef WP_PLATFORM_WIN32
#    include "minwindef.h"
extern ImGuiKey ImGui_ImplWin32_VirtualKeyToImGuiKey( WPARAM wParam );
extern void ImGui_ImplWin32_AddKeyEvent( ImGuiKey key, bool down, int native_keycode,
                                         int native_scancode = -1 );
#endif

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiApplication, IUIApplication );

    Array<u8> ImGuiApplication::s_fontData;
    ImFont *ImGuiApplication::s_fontAwesomeFont = nullptr;

    class Test
    {
    public:
        enum class eImGuiKey
        {
            // Keyboard
            ImGuiKey_None = 0,
            ImGuiKey_Tab = 512,
            // == ImGuiKey_NamedKey_BEGIN
            ImGuiKey_LeftArrow,
            ImGuiKey_RightArrow,
            ImGuiKey_UpArrow,
            ImGuiKey_DownArrow,
            ImGuiKey_PageUp,
            ImGuiKey_PageDown,
            ImGuiKey_Home,
            ImGuiKey_End,
            ImGuiKey_Insert,
            ImGuiKey_Delete,
            ImGuiKey_Backspace,
            ImGuiKey_Space,
            ImGuiKey_Enter,
            ImGuiKey_Escape,
            ImGuiKey_LeftCtrl,
            ImGuiKey_LeftShift,
            ImGuiKey_LeftAlt,
            ImGuiKey_LeftSuper,
            ImGuiKey_RightCtrl,
            ImGuiKey_RightShift,
            ImGuiKey_RightAlt,
            ImGuiKey_RightSuper,
            ImGuiKey_Menu,
            ImGuiKey_0,
            ImGuiKey_1,
            ImGuiKey_2,
            ImGuiKey_3,
            ImGuiKey_4,
            ImGuiKey_5,
            ImGuiKey_6,
            ImGuiKey_7,
            ImGuiKey_8,
            ImGuiKey_9,
            ImGuiKey_A,
            ImGuiKey_B,
            ImGuiKey_C,
            ImGuiKey_D,
            ImGuiKey_E,
            ImGuiKey_F,
            ImGuiKey_G,
            ImGuiKey_H,
            ImGuiKey_I,
            ImGuiKey_J,
            ImGuiKey_K,
            ImGuiKey_L,
            ImGuiKey_M,
            ImGuiKey_N,
            ImGuiKey_O,
            ImGuiKey_P,
            ImGuiKey_Q,
            ImGuiKey_R,
            ImGuiKey_S,
            ImGuiKey_T,
            ImGuiKey_U,
            ImGuiKey_V,
            ImGuiKey_W,
            ImGuiKey_X,
            ImGuiKey_Y,
            ImGuiKey_Z,
            ImGuiKey_F1,
            ImGuiKey_F2,
            ImGuiKey_F3,
            ImGuiKey_F4,
            ImGuiKey_F5,
            ImGuiKey_F6,
            ImGuiKey_F7,
            ImGuiKey_F8,
            ImGuiKey_F9,
            ImGuiKey_F10,
            ImGuiKey_F11,
            ImGuiKey_F12,
            ImGuiKey_Apostrophe,
            // '
            ImGuiKey_Comma,
            // ,
            ImGuiKey_Minus,
            // -
            ImGuiKey_Period,
            // .
            ImGuiKey_Slash,
            // /
            ImGuiKey_Semicolon,
            // ;
            ImGuiKey_Equal,
            // =
            ImGuiKey_LeftBracket,
            // [
            ImGuiKey_Backslash,
            // \ (this text inhibit multiline comment caused by backslash)
            ImGuiKey_RightBracket,
            // ]
            ImGuiKey_GraveAccent,
            // `
            ImGuiKey_CapsLock,
            ImGuiKey_ScrollLock,
            ImGuiKey_NumLock,
            ImGuiKey_PrintScreen,
            ImGuiKey_Pause,
            ImGuiKey_Keypad0,
            ImGuiKey_Keypad1,
            ImGuiKey_Keypad2,
            ImGuiKey_Keypad3,
            ImGuiKey_Keypad4,
            ImGuiKey_Keypad5,
            ImGuiKey_Keypad6,
            ImGuiKey_Keypad7,
            ImGuiKey_Keypad8,
            ImGuiKey_Keypad9,
            ImGuiKey_KeypadDecimal,
            ImGuiKey_KeypadDivide,
            ImGuiKey_KeypadMultiply,
            ImGuiKey_KeypadSubtract,
            ImGuiKey_KeypadAdd,
            ImGuiKey_KeypadEnter,
            ImGuiKey_KeypadEqual,

            // Gamepad (some of those are analog values, 0.0f to 1.0f) // NAVIGATION action
            ImGuiKey_GamepadStart,
            // Menu (Xbox)          + (Switch)   Start/Options (PS) // --
            ImGuiKey_GamepadBack,
            // View (Xbox)          - (Switch)   Share (PS)         // --
            ImGuiKey_GamepadFaceUp,
            // Y (Xbox)             X (Switch)   Triangle (PS)      // ->
            // ImGuiNavInput_Input
            ImGuiKey_GamepadFaceDown,
            // A (Xbox)             B (Switch)   Cross (PS)         // ->
            // ImGuiNavInput_Activate
            ImGuiKey_GamepadFaceLeft,
            // X (Xbox)             Y (Switch)   Square (PS)        // ->
            // ImGuiNavInput_Menu
            ImGuiKey_GamepadFaceRight,
            // B (Xbox)             A (Switch)   Circle (PS)        // ->
            // ImGuiNavInput_Cancel
            ImGuiKey_GamepadDpadUp,
            // D-pad Up                                             // ->
            // ImGuiNavInput_DpadUp
            ImGuiKey_GamepadDpadDown,
            // D-pad Down                                           // ->
            // ImGuiNavInput_DpadDown
            ImGuiKey_GamepadDpadLeft,
            // D-pad Left                                           // ->
            // ImGuiNavInput_DpadLeft
            ImGuiKey_GamepadDpadRight,
            // D-pad Right                                          // ->
            // ImGuiNavInput_DpadRight
            ImGuiKey_GamepadL1,
            // L Bumper (Xbox)      L (Switch)   L1 (PS)            // ->
            // ImGuiNavInput_FocusPrev + ImGuiNavInput_TweakSlow
            ImGuiKey_GamepadR1,
            // R Bumper (Xbox)      R (Switch)   R1 (PS)            // ->
            // ImGuiNavInput_FocusNext + ImGuiNavInput_TweakFast
            ImGuiKey_GamepadL2,
            // L Trigger (Xbox)     ZL (Switch)  L2 (PS) [Analog]
            ImGuiKey_GamepadR2,
            // R Trigger (Xbox)     ZR (Switch)  R2 (PS) [Analog]
            ImGuiKey_GamepadL3,
            // L Thumbstick (Xbox)  L3 (Switch)  L3 (PS)
            ImGuiKey_GamepadR3,
            // R Thumbstick (Xbox)  R3 (Switch)  R3 (PS)
            ImGuiKey_GamepadLStickUp,
            // [Analog]                                             // ->
            // ImGuiNavInput_LStickUp
            ImGuiKey_GamepadLStickDown,
            // [Analog]                                             //
            // -> ImGuiNavInput_LStickDown
            ImGuiKey_GamepadLStickLeft,
            // [Analog]                                             //
            // -> ImGuiNavInput_LStickLeft
            ImGuiKey_GamepadLStickRight,
            // [Analog]                                             //
            // -> ImGuiNavInput_LStickRight
            ImGuiKey_GamepadRStickUp,
            // [Analog]
            ImGuiKey_GamepadRStickDown,
            // [Analog]
            ImGuiKey_GamepadRStickLeft,
            // [Analog]
            ImGuiKey_GamepadRStickRight,
            // [Analog]

            // Keyboard Modifiers (explicitly submitted by backend via AddKeyEvent() calls)
            // - This is mirroring the data also written to io.KeyCtrl, io.KeyShift, io.KeyAlt,
            // io.KeySuper, in a format allowing
            //   them to be accessed via standard key API, allowing calls such as IsKeyPressed(),
            //   IsKeyReleased(), querying duration etc.
            // - Code polling every keys (e.g. an interface to detect a key press for input mapping)
            // might want to ignore those
            //   and prefer using the real keys (e.g. ImGuiKey_LeftCtrl, ImGuiKey_RightCtrl instead
            //   of ImGuiKey_ModCtrl).
            // - In theory the value of keyboard modifiers should be roughly equivalent to a logical
            // or of the equivalent left/right keys.
            //   In practice: it's complicated; mods are often provided from different sources.
            //   Keyboard layout, IME, sticky keys and backends tend to interfere and break that
            //   equivalence. The safer decision is to relay that ambiguity down to the end-user...
            ImGuiKey_ModCtrl,
            ImGuiKey_ModShift,
            ImGuiKey_ModAlt,
            ImGuiKey_ModSuper,

            // End of list
            ImGuiKey_COUNT,
            // No valid ImGuiKey is ever greater than this value

            // [Internal] Prior to 1.87 we required user to fill io.KeysDown[512] using their own
            // native index + a io.KeyMap[] array. We are ditching this method but keeping a legacy
            // path for user code doing e.g. IsKeyPressed(MY_NATIVE_KEY_CODE)
            ImGuiKey_NamedKey_BEGIN = 512,
            ImGuiKey_NamedKey_END = ImGuiKey_COUNT,
            ImGuiKey_NamedKey_COUNT = ImGuiKey_NamedKey_END - ImGuiKey_NamedKey_BEGIN,
#ifdef IMGUI_DISABLE_OBSOLETE_KEYIO
            ImGuiKey_KeysData_SIZE =
                ImGuiKey_NamedKey_COUNT,  // Size of KeysData[]: only hold named keys
            ImGuiKey_KeysData_OFFSET = ImGuiKey_NamedKey_BEGIN  // First key stored in KeysData[0]
#else
            ImGuiKey_KeysData_SIZE = ImGuiKey_COUNT,
            // Size of KeysData[]: hold legacy 0..512 keycodes + named keys
            ImGuiKey_KeysData_OFFSET = 0  // First key stored in KeysData[0]
#endif

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
            ,
            ImGuiKey_KeyPadEnter = ImGuiKey_KeypadEnter  // Renamed in 1.87
#endif
        };
    };

    float cameraView[16] = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
                             0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f };

    float cameraProjection[16];

    float objectMatrix[4][16] = {
        { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f },

        { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 2.f, 0.f, 0.f, 1.f },

        { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 2.f, 0.f, 2.f, 1.f },

        { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 2.f, 1.f }
    };

    static const float identityMatrix[16] = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
                                              0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f };

    void Frustum( float left, float right, float bottom, float top, float znear, float zfar, float *m16 )
    {
        float temp, temp2, temp3, temp4;
        temp = 2.0f * znear;
        temp2 = right - left;
        temp3 = top - bottom;
        temp4 = zfar - znear;
        m16[0] = temp / temp2;
        m16[1] = 0.0;
        m16[2] = 0.0;
        m16[3] = 0.0;
        m16[4] = 0.0;
        m16[5] = temp / temp3;
        m16[6] = 0.0;
        m16[7] = 0.0;
        m16[8] = ( right + left ) / temp2;
        m16[9] = ( top + bottom ) / temp3;
        m16[10] = ( -zfar - znear ) / temp4;
        m16[11] = -1.0f;
        m16[12] = 0.0;
        m16[13] = 0.0;
        m16[14] = ( -temp * zfar ) / temp4;
        m16[15] = 0.0;
    }

    void Perspective( float fovyInDegrees, float aspectRatio, float znear, float zfar, float *m16 )
    {
        float ymax, xmax;
        ymax = znear * tanf( fovyInDegrees * 3.141592f / 180.0f );
        xmax = ymax * aspectRatio;
        Frustum( -xmax, xmax, -ymax, ymax, znear, zfar, m16 );
    }

    void Cross( const float *a, const float *b, float *r )
    {
        r[0] = a[1] * b[2] - a[2] * b[1];
        r[1] = a[2] * b[0] - a[0] * b[2];
        r[2] = a[0] * b[1] - a[1] * b[0];
    }

    float Dot( const float *a, const float *b )
    {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    }

    void Normalize( const float *a, float *r )
    {
        float il = 1.f / ( sqrtf( Dot( a, a ) ) + FLT_EPSILON );
        r[0] = a[0] * il;
        r[1] = a[1] * il;
        r[2] = a[2] * il;
    }

    void LookAt( const float *eye, const float *at, const float *up, float *m16 )
    {
        float X[3], Y[3], Z[3], tmp[3];

        tmp[0] = eye[0] - at[0];
        tmp[1] = eye[1] - at[1];
        tmp[2] = eye[2] - at[2];
        Normalize( tmp, Z );
        Normalize( up, Y );

        Cross( Y, Z, tmp );
        Normalize( tmp, X );

        Cross( Z, X, tmp );
        Normalize( tmp, Y );

        m16[0] = X[0];
        m16[1] = Y[0];
        m16[2] = Z[0];
        m16[3] = 0.0f;
        m16[4] = X[1];
        m16[5] = Y[1];
        m16[6] = Z[1];
        m16[7] = 0.0f;
        m16[8] = X[2];
        m16[9] = Y[2];
        m16[10] = Z[2];
        m16[11] = 0.0f;
        m16[12] = -Dot( X, eye );
        m16[13] = -Dot( Y, eye );
        m16[14] = -Dot( Z, eye );
        m16[15] = 1.0f;
    }

    void OrthoGraphic( const float l, float r, float b, const float t, float zn, const float zf,
                       float *m16 )
    {
        m16[0] = 2 / ( r - l );
        m16[1] = 0.0f;
        m16[2] = 0.0f;
        m16[3] = 0.0f;
        m16[4] = 0.0f;
        m16[5] = 2 / ( t - b );
        m16[6] = 0.0f;
        m16[7] = 0.0f;
        m16[8] = 0.0f;
        m16[9] = 0.0f;
        m16[10] = 1.0f / ( zf - zn );
        m16[11] = 0.0f;
        m16[12] = ( l + r ) / ( l - r );
        m16[13] = ( t + b ) / ( b - t );
        m16[14] = zn / ( zn - zf );
        m16[15] = 1.0f;
    }

    inline void rotationY( const float angle, float *m16 )
    {
        float c = cosf( angle );
        float s = sinf( angle );

        m16[0] = c;
        m16[1] = 0.0f;
        m16[2] = -s;
        m16[3] = 0.0f;
        m16[4] = 0.0f;
        m16[5] = 1.f;
        m16[6] = 0.0f;
        m16[7] = 0.0f;
        m16[8] = s;
        m16[9] = 0.0f;
        m16[10] = c;
        m16[11] = 0.0f;
        m16[12] = 0.f;
        m16[13] = 0.f;
        m16[14] = 0.f;
        m16[15] = 1.0f;
    }

    ImGuiOverlayOgre *ImGuiApplication::m_overlay = nullptr;

    void itemRowsBackground( float lineHeight = -1.0f, const ImColor &color = ImColor( 20, 20, 20, 64 ) )
    {
        auto *drawList = ImGui::GetWindowDrawList();
        const auto &style = ImGui::GetStyle();

        if( lineHeight < 0 )
        {
            lineHeight = ImGui::GetTextLineHeight();
        }
        lineHeight += style.ItemSpacing.y;

        float scrollOffsetH = ImGui::GetScrollX();
        float scrollOffsetV = ImGui::GetScrollY();
        float scrolledOutLines = floorf( scrollOffsetV / lineHeight );
        scrollOffsetV -= lineHeight * scrolledOutLines;

        ImVec2 clipRectMin( ImGui::GetWindowPos().x, ImGui::GetWindowPos().y );
        ImVec2 clipRectMax( clipRectMin.x + ImGui::GetWindowWidth(),
                            clipRectMin.y + ImGui::GetWindowHeight() );

        if( ImGui::GetScrollMaxX() > 0 )
        {
            clipRectMax.y -= style.ScrollbarSize;
        }

        drawList->PushClipRect( clipRectMin, clipRectMax );

        bool isOdd = ( static_cast<int>( scrolledOutLines ) % 2 ) == 0;

        float yMin = clipRectMin.y - scrollOffsetV + ImGui::GetCursorPosY();
        float yMax = clipRectMax.y - scrollOffsetV + lineHeight;
        float xMin = clipRectMin.x + scrollOffsetH + ImGui::GetWindowContentRegionMin().x;
        float xMax = clipRectMin.x + scrollOffsetH + ImGui::GetWindowContentRegionMax().x;

        for( float y = yMin; y < yMax; y += lineHeight, isOdd = !isOdd )
        {
            if( isOdd )
            {
                drawList->AddRectFilled( { xMin, y - style.ItemSpacing.y }, { xMax, y + lineHeight },
                                         color );
            }
        }

        drawList->PopClipRect();
    }

    bool showVector3( const char *label, Vector3F &vec )
    {
#if 1
        float label_width =
            ImGui::GetWindowWidth() *
            0.4f;  //ImGui::CalcTextSize( label ).x + ImGui::GetStyle().FramePadding.x * 2;

        bool ret = false;
        ImGui::Text( "%s:", label );
        ImGui::SameLine();
        ImGui::PushID( label );
        ImGui::SetCursorPosX( label_width );
        ImGui::PushItemWidth( ImGui::GetWindowWidth() * 0.6f );
        ret = ImGui::DragFloat3( "", &vec[0], 0.1f );
        ImGui::PopItemWidth();
        ImGui::PopID();

        return ret;
#else
        bool ret = false;

        //float label_width = ImGui::CalcTextSize( label ).x + ImGui::GetStyle().FramePadding.x * 2;
        float label_width =
            ImGui::GetWindowWidth() *
            0.4f;  //ImGui::CalcTextSize( label ).x + ImGui::GetStyle().FramePadding.x * 2;

        ImGui::Text( "%s:", label );
        ImGui::SameLine();
        ImGui::PushID( label );
        ImGui::SetCursorPosX( label_width );
        ImGui::PushItemWidth( ImGui::GetWindowWidth() * 0.15f );

        //ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
        //ImGui::Text( "X:", label );
        //ImGui::PopStyleColor();

        //ImGui::PushStyleColor( ImGuiCol_TextSelectedBg, ImVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
        ImGui::SameLine();
        if( ImGui::InputFloat( "x", &vec.x, 0.1f, 1.0f, "%.2f" ) )
        {
            ret = true;
        }

        //ImGui::PopStyleColor();

        //ImGui::SameLine();
        //ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.0f, 1.0f, 0.0f, 1.0f ) );
        //ImGui::Text( "Y:", label );
        //ImGui::PopStyleColor();

        //ImGui::PushStyleColor( ImGuiCol_TextSelectedBg, ImVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
        ImGui::SameLine();
        if( ImGui::InputFloat( "y", &vec.y, 0.1f, 1.0f, "%.2f" ) )
        {
            ret = true;
        }

        //ImGui::PopStyleColor();

        //ImGui::SameLine();
        //ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.0f, 0.0f, 1.0f, 1.0f ) );
        //ImGui::Text( "Z:", label );
        //ImGui::PopStyleColor();

        //ImGui::PushStyleColor( ImGuiCol_TextSelectedBg, ImVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
        ImGui::SameLine();
        if( ImGui::InputFloat( "z", &vec.z, 0.1f, 1.0f, "%.2f" ) )
        {
            ret = true;
        }

        //ImGui::PopStyleColor();

        ImGui::PopItemWidth();
        ImGui::PopID();

        return ret;
#endif
    }

    ImGuiApplication::ImGuiApplication() = default;

    ImGuiApplication::~ImGuiApplication()
    {
        unload( nullptr );
    }

    void ImGuiApplication::load( SmartPtr<ISharedObject> data )
    {
        WP_ASSERT( isLoaded() == false );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto application = applicationManager->getApplication();
        auto applicationName = application->getName();

        auto ui = workphone::static_pointer_cast<ImGuiManager>( applicationManager->getUI() );

        if( ImGui::GetCurrentContext() )
        {
            setCustomStyle();
        }

        auto sceneManager = graphicsSystem->getGraphicsScene();
        WP_ASSERT( sceneManager );

        auto window = graphicsSystem->getDefaultWindow();
        WP_ASSERT( window );

        auto listener = workphone::make_ptr<WindowListener>();
        listener->setOwner( this );
        window->addListener( listener );
        m_windowListener = listener;

        size_t windowHandle = 0;
        window->getCustomAttribute( "WINDOW", &windowHandle );

        m_hwnd = (void *)windowHandle;

        setLoadingState( LoadingState::Loaded );
    }

    void ImGuiApplication::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto ui = workphone::static_pointer_cast<ImGuiManager>( applicationManager->getUI() );

                ui->unloadFont();

                if( m_windowListener )
                {
                    m_windowListener->unload( nullptr );
                    m_windowListener = nullptr;
                }

                m_toolbar = nullptr;
                m_menuBar = nullptr;

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ImGuiApplication::setCustomStyle()
    {
        WPImGuiStyle::apply();
    }

    void ImGuiApplication::setDarkGreenStyle()
    {
        ImGuiStyle &style = ImGui::GetStyle();

        // Modify colors for a dark green scheme
        ImVec4 *colors = style.Colors;
        colors[ImGuiCol_Text] = ImVec4( 0.80f, 0.80f, 0.80f, 1.00f );
        colors[ImGuiCol_TextDisabled] = ImVec4( 0.50f, 0.50f, 0.50f, 1.00f );
        colors[ImGuiCol_WindowBg] = ImVec4( 0.05f, 0.15f, 0.05f, 0.94f );
        colors[ImGuiCol_ChildBg] = ImVec4( 0.10f, 0.20f, 0.10f, 0.00f );
        colors[ImGuiCol_PopupBg] = ImVec4( 0.05f, 0.15f, 0.05f, 0.94f );
        colors[ImGuiCol_Border] = ImVec4( 0.50f, 0.70f, 0.50f, 0.50f );
        colors[ImGuiCol_BorderShadow] = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
        colors[ImGuiCol_FrameBg] = ImVec4( 0.10f, 0.20f, 0.10f, 1.00f );
        colors[ImGuiCol_FrameBgHovered] = ImVec4( 0.20f, 0.30f, 0.20f, 0.80f );
        colors[ImGuiCol_FrameBgActive] = ImVec4( 0.15f, 0.25f, 0.15f, 1.00f );
        colors[ImGuiCol_TitleBg] = ImVec4( 0.05f, 0.15f, 0.05f, 1.00f );
        colors[ImGuiCol_TitleBgActive] = ImVec4( 0.10f, 0.25f, 0.10f, 1.00f );
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4( 0.00f, 0.10f, 0.00f, 0.51f );
        colors[ImGuiCol_MenuBarBg] = ImVec4( 0.10f, 0.20f, 0.10f, 1.00f );
        colors[ImGuiCol_ScrollbarBg] = ImVec4( 0.02f, 0.10f, 0.02f, 0.53f );
        colors[ImGuiCol_ScrollbarGrab] = ImVec4( 0.20f, 0.50f, 0.20f, 1.00f );
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4( 0.30f, 0.60f, 0.30f, 1.00f );
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4( 0.40f, 0.70f, 0.40f, 1.00f );
        colors[ImGuiCol_CheckMark] = ImVec4( 0.50f, 1.00f, 0.50f, 1.00f );
        colors[ImGuiCol_SliderGrab] = ImVec4( 0.20f, 0.50f, 0.20f, 1.00f );
        colors[ImGuiCol_SliderGrabActive] = ImVec4( 0.30f, 0.60f, 0.30f, 1.00f );
        colors[ImGuiCol_Button] = ImVec4( 0.20f, 0.50f, 0.20f, 1.00f );
        colors[ImGuiCol_ButtonHovered] = ImVec4( 0.30f, 0.60f, 0.30f, 1.00f );
        colors[ImGuiCol_ButtonActive] = ImVec4( 0.40f, 0.70f, 0.40f, 1.00f );
        colors[ImGuiCol_Header] = ImVec4( 0.20f, 0.50f, 0.20f, 1.00f );
        colors[ImGuiCol_HeaderHovered] = ImVec4( 0.30f, 0.60f, 0.30f, 1.00f );
        colors[ImGuiCol_HeaderActive] = ImVec4( 0.40f, 0.70f, 0.40f, 1.00f );
        colors[ImGuiCol_Separator] = ImVec4( 0.50f, 0.70f, 0.50f, 0.60f );
        colors[ImGuiCol_SeparatorHovered] = ImVec4( 0.60f, 0.80f, 0.60f, 0.70f );
        colors[ImGuiCol_SeparatorActive] = ImVec4( 0.70f, 0.90f, 0.70f, 0.80f );
        colors[ImGuiCol_ResizeGrip] = ImVec4( 0.40f, 0.80f, 0.40f, 0.25f );
        colors[ImGuiCol_ResizeGripHovered] = ImVec4( 0.50f, 0.90f, 0.50f, 0.67f );
        colors[ImGuiCol_ResizeGripActive] = ImVec4( 0.60f, 1.00f, 0.60f, 0.95f );
        colors[ImGuiCol_Tab] = ImVec4( 0.20f, 0.50f, 0.20f, 1.00f );
        colors[ImGuiCol_TabHovered] = ImVec4( 0.30f, 0.60f, 0.30f, 1.00f );
        colors[ImGuiCol_TabActive] = ImVec4( 0.40f, 0.70f, 0.40f, 1.00f );
        colors[ImGuiCol_TabUnfocused] = ImVec4( 0.20f, 0.50f, 0.20f, 1.00f );
        //colors[ImGuiCol_TabUnfocusedHovered] = ImVec4(0.30f, 0.60f, 0.30f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4( 0.40f, 0.70f, 0.40f, 1.00f );
        colors[ImGuiCol_PlotLines] = ImVec4( 0.80f, 0.80f, 0.80f, 1.00f );
        colors[ImGuiCol_PlotLinesHovered] = ImVec4( 0.70f, 0.90f, 0.70f, 1.00f );
        colors[ImGuiCol_PlotHistogram] = ImVec4( 0.70f, 0.90f, 0.70f, 1.00f );
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4( 0.80f, 1.00f, 0.80f, 1.00f );
        colors[ImGuiCol_TextSelectedBg] = ImVec4( 0.40f, 0.80f, 0.40f, 0.35f );
        colors[ImGuiCol_DragDropTarget] = ImVec4( 0.80f, 1.00f, 0.80f, 0.90f );
        colors[ImGuiCol_NavHighlight] = ImVec4( 0.40f, 0.80f, 0.40f, 1.00f );
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4( 0.80f, 1.00f, 0.80f, 0.70f );
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4( 0.60f, 0.80f, 0.60f, 0.20f );
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4( 0.60f, 0.80f, 0.60f, 0.35f );
    }

    void ImGuiApplication::setDarkBlueStyle()
    {
        ImGuiStyle &style = ImGui::GetStyle();

        // Modify colors for a dark blue style
        ImVec4 *colors = style.Colors;
        colors[ImGuiCol_Text] = ImVec4( 0.80f, 0.80f, 0.80f, 1.00f );
        colors[ImGuiCol_TextDisabled] = ImVec4( 0.50f, 0.50f, 0.50f, 1.00f );
        colors[ImGuiCol_WindowBg] = ImVec4( 0.05f, 0.05f, 0.15f, 0.94f );
        colors[ImGuiCol_ChildBg] = ImVec4( 0.10f, 0.10f, 0.20f, 0.00f );
        colors[ImGuiCol_PopupBg] = ImVec4( 0.05f, 0.05f, 0.15f, 0.94f );
        colors[ImGuiCol_Border] = ImVec4( 0.50f, 0.70f, 1.00f, 0.50f );
        colors[ImGuiCol_BorderShadow] = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
        colors[ImGuiCol_FrameBg] = ImVec4( 0.10f, 0.10f, 0.20f, 1.00f );
        colors[ImGuiCol_FrameBgHovered] = ImVec4( 0.20f, 0.20f, 0.30f, 0.80f );
        colors[ImGuiCol_FrameBgActive] = ImVec4( 0.15f, 0.15f, 0.25f, 1.00f );
        colors[ImGuiCol_TitleBg] = ImVec4( 0.05f, 0.05f, 0.15f, 1.00f );
        colors[ImGuiCol_TitleBgActive] = ImVec4( 0.10f, 0.10f, 0.20f, 1.00f );
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4( 0.00f, 0.00f, 0.10f, 0.51f );
        colors[ImGuiCol_MenuBarBg] = ImVec4( 0.10f, 0.10f, 0.20f, 1.00f );
        colors[ImGuiCol_ScrollbarBg] = ImVec4( 0.02f, 0.02f, 0.10f, 0.53f );
        colors[ImGuiCol_ScrollbarGrab] = ImVec4( 0.20f, 0.20f, 0.70f, 1.00f );
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4( 0.30f, 0.30f, 0.90f, 1.00f );
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4( 0.40f, 0.40f, 1.00f, 1.00f );
        colors[ImGuiCol_CheckMark] = ImVec4( 0.50f, 1.00f, 1.00f, 1.00f );
        colors[ImGuiCol_SliderGrab] = ImVec4( 0.20f, 0.20f, 0.70f, 1.00f );
        colors[ImGuiCol_SliderGrabActive] = ImVec4( 0.30f, 0.30f, 0.90f, 1.00f );
        colors[ImGuiCol_Button] = ImVec4( 0.20f, 0.20f, 0.70f, 1.00f );
        colors[ImGuiCol_ButtonHovered] = ImVec4( 0.30f, 0.30f, 0.90f, 1.00f );
        colors[ImGuiCol_ButtonActive] = ImVec4( 0.40f, 0.40f, 1.00f, 1.00f );
        colors[ImGuiCol_Header] = ImVec4( 0.20f, 0.20f, 0.70f, 1.00f );
        colors[ImGuiCol_HeaderHovered] = ImVec4( 0.30f, 0.30f, 0.90f, 1.00f );
        colors[ImGuiCol_HeaderActive] = ImVec4( 0.40f, 0.40f, 1.00f, 1.00f );
        colors[ImGuiCol_Separator] = ImVec4( 0.50f, 0.70f, 1.00f, 0.60f );
        colors[ImGuiCol_SeparatorHovered] = ImVec4( 0.60f, 0.80f, 1.00f, 0.70f );
        colors[ImGuiCol_SeparatorActive] = ImVec4( 0.70f, 0.90f, 1.00f, 0.80f );
        colors[ImGuiCol_ResizeGrip] = ImVec4( 0.40f, 0.80f, 1.00f, 0.25f );
        colors[ImGuiCol_ResizeGripHovered] = ImVec4( 0.50f, 0.90f, 1.00f, 0.67f );
        colors[ImGuiCol_ResizeGripActive] = ImVec4( 0.60f, 1.00f, 1.00f, 0.95f );
        colors[ImGuiCol_Tab] = ImVec4( 0.20f, 0.20f, 0.70f, 1.00f );
        colors[ImGuiCol_TabHovered] = ImVec4( 0.30f, 0.30f, 0.90f, 1.00f );
        colors[ImGuiCol_TabActive] = ImVec4( 0.40f, 0.40f, 1.00f, 1.00f );
        colors[ImGuiCol_TabUnfocused] = ImVec4( 0.20f, 0.20f, 0.70f, 1.00f );
        //colors[ImGuiCol_TabUnfocusedHovered] = ImVec4(0.30f, 0.30f, 0.90f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4( 0.40f, 0.40f, 1.00f, 1.00f );
        colors[ImGuiCol_PlotLines] = ImVec4( 0.80f, 0.80f, 0.80f, 1.00f );
        colors[ImGuiCol_PlotLinesHovered] = ImVec4( 0.70f, 0.90f, 0.90f, 1.00f );
        colors[ImGuiCol_PlotHistogram] = ImVec4( 0.70f, 0.90f, 0.90f, 1.00f );
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4( 0.80f, 1.00f, 1.00f, 1.00f );
        colors[ImGuiCol_TextSelectedBg] = ImVec4( 0.40f, 0.40f, 0.90f, 0.35f );
        colors[ImGuiCol_DragDropTarget] = ImVec4( 0.80f, 1.00f, 1.00f, 0.90f );
        colors[ImGuiCol_NavHighlight] = ImVec4( 0.40f, 0.40f, 0.90f, 1.00f );
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4( 0.80f, 1.00f, 1.00f, 0.70f );
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4( 0.60f, 0.80f, 1.00f, 0.20f );
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4( 0.60f, 0.80f, 1.00f, 0.35f );
    }

    size_t ImGuiApplication::messagePump( SmartPtr<ISharedObject> data )
    {
#if defined WP_PLATFORM_WIN32
        if( data )
        {
            if( m_hwnd )
            {
                auto windowMessageData = workphone::static_pointer_cast<WindowMessageData>( data );

                auto handle = windowMessageData->getWindowHandle();
                auto message = windowMessageData->getMessage();
                auto wParam = windowMessageData->getWParam();
                auto lParam = windowMessageData->getLParam();

                return ImGui_ImplWin32_WndProcHandler( static_cast<HWND>( handle ), message, wParam,
                                                       lParam );
            }
        }
#elif defined( WP_PLATFORM_APPLE )
#endif

        return 0;
    }

    void ImGuiApplication::handleWindowEvent( SmartPtr<render::IGraphicsWindowEvent> event )
    {
        const auto &loadingState = getLoadingState();
        if( loadingState == LoadingState::Loaded )
        {
#if defined WP_PLATFORM_WIN32
            if( m_hwnd )
            {
                auto windowMessageData = workphone::static_pointer_cast<WindowMessageData>( event );

                auto handle = windowMessageData->getWindowHandle();
                auto message = windowMessageData->getMessage();
                auto wParam = windowMessageData->getWParam();
                auto lParam = windowMessageData->getLParam();

                ImGui_ImplWin32_WndProcHandler( static_cast<HWND>( handle ), message, wParam, lParam );
            }
#elif defined WP_PLATFORM_APPLE
            if( m_app )
            {
                m_app->handleWindowEvent( event );
            }
#endif
        }
    }

    bool ImGuiApplication::handleInputEvent( SmartPtr<IInputEvent> event )
    {
        /*
        ImGuiIO &io = ImGui::GetIO();

        auto eventType = event->getEventType();
        if( eventType == IInputEvent::EventType::Mouse )
        {
            auto mouseState = event->getMouseState();
            auto mouseEventType = mouseState->getEventType();
            if( mouseEventType == IMouseState::Event::LeftPressed )
            {
                auto position = mouseState->getAbsolutePosition();
                io.AddMousePosEvent( position.X(), position.Y() );

                int button = static_cast<int>( mouseEventType );
                if( button >= 0 && button < ImGuiMouseButton_COUNT )
                {
                    io.AddMouseButtonEvent( ImGuiMouseButton_Left, true );
                }

                // WP_LOG("Test mouse pressed");
            }
            else if( mouseEventType == IMouseState::Event::LeftReleased )
            {
                int button = static_cast<int>( mouseEventType );
                if( button >= 0 && button < ImGuiMouseButton_COUNT )
                {
                    io.AddMouseButtonEvent( ImGuiMouseButton_Left, false );
                }
            }

            if( mouseEventType == IMouseState::Event::RightPressed )
            {
                auto position = mouseState->getAbsolutePosition();
                io.AddMousePosEvent( position.X(), position.Y() );

                int button = static_cast<int>( mouseEventType );
                if( button >= 0 && button < ImGuiMouseButton_COUNT )
                {
                    io.AddMouseButtonEvent( ImGuiMouseButton_Right, true );
                }

                // WP_LOG("Test mouse pressed");
            }
            else if( mouseEventType == IMouseState::Event::RightReleased )
            {
                int button = static_cast<int>( mouseEventType );
                if( button >= 0 && button < ImGuiMouseButton_COUNT )
                {
                    io.AddMouseButtonEvent( ImGuiMouseButton_Right, false );
                }
            }

            if( mouseEventType == IMouseState::Event::Moved )
            {
                auto p = mouseState->getAbsolutePosition();
                auto position = Vector2I( p.X(), p.Y() );
                if( !MathUtil<s32>::equals( position, m_currentMousePosition ) )
                {
                    io.AddMousePosEvent( static_cast<float>( position.X() ),
                                         static_cast<float>( position.Y() ) );
                    m_currentMousePosition = position;

                    // WP_LOG("Test mouse move");
                }
            }

            return io.WantCaptureMouse;
        }

        if( eventType == IInputEvent::EventType::Key )
        {
            Map<KeyCodes, ImGuiKey_> keyboardMap;

            for( size_t i = 0; i < int( KeyCodes::KEY_COUNT ); ++i )
            {
                auto keyCode = (KeyCodes)i;
                auto imGuiKey = (ImGuiKey_)( (size_t)ImGuiKey_NamedKey_BEGIN + i );

                switch( keyCode )
                {
                case KeyCodes::KEY_KEY_0:
                {
                    imGuiKey = ImGuiKey_::ImGuiKey_0;
                }
                break;
                case KeyCodes::KEY_KEY_1:
                {
                    imGuiKey = ImGuiKey_::ImGuiKey_1;
                }
                break;
                case KeyCodes::KEY_KEY_2:
                {
                    imGuiKey = ImGuiKey_::ImGuiKey_2;
                }
                break;
                case KeyCodes::KEY_KEY_3:
                {
                    imGuiKey = ImGuiKey_::ImGuiKey_3;
                }
                break;
                case KeyCodes::KEY_KEY_4:
                {
                    imGuiKey = ImGuiKey_::ImGuiKey_4;
                }
                break;
                case KeyCodes::KEY_KEY_5:
                {
                    imGuiKey = ImGuiKey_::ImGuiKey_5;
                }
                break;
                case KeyCodes::KEY_KEY_6:
                {
                    imGuiKey = ImGuiKey_::ImGuiKey_6;
                }
                break;
                case KeyCodes::KEY_KEY_7:
                {
                    imGuiKey = ImGuiKey_::ImGuiKey_7;
                }
                break;
                case KeyCodes::KEY_KEY_8:
                {
                    imGuiKey = ImGuiKey_::ImGuiKey_8;
                }
                break;
                case KeyCodes::KEY_KEY_9:
                {
                    imGuiKey = ImGuiKey_::ImGuiKey_9;
                }
                break;
                case KeyCodes::KEY_KEY_A:
                case KeyCodes::KEY_KEY_B:
                case KeyCodes::KEY_KEY_C:
                case KeyCodes::KEY_KEY_D:
                case KeyCodes::KEY_KEY_E:
                case KeyCodes::KEY_KEY_F:
                {
                    imGuiKey = ImGuiKey_::ImGuiKey_F;
                }
                break;
                case KeyCodes::KEY_KEY_G:
                case KeyCodes::KEY_KEY_H:
                case KeyCodes::KEY_KEY_I:
                case KeyCodes::KEY_KEY_J:
                case KeyCodes::KEY_KEY_K:
                case KeyCodes::KEY_KEY_L:
                case KeyCodes::KEY_KEY_M:
                case KeyCodes::KEY_KEY_N:
                case KeyCodes::KEY_KEY_O:
                case KeyCodes::KEY_KEY_P:
                case KeyCodes::KEY_KEY_Q:
                case KeyCodes::KEY_KEY_R:
                case KeyCodes::KEY_KEY_S:
                case KeyCodes::KEY_KEY_T:
                case KeyCodes::KEY_KEY_U:
                case KeyCodes::KEY_KEY_V:
                case KeyCodes::KEY_KEY_W:
                case KeyCodes::KEY_KEY_X:
                case KeyCodes::KEY_KEY_Y:
                case KeyCodes::KEY_KEY_Z:
                {
                }
                break;
                default:
                {
                }
                };

                keyboardMap[keyCode] = imGuiKey;
            }

            auto keyboardState = event->getKeyboardState();
            auto keycode = keyboardState->getKeyCode();
            auto rawKeyCode = keyboardState->getRawKeyCode();
            auto keyChar = keyboardState->getChar();
            bool is_key_down = keyboardState->isPressedDown();

            auto key = keyboardMap[(KeyCodes)keycode];

            if( (ImGuiKey)key != ImGuiKey_None )
            {
                io.AddKeyEvent( (ImGuiKey)key, is_key_down );
            }
        }
        */

        return false;
    }

    void ImGuiApplication::run()
    {
        while( !m_done )
        {
            update();
        }
    }

    void ImGuiApplication::createSubMenus( SmartPtr<IUIMenu> menu )
    {
        auto subMenuElements = menu->getMenuItems();
        for( auto subMenuElement : subMenuElements )
        {
            if( subMenuElement->isDerived<IUIMenu>() )
            {
                auto subMenu = workphone::static_pointer_cast<IUIMenu>( subMenuElement );

                auto menuBar = getMenubar();

                auto label = subMenu->getLabel();
                if( ImGui::BeginMenu( label.c_str() ) )
                {
                    auto menuItemElements = subMenu->getMenuItems();
                    for( auto menuItemElement : menuItemElements )
                    {
                        if( menuItemElement->isDerived<IUIMenuItem>() )
                        {
                            auto menuItem =
                                workphone::static_pointer_cast<IUIMenuItem>( menuItemElement );

                            auto text = menuItem->getText();

                            bool selected = false;
                            ImGui::MenuItem( text.c_str(), nullptr, &selected );

                            if( selected )
                            {
                                auto menuItemId = menuItem->getElementId();

                                auto listeners = menuBar->getObjectListeners();
                                for( auto listener : listeners )
                                {
                                    listener->handleEvent( EventType::UI, menuItemId, Array<Parameter>(),
                                                           menuItemElement, this, nullptr );
                                }
                            }
                        }
                    }

                    ImGui::EndMenu();
                }

                createSubMenus( subMenu );
            }
        }
    }

    void ImGuiApplication::showPlaceholderObject( const char *prefix, int uid )
    {
        // Use object uid as identifier. Most commonly you could also use the object pointer as a
        // base ID.
        ImGui::PushID( uid );

        // Text and Tree nodes are less high than framed widgets, using AlignTextToFramePadding() we
        // add vertical spacing to make the tree lines equal high.
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex( 0 );
        ImGui::AlignTextToFramePadding();
        bool node_open = ImGui::TreeNode( "Object", "%s_%u", prefix, uid );
        ImGui::TableSetColumnIndex( 1 );
        ImGui::Text( "my sailor is rich" );

        if( node_open )
        {
            static float placeholder_members[8] = { 0.0f, 0.0f, 1.0f, 3.1416f, 100.0f, 999.0f };
            for( int i = 0; i < 8; i++ )
            {
                ImGui::PushID( i );  // Use field index as identifier.
                if( i < 2 )
                {
                    showPlaceholderObject( "Child", 424242 );
                }
                else
                {
                    // Here we use a TreeNode to highlight on hover (we could use e.g. Selectable as
                    // well)
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex( 0 );
                    ImGui::AlignTextToFramePadding();
                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf |
                                               ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                               ImGuiTreeNodeFlags_Bullet;
                    ImGui::TreeNodeEx( "Field", flags, "Field_%d", i );

                    ImGui::TableSetColumnIndex( 1 );
                    ImGui::SetNextItemWidth( -FLT_MIN );
                    if( i >= 5 )
                    {
                        ImGui::InputFloat( "##value", &placeholder_members[i], 1.0f );
                    }
                    else
                    {
                        ImGui::DragFloat( "##value", &placeholder_members[i], 0.01f );
                    }
                    ImGui::NextColumn();
                }
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    // int inputTextCallback(ImGuiInputTextCallbackData* data)
    //{

    //}

    void ImGuiApplication::createElement( SmartPtr<IUIElement> element )
    {
        if( element )
        {
            if( element->getSameLine() )
            {
                ImGui::SameLine( 0, 5 );
            }

            //if( element->isDerived<IUIEventWindow>() )
            //{
            //    element->update();
            //}

            if( element->isDerived<IUIWindow>() )
            {
                auto window = workphone::static_pointer_cast<IUIWindow>( element );
                if( window )
                {
                    if( window->isVisible() )
                    {
                        auto label = window->getLabel();
                        if( StringUtil::isNullOrEmpty( label ) )
                        {
                            if( auto parent = window->getParent() )
                            {
                                auto parentWindow = workphone::static_pointer_cast<IUIWindow>( parent );
                                label = parentWindow->getLabel() + String( "_Child_" ) +
                                        StringUtil::toString( m_childWindowCount++ );
                            }
                        }

                        auto border = window->hasBorder();

                        auto windowFlags = ImGuiWindowFlags_AlwaysAutoResize;

                        auto size = window->getSize();
                        auto size_arg = ImVec2( size.X(), size.Y() );
                        if( ImGui::BeginChild( label.c_str(), size_arg, border, windowFlags ) )
                        {
                            element->update();

                            auto children = element->getChildren();
                            for( auto &child : children )
                            {
                                createElement( child );
                            }

                            if( auto dropTarget = window->getDropTarget() )
                            {
                                auto imguiWindow = ImGui::GetCurrentWindow();
                                if( ImGui::BeginDragDropTargetCustom( imguiWindow->InnerRect,
                                                                     imguiWindow->ID ) )
                                {
                                    if( auto payload =
                                            ImGui::AcceptDragDropPayload( "_TREENODE" ) )
                                    {
                                        auto data = String(
                                            static_cast<const char *>( payload->Data ),
                                            payload->DataSize );

                                        Array<Parameter> args;
                                        args.emplace_back( data );

                                        dropTarget->handleEvent(
                                            EventType::UI, IEvent::handleDrop, args, window, window,
                                            nullptr );
                                    }

                                    ImGui::EndDragDropTarget();
                                }
                            }

                            const bool is_hovered = ImGui::IsItemHovered();  // Hovered
                            const bool is_active = ImGui::IsItemActive();    // Held

                            if( is_hovered && ImGui::IsMouseClicked( 0 ) )
                            {
                                auto listeners = window->getObjectListeners();
                                for( auto listener : listeners )
                                {
                                    auto args = Array<Parameter>();
                                    args.resize( 1 );

                                    args[0].object = window;

                                    listener->handleEvent( EventType::UI, IEvent::handleMouseClicked,
                                                           args, window, this, nullptr );
                                }
                            }

                            if( auto contextMenu = window->getContextMenu() )
                            {
                                if( ImGui::BeginPopupContextWindow( label.c_str(),
                                                                    ImGuiPopupFlags_MouseButtonRight ) )
                                {
                                    auto label = contextMenu->getLabel();
                                    if( StringUtil::isNullOrEmpty( label ) )
                                    {
                                        label = "Untitled";
                                    }

                                    //if( ImGui::BeginMenu( label.c_str() ) )
                                    {
                                        auto menuItems = contextMenu->getMenuItems();
                                        for( auto menuItemElement : menuItems )
                                        {
                                            createMenuItem( contextMenu, menuItemElement );
                                        }

                                        //ImGui::EndMenu();
                                    }

                                    ImGui::EndPopup();
                                }
                            }

                            // auto pos = ImVec2( 0, 0 );
                            // ImGui::SetNextWindowPos( pos );
                        }

                        ImGui::EndChild();
                    }
                }
            }
            else
            {
                //if( element->isDerived<IUIButton>() )
                //{
                //    auto button = fb::static_pointer_cast<IUIButton>( element );

                //    auto label = button->getLabel();
                //    if( StringUtil::isNullOrEmpty( label ) )
                //    {
                //        label = "Untitled";
                //    }

                //    if( ImGui::Button( label.c_str() ) )
                //    {
                //        if( auto parent = button->getParent() )
                //        {
                //            if( parent->isDerived<IUIToolbar>() )
                //            {
                //                auto toolbar = fb::static_pointer_cast<IUIToolbar>( parent );
                //                WP_ASSERT( toolbar );

                //                auto listeners = toolbar->getObjectListeners();
                //                for( auto listener : listeners )
                //                {
                //                    auto args = Array<Parameter>();
                //                    listener->handleEvent( EventType::UI, IEvent::handleSelection,
                //                                           args, toolbar, button, nullptr );
                //                }
                //            }
                //            else
                //            {
                //                auto listeners = button->getObjectListeners();
                //                for( auto listener : listeners )
                //                {
                //                    auto args = Array<Parameter>();
                //                    listener->handleEvent( EventType::UI, IEvent::handleSelection,
                //                                           args, button, button, nullptr );
                //                }
                //            }
                //        }
                //    }
                //}
                //else if( element->isDerived<IUIText>() )
                //{
                //    auto text = fb::static_pointer_cast<IUIText>( element );
                //    auto str = text->getText();
                //    if( StringUtil::isNullOrEmpty( str ) )
                //    {
                //        str = "";
                //    }

                //    ImGui::Text( str.c_str() );
                //}
                if( element->isDerived<IUITreeCtrl>() )
                {
                    ImGuiTreeCtrl::createElement( element );
                }
                else if( element->isDerived<IUILabelTogglePair>() )
                {
                    auto labelCheckboxPair =
                        workphone::static_pointer_cast<IUILabelTogglePair>( element );
                    auto label = labelCheckboxPair->getLabel();
                    auto value = labelCheckboxPair->getValue();

                    if( StringUtil::isNullOrEmpty( label ) )
                    {
                        label = "Untitled";
                    }

                    ImGui::Text( "%s", label.c_str() );

                    ImGui::SameLine( 0, 5 );

                    if( ImGuiUtil::ToggleButton( label.c_str(), &value ) )
                    //if( ImGui::Checkbox( label.c_str(), &value ) )
                    {
                        labelCheckboxPair->setValue( value );

                        if( auto parent = labelCheckboxPair->getParent() )
                        {
                            if( parent->isDerived<IUIToolbar>() )
                            {
                                auto toolbar = workphone::static_pointer_cast<IUIToolbar>( parent );
                                WP_ASSERT( toolbar );

                                auto listeners = toolbar->getObjectListeners();
                                for( auto listener : listeners )
                                {
                                    auto args = Array<Parameter>();
                                    listener->handleEvent( EventType::UI, IEvent::handleValueChanged,
                                                           args, toolbar, element, nullptr );
                                }
                            }
                            else
                            {
                                auto listeners = element->getObjectListeners();
                                for( auto listener : listeners )
                                {
                                    auto args = Array<Parameter>();
                                    args.resize( 2 );

                                    args[0].object = element;
                                    args[1].setBool( value );

                                    listener->handleEvent( EventType::UI, IEvent::handleValueChanged,
                                                           args, element, element, nullptr );
                                }
                            }
                        }
                    }
                }
                else if( element->isDerived<IUILabelTextInputPair>() )
                {
                    auto labelTextInputPair =
                        workphone::static_pointer_cast<IUILabelTextInputPair>( element );
                    auto label = labelTextInputPair->getLabel();
                    auto value = labelTextInputPair->getValue();

                    if( StringUtil::isNullOrEmpty( label ) )
                    {
                        label = "Untitled";
                    }

                    constexpr s32 strSize = 256;
                    FixedString<strSize> str;
                    str = value.c_str();

                    if( ImGui::InputText( label.c_str(), const_cast<char *>( str.c_str() ), strSize ) )
                    {
                        labelTextInputPair->setValue( String( str.c_str() ) );

                        auto listeners = element->getObjectListeners();
                        for( auto listener : listeners )
                        {
                            auto args = Array<Parameter>();
                            args.resize( 1 );

                            args[0].object = element;

                            listener->handleEvent( EventType::UI, IEvent::handleValueChanged, args,
                                                   element, this, nullptr );
                        }
                    }
                }
                else if( element->isDerived<IUIDropdown>() )
                {
                    auto dropdown = workphone::static_pointer_cast<IUIDropdown>( element );
                    auto options = dropdown->getOptions();

                    Array<char *> optionsStr;
                    optionsStr.resize( options.size() );

                    for( size_t i = 0; i < options.size(); ++i )
                    {
                        optionsStr[i] = const_cast<char *>( options[i].c_str() );
                    }

                    auto item_current = static_cast<s32>( dropdown->getSelectedOption() );
                    auto strSize = static_cast<int>( optionsStr.size() );

                    auto name = dropdown->getName();
                    auto label = dropdown->getLabel();
                    auto guiLabel = label.empty() ? name : label;

                    if( ImGui::Combo( guiLabel.c_str(), &item_current, &optionsStr[0], strSize ) )
                    {
                        dropdown->setSelectedOption( static_cast<u32>( item_current ) );

                        auto listeners = dropdown->getObjectListeners();
                        for( auto listener : listeners )
                        {
                            auto args = Array<Parameter>();
                            args.resize( 1 );

                            args[0] = Parameter( item_current );

                            listener->handleEvent( EventType::UI, IEvent::handleSelection, args, element,
                                                   this, nullptr );
                        }
                    }
                }
                else if( element->isDerived<IUIPropertyGrid>() )
                {
                    ImGuiPropertyGrid::createElement( element );
                }
                else if( element->isDerived<IUIVector2>() )
                {
                    auto vectorUI = workphone::static_pointer_cast<IUIVector2>( element );

                    auto name = vectorUI->getLabel();
                    auto vector = vectorUI->getValue();

                    f32 value[2] = { 0 };

                    value[0] = vector.X();
                    value[1] = vector.Y();

                    if( ImGui::InputFloat2( name.c_str(), value ) )
                    {
                        vectorUI->setValue( Vector2( value[0], value[1] ) );

                        auto listeners = vectorUI->getObjectListeners();
                        for( auto listener : listeners )
                        {
                            auto args = Array<Parameter>();
                            args.resize( 1 );

                            listener->handleEvent( EventType::UI, IEvent::handleValueChanged, args,
                                                   element, this, nullptr );
                        }
                    }
                }
                else if( element->isDerived<IUIVector3>() )
                {
                    auto vectorUI = workphone::static_pointer_cast<IUIVector3>( element );

                    auto name = vectorUI->getLabel();
                    auto vector = vectorUI->getValue();

                    //if( ImGui::InputFloat3( name.c_str(), value ) )
                    if( showVector3( name.c_str(), vector ) )
                    {
                        vectorUI->setValue( vector );

                        auto listeners = vectorUI->getObjectListeners();
                        for( auto listener : listeners )
                        {
                            auto args = Array<Parameter>();
                            args.resize( 1 );

                            listener->handleEvent( EventType::UI, IEvent::handleValueChanged, args,
                                                   element, this, nullptr );
                        }
                    }
                }
                else if( element->isDerived<IUIVector4>() )
                {
                    auto vectorUI = workphone::static_pointer_cast<IUIVector4>( element );

                    auto name = vectorUI->getLabel();
                    auto vector = vectorUI->getValue();

                    f32 value[4] = { 0 };

                    value[0] = vector.X();
                    value[1] = vector.Y();
                    value[2] = vector.Z();
                    value[3] = vector.W();

                    if( ImGui::InputFloat4( name.c_str(), value ) )
                    {
                        vectorUI->setValue( Vector4( value[0], value[1], value[2], value[3] ) );

                        auto listeners = vectorUI->getObjectListeners();
                        for( auto listener : listeners )
                        {
                            auto args = Array<Parameter>();
                            args.resize( 1 );

                            listener->handleEvent( EventType::UI, IEvent::handleValueChanged, args,
                                                   element, this, nullptr );
                        }
                    }
                }
                else if( element->isDerived<IUIToolbar>() )
                {
                    auto toolbar = workphone::static_pointer_cast<IUIToolbar>( element );

                    static ImGuiAxis toolbar1_axis = ImGuiAxis_X;
                    dockingToolbar( "Toolbar1", &toolbar1_axis, toolbar );
                    return;
                }
                else if( element->isDerived<IUITerrainEditor>() )
                {
                    element->update();
                }
                else if( element->isDerived<IUIImage>() )
                {
                    element->update();
                }
                else
                {
                    element->update();
                }

                if( element->getRenderChildren() )
                {
                    auto children = element->getChildren();
                    for( auto &child : children )
                    {
                        createElement( child );
                    }
                }
            }
        }
    }

    void ImGuiApplication::testDoc()
    {
        const bool enableDocking = ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable;
        if( enableDocking )
        {
            ImGuiViewport *viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos( viewport->WorkPos );
            ImGui::SetNextWindowSize( viewport->WorkSize );
            ImGui::SetNextWindowViewport( viewport->ID );

            ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
            ImGuiWindowFlags host_window_flags = 0;
            host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoDocking;
            host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
            // if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            //	host_window_flags |= ImGuiWindowFlags_NoBackground;

            ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
            ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
            ImGui::Begin( "DockSpace Window", nullptr, host_window_flags );
            ImGui::PopStyleVar( 3 );

            ImGuiID dockspace_id = ImGui::GetID( "DockSpace" );
            ImGui::DockSpace( dockspace_id, ImVec2( 0.0f, 0.0f ), dockspace_flags, nullptr );
            ImGui::End();
        }
    }

    void ImGuiApplication::setupDefaultDockLayout( ImGuiID dockspaceId, const ImVec2 &dockspaceSize )
    {
        ImGui::DockBuilderRemoveNode( dockspaceId );
        ImGui::DockBuilderAddNode( dockspaceId, ImGuiDockNodeFlags_DockSpace );
        ImGui::DockBuilderSetNodeSize( dockspaceId, dockspaceSize );

        ImGuiID leftWorkspaceId = 0;
        ImGuiID rightPanelStripId = 0;
        ImGui::DockBuilderSplitNode( dockspaceId, ImGuiDir_Right, 0.43f, &rightPanelStripId,
                                     &leftWorkspaceId );

        ImGui::DockBuilderSplitNode( leftWorkspaceId, ImGuiDir_Up, 0.045f, &m_dockLeftIdUp,
                                     &m_dockLeftIdLeft );

        ImGuiID objectAndSceneId = 0;
        ImGui::DockBuilderSplitNode( rightPanelStripId, ImGuiDir_Right, 0.32f, &m_dockProjectId,
                                     &objectAndSceneId );
        ImGui::DockBuilderSplitNode( objectAndSceneId, ImGuiDir_Right, 0.51f, &m_dockSceneId,
                                     &m_dockObjectId );

        // Retain the legacy IDs used by ImGuiWindow for sensible fallback placement.
        m_dockLeftIdRight = m_dockProjectId;

        ImGui::DockBuilderDockWindow( "Toolbar1", m_dockLeftIdUp );
        ImGui::DockBuilderDockWindow( "Object", m_dockObjectId );
        ImGui::DockBuilderDockWindow( "Object Window", m_dockObjectId );
        ImGui::DockBuilderDockWindow( "Scene", m_dockSceneId );
        ImGui::DockBuilderDockWindow( "Scene Window", m_dockSceneId );
        ImGui::DockBuilderDockWindow( "Project", m_dockProjectId );
        ImGui::DockBuilderDockWindow( "Project Window", m_dockProjectId );
        ImGui::DockBuilderFinish( dockspaceId );
    }

    void ImGuiApplication::showApp( bool *p_open )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManagerPtr();

        auto selectionManager = applicationManager->getSelectionManagerPtr();

        auto ui = workphone::static_pointer_cast<ImGuiManager>( applicationManager->getUI() );
        WP_ASSERT( ui );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto graphicsScene = graphicsSystem->getGraphicsScene();
        WP_ASSERT( graphicsScene );

        auto window = graphicsSystem->getDefaultWindow();
        WP_ASSERT( window );

        auto editorManager = applicationManager->getEditorManager();

        // These services are supplied by the editor, but ImGuiApplication is also
        // used by lightweight applications such as SampleApplication. The editor
        // tools below are conditional on these editor-only services;
        // keep the base menu, dockspace and ordinary windows usable without them.

        m_childWindowCount = 0;

        auto findRenderCamera =
            [&]( const SmartPtr<render::ITexture> &renderTexture ) -> SmartPtr<render::IGraphicsCamera> {
            auto cameraManager = applicationManager->getCameraManager();
            if( cameraManager )
            {
                auto findActorCamera = [&]( const SmartPtr<scene::IGameActor> &cameraActor )
                    -> SmartPtr<render::IGraphicsCamera> {
                    if( cameraActor )
                    {
                        if( auto cameraComponent = cameraActor->getComponent<scene::Camera>() )
                        {
                            if( cameraComponent->isActive() &&
                                cameraComponent->getTargetTexture() == renderTexture )
                            {
                                return cameraComponent->getCamera();
                            }
                        }
                    }

                    return nullptr;
                };

                if( auto camera = findActorCamera( cameraManager->getEditorCamera() ) )
                {
                    return camera;
                }

                for( const auto &cameraActor : cameraManager->getCameras() )
                {
                    if( auto camera = findActorCamera( cameraActor ) )
                    {
                        return camera;
                    }
                }
            }

            return graphicsScene->getActiveCamera();
        };

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if( opt_fullscreen )
        {
            const ImGuiViewport *viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos( viewport->WorkPos );
            ImGui::SetNextWindowSize( viewport->WorkSize );
            ImGui::SetNextWindowViewport( viewport->ID );
            ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
            ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }
        if( dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode )
        {
            window_flags |= ImGuiWindowFlags_NoBackground;
        }

        if( !opt_padding )
        {
            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
        }

        ImGui::Begin( applicationName.c_str(), p_open, window_flags );

        try
        {
            if( !opt_padding )
            {
                ImGui::PopStyleVar();
            }

            if( opt_fullscreen )
            {
                ImGui::PopStyleVar( 2 );
            }

            // Versioning the ID applies this improved default once. ImGui will persist any
            // subsequent rearrangement made by the user.
            const auto dockspaceId = ImGui::GetID( "WorkphoneMainDockspaceV2" );
            if( ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable )
            {
                if( !ImGui::DockBuilderGetNode( dockspaceId ) )
                {
                    setupDefaultDockLayout( dockspaceId, ImGui::GetContentRegionAvail() );
                }

                ImGui::DockSpace( dockspaceId, ImVec2( 0.0f, 0.0f ), dockspace_flags );
            }

            auto menuBar = getMenubar();
            if( menuBar )
            {
                if( ImGui::BeginMenuBar() )
                {
                    try
                    {
                        auto menus = menuBar->getMenus();
                        for( auto &menu : menus )
                        {
                            auto label = menu->getLabel();

                            if( ImGui::BeginMenu( label.c_str() ) )
                            {
                                try
                                {
                                    auto menuItemElements = menu->getMenuItems();
                                    for( auto menuItemElement : menuItemElements )
                                    {
                                        createMenuItem( menu, menuItemElement );
                                    }
                                }
                                catch( std::exception &e )
                                {
                                    WP_LOG_EXCEPTION( e );
                                }

                                ImGui::EndMenu();
                            }
                        }
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }

                    ImGui::EndMenuBar();
                }
            }

            try
            {
                auto toolbar = getToolbar();
                if( toolbar )
                {
                    if( m_dockLeftIdUp != 0 )
                    {
                        ImGui::SetNextWindowDockID( m_dockLeftIdUp, ImGuiCond_FirstUseEver );
                    }
                    createElement( toolbar );

                    if( auto toolbarWindow = ImGui::FindWindowByName( "Toolbar1" ) )
                    {
                        m_dockLeftIdUp = toolbarWindow->DockId;
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            auto renderWindows = ui->getRenderWindows();
            for( auto &renderWindow : renderWindows )
            {
                auto visible = renderWindow->isVisible();
                if( visible )
                {
                    if( !renderWindow->isDocked() )
                    {
                        if( m_dockLeftIdLeft != 0 )
                        {
                            ImGui::SetNextWindowDockID( m_dockLeftIdLeft, ImGuiCond_FirstUseEver );
                        }
                        renderWindow->setDocked( true );
                    }

                    auto label = renderWindow->getLabel();
                    if( StringUtil::isNullOrEmpty( label ) )
                    {
                        static const String defaultLabel = "Untitled";
                        label = defaultLabel;
                    }

                    if( ImGui::Begin( label.c_str(), &visible, ImGuiWindowFlags_None ) )
                    {
                        if( const auto dockId = ImGui::GetWindowDockID(); dockId != 0 )
                        {
                            m_dockLeftIdLeft = dockId;
                        }

                        renderWindow->setVisible( visible, false );

                        // The scene occupies the window's content region, not the full window.
                        // ImGuizmo uses this rectangle both to project the gizmo and to hit-test
                        // the mouse, so including the title bar/padding offsets interaction.
                        const auto sceneRectMin = ImGui::GetCursorScreenPos();
                        const auto sceneRectSize = ImGui::GetContentRegionAvail();

                        draw( renderWindow );

                        ImGuizmo::SetDrawlist();
                        ImGuizmo::SetRect( sceneRectMin.x, sceneRectMin.y, sceneRectSize.x,
                                           sceneRectSize.y );

                        auto viewMatrix = Matrix4F();
                        auto projectionMatrix = Matrix4F();
                        auto renderCamera = findRenderCamera( renderWindow->getRenderTexture() );
                        if( renderCamera )
                        {
                            viewMatrix = renderCamera->getViewMatrix().transpose();
                            projectionMatrix = renderCamera->getProjectionMatrix().transpose();
                        }

                        if( editorManager && sceneManager && renderCamera )
                        {
                            auto transformLocal = editorManager->isTransformLocal();

                            auto position = Vector3F::zero();
                            auto scale = Vector3F::unit();
                            auto rotation = QuaternionF::identity();

                            if( selectionManager )
                            {
                                auto selection = selectionManager->getSelection();
                                Array<SmartPtr<scene::IGameActor>> selectedActors;
                                selectedActors.reserve( selection.size() );

                                if( !selection.empty() )
                                {
                                    for( auto &selected : selection )
                                    {
                                        SmartPtr<scene::IGameActor> actor;
                                        if( selected->isDerived<scene::IGameActor>() )
                                        {
                                            actor = workphone::static_pointer_cast<scene::IGameActor>(
                                                selected );
                                        }
                                        else if( selected->isDerived<scene::IComponent>() )
                                        {
                                            auto component =
                                                workphone::static_pointer_cast<scene::IComponent>(
                                                    selected );
                                            actor = component->getActor();
                                        }

                                        if( actor &&
                                            std::find( selectedActors.begin(), selectedActors.end(),
                                                       actor ) == selectedActors.end() )
                                        {
                                            selectedActors.push_back( actor );
                                        }
                                    }

                                    for( const auto &actor : selectedActors )
                                    {
                                        position += actor->getPosition();
                                    }

                                    if( !selectedActors.empty() )
                                    {
                                        position /= static_cast<f32>( selectedActors.size() );

                                        // ImGuizmo consumes a world-space object matrix. A single
                                        // selection uses its exact transform; a group uses a clean
                                        // pivot so one actor's scale cannot offset the gizmo.
                                        if( selectedActors.size() == 1 )
                                        {
                                            scale = selectedActors.front()->getScale();
                                            rotation = selectedActors.front()->getOrientation();
                                        }
                                        else if( transformLocal )
                                        {
                                            rotation = selectedActors.front()->getOrientation();
                                        }
                                    }
                                }

                                auto transform = Matrix4F();
                                transform.makeTransform( position, scale, rotation );
                                transform = transform.transpose();

                                auto deltaMatrix = Matrix4F();

                                auto showSceneDebug = editorManager->getDrawSceneDebug();
                                if( showSceneDebug )
                                {
                                    auto drawList = ImGui::GetWindowDrawList();
                                    //auto drawList = ImGui::GetBackgroundDrawList();
                                    ImGuizmo::DrawGrid( drawList, viewMatrix.ptr(),
                                                        projectionMatrix.ptr(), identityMatrix, 100.f );
                                }

                                if( auto translateManipulator =
                                        editorManager->getTranslateManipulator() )
                                {
                                    if( translateManipulator->isEnabled() && !selectedActors.empty() )
                                    {
                                        if( Manipulate(
                                                viewMatrix.ptr(), projectionMatrix.ptr(),
                                                ImGuizmo::TRANSLATE,
                                                transformLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD,
                                                transform.ptr(), deltaMatrix.ptr() ) )
                                        {
                                            auto outTransform = deltaMatrix.transpose();
                                            outTransform.decomposition( position, scale, rotation );

                                            if( selectedActors.size() == 1 )
                                            {
                                                auto actor = selectedActors.front();
                                                auto manipulatedTransform = transform.transpose();
                                                Vector3F manipulatedPosition;
                                                Vector3F manipulatedScale;
                                                QuaternionF manipulatedRotation;
                                                manipulatedTransform.decomposition(
                                                    manipulatedPosition, manipulatedScale,
                                                    manipulatedRotation );

                                                // ImGuizmo's object matrix is always in world space,
                                                // including when its axes are displayed in local mode.
                                                if( !actor->getParent() )
                                                {
                                                    actor->setLocalPosition( manipulatedPosition );
                                                }
                                                else
                                                {
                                                    auto parentTransform =
                                                        actor->getParent()->getTransform();
                                                    auto parentWorldTransform =
                                                        parentTransform->getWorldTransform();
                                                    auto parentWorldMatrix =
                                                        parentWorldTransform.getTransformationMatrix();
                                                    auto parentWorldInverse =
                                                        parentWorldMatrix.inverse();
                                                    auto localPosition =
                                                        parentWorldInverse * manipulatedPosition;

                                                    actor->setLocalPosition( localPosition );
                                                }
                                            }
                                            else
                                            {
                                                for( auto actor : selectedActors )
                                                {
                                                    actor->setLocalPosition( actor->getLocalPosition() +
                                                                             position );
                                                }
                                            }

                                            editorManager->refreshTransformUI();
                                        }
                                    }
                                }

                                if( auto rotateManipulor = editorManager->getRotateManipulator() )
                                {
                                    if( rotateManipulor->isEnabled() && !selectedActors.empty() )
                                    {
                                        if( Manipulate(
                                                viewMatrix.ptr(), projectionMatrix.ptr(),
                                                ImGuizmo::ROTATE,
                                                transformLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD,
                                                transform.ptr(), deltaMatrix.ptr() ) )
                                        {
                                            auto outTransform = deltaMatrix.transpose();
                                            outTransform.decomposition( position, scale, rotation );

                                            if( selectedActors.size() == 1 )
                                            {
                                                auto actor = selectedActors.front();
                                                auto manipulatedTransform = transform.transpose();
                                                Vector3F manipulatedPosition;
                                                Vector3F manipulatedScale;
                                                QuaternionF manipulatedRotation;
                                                manipulatedTransform.decomposition(
                                                    manipulatedPosition, manipulatedScale,
                                                    manipulatedRotation );

                                                auto actorTransform = actor->getTransform();
                                                actorTransform->setOrientation( manipulatedRotation );
                                                actorTransform->setLocalDirty( true, false );
                                                sceneManager->addDirtyTransform( actorTransform );
                                                actor->updateTransform();
                                            }
                                            else
                                            {
                                                for( auto actor : selectedActors )
                                                {
                                                    auto actorTransform = actor->getTransform();
                                                    actorTransform->setOrientation(
                                                        actorTransform->getOrientation() * rotation );
                                                    actorTransform->setLocalDirty( true, false );

                                                    sceneManager->addDirtyTransform( actorTransform );
                                                    actor->updateTransform();
                                                }
                                            }
                                        }
                                    }
                                }

                                if( auto scaleManipulator = editorManager->getScaleManipulator() )
                                {
                                    if( scaleManipulator->isEnabled() && !selectedActors.empty() )
                                    {
                                        if( Manipulate(
                                                viewMatrix.ptr(), projectionMatrix.ptr(),
                                                ImGuizmo::SCALE,
                                                transformLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD,
                                                transform.ptr(), deltaMatrix.ptr() ) )
                                        {
                                            auto outTransform = deltaMatrix.transpose();
                                            outTransform.decomposition( position, scale, rotation );

                                            if( selectedActors.size() == 1 )
                                            {
                                                auto actor = selectedActors.front();
                                                auto manipulatedTransform = transform.transpose();
                                                Vector3F manipulatedPosition;
                                                Vector3F manipulatedScale;
                                                QuaternionF manipulatedRotation;
                                                manipulatedTransform.decomposition(
                                                    manipulatedPosition, manipulatedScale,
                                                    manipulatedRotation );

                                                auto actorTransform = actor->getTransform();
                                                actorTransform->setScale( manipulatedScale );
                                                actorTransform->setLocalDirty( true, false );
                                                sceneManager->addDirtyTransform( actorTransform );
                                                actor->updateTransform();
                                            }
                                            else
                                            {
                                                for( auto actor : selectedActors )
                                                {
                                                    auto actorTransform = actor->getTransform();
                                                    actorTransform->setScale(
                                                        actorTransform->getScale() * scale );
                                                    actorTransform->setLocalDirty( true, false );

                                                    sceneManager->addDirtyTransform( actorTransform );
                                                    actor->updateTransform();
                                                }
                                            }
                                        }
                                    }
                                }

                                auto mainWindow = applicationManager->getWindow();
                                auto windowSize = mainWindow->getSize();
                                auto windowSizeF = Vector2F( static_cast<f32>( windowSize.x ),
                                                             static_cast<f32>( windowSize.y ) );

                                auto editorWindowBorderSize = Vector2F( 0.0f, 20.0f / windowSizeF.y );
                                //editorWindowBorderSize = Vector2F( 0.0f, 0.0f );

                                auto referenceSize = Vector2F( 1920.f, 1080.f );
                                editorWindowBorderSize = editorWindowBorderSize * referenceSize;

                                if( auto uiWindow = ui->getMainWindow() )
                                {
                                    auto viewportPosition = uiWindow->getPosition();
                                    auto viewportSize = uiWindow->getSize();

                                    auto winPos = ImGui::GetWindowPos();
                                    viewportPosition = Vector2F( winPos.x, winPos.y );

                                    auto winSize = ImGui::GetWindowSize();
                                    viewportSize = Vector2F( winSize.x, winSize.y );

                                    viewportPosition += editorWindowBorderSize;

                                    auto windowDrawList = ImGui::GetWindowDrawList();

                                    auto drawUiDebug = editorManager->getDrawUiDebug();
                                    if( drawUiDebug )
                                    {
                                        auto actors = sceneManager->getActors();
                                        for( auto &actor : actors )
                                        {
                                            if( actor )
                                            {
                                                auto layoutTransforms =
                                                    actor->getAllComponentsAndInChildren<
                                                        scene::LayoutTransform>();
                                                for( auto &layoutTransform : layoutTransforms )
                                                {
                                                    if( layoutTransform )
                                                    {
                                                        auto layoutTransformActor =
                                                            layoutTransform->getActor();
                                                        auto enabled =
                                                            layoutTransform->isEnabled() &&
                                                            layoutTransformActor->isEnabledInScene();
                                                        if( enabled )
                                                        {
                                                            auto pos =
                                                                layoutTransform->getAbsolutePosition();
                                                            auto size =
                                                                layoutTransform->getAbsoluteSize();

                                                            auto relativePos = pos / referenceSize;
                                                            auto relativeSize = size / referenceSize;

                                                            auto absolutePos =
                                                                viewportPosition +
                                                                ( relativePos * viewportSize );
                                                            auto absoluteSize =
                                                                relativeSize * viewportSize;

                                                            // draw a rectangle
                                                            windowDrawList->AddRect(
                                                                ImVec2( absolutePos.x, absolutePos.y ),
                                                                ImVec2( absolutePos.x + absoluteSize.x,
                                                                        absolutePos.y + absoluteSize.y ),
                                                                ImColor( 255, 255, 255, 255 ) );
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    ImGui::End();
                }
            }

            auto windows = ui->getWindows();
            for( auto window : windows )
            {
                if( window->isVisible() )
                {
                    auto windowParent = window->getParent();
                    if( windowParent == nullptr )
                    {
                        auto label = window->getLabel();
                        if( StringUtil::isNullOrEmpty( label ) )
                        {
                            label = "Untitled";
                        }

                        auto windowTitle = label;
                        if( label == "Object" )
                        {
                            windowTitle = "Object Window";
                        }
                        else if( label == "Scene" )
                        {
                            windowTitle = "Scene Window";
                        }
                        else if( label == "Project" )
                        {
                            windowTitle = "Project Window";
                        }

                        ImGuiID defaultDockId = m_dockLeftIdRight;
                        if( windowTitle == "Object Window" )
                        {
                            defaultDockId = m_dockObjectId;
                        }
                        else if( windowTitle == "Scene Window" )
                        {
                            defaultDockId = m_dockSceneId;
                        }
                        else if( windowTitle == "Project Window" )
                        {
                            defaultDockId = m_dockProjectId;
                        }

                        if( !window->isDocked() && defaultDockId != 0 )
                        {
                            ImGui::SetNextWindowDockID( defaultDockId, ImGuiCond_FirstUseEver );
                        }

                        auto visible = window->isVisible();
                        if( ImGui::Begin( windowTitle.c_str(), &visible, ImGuiWindowFlags_None ) )
                        {
                            try
                            {
                                const auto dockId = ImGui::GetWindowDockID();
                                window->setDocked( dockId != 0 );

                                if( windowTitle == "Object Window" )
                                {
                                    m_dockObjectId = dockId;
                                }
                                else if( windowTitle == "Scene Window" )
                                {
                                    m_dockSceneId = dockId;
                                }
                                else if( windowTitle == "Project Window" )
                                {
                                    m_dockProjectId = dockId;
                                    m_dockLeftIdRight = dockId;
                                }

                                auto children = window->getChildren();
                                for( auto &child : children )
                                {
                                    createElement( child );
                                }

                                auto dropTarget = window->getDropTarget();
                                if( dropTarget )
                                {
                                    if( ImGui::BeginDragDropTarget() )
                                    {
                                        auto payload = ImGui::AcceptDragDropPayload( "_TREENODE" );
                                        if( payload )
                                        {
                                            auto pData = static_cast<const char *>( payload->Data );
                                            auto dataSize = payload->DataSize;
                                            auto data = String( pData, dataSize );

                                            auto args = Array<Parameter>();
                                            args.reserve( 1 );

                                            args.emplace_back( data );

                                            dropTarget->handleEvent( EventType::UI, IEvent::handleDrop,
                                                                     args, window, window, nullptr );
                                        }

                                        ImGui::EndDragDropTarget();
                                    }
                                }

                                if( auto contextMenu = window->getContextMenu() )
                                {
                                    if( ImGui::BeginPopupContextWindow(
                                            windowTitle.c_str(), ImGuiPopupFlags_MouseButtonRight ) )
                                    {
                                        try
                                        {
                                            auto label = contextMenu->getLabel();
                                            if( StringUtil::isNullOrEmpty( label ) )
                                            {
                                                label = "Untitled";
                                            }

                                            //if( ImGui::BeginMenu( label.c_str() ) )
                                            {
                                                try
                                                {
                                                    auto menuItems = contextMenu->getMenuItems();
                                                    for( auto menuItemElement : menuItems )
                                                    {
                                                        createMenuItem( contextMenu, menuItemElement );
                                                    }
                                                }
                                                catch( std::exception &e )
                                                {
                                                    WP_LOG_EXCEPTION( e );
                                                }

                                                //ImGui::EndMenu();
                                            }
                                        }
                                        catch( std::exception &e )
                                        {
                                            WP_LOG_EXCEPTION( e );
                                        }

                                        ImGui::EndPopup();
                                    }
                                }

                                const bool is_hovered = ImGui::IsItemHovered();  // Hovered
                                const bool is_active = ImGui::IsItemActive();    // Held

                                if( is_hovered && ImGui::IsMouseClicked( 0 ) )
                                {
                                    auto listeners = window->getObjectListeners();
                                    for( auto listener : listeners )
                                    {
                                        auto args = Array<Parameter>();
                                        args.resize( 1 );

                                        args[0].object = window;

                                        listener->handleEvent( EventType::UI, IEvent::handleMouseClicked,
                                                               args, window, this, nullptr );
                                    }
                                }

                                window->setVisible( visible, false );
                            }
                            catch( std::exception &e )
                            {
                                WP_LOG_EXCEPTION( e );
                            }
                        }

                        ImGui::End();
                    }
                }
            }

            try
            {
                auto fileDialogs = ui->getFileBrowsers();
                for( auto fileDialog : fileDialogs )
                {
                    fileDialog->update();
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            // if (m_showEditor)
            //{
            //	ShowEditor(&m_showEditor);
            // }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        ImGui::End();
    }

    void ImGuiApplication::draw( SmartPtr<IUIRenderWindow> renderWindow )
    {
        try
        {
            if( auto renderTexture = renderWindow->getRenderTexture() )
            {
                auto pos = ImGui::GetWindowPos();
                auto avail_size = ImGui::GetContentRegionAvail();

                void *pTexture = nullptr;
                renderTexture->_getObject( &pTexture );

                if( pTexture )
                {
                    // Submit the render texture directly to the window draw list. ImGui::Image
                    // creates a hovered item, which makes ImGuizmo's activation guard reject
                    // mouse presses over the scene.
                    const auto imageMin = ImGui::GetCursorScreenPos();
                    const auto imageMax = ImVec2( imageMin.x + avail_size.x, imageMin.y + avail_size.y );
                    ImGui::GetWindowDrawList()->AddImage( pTexture, imageMin, imageMax );
                }

                // Keep the editor viewport laid out and sized even when a renderer does not
                // yet expose an off-screen texture. This also lets that renderer allocate the
                // target from the requested dimensions on a later frame.
                ImGui::Dummy( avail_size );

                auto textureSize =
                    Vector2I( static_cast<s32>( avail_size.x ), static_cast<s32>( avail_size.y ) );
                renderTexture->setSize( textureSize );

                renderWindow->setPosition( Vector2F( pos.x, pos.y ) );
                renderWindow->setSize( Vector2<real_Num>( static_cast<real_Num>( textureSize.x ),
                                                          static_cast<real_Num>( textureSize.y ) ) );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    bool ImGuiApplication::getUseInputEvents() const
    {
        return m_useInputEvents;
    }

    void ImGuiApplication::setUseInputEvents( bool useInputEvents )
    {
        m_useInputEvents = useInputEvents;
    }

    void *ImGuiApplication::getEmptyTexture() const
    {
        return m_emptyTexture;
    }

    void ImGuiApplication::setEmptyTexture( void *emptyTexture )
    {
        m_emptyTexture = emptyTexture;
    }

    ImGuiOverlayOgre *ImGuiApplication::getOverlay()
    {
        return m_overlay;
    }

    void ImGuiApplication::createMenuItem( SmartPtr<IUIMenu> rootMenu,
                                           SmartPtr<IUIElement> menuItemElement )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();

        if( menuItemElement->isDerived<IUIMenu>() )
        {
            auto menu = workphone::static_pointer_cast<IUIMenu>( menuItemElement );
            auto menuLabel = menu->getLabel();

            if( ImGui::BeginMenu( menuLabel.c_str() ) )
            {
                auto subMenuElements = menu->getMenuItems();

                for( auto subMenuElement : subMenuElements )
                {
                    if( subMenuElement->isDerived<IUIMenu>() )
                    {
                        auto subMenu = workphone::static_pointer_cast<IUIMenu>( subMenuElement );

                        auto label = subMenu->getLabel();
                        if( StringUtil::isNullOrEmpty( label ) )
                        {
                            label = "Untitled";
                        }

                        if( ImGui::BeginMenu( label.c_str() ) )
                        {
                            auto menuItemElements = subMenu->getMenuItems();
                            for( auto &menuItemElement : menuItemElements )
                            {
                                createMenuItem( rootMenu, menuItemElement );
                            }

                            ImGui::EndMenu();
                        }
                    }
                    else if( subMenuElement->isDerived<IUIMenuItem>() )
                    {
                        createMenuItem( rootMenu, subMenuElement );
                    }
                }

                ImGui::EndMenu();
            }
        }
        else if( menuItemElement->isDerived<IUIMenuItem>() )
        {
            auto menuItem = workphone::static_pointer_cast<IUIMenuItem>( menuItemElement );
            auto menuItemType = menuItem->getMenuItemType();
            switch( menuItemType )
            {
            case IUIMenuItem::Type::Normal:
            {
                auto text = menuItem->getText();

                auto selected = false;
                ImGui::MenuItem( text.c_str(), nullptr, &selected );

                if( selected )
                {
                    auto parent = rootMenu->getParent();
                    if( parent && parent->isDerived<IUIMenubar>() )
                    {
                        auto menuBar = workphone::static_pointer_cast<IUIMenubar>( parent );

                        auto args = Array<Parameter>();
                        applicationManager->triggerEvent( EventType::UI, IEvent::handleSelection, args,
                                                          menuBar, menuItem, nullptr, false,
                                                          Thread::Application_Flag );
                    }
                    else
                    {
                        auto args = Array<Parameter>();
                        applicationManager->triggerEvent( EventType::UI, IEvent::handleSelection, args,
                                                          rootMenu, menuItem, nullptr, false,
                                                          Thread::Application_Flag );
                    }
                }
            }
            break;
            case IUIMenuItem::Type::Separator:
            {
                ImGui::Separator();
            }
            break;
            default:
            {
            }
            }
        }
    }

    Vector2I ImGuiApplication::getWindowSize() const
    {
        return Vector2I::zero();
    }

    void ImGuiApplication::setWindowSize( const Vector2I &size )
    {
        if( m_size != size )
        {
            m_size = size;

            //auto viewport = ImGui::GetMainViewport();
            // ImGui_ImplDX11_SetWindowSize(viewport, ImVec2(size.X(), size.Y()));

            // ImGui_ImplDX11_InvalidateDeviceObjects();
            // ImGui_ImplDX11_CreateDeviceObjects();
        }
    }

#if WP_BUILD_SDL2
    SDL_Texture *ImGuiApplication::IMG_LoadTexture( SDL_Renderer *renderer, const char *file )
    {
        SDL_Texture *texture = NULL;
        SDL_Surface *surface = IMG_Load( file );
        if( surface )
        {
            texture = SDL_CreateTextureFromSurface( renderer, surface );
            SDL_FreeSurface( surface );
        }
        return texture;
    }
#endif

    static ImGuizmo::OPERATION mCurrentGizmoOperation( ImGuizmo::TRANSLATE );

    // Camera projection
    bool isPerspective = true;
    float fov = 27.f;
    float viewWidth = 10.f;  // for orthographic
    float camYAngle = 165.f / 180.f * 3.14159f;
    float camXAngle = 32.f / 180.f * 3.14159f;

    bool firstFrame = true;

    void ImGuiApplication::editTransform( float *cameraView, float *cameraProjection, float *matrix,
                                          bool editTransformDecomposition )
    {
        static ImGuizmo::MODE mCurrentGizmoMode( ImGuizmo::LOCAL );
        static bool useSnap = false;
        static float snap[3] = { 1.f, 1.f, 1.f };
        static float bounds[] = { -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f };
        static float boundsSnap[] = { 0.1f, 0.1f, 0.1f };
        static bool boundSizing = false;
        static bool boundSizingSnap = false;

        if( editTransformDecomposition )
        {
            if( ImGui::IsKeyPressed( ImGuiKey_Z ) )
            {
                mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
            }
            if( ImGui::IsKeyPressed( ImGuiKey_E ) )
            {
                mCurrentGizmoOperation = ImGuizmo::ROTATE;
            }
            if( ImGui::IsKeyPressed( ImGuiKey_R ) )
            {
                // r Key
                mCurrentGizmoOperation = ImGuizmo::SCALE;
            }
            if( ImGui::RadioButton( "Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE ) )
            {
                mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
            }
            ImGui::SameLine();
            if( ImGui::RadioButton( "Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE ) )
            {
                mCurrentGizmoOperation = ImGuizmo::ROTATE;
            }
            ImGui::SameLine();
            if( ImGui::RadioButton( "Scale", mCurrentGizmoOperation == ImGuizmo::SCALE ) )
            {
                mCurrentGizmoOperation = ImGuizmo::SCALE;
            }
            if( ImGui::RadioButton( "Universal", mCurrentGizmoOperation == ImGuizmo::UNIVERSAL ) )
            {
                mCurrentGizmoOperation = ImGuizmo::UNIVERSAL;
            }
            float matrixTranslation[3], matrixRotation[3], matrixScale[3];
            ImGuizmo::DecomposeMatrixToComponents( matrix, matrixTranslation, matrixRotation,
                                                   matrixScale );
            ImGui::InputFloat3( "Tr", matrixTranslation );
            ImGui::InputFloat3( "Rt", matrixRotation );
            ImGui::InputFloat3( "Sc", matrixScale );
            ImGuizmo::RecomposeMatrixFromComponents( matrixTranslation, matrixRotation, matrixScale,
                                                     matrix );

            if( mCurrentGizmoOperation != ImGuizmo::SCALE )
            {
                if( ImGui::RadioButton( "Local", mCurrentGizmoMode == ImGuizmo::LOCAL ) )
                {
                    mCurrentGizmoMode = ImGuizmo::LOCAL;
                }
                ImGui::SameLine();
                if( ImGui::RadioButton( "World", mCurrentGizmoMode == ImGuizmo::WORLD ) )
                {
                    mCurrentGizmoMode = ImGuizmo::WORLD;
                }
            }
            if( ImGui::IsKeyPressed( ImGuiKey_S ) )
            {
                useSnap = !useSnap;
            }

            // return;
            // ImGui::Checkbox("", &useSnap);
            ImGui::SameLine();

            switch( mCurrentGizmoOperation )
            {
            case ImGuizmo::TRANSLATE:
                ImGui::InputFloat3( "Snap", &snap[0] );
                break;
            case ImGuizmo::ROTATE:
                ImGui::InputFloat( "Angle Snap", &snap[0] );
                break;
            case ImGuizmo::SCALE:
                ImGui::InputFloat( "Scale Snap", &snap[0] );
                break;
            }
            ImGui::Checkbox( "Bound Sizing", &boundSizing );
            if( boundSizing )
            {
                ImGui::PushID( 3 );
                ImGui::Checkbox( "", &boundSizingSnap );
                ImGui::SameLine();
                ImGui::InputFloat3( "Snap", boundsSnap );
                ImGui::PopID();
            }
        }

        ImGuiIO &io = ImGui::GetIO();
        float viewManipulateRight = io.DisplaySize.x;
        float viewManipulateTop = 0;
        static ImGuiWindowFlags gizmoWindowFlags = 0;
        if( m_useWindow )
        {
            ImGui::SetNextWindowSize( ImVec2( 800, 400 ), ImGuiCond_Appearing );
            ImGui::SetNextWindowPos( ImVec2( 400, 20 ), ImGuiCond_Appearing );
            ImGui::PushStyleColor( ImGuiCol_WindowBg,
                                   static_cast<ImVec4>( ImColor( 0.35f, 0.3f, 0.3f ) ) );
            ImGui::Begin( "Gizmo", nullptr, gizmoWindowFlags );
            ImGuizmo::SetDrawlist();
            float windowWidth = ImGui::GetWindowWidth();
            float windowHeight = ImGui::GetWindowHeight();
            ImGuizmo::SetRect( ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth,
                               windowHeight );
            viewManipulateRight = ImGui::GetWindowPos().x + windowWidth;
            viewManipulateTop = ImGui::GetWindowPos().y;
            auto window = ImGui::GetCurrentWindow();
            gizmoWindowFlags =
                ImGui::IsWindowHovered() &&
                        ImGui::IsMouseHoveringRect( window->InnerRect.Min, window->InnerRect.Max )
                    ? ImGuiWindowFlags_NoMove
                    : 0;
        }
        else
        {
            ImGuizmo::SetRect( 0, 0, io.DisplaySize.x, io.DisplaySize.y );
        }

        //ImGuizmo::DrawGrid( cameraView, cameraProjection, identityMatrix, 100.f );
        ImGuizmo::DrawCubes( cameraView, cameraProjection, &objectMatrix[0][0], m_gizmoCount );
        Manipulate( cameraView, cameraProjection, mCurrentGizmoOperation, mCurrentGizmoMode, matrix,
                    nullptr, useSnap ? &snap[0] : nullptr, boundSizing ? bounds : nullptr,
                    boundSizingSnap ? boundsSnap : nullptr );

        ImGuizmo::ViewManipulate( cameraView, m_camDistance,
                                  ImVec2( viewManipulateRight - 128, viewManipulateTop ),
                                  ImVec2( 128, 128 ), 0x10101010 );

        if( m_useWindow )
        {
            ImGui::End();
            ImGui::PopStyleColor( 1 );
        }
    }

    void ImGuiApplication::showEditor( bool *p_open )
    {
        // create a window and insert the inspector
        ImGui::SetNextWindowPos( ImVec2( 10, 10 ), ImGuiCond_Appearing );
        ImGui::SetNextWindowSize( ImVec2( 320, 340 ), ImGuiCond_Appearing );
        ImGui::Begin( "Editor" );

        if( ImGui::RadioButton( "Full view", !m_useWindow ) )
        {
            m_useWindow = false;
        }

        ImGui::SameLine();

        if( ImGui::RadioButton( "Window", m_useWindow ) )
        {
            m_useWindow = true;
        }

        ImGui::Text( "Camera" );
        bool viewDirty = false;

        if( ImGui::RadioButton( "Perspective", isPerspective ) )
        {
            isPerspective = true;
        }

        ImGui::SameLine();

        if( ImGui::RadioButton( "Orthographic", !isPerspective ) )
        {
            isPerspective = false;
        }

        if( isPerspective )
        {
            ImGui::SliderFloat( "Fov", &fov, 20.f, 110.f );
        }
        else
        {
            ImGui::SliderFloat( "Ortho width", &viewWidth, 1, 20 );
        }

        viewDirty |= ImGui::SliderFloat( "Distance", &m_camDistance, 1.f, 10.f );
        ImGui::SliderInt( "Gizmo count", &m_gizmoCount, 1, 4 );

        if( viewDirty || firstFrame )
        {
            float eye[] = { cosf( camYAngle ) * cosf( camXAngle ) * m_camDistance,
                            sinf( camXAngle ) * m_camDistance,
                            sinf( camYAngle ) * cosf( camXAngle ) * m_camDistance };
            float at[] = { 0.f, 0.f, 0.f };
            float up[] = { 0.f, 1.f, 0.f };
            LookAt( eye, at, up, cameraView );
            firstFrame = false;
        }

        // ImGui::Text("X: %f Y: %f", io.MousePos.x, io.MousePos.y);

        if( ImGuizmo::IsUsing() )
        {
            ImGui::Text( "Using gizmo" );
        }
        else
        {
            ImGui::Text( ImGuizmo::IsOver() ? "Over gizmo" : "" );
            ImGui::SameLine();
            ImGui::Text( IsOver( ImGuizmo::TRANSLATE ) ? "Over translate gizmo" : "" );
            ImGui::SameLine();
            ImGui::Text( IsOver( ImGuizmo::ROTATE ) ? "Over rotate gizmo" : "" );
            ImGui::SameLine();
            ImGui::Text( IsOver( ImGuizmo::SCALE ) ? "Over scale gizmo" : "" );
        }

        ImGui::Separator();

        for( s32 matId = 0; matId < m_gizmoCount; matId++ )
        {
            ImGuizmo::SetID( matId );

            editTransform( cameraView, cameraProjection, objectMatrix[matId], m_lastUsing == matId );
            if( ImGuizmo::IsUsing() )
            {
                m_lastUsing = matId;
            }
        }

        ImGui::End();
    }

    void ImGuiApplication::showGuizmo()
    {
        ImGuiIO &io = ImGui::GetIO();
        if( isPerspective )
        {
            Perspective( fov, io.DisplaySize.x / io.DisplaySize.y, 0.1f, 100.f, cameraProjection );
        }
        else
        {
            float viewHeight = viewWidth * io.DisplaySize.y / io.DisplaySize.x;
            OrthoGraphic( -viewWidth, viewWidth, -viewHeight, viewHeight, 1000.f, -1000.f,
                          cameraProjection );
        }

        ImGuizmo::SetOrthographic( !isPerspective );
        ImGuizmo::BeginFrame();

        ImGui::SetNextWindowPos( ImVec2( 1024, 100 ), ImGuiCond_Appearing );
        ImGui::SetNextWindowSize( ImVec2( 256, 256 ), ImGuiCond_Appearing );

        // create a window and insert the inspector
        ImGui::SetNextWindowPos( ImVec2( 10, 10 ), ImGuiCond_Appearing );
        ImGui::SetNextWindowSize( ImVec2( 320, 340 ), ImGuiCond_Appearing );
        ImGui::Begin( "Editor" );

        if( ImGui::RadioButton( "Full view", !m_useWindow ) )
        {
            m_useWindow = false;
        }

        ImGui::SameLine();

        if( ImGui::RadioButton( "Window", m_useWindow ) )
        {
            m_useWindow = true;
        }

        ImGui::Text( "Camera" );
        bool viewDirty = false;
        if( ImGui::RadioButton( "Perspective", isPerspective ) )
        {
            isPerspective = true;
        }
        ImGui::SameLine();
        if( ImGui::RadioButton( "Orthographic", !isPerspective ) )
        {
            isPerspective = false;
        }
        if( isPerspective )
        {
            ImGui::SliderFloat( "Fov", &fov, 20.f, 110.f );
        }
        else
        {
            ImGui::SliderFloat( "Ortho width", &viewWidth, 1, 20 );
        }
        viewDirty |= ImGui::SliderFloat( "Distance", &m_camDistance, 1.f, 10.f );
        ImGui::SliderInt( "Gizmo count", &m_gizmoCount, 1, 4 );

        if( viewDirty || firstFrame )
        {
            float eye[] = { cosf( camYAngle ) * cosf( camXAngle ) * m_camDistance,
                            sinf( camXAngle ) * m_camDistance,
                            sinf( camYAngle ) * cosf( camXAngle ) * m_camDistance };
            float at[] = { 0.f, 0.f, 0.f };
            float up[] = { 0.f, 1.f, 0.f };
            LookAt( eye, at, up, cameraView );
            firstFrame = false;
        }

        ImGui::Text( "X: %f Y: %f", io.MousePos.x, io.MousePos.y );
        if( ImGuizmo::IsUsing() )
        {
            ImGui::Text( "Using gizmo" );
        }
        else
        {
            ImGui::Text( ImGuizmo::IsOver() ? "Over gizmo" : "" );
            ImGui::SameLine();
            ImGui::Text( IsOver( ImGuizmo::TRANSLATE ) ? "Over translate gizmo" : "" );
            ImGui::SameLine();
            ImGui::Text( IsOver( ImGuizmo::ROTATE ) ? "Over rotate gizmo" : "" );
            ImGui::SameLine();
            ImGui::Text( IsOver( ImGuizmo::SCALE ) ? "Over scale gizmo" : "" );
        }

        ImGui::Separator();

        for( int matId = 0; matId < m_gizmoCount; matId++ )
        {
            ImGuizmo::SetID( matId );

            editTransform( cameraView, cameraProjection, objectMatrix[matId], m_lastUsing == matId );
            if( ImGuizmo::IsUsing() )
            {
                m_lastUsing = matId;
            }
        }

        ImGui::End();

        ImGui::SetNextWindowPos( ImVec2( 10, 350 ), ImGuiCond_Appearing );

        ImGui::SetNextWindowSize( ImVec2( 940, 480 ), ImGuiCond_Appearing );
        // ImGui::Begin("Other controls");
        // if (ImGui::CollapsingHeader("Zoom Slider"))
        //{
        //	static float uMin = 0.4f, uMax = 0.6f;
        //	static float vMin = 0.4f, vMax = 0.6f;
        //	ImGui::Image((ImTextureID)(uint64_t)procTexture, ImVec2(900, 300), ImVec2(uMin, vMin),
        // ImVec2(uMax, vMax));
        //	{
        //		ImGui::SameLine();
        //		ImGui::PushID(18);
        //		ImZoomSlider::ImZoomSlider(0.f, 1.f, vMin, vMax, 0.01f,
        // ImZoomSlider::ImGuiZoomSliderFlags_Vertical); 		ImGui::PopID();
        //	}

        //	{
        //		ImGui::PushID(19);
        //		ImZoomSlider::ImZoomSlider(0.f, 1.f, uMin, uMax);
        //		ImGui::PopID();
        //	}
        //}
        // if (ImGui::CollapsingHeader("Sequencer"))
        //{
        //	// let's create the sequencer
        //	static int selectedEntry = -1;
        //	static int firstFrame = 0;
        //	static bool expanded = true;
        //	static int currentFrame = 100;

        //	ImGui::PushItemWidth(130);
        //	ImGui::InputInt("Frame Min", &mySequence.mFrameMin);
        //	ImGui::SameLine();
        //	ImGui::InputInt("Frame ", &currentFrame);
        //	ImGui::SameLine();
        //	ImGui::InputInt("Frame Max", &mySequence.mFrameMax);
        //	ImGui::PopItemWidth();
        //	Sequencer(&mySequence, &currentFrame, &expanded, &selectedEntry, &firstFrame,
        // ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_ADD |
        // ImSequencer::SEQUENCER_DEL | ImSequencer::SEQUENCER_COPYPASTE |
        // ImSequencer::SEQUENCER_CHANGE_FRAME);
        //	// add a UI to edit that particular item
        //	if (selectedEntry != -1)
        //	{
        //		const MySequence::MySequenceItem& item = mySequence.myItems[selectedEntry];
        //		ImGui::Text("I am a %s, please edit me", SequencerItemTypeNames[item.mType]);
        //		// switch (type) ....
        //	}
        //}

        //// Graph Editor
        // static GraphEditor::Options options;
        // static GraphEditorDelegate delegate;
        // static GraphEditor::ViewState viewState;
        // static GraphEditor::FitOnScreen fit = GraphEditor::Fit_None;
        // static bool showGraphEditor = true;

        // if (ImGui::CollapsingHeader("Graph Editor"))
        //{
        //	ImGui::Checkbox("Show GraphEditor", &showGraphEditor);
        //	GraphEditor::EditOptions(options);
        // }

        // ImGui::End();

        // if (showGraphEditor)
        //{
        //	ImGui::Begin("Graph Editor", NULL, 0);
        //	if (ImGui::Button("Fit all nodes"))
        //	{
        //		fit = GraphEditor::Fit_AllNodes;
        //	}
        //	ImGui::SameLine();
        //	if (ImGui::Button("Fit selected nodes"))
        //	{
        //		fit = GraphEditor::Fit_SelectedNodes;
        //	}
        //	GraphEditor::Show(delegate, options, viewState, true, &fit);

        //	ImGui::End();
        //}
    }

    const float toolbarSize = 50;
    float menuBarHeight = 60.0f;

    void ImGuiApplication::dockSpaceUI()
    {
        ImGuiViewport *viewport = ImGui::GetMainViewport();

        auto toolbarSizeVec = ImVec2( 0, toolbarSize );
        ImGui::SetNextWindowPos(
            ImVec2( viewport->Pos.x + toolbarSizeVec.x, viewport->Pos.y + toolbarSizeVec.y ) );
        ImGui::SetNextWindowSize(
            ImVec2( viewport->Size.x - toolbarSizeVec.x, viewport->Size.y - toolbarSizeVec.y ) );
        ImGui::SetNextWindowViewport( viewport->ID );
        ImGuiWindowFlags window_flags =
            0 | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
        ImGui::Begin( "Master DockSpace", nullptr, window_flags );
        ImGuiID dockMain = ImGui::GetID( "MyDockspace" );

        // Save off menu bar height for later.
        menuBarHeight = ImGui::GetCurrentWindow()->MenuBarHeight();

        ImGui::DockSpace( dockMain );
        ImGui::End();
        ImGui::PopStyleVar( 3 );
    }

    void ImGuiApplication::toolbarUI()
    {
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos( ImVec2( viewport->Pos.x, viewport->Pos.y + menuBarHeight ) );
        ImGui::SetNextWindowSize( ImVec2( viewport->Size.x, toolbarSize ) );
        ImGui::SetNextWindowViewport( viewport->ID );

        ImGuiWindowFlags window_flags = 0 | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
        ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0 );
        ImGui::Begin( "TOOLBAR", nullptr, window_flags );
        ImGui::PopStyleVar();

        ImGui::Button( "Toolbar goes here", ImVec2( 0, 37 ) );
        ImGui::Button( "Toolbar goes here 2", ImVec2( 100, 37 ) );

        ImGui::End();
    }

    // Toolbar test [Experimental]
    // Usage:
    // {
    //   static ImGuiAxis toolbar1_axis = ImGuiAxis_X; // Your storage for the current direction.
    //   DockingToolbar("Toolbar1", &toolbar1_axis);
    // }
    void ImGuiApplication::dockingToolbar( const char *name, ImGuiAxis *p_toolbar_axis,
                                           SmartPtr<IUIElement> toolbarElement )
    {
        // [Option] Automatically update axis based on parent split (inside of doing it via
        // right-click on the toolbar) Pros:
        // - Less user intervention.
        // - Avoid for need for saving the toolbar direction, since it's automatic.
        // Cons:
        // - This is currently leading to some glitches.
        // - Some docking setup won't return the axis the user would expect.
        const bool TOOLBAR_AUTO_DIRECTION_WHEN_DOCKED = true;

        // ImGuiAxis_X = horizontal toolbar
        // ImGuiAxis_Y = vertical toolbar
        ImGuiAxis toolbar_axis = *p_toolbar_axis;

        // 1. We request auto-sizing on one axis
        // Note however this will only affect the toolbar when NOT docked.
        ImVec2 requested_size =
            ( toolbar_axis == ImGuiAxis_X ) ? ImVec2( -1.0f, 0.0f ) : ImVec2( 0.0f, -1.0f );
        ImGui::SetNextWindowSize( requested_size );

        // 2. Specific docking options for toolbars.
        // Currently they add some constraint we ideally wouldn't want, but this is simplifying our
        // first implementation
        ImGuiWindowClass window_class;
        window_class.DockingAllowUnclassed = true;
        window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoCloseButton;
        window_class.DockNodeFlagsOverrideSet |=
            ImGuiDockNodeFlags_HiddenTabBar;  // ImGuiDockNodeFlags_NoTabBar // FIXME: Will need a
        // working Undock widget for _NoTabBar to work
        window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoDockingSplitMe;
        window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoDockingOverMe;
        window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoDockingOverOther;
        if( toolbar_axis == ImGuiAxis_X )
        {
            window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoResizeY;
        }
        else
        {
            window_class.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoResizeX;
        }
        ImGui::SetNextWindowClass( &window_class );

        // 3. Begin into the window
        const float font_size = ImGui::GetFontSize();
        const ImVec2 icon_size( ImFloor( font_size * 1.7f ), ImFloor( font_size * 1.7f ) );
        ImGui::Begin(
            name, nullptr,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar );

        // 4. Overwrite node size
        ImGuiDockNode *node = ImGui::GetWindowDockNode();
        if( node != nullptr )
        {
            // Overwrite size of the node
            ImGuiStyle &style = ImGui::GetStyle();
            const auto toolbar_axis_perp = static_cast<ImGuiAxis>( toolbar_axis ^ 1 );
            const float TOOLBAR_SIZE_WHEN_DOCKED =
                style.WindowPadding[toolbar_axis_perp] * 2.0f + icon_size[toolbar_axis_perp];
            node->WantLockSizeOnce = true;
            node->Size[toolbar_axis_perp] = node->SizeRef[toolbar_axis_perp] = TOOLBAR_SIZE_WHEN_DOCKED;

            if( TOOLBAR_AUTO_DIRECTION_WHEN_DOCKED )
            {
                if( node->ParentNode && node->ParentNode->SplitAxis != ImGuiAxis_None )
                {
                    toolbar_axis = static_cast<ImGuiAxis>( node->ParentNode->SplitAxis ^ 1 );
                }
            }
        }

        // 5. Dummy populate tab bar
        ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 3.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 5.0f, 5.0f ) );

        // UndockWidget(icon_size, toolbar_axis);
        // for (int icon_n = 0; icon_n < 10; icon_n++)
        //{
        //	char label[32];
        //	ImFormatString(label, IM_ARRAYSIZE(label), "%02d", icon_n);
        //	if (icon_n > 0 && toolbar_axis == ImGuiAxis_X)
        //		ImGui::SameLine();

        //	ImGui::Button(label, icon_size);
        //}

        // ImGui::ImageButton(my_tex_id, size);

        auto childCount = 0;

        auto children = toolbarElement->getChildren();
        for( auto &child : children )
        {
            if( childCount > 0 && toolbar_axis == ImGuiAxis_X )
            {
                ImGui::SameLine();
            }

            createElement( child );

            childCount++;
        }

        ImGui::PopStyleVar( 2 );

        // 6. Context-menu to change axis
        if( node == nullptr || !TOOLBAR_AUTO_DIRECTION_WHEN_DOCKED )
        {
            if( ImGui::BeginPopupContextWindow() )
            {
                ImGui::TextUnformatted( name );
                ImGui::Separator();

                if( ImGui::MenuItem( "Horizontal", "", ( toolbar_axis == ImGuiAxis_X ) ) )
                {
                    toolbar_axis = ImGuiAxis_X;
                }

                if( ImGui::MenuItem( "Vertical", "", ( toolbar_axis == ImGuiAxis_Y ) ) )
                {
                    toolbar_axis = ImGuiAxis_Y;
                }

                ImGui::EndPopup();
            }
        }

        ImGui::End();

        // Output user stored data
        *p_toolbar_axis = toolbar_axis;
    }

    void ImGuiApplication::update()
    {
        ScopedLock lock( this, true );

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto timer = applicationManager->getTimerPtr();
        WP_ASSERT( timer );

        if( timer->getTimeSinceSceneLoad() < 0.1 )
        {
            return;
        }

        auto ui = applicationManager->getUI();
        WP_ASSERT( ui );

        auto application = ui->getApplication();
        WP_ASSERT( application );

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        WP_ASSERT( graphicsSystem );

        static auto show_demo_window = true;
        if( show_demo_window )
        {
            //ImGui::ShowDemoWindow( &show_demo_window );
        }

        ImGuiIO &io = ImGui::GetIO();
        if( isPerspective )
        {
            Perspective( fov, io.DisplaySize.x / io.DisplaySize.y, 0.1f, 100.f, cameraProjection );
        }
        else
        {
            auto viewHeight = viewWidth * io.DisplaySize.y / io.DisplaySize.x;
            OrthoGraphic( -viewWidth, viewWidth, -viewHeight, viewHeight, 1000.f, -1000.f,
                          cameraProjection );
        }

        ImGuizmo::SetOrthographic( !isPerspective );
        ImGuizmo::BeginFrame();

        if( m_showDockSpace )
        {
            showApp( &m_showDockSpace );
        }
    }

    void *ImGuiApplication::getHWND() const
    {
        return m_hwnd;
    }

    SmartPtr<IUIMenubar> ImGuiApplication::getMenubar() const
    {
        return m_menuBar;
    }

    void ImGuiApplication::setMenubar( SmartPtr<IUIMenubar> menubar )
    {
        m_menuBar = menubar;
    }

    SmartPtr<IUIToolbar> ImGuiApplication::getToolbar() const
    {
        return m_toolbar;
    }

    void ImGuiApplication::setToolbar( SmartPtr<IUIToolbar> toolbar )
    {
        m_toolbar = toolbar;
    }

    void ImGuiApplication::WindowListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

    void ImGuiApplication::WindowListener::handleEvent( SmartPtr<render::IGraphicsWindowEvent> event )
    {
        if( auto owner = getOwner() )
        {
            owner->handleWindowEvent( event );
        }
    }

    Parameter ImGuiApplication::WindowListener::handleEvent( EventType eventType, hash_type eventValue,
                                                             const Array<Parameter> &arguments,
                                                             SmartPtr<ISharedObject> sender,
                                                             SmartPtr<ISharedObject> object,
                                                             SmartPtr<IEvent> event )
    {
        if( eventValue == windowClosingHash )
        {
            return Parameter( true );
        }

        return {};
    }

    void ImGuiApplication::WindowListener::setOwner( SmartPtr<ImGuiApplication> owner )
    {
        m_owner = owner;
    }

    SmartPtr<ImGuiApplication> ImGuiApplication::WindowListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    ImGuiApplication::WindowListener::WindowListener() = default;

    ImGuiApplication::WindowListener::~WindowListener() = default;
}  // namespace workphone::ui
