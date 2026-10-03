#include "workphone_button.h"
#include "workphone_context.h"
#include "workphone_popup.h"
#include "workphone_widget.h"
#include "workphone_util.h"

/* Helper function defined first so it can be called by other functions */
WORKPHONE_API void wp_contextual_close( struct wp_context *ctx )
{
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;
    wp_popup_close( ctx );
}

WORKPHONE_API wp_bool wp_contextual_begin( struct wp_context *ctx, wp_flags flags, struct wp_vec2f size,
                                           struct wp_rect trigger_bounds )
{
    struct wp_window *win;
    struct wp_window *popup;
    struct wp_rect body;
    struct wp_input *in;

    WORKPHONE_STORAGE const struct wp_rect null_rect = { -1, -1, 0, 0 };
    wp_s32 is_clicked = 0;
    wp_s32 is_open = 0;
    wp_s32 ret = 0;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    ++win->popup.con_count;
    if( ctx->current != ctx->active )
        return 0;

    /* check if currently active contextual is active */
    popup = win->popup.win;
    is_open = ( popup && win->popup.type == WORKPHONE_PANEL_CONTEXTUAL );
    in = win->widgets_disabled ? 0 : &ctx->input;
    if( in )
    {
        is_clicked = wp_input_mouse_clicked( in, WORKPHONE_BUTTON_RIGHT, trigger_bounds );
        if( win->popup.active_con && win->popup.con_count != win->popup.active_con )
            return 0;
        if( !is_open && win->popup.active_con )
            win->popup.active_con = 0;
        if( ( !is_open && !is_clicked ) )
            return 0;

        /* calculate contextual position on click */
        win->popup.active_con = win->popup.con_count;
        if( is_clicked )
        {
            body.x = in->mouse.pos.x;
            body.y = in->mouse.pos.y;
        }
        else
        {
            body.x = popup->bounds.x;
            body.y = popup->bounds.y;
        }

        body.w = size.x;
        body.h = size.y;

        /* start nonblocking contextual popup */
        ret = wp_nonblock_begin( ctx, flags | WORKPHONE_WINDOW_NO_SCROLLBAR, body, null_rect,
                                 WORKPHONE_PANEL_CONTEXTUAL );
        if( ret )
            win->popup.type = WORKPHONE_PANEL_CONTEXTUAL;
        else
        {
            win->popup.active_con = 0;
            win->popup.type = WORKPHONE_PANEL_NONE;
            if( win->popup.win )
                win->popup.win->flags = 0;
        }
    }
    return ret;
}
WORKPHONE_API wp_bool wp_contextual_item_text( struct wp_context *ctx, const wp_c8 *text, wp_s32 len,
                                               wp_flags alignment )
{
    struct wp_window *win;
    const struct wp_input *in;
    const struct wp_style *style;

    struct wp_rect bounds;
    enum wp_widget_layout_states state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    style = &ctx->style;
    state = wp_widget_fitting( &bounds, ctx, style->contextual_button.padding );
    if( !state )
        return wp_false;

    in =
        ( state == WORKPHONE_WIDGET_ROM || win->layout->flags & WORKPHONE_WINDOW_ROM ) ? 0 : &ctx->input;
    if( wp_do_button_text( &ctx->last_widget_state, &win->buffer, bounds, text, len, alignment,
                           WORKPHONE_BUTTON_DEFAULT, &style->contextual_button, in, style->font ) )
    {
        wp_contextual_close( ctx );
        return wp_true;
    }
    return wp_false;
}
WORKPHONE_API wp_bool wp_contextual_item_label( struct wp_context *ctx, const wp_c8 *label,
                                                wp_flags align )
{
    return wp_contextual_item_text( ctx, label, wp_strlen( label ), align );
}
WORKPHONE_API wp_bool wp_contextual_item_image_text( struct wp_context *ctx, struct wp_image img,
                                                     const wp_c8 *text, wp_s32 len, wp_flags align )
{
    struct wp_window *win;
    const struct wp_input *in;
    const struct wp_style *style;

    struct wp_rect bounds;
    enum wp_widget_layout_states state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    style = &ctx->style;
    state = wp_widget_fitting( &bounds, ctx, style->contextual_button.padding );
    if( !state )
        return wp_false;

    in =
        ( state == WORKPHONE_WIDGET_ROM || win->layout->flags & WORKPHONE_WINDOW_ROM ) ? 0 : &ctx->input;
    if( wp_do_button_text_image( &ctx->last_widget_state, &win->buffer, bounds, img, text, len, align,
                                 WORKPHONE_BUTTON_DEFAULT, &style->contextual_button, style->font, in ) )
    {
        wp_contextual_close( ctx );
        return wp_true;
    }
    return wp_false;
}
WORKPHONE_API wp_bool wp_contextual_item_image_label( struct wp_context *ctx, struct wp_image img,
                                                      const wp_c8 *label, wp_flags align )
{
    return wp_contextual_item_image_text( ctx, img, label, wp_strlen( label ), align );
}
WORKPHONE_API wp_bool wp_contextual_item_symbol_text( struct wp_context *ctx, enum wp_symbol_type symbol,
                                                      const wp_c8 *text, wp_s32 len, wp_flags align )
{
    struct wp_window *win;
    const struct wp_input *in;
    const struct wp_style *style;

    struct wp_rect bounds;
    enum wp_widget_layout_states state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    style = &ctx->style;
    state = wp_widget_fitting( &bounds, ctx, style->contextual_button.padding );
    if( !state )
        return wp_false;

    in =
        ( state == WORKPHONE_WIDGET_ROM || win->layout->flags & WORKPHONE_WINDOW_ROM ) ? 0 : &ctx->input;
    if( wp_do_button_text_symbol( &ctx->last_widget_state, &win->buffer, bounds, symbol, text, len,
                                  align, WORKPHONE_BUTTON_DEFAULT, &style->contextual_button,
                                  style->font, in ) )
    {
        wp_contextual_close( ctx );
        return wp_true;
    }
    return wp_false;
}
WORKPHONE_API wp_bool wp_contextual_item_symbol_label( struct wp_context *ctx,
                                                       enum wp_symbol_type symbol, const wp_c8 *text,
                                                       wp_flags align )
{
    return wp_contextual_item_symbol_text( ctx, symbol, text, wp_strlen( text ), align );
}
WORKPHONE_API void wp_contextual_end( struct wp_context *ctx )
{
    struct wp_window *popup;
    struct wp_panel *panel;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return;

    popup = ctx->current;
    panel = popup->layout;
    WORKPHONE_ASSERT( popup->parent );
    WORKPHONE_ASSERT( (wp_s32)panel->type & (wp_s32)WORKPHONE_PANEL_SET_POPUP );
    if( panel->flags & WORKPHONE_WINDOW_DYNAMIC )
    {
        /* Close behavior
        This is a bit of a hack solution since we do not know before we end our popup
        how big it will be. We therefore do not directly know when a
        click outside the non-blocking popup must close it at that direct frame.
        Instead it will be closed in the next frame.*/
        struct wp_rect body = { 0, 0, 0, 0 };
        if( panel->at_y < ( panel->bounds.y + panel->bounds.h ) )
        {
            struct wp_vec2f padding = wp_panel_get_padding( &ctx->style, panel->type );
            body = panel->bounds;
            body.y =
                ( panel->at_y + panel->footer_height + panel->border + padding.y + panel->row.height );
            body.h = ( panel->bounds.y + panel->bounds.h ) - body.y;
        }
        {
            wp_s32 pressed = wp_input_is_mouse_pressed( &ctx->input, WORKPHONE_BUTTON_LEFT );
            wp_s32 in_body = wp_input_is_mouse_hovering_rect( &ctx->input, body );
            if( pressed && in_body )
                popup->flags |= WORKPHONE_WINDOW_HIDDEN;
        }
    }
    if( popup->flags & WORKPHONE_WINDOW_HIDDEN )
        popup->seq = 0;
    wp_popup_end( ctx );
    return;
}
