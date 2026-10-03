#include "workphone_ui.h"
#include "workphone_font.h"
#include "workphone_context.h"

void wp_layout_set_min_row_height( struct wp_context *ctx, wp_f32 height )
{
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    layout = win->layout;
    layout->row.min_height = height;
}

void wp_layout_reset_min_row_height( struct wp_context *ctx )
{
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    layout = win->layout;
    layout->row.min_height = ctx->style.font->height;
    layout->row.min_height += ctx->style.text.padding.y * 2;
    layout->row.min_height += ctx->style.window.min_row_height_padding * 2;
}

wp_f32 wp_layout_row_calculate_usable_space( const struct wp_style *style, enum wp_panel_type type,
                                             wp_f32 total_space, wp_s32 columns )
{
    wp_f32 panel_spacing;
    wp_f32 panel_space;

    struct wp_vec2f spacing;

    WORKPHONE_UNUSED( type );

    spacing = style->window.spacing;

    /* calculate the usable panel space */
    panel_spacing = (wp_f32)WORKPHONE_MAX( columns - 1, 0 ) * spacing.x;
    panel_space = total_space - panel_spacing;
    return panel_space;
}

void wp_panel_layout( const struct wp_context *ctx, struct wp_window *win, wp_f32 height, wp_s32 cols )
{
    struct wp_panel *layout;
    const struct wp_style *style;
    struct wp_command_buffer *out;

    struct wp_vec2f item_spacing;
    struct wp_color color;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    /* prefetch some configuration data */
    layout = win->layout;
    style = &ctx->style;
    out = &win->buffer;
    color = style->window.background;
    item_spacing = style->window.spacing;

    /*  if one of these triggers you forgot to add an `if` condition around either
        a window, group, popup, combobox or contextual menu `begin` and `end` block.
        Example:
            if (wp_begin(...) {...} wp_end(...); or
            if (wp_group_begin(...) { wp_group_end(...);} */
    WORKPHONE_ASSERT( !( layout->flags & WORKPHONE_WINDOW_MINIMIZED ) );
    WORKPHONE_ASSERT( !( layout->flags & WORKPHONE_WINDOW_HIDDEN ) );
    WORKPHONE_ASSERT( !( layout->flags & WORKPHONE_WINDOW_CLOSED ) );

    /* update the current row and set the current row layout */
    layout->row.index = 0;
    layout->at_y += layout->row.height;
    layout->row.columns = cols;
    if( height == 0.0f )
        layout->row.height = WORKPHONE_MAX( height, layout->row.min_height ) + item_spacing.y;
    else
        layout->row.height = height + item_spacing.y;

    layout->row.item_offset = 0;
    if( layout->flags & WORKPHONE_WINDOW_DYNAMIC )
    {
        /* draw background for dynamic panels */
        struct wp_rect background;
        background.x = win->bounds.x;
        background.w = win->bounds.w;
        background.y = layout->at_y - 1.0f;
        background.h = layout->row.height + 1.0f;
        wp_fill_rect( out, background, 0, color );
    }
}

void wp_row_layout( struct wp_context *ctx, enum wp_layout_format fmt, wp_f32 height, wp_s32 cols,
                    wp_s32 width )
{
    /* update the current row and set the current row layout */
    struct wp_window *win;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    wp_panel_layout( ctx, win, height, cols );
    if( fmt == WORKPHONE_DYNAMIC )
        win->layout->row.type = WORKPHONE_LAYOUT_DYNAMIC_FIXED;
    else
        win->layout->row.type = WORKPHONE_LAYOUT_STATIC_FIXED;

    win->layout->row.ratio = 0;
    win->layout->row.filled = 0;
    win->layout->row.item_offset = 0;
    win->layout->row.item_width = (wp_f32)width;
}

