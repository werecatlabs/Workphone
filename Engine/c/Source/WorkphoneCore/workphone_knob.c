#include "workphone.h"
#include "workphone_math.h"
#include "workphone_widget.h"

wp_f32 wp_knob_behavior( wp_flags *state, struct wp_input *in, struct wp_rect bounds, wp_f32 knob_min,
                         wp_f32 knob_max, wp_f32 knob_value, wp_f32 knob_step, wp_f32 knob_steps,
                         enum wp_heading zero_direction, wp_f32 dead_zone_percent )
{
    struct wp_vec2f origin;
    wp_f32 angle = 0.0f;
    origin.x = bounds.x + ( bounds.w / 2 );
    origin.y = bounds.y + ( bounds.h / 2 );

    wp_widget_state_reset( state );

    /* handle click and drag input */
    if( in && in->mouse.buttons[WORKPHONE_BUTTON_LEFT].down &&
        wp_input_has_mouse_click_down_in_rect( in, WORKPHONE_BUTTON_LEFT, bounds, wp_true ) )
    {
        /* calculate angle from origin and rotate */
        const wp_f32 direction_rads[4] = {
            WORKPHONE_PI * 2.5f, /* 90  WORKPHONE_UP */
            WORKPHONE_PI * 2.0f, /* 0   WORKPHONE_RIGHT */
            WORKPHONE_PI * 1.5f, /* 270 WORKPHONE_DOWN */
            WORKPHONE_PI,        /* 180 WORKPHONE_LEFT */
        };
        *state = WORKPHONE_WIDGET_STATE_ACTIVE;

        angle = wp_atan2( in->mouse.pos.y - origin.y, in->mouse.pos.x - origin.x ) +
                direction_rads[zero_direction];
        angle -= ( angle > WORKPHONE_PI * 2 ) ? WORKPHONE_PI * 3 : WORKPHONE_PI;

        /* account for dead space applied when drawing */
        angle *= 1.0f / ( 1.0f - dead_zone_percent );
        angle = wp_clampf( -WORKPHONE_PI, angle, WORKPHONE_PI );

        /* convert -pi -> pi range to 0.0 -> 1.0 */
        angle = ( angle + WORKPHONE_PI ) / ( WORKPHONE_PI * 2 );

        /* click to closest step */
        knob_value = knob_min + ( (wp_s32)( angle * knob_steps + ( knob_step / 2 ) ) ) * knob_step;
        knob_value = wp_clampf( knob_min, knob_value, knob_max );
    }

    /* knob widget state */
    if( wp_input_is_mouse_hovering_rect( in, bounds ) )
    {
        *state = WORKPHONE_WIDGET_STATE_HOVERED;
        /* handle scroll and arrow inputs */
        if( in->mouse.scroll_delta.y > 0 ||
            ( in->keyboard.keys[WORKPHONE_KEY_UP].down && in->keyboard.keys[WORKPHONE_KEY_UP].clicked ) )
        {
            knob_value += knob_step;
        }

        if( in->mouse.scroll_delta.y < 0 || ( in->keyboard.keys[WORKPHONE_KEY_DOWN].down &&
                                              in->keyboard.keys[WORKPHONE_KEY_DOWN].clicked ) )
        {
            knob_value -= knob_step;
        }
        /* easiest way to disable scrolling of parent panels..knob eats scrolling */
        in->mouse.scroll_delta.y = 0;
        knob_value = WORKPHONE_CLAMP( knob_min, knob_value, knob_max );
    }
    if( *state & WORKPHONE_WIDGET_STATE_HOVER && !wp_input_is_mouse_prev_hovering_rect( in, bounds ) )
        *state |= WORKPHONE_WIDGET_STATE_ENTERED;
    else if( wp_input_is_mouse_prev_hovering_rect( in, bounds ) )
        *state |= WORKPHONE_WIDGET_STATE_LEFT;

