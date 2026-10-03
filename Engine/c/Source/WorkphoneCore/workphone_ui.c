#include "workphone_ui.h"
#include <string.h>
#include <stdio.h>

// UI context
struct wp_ui_context
{
    wp_s32 active_window;
    wp_s32 layout_row_height;
    wp_s32 layout_row_columns;
    wp_s32 layout_row_index;
    wp_s32 layout_row_widths[8];
};

// Window state
struct wp_ui_window
{
    wp_s32 id;
    wp_s32 x, y, w, h;
    wp_s32 flags;
    wp_s32 open;
};

// Internal static context
static struct wp_ui_context g_ui_ctx;
static struct wp_ui_window g_ui_win;

void wp_ui_init()
{
    memset( &g_ui_ctx, 0, sizeof( g_ui_ctx ) );
    memset( &g_ui_win, 0, sizeof( g_ui_win ) );
}

wp_s32 wp_ui_begin( const wp_c8 *title, wp_s32 x, wp_s32 y, wp_s32 w, wp_s32 h, wp_s32 flags )
{
    g_ui_win.id = 1;  // simplistic
    g_ui_win.x = x;
    g_ui_win.y = y;
    g_ui_win.w = w;
    g_ui_win.h = h;
    g_ui_win.flags = flags;
    g_ui_win.open = 1;
    g_ui_ctx.active_window = 1;
    return 1;
}

void wp_ui_end()
{
    g_ui_ctx.active_window = 0;
}

void wp_ui_layout_row_static( wp_s32 height, wp_s32 width, wp_s32 columns )
{
    g_ui_ctx.layout_row_height = height;
    g_ui_ctx.layout_row_columns = columns;
    for( wp_s32 i = 0; i < columns; ++i )
        g_ui_ctx.layout_row_widths[i] = width;
    g_ui_ctx.layout_row_index = 0;
}

void wp_ui_layout_row_dynamic( wp_s32 height, wp_s32 columns )
{
    g_ui_ctx.layout_row_height = height;
    g_ui_ctx.layout_row_columns = columns;
    wp_s32 col_width = g_ui_win.w / columns;
    for( wp_s32 i = 0; i < columns; ++i )
        g_ui_ctx.layout_row_widths[i] = col_width;
    g_ui_ctx.layout_row_index = 0;
}

void wp_ui_layout_row_begin( wp_s32 height, wp_s32 columns )
{
    g_ui_ctx.layout_row_height = height;
    g_ui_ctx.layout_row_columns = columns;
    g_ui_ctx.layout_row_index = 0;
}

void wp_ui_layout_row_push( wp_s32 width )
{
    if( g_ui_ctx.layout_row_index < 8 )
        g_ui_ctx.layout_row_widths[g_ui_ctx.layout_row_index++] = width;
}

void wp_ui_layout_row_end()
{
    g_ui_ctx.layout_row_index = 0;
}

wp_s32 wp_ui_button_label( const wp_c8 *label )
{
    // Stub: always returns 0 (not pressed)
    printf( "[Button] %s\n", label );
    return 0;
}

void wp_ui_label( const wp_c8 *label )
{
    printf( "[Label] %s\n", label );
}

wp_s32 wp_ui_option_label( const wp_c8 *label, wp_s32 active )
{
    printf( "[Option] %s (%s)\n", label, active ? "active" : "inactive" );
    return active;
}

wp_f32 wp_ui_slider_wp_f32( wp_f32 min, wp_f32 *value, wp_f32 max, wp_f32 step )
{
    printf( "[Slider] %f <= %f <= %f (step %f)\n", min, *value, max, step );
    // Stub: does not change value
    return *value;
}
