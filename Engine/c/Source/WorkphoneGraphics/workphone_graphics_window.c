/**
 * @file workphone_graphics_window.c
 * @brief Implementation of the C graphics window API.
 */

#include "workphone_graphics_window.h"
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal structure
 * ====================================================================== */

typedef struct wp_graphics_window
{
    wp_c8 title[WP_WINDOW_MAX_TITLE];

    wp_u32 width;
    wp_u32 height;

    wp_s32 pos_x;
    wp_s32 pos_y;

    wp_u32 flags;

    wp_u32 colour_depth;
    wp_u8 priority;
    wp_s32 active;
    wp_s32 auto_updated;

    void *window_handle;
    void *device_handle;
    void *native;
} wp_graphics_window;

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_graphics_window *wp_graphics_window_create( wp_u32 width, wp_u32 height )
{
    wp_graphics_window *win = (wp_graphics_window *)malloc( sizeof( wp_graphics_window ) );
    if( !win )
        return NULL;

    memset( win, 0, sizeof( *win ) );

    win->title[0] = '\0';
    win->width = width;
    win->height = height;
    win->pos_x = 0;
    win->pos_y = 0;
    win->flags = WP_WINDOW_FLAG_VISIBLE | WP_WINDOW_FLAG_RESIZABLE;
    win->colour_depth = 32;
    win->priority = 0;
    win->active = 1;
    win->auto_updated = 1;

    return win;
}

void wp_graphics_window_destroy( wp_graphics_window *win )
{
    if( !win )
        return;

    free( win );
}

/* =========================================================================
 * Title
 * ====================================================================== */

const wp_c8 *wp_graphics_window_get_title( const wp_graphics_window *win )
{
    if( !win )
        return "";

    return win->title;
}

void wp_graphics_window_set_title( wp_graphics_window *win, const wp_c8 *title )
{
    if( !win )
        return;

    if( title )
    {
        strncpy( win->title, title, WP_WINDOW_MAX_TITLE - 1 );
        win->title[WP_WINDOW_MAX_TITLE - 1] = '\0';
    }
    else
    {
        win->title[0] = '\0';
    }
}

/* =========================================================================
 * Size
 * ====================================================================== */

wp_u32 wp_graphics_window_get_width( const wp_graphics_window *win )
{
    if( !win )
        return 0;

    return win->width;
}

wp_u32 wp_graphics_window_get_height( const wp_graphics_window *win )
{
    if( !win )
        return 0;

    return win->height;
}

void wp_graphics_window_set_size( wp_graphics_window *win, wp_u32 width, wp_u32 height )
{
    if( !win )
        return;

    win->width = width;
    win->height = height;
}

void wp_graphics_window_resize( wp_graphics_window *win, wp_u32 width, wp_u32 height )
{
    wp_graphics_window_set_size( win, width, height );
}

/* =========================================================================
 * Position
 * ====================================================================== */

wp_s32 wp_graphics_window_get_x( const wp_graphics_window *win )
{
    if( !win )
        return 0;

    return win->pos_x;
}

wp_s32 wp_graphics_window_get_y( const wp_graphics_window *win )
{
    if( !win )
        return 0;

    return win->pos_y;
}

void wp_graphics_window_set_position( wp_graphics_window *win, wp_s32 x, wp_s32 y )
{
    if( !win )
        return;

    win->pos_x = x;
    win->pos_y = y;
}

void wp_graphics_window_reposition( wp_graphics_window *win, wp_s32 x, wp_s32 y )
{
    wp_graphics_window_set_position( win, x, y );
}

/* =========================================================================
 * Flags
 * ====================================================================== */

wp_u32 wp_graphics_window_get_flags( const wp_graphics_window *win )
{
    if( !win )
        return 0;

    return win->flags;
}

void wp_graphics_window_set_flags( wp_graphics_window *win, wp_u32 flags )
{
    if( !win )
        return;

    win->flags = flags;
}

wp_s32 wp_graphics_window_get_flag( const wp_graphics_window *win, wp_u32 flag )
{
    if( !win )
        return 0;

    return ( win->flags & flag ) != 0;
}

void wp_graphics_window_set_flag( wp_graphics_window *win, wp_u32 flag, wp_s32 value )
{
    if( !win )
        return;

    if( value )
        win->flags |= flag;
    else
        win->flags &= ~flag;
}

/* =========================================================================
 * State queries
 * ====================================================================== */

wp_s32 wp_graphics_window_is_fullscreen( const wp_graphics_window *win )
{
    return wp_graphics_window_get_flag( win, WP_WINDOW_FLAG_FULLSCREEN );
}

void wp_graphics_window_set_fullscreen( wp_graphics_window *win, wp_s32 fullscreen )
{
    wp_graphics_window_set_flag( win, WP_WINDOW_FLAG_FULLSCREEN, fullscreen );
}

