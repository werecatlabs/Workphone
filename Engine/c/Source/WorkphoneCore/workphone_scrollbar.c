#include "workphone.h"
#include "workphone_button.h"
#include "workphone_widget.h"

wp_f32 wp_scrollbar_behavior( wp_flags *state, struct wp_input *in, wp_s32 has_scrolling,
                              const struct wp_rect *scroll, const struct wp_rect *cursor,
                              const struct wp_rect *empty0, const struct wp_rect *empty1,
                              wp_f32 scroll_offset, wp_f32 target, wp_f32 scroll_step,
                              enum wp_orientation o )
{
    wp_flags ws = 0;
    wp_s32 left_mouse_down;
    wp_u32 left_mouse_clicked;
    wp_s32 left_mouse_click_in_cursor;
    wp_f32 scroll_delta;

    wp_widget_state_reset( state );
    if( !in )
        return scroll_offset;

    left_mouse_down = in->mouse.buttons[WORKPHONE_BUTTON_LEFT].down;
    left_mouse_clicked = in->mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked;
    left_mouse_click_in_cursor =
        wp_input_has_mouse_click_down_in_rect( in, WORKPHONE_BUTTON_LEFT, *cursor, wp_true );
    if( wp_input_is_mouse_hovering_rect( in, *scroll ) )
        *state = WORKPHONE_WIDGET_STATE_HOVERED;

