#include "workphone.h"
#include "workphone_button.h"
#include "workphone_font.h"
#include "workphone_widget.h"

wp_bool wp_toggle_behavior( const struct wp_input *in, struct wp_rect select, wp_flags *state,
                            wp_bool active )
{
    wp_widget_state_reset( state );
    if( wp_button_behavior( state, select, in, WORKPHONE_BUTTON_DEFAULT ) )
    {
        *state = WORKPHONE_WIDGET_STATE_ACTIVE;
        active = !active;
    }
    if( *state & WORKPHONE_WIDGET_STATE_HOVER && !wp_input_is_mouse_prev_hovering_rect( in, select ) )
        *state |= WORKPHONE_WIDGET_STATE_ENTERED;
    else if( wp_input_is_mouse_prev_hovering_rect( in, select ) )
        *state |= WORKPHONE_WIDGET_STATE_LEFT;
    return active;
}
void wp_draw_checkbox( struct wp_command_buffer *out, wp_flags state,
                       const struct wp_style_toggle *style, wp_bool active, const struct wp_rect *label,
                       const struct wp_rect *selector, const struct wp_rect *cursors,
                       const wp_c8 *string, wp_s32 len, const struct wp_user_font *font,
                       wp_flags text_alignment )
{
    const struct wp_style_item *background;
    const struct wp_style_item *cursor;
    struct wp_text text;

    /* select correct colors/images */
    if( state & WORKPHONE_WIDGET_STATE_HOVER )
    {
        background = &style->hover;
        cursor = &style->cursor_hover;
        text.text = style->text_hover;
    }
    else if( state & WORKPHONE_WIDGET_STATE_ACTIVED )
    {
        background = &style->hover;
        cursor = &style->cursor_hover;
        text.text = style->text_active;
    }
    else
    {
        background = &style->normal;
        cursor = &style->cursor_normal;
        text.text = style->text_normal;
    }

    text.text = wp_rgb_factor( text.text, style->color_factor );
    text.padding.x = 0;
    text.padding.y = 0;
    text.background = style->text_background;
    wp_widget_text( out, *label, string, len, &text, text_alignment, font );

    /* draw background and cursor */
    if( background->type == WORKPHONE_STYLE_ITEM_COLOR )
    {
        wp_fill_rect( out, *selector, 0, wp_rgb_factor( style->border_color, style->color_factor ) );
        wp_fill_rect( out, wp_shrink_make_rect( *selector, style->border ), 0,
                      wp_rgb_factor( background->data.color, style->color_factor ) );
    }
    else
        wp_draw_image( out, *selector, &background->data.image,
                       wp_rgb_factor( wp_white, style->color_factor ) );
    if( active )
    {
        if( cursor->type == WORKPHONE_STYLE_ITEM_IMAGE )
            wp_draw_image( out, *cursors, &cursor->data.image,
                           wp_rgb_factor( wp_white, style->color_factor ) );
        else
            wp_fill_rect( out, *cursors, 0, cursor->data.color );
    }
}

void wp_draw_option( struct wp_command_buffer *out, wp_flags state, const struct wp_style_toggle *style,
                     wp_bool active, const struct wp_rect *label, const struct wp_rect *selector,
                     const struct wp_rect *cursors, const wp_c8 *string, wp_s32 len,
                     const struct wp_user_font *font, wp_flags text_alignment )
{
    const struct wp_style_item *background;
    const struct wp_style_item *cursor;
    struct wp_text text;

    /* select correct colors/images */
    if( state & WORKPHONE_WIDGET_STATE_HOVER )
    {
        background = &style->hover;
        cursor = &style->cursor_hover;
        text.text = style->text_hover;
    }
    else if( state & WORKPHONE_WIDGET_STATE_ACTIVED )
    {
        background = &style->hover;
        cursor = &style->cursor_hover;
        text.text = style->text_active;
    }
    else
    {
        background = &style->normal;
        cursor = &style->cursor_normal;
        text.text = style->text_normal;
    }

    text.text = wp_rgb_factor( text.text, style->color_factor );
    text.padding.x = 0;
    text.padding.y = 0;
    text.background = style->text_background;
    wp_widget_text( out, *label, string, len, &text, text_alignment, font );

    /* draw background and cursor */
    if( background->type == WORKPHONE_STYLE_ITEM_COLOR )
    {
        wp_fill_circle( out, *selector, wp_rgb_factor( style->border_color, style->color_factor ) );
        wp_fill_circle( out, wp_shrink_make_rect( *selector, style->border ),
                        wp_rgb_factor( background->data.color, style->color_factor ) );
    }
    else
        wp_draw_image( out, *selector, &background->data.image,
                       wp_rgb_factor( wp_white, style->color_factor ) );
    if( active )
    {
        if( cursor->type == WORKPHONE_STYLE_ITEM_IMAGE )
            wp_draw_image( out, *cursors, &cursor->data.image,
                           wp_rgb_factor( wp_white, style->color_factor ) );
        else
            wp_fill_circle( out, *cursors, cursor->data.color );
    }
}