wp_f32 wp_layout_ratio_from_pixel( const struct wp_context *ctx, wp_f32 pixel_width )
{
    struct wp_window *win;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( pixel_width );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;
    win = ctx->current;
    return WORKPHONE_CLAMP( 0.0f, pixel_width / win->bounds.x, 1.0f );
}

void wp_layout_row_dynamic( struct wp_context *ctx, wp_f32 height, wp_s32 cols )
{
    wp_row_layout( ctx, WORKPHONE_DYNAMIC, height, cols, 0 );
}

void wp_layout_row_static( struct wp_context *ctx, wp_f32 height, wp_s32 item_width, wp_s32 cols )
{
    wp_row_layout( ctx, WORKPHONE_STATIC, height, cols, item_width );
}

void wp_layout_row_begin( struct wp_context *ctx, enum wp_layout_format fmt, wp_f32 row_height,
                          wp_s32 cols )
{
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    layout = win->layout;
    wp_panel_layout( ctx, win, row_height, cols );
    if( fmt == WORKPHONE_DYNAMIC )
        layout->row.type = WORKPHONE_LAYOUT_DYNAMIC_ROW;
    else
        layout->row.type = WORKPHONE_LAYOUT_STATIC_ROW;

    layout->row.ratio = 0;
    layout->row.filled = 0;
    layout->row.item_width = 0;
    layout->row.item_offset = 0;
    layout->row.columns = cols;
}

void wp_layout_row_push( struct wp_context *ctx, wp_f32 ratio_or_width )
{
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    layout = win->layout;
    WORKPHONE_ASSERT( layout->row.type == WORKPHONE_LAYOUT_STATIC_ROW ||
                      layout->row.type == WORKPHONE_LAYOUT_DYNAMIC_ROW );
    if( layout->row.type != WORKPHONE_LAYOUT_STATIC_ROW &&
        layout->row.type != WORKPHONE_LAYOUT_DYNAMIC_ROW )
        return;

    if( layout->row.type == WORKPHONE_LAYOUT_DYNAMIC_ROW )
    {
        wp_f32 ratio = ratio_or_width;
        if( ( ratio + layout->row.filled ) > 1.0f )
            return;
        if( ratio > 0.0f )
            layout->row.item_width = WORKPHONE_SATURATE( ratio );
        else
            layout->row.item_width = 1.0f - layout->row.filled;
    }
    else
        layout->row.item_width = ratio_or_width;
}

void wp_layout_row_end( struct wp_context *ctx )
{
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    layout = win->layout;
    WORKPHONE_ASSERT( layout->row.type == WORKPHONE_LAYOUT_STATIC_ROW ||
                      layout->row.type == WORKPHONE_LAYOUT_DYNAMIC_ROW );
    if( layout->row.type != WORKPHONE_LAYOUT_STATIC_ROW &&
        layout->row.type != WORKPHONE_LAYOUT_DYNAMIC_ROW )
        return;
    layout->row.item_width = 0;
    layout->row.item_offset = 0;
}

void wp_layout_row( struct wp_context *ctx, enum wp_layout_format fmt, wp_f32 height, wp_s32 cols,
                    const wp_f32 *ratio )
{
    wp_s32 i;
    wp_s32 n_undef = 0;
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    layout = win->layout;
    wp_panel_layout( ctx, win, height, cols );
    if( fmt == WORKPHONE_DYNAMIC )
    {
        /* calculate width of undefined widget ratios */
        wp_f32 r = 0;
        layout->row.ratio = ratio;
        for( i = 0; i < cols; ++i )
        {
            if( ratio[i] < 0.0f )
                n_undef++;
            else
                r += ratio[i];
        }
        r = WORKPHONE_SATURATE( 1.0f - r );
        layout->row.type = WORKPHONE_LAYOUT_DYNAMIC;
        layout->row.item_width = ( r > 0 && n_undef > 0 ) ? ( r / (wp_f32)n_undef ) : 0;
    }
    else
    {
        layout->row.ratio = ratio;
        layout->row.type = WORKPHONE_LAYOUT_STATIC;
        layout->row.item_width = 0;
        layout->row.item_offset = 0;
    }

    layout->row.item_offset = 0;
    layout->row.filled = 0;
}

