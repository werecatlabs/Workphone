/**
 * @file workphone_platform_window_win32.c
 * @brief Win32 native window implementation.
 */

#ifdef _WIN32

#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#    include <stdlib.h>
#    include <string.h>

#    include "workphone_platform_window_win32.h"

/* =========================================================================
 * Internal constants
 * ====================================================================== */

#    define WP_WIN32_CLASS_NAME L"WorkphoneWindowClass"

/* =========================================================================
 * Internal state
 * ====================================================================== */

static wp_s32 s_class_registered = 0;
static wp_u32 s_window_count = 0;

/* =========================================================================
 * Internal structure
 * ====================================================================== */

struct wp_platform_window_win32
{
    HWND hwnd;
    wp_u32 width;
    wp_u32 height;
    wp_s32 closed;
    wp_c8 title[WP_PLATFORM_WINDOW_MAX_TITLE];
    wp_platform_window_win32_event_callback event_callback;
    void *event_userdata;
};

/* =========================================================================
 * WndProc
 * ====================================================================== */

static LRESULT CALLBACK wp_win32_wnd_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    wp_platform_window_win32 *win = (wp_platform_window_win32 *)GetWindowLongPtrW( hwnd, GWLP_USERDATA );

    switch( msg )
    {
    case WM_NCCREATE:
    {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)lparam;
        SetWindowLongPtrW( hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams );
    }
        return DefWindowProcW( hwnd, msg, wparam, lparam );

    case WM_SIZE:
        if( win && wparam != SIZE_MINIMIZED )
        {
            win->width = (wp_u32)LOWORD( lparam );
            win->height = (wp_u32)HIWORD( lparam );
        }
        break;

    case WM_CLOSE:
        if( win )
        {
            win->closed = 1;
            win->hwnd = NULL;  /* prevent pump from dispatching after destroy */
        }
        DestroyWindow( hwnd );
        return 0;

    case WM_DESTROY:
        /* Closure belongs to this window. Posting WM_QUIT here also closes the
         * next editor instance created on the same thread after teardown. */
        if( win )
        {
            win->closed = 1;
            win->hwnd = NULL;
        }
        return 0;

    default:
        break;
    }

    if( win && win->event_callback && win->event_callback( win->event_userdata, hwnd, msg, wparam, lparam ) )
        return 0;

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

/* =========================================================================
 * Window class registration
 * ====================================================================== */

static wp_s32 wp_win32_register_class( void )
{
    WNDCLASSW wc;

    if( s_class_registered )
        return 1;

    memset( &wc, 0, sizeof( wc ) );
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = wp_win32_wnd_proc;
    wc.hInstance = GetModuleHandleW( NULL );
    wc.hIcon = LoadIconW( NULL, (LPCWSTR)IDI_APPLICATION );
    wc.hCursor = LoadCursorW( NULL, (LPCWSTR)IDC_ARROW );
    wc.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
    wc.lpszClassName = WP_WIN32_CLASS_NAME;
    if( !RegisterClassW( &wc ) )
        return 0;

    s_class_registered = 1;
    return 1;
}

/* The window procedure may live in an unloadable renderer DLL. Do not leave
 * the process-wide class referring to it after its last window. */
