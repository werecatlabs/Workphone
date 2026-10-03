#include "workphone_property.h"
#include "workphone_button.h"
#include "workphone_context.h"
#include "workphone_edit.h"
#include "workphone_font.h"
#include "workphone_input.h"
#include "workphone_math.h"
#include "workphone_style.h"
#include "workphone_text.h"
#include "workphone_utf8.h"
#include "workphone_util.h"
#include "workphone_widget.h"
#include "workphone_window.h"

void wp_drag_behavior( wp_flags *state, const struct wp_input *in, struct wp_rect drag,
                       struct wp_property_variant *variant, wp_f32 inc_per_pixel )
{
    wp_s32 left_mouse_down = in && in->mouse.buttons[WORKPHONE_BUTTON_LEFT].down;
    wp_s32 left_mouse_click_in_cursor =
        in && wp_input_has_mouse_click_down_in_rect( in, WORKPHONE_BUTTON_LEFT, drag, wp_true );

    wp_widget_state_reset( state );
    if( wp_input_is_mouse_hovering_rect( in, drag ) )
        *state = WORKPHONE_WIDGET_STATE_HOVERED;

    if( left_mouse_down && left_mouse_click_in_cursor )
    {
        wp_f32 delta, pixels;
        pixels = in->mouse.delta.x;
        delta = pixels * inc_per_pixel;
        switch( variant->kind )
        {
        default:
            break;
        case WORKPHONE_PROPERTY_INT:
            variant->value.i = variant->value.i + (wp_s32)delta;
            variant->value.i =
                WORKPHONE_CLAMP( variant->min_value.i, variant->value.i, variant->max_value.i );
            break;
        case WORKPHONE_PROPERTY_FLOAT:
            variant->value.f = variant->value.f + (wp_f32)delta;
            variant->value.f =
                WORKPHONE_CLAMP( variant->min_value.f, variant->value.f, variant->max_value.f );
            break;
        case WORKPHONE_PROPERTY_DOUBLE:
            variant->value.d = variant->value.d + (wp_f64)delta;
            variant->value.d =
                WORKPHONE_CLAMP( variant->min_value.d, variant->value.d, variant->max_value.d );
            break;
        }
        *state = WORKPHONE_WIDGET_STATE_ACTIVE;
    }
    if( *state & WORKPHONE_WIDGET_STATE_HOVER && !wp_input_is_mouse_prev_hovering_rect( in, drag ) )
        *state |= WORKPHONE_WIDGET_STATE_ENTERED;
    else if( wp_input_is_mouse_prev_hovering_rect( in, drag ) )
        *state |= WORKPHONE_WIDGET_STATE_LEFT;
}
WORKPHONE_LIB void wp_property_behavior( wp_flags *ws, const struct wp_input *in,
                                         struct wp_rect property, struct wp_rect label,
                                         struct wp_rect edit, struct wp_rect empty, wp_s32 *state,
                                         struct wp_property_variant *variant, wp_f32 inc_per_pixel )
{
    wp_widget_state_reset( ws );
    if( in && *state == WORKPHONE_PROPERTY_DEFAULT )
    {
        if( wp_button_behavior( ws, edit, in, WORKPHONE_BUTTON_DEFAULT ) )
            *state = WORKPHONE_PROPERTY_EDIT;
        else if( wp_input_is_mouse_click_down_in_rect( in, WORKPHONE_BUTTON_LEFT, label, wp_true ) )
            *state = WORKPHONE_PROPERTY_DRAG;
        else if( wp_input_is_mouse_click_down_in_rect( in, WORKPHONE_BUTTON_LEFT, empty, wp_true ) )
            *state = WORKPHONE_PROPERTY_DRAG;
    }
    if( *state == WORKPHONE_PROPERTY_DRAG )
    {
        wp_drag_behavior( ws, in, property, variant, inc_per_pixel );
        if( !( *ws & WORKPHONE_WIDGET_STATE_ACTIVED ) )
            *state = WORKPHONE_PROPERTY_DEFAULT;
    }
}
WORKPHONE_LIB void wp_draw_property( struct wp_command_buffer *out,
                                     const struct wp_style_property *style, const struct wp_rect *bounds,
                                     const struct wp_rect *label, wp_flags state, const wp_c8 *name,
                                     wp_s32 len, const struct wp_user_font *font )
{
    struct wp_text text;
    const struct wp_style_item *background;

    /* select correct background and text color */
    if( state & WORKPHONE_WIDGET_STATE_ACTIVED )
    {
        background = &style->active;
        text.text = style->label_active;
    }
    else if( state & WORKPHONE_WIDGET_STATE_HOVER )
    {
        background = &style->hover;
        text.text = style->label_hover;
    }
    else
    {
        background = &style->normal;
        text.text = style->label_normal;
    }

    text.text = wp_rgb_factor( text.text, style->color_factor );

    /* draw background */
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
        wp_fill_rect( out, *bounds, style->rounding,
                      wp_rgb_factor( background->data.color, style->color_factor ) );
        wp_stroke_rect( out, *bounds, style->rounding, style->border,
                        wp_rgb_factor( style->border_color, style->color_factor ) );
        break;
    }

    /* draw label */
    text.padding = wp_make_vec2f( 0, 0 );
    if( name && name[0] != '#' )
    {
        wp_widget_text( out, *label, name, len, &text, WORKPHONE_TEXT_CENTERED, font );
    }
}
WORKPHONE_INTERN void wp_property_save( struct wp_property_variant *variant, wp_c8 *buffer, wp_s32 len )
{
    buffer[len] = '\0';
    switch( variant->kind )
    {
    default:
        break;
    case WORKPHONE_PROPERTY_INT:
        variant->value.i = wp_strtoi( buffer, 0 );
        variant->value.i =
            WORKPHONE_CLAMP( variant->min_value.i, variant->value.i, variant->max_value.i );
        break;
    case WORKPHONE_PROPERTY_FLOAT:
        wp_string_wp_f32_limit( buffer, WORKPHONE_MAX_FLOAT_PRECISION );
        variant->value.f = wp_strtof( buffer, 0 );
        variant->value.f =
            WORKPHONE_CLAMP( variant->min_value.f, variant->value.f, variant->max_value.f );
        break;
    case WORKPHONE_PROPERTY_DOUBLE:
        wp_string_wp_f32_limit( buffer, WORKPHONE_MAX_FLOAT_PRECISION );
        variant->value.d = WORKPHONE_STRTOD( buffer, 0 );
        variant->value.d =
            WORKPHONE_CLAMP( variant->min_value.d, variant->value.d, variant->max_value.d );
        break;
    }
}
WORKPHONE_LIB void wp_do_property( wp_flags *ws, struct wp_command_buffer *out, struct wp_rect property,
                                   const wp_c8 *name, struct wp_property_variant *variant,
                                   wp_f32 inc_per_pixel, wp_c8 *buffer, wp_s32 *len, wp_s32 *state,
                                   wp_s32 *cursor, wp_s32 *select_begin, wp_s32 *select_end,
                                   const struct wp_style_property *style, enum wp_property_filter filter,
                                   struct wp_input *in, const struct wp_user_font *font,
                                   struct wp_text_edit *text_edit, enum wp_button_behavior behavior )
{
    const wp_plugin_filter filters[] = { wp_filter_decimal, wp_filter_wp_f32 };
    wp_bool active, old;
    wp_s32 num_len = 0, name_len = 0;
    wp_c8 string[WORKPHONE_MAX_NUMBER_BUFFER];
    wp_f32 size;