void wp_layout_row_template_begin( struct wp_context *ctx, wp_f32 height )
{
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );

    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    layout = win->layout;
    wp_panel_layout( ctx, win, height, 1 );
    layout->row.type = WORKPHONE_LAYOUT_TEMPLATE;
    layout->row.columns = 0;
    layout->row.ratio = 0;
    layout->row.item_width = 0;
    layout->row.item_height = 0;
    layout->row.item_offset = 0;
    layout->row.filled = 0;
    layout->row.item.x = 0;
    layout->row.item.y = 0;
    layout->row.item.w = 0;
    layout->row.item.h = 0;
}

void wp_layout_row_template_push_dynamic( struct wp_context *ctx )
{
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );

    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    layout = win->layout;

    WORKPHONE_ASSERT( layout->row.type == WORKPHONE_LAYOUT_TEMPLATE );
    WORKPHONE_ASSERT( layout->row.columns < WORKPHONE_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS );

    if( layout->row.type != WORKPHONE_LAYOUT_TEMPLATE )
        return;

    if( layout->row.columns >= WORKPHONE_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS )
        return;

    layout->row.templates[layout->row.columns++] = -1.0f;
}

void wp_layout_row_template_push_variable( struct wp_context *ctx, wp_f32 min_width )
{
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );

    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    layout = win->layout;

    WORKPHONE_ASSERT( layout->row.type == WORKPHONE_LAYOUT_TEMPLATE );
    WORKPHONE_ASSERT( layout->row.columns < WORKPHONE_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS );

    if( layout->row.type != WORKPHONE_LAYOUT_TEMPLATE )
        return;

    if( layout->row.columns >= WORKPHONE_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS )
        return;

    layout->row.templates[layout->row.columns++] = -min_width;
}

void wp_layout_row_template_push_static( struct wp_context *ctx, wp_f32 width )
{
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    layout = win->layout;
    WORKPHONE_ASSERT( layout->row.type == WORKPHONE_LAYOUT_TEMPLATE );
    WORKPHONE_ASSERT( layout->row.columns < WORKPHONE_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS );
    if( layout->row.type != WORKPHONE_LAYOUT_TEMPLATE )
        return;
    if( layout->row.columns >= WORKPHONE_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS )
        return;
    layout->row.templates[layout->row.columns++] = width;
}

void wp_layout_row_template_end( struct wp_context *ctx )
{
    struct wp_window *win;
    struct wp_panel *layout;

    wp_s32 i = 0;
    wp_s32 variable_count = 0;
    wp_s32 min_variable_count = 0;
    wp_f32 min_fixed_width = 0.0f;
    wp_f32 total_fixed_width = 0.0f;
    wp_f32 max_variable_width = 0.0f;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    layout = win->layout;
    WORKPHONE_ASSERT( layout->row.type == WORKPHONE_LAYOUT_TEMPLATE );
    if( layout->row.type != WORKPHONE_LAYOUT_TEMPLATE )
        return;
    for( i = 0; i < layout->row.columns; ++i )
    {
        wp_f32 width = layout->row.templates[i];
        if( width >= 0.0f )
        {
            total_fixed_width += width;
            min_fixed_width += width;
        }
        else if( width < -1.0f )
        {
            width = -width;
            total_fixed_width += width;
            max_variable_width = WORKPHONE_MAX( max_variable_width, width );
            variable_count++;
        }
        else
        {
            min_variable_count++;
            variable_count++;
        }
    }
    if( variable_count )
    {
        wp_f32 space = wp_layout_row_calculate_usable_space( &ctx->style, layout->type, layout->bounds.w,
                                                             layout->row.columns );
        wp_f32 var_width = ( WORKPHONE_MAX( space - min_fixed_width, 0.0f ) ) / (wp_f32)variable_count;
        wp_s32 enough_space = var_width >= max_variable_width;
        if( !enough_space )
            var_width = ( WORKPHONE_MAX( space - total_fixed_width, 0 ) ) / (wp_f32)min_variable_count;
        for( i = 0; i < layout->row.columns; ++i )
        {
            wp_f32 *width = &layout->row.templates[i];
            *width = ( *width >= 0.0f )                    ? *width
                     : ( *width < -1.0f && !enough_space ) ? -( *width )
                                                           : var_width;
        }
    }
}

