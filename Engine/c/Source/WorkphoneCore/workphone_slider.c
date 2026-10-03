#include "workphone.h"
#include "workphone_button.h"
#include "workphone_widget.h"

wp_f32 wp_slider_behavior( wp_flags *state, struct wp_rect *logical_cursor,
                           struct wp_rect *visual_cursor, struct wp_input *in, struct wp_rect bounds,
                           wp_f32 slider_min, wp_f32 slider_max, wp_f32 slider_value, wp_f32 slider_step,
                           wp_f32 slider_steps )
{
    wp_s32 left_mouse_down;
    wp_s32 left_mouse_click_in_cursor;

    /* check if visual cursor is being dragged */
    wp_widget_state_reset( state );
    left_mouse_down = in && in->mouse.buttons[WORKPHONE_BUTTON_LEFT].down;
    left_mouse_click_in_cursor = in && wp_input_has_mouse_click_down_in_rect( in, WORKPHONE_BUTTON_LEFT,
                                                                              *visual_cursor, wp_true );

    if( left_mouse_down && left_mouse_click_in_cursor )
    {
        wp_f32 ratio = 0;
        const wp_f32 d = in->mouse.pos.x - ( visual_cursor->x + visual_cursor->w * 0.5f );
        const wp_f32 pxstep = bounds.w / slider_steps;

        /* only update value if the next slider step is reached */
        *state = WORKPHONE_WIDGET_STATE_ACTIVE;
        if( WORKPHONE_ABS( d ) >= pxstep )
        {
            const wp_f32 steps = (wp_f32)( (wp_s32)( WORKPHONE_ABS( d ) / pxstep ) );
            slider_value += ( d > 0 ) ? ( slider_step * steps ) : -( slider_step * steps );
            slider_value = WORKPHONE_CLAMP( slider_min, slider_value, slider_max );
            ratio = ( slider_value - slider_min ) / slider_step;
            logical_cursor->x = bounds.x + ( logical_cursor->w * ratio );
            in->mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked_pos.x = logical_cursor->x;
        }
    }

    /* slider widget state */
    if( wp_input_is_mouse_hovering_rect( in, bounds ) )
        *state = WORKPHONE_WIDGET_STATE_HOVERED;
    if( *state & WORKPHONE_WIDGET_STATE_HOVER && !wp_input_is_mouse_prev_hovering_rect( in, bounds ) )
        *state |= WORKPHONE_WIDGET_STATE_ENTERED;
    else if( wp_input_is_mouse_prev_hovering_rect( in, bounds ) )
        *state |= WORKPHONE_WIDGET_STATE_LEFT;
    return slider_value;
}
WORKPHONE_LIB void wp_draw_slider( struct wp_command_buffer *out, wp_flags state,
                                   const struct wp_style_slider *style, const struct wp_rect *bounds,
                                   const struct wp_rect *visual_cursor, wp_f32 min, wp_f32 value,
                                   wp_f32 max )
{
    struct wp_rect fill;
    struct wp_rect bar;
    const struct wp_style_item *background;

    /* select correct slider images/colors */
    struct wp_color bar_color;
    const struct wp_style_item *cursor;

    WORKPHONE_UNUSED( min );
    WORKPHONE_UNUSED( max );
    WORKPHONE_UNUSED( value );

    if( state & WORKPHONE_WIDGET_STATE_ACTIVED )
    {
        background = &style->active;
        bar_color = style->bar_active;
        cursor = &style->cursor_active;
    }
    else if( state & WORKPHONE_WIDGET_STATE_HOVER )
    {
        background = &style->hover;
        bar_color = style->bar_hover;
        cursor = &style->cursor_hover;
    }
    else
    {
        background = &style->normal;
        bar_color = style->bar_normal;
        cursor = &style->cursor_normal;
    }

    /* calculate slider background bar */
    bar.x = bounds->x;
    bar.y = ( visual_cursor->y + visual_cursor->h / 2 ) - bounds->h / 12;
    bar.w = bounds->w;
    bar.h = bounds->h / 6;

    /* filled background bar style */
    fill.w = ( visual_cursor->x + ( visual_cursor->w / 2.0f ) ) - bar.x;
    fill.x = bar.x;
    fill.y = bar.y;
    fill.h = bar.h;

    /* draw background */
    switch( background->type )
    {
    case WORKPHONE_STYLE_ITEM_IMAGE:
        wp_draw_image( out, *bounds, &background->data.image,
                       wp_rgb_factor( wp_white, style->color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_NINE_SLICE:
        wp_draw_nine_slice( out, *bounds, &background->data.slice,
                            wp_rgb_factor( wp_white, style->color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_COLOR:
        wp_fill_rect( out, *bounds, style->rounding,
                      wp_rgb_factor( background->data.color, style->color_factor ) );
        wp_stroke_rect( out, *bounds, style->rounding, style->border,
                        wp_rgb_factor( style->border_color, style->color_factor ) );
        break;
    }

    /* draw slider bar */
    wp_fill_rect( out, bar, style->rounding, wp_rgb_factor( bar_color, style->color_factor ) );
    wp_fill_rect( out, fill, style->rounding, wp_rgb_factor( style->bar_filled, style->color_factor ) );

    /* draw cursor */
    if( cursor->type == WORKPHONE_STYLE_ITEM_IMAGE )
        wp_draw_image( out, *visual_cursor, &cursor->data.image,
                       wp_rgb_factor( wp_white, style->color_factor ) );
    else
        wp_fill_circle( out, *visual_cursor, wp_rgb_factor( cursor->data.color, style->color_factor ) );
}
WORKPHONE_LIB wp_f32 wp_do_slider( wp_flags *state, struct wp_command_buffer *out, struct wp_rect bounds,
                                   wp_f32 min, wp_f32 val, wp_f32 max, wp_f32 step,
                                   const struct wp_style_slider *style, struct wp_input *in,
                                   const struct wp_user_font *font )
{
    wp_f32 slider_range;
    wp_f32 slider_min;
    wp_f32 slider_max;
    wp_f32 slider_value;
    wp_f32 slider_steps;
    wp_f32 cursor_offset;

    struct wp_rect visual_cursor;
    struct wp_rect logical_cursor;

    WORKPHONE_ASSERT( style );
    WORKPHONE_ASSERT( out );
    if( !out || !style )
        return 0;

    /* remove padding from slider bounds */
    bounds.x = bounds.x + style->padding.x;
    bounds.y = bounds.y + style->padding.y;
    bounds.h = WORKPHONE_MAX( bounds.h, 2 * style->padding.y );
    bounds.w = WORKPHONE_MAX( bounds.w, 2 * style->padding.x + style->cursor_size.x );
    bounds.w -= 2 * style->padding.x;
    bounds.h -= 2 * style->padding.y;

    /* optional buttons */
    if( style->show_buttons )
    {
        wp_flags ws;
        struct wp_rect button;
        button.y = bounds.y;
        button.w = bounds.h;
        button.h = bounds.h;

        /* decrement button */
        button.x = bounds.x;
        if( wp_do_button_symbol( &ws, out, button, style->dec_symbol, WORKPHONE_BUTTON_DEFAULT,
                                 &style->dec_button, in, font ) )
            val -= step;

        /* increment button */
        button.x = ( bounds.x + bounds.w ) - button.w;
        if( wp_do_button_symbol( &ws, out, button, style->inc_symbol, WORKPHONE_BUTTON_DEFAULT,
                                 &style->inc_button, in, font ) )
            val += step;

        bounds.x = bounds.x + button.w + style->spacing.x;
        bounds.w = bounds.w - ( 2 * button.w + 2 * style->spacing.x );
    }

    /* remove one cursor size to support visual cursor */
    bounds.x += style->cursor_size.x * 0.5f;
    bounds.w -= style->cursor_size.x;

    /* make sure the provided values are correct */
    slider_max = WORKPHONE_MAX( min, max );
    slider_min = WORKPHONE_MIN( min, max );
    slider_value = WORKPHONE_CLAMP( slider_min, val, slider_max );
    slider_range = slider_max - slider_min;
    slider_steps = slider_range / step;
    cursor_offset = ( slider_value - slider_min ) / step;

    /* calculate cursor
    Basically you have two cursors. One for visual representation and interaction
    and one for updating the actual cursor value. */
    logical_cursor.h = bounds.h;
    logical_cursor.w = bounds.w / slider_steps;
    logical_cursor.x = bounds.x + ( logical_cursor.w * cursor_offset );
    logical_cursor.y = bounds.y;

    visual_cursor.h = style->cursor_size.y;
    visual_cursor.w = style->cursor_size.x;
    visual_cursor.y = ( bounds.y + bounds.h * 0.5f ) - visual_cursor.h * 0.5f;
    visual_cursor.x = logical_cursor.x - visual_cursor.w * 0.5f;

    slider_value = wp_slider_behavior( state, &logical_cursor, &visual_cursor, in, bounds, slider_min,
                                       slider_max, slider_value, step, slider_steps );
    visual_cursor.x = logical_cursor.x - visual_cursor.w * 0.5f;

    /* draw slider */
    if( style->draw_begin )
        style->draw_begin( out, style->userdata );
    wp_draw_slider( out, *state, style, &bounds, &visual_cursor, slider_min, slider_value, slider_max );
    if( style->draw_end )
        style->draw_end( out, style->userdata );
    return slider_value;
}
WORKPHONE_API wp_bool wp_slider_float( struct wp_context *ctx, wp_f32 min_value, wp_f32 *value,
                                       wp_f32 max_value, wp_f32 value_step )
{
    struct wp_window *win;
    struct wp_panel *layout;
    struct wp_input *in;
    const struct wp_style *style;

    wp_s32 ret = 0;
    wp_f32 old_value;
    struct wp_rect bounds;
    enum wp_widget_layout_states state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    WORKPHONE_ASSERT( value );
    if( !ctx || !ctx->current || !ctx->current->layout || !value )
        return ret;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;

    state = wp_widget( &bounds, ctx );
    if( !state )
        return ret;
    in = ( /*state == WORKPHONE_WIDGET_ROM || */ state == WORKPHONE_WIDGET_DISABLED ||
           layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;

    old_value = *value;
    *value = wp_do_slider( &ctx->last_widget_state, &win->buffer, bounds, min_value, old_value,
                           max_value, value_step, &style->slider, in, style->font );
    return ( old_value > *value || old_value < *value );
}
WORKPHONE_API wp_f32 wp_slide_wp_f32( struct wp_context *ctx, wp_f32 min, wp_f32 val, wp_f32 max,
                                      wp_f32 step )
{
    wp_slider_float( ctx, min, &val, max, step );
    return val;
}
WORKPHONE_API wp_s32 wp_slide_int( struct wp_context *ctx, wp_s32 min, wp_s32 val, wp_s32 max,
                                   wp_s32 step )
{
    wp_f32 value = (wp_f32)val;
    wp_slider_float( ctx, (wp_f32)min, &value, (wp_f32)max, (wp_f32)step );
    return (wp_s32)value;
}
WORKPHONE_API wp_bool wp_slider_int( struct wp_context *ctx, wp_s32 min, wp_s32 *val, wp_s32 max,
                                     wp_s32 step )
{
    wp_s32 ret;
    wp_f32 value = (wp_f32)*val;
    ret = wp_slider_float( ctx, (wp_f32)min, &value, (wp_f32)max, (wp_f32)step );
    *val = (wp_s32)value;
    return ret;
}
