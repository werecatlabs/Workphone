#include "workphone_button.h"
#include "workphone_command_buffer.h"
#include "workphone_context.h"
#include "workphone_font.h"
#include "workphone_style.h"
#include "workphone_text.h"
#include "workphone_widget.h"

void wp_draw_symbol( struct wp_command_buffer *out, enum wp_symbol_type type, struct wp_rect content,
                     struct wp_color background, struct wp_color foreground, wp_f32 border_width,
                     const struct wp_user_font *font )
{
    switch( type )
    {
    case WORKPHONE_SYMBOL_X:
    case WORKPHONE_SYMBOL_UNDERSCORE:
    case WORKPHONE_SYMBOL_PLUS:
    case WORKPHONE_SYMBOL_MINUS:
    {
        /* single wp_c8acter text symbol */
        const wp_c8 *X = ( type == WORKPHONE_SYMBOL_X )            ? "x"
                         : ( type == WORKPHONE_SYMBOL_UNDERSCORE ) ? "_"
                         : ( type == WORKPHONE_SYMBOL_PLUS )       ? "+"
                                                                   : "-";
        struct wp_text text;
        text.padding = wp_make_vec2f( 0, 0 );
        text.background = background;
        text.text = foreground;
        wp_widget_text( out, content, X, 1, &text, WORKPHONE_TEXT_CENTERED, font );
    }
    break;
    case WORKPHONE_SYMBOL_CIRCLE_SOLID:
    case WORKPHONE_SYMBOL_CIRCLE_OUTLINE:
    case WORKPHONE_SYMBOL_RECT_SOLID:
    case WORKPHONE_SYMBOL_RECT_OUTLINE:
    {
        /* simple empty/filled shapes */
        if( type == WORKPHONE_SYMBOL_RECT_SOLID || type == WORKPHONE_SYMBOL_RECT_OUTLINE )
        {
            wp_fill_rect( out, content, 0, foreground );
            if( type == WORKPHONE_SYMBOL_RECT_OUTLINE )
                wp_fill_rect( out, wp_shrink_rect( content, border_width ), 0, background );
        }
        else
        {
            wp_fill_circle( out, content, foreground );
            if( type == WORKPHONE_SYMBOL_CIRCLE_OUTLINE )
                wp_fill_circle( out, wp_shrink_rect( content, 1 ), background );
        }
    }
    break;
    case WORKPHONE_SYMBOL_TRIANGLE_UP:
    case WORKPHONE_SYMBOL_TRIANGLE_DOWN:
    case WORKPHONE_SYMBOL_TRIANGLE_LEFT:
    case WORKPHONE_SYMBOL_TRIANGLE_RIGHT:
    {
        enum wp_heading heading;
        struct wp_vec2f points[3];
        heading = ( type == WORKPHONE_SYMBOL_TRIANGLE_RIGHT )  ? WORKPHONE_RIGHT
                  : ( type == WORKPHONE_SYMBOL_TRIANGLE_LEFT ) ? WORKPHONE_LEFT
                  : ( type == WORKPHONE_SYMBOL_TRIANGLE_UP )   ? WORKPHONE_UP
                                                               : WORKPHONE_DOWN;
        wp_triangle_from_direction( points, content, 0, 0, heading );
        wp_fill_triangle( out, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x,
                          points[2].y, foreground );
    }
    break;
    case WORKPHONE_SYMBOL_TRIANGLE_UP_OUTLINE:
    case WORKPHONE_SYMBOL_TRIANGLE_DOWN_OUTLINE:
    case WORKPHONE_SYMBOL_TRIANGLE_LEFT_OUTLINE:
    case WORKPHONE_SYMBOL_TRIANGLE_RIGHT_OUTLINE:
    {
        enum wp_heading heading;
        struct wp_vec2f points[3];
        heading = ( type == WORKPHONE_SYMBOL_TRIANGLE_RIGHT_OUTLINE )  ? WORKPHONE_RIGHT
                  : ( type == WORKPHONE_SYMBOL_TRIANGLE_LEFT_OUTLINE ) ? WORKPHONE_LEFT
                  : ( type == WORKPHONE_SYMBOL_TRIANGLE_UP_OUTLINE )   ? WORKPHONE_UP
                                                                       : WORKPHONE_DOWN;
        wp_triangle_from_direction( points, content, 0, 0, heading );
        wp_stroke_triangle( out, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x,
                            points[2].y, border_width, foreground );
    }
    break;
    default:
    case WORKPHONE_SYMBOL_NONE:
    case WORKPHONE_SYMBOL_MAX:
        break;
    }
}