void wp_layout_space_begin( struct wp_context *ctx, enum wp_layout_format fmt, wp_f32 height,
                            wp_s32 widget_count )
{
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    layout = win->layout;
    wp_panel_layout( ctx, win, height, widget_count );
    if( fmt == WORKPHONE_STATIC )
        layout->row.type = WORKPHONE_LAYOUT_STATIC_FREE;
    else
        layout->row.type = WORKPHONE_LAYOUT_DYNAMIC_FREE;

    layout->row.ratio = 0;
    layout->row.filled = 0;
    layout->row.item_width = 0;
    layout->row.item_offset = 0;
}

void wp_layout_space_end( struct wp_context *ctx )
{
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    layout = win->layout;
    layout->row.item_width = 0;
    layout->row.item_height = 0;
    layout->row.item_offset = 0;
    wp_zero( &layout->row.item, sizeof( layout->row.item ) );
}

void wp_layout_space_push( struct wp_context *ctx, struct wp_rect rect )
{
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    layout = win->layout;
    layout->row.item = rect;
}

struct wp_rect wp_layout_space_bounds( const struct wp_context *ctx )
{
    struct wp_rect ret;
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    win = ctx->current;
    layout = win->layout;

    ret.x = layout->clip.x;
    ret.y = layout->clip.y;
    ret.w = layout->clip.w;
    ret.h = layout->row.height;
    return ret;
}

struct wp_rect wp_layout_widget_bounds( const struct wp_context *ctx )
{
    struct wp_rect ret;
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    win = ctx->current;
    layout = win->layout;

    ret.x = layout->at_x;
    ret.y = layout->at_y;
    ret.w = layout->bounds.w - WORKPHONE_MAX( layout->at_x - layout->bounds.x, 0 );
    ret.h = layout->row.height;
    return ret;
}

struct wp_vec2f wp_layout_space_to_screen( const struct wp_context *ctx, struct wp_vec2f ret )
{
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    win = ctx->current;
    layout = win->layout;

    ret.x += layout->at_x - (wp_f32)*layout->offset_x;
    ret.y += layout->at_y - (wp_f32)*layout->offset_y;
    return ret;
}

struct wp_vec2f wp_layout_space_to_local( const struct wp_context *ctx, struct wp_vec2f ret )
{
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    win = ctx->current;
    layout = win->layout;

    ret.x += -layout->at_x + (wp_f32)*layout->offset_x;
    ret.y += -layout->at_y + (wp_f32)*layout->offset_y;
    return ret;
}

struct wp_rect wp_layout_space_rect_to_screen( const struct wp_context *ctx, struct wp_rect ret )
{
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    win = ctx->current;
    layout = win->layout;

    ret.x += layout->at_x - (wp_f32)*layout->offset_x;
    ret.y += layout->at_y - (wp_f32)*layout->offset_y;
    return ret;
}

struct wp_rect wp_layout_space_rect_to_local( const struct wp_context *ctx, struct wp_rect ret )
{
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    win = ctx->current;
    layout = win->layout;

    ret.x += -layout->at_x + (wp_f32)*layout->offset_x;
    ret.y += -layout->at_y + (wp_f32)*layout->offset_y;
    return ret;
}