    wp_c8 *dst = 0;
    wp_s32 *length;

    struct wp_rect left;
    struct wp_rect right;
    struct wp_rect label;
    struct wp_rect edit;
    struct wp_rect empty;

    /* left decrement button */
    left.h = font->height / 2;
    left.w = left.h;
    left.x = property.x + style->border + style->padding.x;
    left.y = property.y + style->border + property.h / 2.0f - left.h / 2;

    /* text label */
    if( name && name[0] != '#' )
    {
        name_len = wp_strlen( name );
    }
    size = font->width( font->userdata, font->height, name, name_len );
    label.x = left.x + left.w + style->padding.x;
    label.w = (wp_f32)size + 2 * style->padding.x;
    label.y = property.y + style->border + style->padding.y;
    label.h = property.h - ( 2 * style->border + 2 * style->padding.y );

    /* right increment button */
    right.y = left.y;
    right.w = left.w;
    right.h = left.h;
    right.x = property.x + property.w - ( right.w + style->padding.x );

    /* edit */
    if( *state == WORKPHONE_PROPERTY_EDIT )
    {
        size = font->width( font->userdata, font->height, buffer, *len );
        size += style->edit.cursor_size;
        length = len;
        dst = buffer;
    }
    else
    {
        switch( variant->kind )
        {
        default:
            break;
        case WORKPHONE_PROPERTY_INT:
            wp_itoa( string, variant->value.i );
            num_len = wp_strlen( string );
            break;
        case WORKPHONE_PROPERTY_FLOAT:
            WORKPHONE_DTOA( string, (wp_f64)variant->value.f );
            num_len = wp_string_wp_f32_limit( string, WORKPHONE_MAX_FLOAT_PRECISION );
            break;
        case WORKPHONE_PROPERTY_DOUBLE:
            WORKPHONE_DTOA( string, variant->value.d );
            num_len = wp_string_wp_f32_limit( string, WORKPHONE_MAX_FLOAT_PRECISION );
            break;
        }
        size = font->width( font->userdata, font->height, string, num_len );
        dst = string;
        length = &num_len;
    }