wp_bool wp_button_behavior( wp_flags *state, struct wp_rect r, const struct wp_input *i,
                            wp_button_behavior_enum behavior )
{
    wp_s32 ret = 0;
    wp_widget_state_reset( state );
    if( !i )
        return 0;
    if( wp_input_is_mouse_hovering_rect( i, r ) )
    {
        *state = WORKPHONE_WIDGET_STATE_HOVER;
        if( wp_input_is_mouse_down( i, WORKPHONE_BUTTON_LEFT ) )
            *state = WORKPHONE_WIDGET_STATE_ACTIVE;
        if( wp_input_has_mouse_click_in_button_rect( i, WORKPHONE_BUTTON_LEFT, r ) )
        {
            ret = ( behavior != WORKPHONE_BUTTON_DEFAULT )
                      ? wp_input_is_mouse_down( i, WORKPHONE_BUTTON_LEFT )
                      :
#ifdef WORKPHONE_BUTTON_TRIGGER_ON_RELEASE
                      wp_input_is_mouse_released( i, WORKPHONE_BUTTON_LEFT );
#else
                      wp_input_is_mouse_pressed( i, WORKPHONE_BUTTON_LEFT );
#endif
        }
    }
    if( *state & WORKPHONE_WIDGET_STATE_HOVER && !wp_input_is_mouse_prev_hovering_rect( i, r ) )
        *state |= WORKPHONE_WIDGET_STATE_ENTERED;
    else if( wp_input_is_mouse_prev_hovering_rect( i, r ) )
        *state |= WORKPHONE_WIDGET_STATE_LEFT;
    return ret;
}

const struct wp_style_item *wp_draw_button( struct wp_command_buffer *out, const struct wp_rect *bounds,
                                            wp_flags state, const struct wp_style_button *style )
{
    const struct wp_style_item *background;
    if( state & WORKPHONE_WIDGET_STATE_HOVER )
        background = &style->hover;
    else if( state & WORKPHONE_WIDGET_STATE_ACTIVED )
        background = &style->active;
    else
        background = &style->normal;

    switch( background->type )
    {
    case WORKPHONE_STYLE_ITEM_IMAGE:
        wp_draw_image( out, *bounds, &background->data.image,
                       wp_rgb_factor( wp_white, style->color_factor_background ) );
        break;
    case WORKPHONE_STYLE_ITEM_NINE_SLICE:
        wp_draw_nine_slice( out, *bounds, &background->data.slice,
                            wp_rgb_factor( wp_white, style->color_factor_background ) );
        break;
    case WORKPHONE_STYLE_ITEM_COLOR:
        wp_fill_rect( out, *bounds, style->rounding,
                      wp_rgb_factor( background->data.color, style->color_factor_background ) );
        wp_stroke_rect( out, *bounds, style->rounding, style->border,
                        wp_rgb_factor( style->border_color, style->color_factor_background ) );
        break;
    }
    return background;
}
WORKPHONE_LIB wp_bool wp_do_button( wp_flags *state, struct wp_command_buffer *out, struct wp_rect r,
                                    const struct wp_style_button *style, const struct wp_input *in,
                                    enum wp_button_behavior behavior, struct wp_rect *content )
{
    struct wp_rect bounds;
    WORKPHONE_ASSERT( style );
    WORKPHONE_ASSERT( state );
    WORKPHONE_ASSERT( out );
    if( !out || !style )
        return wp_false;

