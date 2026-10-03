#include "workphone_color_picker.h"
#include "workphone_button.h"
#include "workphone_widget.h"
#include "workphone.h"

WORKPHONE_LIB wp_bool wp_color_picker_behavior( wp_flags *state, const struct wp_rect *bounds,
                                                const struct wp_rect *matrix,
                                                const struct wp_rect *hue_bar,
                                                const struct wp_rect *alpha_bar, struct wp_colorf *color,
                                                const struct wp_input *in )
{
    wp_f32 hsva[4];
    wp_bool value_changed = 0;
    wp_bool hsv_changed = 0;

    WORKPHONE_ASSERT( state );
    WORKPHONE_ASSERT( matrix );
    WORKPHONE_ASSERT( hue_bar );
    WORKPHONE_ASSERT( color );

    /* color matrix */
    wp_colorf_hsva_fv( hsva, *color );
    if( wp_button_behavior( state, *matrix, in, WORKPHONE_BUTTON_REPEATER ) )
    {
        hsva[1] = WORKPHONE_SATURATE( ( in->mouse.pos.x - matrix->x ) / ( matrix->w - 1 ) );
        hsva[2] = 1.0f - WORKPHONE_SATURATE( ( in->mouse.pos.y - matrix->y ) / ( matrix->h - 1 ) );
        value_changed = hsv_changed = 1;
    }
    /* hue bar */
    if( wp_button_behavior( state, *hue_bar, in, WORKPHONE_BUTTON_REPEATER ) )
    {
        hsva[0] = WORKPHONE_SATURATE( ( in->mouse.pos.y - hue_bar->y ) / ( hue_bar->h - 1 ) );
        value_changed = hsv_changed = 1;
    }
    /* alpha bar */
    if( alpha_bar )
    {
        if( wp_button_behavior( state, *alpha_bar, in, WORKPHONE_BUTTON_REPEATER ) )
        {
            hsva[3] =
                1.0f - WORKPHONE_SATURATE( ( in->mouse.pos.y - alpha_bar->y ) / ( alpha_bar->h - 1 ) );
            value_changed = 1;
        }
    }
    wp_widget_state_reset( state );
    if( hsv_changed )
    {
        *color = wp_hsva_colorfv( hsva );
        *state = WORKPHONE_WIDGET_STATE_ACTIVE;
    }
    if( value_changed )
    {
        color->a = hsva[3];
        *state = WORKPHONE_WIDGET_STATE_ACTIVE;
    }
    /* set color picker widget state */
    if( wp_input_is_mouse_hovering_rect( in, *bounds ) )
        *state = WORKPHONE_WIDGET_STATE_HOVERED;
    if( *state & WORKPHONE_WIDGET_STATE_HOVER && !wp_input_is_mouse_prev_hovering_rect( in, *bounds ) )
        *state |= WORKPHONE_WIDGET_STATE_ENTERED;
    else if( wp_input_is_mouse_prev_hovering_rect( in, *bounds ) )
        *state |= WORKPHONE_WIDGET_STATE_LEFT;
    return value_changed;
}

WORKPHONE_LIB void wp_draw_color_picker( struct wp_command_buffer *o, const struct wp_rect *matrix,
                                         const struct wp_rect *hue_bar, const struct wp_rect *alpha_bar,
                                         struct wp_colorf col )
{
    WORKPHONE_STORAGE const struct wp_color black = { 0, 0, 0, 255 };
    WORKPHONE_STORAGE const struct wp_color white = { 255, 255, 255, 255 };
    WORKPHONE_STORAGE const struct wp_color black_trans = { 0, 0, 0, 0 };

    const wp_f32 crosshair_size = 7.0f;
    struct wp_color temp;
    wp_f32 hsva[4];
    wp_f32 line_y;
    wp_s32 i;

    WORKPHONE_ASSERT( o );
    WORKPHONE_ASSERT( matrix );
    WORKPHONE_ASSERT( hue_bar );

    /* draw hue bar */
    wp_colorf_hsva_fv( hsva, col );
    for( i = 0; i < 6; ++i )
    {
        WORKPHONE_GLOBAL const struct wp_color hue_colors[] = { { 255, 0, 0, 255 }, { 255, 255, 0, 255 },
                                                                { 0, 255, 0, 255 }, { 0, 255, 255, 255 },
                                                                { 0, 0, 255, 255 }, { 255, 0, 255, 255 },
                                                                { 255, 0, 0, 255 } };
        wp_fill_rect_multi_color(
            o,
            wp_make_rect( hue_bar->x, hue_bar->y + (wp_f32)i * ( hue_bar->h / 6.0f ) + 0.5f, hue_bar->w,
                          ( hue_bar->h / 6.0f ) + 0.5f ),
            hue_colors[i], hue_colors[i], hue_colors[i + 1], hue_colors[i + 1] );
    }
    line_y = (wp_f32)(wp_s32)( hue_bar->y + hsva[0] * matrix->h + 0.5f );
    wp_stroke_line( o, hue_bar->x - 1, line_y, hue_bar->x + hue_bar->w + 2, line_y, 1,
                    wp_rgb( 255, 255, 255 ) );