void wp_panel_alloc_row( const struct wp_context *ctx, struct wp_window *win )
{
    struct wp_panel *layout = win->layout;
    struct wp_vec2f spacing = ctx->style.window.spacing;
    const wp_f32 row_height = layout->row.height - spacing.y;
    wp_panel_layout( ctx, win, row_height, layout->row.columns );
}

void wp_layout_widget_space( struct wp_rect *bounds, const struct wp_context *ctx, struct wp_window *win,
                             wp_s32 modify )
{
    struct wp_panel *layout;
    const struct wp_style *style;

    struct wp_vec2f spacing;

    wp_f32 item_offset = 0;
    wp_f32 item_width = 0;
    wp_f32 item_spacing = 0;
    wp_f32 panel_space = 0;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    layout = win->layout;
    style = &ctx->style;
    WORKPHONE_ASSERT( bounds );

    spacing = style->window.spacing;
    panel_space = wp_layout_row_calculate_usable_space( &ctx->style, layout->type, layout->bounds.w,
                                                        layout->row.columns );

#define WORKPHONE_FRAC( x ) \
    ( x - (wp_f32)(wp_s32)wp_roundf( x ) ) /* will be used to remove fookin gaps */
    /* calculate the width of one item inside the current layout space */
    switch( layout->row.type )
    {
    case WORKPHONE_LAYOUT_DYNAMIC_FIXED:
    {
        /* scaling fixed size widgets item width */
        wp_f32 w = WORKPHONE_MAX( 1.0f, panel_space ) / (wp_f32)layout->row.columns;
        item_offset = (wp_f32)layout->row.index * w;
        item_width = w + WORKPHONE_FRAC( item_offset );
        item_spacing = (wp_f32)layout->row.index * spacing.x;
    }
    break;
    case WORKPHONE_LAYOUT_DYNAMIC_ROW:
    {
        /* scaling single ratio widget width */
        wp_f32 w = layout->row.item_width * panel_space;
        item_offset = layout->row.item_offset;
        item_width = w + WORKPHONE_FRAC( item_offset );
        item_spacing = 0;

        if( modify )
        {
            layout->row.item_offset += w + spacing.x;
            layout->row.filled += layout->row.item_width;
            layout->row.index = 0;
        }
    }
    break;
    case WORKPHONE_LAYOUT_DYNAMIC_FREE:
    {
        /* panel width depended free widget placing */
        bounds->x = layout->at_x + ( layout->bounds.w * layout->row.item.x );
        bounds->x -= (wp_f32)*layout->offset_x;
        bounds->y = layout->at_y + ( layout->row.height * layout->row.item.y );
        bounds->y -= (wp_f32)*layout->offset_y;
        bounds->w = layout->bounds.w * layout->row.item.w + WORKPHONE_FRAC( bounds->x );
        bounds->h = layout->row.height * layout->row.item.h + WORKPHONE_FRAC( bounds->y );
        return;
    }
    case WORKPHONE_LAYOUT_DYNAMIC:
    {
        /* scaling arrays of panel width ratios for every widget */
        wp_f32 ratio, w;
        WORKPHONE_ASSERT( layout->row.ratio );
        ratio = ( layout->row.ratio[layout->row.index] < 0 ) ? layout->row.item_width
                                                             : layout->row.ratio[layout->row.index];

        w = ( ratio * panel_space );
        item_spacing = (wp_f32)layout->row.index * spacing.x;
        item_offset = layout->row.item_offset;
        item_width = w + WORKPHONE_FRAC( item_offset );

        if( modify )
        {
            layout->row.item_offset += w;
            layout->row.filled += ratio;
        }
    }
    break;
    case WORKPHONE_LAYOUT_STATIC_FIXED:
    {
        /* non-scaling fixed widgets item width */
        item_width = layout->row.item_width;
        item_offset = (wp_f32)layout->row.index * item_width;
        item_spacing = (wp_f32)layout->row.index * spacing.x;
    }
    break;
    case WORKPHONE_LAYOUT_STATIC_ROW:
    {
        /* scaling single ratio widget width */
        item_width = layout->row.item_width;
        item_offset = layout->row.item_offset;
        item_spacing = (wp_f32)layout->row.index * spacing.x;
        if( modify )
            layout->row.item_offset += item_width;
    }
    break;
    case WORKPHONE_LAYOUT_STATIC_FREE:
    {
        /* free widget placing */
        bounds->x = layout->at_x + layout->row.item.x;
        bounds->w = layout->row.item.w;
        if( ( ( bounds->x + bounds->w ) > layout->max_x ) && modify )
            layout->max_x = ( bounds->x + bounds->w );
        bounds->x -= (wp_f32)*layout->offset_x;
        bounds->y = layout->at_y + layout->row.item.y;
        bounds->y -= (wp_f32)*layout->offset_y;
        bounds->h = layout->row.item.h;
        return;
    }
    case WORKPHONE_LAYOUT_STATIC:
    {
        /* non-scaling array of panel pixel width for every widget */
        item_spacing = (wp_f32)layout->row.index * spacing.x;
        item_width = layout->row.ratio[layout->row.index];
        item_offset = layout->row.item_offset;
        if( modify )
            layout->row.item_offset += item_width;
    }
    break;
    case WORKPHONE_LAYOUT_TEMPLATE:
    {
        /* stretchy row layout with combined dynamic/static widget width*/
        wp_f32 w;
        WORKPHONE_ASSERT( layout->row.index < layout->row.columns );
        WORKPHONE_ASSERT( layout->row.index < WORKPHONE_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS );
        w = layout->row.templates[layout->row.index];
        item_offset = layout->row.item_offset;
        item_width = w + WORKPHONE_FRAC( item_offset );
        item_spacing = (wp_f32)layout->row.index * spacing.x;
        if( modify )
            layout->row.item_offset += w;
    }
    break;
#undef WORKPHONE_FRAC
    default:
        WORKPHONE_ASSERT( 0 );
        break;
    };

    /* set the bounds of the newly allocated widget */
    bounds->w = item_width;
    bounds->h = layout->row.height - spacing.y;
    bounds->y = layout->at_y - (wp_f32)*layout->offset_y;
    bounds->x = layout->at_x + item_offset + item_spacing;
    if( ( ( bounds->x + bounds->w ) > layout->max_x ) && modify )
        layout->max_x = bounds->x + bounds->w;
    bounds->x -= (wp_f32)*layout->offset_x;
}