wp_bool wp_do_toggle( wp_flags *state, struct wp_command_buffer *out, struct wp_rect r, wp_bool *active,
                      const wp_c8 *str, wp_s32 len, enum wp_toggle_type type,
                      const struct wp_style_toggle *style, const struct wp_input *in,
                      const struct wp_user_font *font, wp_flags widget_alignment,
                      wp_flags text_alignment )
{
    wp_s32 was_active;
    struct wp_rect bounds;
    struct wp_rect select;
    struct wp_rect cursor;
    struct wp_rect label;

    WORKPHONE_ASSERT( style );
    WORKPHONE_ASSERT( out );
    WORKPHONE_ASSERT( font );
    if( !out || !style || !font || !active )
        return 0;

    r.w = WORKPHONE_MAX( r.w, font->height + 2 * style->padding.x );
    r.h = WORKPHONE_MAX( r.h, font->height + 2 * style->padding.y );

    /* add additional touch padding for touch screen devices */
    bounds.x = r.x - style->touch_padding.x;
    bounds.y = r.y - style->touch_padding.y;
    bounds.w = r.w + 2 * style->touch_padding.x;
    bounds.h = r.h + 2 * style->touch_padding.y;

    /* calculate the selector space */
    select.w = font->height;
    select.h = select.w;

    if( widget_alignment & WORKPHONE_WIDGET_ALIGN_RIGHT )
    {
        select.x = r.x + r.w - font->height;

        /* label in front of the selector */
        label.x = r.x;
        label.w = r.w - select.w - style->spacing * 2;
    }
    else if( widget_alignment & WORKPHONE_WIDGET_ALIGN_CENTERED )
    {
        select.x = r.x + ( r.w - select.w ) / 2;

        /* label in front of selector */
        label.x = r.x;
        label.w = ( r.w - select.w - style->spacing * 2 ) / 2;
    }
    else
    { /* Default: WORKPHONE_WIDGET_ALIGN_LEFT */
        select.x = r.x;

        /* label behind the selector */
        label.x = select.x + select.w + style->spacing;
        label.w = WORKPHONE_MAX( r.x + r.w, label.x ) - label.x;
    }

    if( widget_alignment & WORKPHONE_WIDGET_ALIGN_TOP )
    {
        select.y = r.y;
    }
    else if( widget_alignment & WORKPHONE_WIDGET_ALIGN_BOTTOM )
    {
        select.y = r.y + r.h - select.h - 2 * style->padding.y;
    }
    else
    { /* Default: WORKPHONE_WIDGET_ALIGN_MIDDLE */
        select.y = r.y + r.h / 2.0f - select.h / 2.0f;
    }

    label.y = select.y;
    label.h = select.w;

    /* calculate the bounds of the cursor inside the selector */
    cursor.x = select.x + style->padding.x + style->border;
    cursor.y = select.y + style->padding.y + style->border;
    cursor.w = select.w - ( 2 * style->padding.x + 2 * style->border );
    cursor.h = select.h - ( 2 * style->padding.y + 2 * style->border );

    /* update selector */
    was_active = *active;
    *active = wp_toggle_behavior( in, bounds, state, *active );

    /* draw selector */
    if( style->draw_begin )
        style->draw_begin( out, style->userdata );
    if( type == WORKPHONE_TOGGLE_CHECK )
    {
        wp_draw_checkbox( out, *state, style, *active, &label, &select, &cursor, str, len, font,
                          text_alignment );
    }
    else
    {
        wp_draw_option( out, *state, style, *active, &label, &select, &cursor, str, len, font,
                        text_alignment );
    }
    if( style->draw_end )
        style->draw_end( out, style->userdata );
    return ( was_active != *active );
}

