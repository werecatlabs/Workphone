#include "workphone_ui.h"
#include "workphone_context.h"
#include "workphone_group.h"
#include "workphone_list_view.h"
#include "workphone_table.h"
#include "workphone_util.h"

WORKPHONE_API wp_bool wp_list_view_begin( struct wp_context *ctx, struct wp_list_view *view,
                                          const wp_c8 *title, wp_flags flags, wp_s32 row_height,
                                          wp_s32 row_count )
{
    wp_s32 title_len;
    wp_hash title_hash;
    wp_u32 *x_offset;
    wp_u32 *y_offset;

    wp_s32 result;
    struct wp_window *win;
    struct wp_panel *layout;
    const struct wp_style *style;
    struct wp_vec2f item_spacing;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( view );
    WORKPHONE_ASSERT( title );
    if( !ctx || !view || !title )
        return 0;

    win = ctx->current;
    style = &ctx->style;
    item_spacing = style->window.spacing;
    row_height += WORKPHONE_MAX( 0, (wp_s32)item_spacing.y );

    /* find persistent list view scrollbar offset */
    title_len = (wp_s32)wp_strlen( title );
    title_hash = wp_murmur_hash( title, (wp_s32)title_len, WORKPHONE_PANEL_GROUP );
    x_offset = wp_find_value( win, title_hash );
    if( !x_offset )
    {
        x_offset = wp_add_value( ctx, win, title_hash, 0 );
        y_offset = wp_add_value( ctx, win, title_hash + 1, 0 );

        WORKPHONE_ASSERT( x_offset );
        WORKPHONE_ASSERT( y_offset );
        if( !x_offset || !y_offset )
            return 0;
        *x_offset = *y_offset = 0;
    }
    else
        y_offset = wp_find_value( win, title_hash + 1 );
    view->scroll_value = *y_offset;
    view->scroll_pointer = y_offset;

    *y_offset = 0;
    result = wp_group_scrolled_offset_begin( ctx, x_offset, y_offset, title, flags );
    win = ctx->current;
    layout = win->layout;

    view->total_height = row_height * WORKPHONE_MAX( row_count, 1 );
    view->begin = (wp_s32)WORKPHONE_MAX( ( (wp_f32)view->scroll_value / (wp_f32)row_height ), 0.0f );
    view->count = (wp_s32)WORKPHONE_MAX( wp_iceilf( ( layout->clip.h ) / (wp_f32)row_height ), 0 );
    view->count = WORKPHONE_MIN( view->count, row_count - view->begin );
    view->end = view->begin + view->count;
    view->ctx = ctx;
    return result;
}
WORKPHONE_API void wp_list_view_end( struct wp_list_view *view )
{
    struct wp_context *ctx;
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( view );
    WORKPHONE_ASSERT( view->ctx );
    WORKPHONE_ASSERT( view->scroll_pointer );
    if( !view || !view->ctx )
        return;

    ctx = view->ctx;
    win = ctx->current;
    layout = win->layout;
    layout->at_y = layout->bounds.y + (wp_f32)view->total_height;
    *view->scroll_pointer = *view->scroll_pointer + view->scroll_value;
    wp_group_end( view->ctx );
}