void wp_graphics_window_set_fullscreen_with_size( wp_graphics_window *win, wp_s32 fullscreen,
                                                  wp_u32 width, wp_u32 height )
{
    if( !win )
        return;

    wp_graphics_window_set_fullscreen( win, fullscreen );
    wp_graphics_window_set_size( win, width, height );
}

wp_s32 wp_graphics_window_is_visible( const wp_graphics_window *win )
{
    return wp_graphics_window_get_flag( win, WP_WINDOW_FLAG_VISIBLE );
}

void wp_graphics_window_set_visible( wp_graphics_window *win, wp_s32 visible )
{
    wp_graphics_window_set_flag( win, WP_WINDOW_FLAG_VISIBLE, visible );
}

wp_s32 wp_graphics_window_is_closed( const wp_graphics_window *win )
{
    return wp_graphics_window_get_flag( win, WP_WINDOW_FLAG_IS_CLOSED );
}

wp_s32 wp_graphics_window_is_primary( const wp_graphics_window *win )
{
    return wp_graphics_window_get_flag( win, WP_WINDOW_FLAG_IS_PRIMARY );
}

wp_s32 wp_graphics_window_is_deactivated_on_focus_change( const wp_graphics_window *win )
{
    return wp_graphics_window_get_flag( win, WP_WINDOW_FLAG_DEACTIVATE_ON_FOCUS_CHANGE );
}

void wp_graphics_window_set_deactivate_on_focus_change( wp_graphics_window *win, wp_s32 deactivate )
{
    wp_graphics_window_set_flag( win, WP_WINDOW_FLAG_DEACTIVATE_ON_FOCUS_CHANGE, deactivate );
}

/* =========================================================================
 * Maximize / swap / moved-or-resized
 * ====================================================================== */

void wp_graphics_window_maximize( wp_graphics_window *win )
{
    if( !win )
        return;

    wp_graphics_window_set_flag( win, WP_WINDOW_FLAG_MAXIMIZED, 1 );
    wp_graphics_window_set_flag( win, WP_WINDOW_FLAG_MINIMIZED, 0 );
}

void wp_graphics_window_swap_buffers( wp_graphics_window *win )
{
    (void)win;
}

void wp_graphics_window_moved_or_resized( wp_graphics_window *win )
{
    (void)win;
}

/* =========================================================================
 * Colour depth
 * ====================================================================== */

wp_u32 wp_graphics_window_get_colour_depth( const wp_graphics_window *win )
{
    if( !win )
        return 0;

    return win->colour_depth;
}

void wp_graphics_window_set_colour_depth( wp_graphics_window *win, wp_u32 depth )
{
    if( !win )
        return;

    win->colour_depth = depth;
}

/* =========================================================================
 * Active / auto-updated / priority
 * ====================================================================== */

wp_s32 wp_graphics_window_is_active( const wp_graphics_window *win )
{
    if( !win )
        return 0;

    return win->active;
}

void wp_graphics_window_set_active( wp_graphics_window *win, wp_s32 active )
{
    if( !win )
        return;

    win->active = active;
}

wp_s32 wp_graphics_window_is_auto_updated( const wp_graphics_window *win )
{
    if( !win )
        return 0;

    return win->auto_updated;
}

void wp_graphics_window_set_auto_updated( wp_graphics_window *win, wp_s32 autoupdate )
{
    if( !win )
        return;

    win->auto_updated = autoupdate;
}

wp_u8 wp_graphics_window_get_priority( const wp_graphics_window *win )
{
    if( !win )
        return 0;

    return win->priority;
}

void wp_graphics_window_set_priority( wp_graphics_window *win, wp_u8 priority )
{
    if( !win )
        return;

    win->priority = priority;
}

/* =========================================================================
 * Window / device handle
 * ====================================================================== */

void *wp_graphics_window_get_window_handle( const wp_graphics_window *win )
{
    if( !win )
        return NULL;

    return win->window_handle;
}

void wp_graphics_window_set_window_handle( wp_graphics_window *win, void *handle )
{
    if( !win )
        return;

    win->window_handle = handle;
}

void *wp_graphics_window_get_device_handle( const wp_graphics_window *win )
{
    if( !win )
        return NULL;

    return win->device_handle;
}

void wp_graphics_window_set_device_handle( wp_graphics_window *win, void *handle )
{
    if( !win )
        return;

    win->device_handle = handle;
}

/* =========================================================================
 * Native access
 * ====================================================================== */

void wp_graphics_window_get_native( const wp_graphics_window *win, void **pp_object )
{
    if( !pp_object )
        return;

    *pp_object = win ? win->native : NULL;
}

void wp_graphics_window_set_native( wp_graphics_window *win, void *native )
{
    if( !win )
        return;

    win->native = native;
}