    edit.w = (wp_f32)size + 2 * style->padding.x;
    edit.w = WORKPHONE_MIN( edit.w, right.x - ( label.x + label.w ) );
    edit.x = right.x - ( edit.w + style->padding.x );
    edit.y = property.y + style->border;
    edit.h = property.h - ( 2 * style->border );

    /* empty left space activator */
    empty.w = edit.x - ( label.x + label.w );
    empty.x = label.x + label.w;
    empty.y = property.y;
    empty.h = property.h;

    /* update property */
    old = ( *state == WORKPHONE_PROPERTY_EDIT );
    wp_property_behavior( ws, in, property, label, edit, empty, state, variant, inc_per_pixel );

    /* draw property */
    if( style->draw_begin )
        style->draw_begin( out, style->userdata );
    wp_draw_property( out, style, &property, &label, *ws, name, name_len, font );
    if( style->draw_end )
        style->draw_end( out, style->userdata );

    /* execute right button  */
    if( wp_do_button_symbol( ws, out, left, style->sym_left, behavior, &style->dec_button, in, font ) )
    {
        switch( variant->kind )
        {
        default:
            break;
        case WORKPHONE_PROPERTY_INT:
            variant->value.i = WORKPHONE_CLAMP( variant->min_value.i, variant->value.i - variant->step.i,
                                                variant->max_value.i );
            break;
        case WORKPHONE_PROPERTY_FLOAT:
            variant->value.f = WORKPHONE_CLAMP( variant->min_value.f, variant->value.f - variant->step.f,
                                                variant->max_value.f );
            break;
        case WORKPHONE_PROPERTY_DOUBLE:
            variant->value.d = WORKPHONE_CLAMP( variant->min_value.d, variant->value.d - variant->step.d,
                                                variant->max_value.d );
            break;
        }
    }
    /* execute left button  */
    if( wp_do_button_symbol( ws, out, right, style->sym_right, behavior, &style->inc_button, in, font ) )
    {
        switch( variant->kind )
        {
        default:
            break;
        case WORKPHONE_PROPERTY_INT:
            variant->value.i = WORKPHONE_CLAMP( variant->min_value.i, variant->value.i + variant->step.i,
                                                variant->max_value.i );
            break;
        case WORKPHONE_PROPERTY_FLOAT:
            variant->value.f = WORKPHONE_CLAMP( variant->min_value.f, variant->value.f + variant->step.f,
                                                variant->max_value.f );
            break;
        case WORKPHONE_PROPERTY_DOUBLE:
            variant->value.d = WORKPHONE_CLAMP( variant->min_value.d, variant->value.d + variant->step.d,
                                                variant->max_value.d );
            break;
        }
    }
    if( !old && ( *state == WORKPHONE_PROPERTY_EDIT ) )
    {
        /* property has been activated so setup buffer */
        WORKPHONE_MEMCPY( buffer, dst, (wp_size)*length );
        *cursor = wp_utf_len( buffer, *length );
        *len = *length;
        length = len;
        dst = buffer;
        active = 0;
    }
    else
        active = ( *state == WORKPHONE_PROPERTY_EDIT );

