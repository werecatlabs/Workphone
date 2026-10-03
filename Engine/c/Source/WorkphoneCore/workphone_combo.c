#include "workphone_ui.h"
#include "workphone_button.h"
#include "workphone_context.h"
#include "workphone_layout.h"
#include "workphone_popup.h"
#include "workphone_widget.h"
#include "workphone_text.h"

WORKPHONE_INTERN wp_bool wp_combo_begin( struct wp_context *ctx, struct wp_window *win,
                                         struct wp_vec2f size, wp_bool is_clicked, struct wp_rect header )
{
    struct wp_window *popup;
    wp_s32 is_open = 0;
    wp_s32 is_active = 0;
    struct wp_rect body;
    wp_hash hash;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    popup = win->popup.win;
    body.x = header.x;
    body.w = size.x;
    body.y = header.y + header.h - ctx->style.window.combo_border;
    body.h = size.y;

    hash = win->popup.combo_count++;
    is_open = ( popup ) ? wp_true : wp_false;
    is_active = ( popup && ( win->popup.name == hash ) && win->popup.type == WORKPHONE_PANEL_COMBO );
    if( ( is_clicked && is_open && !is_active ) || ( is_open && !is_active ) ||
        ( !is_open && !is_active && !is_clicked ) )
        return 0;
    if( !wp_nonblock_begin( ctx, 0, body,
                            ( is_clicked && is_open ) ? wp_make_rect( 0, 0, 0, 0 ) : header,
                            WORKPHONE_PANEL_COMBO ) )
        return 0;

    win->popup.type = WORKPHONE_PANEL_COMBO;
    win->popup.name = hash;
    return 1;
}
WORKPHONE_API wp_bool wp_combo_begin_text( struct wp_context *ctx, const wp_c8 *selected, wp_s32 len,
                                           struct wp_vec2f size )
{
    const struct wp_input *in;
    struct wp_window *win;
    struct wp_style *style;

