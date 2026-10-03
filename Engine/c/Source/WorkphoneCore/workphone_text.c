#include "workphone_text.h"
#include "workphone_font.h"
#include "workphone_context.h"
#include "workphone_layout.h"
#include "workphone_panel.h"

WORKPHONE_LIB void wp_widget_text( struct wp_command_buffer *o, struct wp_rect b, const wp_c8 *string,
                                   wp_s32 len, const struct wp_text *t, wp_flags a,
                                   const struct wp_user_font *f )
{
    struct wp_rect label;
    wp_f32 text_width;

    WORKPHONE_ASSERT( o );
    WORKPHONE_ASSERT( t );
    if( !o || !t )
        return;

    b.h = WORKPHONE_MAX( b.h, 2 * t->padding.y );

    text_width = f->width( f->userdata, f->height, (const wp_c8 *)string, len );
    text_width += ( 2.0f * t->padding.x );

    /* use top-left alignment by default */
    if( !( a &
           ( WORKPHONE_TEXT_ALIGN_LEFT | WORKPHONE_TEXT_ALIGN_CENTERED | WORKPHONE_TEXT_ALIGN_RIGHT ) ) )
        a |= WORKPHONE_TEXT_ALIGN_LEFT;
    if( !( a &
           ( WORKPHONE_TEXT_ALIGN_TOP | WORKPHONE_TEXT_ALIGN_MIDDLE | WORKPHONE_TEXT_ALIGN_BOTTOM ) ) )
        a |= WORKPHONE_TEXT_ALIGN_TOP;

    /* align in x-axis */
    if( a & WORKPHONE_TEXT_ALIGN_LEFT )
    {
        label.x = b.x + t->padding.x;
        label.w = WORKPHONE_MAX( 0, b.w - 2 * t->padding.x );
    }
    else if( a & WORKPHONE_TEXT_ALIGN_CENTERED )
    {
        label.w = WORKPHONE_MAX( 1, 2 * t->padding.x + (wp_f32)text_width );
        label.x = ( b.x + t->padding.x + ( ( b.w - 2 * t->padding.x ) - label.w ) / 2 );
        label.x = WORKPHONE_MAX( b.x + t->padding.x, label.x );
        label.w = WORKPHONE_MIN( b.x + b.w, label.x + label.w );
        if( label.w >= label.x )
            label.w -= label.x;
    }
    else if( a & WORKPHONE_TEXT_ALIGN_RIGHT )
    {
        label.x = WORKPHONE_MAX( b.x + t->padding.x,
                                 ( b.x + b.w ) - ( 2 * t->padding.x + (wp_f32)text_width ) );
        label.w = (wp_f32)text_width + 2 * t->padding.x;
    }

    /* align in y-axis */
    if( a & WORKPHONE_TEXT_ALIGN_TOP )
    {
        label.y = b.y + t->padding.y;
        label.h = WORKPHONE_MIN( f->height, b.h - 2 * t->padding.y );
    }
    else if( a & WORKPHONE_TEXT_ALIGN_MIDDLE )
    {
        label.y = b.y + b.h / 2.0f - (wp_f32)f->height / 2.0f;
        label.h = WORKPHONE_MAX( b.h / 2.0f, b.h - ( b.h / 2.0f + f->height / 2.0f ) );
    }
    else if( a & WORKPHONE_TEXT_ALIGN_BOTTOM )
    {
        label.y = b.y + b.h - f->height;
        label.h = f->height;
    }

    wp_draw_text( o, label, (const wp_c8 *)string, len, f, t->background, t->text );
}
WORKPHONE_LIB void wp_widget_text_wrap( struct wp_command_buffer *o, struct wp_rect b,
                                        const wp_c8 *string, wp_s32 len, const struct wp_text *t,
                                        const struct wp_user_font *f )
{
    wp_f32 width;
    wp_s32 glyphs = 0;
    wp_s32 fitting = 0;
    wp_s32 done = 0;
    struct wp_rect line;
    struct wp_text text;
    WORKPHONE_INTERN wp_rune seperator[] = { ' ' };

    WORKPHONE_ASSERT( o );
    WORKPHONE_ASSERT( t );
    if( !o || !t )
        return;

    text.padding = wp_make_vec2f( 0, 0 );
    text.background = t->background;
    text.text = t->text;

    b.w = WORKPHONE_MAX( b.w, 2 * t->padding.x );
    b.h = WORKPHONE_MAX( b.h, 2 * t->padding.y );
    b.h = b.h - 2 * t->padding.y;

    line.x = b.x + t->padding.x;
    line.y = b.y + t->padding.y;
    line.w = b.w - 2 * t->padding.x;
    line.h = 2 * t->padding.y + f->height;

    fitting =
        wp_text_clamp( f, string, len, line.w, &glyphs, &width, seperator, WORKPHONE_LEN( seperator ) );
    while( done < len )
    {
        if( !fitting || line.y + line.h >= ( b.y + b.h ) )
            break;
        wp_widget_text( o, line, &string[done], fitting, &text, WORKPHONE_TEXT_LEFT, f );
        done += fitting;
        line.y += f->height + 2 * t->padding.y;
        fitting = wp_text_clamp( f, &string[done], len - done, line.w, &glyphs, &width, seperator,
                                 WORKPHONE_LEN( seperator ) );
    }
}
WORKPHONE_API void wp_text_colored( struct wp_context *ctx, const wp_c8 *str, wp_s32 len,
                                    wp_flags alignment, struct wp_color color )
{
    struct wp_window *win;
    const struct wp_style *style;

    struct wp_vec2f item_padding;
    struct wp_rect bounds;
    struct wp_text text;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    style = &ctx->style;
    wp_panel_alloc_space( &bounds, ctx );
    item_padding = style->text.padding;

    text.padding.x = item_padding.x;
    text.padding.y = item_padding.y;
    text.background = style->window.background;
    text.text = wp_rgb_factor( color, style->text.color_factor );
    wp_widget_text( &win->buffer, bounds, str, len, &text, alignment, style->font );
}
WORKPHONE_API void wp_text_wrap_colored( struct wp_context *ctx, const wp_c8 *str, wp_s32 len,
                                         struct wp_color color )
{
    struct wp_window *win;
    const struct wp_style *style;

