#include "workphone.h"
#include "workphone_widget.h"

wp_size wp_progress_behavior( wp_flags *state, struct wp_input *in, struct wp_rect r,
                              struct wp_rect cursor, wp_size max, wp_size value, wp_bool modifiable )
{
    wp_s32 left_mouse_down = 0;
    wp_s32 left_mouse_click_in_cursor = 0;

    wp_widget_state_reset( state );
    if( !in || !modifiable )
        return value;
    left_mouse_down = in && in->mouse.buttons[WORKPHONE_BUTTON_LEFT].down;
    left_mouse_click_in_cursor =
        in && wp_input_has_mouse_click_down_in_rect( in, WORKPHONE_BUTTON_LEFT, cursor, wp_true );
    if( wp_input_is_mouse_hovering_rect( in, r ) )
        *state = WORKPHONE_WIDGET_STATE_HOVERED;

    if( in && left_mouse_down && left_mouse_click_in_cursor )
    {
        if( left_mouse_down && left_mouse_click_in_cursor )
        {
            wp_f32 ratio = WORKPHONE_MAX( 0, (wp_f32)( in->mouse.pos.x - cursor.x ) ) / (wp_f32)cursor.w;
            value = (wp_size)WORKPHONE_CLAMP( 0, (wp_f32)max * ratio, (wp_f32)max );
            in->mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked_pos.x = cursor.x + cursor.w / 2.0f;
            *state |= WORKPHONE_WIDGET_STATE_ACTIVE;
        }
    }
    /* set progressbar widget state */
    if( *state & WORKPHONE_WIDGET_STATE_HOVER && !wp_input_is_mouse_prev_hovering_rect( in, r ) )
        *state |= WORKPHONE_WIDGET_STATE_ENTERED;
    else if( wp_input_is_mouse_prev_hovering_rect( in, r ) )
        *state |= WORKPHONE_WIDGET_STATE_LEFT;
    return value;
}
WORKPHONE_LIB void wp_draw_progress( struct wp_command_buffer *out, wp_flags state,
                                     const struct wp_style_progress *style, const struct wp_rect *bounds,
                                     const struct wp_rect *scursor, wp_size value, wp_size max )
{
    const struct wp_style_item *background;
    const struct wp_style_item *cursor;

    WORKPHONE_UNUSED( max );
    WORKPHONE_UNUSED( value );

    /* select correct colors/images to draw */
    if( state & WORKPHONE_WIDGET_STATE_ACTIVED )
    {
        background = &style->active;
        cursor = &style->cursor_active;
    }
    else if( state & WORKPHONE_WIDGET_STATE_HOVER )
    {
        background = &style->hover;
        cursor = &style->cursor_hover;
    }
    else
    {
        background = &style->normal;
        cursor = &style->cursor_normal;
    }

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

    /* draw cursor */
    switch( cursor->type )
    {
    case WORKPHONE_STYLE_ITEM_IMAGE:
        wp_draw_image( out, *scursor, &cursor->data.image,
                       wp_rgb_factor( wp_white, style->color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_NINE_SLICE:
        wp_draw_nine_slice( out, *scursor, &cursor->data.slice,
                            wp_rgb_factor( wp_white, style->color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_COLOR:
        wp_fill_rect( out, *scursor, style->rounding,
                      wp_rgb_factor( cursor->data.color, style->color_factor ) );
        wp_stroke_rect( out, *scursor, style->rounding, style->border,
                        wp_rgb_factor( style->border_color, style->color_factor ) );
        break;
    }
}
WORKPHONE_LIB wp_size wp_do_progress( wp_flags *state, struct wp_command_buffer *out,
                                      struct wp_rect bounds, wp_size value, wp_size max,
                                      wp_bool modifiable, const struct wp_style_progress *style,
                                      struct wp_input *in )
{
    wp_f32 prog_scale;
    wp_size prog_value;
    struct wp_rect cursor;

    WORKPHONE_ASSERT( style );
    WORKPHONE_ASSERT( out );
    if( !out || !style )
        return 0;

    /* calculate progressbar cursor */
    cursor.w = WORKPHONE_MAX( bounds.w, 2 * style->padding.x + 2 * style->border );
    cursor.h = WORKPHONE_MAX( bounds.h, 2 * style->padding.y + 2 * style->border );
    cursor = wp_pad_rect(
        bounds, wp_make_vec2f( style->padding.x + style->border, style->padding.y + style->border ) );
    prog_scale = (wp_f32)value / (wp_f32)max;

    /* update progressbar */
    prog_value = WORKPHONE_MIN( value, max );
    prog_value = wp_progress_behavior( state, in, bounds, cursor, max, prog_value, modifiable );
    cursor.w = cursor.w * prog_scale;

    /* draw progressbar */
    if( style->draw_begin )
        style->draw_begin( out, style->userdata );
    wp_draw_progress( out, *state, style, &bounds, &cursor, value, max );
    if( style->draw_end )
        style->draw_end( out, style->userdata );
    return prog_value;
}
WORKPHONE_API wp_bool wp_progress( struct wp_context *ctx, wp_size *cur, wp_size max,
                                   wp_bool is_modifyable )
{
    struct wp_window *win;
    struct wp_panel *layout;
    const struct wp_style *style;
    struct wp_input *in;

    struct wp_rect bounds;
    enum wp_widget_layout_states state;
    wp_size old_value;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( cur );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout || !cur )
        return 0;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;
    state = wp_widget( &bounds, ctx );
    if( !state )
        return 0;

    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;
    old_value = *cur;
    *cur = wp_do_progress( &ctx->last_widget_state, &win->buffer, bounds, *cur, max, is_modifyable,
                           &style->progress, in );
    return ( *cur != old_value );
}
WORKPHONE_API wp_size wp_prog( struct wp_context *ctx, wp_size cur, wp_size max, wp_bool modifyable )
{
    wp_progress( ctx, &cur, max, modifyable );
    return cur;
}
