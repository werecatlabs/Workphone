#include "workphone.h"
#include "workphone_button.h"
#include "workphone_widget.h"

void wp_draw_selectable( struct wp_command_buffer *out, wp_flags state,
                         const struct wp_style_selectable *style, wp_bool active,
                         const struct wp_rect *bounds, const struct wp_rect *icon,
                         const struct wp_image *img, enum wp_symbol_type sym, const wp_c8 *string,
                         wp_s32 len, wp_flags align, const struct wp_user_font *font )
{
    const struct wp_style_item *background;
    struct wp_text text;
    text.padding = style->padding;

    /* select correct colors/images */
    if( !active )
    {
        if( state & WORKPHONE_WIDGET_STATE_ACTIVED )
        {
            background = &style->pressed;
            text.text = style->text_pressed;
        }
        else if( state & WORKPHONE_WIDGET_STATE_HOVER )
        {
            background = &style->hover;
            text.text = style->text_hover;
        }
        else
        {
            background = &style->normal;
            text.text = style->text_normal;
        }
    }
    else
    {
        if( state & WORKPHONE_WIDGET_STATE_ACTIVED )
        {
            background = &style->pressed_active;
            text.text = style->text_pressed_active;
        }
        else if( state & WORKPHONE_WIDGET_STATE_HOVER )
        {
            background = &style->hover_active;
            text.text = style->text_hover_active;
        }
        else
        {
            background = &style->normal_active;
            text.text = style->text_normal_active;
        }
    }

    text.text = wp_rgb_factor( text.text, style->color_factor );

    /* draw selectable background and text */
    switch( background->type )
    {
    case WORKPHONE_STYLE_ITEM_IMAGE:
        text.background = wp_rgba( 0, 0, 0, 0 );
        wp_draw_image( out, *bounds, &background->data.image,
                       wp_rgb_factor( wp_white, style->color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_NINE_SLICE:
        text.background = wp_rgba( 0, 0, 0, 0 );
        wp_draw_nine_slice( out, *bounds, &background->data.slice,
                            wp_rgb_factor( wp_white, style->color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_COLOR:
        text.background = background->data.color;
        wp_fill_rect( out, *bounds, style->rounding, background->data.color );
        break;
    }
    if( icon )
    {
        if( img )
            wp_draw_image( out, *icon, img, wp_rgb_factor( wp_white, style->color_factor ) );
        else
            wp_draw_symbol( out, sym, *icon, text.background, text.text, 1, font );
    }
    wp_widget_text( out, *bounds, string, len, &text, align, font );
}

wp_bool wp_do_selectable( wp_flags *state, struct wp_command_buffer *out, struct wp_rect bounds,
                          const wp_c8 *str, wp_s32 len, wp_flags align, wp_bool *value,
                          const struct wp_style_selectable *style, const struct wp_input *in,
                          const struct wp_user_font *font )
{
    wp_s32 old_value;
    struct wp_rect touch;

    WORKPHONE_ASSERT( state );
    WORKPHONE_ASSERT( out );
    WORKPHONE_ASSERT( str );
    WORKPHONE_ASSERT( len );
    WORKPHONE_ASSERT( value );
    WORKPHONE_ASSERT( style );
    WORKPHONE_ASSERT( font );

    if( !state || !out || !str || !len || !value || !style || !font )
        return 0;
    old_value = *value;

    /* remove padding */
    touch.x = bounds.x - style->touch_padding.x;
    touch.y = bounds.y - style->touch_padding.y;
    touch.w = bounds.w + style->touch_padding.x * 2;
    touch.h = bounds.h + style->touch_padding.y * 2;

    /* update button */
    if( wp_button_behavior( state, touch, in, WORKPHONE_BUTTON_DEFAULT ) )
        *value = !( *value );

    /* draw selectable */
    if( style->draw_begin )
        style->draw_begin( out, style->userdata );
    wp_draw_selectable( out, *state, style, *value, &bounds, 0, 0, WORKPHONE_SYMBOL_NONE, str, len,
                        align, font );
    if( style->draw_end )
        style->draw_end( out, style->userdata );
    return old_value != *value;
}
WORKPHONE_LIB wp_bool wp_do_selectable_image( wp_flags *state, struct wp_command_buffer *out,
                                              struct wp_rect bounds, const wp_c8 *str, wp_s32 len,
                                              wp_flags align, wp_bool *value, const struct wp_image *img,
                                              const struct wp_style_selectable *style,
                                              const struct wp_input *in,
                                              const struct wp_user_font *font )
{
    wp_bool old_value;
    struct wp_rect touch;
    struct wp_rect icon;

    WORKPHONE_ASSERT( state );
    WORKPHONE_ASSERT( out );
    WORKPHONE_ASSERT( str );
    WORKPHONE_ASSERT( len );
    WORKPHONE_ASSERT( value );
    WORKPHONE_ASSERT( style );
    WORKPHONE_ASSERT( font );

    if( !state || !out || !str || !len || !value || !style || !font )
        return 0;
    old_value = *value;

    /* toggle behavior */
    touch.x = bounds.x - style->touch_padding.x;
    touch.y = bounds.y - style->touch_padding.y;
    touch.w = bounds.w + style->touch_padding.x * 2;
    touch.h = bounds.h + style->touch_padding.y * 2;
    if( wp_button_behavior( state, touch, in, WORKPHONE_BUTTON_DEFAULT ) )
        *value = !( *value );

    icon.y = bounds.y + style->padding.y;
    icon.w = icon.h = bounds.h - 2 * style->padding.y;
    if( align & WORKPHONE_TEXT_ALIGN_LEFT )
    {
        icon.x = ( bounds.x + bounds.w ) - ( 2 * style->padding.x + icon.w );
        icon.x = WORKPHONE_MAX( icon.x, 0 );
    }
    else
        icon.x = bounds.x + 2 * style->padding.x;

    icon.x += style->image_padding.x;
    icon.y += style->image_padding.y;
    icon.w -= 2 * style->image_padding.x;
    icon.h -= 2 * style->image_padding.y;

    /* draw selectable */
    if( style->draw_begin )
        style->draw_begin( out, style->userdata );
    wp_draw_selectable( out, *state, style, *value, &bounds, &icon, img, WORKPHONE_SYMBOL_NONE, str, len,
                        align, font );
    if( style->draw_end )
        style->draw_end( out, style->userdata );
    return old_value != *value;
}
WORKPHONE_LIB wp_bool wp_do_selectable_symbol( wp_flags *state, struct wp_command_buffer *out,
                                               struct wp_rect bounds, const wp_c8 *str, wp_s32 len,
                                               wp_flags align, wp_bool *value, enum wp_symbol_type sym,
                                               const struct wp_style_selectable *style,
                                               const struct wp_input *in,
                                               const struct wp_user_font *font )
{
    wp_s32 old_value;
    struct wp_rect touch;
    struct wp_rect icon;

    WORKPHONE_ASSERT( state );
    WORKPHONE_ASSERT( out );
    WORKPHONE_ASSERT( str );
    WORKPHONE_ASSERT( len );
    WORKPHONE_ASSERT( value );
    WORKPHONE_ASSERT( style );
    WORKPHONE_ASSERT( font );

    if( !state || !out || !str || !len || !value || !style || !font )
        return 0;
    old_value = *value;

    /* toggle behavior */
    touch.x = bounds.x - style->touch_padding.x;
    touch.y = bounds.y - style->touch_padding.y;
    touch.w = bounds.w + style->touch_padding.x * 2;
    touch.h = bounds.h + style->touch_padding.y * 2;
    if( wp_button_behavior( state, touch, in, WORKPHONE_BUTTON_DEFAULT ) )
        *value = !( *value );

    icon.y = bounds.y + style->padding.y;
    icon.w = icon.h = bounds.h - 2 * style->padding.y;
    if( align & WORKPHONE_TEXT_ALIGN_LEFT )
    {
        icon.x = ( bounds.x + bounds.w ) - ( 2 * style->padding.x + icon.w );
        icon.x = WORKPHONE_MAX( icon.x, 0 );
    }
    else
        icon.x = bounds.x + 2 * style->padding.x;

    icon.x += style->image_padding.x;
    icon.y += style->image_padding.y;
    icon.w -= 2 * style->image_padding.x;
    icon.h -= 2 * style->image_padding.y;

    /* draw selectable */
    if( style->draw_begin )
        style->draw_begin( out, style->userdata );
    wp_draw_selectable( out, *state, style, *value, &bounds, &icon, 0, sym, str, len, align, font );
    if( style->draw_end )
        style->draw_end( out, style->userdata );
    return old_value != *value;
}

WORKPHONE_API wp_bool wp_selectable_text( struct wp_context *ctx, const wp_c8 *str, wp_s32 len,
                                          wp_flags align, wp_bool *value )
{
    struct wp_window *win;
    struct wp_panel *layout;
    const struct wp_input *in;
    const struct wp_style *style;

    enum wp_widget_layout_states state;
    struct wp_rect bounds;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( value );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout || !value )
        return 0;

    win = ctx->current;
    layout = win->layout;
    style = &ctx->style;

    state = wp_widget( &bounds, ctx );
    if( !state )
        return 0;
    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;
    return wp_do_selectable( &ctx->last_widget_state, &win->buffer, bounds, str, len, align, value,
                             &style->selectable, in, style->font );
}
WORKPHONE_API wp_bool wp_selectable_image_text( struct wp_context *ctx, struct wp_image img,
                                                const wp_c8 *str, wp_s32 len, wp_flags align,
                                                wp_bool *value )
{
    struct wp_window *win;
    struct wp_panel *layout;
    const struct wp_input *in;
    const struct wp_style *style;

    enum wp_widget_layout_states state;
    struct wp_rect bounds;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( value );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout || !value )
        return 0;

    win = ctx->current;
    layout = win->layout;
    style = &ctx->style;

    state = wp_widget( &bounds, ctx );
    if( !state )
        return 0;
    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;
    return wp_do_selectable_image( &ctx->last_widget_state, &win->buffer, bounds, str, len, align, value,
                                   &img, &style->selectable, in, style->font );
}
WORKPHONE_API wp_bool wp_selectable_symbol_text( struct wp_context *ctx, enum wp_symbol_type sym,
                                                 const wp_c8 *str, wp_s32 len, wp_flags align,
                                                 wp_bool *value )
{
    struct wp_window *win;
    struct wp_panel *layout;
    const struct wp_input *in;
    const struct wp_style *style;

    enum wp_widget_layout_states state;
    struct wp_rect bounds;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( value );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout || !value )
        return 0;

    win = ctx->current;
    layout = win->layout;
    style = &ctx->style;

    state = wp_widget( &bounds, ctx );
    if( !state )
        return 0;
    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;
    return wp_do_selectable_symbol( &ctx->last_widget_state, &win->buffer, bounds, str, len, align,
                                    value, sym, &style->selectable, in, style->font );
}
WORKPHONE_API wp_bool wp_selectable_symbol_label( struct wp_context *ctx, enum wp_symbol_type sym,
                                                  const wp_c8 *title, wp_flags align, wp_bool *value )
{
    return wp_selectable_symbol_text( ctx, sym, title, wp_strlen( title ), align, value );
}
WORKPHONE_API wp_bool wp_select_text( struct wp_context *ctx, const wp_c8 *str, wp_s32 len,
                                      wp_flags align, wp_bool value )
{
    wp_selectable_text( ctx, str, len, align, &value );
    return value;
}
WORKPHONE_API wp_bool wp_selectable_label( struct wp_context *ctx, const wp_c8 *str, wp_flags align,
                                           wp_bool *value )
{
    return wp_selectable_text( ctx, str, wp_strlen( str ), align, value );
}
WORKPHONE_API wp_bool wp_selectable_image_label( struct wp_context *ctx, struct wp_image img,
                                                 const wp_c8 *str, wp_flags align, wp_bool *value )
{
    return wp_selectable_image_text( ctx, img, str, wp_strlen( str ), align, value );
}
WORKPHONE_API wp_bool wp_select_label( struct wp_context *ctx, const wp_c8 *str, wp_flags align,
                                       wp_bool value )
{
    wp_selectable_text( ctx, str, wp_strlen( str ), align, &value );
    return value;
}
WORKPHONE_API wp_bool wp_select_image_label( struct wp_context *ctx, struct wp_image img,
                                             const wp_c8 *str, wp_flags align, wp_bool value )
{
    wp_selectable_image_text( ctx, img, str, wp_strlen( str ), align, &value );
    return value;
}
WORKPHONE_API wp_bool wp_select_image_text( struct wp_context *ctx, struct wp_image img,
                                            const wp_c8 *str, wp_s32 len, wp_flags align, wp_bool value )
{
    wp_selectable_image_text( ctx, img, str, len, align, &value );
    return value;
}
WORKPHONE_API wp_bool wp_select_symbol_text( struct wp_context *ctx, enum wp_symbol_type sym,
                                             const wp_c8 *title, wp_s32 title_len, wp_flags align,
                                             wp_bool value )
{
    wp_selectable_symbol_text( ctx, sym, title, title_len, align, &value );
    return value;
}
WORKPHONE_API wp_bool wp_select_symbol_label( struct wp_context *ctx, enum wp_symbol_type sym,
                                              const wp_c8 *title, wp_flags align, wp_bool value )
{
    return wp_select_symbol_text( ctx, sym, title, wp_strlen( title ), align, value );
}