    /* calculate button content space */
    content->x = r.x + style->padding.x + style->border + style->rounding;
    content->y = r.y + style->padding.y + style->border + style->rounding;
    content->w = r.w - ( 2 * ( style->padding.x + style->border + style->rounding ) );
    content->h = r.h - ( 2 * ( style->padding.y + style->border + style->rounding ) );

    /* execute button behavior */
    bounds.x = r.x - style->touch_padding.x;
    bounds.y = r.y - style->touch_padding.y;
    bounds.w = r.w + 2 * style->touch_padding.x;
    bounds.h = r.h + 2 * style->touch_padding.y;
    return wp_button_behavior( state, bounds, in, behavior );
}

void wp_draw_button_text( struct wp_command_buffer *out, const struct wp_rect *bounds,
                          const struct wp_rect *content, wp_flags state,
                          const struct wp_style_button *style, const wp_c8 *txt, wp_s32 len,
                          wp_flags text_alignment, const struct wp_user_font *font )
{
    struct wp_text text;
    const struct wp_style_item *background;
    background = wp_draw_button( out, bounds, state, style );

    /* select correct colors/images */
    if( background->type == WORKPHONE_STYLE_ITEM_COLOR )
        text.background = background->data.color;
    else
        text.background = style->text_background;
    if( state & WORKPHONE_WIDGET_STATE_HOVER )
        text.text = style->text_hover;
    else if( state & WORKPHONE_WIDGET_STATE_ACTIVED )
        text.text = style->text_active;
    else
        text.text = style->text_normal;

    text.text = wp_rgb_factor( text.text, style->color_factor_text );

    text.padding = wp_make_vec2f( 0, 0 );
    wp_widget_text( out, *content, txt, len, &text, text_alignment, font );
}

wp_bool wp_do_button_text( wp_flags *state, struct wp_command_buffer *out, struct wp_rect bounds,
                           const wp_c8 *string, wp_s32 len, wp_flags align,
                           enum wp_button_behavior behavior, const struct wp_style_button *style,
                           const struct wp_input *in, const struct wp_user_font *font )
{
    struct wp_rect content;
    wp_s32 ret = wp_false;

    WORKPHONE_ASSERT( state );
    WORKPHONE_ASSERT( style );
    WORKPHONE_ASSERT( out );
    WORKPHONE_ASSERT( string );
    WORKPHONE_ASSERT( font );
    if( !out || !style || !font || !string )
        return wp_false;

    ret = wp_do_button( state, out, bounds, style, in, behavior, &content );
    if( style->draw_begin )
        style->draw_begin( out, style->userdata );
    wp_draw_button_text( out, &bounds, &content, *state, style, string, len, align, font );
    if( style->draw_end )
        style->draw_end( out, style->userdata );
    return ret;
}

void wp_draw_button_symbol( struct wp_command_buffer *out, const struct wp_rect *bounds,
                            const struct wp_rect *content, wp_flags state,
                            const struct wp_style_button *style, enum wp_symbol_type type,
                            const struct wp_user_font *font )
{
    struct wp_color sym, bg;
    const struct wp_style_item *background;

    /* select correct colors/images */
    background = wp_draw_button( out, bounds, state, style );
    if( background->type == WORKPHONE_STYLE_ITEM_COLOR )
        bg = background->data.color;
    else
        bg = style->text_background;

    if( state & WORKPHONE_WIDGET_STATE_HOVER )
        sym = style->text_hover;
    else if( state & WORKPHONE_WIDGET_STATE_ACTIVED )
        sym = style->text_active;
    else
        sym = style->text_normal;

    sym = wp_rgb_factor( sym, style->color_factor_text );
    wp_draw_symbol( out, type, *content, bg, sym, 1, font );
}