    struct wp_vec2f item_padding;
    struct wp_rect bounds;
    struct wp_text text;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    style = &ctx->style;
    wp_panel_alloc_space( &bounds, ctx );
    item_padding = style->text.padding;

    text.padding.x = item_padding.x;
    text.padding.y = item_padding.y;
    text.background = style->window.background;
    text.text = wp_rgb_factor( color, style->text.color_factor );
    wp_widget_text_wrap( &win->buffer, bounds, str, len, &text, style->font );
}
#ifdef WORKPHONE_INCLUDE_STANDARD_VARARGS
WORKPHONE_API void wp_labelf_colored( struct wp_context *ctx, wp_flags flags, struct wp_color color,
                                      const wp_c8 *fmt, ... )
{
    va_list args;
    va_start( args, fmt );
    wp_labelfv_colored( ctx, flags, color, fmt, args );
    va_end( args );
}
WORKPHONE_API void wp_labelf_colored_wrap( struct wp_context *ctx, struct wp_color color,
                                           const wp_c8 *fmt, ... )
{
    va_list args;
    va_start( args, fmt );
    wp_labelfv_colored_wrap( ctx, color, fmt, args );
    va_end( args );
}
WORKPHONE_API void wp_labelf( struct wp_context *ctx, wp_flags flags, const wp_c8 *fmt, ... )
{
    va_list args;
    va_start( args, fmt );
    wp_labelfv( ctx, flags, fmt, args );
    va_end( args );
}
WORKPHONE_API void wp_labelf_wrap( struct wp_context *ctx, const wp_c8 *fmt, ... )
{
    va_list args;
    va_start( args, fmt );
    wp_labelfv_wrap( ctx, fmt, args );
    va_end( args );
}
WORKPHONE_API void wp_labelfv_colored( struct wp_context *ctx, wp_flags flags, struct wp_color color,
                                       const wp_c8 *fmt, va_list args )
{
    wp_c8 buf[256];
    wp_strfmt( buf, WORKPHONE_LEN( buf ), fmt, args );
    wp_label_colored( ctx, buf, flags, color );
}

