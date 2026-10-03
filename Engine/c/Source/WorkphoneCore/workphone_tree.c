#include "workphone.h"
#include "workphone_button.h"
#include "workphone_context.h"
#include "workphone_font.h"
#include "workphone_image.h"
#include "workphone_layout.h"
#include "workphone_selectable.h"
#include "workphone_style.h"
#include "workphone_table.h"
#include "workphone_util.h"
#include "workphone_widget.h"

wp_s32 wp_tree_state_base( struct wp_context *ctx, enum wp_tree_type type, struct wp_image *img,
                           const wp_c8 *title, enum wp_collapse_states *state )
{
    struct wp_window *win;
    struct wp_panel *layout;
    const struct wp_style *style;
    struct wp_command_buffer *out;
    const struct wp_input *in;
    const struct wp_style_button *button;
    enum wp_symbol_type symbol;
    wp_f32 row_height;

    struct wp_vec2f item_spacing;
    struct wp_rect header = { 0, 0, 0, 0 };
    struct wp_rect sym = { 0, 0, 0, 0 };
    struct wp_text text;

    wp_flags ws = 0;
    enum wp_widget_layout_states widget_state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    /* cache some data */
    win = ctx->current;
    layout = win->layout;
    out = &win->buffer;
    style = &ctx->style;
    item_spacing = style->window.spacing;

    /* calculate header bounds and draw background */
    row_height = style->font->height + 2 * style->tab.padding.y;
    wp_layout_set_min_row_height( ctx, row_height );
    wp_layout_row_dynamic( ctx, row_height, 1 );
    wp_layout_reset_min_row_height( ctx );

    widget_state = wp_widget( &header, ctx );
    if( type == WORKPHONE_TREE_TAB )
    {
        const struct wp_style_item *background = &style->tab.background;

        switch( background->type )
        {
        case WORKPHONE_STYLE_ITEM_IMAGE:
            wp_draw_image( out, header, &background->data.image,
                           wp_rgb_factor( wp_white, style->tab.color_factor ) );
            break;
        case WORKPHONE_STYLE_ITEM_NINE_SLICE:
            wp_draw_nine_slice( out, header, &background->data.slice,
                                wp_rgb_factor( wp_white, style->tab.color_factor ) );
            break;
        case WORKPHONE_STYLE_ITEM_COLOR:
            wp_fill_rect( out, header, 0,
                          wp_rgb_factor( style->tab.border_color, style->tab.color_factor ) );
            wp_fill_rect( out, wp_shrink_make_rect( header, style->tab.border ), style->tab.rounding,
                          wp_rgb_factor( background->data.color, style->tab.color_factor ) );
            break;
        }
    }
    else
        text.background = style->window.background;

    /* update node state */
    in = ( !( layout->flags & WORKPHONE_WINDOW_ROM ) ) ? &ctx->input : 0;
    in = ( in && widget_state == WORKPHONE_WIDGET_VALID ) ? &ctx->input : 0;
    if( wp_button_behavior( &ws, header, in, WORKPHONE_BUTTON_DEFAULT ) )
        *state = ( *state == WORKPHONE_MAXIMIZED ) ? WORKPHONE_MINIMIZED : WORKPHONE_MAXIMIZED;

    /* select correct button style */
    if( *state == WORKPHONE_MAXIMIZED )
    {
        symbol = style->tab.sym_maximize;
        if( type == WORKPHONE_TREE_TAB )
            button = &style->tab.tab_maximize_button;
        else
            button = &style->tab.node_maximize_button;
    }
    else
    {
        symbol = style->tab.sym_minimize;
        if( type == WORKPHONE_TREE_TAB )
            button = &style->tab.tab_minimize_button;
        else
            button = &style->tab.node_minimize_button;
    }

    { /* draw triangle button */
        sym.w = sym.h = style->font->height;
        sym.y = header.y + style->tab.padding.y;
        sym.x = header.x + style->tab.padding.x;
        wp_do_button_symbol( &ws, &win->buffer, sym, symbol, WORKPHONE_BUTTON_DEFAULT, button, 0,
                             style->font );

        if( img )
        {
            /* draw optional image icon */
            sym.x = sym.x + sym.w + 4 * item_spacing.x;
            wp_draw_image( &win->buffer, sym, img, wp_white );
            sym.w = style->font->height + style->tab.spacing.x;
        }
    }

    { /* draw label */
        struct wp_rect label;
        header.w = WORKPHONE_MAX( header.w, sym.w + item_spacing.x );
        label.x = sym.x + sym.w + item_spacing.x;
        label.y = sym.y;
        label.w = header.w - ( sym.w + item_spacing.y + style->tab.indent );
        label.h = style->font->height;
        text.text = wp_rgb_factor( style->tab.text, style->tab.color_factor );
        text.padding = wp_make_vec2f( 0, 0 );
        wp_widget_text( out, label, title, wp_strlen( title ), &text, WORKPHONE_TEXT_LEFT, style->font );
    }

    /* increase x-axis cursor widget position pointer */
    if( *state == WORKPHONE_MAXIMIZED )
    {
        layout->at_x = header.x + (wp_f32)*layout->offset_x + style->tab.indent;
        layout->bounds.w = WORKPHONE_MAX( layout->bounds.w, style->tab.indent );
        layout->bounds.w -= ( style->tab.indent + style->window.padding.x );
        layout->row.tree_depth++;
        return wp_true;
    }
    else
        return wp_false;
}
wp_s32 wp_tree_base( struct wp_context *ctx, enum wp_tree_type type, struct wp_image *img,
                     const wp_c8 *title, enum wp_collapse_states initial_state, const wp_c8 *hash,
                     wp_s32 len, wp_s32 line )
{
    struct wp_window *win = ctx->current;
    wp_s32 title_len = 0;
    wp_hash tree_hash = 0;
    wp_u32 *state = 0;

    /* retrieve tree state from internal widget state tables */
    if( !hash )
    {
        title_len = (wp_s32)wp_strlen( title );
        tree_hash = wp_murmur_hash( title, (wp_s32)title_len, (wp_hash)line );
    }
    else
        tree_hash = wp_murmur_hash( hash, len, (wp_hash)line );
    state = wp_find_value( win, tree_hash );
    if( !state )
    {
        state = wp_add_value( ctx, win, tree_hash, 0 );
        *state = initial_state;
    }
    return wp_tree_state_base( ctx, type, img, title, (enum wp_collapse_states *)state );
}
WORKPHONE_API wp_bool wp_tree_state_push( struct wp_context *ctx, enum wp_tree_type type,
                                          const wp_c8 *title, enum wp_collapse_states *state )
{
    return wp_tree_state_base( ctx, type, 0, title, state );
}
WORKPHONE_API wp_bool wp_tree_state_image_push( struct wp_context *ctx, enum wp_tree_type type,
                                                struct wp_image img, const wp_c8 *title,
                                                enum wp_collapse_states *state )
{
    return wp_tree_state_base( ctx, type, &img, title, state );
}
WORKPHONE_API void wp_tree_state_pop( struct wp_context *ctx )
{
    struct wp_window *win = 0;
    struct wp_panel *layout = 0;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    layout = win->layout;
    layout->at_x -= ctx->style.tab.indent + (wp_f32)*layout->offset_x;
    layout->bounds.w += ctx->style.tab.indent + ctx->style.window.padding.x;
    WORKPHONE_ASSERT( layout->row.tree_depth );
    layout->row.tree_depth--;
}
WORKPHONE_API wp_bool wp_tree_push_hashed( struct wp_context *ctx, enum wp_tree_type type,
                                           const wp_c8 *title, enum wp_collapse_states initial_state,
                                           const wp_c8 *hash, wp_s32 len, wp_s32 line )
{
    return wp_tree_base( ctx, type, 0, title, initial_state, hash, len, line );
}
WORKPHONE_API wp_bool wp_tree_image_push_hashed( struct wp_context *ctx, enum wp_tree_type type,
                                                 struct wp_image img, const wp_c8 *title,
                                                 enum wp_collapse_states initial_state,
                                                 const wp_c8 *hash, wp_s32 len, wp_s32 seed )
{
    return wp_tree_base( ctx, type, &img, title, initial_state, hash, len, seed );
}
WORKPHONE_API void wp_tree_pop( struct wp_context *ctx )
{
    wp_tree_state_pop( ctx );
}
wp_s32 wp_tree_element_image_push_hashed_base( struct wp_context *ctx, enum wp_tree_type type,
                                               struct wp_image *img, const wp_c8 *title,
                                               wp_s32 title_len, enum wp_collapse_states *state,
                                               wp_bool *selected )
{
    struct wp_window *win;
    struct wp_panel *layout;
    const struct wp_style *style;
    struct wp_command_buffer *out;
    const struct wp_input *in;
    const struct wp_style_button *button;
    enum wp_symbol_type symbol;
    wp_f32 row_height;
    struct wp_vec2f padding;

    wp_s32 text_len;
    wp_f32 text_width;

    struct wp_vec2f item_spacing;
    struct wp_rect header = { 0, 0, 0, 0 };
    struct wp_rect sym = { 0, 0, 0, 0 };

    wp_flags ws = 0;
    enum wp_widget_layout_states widget_state;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    /* cache some data */
    win = ctx->current;
    layout = win->layout;
    out = &win->buffer;
    style = &ctx->style;
    item_spacing = style->window.spacing;
    padding = style->selectable.padding;

    /* calculate header bounds and draw background */
    row_height = style->font->height + 2 * style->tab.padding.y;
    wp_layout_set_min_row_height( ctx, row_height );
    wp_layout_row_dynamic( ctx, row_height, 1 );
    wp_layout_reset_min_row_height( ctx );

    widget_state = wp_widget( &header, ctx );
    if( type == WORKPHONE_TREE_TAB )
    {
        const struct wp_style_item *background = &style->tab.background;

        switch( background->type )
        {
        case WORKPHONE_STYLE_ITEM_IMAGE:
            wp_draw_image( out, header, &background->data.image,
                           wp_rgb_factor( wp_white, style->tab.color_factor ) );
            break;
        case WORKPHONE_STYLE_ITEM_NINE_SLICE:
            wp_draw_nine_slice( out, header, &background->data.slice,
                                wp_rgb_factor( wp_white, style->tab.color_factor ) );
            break;
        case WORKPHONE_STYLE_ITEM_COLOR:
            wp_fill_rect( out, header, 0,
                          wp_rgb_factor( style->tab.border_color, style->tab.color_factor ) );
            wp_fill_rect( out, wp_shrink_make_rect( header, style->tab.border ), style->tab.rounding,
                          wp_rgb_factor( background->data.color, style->tab.color_factor ) );

            break;
        }
    }

    in = ( !( layout->flags & WORKPHONE_WINDOW_ROM ) ) ? &ctx->input : 0;
    in = ( in && widget_state == WORKPHONE_WIDGET_VALID ) ? &ctx->input : 0;

    /* select correct button style */
    if( *state == WORKPHONE_MAXIMIZED )
    {
        symbol = style->tab.sym_maximize;
        if( type == WORKPHONE_TREE_TAB )
            button = &style->tab.tab_maximize_button;
        else
            button = &style->tab.node_maximize_button;
    }
    else
    {
        symbol = style->tab.sym_minimize;
        if( type == WORKPHONE_TREE_TAB )
            button = &style->tab.tab_minimize_button;
        else
            button = &style->tab.node_minimize_button;
    }
    { /* draw triangle button */
        sym.w = sym.h = style->font->height;
        sym.y = header.y + style->tab.padding.y;
        sym.x = header.x + style->tab.padding.x;
        if( wp_do_button_symbol( &ws, &win->buffer, sym, symbol, WORKPHONE_BUTTON_DEFAULT, button, in,
                                 style->font ) )
            *state = ( *state == WORKPHONE_MAXIMIZED ) ? WORKPHONE_MINIMIZED : WORKPHONE_MAXIMIZED;
    }

    /* draw label */
    {
        wp_flags dummy = 0;
        struct wp_rect label;
        /* calculate size of the text and tooltip */
        text_len = wp_strlen( title );
        text_width = style->font->width( style->font->userdata, style->font->height, title, text_len );
        text_width += ( 4 * padding.x );

        header.w = WORKPHONE_MAX( header.w, sym.w + item_spacing.x );
        label.x = sym.x + sym.w + item_spacing.x;
        label.y = sym.y;
        label.w = WORKPHONE_MIN( header.w - ( sym.w + item_spacing.y + style->tab.indent ), text_width );
        label.h = style->font->height;

        if( img )
        {
            wp_do_selectable_image( &dummy, &win->buffer, label, title, title_len, WORKPHONE_TEXT_LEFT,
                                    selected, img, &style->selectable, in, style->font );
        }
        else
            wp_do_selectable( &dummy, &win->buffer, label, title, title_len, WORKPHONE_TEXT_LEFT,
                              selected, &style->selectable, in, style->font );
    }
    /* increase x-axis cursor widget position pointer */
    if( *state == WORKPHONE_MAXIMIZED )
    {
        layout->at_x = header.x + (wp_f32)*layout->offset_x + style->tab.indent;
        layout->bounds.w = WORKPHONE_MAX( layout->bounds.w, style->tab.indent );
        layout->bounds.w -= ( style->tab.indent + style->window.padding.x );
        layout->row.tree_depth++;
        return wp_true;
    }
    else
        return wp_false;
}
wp_s32 wp_tree_element_base( struct wp_context *ctx, enum wp_tree_type type, struct wp_image *img,
                             const wp_c8 *title, enum wp_collapse_states initial_state,
                             wp_bool *selected, const wp_c8 *hash, wp_s32 len, wp_s32 line )
{
    struct wp_window *win = ctx->current;
    wp_s32 title_len = 0;
    wp_hash tree_hash = 0;
    wp_u32 *state = 0;

    /* retrieve tree state from internal widget state tables */
    if( !hash )
    {
        title_len = (wp_s32)wp_strlen( title );
        tree_hash = wp_murmur_hash( title, (wp_s32)title_len, (wp_hash)line );
    }
    else
        tree_hash = wp_murmur_hash( hash, len, (wp_hash)line );
    state = wp_find_value( win, tree_hash );
    if( !state )
    {
        state = wp_add_value( ctx, win, tree_hash, 0 );
        *state = initial_state;
    }
    return wp_tree_element_image_push_hashed_base( ctx, type, img, title, wp_strlen( title ),
                                                   (enum wp_collapse_states *)state, selected );
}
WORKPHONE_API wp_bool wp_tree_element_push_hashed( struct wp_context *ctx, enum wp_tree_type type,
                                                   const wp_c8 *title,
                                                   enum wp_collapse_states initial_state,
                                                   wp_bool *selected, const wp_c8 *hash, wp_s32 len,
                                                   wp_s32 seed )
{
    return wp_tree_element_base( ctx, type, 0, title, initial_state, selected, hash, len, seed );
}
WORKPHONE_API wp_bool wp_tree_element_image_push_hashed( struct wp_context *ctx, enum wp_tree_type type,
                                                         struct wp_image img, const wp_c8 *title,
                                                         enum wp_collapse_states initial_state,
                                                         wp_bool *selected, const wp_c8 *hash,
                                                         wp_s32 len, wp_s32 seed )
{
    return wp_tree_element_base( ctx, type, &img, title, initial_state, selected, hash, len, seed );
}
WORKPHONE_API void wp_tree_element_pop( struct wp_context *ctx )
{
    wp_tree_state_pop( ctx );
}