wp_bool wp_do_button_symbol( wp_flags *state, struct wp_command_buffer *out, struct wp_rect bounds,
                             enum wp_symbol_type symbol, enum wp_button_behavior behavior,
                             const struct wp_style_button *style, const struct wp_input *in,
                             const struct wp_user_font *font )
{
    wp_s32 ret;
    struct wp_rect content;

    WORKPHONE_ASSERT( state );
    WORKPHONE_ASSERT( style );
    WORKPHONE_ASSERT( font );
    WORKPHONE_ASSERT( out );
    if( !out || !style || !font || !state )
        return wp_false;

    ret = wp_do_button( state, out, bounds, style, in, behavior, &content );
    if( style->draw_begin )
        style->draw_begin( out, style->userdata );
    wp_draw_button_symbol( out, &bounds, &content, *state, style, symbol, font );
    if( style->draw_end )
        style->draw_end( out, style->userdata );
    return ret;
}

void wp_draw_button_image( struct wp_command_buffer *out, const struct wp_rect *bounds,
                           const struct wp_rect *content, wp_flags state,
                           const struct wp_style_button *style, const struct wp_image *img )
{
    wp_draw_button( out, bounds, state, style );
    wp_draw_image( out, *content, img, wp_rgb_factor( wp_white, style->color_factor_background ) );
}

wp_bool wp_do_button_image( wp_flags *state, struct wp_command_buffer *out, struct wp_rect bounds,
                            struct wp_image img, enum wp_button_behavior b,
                            const struct wp_style_button *style, const struct wp_input *in )
{
    wp_s32 ret;
    struct wp_rect content;

    WORKPHONE_ASSERT( state );
    WORKPHONE_ASSERT( style );
    WORKPHONE_ASSERT( out );
    if( !out || !style || !state )
        return wp_false;

    ret = wp_do_button( state, out, bounds, style, in, b, &content );
    content.x += style->image_padding.x;
    content.y += style->image_padding.y;
    content.w -= 2 * style->image_padding.x;
    content.h -= 2 * style->image_padding.y;

    if( style->draw_begin )
        style->draw_begin( out, style->userdata );
    wp_draw_button_image( out, &bounds, &content, *state, style, &img );
    if( style->draw_end )
        style->draw_end( out, style->userdata );
    return ret;
}

void wp_draw_button_text_symbol( struct wp_command_buffer *out, const struct wp_rect *bounds,
                                 const struct wp_rect *label, const struct wp_rect *symbol,
                                 wp_flags state, const struct wp_style_button *style, const wp_c8 *str,
                                 wp_s32 len, enum wp_symbol_type type, const struct wp_user_font *font )
{
    struct wp_color sym;
    struct wp_text text;
    const struct wp_style_item *background;

    /* select correct background colors/images */
    background = wp_draw_button( out, bounds, state, style );
    if( background->type == WORKPHONE_STYLE_ITEM_COLOR )
        text.background = background->data.color;
    else
        text.background = style->text_background;

    /* select correct text colors */
    if( state & WORKPHONE_WIDGET_STATE_HOVER )
    {
        sym = style->text_hover;
        text.text = style->text_hover;
    }
    else if( state & WORKPHONE_WIDGET_STATE_ACTIVED )
    {
        sym = style->text_active;
        text.text = style->text_active;
    }
    else
    {
        sym = style->text_normal;
        text.text = style->text_normal;
    }

    sym = wp_rgb_factor( sym, style->color_factor_text );
    text.text = wp_rgb_factor( text.text, style->color_factor_text );
    text.padding = wp_make_vec2f( 0, 0 );
    wp_draw_symbol( out, type, *symbol, style->text_background, sym, 0, font );
    wp_widget_text( out, *label, str, len, &text, WORKPHONE_TEXT_CENTERED, font );
}