/*----------------------------------------------------------------
 *
 *                          CHECKBOX
 *
 * --------------------------------------------------------------*/
wp_bool wp_check_text( struct wp_context *ctx, const wp_c8 *text, wp_s32 len, wp_bool active )
{
    struct wp_window *win;
    struct wp_panel *layout;
    const struct wp_input *in;
    const struct wp_style *style;

    struct wp_rect bounds;
    enum wp_widget_layout_states state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return active;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;

    state = wp_widget( &bounds, ctx );
    if( !state )
        return active;
    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;
    wp_do_toggle( &ctx->last_widget_state, &win->buffer, bounds, &active, text, len,
                  WORKPHONE_TOGGLE_CHECK, &style->checkbox, in, style->font, WORKPHONE_WIDGET_LEFT,
                  WORKPHONE_TEXT_LEFT );
    return active;
}
wp_bool wp_check_text_align( struct wp_context *ctx, const wp_c8 *text, wp_s32 len, wp_bool active,
                             wp_flags widget_alignment, wp_flags text_alignment )
{
    struct wp_window *win;
    struct wp_panel *layout;
    const struct wp_input *in;
    const struct wp_style *style;

    struct wp_rect bounds;
    enum wp_widget_layout_states state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return active;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;

    state = wp_widget( &bounds, ctx );
    if( !state )
        return active;
    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;
    wp_do_toggle( &ctx->last_widget_state, &win->buffer, bounds, &active, text, len,
                  WORKPHONE_TOGGLE_CHECK, &style->checkbox, in, style->font, widget_alignment,
                  text_alignment );
    return active;
}
wp_u32 wp_check_flags_text( struct wp_context *ctx, const wp_c8 *text, wp_s32 len, wp_u32 flags,
                            wp_u32 value )
{
    wp_s32 old_active;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( text );
    if( !ctx || !text )
        return flags;
    old_active = (wp_s32)( ( flags & value ) & value );
    if( wp_check_text( ctx, text, len, old_active ) )
        flags |= value;
    else
        flags &= ~value;
    return flags;
}
wp_bool wp_checkbox_text( struct wp_context *ctx, const wp_c8 *text, wp_s32 len, wp_bool *active )
{
    wp_s32 old_val;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( text );
    WORKPHONE_ASSERT( active );
    if( !ctx || !text || !active )
        return 0;
    old_val = *active;
    *active = wp_check_text( ctx, text, len, *active );
    return old_val != *active;
}
wp_bool wp_checkbox_text_align( struct wp_context *ctx, const wp_c8 *text, wp_s32 len, wp_bool *active,
                                wp_flags widget_alignment, wp_flags text_alignment )
{
    wp_s32 old_val;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( text );
    WORKPHONE_ASSERT( active );
    if( !ctx || !text || !active )
        return 0;
    old_val = *active;
    *active = wp_check_text_align( ctx, text, len, *active, widget_alignment, text_alignment );
    return old_val != *active;
}
wp_bool wp_checkbox_flags_text( struct wp_context *ctx, const wp_c8 *text, wp_s32 len, wp_u32 *flags,
                                wp_u32 value )
{
    wp_bool active;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( text );
    WORKPHONE_ASSERT( flags );
    if( !ctx || !text || !flags )
        return 0;

    active = (wp_s32)( ( *flags & value ) & value );
    if( wp_checkbox_text( ctx, text, len, &active ) )
    {
        if( active )
            *flags |= value;
        else
            *flags &= ~value;
        return 1;
    }
    return 0;
}
wp_bool wp_check_label( struct wp_context *ctx, const wp_c8 *label, wp_bool active )
{
    return wp_check_text( ctx, label, wp_strlen( label ), active );
}

wp_u32 wp_check_flags_label( struct wp_context *ctx, const wp_c8 *label, wp_u32 flags, wp_u32 value )
{
    return wp_check_flags_text( ctx, label, wp_strlen( label ), flags, value );
}

