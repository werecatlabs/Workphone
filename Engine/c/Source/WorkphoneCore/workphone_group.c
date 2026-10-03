#include "workphone_context.h"
#include "workphone_group.h"
#include "workphone_layout.h"
#include "workphone_panel.h"
#include "workphone_table.h"
#include "workphone_window.h"
#include "workphone_util.h"
#include "workphone_ui.h"

wp_bool wp_group_scrolled_offset_begin( struct wp_context *ctx, wp_u32 *x_offset, wp_u32 *y_offset,
                                        const wp_c8 *title, wp_flags flags )
{
    struct wp_rect bounds;
    struct wp_window panel;
    struct wp_window *win;

    win = ctx->current;
    wp_panel_alloc_space( &bounds, ctx );
    {
        const struct wp_rect *c = &win->layout->clip;
        if( !WORKPHONE_INTERSECT( c->x, c->y, c->w, c->h, bounds.x, bounds.y, bounds.w, bounds.h ) &&
            !( flags & WORKPHONE_WINDOW_MOVABLE ) )
        {
            return 0;
        }
    }
    if( win->flags & WORKPHONE_WINDOW_ROM )
        flags |= WORKPHONE_WINDOW_ROM;

    /* initialize a fake window to create the panel from */
    wp_zero( &panel, sizeof( panel ) );
    panel.bounds = bounds;
    panel.flags = flags;
    panel.scrollbar.x = *x_offset;
    panel.scrollbar.y = *y_offset;
    panel.buffer = win->buffer;
    panel.layout = (struct wp_panel *)wp_create_panel( ctx );
    ctx->current = &panel;
    wp_panel_begin( ctx, ( flags & WORKPHONE_WINDOW_TITLE ) ? title : 0, WORKPHONE_PANEL_GROUP );

    win->buffer = panel.buffer;
    win->buffer.clip = panel.layout->clip;
    panel.layout->offset_x = x_offset;
    panel.layout->offset_y = y_offset;
    panel.layout->parent = win->layout;
    win->layout = panel.layout;

    ctx->current = win;
    if( ( panel.layout->flags & WORKPHONE_WINDOW_CLOSED ) ||
        ( panel.layout->flags & WORKPHONE_WINDOW_MINIMIZED ) )
    {
        wp_flags f = panel.layout->flags;
        wp_group_scrolled_end( ctx );
        if( f & WORKPHONE_WINDOW_CLOSED )
            return WORKPHONE_WINDOW_CLOSED;
        if( f & WORKPHONE_WINDOW_MINIMIZED )
            return WORKPHONE_WINDOW_MINIMIZED;
    }
    return 1;
}
WORKPHONE_API void wp_group_scrolled_end( struct wp_context *ctx )
{
    struct wp_window *win;
    struct wp_panel *parent;
    struct wp_panel *g;

    struct wp_rect clip;
    struct wp_window pan;
    struct wp_vec2f panel_padding;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return;

    /* make sure wp_group_begin was called correctly */
    WORKPHONE_ASSERT( ctx->current );
    win = ctx->current;
    WORKPHONE_ASSERT( win->layout );
    g = win->layout;
    WORKPHONE_ASSERT( g->parent );
    parent = g->parent;

    /* dummy window */
    wp_zero_struct( pan );
    panel_padding = wp_panel_get_padding( &ctx->style, WORKPHONE_PANEL_GROUP );
    pan.bounds.y = g->bounds.y - ( g->header_height + g->menu.h );
    pan.bounds.x = g->bounds.x - panel_padding.x;
    pan.bounds.w = g->bounds.w + 2 * panel_padding.x;
    pan.bounds.h = g->bounds.h + g->header_height + g->menu.h;
    if( g->flags & WORKPHONE_WINDOW_BORDER )
    {
        pan.bounds.x -= g->border;
        pan.bounds.y -= g->border;
        pan.bounds.w += 2 * g->border;
        pan.bounds.h += 2 * g->border;
    }
    if( !( g->flags & WORKPHONE_WINDOW_NO_SCROLLBAR ) )
    {
        pan.bounds.w += ctx->style.window.scrollbar_size.x;
        pan.bounds.h += ctx->style.window.scrollbar_size.y;
    }
    pan.scrollbar.x = *g->offset_x;
    pan.scrollbar.y = *g->offset_y;
    pan.flags = g->flags;
    pan.buffer = win->buffer;
    pan.layout = g;
    pan.parent = win;
    ctx->current = &pan;

    /* make sure group has correct clipping rectangle */
    wp_unify( &clip, &parent->clip, pan.bounds.x, pan.bounds.y, pan.bounds.x + pan.bounds.w,
              pan.bounds.y + pan.bounds.h + panel_padding.x );
    wp_push_scissor( &pan.buffer, clip );
    wp_end( ctx );

    win->buffer = pan.buffer;
    wp_push_scissor( &win->buffer, parent->clip );
    ctx->current = win;
    win->layout = parent;
    g->bounds = pan.bounds;
    return;
}
WORKPHONE_API wp_bool wp_group_scrolled_begin( struct wp_context *ctx, struct wp_scroll *scroll,
                                               const wp_c8 *title, wp_flags flags )
{
    return wp_group_scrolled_offset_begin( ctx, &scroll->x, &scroll->y, title, flags );
}
WORKPHONE_API wp_bool wp_group_begin_titled( struct wp_context *ctx, const wp_c8 *id, const wp_c8 *title,
                                             wp_flags flags )
{
    wp_s32 id_len;
    wp_hash id_hash;
    struct wp_window *win;
    wp_u32 *x_offset;
    wp_u32 *y_offset;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( id );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout || !id )
        return 0;

    /* find persistent group scrollbar value */
    win = ctx->current;
    id_len = (wp_s32)wp_strlen( id );
    id_hash = wp_murmur_hash( id, (wp_s32)id_len, WORKPHONE_PANEL_GROUP );
    x_offset = wp_find_value( win, id_hash );
    if( !x_offset )
    {
        x_offset = wp_add_value( ctx, win, id_hash, 0 );
        y_offset = wp_add_value( ctx, win, id_hash + 1, 0 );

        WORKPHONE_ASSERT( x_offset );
        WORKPHONE_ASSERT( y_offset );
        if( !x_offset || !y_offset )
            return 0;
        *x_offset = *y_offset = 0;
    }
    else
        y_offset = wp_find_value( win, id_hash + 1 );
    return wp_group_scrolled_offset_begin( ctx, x_offset, y_offset, title, flags );
}
WORKPHONE_API wp_bool wp_group_begin( struct wp_context *ctx, const wp_c8 *title, wp_flags flags )
{
    return wp_group_begin_titled( ctx, title, title, flags );
}
WORKPHONE_API void wp_group_end( struct wp_context *ctx )
{
    wp_group_scrolled_end( ctx );
}
WORKPHONE_API void wp_group_get_scroll( struct wp_context *ctx, const wp_c8 *id, wp_u32 *x_offset,
                                        wp_u32 *y_offset )
{
    wp_s32 id_len;
    wp_hash id_hash;
    struct wp_window *win;
    wp_u32 *x_offset_ptr;
    wp_u32 *y_offset_ptr;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( id );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout || !id )
        return;

    /* find persistent group scrollbar value */
    win = ctx->current;
    id_len = (wp_s32)wp_strlen( id );
    id_hash = wp_murmur_hash( id, (wp_s32)id_len, WORKPHONE_PANEL_GROUP );
    x_offset_ptr = wp_find_value( win, id_hash );
    if( !x_offset_ptr )
    {
        x_offset_ptr = wp_add_value( ctx, win, id_hash, 0 );
        y_offset_ptr = wp_add_value( ctx, win, id_hash + 1, 0 );

        WORKPHONE_ASSERT( x_offset_ptr );
        WORKPHONE_ASSERT( y_offset_ptr );
        if( !x_offset_ptr || !y_offset_ptr )
            return;
        *x_offset_ptr = *y_offset_ptr = 0;
    }
    else
        y_offset_ptr = wp_find_value( win, id_hash + 1 );
    if( x_offset )
        *x_offset = *x_offset_ptr;
    if( y_offset )
        *y_offset = *y_offset_ptr;
}
WORKPHONE_API void wp_group_set_scroll( struct wp_context *ctx, const wp_c8 *id, wp_u32 x_offset,
                                        wp_u32 y_offset )
{
    wp_s32 id_len;
    wp_hash id_hash;
    struct wp_window *win;
    wp_u32 *x_offset_ptr;
    wp_u32 *y_offset_ptr;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( id );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout || !id )
        return;

    /* find persistent group scrollbar value */
    win = ctx->current;
    id_len = (wp_s32)wp_strlen( id );
    id_hash = wp_murmur_hash( id, (wp_s32)id_len, WORKPHONE_PANEL_GROUP );
    x_offset_ptr = wp_find_value( win, id_hash );
    if( !x_offset_ptr )
    {
        x_offset_ptr = wp_add_value( ctx, win, id_hash, 0 );
        y_offset_ptr = wp_add_value( ctx, win, id_hash + 1, 0 );

        WORKPHONE_ASSERT( x_offset_ptr );
        WORKPHONE_ASSERT( y_offset_ptr );
        if( !x_offset_ptr || !y_offset_ptr )
            return;
        *x_offset_ptr = *y_offset_ptr = 0;
    }
    else
        y_offset_ptr = wp_find_value( win, id_hash + 1 );
    *x_offset_ptr = x_offset;
    *y_offset_ptr = y_offset;
}