wp_bool wp_do_button_text_symbol( wp_flags *state, struct wp_command_buffer *out, struct wp_rect bounds,
                                  enum wp_symbol_type symbol, const wp_c8 *str, wp_s32 len,
                                  wp_flags align, enum wp_button_behavior behavior,
                                  const struct wp_style_button *style, const struct wp_user_font *font,
                                  const struct wp_input *in )
{
    wp_s32 ret;
    struct wp_rect tri = { 0, 0, 0, 0 };
    struct wp_rect content;

    WORKPHONE_ASSERT( style );
    WORKPHONE_ASSERT( out );
    WORKPHONE_ASSERT( font );

    if( !out || !style || !font )
        return wp_false;

    ret = wp_do_button( state, out, bounds, style, in, behavior, &content );
    tri.y = content.y + ( content.h / 2 ) - font->height / 2;
    tri.w = font->height;
    tri.h = font->height;

    if( align & WORKPHONE_TEXT_ALIGN_LEFT )
    {
        tri.x = ( content.x + content.w ) - ( 2 * style->padding.x + tri.w );
        tri.x = WORKPHONE_MAX( tri.x, 0 );
    }
    else
        tri.x = content.x + 2 * style->padding.x;

    /* draw button */
    if( style->draw_begin )
        style->draw_begin( out, style->userdata );

    wp_draw_button_text_symbol( out, &bounds, &content, &tri, *state, style, str, len, symbol, font );
    
    if( style->draw_end )
        style->draw_end( out, style->userdata );

    return ret;
}

void wp_draw_button_text_image( struct wp_command_buffer *out, const struct wp_rect *bounds,
                                const struct wp_rect *label, const struct wp_rect *image, wp_flags state,
                                const struct wp_style_button *style, const wp_c8 *str, wp_s32 len,
                                const struct wp_user_font *font, const struct wp_image *img )
{
    struct wp_text text;
    const struct wp_style_item *background;
    background = wp_draw_button( out, bounds, state, style );

    /* select correct colors */
    if( background->type == WORKPHONE_STYLE_ITEM_COLOR )
        text.background = background->data.color;
    else
        text.background = style->text_background;
    if( state & WORKPHONE_WIDGET_STATE_HOVER )
        text.text = style->text_hover;
    else if( state & WORKPHONE_WIDGET_STATE_ACTIVED )
        text.text = style->text_active;
    else
        text.text = style->text_normal;

    text.text = wp_rgb_factor( text.text, style->color_factor_text );
    text.padding = wp_make_vec2f( 0, 0 );
    wp_widget_text( out, *label, str, len, &text, WORKPHONE_TEXT_CENTERED, font );
    wp_draw_image( out, *image, img, wp_rgb_factor( wp_white, style->color_factor_background ) );
}

wp_bool wp_do_button_text_image( wp_flags *state, struct wp_command_buffer *out, struct wp_rect bounds,
                                 struct wp_image img, const wp_c8 *str, wp_s32 len, wp_flags align,
                                 enum wp_button_behavior behavior, const struct wp_style_button *style,
                                 const struct wp_user_font *font, const struct wp_input *in )
{
    wp_s32 ret;
    struct wp_rect icon;
    struct wp_rect content;

    WORKPHONE_ASSERT( style );
    WORKPHONE_ASSERT( state );
    WORKPHONE_ASSERT( font );
    WORKPHONE_ASSERT( out );

    if( !out || !font || !style || !str )
        return wp_false;

    ret = wp_do_button( state, out, bounds, style, in, behavior, &content );
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

    if( style->draw_begin )
        style->draw_begin( out, style->userdata );

    wp_draw_button_text_image( out, &bounds, &content, &icon, *state, style, str, len, font, &img );
    
    if( style->draw_end )
        style->draw_end( out, style->userdata );

    return ret;
}

void wp_button_set_behavior( struct wp_context *ctx, enum wp_button_behavior behavior )
{
    WORKPHONE_ASSERT( ctx );

    if( !ctx )
        return;

    ctx->button_behavior = ( wp_button_behavior_enum )behavior;
}