    /* draw alpha bar */
    if( alpha_bar )
    {
        wp_f32 alpha = WORKPHONE_SATURATE( col.a );
        line_y = (wp_f32)(wp_s32)( alpha_bar->y + ( 1.0f - alpha ) * matrix->h + 0.5f );

        wp_fill_rect_multi_color( o, *alpha_bar, white, white, black, black );
        wp_stroke_line( o, alpha_bar->x - 1, line_y, alpha_bar->x + alpha_bar->w + 2, line_y, 1,
                        wp_rgb( 255, 255, 255 ) );
    }

    /* draw color matrix */
    temp = wp_hsv_f( hsva[0], 1.0f, 1.0f );
    wp_fill_rect_multi_color( o, *matrix, white, temp, temp, white );
    wp_fill_rect_multi_color( o, *matrix, black_trans, black_trans, black, black );

    /* draw cross-hair */
    {
        struct wp_vec2f p;
        wp_f32 S = hsva[1];
        wp_f32 V = hsva[2];
        p.x = (wp_f32)(wp_s32)( matrix->x + S * matrix->w );
        p.y = (wp_f32)(wp_s32)( matrix->y + ( 1.0f - V ) * matrix->h );
        wp_stroke_line( o, p.x - crosshair_size, p.y, p.x - 2, p.y, 1.0f, white );
        wp_stroke_line( o, p.x + crosshair_size + 1, p.y, p.x + 3, p.y, 1.0f, white );
        wp_stroke_line( o, p.x, p.y + crosshair_size + 1, p.x, p.y + 3, 1.0f, white );
        wp_stroke_line( o, p.x, p.y - crosshair_size, p.x, p.y - 2, 1.0f, white );
    }
}

WORKPHONE_LIB wp_bool wp_do_color_picker( wp_flags *state, struct wp_command_buffer *out,
                                          struct wp_colorf *col, enum wp_color_format fmt,
                                          struct wp_rect bounds, struct wp_vec2f padding,
                                          const struct wp_input *in, const struct wp_user_font *font )
{
    wp_s32 ret = 0;
    struct wp_rect matrix;
    struct wp_rect hue_bar;
    struct wp_rect alpha_bar;
    wp_f32 bar_w;

    WORKPHONE_ASSERT( out );
    WORKPHONE_ASSERT( col );
    WORKPHONE_ASSERT( state );
    WORKPHONE_ASSERT( font );
    if( !out || !col || !state || !font )
        return ret;

    bar_w = font->height;
    bounds.x += padding.x;
    bounds.y += padding.x;
    bounds.w -= 2 * padding.x;
    bounds.h -= 2 * padding.y;

    matrix.x = bounds.x;
    matrix.y = bounds.y;
    matrix.h = bounds.h;
    matrix.w = bounds.w - ( 3 * padding.x + 2 * bar_w );

    hue_bar.w = bar_w;
    hue_bar.y = bounds.y;
    hue_bar.h = matrix.h;
    hue_bar.x = matrix.x + matrix.w + padding.x;

    alpha_bar.x = hue_bar.x + hue_bar.w + padding.x;
    alpha_bar.y = bounds.y;
    alpha_bar.w = bar_w;
    alpha_bar.h = matrix.h;

    ret = wp_color_picker_behavior( state, &bounds, &matrix, &hue_bar,
                                    ( fmt == WORKPHONE_RGBA ) ? &alpha_bar : 0, col, in );
    wp_draw_color_picker( out, &matrix, &hue_bar, ( fmt == WORKPHONE_RGBA ) ? &alpha_bar : 0, *col );
    return ret;
}

WORKPHONE_API wp_bool wp_color_pick( struct wp_context *ctx, struct wp_colorf *color,
                                     enum wp_color_format fmt )
{
    struct wp_window *win;
    struct wp_panel *layout;
    const struct wp_style *config;
    const struct wp_input *in;

    enum wp_widget_layout_states state;
    struct wp_rect bounds;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( color );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout || !color )
        return 0;

    win = ctx->current;
    config = &ctx->style;
    layout = win->layout;
    state = wp_widget( &bounds, ctx );
    if( !state )
        return 0;
    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;
    return wp_do_color_picker( &ctx->last_widget_state, &win->buffer, color, fmt, bounds,
                               wp_make_vec2f( 0, 0 ), in, config->font );
}

WORKPHONE_API struct wp_colorf wp_color_picker( struct wp_context *ctx, struct wp_colorf color,
                                                enum wp_color_format fmt )
{
    wp_color_pick( ctx, &color, fmt );
    return color;
}