    return knob_value;
}
WORKPHONE_LIB void wp_draw_knob( struct wp_command_buffer *out, wp_flags state,
                                 const struct wp_style_knob *style, const struct wp_rect *bounds,
                                 wp_f32 min, wp_f32 value, wp_f32 max, enum wp_heading zero_direction,
                                 wp_f32 dead_zone_percent )
{
    const struct wp_style_item *background;
    struct wp_color knob_color, cursor;

    WORKPHONE_UNUSED( min );
    WORKPHONE_UNUSED( max );
    WORKPHONE_UNUSED( value );

    if( state & WORKPHONE_WIDGET_STATE_ACTIVED )
    {
        background = &style->active;
        knob_color = style->knob_active;
        cursor = style->cursor_active;
    }
    else if( state & WORKPHONE_WIDGET_STATE_HOVER )
    {
        background = &style->hover;
        knob_color = style->knob_hover;
        cursor = style->cursor_hover;
    }
    else
    {
        background = &style->normal;
        knob_color = style->knob_normal;
        cursor = style->cursor_normal;
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
        wp_fill_rect( out, *bounds, 0, wp_rgb_factor( background->data.color, style->color_factor ) );
        wp_stroke_rect( out, *bounds, 0, style->border,
                        wp_rgb_factor( style->border_color, style->color_factor ) );
        break;
    }

    /* draw knob */
    wp_fill_circle( out, *bounds, wp_rgb_factor( knob_color, style->color_factor ) );
    if( style->knob_border > 0 )
    {
        struct wp_rect border_bounds = *bounds;
        border_bounds.x += style->knob_border / 2;
        border_bounds.y += style->knob_border / 2;
        border_bounds.w -= style->knob_border;
        border_bounds.h -= style->knob_border;
        wp_stroke_circle( out, border_bounds, style->knob_border,
                          wp_rgb_factor( style->knob_border_color, style->color_factor ) );
    }
    { /* calculate cursor line cords */
        wp_f32 half_circle_size = ( bounds->w / 2 );
        wp_f32 angle = ( value - min ) / ( max - min );
        wp_f32 alive_zone = 1.0f - dead_zone_percent;
        struct wp_vec2f cursor_start, cursor_end;
        const wp_f32 direction_rads[4] = {
            WORKPHONE_PI * 1.5f, /* 90  WORKPHONE_UP */
            0.0f,                /* 0   WORKPHONE_RIGHT */
            WORKPHONE_PI * 0.5f, /* 270 WORKPHONE_DOWN */
            WORKPHONE_PI,        /* 180 WORKPHONE_LEFT */
        };
        /* calculate + apply dead zone */
        angle = ( angle * alive_zone ) + ( dead_zone_percent / 2 );

        /* percentage 0.0 -> 1.0 to radians, rads are 0.0 to (2*pi) NOT -pi to pi */
        angle *= WORKPHONE_PI * 2.0f;

        /* apply zero angle */
        angle += direction_rads[zero_direction];
        if( angle > WORKPHONE_PI * 2.0f )
            angle -= WORKPHONE_PI * 2.0f;

        cursor_start.x = bounds->x + half_circle_size + ( angle > WORKPHONE_PI );
        cursor_start.y = bounds->y + half_circle_size +
                         ( angle < WORKPHONE_PI_HALF || angle > ( WORKPHONE_PI * 1.5f ) );

        cursor_end.x = cursor_start.x + ( half_circle_size * wp_cos( angle ) );
        cursor_end.y = cursor_start.y + ( half_circle_size * wp_sin( angle ) );

        /* cut off half of the cursor */
        cursor_start.x = ( cursor_start.x + cursor_end.x ) / 2;
        cursor_start.y = ( cursor_start.y + cursor_end.y ) / 2;

        /* draw cursor */
        wp_stroke_line( out, cursor_start.x, cursor_start.y, cursor_end.x, cursor_end.y, 2,
                        wp_rgb_factor( cursor, style->color_factor ) );
    }
}
WORKPHONE_LIB wp_f32 wp_do_knob( wp_flags *state, struct wp_command_buffer *out, struct wp_rect bounds,
                                 wp_f32 min, wp_f32 val, wp_f32 max, wp_f32 step,
                                 enum wp_heading zero_direction, wp_f32 dead_zone_percent,
                                 const struct wp_style_knob *style, struct wp_input *in )
{
    wp_f32 knob_range;
    wp_f32 knob_min;
    wp_f32 knob_max;
    wp_f32 knob_value;
    wp_f32 knob_steps;

    WORKPHONE_ASSERT( style );
    WORKPHONE_ASSERT( out );
    if( !out || !style )
        return 0;

    /* remove padding from knob bounds */
    bounds.y = bounds.y + style->padding.y;
    bounds.x = bounds.x + style->padding.x;
    bounds.h = WORKPHONE_MAX( bounds.h, 2 * style->padding.y );
    bounds.w = WORKPHONE_MAX( bounds.w, 2 * style->padding.x );
    bounds.w -= 2 * style->padding.x;
    bounds.h -= 2 * style->padding.y;
    if( bounds.h < bounds.w )
    {
        bounds.x += ( bounds.w - bounds.h ) / 2;
        bounds.w = bounds.h;
    }

    /* make sure the provided values are correct */
    knob_max = WORKPHONE_MAX( min, max );
    knob_min = WORKPHONE_MIN( min, max );
    knob_value = WORKPHONE_CLAMP( knob_min, val, knob_max );
    knob_range = knob_max - knob_min;
    knob_steps = knob_range / step;

    knob_value = wp_knob_behavior( state, in, bounds, knob_min, knob_max, knob_value, step, knob_steps,
                                   zero_direction, dead_zone_percent );

    /* draw knob */
    if( style->draw_begin )
        style->draw_begin( out, style->userdata );
    wp_draw_knob( out, *state, style, &bounds, knob_min, knob_value, knob_max, zero_direction,
                  dead_zone_percent );
    if( style->draw_end )
        style->draw_end( out, style->userdata );
    return knob_value;
}
WORKPHONE_API wp_bool wp_knob_wp_f32( struct wp_context *ctx, wp_f32 min_value, wp_f32 *value,
                                      wp_f32 max_value, wp_f32 value_step,
                                      enum wp_heading zero_direction, wp_f32 dead_zone_degrees )
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
    WORKPHONE_ASSERT( WORKPHONE_BETWEEN( dead_zone_degrees, 0.0f, 360.0f ) );
    if( !ctx || !ctx->current || !ctx->current->layout || !value )
        return ret;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;

    state = wp_widget( &bounds, ctx );
    if( !state )
        return ret;
    in =
        ( state == WORKPHONE_WIDGET_DISABLED || layout->flags & WORKPHONE_WINDOW_ROM ) ? 0 : &ctx->input;

    old_value = *value;
    *value = wp_do_knob( &ctx->last_widget_state, &win->buffer, bounds, min_value, old_value, max_value,
                         value_step, zero_direction, dead_zone_degrees / 360.0f, &style->knob, in );

    return ( old_value > *value || old_value < *value );
}
WORKPHONE_API wp_bool wp_knob_int( struct wp_context *ctx, wp_s32 min, wp_s32 *val, wp_s32 max,
                                   wp_s32 step, enum wp_heading zero_direction,
                                   wp_f32 dead_zone_degrees )
{
    wp_s32 ret;
    wp_f32 value = (wp_f32)*val;
    ret = wp_knob_wp_f32( ctx, (wp_f32)min, &value, (wp_f32)max, (wp_f32)step, zero_direction,
                          dead_zone_degrees );
    *val = (wp_s32)value;
    return ret;
}
