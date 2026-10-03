
/**
 * @file workphone_platform_window_win32.h
 * @brief Win32 native window creation and message handling.
 *
 * Provides a lightweight wrapper around a Win32 HWND.  The platform window is
 * intentionally decoupled from the graphics-layer wp_graphics_window so that
 * the platform module has no dependency on WorkphoneGraphics.  Callers that
 * need to bind the two together should retrieve the HWND via
 * wp_platform_window_win32_get_hwnd() and pass it to
 * wp_graphics_window_set_window_handle().
 */

#ifndef WORKPHONE_PLATFORM_WINDOW_WIN32_H
#define WORKPHONE_PLATFORM_WINDOW_WIN32_H

#ifdef _WIN32

#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#    include "workphone_types.h"

#    ifdef __cplusplus
extern "C" {
#    endif

/* -------------------------------------------------------------------------
 * Constants
 * ---------------------------------------------------------------------- */

#    ifndef WP_PLATFORM_WINDOW_MAX_TITLE
#        define WP_PLATFORM_WINDOW_MAX_TITLE 256
#    endif

/* -------------------------------------------------------------------------
 * Opaque type
 * ---------------------------------------------------------------------- */

typedef struct wp_platform_window_win32 wp_platform_window_win32;

typedef wp_s32 ( *wp_platform_window_win32_event_callback )( void *userdata, HWND hwnd, UINT msg,
                                                              WPARAM wparam, LPARAM lparam );

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_platform_window_win32 *wp_platform_window_win32_create( const wp_c8 *title, wp_u32 width,
                                                           wp_u32 height );

void wp_platform_window_win32_destroy( wp_platform_window_win32 *win );

/* =========================================================================
 * Message pump
 * ====================================================================== */

void wp_platform_window_win32_pump_messages( wp_platform_window_win32 *win );
void wp_platform_window_win32_set_event_callback(
    wp_platform_window_win32 *win, wp_platform_window_win32_event_callback callback, void *userdata );

/* =========================================================================
 * State queries
 * ====================================================================== */

wp_s32 wp_platform_window_win32_is_closed( const wp_platform_window_win32 *win );
void *wp_platform_window_win32_get_hwnd( const wp_platform_window_win32 *win );
wp_u32 wp_platform_window_win32_get_width( const wp_platform_window_win32 *win );
wp_u32 wp_platform_window_win32_get_height( const wp_platform_window_win32 *win );
const wp_c8 *wp_platform_window_win32_get_title( const wp_platform_window_win32 *win );

/* =========================================================================
 * Mutators
 * ====================================================================== */

void wp_platform_window_win32_set_title( wp_platform_window_win32 *win, const wp_c8 *title );
void wp_platform_window_win32_resize( wp_platform_window_win32 *win, wp_u32 width, wp_u32 height );
void wp_platform_window_win32_show( wp_platform_window_win32 *win );
void wp_platform_window_win32_hide( wp_platform_window_win32 *win );
void wp_platform_window_win32_maximize( wp_platform_window_win32 *win );
void wp_platform_window_win32_minimize( wp_platform_window_win32 *win );
void wp_platform_window_win32_restore( wp_platform_window_win32 *win );

#    ifdef __cplusplus
}
#    endif

#endif /* _WIN32 */

#endif /* WORKPHONE_PLATFORM_WINDOW_WIN32_H */