    /* execute and run text edit field */
    wp_textedit_clear_state( text_edit, WORKPHONE_TEXT_EDIT_SINGLE_LINE, filters[filter] );
    text_edit->active = (wp_u8)active;
    text_edit->string.len = *length;
    text_edit->cursor = WORKPHONE_CLAMP( 0, *cursor, *length );
    text_edit->select_start = WORKPHONE_CLAMP( 0, *select_begin, *length );
    text_edit->select_end = WORKPHONE_CLAMP( 0, *select_end, *length );
    text_edit->string.buffer.allocated = (wp_size)*length;
    text_edit->string.buffer.memory.size = WORKPHONE_MAX_NUMBER_BUFFER;
    text_edit->string.buffer.memory.ptr = dst;
    text_edit->string.buffer.size = WORKPHONE_MAX_NUMBER_BUFFER;
    text_edit->mode = WORKPHONE_TEXT_EDIT_MODE_INSERT;
    wp_do_edit( ws, out, edit, (wp_s32)WORKPHONE_EDIT_FIELD | (wp_s32)WORKPHONE_EDIT_AUTO_SELECT,
                filters[filter], text_edit, &style->edit, ( *state == WORKPHONE_PROPERTY_EDIT ) ? in : 0,
                font );

    *length = text_edit->string.len;
    *cursor = text_edit->cursor;
    *select_begin = text_edit->select_start;
    *select_end = text_edit->select_end;
    if( text_edit->active && wp_input_is_key_pressed( in, WORKPHONE_KEY_ENTER ) )
        text_edit->active = wp_false;

