/**
 * @file workphone_graphics_window.h
 * @brief C API for a render window.
 *
 * Mirrors the C++ IGraphicsWindow / Window.  A render window is a platform-native
 * window that acts as a render target.  It carries a title, dimensions,
 * position, flag-based state (fullscreen, visible, closed, etc.) and an
 * opaque native handle.
 */

#ifndef WORKPHONE_GRAPHICS_WINDOW_H
#define WORKPHONE_GRAPHICS_WINDOW_H

#include <stdint.h>
#include "workphone_graphics_node.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

typedef struct wp_graphics_window wp_graphics_window;
typedef struct wp_graphics_scene wp_graphics_scene;

/* -------------------------------------------------------------------------
 * Window flag bits
 * ---------------------------------------------------------------------- */

enum
{
    WP_WINDOW_FLAG_FULLSCREEN = ( 1u << 0 ),
    WP_WINDOW_FLAG_VISIBLE = ( 1u << 1 ),
    WP_WINDOW_FLAG_BORDERLESS = ( 1u << 2 ),
    WP_WINDOW_FLAG_RESIZABLE = ( 1u << 3 ),
    WP_WINDOW_FLAG_HIDDEN = ( 1u << 4 ),
    WP_WINDOW_FLAG_MAXIMIZED = ( 1u << 5 ),
    WP_WINDOW_FLAG_MINIMIZED = ( 1u << 6 ),
    WP_WINDOW_FLAG_NO_TASKBAR = ( 1u << 7 ),
    WP_WINDOW_FLAG_ALWAYS_ON_TOP = ( 1u << 8 ),
    WP_WINDOW_FLAG_DEACTIVATE_ON_FOCUS_CHANGE = ( 1u << 9 ),
    WP_WINDOW_FLAG_IS_PRIMARY = ( 1u << 10 ),
    WP_WINDOW_FLAG_IS_CLOSED = ( 1u << 11 )
};

/* -------------------------------------------------------------------------
 * Constants
 * ---------------------------------------------------------------------- */

#ifndef WP_WINDOW_MAX_TITLE
#    define WP_WINDOW_MAX_TITLE 256
#endif

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

/**
 * @brief Creates a new render window with the given dimensions.
 * @param width  Initial width in pixels.
 * @param height Initial height in pixels.
 * @return Pointer to the created window, or NULL on failure.
 */
wp_graphics_window *wp_graphics_window_create( wp_u32 width, wp_u32 height );

/**
 * @brief Destroys a render window and releases its resources.
 * @param win Pointer to the window to destroy. Ignored if NULL.
 */
void wp_graphics_window_destroy( wp_graphics_window *win );

/* =========================================================================
 * Title
 * ====================================================================== */

const wp_c8 *wp_graphics_window_get_title( const wp_graphics_window *win );
void wp_graphics_window_set_title( wp_graphics_window *win, const wp_c8 *title );

/* =========================================================================
 * Size
 * ====================================================================== */

wp_u32 wp_graphics_window_get_width( const wp_graphics_window *win );
wp_u32 wp_graphics_window_get_height( const wp_graphics_window *win );
void wp_graphics_window_set_size( wp_graphics_window *win, wp_u32 width, wp_u32 height );
void wp_graphics_window_resize( wp_graphics_window *win, wp_u32 width, wp_u32 height );

/* =========================================================================
 * Position
 * ====================================================================== */

wp_s32 wp_graphics_window_get_x( const wp_graphics_window *win );
wp_s32 wp_graphics_window_get_y( const wp_graphics_window *win );
void wp_graphics_window_set_position( wp_graphics_window *win, wp_s32 x, wp_s32 y );
void wp_graphics_window_reposition( wp_graphics_window *win, wp_s32 x, wp_s32 y );

/* =========================================================================
 * Flags
 * ====================================================================== */

wp_u32 wp_graphics_window_get_flags( const wp_graphics_window *win );
void wp_graphics_window_set_flags( wp_graphics_window *win, wp_u32 flags );
wp_s32 wp_graphics_window_get_flag( const wp_graphics_window *win, wp_u32 flag );
void wp_graphics_window_set_flag( wp_graphics_window *win, wp_u32 flag, wp_s32 value );

/* =========================================================================
 * State queries
 * ====================================================================== */

wp_s32 wp_graphics_window_is_fullscreen( const wp_graphics_window *win );
void wp_graphics_window_set_fullscreen( wp_graphics_window *win, wp_s32 fullscreen );
void wp_graphics_window_set_fullscreen_with_size( wp_graphics_window *win, wp_s32 fullscreen,
                                                  wp_u32 width, wp_u32 height );

wp_s32 wp_graphics_window_is_visible( const wp_graphics_window *win );
void wp_graphics_window_set_visible( wp_graphics_window *win, wp_s32 visible );

wp_s32 wp_graphics_window_is_closed( const wp_graphics_window *win );
wp_s32 wp_graphics_window_is_primary( const wp_graphics_window *win );

wp_s32 wp_graphics_window_is_deactivated_on_focus_change( const wp_graphics_window *win );
void wp_graphics_window_set_deactivate_on_focus_change( wp_graphics_window *win, wp_s32 deactivate );

/* =========================================================================
 * Maximize / swap / moved-or-resized notification
 * ====================================================================== */

void wp_graphics_window_maximize( wp_graphics_window *win );
void wp_graphics_window_swap_buffers( wp_graphics_window *win );
void wp_graphics_window_moved_or_resized( wp_graphics_window *win );

/* =========================================================================
 * Colour depth
 * ====================================================================== */

wp_u32 wp_graphics_window_get_colour_depth( const wp_graphics_window *win );
void wp_graphics_window_set_colour_depth( wp_graphics_window *win, wp_u32 depth );

/* =========================================================================
 * Active / auto-updated / priority
 * ====================================================================== */

wp_s32 wp_graphics_window_is_active( const wp_graphics_window *win );
void wp_graphics_window_set_active( wp_graphics_window *win, wp_s32 active );

wp_s32 wp_graphics_window_is_auto_updated( const wp_graphics_window *win );
void wp_graphics_window_set_auto_updated( wp_graphics_window *win, wp_s32 autoupdate );

wp_u8 wp_graphics_window_get_priority( const wp_graphics_window *win );
void wp_graphics_window_set_priority( wp_graphics_window *win, wp_u8 priority );

/* =========================================================================
 * Window / device handle
 * ====================================================================== */

void *wp_graphics_window_get_window_handle( const wp_graphics_window *win );
void wp_graphics_window_set_window_handle( wp_graphics_window *win, void *handle );

void *wp_graphics_window_get_device_handle( const wp_graphics_window *win );
void wp_graphics_window_set_device_handle( wp_graphics_window *win, void *handle );

/* =========================================================================
 * Native access
 * ====================================================================== */

void wp_graphics_window_get_native( const wp_graphics_window *win, void **pp_object );
void wp_graphics_window_set_native( wp_graphics_window *win, void *native );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GRAPHICS_WINDOW_H */
