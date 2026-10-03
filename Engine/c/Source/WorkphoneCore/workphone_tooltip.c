#include "workphone.h"
#include "workphone_context.h"
#include "workphone_font.h"
#include "workphone_layout.h"
#include "workphone_style.h"
#include "workphone_text.h"

wp_bool wp_tooltip_begin( struct wp_context *ctx, wp_f32 width )
{
    wp_s32 x, y, w, h;
    struct wp_window *win;
    const struct wp_input *in;
    struct wp_rect bounds;
    wp_s32 ret;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    /* make sure that no nonblocking popup is currently active */
    win = ctx->current;
    in = &ctx->input;
    if( win->popup.win && ( (wp_s32)win->popup.type & (wp_s32)WORKPHONE_PANEL_SET_NONBLOCK ) )
        return 0;

    w = wp_iceilf( width );
    h = wp_iceilf( wp_null_rect.h );
    x = wp_ifloorf( in->mouse.pos.x + 1 ) - (wp_s32)win->layout->clip.x;
    y = wp_ifloorf( in->mouse.pos.y + 1 ) - (wp_s32)win->layout->clip.y;

    bounds.x = (wp_f32)x;
    bounds.y = (wp_f32)y;
    bounds.w = (wp_f32)w;
    bounds.h = (wp_f32)h;

    ret = wp_popup_begin( ctx, WORKPHONE_POPUP_DYNAMIC, "__##Tooltip##__",
                          WORKPHONE_WINDOW_NO_SCROLLBAR | WORKPHONE_WINDOW_BORDER, bounds );
    if( ret )
        win->layout->flags &= ~(wp_flags)WORKPHONE_WINDOW_ROM;
    win->popup.type = WORKPHONE_PANEL_TOOLTIP;
    ctx->current->layout->type = WORKPHONE_PANEL_TOOLTIP;
    return ret;
}

void wp_tooltip_end( struct wp_context *ctx )
{
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return;
    ctx->current->seq--;
    wp_popup_close( ctx );
    wp_popup_end( ctx );
}
void wp_tooltip( struct wp_context *ctx, const wp_c8 *text )
{
    const struct wp_style *style;
    struct wp_vec2f padding;

    wp_s32 text_len;
    wp_f32 text_width;
    wp_f32 text_height;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    WORKPHONE_ASSERT( text );
    if( !ctx || !ctx->current || !ctx->current->layout || !text )
        return;

    /* fetch configuration data */
    style = &ctx->style;
    padding = style->window.padding;

    /* calculate size of the text and tooltip */
    text_len = wp_strlen( text );
    text_width = style->font->width( style->font->userdata, style->font->height, text, text_len );
    text_width += ( 4 * padding.x );
    text_height = ( style->font->height + 2 * padding.y );

    /* execute tooltip and fill with text */
    if( wp_tooltip_begin( ctx, (wp_f32)text_width ) )
    {
        wp_layout_row_dynamic( ctx, (wp_f32)text_height, 1 );
        wp_text( ctx, text, text_len, WORKPHONE_TEXT_LEFT );
        wp_tooltip_end( ctx );
    }
}
#ifdef WORKPHONE_INCLUDE_STANDARD_VARARGS
void wp_tooltipf( struct wp_context *ctx, const wp_c8 *fmt, ... )
{
    va_list args;
    va_start( args, fmt );
    wp_tooltipfv( ctx, fmt, args );
    va_end( args );
}
void wp_tooltipfv( struct wp_context *ctx, const wp_c8 *fmt, va_list args )
{
    wp_c8 buf[256];
    wp_strfmt( buf, WORKPHONE_LEN( buf ), fmt, args );
    wp_tooltip( ctx, buf );
}
#endif