    if( active && !text_edit->active )
    {
        /* property is now not active so convert edit text to value*/
        *state = WORKPHONE_PROPERTY_DEFAULT;
        wp_property_save( variant, buffer, *len );
    }
}
WORKPHONE_LIB struct wp_property_variant wp_property_variant_int( wp_s32 value, wp_s32 min_value,
                                                                  wp_s32 max_value, wp_s32 step )
{
    struct wp_property_variant result;
    result.kind = WORKPHONE_PROPERTY_INT;
    result.value.i = value;
    result.min_value.i = min_value;
    result.max_value.i = max_value;
    result.step.i = step;
    return result;
}
WORKPHONE_LIB struct wp_property_variant wp_property_variant_wp_f32( wp_f32 value, wp_f32 min_value,
                                                                     wp_f32 max_value, wp_f32 step )
{
    struct wp_property_variant result;
    result.kind = WORKPHONE_PROPERTY_FLOAT;
    result.value.f = value;
    result.min_value.f = min_value;
    result.max_value.f = max_value;
    result.step.f = step;
    return result;
}
WORKPHONE_LIB struct wp_property_variant wp_property_variant_wp_f64( wp_f64 value, wp_f64 min_value,
                                                                     wp_f64 max_value, wp_f64 step )
{
    struct wp_property_variant result;
    result.kind = WORKPHONE_PROPERTY_DOUBLE;
    result.value.d = value;
    result.min_value.d = min_value;
    result.max_value.d = max_value;
    result.step.d = step;
    return result;
}
WORKPHONE_LIB void wp_property( struct wp_context *ctx, const wp_c8 *name,
                                struct wp_property_variant *variant, wp_f32 inc_per_pixel,
                                const enum wp_property_filter filter )
{
    struct wp_window *win;
    struct wp_panel *layout;
    struct wp_input *in;
    const struct wp_style *style;

    struct wp_rect bounds;
    enum wp_widget_layout_states s;
    wp_bool hot;

    wp_s32 *state = 0;
    wp_hash hash = 0;
    wp_c8 *buffer = 0;
    wp_s32 *len = 0;
    wp_s32 *cursor = 0;
    wp_s32 *select_begin = 0;
    wp_s32 *select_end = 0;
    wp_s32 old_state;
    wp_s32 prev_state;

    wp_c8 dummy_buffer[WORKPHONE_MAX_NUMBER_BUFFER];
    wp_s32 dummy_state = WORKPHONE_PROPERTY_DEFAULT;
    wp_s32 dummy_length = 0;
    wp_s32 dummy_cursor = 0;
    wp_s32 dummy_select_begin = 0;
    wp_s32 dummy_select_end = 0;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    layout = win->layout;
    style = &ctx->style;
    s = wp_widget( &bounds, ctx );
    if( !s )
        return;

    /* calculate hash from name */
    if( name[0] == '#' )
    {
        hash = wp_murmur_hash( name, (wp_s32)wp_strlen( name ), win->property.seq++ );
        name++; /* special number hash */
    }
    else
        hash = wp_murmur_hash( name, (wp_s32)wp_strlen( name ), 42 );

    /* check if property is previously hot */
    if( win->property.prev_state == WORKPHONE_PROPERTY_EDIT && hash == win->property.prev_name )
    {
        wp_property_save( variant, win->property.prev_buffer, win->property.prev_length );
        win->property.prev_state = WORKPHONE_PROPERTY_DEFAULT;
    }

    /* check if property is currently hot item */
    hot = win->property.active && hash == win->property.name;
    if( hot )
    {
        buffer = win->property.buffer;
        len = &win->property.length;
        cursor = &win->property.cursor;
        state = &win->property.state;
        select_begin = &win->property.select_start;
        select_end = &win->property.select_end;
    }
    else
    {
        buffer = dummy_buffer;
        len = &dummy_length;
        cursor = &dummy_cursor;
        state = &dummy_state;
        select_begin = &dummy_select_begin;
        select_end = &dummy_select_end;
    }

    /* execute property widget */
    old_state = *state;
    prev_state = win->property.state;
    ctx->text_edit.clip = ctx->clip;
    in = ( ( s == WORKPHONE_WIDGET_ROM && !win->property.active ) ||
           layout->flags & WORKPHONE_WINDOW_ROM || s == WORKPHONE_WIDGET_DISABLED )
             ? 0
             : &ctx->input;
    wp_do_property( &ctx->last_widget_state, &win->buffer, bounds, name, variant, inc_per_pixel, buffer,
                    len, state, cursor, select_begin, select_end, &style->property, filter, in,
                    style->font, &ctx->text_edit, ctx->button_behavior );

    if( in && *state != WORKPHONE_PROPERTY_DEFAULT && !hot )
    {
        /* another property was active */
        if( win->property.active /* && hash != win->property.name */ )
        {
            win->property.prev_state = prev_state;
            win->property.prev_name = win->property.name;
            win->property.prev_length = win->property.length;
            WORKPHONE_MEMCPY( win->property.prev_buffer, win->property.buffer, win->property.length );
        }
        /* current property is now hot */
        win->property.active = 1;
        WORKPHONE_MEMCPY( win->property.buffer, buffer, (wp_size)*len );
        win->property.length = *len;
        win->property.cursor = *cursor;
        win->property.state = *state;
        win->property.name = hash;
        win->property.select_start = *select_begin;
        win->property.select_end = *select_end;
        win->edit.active = wp_true;
        if( *state == WORKPHONE_PROPERTY_DRAG )
        {
            ctx->input.mouse.grab = wp_true;
            ctx->input.mouse.grabbed = wp_true;
        }
    }
    /* check if previously active property is now inactive */
    if( *state == WORKPHONE_PROPERTY_DEFAULT && old_state != WORKPHONE_PROPERTY_DEFAULT )
    {
        if( old_state == WORKPHONE_PROPERTY_DRAG )
        {
            ctx->input.mouse.grab = wp_false;
            ctx->input.mouse.grabbed = wp_false;
            ctx->input.mouse.ungrab = wp_true;
        }
        win->property.select_start = 0;
        win->property.select_end = 0;
        win->property.active = 0;
        win->edit.active = wp_false;
    }
}
WORKPHONE_API void wp_property_int( struct wp_context *ctx, const wp_c8 *name, wp_s32 min, wp_s32 *val,
                                    wp_s32 max, wp_s32 step, wp_f32 inc_per_pixel )
{
    struct wp_property_variant variant;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( name );
    WORKPHONE_ASSERT( val );

    if( !ctx || !ctx->current || !name || !val )
        return;
    variant = wp_property_variant_int( *val, min, max, step );
    wp_property( ctx, name, &variant, inc_per_pixel, WORKPHONE_FILTER_INT );
    *val = variant.value.i;
}
WORKPHONE_API void wp_property_wp_f32( struct wp_context *ctx, const wp_c8 *name, wp_f32 min,
                                       wp_f32 *val, wp_f32 max, wp_f32 step, wp_f32 inc_per_pixel )
{
    struct wp_property_variant variant;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( name );
    WORKPHONE_ASSERT( val );

    if( !ctx || !ctx->current || !name || !val )
        return;
    variant = wp_property_variant_wp_f32( *val, min, max, step );
    wp_property( ctx, name, &variant, inc_per_pixel, WORKPHONE_FILTER_FLOAT );
    *val = variant.value.f;
}
WORKPHONE_API void wp_property_wp_f64( struct wp_context *ctx, const wp_c8 *name, wp_f64 min,
                                       wp_f64 *val, wp_f64 max, wp_f64 step, wp_f32 inc_per_pixel )
{
    struct wp_property_variant variant;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( name );
    WORKPHONE_ASSERT( val );

    if( !ctx || !ctx->current || !name || !val )
        return;
    variant = wp_property_variant_wp_f64( *val, min, max, step );
    wp_property( ctx, name, &variant, inc_per_pixel, WORKPHONE_FILTER_FLOAT );
    *val = variant.value.d;
}
WORKPHONE_API wp_s32 wp_propertyi( struct wp_context *ctx, const wp_c8 *name, wp_s32 min, wp_s32 val,
                                   wp_s32 max, wp_s32 step, wp_f32 inc_per_pixel )
{
    struct wp_property_variant variant;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( name );

    if( !ctx || !ctx->current || !name )
        return val;
    variant = wp_property_variant_int( val, min, max, step );
    wp_property( ctx, name, &variant, inc_per_pixel, WORKPHONE_FILTER_INT );
    val = variant.value.i;
    return val;
}
WORKPHONE_API wp_f32 wp_propertyf( struct wp_context *ctx, const wp_c8 *name, wp_f32 min, wp_f32 val,
                                   wp_f32 max, wp_f32 step, wp_f32 inc_per_pixel )
{
    struct wp_property_variant variant;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( name );

    if( !ctx || !ctx->current || !name )
        return val;
    variant = wp_property_variant_wp_f32( val, min, max, step );
    wp_property( ctx, name, &variant, inc_per_pixel, WORKPHONE_FILTER_FLOAT );
    val = variant.value.f;
    return val;
}
WORKPHONE_API wp_f64 wp_propertyd( struct wp_context *ctx, const wp_c8 *name, wp_f64 min, wp_f64 val,
                                   wp_f64 max, wp_f64 step, wp_f32 inc_per_pixel )
{
    struct wp_property_variant variant;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( name );

    if( !ctx || !ctx->current || !name )
        return val;
    variant = wp_property_variant_wp_f64( val, min, max, step );
    wp_property( ctx, name, &variant, inc_per_pixel, WORKPHONE_FILTER_FLOAT );
    val = variant.value.d;
    return val;
}