void wp_panel_alloc_space( struct wp_rect *bounds, const struct wp_context *ctx )
{
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    /* check if the end of the row has been hit and begin new row if so */
    win = ctx->current;
    layout = win->layout;
    if( layout->row.index >= layout->row.columns )
        wp_panel_alloc_row( ctx, win );

    /* calculate widget position and size */
    wp_layout_widget_space( bounds, ctx, win, wp_true );
    layout->row.index++;
}

void wp_layout_peek( struct wp_rect *bounds, const struct wp_context *ctx )
{
    wp_f32 y;
    wp_s32 index;
    struct wp_window *win;
    struct wp_panel *layout;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
    {
        *bounds = wp_make_rect( 0, 0, 0, 0 );
        return;
    }

    win = ctx->current;
    layout = win->layout;
    y = layout->at_y;
    index = layout->row.index;
    if( layout->row.index >= layout->row.columns )
    {
        layout->at_y += layout->row.height;
        layout->row.index = 0;
    }
    wp_layout_widget_space( bounds, ctx, win, wp_false );
    if( !layout->row.index )
    {
        bounds->x -= layout->row.item_offset;
    }
    layout->at_y = y;
    layout->row.index = index;
}

void wp_spacer( struct wp_context *ctx )
{
    struct wp_rect dummy_rect = { 0, 0, 0, 0 };
    wp_panel_alloc_space( &dummy_rect, ctx );
}
