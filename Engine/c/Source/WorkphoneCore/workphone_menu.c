#include "workphone.h"
#include "workphone_button.h"
#include "workphone_popup.h"
#include "workphone_util.h"
#include "workphone_widget.h"

void wp_menubar_begin( struct wp_context *ctx )
{
    struct wp_panel *layout;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    layout = ctx->current->layout;
    WORKPHONE_ASSERT( layout->at_y == layout->bounds.y );
    /* if this assert triggers you allocated space between wp_begin and wp_menubar_begin.
    If you want a menubar the first nuklear function after `wp_begin` has to be a
    `wp_menubar_begin` call. Inside the menubar you then have to allocate space for
    widgets (also supports multiple rows).
    Example:
        if (wp_begin(...)) {
            wp_menubar_begin(...);
                wp_layout_xxxx(...);
                wp_button(...);
                wp_layout_xxxx(...);
                wp_button(...);
            wp_menubar_end(...);
        }
        wp_end(...);
    */
    if( layout->flags & WORKPHONE_WINDOW_HIDDEN || layout->flags & WORKPHONE_WINDOW_MINIMIZED )
        return;

    layout->menu.x = layout->at_x;
    layout->menu.y = layout->at_y + layout->row.height;
    layout->menu.w = layout->bounds.w;
    layout->menu.offset.x = *layout->offset_x;
    layout->menu.offset.y = *layout->offset_y;
    *layout->offset_y = 0;
}
WORKPHONE_API void wp_menubar_end( struct wp_context *ctx )
{
    struct wp_window *win;
    struct wp_panel *layout;
    struct wp_command_buffer *out;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    out = &win->buffer;
    layout = win->layout;
    if( layout->flags & WORKPHONE_WINDOW_HIDDEN || layout->flags & WORKPHONE_WINDOW_MINIMIZED )
        return;

    layout->menu.h = layout->at_y - layout->menu.y;
    layout->menu.h += layout->row.height + ctx->style.window.spacing.y;

    layout->bounds.y += layout->menu.h;
    layout->bounds.h -= layout->menu.h;

    *layout->offset_x = layout->menu.offset.x;
    *layout->offset_y = layout->menu.offset.y;
    layout->at_y = layout->bounds.y - layout->row.height;

    layout->clip.y = layout->bounds.y;
    layout->clip.h = layout->bounds.h;
    wp_push_scissor( out, layout->clip );
}
WORKPHONE_INTERN wp_s32 wp_menu_begin( struct wp_context *ctx, struct wp_window *win, const wp_c8 *id,
                                       wp_s32 is_clicked, struct wp_rect header, struct wp_vec2f size )
{
    wp_s32 is_open = 0;
    wp_s32 is_active = 0;
    struct wp_rect body;
    struct wp_window *popup;
    wp_hash hash = wp_murmur_hash( id, (wp_s32)wp_strlen( id ), WORKPHONE_PANEL_MENU );

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    body.x = header.x;
    body.w = size.x;
    body.y = header.y + header.h;
    body.h = size.y;

    popup = win->popup.win;
    is_open = popup ? wp_true : wp_false;
    is_active = ( popup && ( win->popup.name == hash ) && win->popup.type == WORKPHONE_PANEL_MENU );
    if( ( is_clicked && is_open && !is_active ) || ( is_open && !is_active ) ||
        ( !is_open && !is_active && !is_clicked ) )
        return 0;
    if( !wp_nonblock_begin( ctx, WORKPHONE_WINDOW_NO_SCROLLBAR, body, header, WORKPHONE_PANEL_MENU ) )
        return 0;

    win->popup.type = WORKPHONE_PANEL_MENU;
    win->popup.name = hash;
    return 1;
}
WORKPHONE_API wp_bool wp_menu_begin_text( struct wp_context *ctx, const wp_c8 *title, wp_s32 len,
                                          wp_flags align, struct wp_vec2f size )
{
    struct wp_window *win;
    const struct wp_input *in;
    struct wp_rect header;
    wp_s32 is_clicked = wp_false;
    wp_flags state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    state = wp_widget( &header, ctx );
    if( !state )
        return 0;
    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           win->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;
    if( wp_do_button_text( &ctx->last_widget_state, &win->buffer, header, title, len, align,
                           WORKPHONE_BUTTON_DEFAULT, &ctx->style.menu_button, in, ctx->style.font ) )
        is_clicked = wp_true;
    return wp_menu_begin( ctx, win, title, is_clicked, header, size );
}
WORKPHONE_API wp_bool wp_menu_begin_label( struct wp_context *ctx, const wp_c8 *text, wp_flags align,
                                           struct wp_vec2f size )
{
    return wp_menu_begin_text( ctx, text, wp_strlen( text ), align, size );
}
WORKPHONE_API wp_bool wp_menu_begin_image( struct wp_context *ctx, const wp_c8 *id, struct wp_image img,
                                           struct wp_vec2f size )
{
    struct wp_window *win;
    struct wp_rect header;
    const struct wp_input *in;
    wp_s32 is_clicked = wp_false;
    wp_flags state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    state = wp_widget( &header, ctx );
    if( !state )
        return 0;
    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           win->layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;
    if( wp_do_button_image( &ctx->last_widget_state, &win->buffer, header, img, WORKPHONE_BUTTON_DEFAULT,
                            &ctx->style.menu_button, in ) )
        is_clicked = wp_true;
    return wp_menu_begin( ctx, win, id, is_clicked, header, size );
}
WORKPHONE_API wp_bool wp_menu_begin_symbol( struct wp_context *ctx, const wp_c8 *id,
                                            enum wp_symbol_type sym, struct wp_vec2f size )
{
    struct wp_window *win;
    const struct wp_input *in;
    struct wp_rect header;
    wp_s32 is_clicked = wp_false;
    wp_flags state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    state = wp_widget( &header, ctx );
    if( !state )
        return 0;
    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           win->layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;
    if( wp_do_button_symbol( &ctx->last_widget_state, &win->buffer, header, sym,
                             WORKPHONE_BUTTON_DEFAULT, &ctx->style.menu_button, in, ctx->style.font ) )
        is_clicked = wp_true;
    return wp_menu_begin( ctx, win, id, is_clicked, header, size );
}
WORKPHONE_API wp_bool wp_menu_begin_image_text( struct wp_context *ctx, const wp_c8 *title, wp_s32 len,
                                                wp_flags align, struct wp_image img,
                                                struct wp_vec2f size )
{
    struct wp_window *win;
    struct wp_rect header;
    const struct wp_input *in;
    wp_s32 is_clicked = wp_false;
    wp_flags state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    state = wp_widget( &header, ctx );
    if( !state )
        return 0;
    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           win->layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;
    if( wp_do_button_text_image( &ctx->last_widget_state, &win->buffer, header, img, title, len, align,
                                 WORKPHONE_BUTTON_DEFAULT, &ctx->style.menu_button, ctx->style.font,
                                 in ) )
        is_clicked = wp_true;
    return wp_menu_begin( ctx, win, title, is_clicked, header, size );
}
WORKPHONE_API wp_bool wp_menu_begin_image_label( struct wp_context *ctx, const wp_c8 *title,
                                                 wp_flags align, struct wp_image img,
                                                 struct wp_vec2f size )
{
    return wp_menu_begin_image_text( ctx, title, wp_strlen( title ), align, img, size );
}
WORKPHONE_API wp_bool wp_menu_begin_symbol_text( struct wp_context *ctx, const wp_c8 *title, wp_s32 len,
                                                 wp_flags align, enum wp_symbol_type sym,
                                                 struct wp_vec2f size )
{
    struct wp_window *win;
    struct wp_rect header;
    const struct wp_input *in;
    wp_s32 is_clicked = wp_false;
    wp_flags state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    state = wp_widget( &header, ctx );
    if( !state )
        return 0;

    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           win->layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;
    if( wp_do_button_text_symbol( &ctx->last_widget_state, &win->buffer, header, sym, title, len, align,
                                  WORKPHONE_BUTTON_DEFAULT, &ctx->style.menu_button, ctx->style.font,
                                  in ) )
        is_clicked = wp_true;
    return wp_menu_begin( ctx, win, title, is_clicked, header, size );
}
WORKPHONE_API wp_bool wp_menu_begin_symbol_label( struct wp_context *ctx, const wp_c8 *title,
                                                  wp_flags align, enum wp_symbol_type sym,
                                                  struct wp_vec2f size )
{
    return wp_menu_begin_symbol_text( ctx, title, wp_strlen( title ), align, sym, size );
}
WORKPHONE_API wp_bool wp_menu_item_text( struct wp_context *ctx, const wp_c8 *title, wp_s32 len,
                                         wp_flags align )
{
    return wp_contextual_item_text( ctx, title, len, align );
}
WORKPHONE_API wp_bool wp_menu_item_label( struct wp_context *ctx, const wp_c8 *label, wp_flags align )
{
    return wp_contextual_item_label( ctx, label, align );
}
WORKPHONE_API wp_bool wp_menu_item_image_label( struct wp_context *ctx, struct wp_image img,
                                                const wp_c8 *label, wp_flags align )
{
    return wp_contextual_item_image_label( ctx, img, label, align );
}
WORKPHONE_API wp_bool wp_menu_item_image_text( struct wp_context *ctx, struct wp_image img,
                                               const wp_c8 *text, wp_s32 len, wp_flags align )
{
    return wp_contextual_item_image_text( ctx, img, text, len, align );
}
WORKPHONE_API wp_bool wp_menu_item_symbol_text( struct wp_context *ctx, enum wp_symbol_type sym,
                                                const wp_c8 *text, wp_s32 len, wp_flags align )
{
    return wp_contextual_item_symbol_text( ctx, sym, text, len, align );
}
WORKPHONE_API wp_bool wp_menu_item_symbol_label( struct wp_context *ctx, enum wp_symbol_type sym,
                                                 const wp_c8 *label, wp_flags align )
{
    return wp_contextual_item_symbol_label( ctx, sym, label, align );
}
WORKPHONE_API void wp_menu_close( struct wp_context *ctx )
{
    wp_contextual_close( ctx );
}
WORKPHONE_API void wp_menu_end( struct wp_context *ctx )
{
    wp_contextual_end( ctx );
}