wp_bool wp_button_push_behavior( struct wp_context *ctx, wp_button_behavior_enum behavior )
{
    struct wp_config_stack_button_behavior *button_stack;
    struct wp_config_stack_button_behavior_element *element;

    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return 0;

    button_stack = &ctx->stacks.button_behaviors;
    WORKPHONE_ASSERT( button_stack->head < (wp_s32)WORKPHONE_LEN( button_stack->elements ) );
    if( button_stack->head >= (wp_s32)WORKPHONE_LEN( button_stack->elements ) )
        return 0;

    element = &button_stack->elements[button_stack->head++];
    element->address = &ctx->button_behavior;
    element->old_value = ctx->button_behavior;
    ctx->button_behavior = ( wp_button_behavior_enum )behavior;
    return 1;
}

wp_bool wp_button_pop_behavior( struct wp_context *ctx )
{
    struct wp_config_stack_button_behavior *button_stack;
    struct wp_config_stack_button_behavior_element *element;

    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return 0;

    button_stack = &ctx->stacks.button_behaviors;
    WORKPHONE_ASSERT( button_stack->head > 0 );
    if( button_stack->head < 1 )
        return 0;

    element = &button_stack->elements[--button_stack->head];
    *element->address = element->old_value;
    return 1;
}

wp_bool wp_button_text_styled( struct wp_context *ctx, const struct wp_style_button *style,
                               const wp_c8 *title, wp_s32 len )
{
    struct wp_window *win;
    struct wp_panel *layout;
    const struct wp_input *in;

    struct wp_rect bounds;
    enum wp_widget_layout_states state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( style );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !style || !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    layout = win->layout;
    state = wp_widget( &bounds, ctx );

    if( !state )
        return 0;
    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;
    return wp_do_button_text( &ctx->last_widget_state, &win->buffer, bounds, title, len,
                              style->text_alignment, ctx->button_behavior, style, in, ctx->style.font );
}

wp_bool wp_button_text( struct wp_context *ctx, const wp_c8 *title, wp_s32 len )
{
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return 0;
    return wp_button_text_styled( ctx, &ctx->style.button, title, len );
}

wp_bool wp_button_label_styled( struct wp_context *ctx, const struct wp_style_button *style,
                                const wp_c8 *title )
{
    return wp_button_text_styled( ctx, style, title, wp_strlen( title ) );
}

wp_bool wp_button_label( struct wp_context *ctx, const wp_c8 *title )
{
    return wp_button_text( ctx, title, wp_strlen( title ) );
}

wp_bool wp_button_color( struct wp_context *ctx, struct wp_color color )
{
    struct wp_window *win;
    struct wp_panel *layout;
    const struct wp_input *in;
    struct wp_style_button button;

    wp_s32 ret = 0;
    struct wp_rect bounds;
    struct wp_rect content;
    enum wp_widget_layout_states state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    layout = win->layout;

    state = wp_widget( &bounds, ctx );
    if( !state )
        return 0;
    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;

    button = ctx->style.button;
    button.normal = wp_style_item_color( color );
    button.hover = wp_style_item_color( color );
    button.active = wp_style_item_color( color );
    ret = wp_do_button( &ctx->last_widget_state, &win->buffer, bounds, &button, in, ctx->button_behavior,
                        &content );
    wp_draw_button( &win->buffer, &bounds, ctx->last_widget_state, &button );
    return ret;
}

wp_bool wp_button_symbol_styled( struct wp_context *ctx, const struct wp_style_button *style,
                                 enum wp_symbol_type symbol )
{
    struct wp_window *win;
    struct wp_panel *layout;
    const struct wp_input *in;

    struct wp_rect bounds;
    enum wp_widget_layout_states state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    layout = win->layout;
    state = wp_widget( &bounds, ctx );
    if( !state )
        return 0;
    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;
    return wp_do_button_symbol( &ctx->last_widget_state, &win->buffer, bounds, symbol,
                                ctx->button_behavior, style, in, ctx->style.font );
}