wp_bool wp_checkbox_label( struct wp_context *ctx, const wp_c8 *label, wp_bool *active )
{
    return wp_checkbox_text( ctx, label, wp_strlen( label ), active );
}
wp_bool wp_checkbox_label_align( struct wp_context *ctx, const wp_c8 *label, wp_bool *active,
                                 wp_flags widget_alignment, wp_flags text_alignment )
{
    return wp_checkbox_text_align( ctx, label, wp_strlen( label ), active, widget_alignment,
                                   text_alignment );
}

wp_bool wp_checkbox_flags_label( struct wp_context *ctx, const wp_c8 *label, wp_u32 *flags,
                                 wp_u32 value )
{
    return wp_checkbox_flags_text( ctx, label, wp_strlen( label ), flags, value );
}

wp_bool wp_option_text( struct wp_context *ctx, const wp_c8 *text, wp_s32 len, wp_bool is_active )
{
    struct wp_window *win;
    struct wp_panel *layout;
    const struct wp_input *in;
    const struct wp_style *style;

    struct wp_rect bounds;
    enum wp_widget_layout_states state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return is_active;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;

    state = wp_widget( &bounds, ctx );
    if( !state )
        return (wp_s32)state;
    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;
    wp_do_toggle( &ctx->last_widget_state, &win->buffer, bounds, &is_active, text, len,
                  WORKPHONE_TOGGLE_OPTION, &style->option, in, style->font, WORKPHONE_WIDGET_LEFT,
                  WORKPHONE_TEXT_LEFT );
    return is_active;
}

wp_bool wp_option_text_align( struct wp_context *ctx, const wp_c8 *text, wp_s32 len, wp_bool is_active,
                              wp_flags widget_alignment, wp_flags text_alignment )
{
    struct wp_window *win;
    struct wp_panel *layout;
    const struct wp_input *in;
    const struct wp_style *style;

    struct wp_rect bounds;
    enum wp_widget_layout_states state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return is_active;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;

    state = wp_widget( &bounds, ctx );
    if( !state )
        return (wp_s32)state;
    in = ( state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
           layout->flags & WORKPHONE_WINDOW_ROM )
             ? 0
             : &ctx->input;
    wp_do_toggle( &ctx->last_widget_state, &win->buffer, bounds, &is_active, text, len,
                  WORKPHONE_TOGGLE_OPTION, &style->option, in, style->font, widget_alignment,
                  text_alignment );
    return is_active;
}

wp_bool wp_radio_text( struct wp_context *ctx, const wp_c8 *text, wp_s32 len, wp_bool *active )
{
    wp_s32 old_value;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( text );
    WORKPHONE_ASSERT( active );
    if( !ctx || !text || !active )
        return 0;
    old_value = *active;
    *active = wp_option_text( ctx, text, len, old_value );
    return old_value != *active;
}

wp_bool wp_radio_text_align( struct wp_context *ctx, const wp_c8 *text, wp_s32 len, wp_bool *active,
                             wp_flags widget_alignment, wp_flags text_alignment )
{
    wp_s32 old_value;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( text );
    WORKPHONE_ASSERT( active );
    if( !ctx || !text || !active )
        return 0;
    old_value = *active;
    *active = wp_option_text_align( ctx, text, len, old_value, widget_alignment, text_alignment );
    return old_value != *active;
}

wp_bool wp_option_label( struct wp_context *ctx, const wp_c8 *label, wp_bool active )
{
    return wp_option_text( ctx, label, wp_strlen( label ), active );
}

wp_bool wp_option_label_align( struct wp_context *ctx, const wp_c8 *label, wp_bool active,
                               wp_flags widget_alignment, wp_flags text_alignment )
{
    return wp_option_text_align( ctx, label, wp_strlen( label ), active, widget_alignment,
                                 text_alignment );
}

wp_bool wp_radio_label( struct wp_context *ctx, const wp_c8 *label, wp_bool *active )
{
    return wp_radio_text( ctx, label, wp_strlen( label ), active );
}

wp_bool wp_radio_label_align( struct wp_context *ctx, const wp_c8 *label, wp_bool *active,
                              wp_flags widget_alignment, wp_flags text_alignment )
{
    return wp_radio_text_align( ctx, label, wp_strlen( label ), active, widget_alignment,
                                text_alignment );
}