static void wp_win32_unregister_unused_class( void )
{
    if( s_window_count == 0 && s_class_registered &&
        UnregisterClassW( WP_WIN32_CLASS_NAME, GetModuleHandleW( NULL ) ) )
        s_class_registered = 0;
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_platform_window_win32 *wp_platform_window_win32_create( const wp_c8 *title, wp_u32 width,
                                                           wp_u32 height )
{
    wp_platform_window_win32 *win;
    WCHAR wtitle[WP_PLATFORM_WINDOW_MAX_TITLE];
    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exstyle = WS_EX_APPWINDOW;
    RECT rect;
    HWND hwnd;

    if( !wp_win32_register_class() )
        return NULL;

    win = (wp_platform_window_win32 *)malloc( sizeof( wp_platform_window_win32 ) );
    if( !win )
    {
        wp_win32_unregister_unused_class();
        return NULL;
    }

    memset( win, 0, sizeof( wp_platform_window_win32 ) );
    win->width = width;
    win->height = height;
    win->closed = 0;

    if( title && title[0] )
    {
        strncpy( win->title, title, WP_PLATFORM_WINDOW_MAX_TITLE - 1 );
        win->title[WP_PLATFORM_WINDOW_MAX_TITLE - 1] = '\0';
        MultiByteToWideChar( CP_UTF8, 0, title, -1, wtitle, WP_PLATFORM_WINDOW_MAX_TITLE );
    }
    else
    {
        win->title[0] = '\0';
        wtitle[0] = L'\0';
    }
    wtitle[WP_PLATFORM_WINDOW_MAX_TITLE - 1] = L'\0';

    rect.left = 0;
    rect.top = 0;
    rect.right = (LONG)width;
    rect.bottom = (LONG)height;
    AdjustWindowRectEx( &rect, style, FALSE, exstyle );

    hwnd =
        CreateWindowExW( exstyle, WP_WIN32_CLASS_NAME, wtitle, style | WS_VISIBLE, CW_USEDEFAULT,
                         CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL,
                         GetModuleHandleW( NULL ), win /* stored in GWLP_USERDATA via WM_NCCREATE */ );

    if( !hwnd )
    {
        free( win );
        wp_win32_unregister_unused_class();
        return NULL;
    }

    win->hwnd = hwnd;
    ++s_window_count;
    UpdateWindow( hwnd );

    return win;
}

void wp_platform_window_win32_destroy( wp_platform_window_win32 *win )
{
    if( !win )
        return;

    if( win->hwnd && IsWindow( win->hwnd ) )
    {
        DestroyWindow( win->hwnd );
        win->hwnd = NULL;
    }

    free( win );

    if( s_window_count > 0 )
        --s_window_count;
    wp_win32_unregister_unused_class();
}

/* =========================================================================
 * Message pump
 * ====================================================================== */

void wp_platform_window_win32_pump_messages( wp_platform_window_win32 *win )
{
    MSG msg;

    if( !win || !win->hwnd )
        return;

    while( PeekMessageW( &msg, NULL, 0, 0, PM_REMOVE ) )
    {
        if( msg.message == WM_QUIT )
        {
            win->closed = 1;
            break;
        }
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }
}

void wp_platform_window_win32_set_event_callback(
    wp_platform_window_win32 *win, wp_platform_window_win32_event_callback callback, void *userdata )
{
    if( !win )
        return;

    win->event_callback = callback;
    win->event_userdata = userdata;
}

/* =========================================================================
 * State queries
 * ====================================================================== */

wp_s32 wp_platform_window_win32_is_closed( const wp_platform_window_win32 *win )
{
    return win ? win->closed : 1;
}

void *wp_platform_window_win32_get_hwnd( const wp_platform_window_win32 *win )
{
    return win ? (void *)win->hwnd : NULL;
}

wp_u32 wp_platform_window_win32_get_width( const wp_platform_window_win32 *win )
{
    return win ? win->width : 0;
}

wp_u32 wp_platform_window_win32_get_height( const wp_platform_window_win32 *win )
{
    return win ? win->height : 0;
}

const wp_c8 *wp_platform_window_win32_get_title( const wp_platform_window_win32 *win )
{
    return win ? win->title : "";
}

/* =========================================================================
 * Mutators
 * ====================================================================== */

void wp_platform_window_win32_set_title( wp_platform_window_win32 *win, const wp_c8 *title )
{
    WCHAR wtitle[WP_PLATFORM_WINDOW_MAX_TITLE];

    if( !win || !title )
        return;

    strncpy( win->title, title, WP_PLATFORM_WINDOW_MAX_TITLE - 1 );
    win->title[WP_PLATFORM_WINDOW_MAX_TITLE - 1] = '\0';

    if( !win->hwnd || !IsWindow( win->hwnd ) )
        return;

    MultiByteToWideChar( CP_UTF8, 0, title, -1, wtitle, WP_PLATFORM_WINDOW_MAX_TITLE );
    wtitle[WP_PLATFORM_WINDOW_MAX_TITLE - 1] = L'\0';
    SetWindowTextW( win->hwnd, wtitle );
}

void wp_platform_window_win32_resize( wp_platform_window_win32 *win, wp_u32 width, wp_u32 height )
{
    DWORD style, exstyle;
    RECT rect;

    if( !win || !win->hwnd || !IsWindow( win->hwnd ) )
        return;

    win->width = width;
    win->height = height;

    style = (DWORD)GetWindowLongW( win->hwnd, GWL_STYLE );
    exstyle = (DWORD)GetWindowLongW( win->hwnd, GWL_EXSTYLE );
    rect.left = 0;
    rect.top = 0;
    rect.right = (LONG)width;
    rect.bottom = (LONG)height;
    AdjustWindowRectEx( &rect, style, FALSE, exstyle );

    SetWindowPos( win->hwnd, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
                  SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
}

/* =========================================================================
 * Show / hide / maximize / minimize / restore
 * ====================================================================== */

void wp_platform_window_win32_show( wp_platform_window_win32 *win )
{
    if( win && win->hwnd )
        ShowWindow( win->hwnd, SW_SHOW );
}

void wp_platform_window_win32_hide( wp_platform_window_win32 *win )
{
    if( win && win->hwnd )
        ShowWindow( win->hwnd, SW_HIDE );
}

void wp_platform_window_win32_maximize( wp_platform_window_win32 *win )
{
    if( win && win->hwnd )
        ShowWindow( win->hwnd, SW_MAXIMIZE );
}

void wp_platform_window_win32_minimize( wp_platform_window_win32 *win )
{
    if( win && win->hwnd )
        ShowWindow( win->hwnd, SW_MINIMIZE );
}

void wp_platform_window_win32_restore( wp_platform_window_win32 *win )
{
    if( win && win->hwnd )
        ShowWindow( win->hwnd, SW_RESTORE );
}

#endif /* _WIN32 */