wp_bool wp_button_symbol( struct wp_context *ctx, enum wp_symbol_type symbol )
{
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return 0;
    return wp_button_symbol_styled( ctx, &ctx->style.button, symbol );
}

wp_bool wp_button_image_styled( struct wp_context *ctx, const struct wp_style_button *style,
                                struct wp_image img )
{
    struct wp_window *win;
    struct wp_panel *layout;
    const struct wp_input *in;

    struct wp_rect bounds;
    enum wp_widget_layout_states state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    layout = win->layout;

    state = wp_widget( &bounds, ctx );
    if( !state )
        return 0;
    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;
    return wp_do_button_image( &ctx->last_widget_state, &win->buffer, bounds, img, ctx->button_behavior,
                               style, in );
}

wp_bool wp_button_image( struct wp_context *ctx, struct wp_image img )
{
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return 0;
    return wp_button_image_styled( ctx, &ctx->style.button, img );
}

wp_bool wp_button_symbol_text_styled( struct wp_context *ctx, const struct wp_style_button *style,
                                      enum wp_symbol_type symbol, const wp_c8 *text, wp_s32 len,
                                      wp_flags align )
{
    struct wp_window *win;
    struct wp_panel *layout;
    const struct wp_input *in;

    struct wp_rect bounds;
    enum wp_widget_layout_states state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    layout = win->layout;

    state = wp_widget( &bounds, ctx );
    if( !state )
        return 0;
    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;
    return wp_do_button_text_symbol( &ctx->last_widget_state, &win->buffer, bounds, symbol, text, len,
                                     align, ctx->button_behavior, style, ctx->style.font, in );
}

wp_bool wp_button_symbol_text( struct wp_context *ctx, enum wp_symbol_type symbol, const wp_c8 *text,
                               wp_s32 len, wp_flags align )
{
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return 0;
    return wp_button_symbol_text_styled( ctx, &ctx->style.button, symbol, text, len, align );
}

wp_bool wp_button_symbol_label( struct wp_context *ctx, enum wp_symbol_type symbol, const wp_c8 *label,
                                wp_flags align )
{
    return wp_button_symbol_text( ctx, symbol, label, wp_strlen( label ), align );
}

wp_bool wp_button_symbol_label_styled( struct wp_context *ctx, const struct wp_style_button *style,
                                       enum wp_symbol_type symbol, const wp_c8 *title, wp_flags align )
{
    return wp_button_symbol_text_styled( ctx, style, symbol, title, wp_strlen( title ), align );
}

wp_bool wp_button_image_text_styled( struct wp_context *ctx, const struct wp_style_button *style,
                                     struct wp_image img, const wp_c8 *text, wp_s32 len, wp_flags align )
{
    struct wp_window *win;
    struct wp_panel *layout;
    const struct wp_input *in;

    struct wp_rect bounds;
    enum wp_widget_layout_states state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    layout = win->layout;

    state = wp_widget( &bounds, ctx );
    if( !state )
        return 0;
    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;
    return wp_do_button_text_image( &ctx->last_widget_state, &win->buffer, bounds, img, text, len, align,
                                    ctx->button_behavior, style, ctx->style.font, in );
}

wp_bool wp_button_image_text( struct wp_context *ctx, struct wp_image img, const wp_c8 *text, wp_s32 len,
                              wp_flags align )
{
    return wp_button_image_text_styled( ctx, &ctx->style.button, img, text, len, align );
}

wp_bool wp_button_image_label( struct wp_context *ctx, struct wp_image img, const wp_c8 *label,
                               wp_flags align )
{
    return wp_button_image_text( ctx, img, label, wp_strlen( label ), align );
}

wp_bool wp_button_image_label_styled( struct wp_context *ctx, const struct wp_style_button *style,
                                      struct wp_image img, const wp_c8 *label, wp_flags text_alignment )
{
    return wp_button_image_text_styled( ctx, style, img, label, wp_strlen( label ), text_alignment );
}