    scroll_delta = ( o == WORKPHONE_VERTICAL ) ? in->mouse.scroll_delta.y : in->mouse.scroll_delta.x;
    if( left_mouse_down && left_mouse_click_in_cursor && !left_mouse_clicked )
    {
        /* update cursor by mouse dragging */
        wp_f32 pixel, delta;
        *state = WORKPHONE_WIDGET_STATE_ACTIVE;
        if( o == WORKPHONE_VERTICAL )
        {
            wp_f32 cursor_y;
            pixel = in->mouse.delta.y;
            delta = ( pixel / scroll->h ) * target;
            scroll_offset = WORKPHONE_CLAMP( 0, scroll_offset + delta, target - scroll->h );
            cursor_y = scroll->y + ( ( scroll_offset / target ) * scroll->h );
            in->mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked_pos.y = cursor_y + cursor->h / 2.0f;
        }
        else
        {
            wp_f32 cursor_x;
            pixel = in->mouse.delta.x;
            delta = ( pixel / scroll->w ) * target;
            scroll_offset = WORKPHONE_CLAMP( 0, scroll_offset + delta, target - scroll->w );
            cursor_x = scroll->x + ( ( scroll_offset / target ) * scroll->w );
            in->mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked_pos.x = cursor_x + cursor->w / 2.0f;
        }
    }
    else if( ( wp_input_is_key_pressed( in, WORKPHONE_KEY_SCROLL_UP ) && o == WORKPHONE_VERTICAL &&
               has_scrolling ) ||
             wp_button_behavior( &ws, *empty0, in, WORKPHONE_BUTTON_DEFAULT ) )
    {
        /* scroll page up by click on empty space or shortcut */
        if( o == WORKPHONE_VERTICAL )
            scroll_offset = WORKPHONE_MAX( 0, scroll_offset - scroll->h );
        else
            scroll_offset = WORKPHONE_MAX( 0, scroll_offset - scroll->w );
    }
    else if( ( wp_input_is_key_pressed( in, WORKPHONE_KEY_SCROLL_DOWN ) && o == WORKPHONE_VERTICAL &&
               has_scrolling ) ||
             wp_button_behavior( &ws, *empty1, in, WORKPHONE_BUTTON_DEFAULT ) )
    {
        /* scroll page down by click on empty space or shortcut */
        if( o == WORKPHONE_VERTICAL )
            scroll_offset = WORKPHONE_MIN( scroll_offset + scroll->h, target - scroll->h );
        else
            scroll_offset = WORKPHONE_MIN( scroll_offset + scroll->w, target - scroll->w );
    }
    else if( has_scrolling )
    {
        if( ( scroll_delta < 0 || ( scroll_delta > 0 ) ) )
        {
            /* update cursor by mouse scrolling */
            scroll_offset = scroll_offset + scroll_step * ( -scroll_delta );
            if( o == WORKPHONE_VERTICAL )
                scroll_offset = WORKPHONE_CLAMP( 0, scroll_offset, target - scroll->h );
            else
                scroll_offset = WORKPHONE_CLAMP( 0, scroll_offset, target - scroll->w );
        }
        else if( wp_input_is_key_pressed( in, WORKPHONE_KEY_SCROLL_START ) )
        {
            /* update cursor to the beginning  */
            if( o == WORKPHONE_VERTICAL )
                scroll_offset = 0;
        }
        else if( wp_input_is_key_pressed( in, WORKPHONE_KEY_SCROLL_END ) )
        {
            /* update cursor to the end */
            if( o == WORKPHONE_VERTICAL )
                scroll_offset = target - scroll->h;
        }
    }
    if( *state & WORKPHONE_WIDGET_STATE_HOVER && !wp_input_is_mouse_prev_hovering_rect( in, *scroll ) )
        *state |= WORKPHONE_WIDGET_STATE_ENTERED;
    else if( wp_input_is_mouse_prev_hovering_rect( in, *scroll ) )
        *state |= WORKPHONE_WIDGET_STATE_LEFT;
    return scroll_offset;
}
WORKPHONE_LIB void wp_draw_scrollbar( struct wp_command_buffer *out, wp_flags state,
                                      const struct wp_style_scrollbar *style,
                                      const struct wp_rect *bounds, const struct wp_rect *scroll )
{
    const struct wp_style_item *background;
    const struct wp_style_item *cursor;

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
        wp_draw_image( out, *bounds, &background->data.image, wp_white );
        break;
    case WORKPHONE_STYLE_ITEM_NINE_SLICE:
        wp_draw_nine_slice( out, *bounds, &background->data.slice, wp_white );
        break;
    case WORKPHONE_STYLE_ITEM_COLOR:
        wp_fill_rect( out, *bounds, style->rounding, background->data.color );
        wp_stroke_rect( out, *bounds, style->rounding, style->border, style->border_color );
        break;
    }

    /* draw cursor */
    switch( cursor->type )
    {
    case WORKPHONE_STYLE_ITEM_IMAGE:
        wp_draw_image( out, *scroll, &cursor->data.image, wp_white );
        break;
    case WORKPHONE_STYLE_ITEM_NINE_SLICE:
        wp_draw_nine_slice( out, *scroll, &cursor->data.slice, wp_white );
        break;
    case WORKPHONE_STYLE_ITEM_COLOR:
        wp_fill_rect( out, *scroll, style->rounding_cursor, cursor->data.color );
        wp_stroke_rect( out, *scroll, style->rounding_cursor, style->border_cursor,
                        style->cursor_border_color );
        break;
    }
}
WORKPHONE_LIB wp_f32 wp_do_scrollbarv( wp_flags *state, struct wp_command_buffer *out,
                                       struct wp_rect scroll, wp_s32 has_scrolling, wp_f32 offset,
                                       wp_f32 target, wp_f32 step, wp_f32 button_pixel_inc,
                                       const struct wp_style_scrollbar *style, struct wp_input *in,
                                       const struct wp_user_font *font )
{
    struct wp_rect empty_north;
    struct wp_rect empty_south;
    struct wp_rect cursor;

    wp_f32 scroll_step;
    wp_f32 scroll_offset;
    wp_f32 scroll_off;
    wp_f32 scroll_ratio;

    WORKPHONE_ASSERT( out );
    WORKPHONE_ASSERT( style );
    WORKPHONE_ASSERT( state );
    if( !out || !style )
        return 0;

    scroll.w = WORKPHONE_MAX( scroll.w, 1 );
    scroll.h = WORKPHONE_MAX( scroll.h, 0 );
    if( target <= scroll.h )
        return 0;

    /* optional scrollbar buttons */
    if( style->show_buttons )
    {
        wp_flags ws;
        wp_f32 scroll_h;
        struct wp_rect button;

        button.x = scroll.x;
        button.w = scroll.w;
        button.h = scroll.w;

        scroll_h = WORKPHONE_MAX( scroll.h - 2 * button.h, 0 );
        scroll_step = WORKPHONE_MIN( step, button_pixel_inc );

        /* decrement button */
        button.y = scroll.y;
        if( wp_do_button_symbol( &ws, out, button, style->dec_symbol, WORKPHONE_BUTTON_REPEATER,
                                 &style->dec_button, in, font ) )
            offset = offset - scroll_step;

        /* increment button */
        button.y = scroll.y + scroll.h - button.h;
        if( wp_do_button_symbol( &ws, out, button, style->inc_symbol, WORKPHONE_BUTTON_REPEATER,
                                 &style->inc_button, in, font ) )
            offset = offset + scroll_step;

        scroll.y = scroll.y + button.h;
        scroll.h = scroll_h;
    }

    /* calculate scrollbar constants */
    scroll_step = WORKPHONE_MIN( step, scroll.h );
    scroll_offset = WORKPHONE_CLAMP( 0, offset, target - scroll.h );
    scroll_ratio = scroll.h / target;
    scroll_off = scroll_offset / target;

    /* calculate scrollbar cursor bounds */
    cursor.h =
        WORKPHONE_MAX( ( scroll_ratio * scroll.h ) - ( 2 * style->border + 2 * style->padding.y ), 0 );
    cursor.y = scroll.y + ( scroll_off * scroll.h ) + style->border + style->padding.y;
    cursor.w = scroll.w - ( 2 * style->border + 2 * style->padding.x );
    cursor.x = scroll.x + style->border + style->padding.x;

    /* calculate empty space around cursor */
    empty_north.x = scroll.x;
    empty_north.y = scroll.y;
    empty_north.w = scroll.w;
    empty_north.h = WORKPHONE_MAX( cursor.y - scroll.y, 0 );

    empty_south.x = scroll.x;
    empty_south.y = cursor.y + cursor.h;
    empty_south.w = scroll.w;
    empty_south.h = WORKPHONE_MAX( ( scroll.y + scroll.h ) - ( cursor.y + cursor.h ), 0 );

    /* update scrollbar */
    scroll_offset =
        wp_scrollbar_behavior( state, in, has_scrolling, &scroll, &cursor, &empty_north, &empty_south,
                               scroll_offset, target, scroll_step, WORKPHONE_VERTICAL );
    scroll_off = scroll_offset / target;
    cursor.y = scroll.y + ( scroll_off * scroll.h ) + style->border_cursor + style->padding.y;

    /* draw scrollbar */
    if( style->draw_begin )
        style->draw_begin( out, style->userdata );
    wp_draw_scrollbar( out, *state, style, &scroll, &cursor );
    if( style->draw_end )
        style->draw_end( out, style->userdata );
    return scroll_offset;
}
WORKPHONE_LIB wp_f32 wp_do_scrollbarh( wp_flags *state, struct wp_command_buffer *out,
                                       struct wp_rect scroll, wp_s32 has_scrolling, wp_f32 offset,
                                       wp_f32 target, wp_f32 step, wp_f32 button_pixel_inc,
                                       const struct wp_style_scrollbar *style, struct wp_input *in,
                                       const struct wp_user_font *font )
{
    struct wp_rect cursor;
    struct wp_rect empty_west;
    struct wp_rect empty_east;

    wp_f32 scroll_step;
    wp_f32 scroll_offset;
    wp_f32 scroll_off;
    wp_f32 scroll_ratio;

    WORKPHONE_ASSERT( out );
    WORKPHONE_ASSERT( style );
    if( !out || !style )
        return 0;

    /* scrollbar background */
    scroll.h = WORKPHONE_MAX( scroll.h, 1 );
    scroll.w = WORKPHONE_MAX( scroll.w, 2 * scroll.h );
    if( target <= scroll.w )
        return 0;

    /* optional scrollbar buttons */
    if( style->show_buttons )
    {
        wp_flags ws;
        wp_f32 scroll_w;
        struct wp_rect button;
        button.y = scroll.y;
        button.w = scroll.h;
        button.h = scroll.h;

        scroll_w = scroll.w - 2 * button.w;
        scroll_step = WORKPHONE_MIN( step, button_pixel_inc );

        /* decrement button */
        button.x = scroll.x;
        if( wp_do_button_symbol( &ws, out, button, style->dec_symbol, WORKPHONE_BUTTON_REPEATER,
                                 &style->dec_button, in, font ) )
            offset = offset - scroll_step;

        /* increment button */
        button.x = scroll.x + scroll.w - button.w;
        if( wp_do_button_symbol( &ws, out, button, style->inc_symbol, WORKPHONE_BUTTON_REPEATER,
                                 &style->inc_button, in, font ) )
            offset = offset + scroll_step;

        scroll.x = scroll.x + button.w;
        scroll.w = scroll_w;
    }

    /* calculate scrollbar constants */
    scroll_step = WORKPHONE_MIN( step, scroll.w );
    scroll_offset = WORKPHONE_CLAMP( 0, offset, target - scroll.w );
    scroll_ratio = scroll.w / target;
    scroll_off = scroll_offset / target;

    /* calculate cursor bounds */
    cursor.w = ( scroll_ratio * scroll.w ) - ( 2 * style->border + 2 * style->padding.x );
    cursor.x = scroll.x + ( scroll_off * scroll.w ) + style->border + style->padding.x;
    cursor.h = scroll.h - ( 2 * style->border + 2 * style->padding.y );
    cursor.y = scroll.y + style->border + style->padding.y;

    /* calculate empty space around cursor */
    empty_west.x = scroll.x;
    empty_west.y = scroll.y;
    empty_west.w = cursor.x - scroll.x;
    empty_west.h = scroll.h;

    empty_east.x = cursor.x + cursor.w;
    empty_east.y = scroll.y;
    empty_east.w = ( scroll.x + scroll.w ) - ( cursor.x + cursor.w );
    empty_east.h = scroll.h;

    /* update scrollbar */
    scroll_offset =
        wp_scrollbar_behavior( state, in, has_scrolling, &scroll, &cursor, &empty_west, &empty_east,
                               scroll_offset, target, scroll_step, WORKPHONE_HORIZONTAL );
    scroll_off = scroll_offset / target;
    cursor.x = scroll.x + ( scroll_off * scroll.w );

    /* draw scrollbar */
    if( style->draw_begin )
        style->draw_begin( out, style->userdata );
    wp_draw_scrollbar( out, *state, style, &scroll, &cursor );
    if( style->draw_end )
        style->draw_end( out, style->userdata );
    return scroll_offset;
}