    enum wp_widget_layout_states s;
    wp_s32 is_clicked = wp_false;
    struct wp_rect header;
    const struct wp_style_item *background;
    struct wp_text text;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( selected );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout || !selected )
        return 0;

    win = ctx->current;
    style = &ctx->style;
    s = wp_widget( &header, ctx );
    if( s == WORKPHONE_WIDGET_INVALID )
        return 0;

    in = ( win->layout->flags & WORKPHONE_WINDOW_ROM || s == WORKPHONE_WIDGET_DISABLED ||
           s == WORKPHONE_WIDGET_ROM )
             ? 0
             : &ctx->input;
    if( wp_button_behavior( &ctx->last_widget_state, header, in, WORKPHONE_BUTTON_DEFAULT ) )
        is_clicked = wp_true;

    /* draw combo box header background and border */
    if( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_ACTIVED )
    {
        background = &style->combo.active;
        text.text = style->combo.label_active;
    }
    else if( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_HOVER )
    {
        background = &style->combo.hover;
        text.text = style->combo.label_hover;
    }
    else
    {
        background = &style->combo.normal;
        text.text = style->combo.label_normal;
    }

    text.text = wp_rgb_factor( text.text, style->combo.color_factor );

    switch( background->type )
    {
    case WORKPHONE_STYLE_ITEM_IMAGE:
        text.background = wp_rgba( 0, 0, 0, 0 );
        wp_draw_image( &win->buffer, header, &background->data.image,
                       wp_rgb_factor( wp_white, style->combo.color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_NINE_SLICE:
        text.background = wp_rgba( 0, 0, 0, 0 );
        wp_draw_nine_slice( &win->buffer, header, &background->data.slice,
                            wp_rgb_factor( wp_white, style->combo.color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_COLOR:
        text.background = background->data.color;
        wp_fill_rect( &win->buffer, header, style->combo.rounding,
                      wp_rgb_factor( background->data.color, style->combo.color_factor ) );
        wp_stroke_rect( &win->buffer, header, style->combo.rounding, style->combo.border,
                        wp_rgb_factor( style->combo.border_color, style->combo.color_factor ) );
        break;
    }
    {
        /* prwp_s32 currently selected text item */
        struct wp_rect label;
        struct wp_rect button;
        struct wp_rect content;
        wp_s32 draw_button_symbol;

        enum wp_symbol_type sym;
        if( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_HOVER )
            sym = style->combo.sym_hover;
        else if( is_clicked )
            sym = style->combo.sym_active;
        else
            sym = style->combo.sym_normal;

        /* represents whether or not the combo's button symbol should be drawn */
        draw_button_symbol = sym != WORKPHONE_SYMBOL_NONE;

        /* calculate button */
        button.w = header.h - 2 * style->combo.button_padding.y;
        button.x = ( header.x + header.w - header.h ) - style->combo.button_padding.x;
        button.y = header.y + style->combo.button_padding.y;
        button.h = button.w;

        content.x = button.x + style->combo.button.padding.x;
        content.y = button.y + style->combo.button.padding.y;
        content.w = button.w - 2 * style->combo.button.padding.x;
        content.h = button.h - 2 * style->combo.button.padding.y;

        /* draw selected label */
        text.padding = wp_make_vec2f( 0, 0 );
        label.x = header.x + style->combo.content_padding.x;
        label.y = header.y + style->combo.content_padding.y;
        label.h = header.h - 2 * style->combo.content_padding.y;
        if( draw_button_symbol )
            label.w = button.x - ( style->combo.content_padding.x + style->combo.spacing.x ) - label.x;
        else
            label.w = header.w - 2 * style->combo.content_padding.x;
        wp_widget_text( &win->buffer, label, selected, len, &text, WORKPHONE_TEXT_LEFT,
                        ctx->style.font );

        /* draw open/close button */
        if( draw_button_symbol )
            wp_draw_button_symbol( &win->buffer, &button, &content, ctx->last_widget_state,
                                   &ctx->style.combo.button, sym, style->font );
    }
    return wp_combo_begin( ctx, win, size, is_clicked, header );
}
WORKPHONE_API wp_bool wp_combo_begin_label( struct wp_context *ctx, const wp_c8 *selected,
                                            struct wp_vec2f size )
{
    return wp_combo_begin_text( ctx, selected, wp_strlen( selected ), size );
}
WORKPHONE_API wp_bool wp_combo_begin_color( struct wp_context *ctx, struct wp_color color,
                                            struct wp_vec2f size )
{
    struct wp_window *win;
    struct wp_style *style;
    const struct wp_input *in;

    struct wp_rect header;
    wp_s32 is_clicked = wp_false;
    enum wp_widget_layout_states s;
    const struct wp_style_item *background;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    style = &ctx->style;
    s = wp_widget( &header, ctx );
    if( s == WORKPHONE_WIDGET_INVALID )
        return 0;

    in = ( win->layout->flags & WORKPHONE_WINDOW_ROM || s == WORKPHONE_WIDGET_DISABLED ||
           s == WORKPHONE_WIDGET_ROM )
             ? 0
             : &ctx->input;
    if( wp_button_behavior( &ctx->last_widget_state, header, in, WORKPHONE_BUTTON_DEFAULT ) )
        is_clicked = wp_true;

    /* draw combo box header background and border */
    if( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_ACTIVED )
        background = &style->combo.active;
    else if( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_HOVER )
        background = &style->combo.hover;
    else
        background = &style->combo.normal;

    switch( background->type )
    {
    case WORKPHONE_STYLE_ITEM_IMAGE:
        wp_draw_image( &win->buffer, header, &background->data.image,
                       wp_rgb_factor( wp_white, style->combo.color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_NINE_SLICE:
        wp_draw_nine_slice( &win->buffer, header, &background->data.slice,
                            wp_rgb_factor( wp_white, style->combo.color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_COLOR:
        wp_fill_rect( &win->buffer, header, style->combo.rounding,
                      wp_rgb_factor( background->data.color, style->combo.color_factor ) );
        wp_stroke_rect( &win->buffer, header, style->combo.rounding, style->combo.border,
                        wp_rgb_factor( style->combo.border_color, style->combo.color_factor ) );
        break;
    }
    {
        struct wp_rect content;
        struct wp_rect button;
        struct wp_rect bounds;
        wp_s32 draw_button_symbol;

        enum wp_symbol_type sym;
        if( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_HOVER )
            sym = style->combo.sym_hover;
        else if( is_clicked )
            sym = style->combo.sym_active;
        else
            sym = style->combo.sym_normal;

        /* represents whether or not the combo's button symbol should be drawn */
        draw_button_symbol = sym != WORKPHONE_SYMBOL_NONE;

        /* calculate button */
        button.w = header.h - 2 * style->combo.button_padding.y;
        button.x = ( header.x + header.w - header.h ) - style->combo.button_padding.x;
        button.y = header.y + style->combo.button_padding.y;
        button.h = button.w;

        content.x = button.x + style->combo.button.padding.x;
        content.y = button.y + style->combo.button.padding.y;
        content.w = button.w - 2 * style->combo.button.padding.x;
        content.h = button.h - 2 * style->combo.button.padding.y;

        /* draw color */
        bounds.h = header.h - 4 * style->combo.content_padding.y;
        bounds.y = header.y + 2 * style->combo.content_padding.y;
        bounds.x = header.x + 2 * style->combo.content_padding.x;
        if( draw_button_symbol )
            bounds.w =
                ( button.x - ( style->combo.content_padding.x + style->combo.spacing.x ) ) - bounds.x;
        else
            bounds.w = header.w - 4 * style->combo.content_padding.x;
        wp_fill_rect( &win->buffer, bounds, 0, wp_rgb_factor( color, style->combo.color_factor ) );

        /* draw open/close button */
        if( draw_button_symbol )
            wp_draw_button_symbol( &win->buffer, &button, &content, ctx->last_widget_state,
                                   &ctx->style.combo.button, sym, style->font );
    }
    return wp_combo_begin( ctx, win, size, is_clicked, header );
}
WORKPHONE_API wp_bool wp_combo_begin_symbol( struct wp_context *ctx, enum wp_symbol_type symbol,
                                             struct wp_vec2f size )
{
    struct wp_window *win;
    struct wp_style *style;
    const struct wp_input *in;

    struct wp_rect header;
    wp_s32 is_clicked = wp_false;
    enum wp_widget_layout_states s;
    const struct wp_style_item *background;
    struct wp_color sym_background;
    struct wp_color symbol_color;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    style = &ctx->style;
    s = wp_widget( &header, ctx );
    if( s == WORKPHONE_WIDGET_INVALID )
        return 0;

    in = ( win->layout->flags & WORKPHONE_WINDOW_ROM || s == WORKPHONE_WIDGET_DISABLED ||
           s == WORKPHONE_WIDGET_ROM )
             ? 0
             : &ctx->input;
    if( wp_button_behavior( &ctx->last_widget_state, header, in, WORKPHONE_BUTTON_DEFAULT ) )
        is_clicked = wp_true;

    /* draw combo box header background and border */
    if( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_ACTIVED )
    {
        background = &style->combo.active;
        symbol_color = style->combo.symbol_active;
    }
    else if( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_HOVER )
    {
        background = &style->combo.hover;
        symbol_color = style->combo.symbol_hover;
    }
    else
    {
        background = &style->combo.normal;
        symbol_color = style->combo.symbol_hover;
    }

    symbol_color = wp_rgb_factor( symbol_color, style->combo.color_factor );

    switch( background->type )
    {
    case WORKPHONE_STYLE_ITEM_IMAGE:
        sym_background = wp_rgba( 0, 0, 0, 0 );
        wp_draw_image( &win->buffer, header, &background->data.image,
                       wp_rgb_factor( wp_white, style->combo.color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_NINE_SLICE:
        sym_background = wp_rgba( 0, 0, 0, 0 );
        wp_draw_nine_slice( &win->buffer, header, &background->data.slice,
                            wp_rgb_factor( wp_white, style->combo.color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_COLOR:
        sym_background = background->data.color;
        wp_fill_rect( &win->buffer, header, style->combo.rounding,
                      wp_rgb_factor( background->data.color, style->combo.color_factor ) );
        wp_stroke_rect( &win->buffer, header, style->combo.rounding, style->combo.border,
                        wp_rgb_factor( style->combo.border_color, style->combo.color_factor ) );
        break;
    }
    {
        struct wp_rect bounds = { 0, 0, 0, 0 };
        struct wp_rect content;
        struct wp_rect button;

        enum wp_symbol_type sym;
        if( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_HOVER )
            sym = style->combo.sym_hover;
        else if( is_clicked )
            sym = style->combo.sym_active;
        else
            sym = style->combo.sym_normal;

        /* calculate button */
        button.w = header.h - 2 * style->combo.button_padding.y;
        button.x = ( header.x + header.w - header.h ) - style->combo.button_padding.y;
        button.y = header.y + style->combo.button_padding.y;
        button.h = button.w;

        content.x = button.x + style->combo.button.padding.x;
        content.y = button.y + style->combo.button.padding.y;
        content.w = button.w - 2 * style->combo.button.padding.x;
        content.h = button.h - 2 * style->combo.button.padding.y;

        /* draw symbol */
        bounds.h = header.h - 2 * style->combo.content_padding.y;
        bounds.y = header.y + style->combo.content_padding.y;
        bounds.x = header.x + style->combo.content_padding.x;
        bounds.w = ( button.x - style->combo.content_padding.y ) - bounds.x;
        wp_draw_symbol( &win->buffer, symbol, bounds, sym_background, symbol_color, 1.0f, style->font );

        /* draw open/close button */
        wp_draw_button_symbol( &win->buffer, &bounds, &content, ctx->last_widget_state,
                               &ctx->style.combo.button, sym, style->font );
    }
    return wp_combo_begin( ctx, win, size, is_clicked, header );
}
WORKPHONE_API wp_bool wp_combo_begin_symbol_text( struct wp_context *ctx, const wp_c8 *selected,
                                                  wp_s32 len, enum wp_symbol_type symbol,
                                                  struct wp_vec2f size )
{
    struct wp_window *win;
    struct wp_style *style;
    struct wp_input *in;

    struct wp_rect header;
    wp_s32 is_clicked = wp_false;
    enum wp_widget_layout_states s;
    const struct wp_style_item *background;
    struct wp_color symbol_color;
    struct wp_text text;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    style = &ctx->style;
    s = wp_widget( &header, ctx );
    if( !s )
        return 0;

    in = ( win->layout->flags & WORKPHONE_WINDOW_ROM || s == WORKPHONE_WIDGET_DISABLED ||
           s == WORKPHONE_WIDGET_ROM )
             ? 0
             : &ctx->input;
    if( wp_button_behavior( &ctx->last_widget_state, header, in, WORKPHONE_BUTTON_DEFAULT ) )
        is_clicked = wp_true;

    /* draw combo box header background and border */
    if( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_ACTIVED )
    {
        background = &style->combo.active;
        symbol_color = style->combo.symbol_active;
        text.text = style->combo.label_active;
    }
    else if( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_HOVER )
    {
        background = &style->combo.hover;
        symbol_color = style->combo.symbol_hover;
        text.text = style->combo.label_hover;
    }
    else
    {
        background = &style->combo.normal;
        symbol_color = style->combo.symbol_normal;
        text.text = style->combo.label_normal;
    }

    text.text = wp_rgb_factor( text.text, style->combo.color_factor );
    symbol_color = wp_rgb_factor( symbol_color, style->combo.color_factor );

    switch( background->type )
    {
    case WORKPHONE_STYLE_ITEM_IMAGE:
        text.background = wp_rgba( 0, 0, 0, 0 );
        wp_draw_image( &win->buffer, header, &background->data.image,
                       wp_rgb_factor( wp_white, style->combo.color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_NINE_SLICE:
        text.background = wp_rgba( 0, 0, 0, 0 );
        wp_draw_nine_slice( &win->buffer, header, &background->data.slice,
                            wp_rgb_factor( wp_white, style->combo.color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_COLOR:
        text.background = background->data.color;
        wp_fill_rect( &win->buffer, header, style->combo.rounding,
                      wp_rgb_factor( background->data.color, style->combo.color_factor ) );
        wp_stroke_rect( &win->buffer, header, style->combo.rounding, style->combo.border,
                        wp_rgb_factor( style->combo.border_color, style->combo.color_factor ) );
        break;
    }
    {
        struct wp_rect content;
        struct wp_rect button;
        struct wp_rect label;
        struct wp_rect image;

        enum wp_symbol_type sym;
        if( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_HOVER )
            sym = style->combo.sym_hover;
        else if( is_clicked )
            sym = style->combo.sym_active;
        else
            sym = style->combo.sym_normal;

        /* calculate button */
        button.w = header.h - 2 * style->combo.button_padding.y;
        button.x = ( header.x + header.w - header.h ) - style->combo.button_padding.x;
        button.y = header.y + style->combo.button_padding.y;
        button.h = button.w;

        content.x = button.x + style->combo.button.padding.x;
        content.y = button.y + style->combo.button.padding.y;
        content.w = button.w - 2 * style->combo.button.padding.x;
        content.h = button.h - 2 * style->combo.button.padding.y;
        wp_draw_button_symbol( &win->buffer, &button, &content, ctx->last_widget_state,
                               &ctx->style.combo.button, sym, style->font );

        /* draw symbol */
        image.x = header.x + style->combo.content_padding.x;
        image.y = header.y + style->combo.content_padding.y;
        image.h = header.h - 2 * style->combo.content_padding.y;
        image.w = image.h;
        wp_draw_symbol( &win->buffer, symbol, image, text.background, symbol_color, 1.0f, style->font );

        /* draw label */
        text.padding = wp_make_vec2f( 0, 0 );
        label.x = image.x + image.w + style->combo.spacing.x + style->combo.content_padding.x;
        label.y = header.y + style->combo.content_padding.y;
        label.w = ( button.x - style->combo.content_padding.x ) - label.x;
        label.h = header.h - 2 * style->combo.content_padding.y;
        wp_widget_text( &win->buffer, label, selected, len, &text, WORKPHONE_TEXT_LEFT, style->font );
    }
    return wp_combo_begin( ctx, win, size, is_clicked, header );
}
WORKPHONE_API wp_bool wp_combo_begin_image( struct wp_context *ctx, struct wp_image img,
                                            struct wp_vec2f size )
{
    struct wp_window *win;
    struct wp_style *style;
    const struct wp_input *in;

    struct wp_rect header;
    wp_s32 is_clicked = wp_false;
    enum wp_widget_layout_states s;
    const struct wp_style_item *background;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    style = &ctx->style;
    s = wp_widget( &header, ctx );
    if( s == WORKPHONE_WIDGET_INVALID )
        return 0;

    in = ( win->layout->flags & WORKPHONE_WINDOW_ROM || s == WORKPHONE_WIDGET_DISABLED ||
           s == WORKPHONE_WIDGET_ROM )
             ? 0
             : &ctx->input;
    if( wp_button_behavior( &ctx->last_widget_state, header, in, WORKPHONE_BUTTON_DEFAULT ) )
        is_clicked = wp_true;

    /* draw combo box header background and border */
    if( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_ACTIVED )
        background = &style->combo.active;
    else if( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_HOVER )
        background = &style->combo.hover;
    else
        background = &style->combo.normal;

    switch( background->type )
    {
    case WORKPHONE_STYLE_ITEM_IMAGE:
        wp_draw_image( &win->buffer, header, &background->data.image,
                       wp_rgb_factor( wp_white, style->combo.color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_NINE_SLICE:
        wp_draw_nine_slice( &win->buffer, header, &background->data.slice,
                            wp_rgb_factor( wp_white, style->combo.color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_COLOR:
        wp_fill_rect( &win->buffer, header, style->combo.rounding,
                      wp_rgb_factor( background->data.color, style->combo.color_factor ) );
        wp_stroke_rect( &win->buffer, header, style->combo.rounding, style->combo.border,
                        wp_rgb_factor( style->combo.border_color, style->combo.color_factor ) );
        break;
    }
    {
        struct wp_rect bounds = { 0, 0, 0, 0 };
        struct wp_rect content;
        struct wp_rect button;
        wp_s32 draw_button_symbol;

        enum wp_symbol_type sym;
        if( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_HOVER )
            sym = style->combo.sym_hover;
        else if( is_clicked )
            sym = style->combo.sym_active;
        else
            sym = style->combo.sym_normal;

        /* represents whether or not the combo's button symbol should be drawn */
        draw_button_symbol = sym != WORKPHONE_SYMBOL_NONE;

        /* calculate button */
        button.w = header.h - 2 * style->combo.button_padding.y;
        button.x = ( header.x + header.w - header.h ) - style->combo.button_padding.y;
        button.y = header.y + style->combo.button_padding.y;
        button.h = button.w;

        content.x = button.x + style->combo.button.padding.x;
        content.y = button.y + style->combo.button.padding.y;
        content.w = button.w - 2 * style->combo.button.padding.x;
        content.h = button.h - 2 * style->combo.button.padding.y;

        /* draw image */
        bounds.h = header.h - 2 * style->combo.content_padding.y;
        bounds.y = header.y + style->combo.content_padding.y;
        bounds.x = header.x + style->combo.content_padding.x;
        if( draw_button_symbol )
            bounds.w = ( button.x - style->combo.content_padding.y ) - bounds.x;
        else
            bounds.w = header.w - 2 * style->combo.content_padding.x;
        wp_draw_image( &win->buffer, bounds, &img,
                       wp_rgb_factor( wp_white, style->combo.color_factor ) );

        /* draw open/close button */
        if( draw_button_symbol )
            wp_draw_button_symbol( &win->buffer, &bounds, &content, ctx->last_widget_state,
                                   &ctx->style.combo.button, sym, style->font );
    }
    return wp_combo_begin( ctx, win, size, is_clicked, header );
}
WORKPHONE_API wp_bool wp_combo_begin_image_text( struct wp_context *ctx, const wp_c8 *selected,
                                                 wp_s32 len, struct wp_image img, struct wp_vec2f size )
{
    struct wp_window *win;
    struct wp_style *style;
    struct wp_input *in;

    struct wp_rect header;
    wp_s32 is_clicked = wp_false;
    enum wp_widget_layout_states s;
    const struct wp_style_item *background;
    struct wp_text text;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    style = &ctx->style;
    s = wp_widget( &header, ctx );
    if( !s )
        return 0;

    in = ( win->layout->flags & WORKPHONE_WINDOW_ROM || s == WORKPHONE_WIDGET_DISABLED ||
           s == WORKPHONE_WIDGET_ROM )
             ? 0
             : &ctx->input;
    if( wp_button_behavior( &ctx->last_widget_state, header, in, WORKPHONE_BUTTON_DEFAULT ) )
        is_clicked = wp_true;

    /* draw combo box header background and border */
    if( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_ACTIVED )
    {
        background = &style->combo.active;
        text.text = style->combo.label_active;
    }
    else if( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_HOVER )
    {
        background = &style->combo.hover;
        text.text = style->combo.label_hover;
    }
    else
    {
        background = &style->combo.normal;
        text.text = style->combo.label_normal;
    }

    text.text = wp_rgb_factor( text.text, style->combo.color_factor );

    switch( background->type )
    {
    case WORKPHONE_STYLE_ITEM_IMAGE:
        text.background = wp_rgba( 0, 0, 0, 0 );
        wp_draw_image( &win->buffer, header, &background->data.image,
                       wp_rgb_factor( wp_white, style->combo.color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_NINE_SLICE:
        text.background = wp_rgba( 0, 0, 0, 0 );
        wp_draw_nine_slice( &win->buffer, header, &background->data.slice,
                            wp_rgb_factor( wp_white, style->combo.color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_COLOR:
        text.background = background->data.color;
        wp_fill_rect( &win->buffer, header, style->combo.rounding,
                      wp_rgb_factor( background->data.color, style->combo.color_factor ) );
        wp_stroke_rect( &win->buffer, header, style->combo.rounding, style->combo.border,
                        wp_rgb_factor( style->combo.border_color, style->combo.color_factor ) );
        break;
    }
    {
        struct wp_rect content;
        struct wp_rect button;
        struct wp_rect label;
        struct wp_rect image;
        wp_s32 draw_button_symbol;

        enum wp_symbol_type sym;
        if( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_HOVER )
            sym = style->combo.sym_hover;
        else if( is_clicked )
            sym = style->combo.sym_active;
        else
            sym = style->combo.sym_normal;

        /* represents whether or not the combo's button symbol should be drawn */
        draw_button_symbol = sym != WORKPHONE_SYMBOL_NONE;

        /* calculate button */
        button.w = header.h - 2 * style->combo.button_padding.y;
        button.x = ( header.x + header.w - header.h ) - style->combo.button_padding.x;
        button.y = header.y + style->combo.button_padding.y;
        button.h = button.w;

        content.x = button.x + style->combo.button.padding.x;
        content.y = button.y + style->combo.button.padding.y;
        content.w = button.w - 2 * style->combo.button.padding.x;
        content.h = button.h - 2 * style->combo.button.padding.y;
        if( draw_button_symbol )
            wp_draw_button_symbol( &win->buffer, &button, &content, ctx->last_widget_state,
                                   &ctx->style.combo.button, sym, style->font );

        /* draw image */
        image.x = header.x + style->combo.content_padding.x;
        image.y = header.y + style->combo.content_padding.y;
        image.h = header.h - 2 * style->combo.content_padding.y;
        image.w = image.h;
        wp_draw_image( &win->buffer, image, &img, wp_rgb_factor( wp_white, style->combo.color_factor ) );

        /* draw label */
        text.padding = wp_make_vec2f( 0, 0 );
        label.x = image.x + image.w + style->combo.spacing.x + style->combo.content_padding.x;
        label.y = header.y + style->combo.content_padding.y;
        label.h = header.h - 2 * style->combo.content_padding.y;
        if( draw_button_symbol )
            label.w = ( button.x - style->combo.content_padding.x ) - label.x;
        else
            label.w = ( header.x + header.w - style->combo.content_padding.x ) - label.x;
        wp_widget_text( &win->buffer, label, selected, len, &text, WORKPHONE_TEXT_LEFT, style->font );
    }
    return wp_combo_begin( ctx, win, size, is_clicked, header );
}
WORKPHONE_API wp_bool wp_combo_begin_symbol_label( struct wp_context *ctx, const wp_c8 *selected,
                                                   enum wp_symbol_type type, struct wp_vec2f size )
{
    return wp_combo_begin_symbol_text( ctx, selected, wp_strlen( selected ), type, size );
}
WORKPHONE_API wp_bool wp_combo_begin_image_label( struct wp_context *ctx, const wp_c8 *selected,
                                                  struct wp_image img, struct wp_vec2f size )
{
    return wp_combo_begin_image_text( ctx, selected, wp_strlen( selected ), img, size );
}
WORKPHONE_API wp_bool wp_combo_item_text( struct wp_context *ctx, const wp_c8 *text, wp_s32 len,
                                          wp_flags align )
{
    return wp_contextual_item_text( ctx, text, len, align );
}
WORKPHONE_API wp_bool wp_combo_item_label( struct wp_context *ctx, const wp_c8 *label, wp_flags align )
{
    return wp_contextual_item_label( ctx, label, align );
}
WORKPHONE_API wp_bool wp_combo_item_image_text( struct wp_context *ctx, struct wp_image img,
                                                const wp_c8 *text, wp_s32 len, wp_flags alignment )
{
    return wp_contextual_item_image_text( ctx, img, text, len, alignment );
}
WORKPHONE_API wp_bool wp_combo_item_image_label( struct wp_context *ctx, struct wp_image img,
                                                 const wp_c8 *text, wp_flags alignment )
{
    return wp_contextual_item_image_label( ctx, img, text, alignment );
}
WORKPHONE_API wp_bool wp_combo_item_symbol_text( struct wp_context *ctx, enum wp_symbol_type sym,
                                                 const wp_c8 *text, wp_s32 len, wp_flags alignment )
{
    return wp_contextual_item_symbol_text( ctx, sym, text, len, alignment );
}
WORKPHONE_API wp_bool wp_combo_item_symbol_label( struct wp_context *ctx, enum wp_symbol_type sym,
                                                  const wp_c8 *label, wp_flags alignment )
{
    return wp_contextual_item_symbol_label( ctx, sym, label, alignment );
}
WORKPHONE_API void wp_combo_end( struct wp_context *ctx )
{
    wp_contextual_end( ctx );
}
WORKPHONE_API void wp_combo_close( struct wp_context *ctx )
{
    wp_contextual_close( ctx );
}
WORKPHONE_API wp_s32 wp_combo( struct wp_context *ctx, const wp_c8 *const *items, wp_s32 count,
                               wp_s32 selected, wp_s32 item_height, struct wp_vec2f size )
{
    wp_s32 i = 0;
    wp_s32 max_height;
    struct wp_vec2f item_spacing;
    struct wp_vec2f window_padding;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( items );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !items || !count )
        return selected;

    item_spacing = ctx->style.window.spacing;
    window_padding = wp_panel_get_padding( &ctx->style, ctx->current->layout->type );
    max_height = count * item_height + count * (wp_s32)item_spacing.y;
    max_height += (wp_s32)item_spacing.y * 2 + (wp_s32)window_padding.y * 2;
    size.y = WORKPHONE_MIN( size.y, (wp_f32)max_height );
    if( wp_combo_begin_label( ctx, items[selected], size ) )
    {
        wp_layout_row_dynamic( ctx, (wp_f32)item_height, 1 );
        for( i = 0; i < count; ++i )
        {
            if( wp_combo_item_label( ctx, items[i], WORKPHONE_TEXT_LEFT ) )
                selected = i;
        }
        wp_combo_end( ctx );
    }
    return selected;
}
WORKPHONE_API wp_s32 wp_combo_separator( struct wp_context *ctx,
                                         const wp_c8 *items_separated_by_separator, wp_s32 separator,
                                         wp_s32 selected, wp_s32 count, wp_s32 item_height,
                                         struct wp_vec2f size )
{
    wp_s32 i;
    wp_s32 max_height;
    struct wp_vec2f item_spacing;
    struct wp_vec2f window_padding;
    const wp_c8 *current_item;
    const wp_c8 *iter;
    wp_s32 length = 0;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( items_separated_by_separator );
    if( !ctx || !items_separated_by_separator )
        return selected;

    /* calculate popup window */
    item_spacing = ctx->style.window.spacing;
    window_padding = wp_panel_get_padding( &ctx->style, ctx->current->layout->type );
    max_height = count * item_height + count * (wp_s32)item_spacing.y;
    max_height += (wp_s32)item_spacing.y * 2 + (wp_s32)window_padding.y * 2;
    size.y = WORKPHONE_MIN( size.y, (wp_f32)max_height );

    /* find selected item */
    current_item = items_separated_by_separator;
    for( i = 0; i < count; ++i )
    {
        iter = current_item;
        while( *iter && *iter != separator )
            iter++;
        length = (wp_s32)( iter - current_item );
        if( i == selected )
            break;
        current_item = iter + 1;
    }

    if( wp_combo_begin_text( ctx, current_item, length, size ) )
    {
        current_item = items_separated_by_separator;
        wp_layout_row_dynamic( ctx, (wp_f32)item_height, 1 );
        for( i = 0; i < count; ++i )
        {
            iter = current_item;
            while( *iter && *iter != separator )
                iter++;
            length = (wp_s32)( iter - current_item );
            if( wp_combo_item_text( ctx, current_item, length, WORKPHONE_TEXT_LEFT ) )
                selected = i;
            current_item = current_item + length + 1;
        }
        wp_combo_end( ctx );
    }
    return selected;
}
WORKPHONE_API wp_s32 wp_combo_string( struct wp_context *ctx, const wp_c8 *items_separated_by_zeros,
                                      wp_s32 selected, wp_s32 count, wp_s32 item_height,
                                      struct wp_vec2f size )
{
    return wp_combo_separator( ctx, items_separated_by_zeros, '\0', selected, count, item_height, size );
}
WORKPHONE_API wp_s32 wp_combo_callback( struct wp_context *ctx,
                                        void ( *item_getter )( void *, int, const wp_c8 ** ),
                                        void *userdata, wp_s32 selected, wp_s32 count,
                                        wp_s32 item_height, struct wp_vec2f size )
{
    wp_s32 i;
    wp_s32 max_height;
    struct wp_vec2f item_spacing;
    struct wp_vec2f window_padding;
    const wp_c8 *item;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( item_getter );
    if( !ctx || !item_getter )
        return selected;

    /* calculate popup window */
    item_spacing = ctx->style.window.spacing;
    window_padding = wp_panel_get_padding( &ctx->style, ctx->current->layout->type );
    max_height = count * item_height + count * (wp_s32)item_spacing.y;
    max_height += (wp_s32)item_spacing.y * 2 + (wp_s32)window_padding.y * 2;
    size.y = WORKPHONE_MIN( size.y, (wp_f32)max_height );

    item_getter( userdata, selected, &item );
    if( wp_combo_begin_label( ctx, item, size ) )
    {
        wp_layout_row_dynamic( ctx, (wp_f32)item_height, 1 );
        for( i = 0; i < count; ++i )
        {
            item_getter( userdata, i, &item );
            if( wp_combo_item_label( ctx, item, WORKPHONE_TEXT_LEFT ) )
                selected = i;
        }
        wp_combo_end( ctx );
    }
    return selected;
}
WORKPHONE_API void wp_combobox( struct wp_context *ctx, const wp_c8 *const *items, wp_s32 count,
                                wp_s32 *selected, wp_s32 item_height, struct wp_vec2f size )
{
    *selected = wp_combo( ctx, items, count, *selected, item_height, size );
}
WORKPHONE_API void wp_combobox_string( struct wp_context *ctx, const wp_c8 *items_separated_by_zeros,
                                       wp_s32 *selected, wp_s32 count, wp_s32 item_height,
                                       struct wp_vec2f size )
{
    *selected = wp_combo_string( ctx, items_separated_by_zeros, *selected, count, item_height, size );
}
WORKPHONE_API void wp_combobox_separator( struct wp_context *ctx,
                                          const wp_c8 *items_separated_by_separator, wp_s32 separator,
                                          wp_s32 *selected, wp_s32 count, wp_s32 item_height,
                                          struct wp_vec2f size )
{
    *selected = wp_combo_separator( ctx, items_separated_by_separator, separator, *selected, count,
                                    item_height, size );
}
WORKPHONE_API void wp_combobox_callback(
    struct wp_context *ctx, void ( *item_getter )( void *data, wp_s32 id, const wp_c8 **out_text ),
    void *userdata, wp_s32 *selected, wp_s32 count, wp_s32 item_height, struct wp_vec2f size )
{
    *selected = wp_combo_callback( ctx, item_getter, userdata, *selected, count, item_height, size );
}