WORKPHONE_API void wp_labelfv_colored_wrap( struct wp_context *ctx, struct wp_color color,
                                            const wp_c8 *fmt, va_list args )
{
    wp_c8 buf[256];
    wp_strfmt( buf, WORKPHONE_LEN( buf ), fmt, args );
    wp_label_colored_wrap( ctx, buf, color );
}

WORKPHONE_API void wp_labelfv( struct wp_context *ctx, wp_flags flags, const wp_c8 *fmt, va_list args )
{
    wp_c8 buf[256];
    wp_strfmt( buf, WORKPHONE_LEN( buf ), fmt, args );
    wp_label( ctx, buf, flags );
}

WORKPHONE_API void wp_labelfv_wrap( struct wp_context *ctx, const wp_c8 *fmt, va_list args )
{
    wp_c8 buf[256];
    wp_strfmt( buf, WORKPHONE_LEN( buf ), fmt, args );
    wp_label_wrap( ctx, buf );
}

WORKPHONE_API void wp_value_bool( struct wp_context *ctx, const wp_c8 *prefix, wp_s32 value )
{
    wp_labelf( ctx, WORKPHONE_TEXT_LEFT, "%s: %s", prefix, ( ( value ) ? "true" : "false" ) );
}
WORKPHONE_API void wp_value_int( struct wp_context *ctx, const wp_c8 *prefix, wp_s32 value )
{
    wp_labelf( ctx, WORKPHONE_TEXT_LEFT, "%s: %d", prefix, value );
}
WORKPHONE_API void wp_value_uint( struct wp_context *ctx, const wp_c8 *prefix, wp_u32 value )
{
    wp_labelf( ctx, WORKPHONE_TEXT_LEFT, "%s: %u", prefix, value );
}
WORKPHONE_API void wp_value_wp_f32( struct wp_context *ctx, const wp_c8 *prefix, wp_f32 value )
{
    wp_f64 wp_f64_value = (wp_f64)value;
    wp_labelf( ctx, WORKPHONE_TEXT_LEFT, "%s: %.3f", prefix, wp_f64_value );
}
WORKPHONE_API void wp_value_color_byte( struct wp_context *ctx, const wp_c8 *p, struct wp_color c )
{
    wp_labelf( ctx, WORKPHONE_TEXT_LEFT, "%s: (%d, %d, %d, %d)", p, c.r, c.g, c.b, c.a );
}
WORKPHONE_API void wp_value_color_wp_f32( struct wp_context *ctx, const wp_c8 *p, struct wp_color color )
{
    wp_f64 c[4];
    wp_color_dv( c, color );
    wp_labelf( ctx, WORKPHONE_TEXT_LEFT, "%s: (%.2f, %.2f, %.2f, %.2f)", p, c[0], c[1], c[2], c[3] );
}
WORKPHONE_API void wp_value_color_hex( struct wp_context *ctx, const wp_c8 *prefix,
                                       struct wp_color color )
{
    wp_c8 hex[16];
    wp_color_hex_rgba( hex, color );
    wp_labelf( ctx, WORKPHONE_TEXT_LEFT, "%s: %s", prefix, hex );
}
#endif
WORKPHONE_API void wp_text( struct wp_context *ctx, const wp_c8 *str, wp_s32 len, wp_flags alignment )
{
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return;
    wp_text_colored( ctx, str, len, alignment, ctx->style.text.color );
}
WORKPHONE_API void wp_text_wrap( struct wp_context *ctx, const wp_c8 *str, wp_s32 len )
{
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return;
    wp_text_wrap_colored( ctx, str, len, ctx->style.text.color );
}
WORKPHONE_API void wp_label( struct wp_context *ctx, const wp_c8 *str, wp_flags alignment )
{
    wp_text( ctx, str, wp_strlen( str ), alignment );
}
WORKPHONE_API void wp_label_colored( struct wp_context *ctx, const wp_c8 *str, wp_flags align,
                                     struct wp_color color )
{
    wp_text_colored( ctx, str, wp_strlen( str ), align, color );
}
WORKPHONE_API void wp_label_wrap( struct wp_context *ctx, const wp_c8 *str )
{
    wp_text_wrap( ctx, str, wp_strlen( str ) );
}
WORKPHONE_API void wp_label_colored_wrap( struct wp_context *ctx, const wp_c8 *str,
                                          struct wp_color color )
{
    wp_text_wrap_colored( ctx, str, wp_strlen( str ), color );
}
