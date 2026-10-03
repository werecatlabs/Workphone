#include "workphone_context.h"
#include "workphone_style.h"
#include "workphone_widget.h"

WORKPHONE_API wp_bool wp_chart_begin_colored( struct wp_context *ctx, enum chart_type type,
                                              struct wp_color color, struct wp_color highlight,
                                              wp_s32 count, wp_f32 min_value, wp_f32 max_value )
{
    struct wp_window *win;
    struct wp_chart *wp_c8t;
    const struct wp_style *config;
    const struct wp_style_chart *style;

    const struct wp_style_item *background;
    struct wp_rect bounds = { 0, 0, 0, 0 };

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );

    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;
    if( !wp_widget( &bounds, ctx ) )
    {
        wp_c8t = (struct wp_chart *)ctx->current->layout->chart;
        wp_zero( wp_c8t, sizeof( *wp_c8t ) );
        return 0;
    }

    win = ctx->current;
    config = &ctx->style;
    wp_c8t = (struct wp_chart *)win->layout->chart;
    style = &config->chart;

    /* setup basic generic wp_c8t  */
    wp_zero( wp_c8t, sizeof( *wp_c8t ) );
    wp_c8t->x = bounds.x + style->padding.x;
    wp_c8t->y = bounds.y + style->padding.y;
    wp_c8t->w = bounds.w - 2 * style->padding.x;
    wp_c8t->h = bounds.h - 2 * style->padding.y;
    wp_c8t->w = WORKPHONE_MAX( wp_c8t->w, 2 * style->padding.x );
    wp_c8t->h = WORKPHONE_MAX( wp_c8t->h, 2 * style->padding.y );

    /* add first slot into wp_c8t */
    {
        struct chart_slot *slot = &wp_c8t->slots[wp_c8t->slot++];
        slot->type = ( wp_chart_type )type;
        slot->count = count;
        slot->color = wp_rgb_factor( color, style->color_factor );
        slot->highlight = highlight;
        slot->min = WORKPHONE_MIN( min_value, max_value );
        slot->max = WORKPHONE_MAX( min_value, max_value );
        slot->range = slot->max - slot->min;
        slot->show_markers = style->show_markers;
    }

    /* draw wp_c8t background */
    background = &style->background;

    switch( background->type )
    {
    case WORKPHONE_STYLE_ITEM_IMAGE:
        wp_draw_image( &win->buffer, bounds, &background->data.image,
                       wp_rgb_factor( wp_white, style->color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_NINE_SLICE:
        wp_draw_nine_slice( &win->buffer, bounds, &background->data.slice,
                            wp_rgb_factor( wp_white, style->color_factor ) );
        break;
    case WORKPHONE_STYLE_ITEM_COLOR:
        wp_fill_rect( &win->buffer, bounds, style->rounding,
                      wp_rgb_factor( style->border_color, style->color_factor ) );
        wp_fill_rect( &win->buffer, wp_shrink_make_rect( bounds, style->border ), style->rounding,
                      wp_rgb_factor( style->background.data.color, style->color_factor ) );
        break;
    }
    return 1;
}

WORKPHONE_API wp_bool wp_wp_c8t_begin( struct wp_context *ctx, const enum chart_type type, wp_s32 count,
                                       wp_f32 min_value, wp_f32 max_value )
{
    return wp_chart_begin_colored( ctx, type, ctx->style.chart.color, ctx->style.chart.selected_color,
                                   count, min_value, max_value );
}

WORKPHONE_API void wp_wp_c8t_add_slot_colored( struct wp_context *ctx, const enum chart_type type,
                                               struct wp_color color, struct wp_color highlight,
                                               wp_s32 count, wp_f32 min_value, wp_f32 max_value )
{
    const struct wp_style_chart *style;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    WORKPHONE_ASSERT( ctx->current->layout->chart->slot < WORKPHONE_CHART_MAX_SLOT );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;
    if( ( (struct wp_chart *)ctx->current->layout->chart )->slot >= WORKPHONE_CHART_MAX_SLOT )
        return;

    style = &ctx->style.chart;

    /* add another slot into the graph */
    {
        struct wp_chart *wp_c8t = (struct wp_chart *)ctx->current->layout->chart;
        struct chart_slot *slot = &wp_c8t->slots[wp_c8t->slot++];
        slot->type = ( wp_chart_type )type;
        slot->count = count;
        slot->color = wp_rgb_factor( color, style->color_factor );
        slot->highlight = highlight;
        slot->min = WORKPHONE_MIN( min_value, max_value );
        slot->max = WORKPHONE_MAX( min_value, max_value );
        slot->range = slot->max - slot->min;
        slot->show_markers = style->show_markers;
    }
}

WORKPHONE_API void wp_wp_c8t_add_slot( struct wp_context *ctx, const enum chart_type type, wp_s32 count,
                                       wp_f32 min_value, wp_f32 max_value )
{
    wp_wp_c8t_add_slot_colored( ctx, type, ctx->style.chart.color, ctx->style.chart.selected_color,
                                count, min_value, max_value );
}
WORKPHONE_INTERN wp_flags wp_wp_c8t_push_line( struct wp_context *ctx, struct wp_window *win,
                                               struct wp_chart *g, wp_f32 value, wp_s32 slot )
{
    struct wp_panel *layout = win->layout;
    const struct wp_input *i = ctx->current->widgets_disabled ? 0 : &ctx->input;
    struct wp_command_buffer *out = &win->buffer;

    wp_flags ret = 0;
    struct wp_vec2f cur;
    struct wp_rect bounds;
    struct wp_color color;
    wp_f32 step;
    wp_f32 range;
    wp_f32 ratio;

    WORKPHONE_ASSERT( slot >= 0 && slot < WORKPHONE_CHART_MAX_SLOT );
    step = g->w / (wp_f32)g->slots[slot].count;
    range = g->slots[slot].max - g->slots[slot].min;
    ratio = ( value - g->slots[slot].min ) / range;

    if( g->slots[slot].index == 0 )
    {
        /* first data point does not have a connection */
        g->slots[slot].last.x = g->x;
        g->slots[slot].last.y = ( g->y + g->h ) - ratio * (wp_f32)g->h;

        bounds.x = g->slots[slot].last.x - 2;
        bounds.y = g->slots[slot].last.y - 2;
        bounds.w = bounds.h = 4;

        color = g->slots[slot].color;
        if( !( layout->flags & WORKPHONE_WINDOW_ROM ) && i &&
            WORKPHONE_INBOX( i->mouse.pos.x, i->mouse.pos.y, g->slots[slot].last.x - 3,
                             g->slots[slot].last.y - 3, 6, 6 ) )
        {
            ret = wp_input_is_mouse_hovering_rect( i, bounds ) ? WORKPHONE_CHART_HOVERING : 0;
            ret |= ( i->mouse.buttons[WORKPHONE_BUTTON_LEFT].down &&
                     i->mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked )
                       ? WORKPHONE_CHART_CLICKED
                       : 0;
            color = g->slots[slot].highlight;
        }
        if( g->slots[slot].show_markers )
        {
            wp_fill_rect( out, bounds, 0, color );
        }
        g->slots[slot].index += 1;
        return ret;
    }

    /* draw a line between the last data point and the new one */
    color = g->slots[slot].color;
    cur.x = g->x + (wp_f32)( step * (wp_f32)g->slots[slot].index );
    cur.y = ( g->y + g->h ) - ( ratio * (wp_f32)g->h );
    wp_stroke_line( out, g->slots[slot].last.x, g->slots[slot].last.y, cur.x, cur.y, 1.0f, color );

    bounds.x = cur.x - 3;
    bounds.y = cur.y - 3;
    bounds.w = bounds.h = 6;

    /* user selection of current data point */
    if( !( layout->flags & WORKPHONE_WINDOW_ROM ) )
    {
        if( wp_input_is_mouse_hovering_rect( i, bounds ) )
        {
            ret = WORKPHONE_CHART_HOVERING;
            ret |= ( !i->mouse.buttons[WORKPHONE_BUTTON_LEFT].down &&
                     i->mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked )
                       ? WORKPHONE_CHART_CLICKED
                       : 0;
            color = g->slots[slot].highlight;
        }
    }
    if( g->slots[slot].show_markers )
    {
        wp_fill_rect( out, wp_make_rect( cur.x - 2, cur.y - 2, 4, 4 ), 0, color );
    }

    /* save current data point position */
    g->slots[slot].last.x = cur.x;
    g->slots[slot].last.y = cur.y;
    g->slots[slot].index += 1;
    return ret;
}

WORKPHONE_INTERN wp_flags wp_wp_c8t_push_column( const struct wp_context *ctx, struct wp_window *win,
                                                 struct wp_chart *wp_c8t, wp_f32 value, wp_s32 slot )
{
    struct wp_command_buffer *out = &win->buffer;
    const struct wp_input *in = ctx->current->widgets_disabled ? 0 : &ctx->input;
    struct wp_panel *layout = win->layout;

    wp_f32 ratio;
    wp_flags ret = 0;
    struct wp_color color;
    struct wp_rect item = { 0, 0, 0, 0 };

    WORKPHONE_ASSERT( slot >= 0 && slot < WORKPHONE_CHART_MAX_SLOT );
    if( wp_c8t->slots[slot].index >= wp_c8t->slots[slot].count )
        return wp_false;
    if( wp_c8t->slots[slot].count )
    {
        wp_f32 padding = (wp_f32)( wp_c8t->slots[slot].count - 1 );
        item.w = ( wp_c8t->w - padding ) / (wp_f32)( wp_c8t->slots[slot].count );
    }

    /* calculate bounds of current bar wp_c8t entry */
    color = wp_c8t->slots[slot].color;
    ;
    item.h = wp_c8t->h * WORKPHONE_ABS( ( value / wp_c8t->slots[slot].range ) );
    if( value >= 0 )
    {
        ratio = ( value + WORKPHONE_ABS( wp_c8t->slots[slot].min ) ) /
                WORKPHONE_ABS( wp_c8t->slots[slot].range );
        item.y = ( wp_c8t->y + wp_c8t->h ) - wp_c8t->h * ratio;
    }
    else
    {
        ratio = ( value - wp_c8t->slots[slot].max ) / wp_c8t->slots[slot].range;
        item.y = wp_c8t->y + ( wp_c8t->h * WORKPHONE_ABS( ratio ) ) - item.h;
    }
    item.x = wp_c8t->x + ( (wp_f32)wp_c8t->slots[slot].index * item.w );
    item.x = item.x + ( (wp_f32)wp_c8t->slots[slot].index );

    /* user wp_c8t bar selection */
    if( !( layout->flags & WORKPHONE_WINDOW_ROM ) && in &&
        WORKPHONE_INBOX( in->mouse.pos.x, in->mouse.pos.y, item.x, item.y, item.w, item.h ) )
    {
        ret = WORKPHONE_CHART_HOVERING;
        ret |= ( !in->mouse.buttons[WORKPHONE_BUTTON_LEFT].down &&
                 in->mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked )
                   ? WORKPHONE_CHART_CLICKED
                   : 0;
        color = wp_c8t->slots[slot].highlight;
    }
    wp_fill_rect( out, item, 0, color );
    wp_c8t->slots[slot].index += 1;
    return ret;
}
WORKPHONE_API wp_flags wp_wp_c8t_push_slot( struct wp_context *ctx, wp_f32 value, wp_s32 slot )
{
    wp_flags flags;
    struct wp_window *win;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( slot >= 0 && slot < WORKPHONE_CHART_MAX_SLOT );
    WORKPHONE_ASSERT( slot < ( (struct wp_chart *)ctx->current->layout->chart )->slot );
    if( !ctx || !ctx->current || slot >= WORKPHONE_CHART_MAX_SLOT )
        return wp_false;
    if( slot >= ( (struct wp_chart *)ctx->current->layout->chart )->slot )
        return wp_false;

    win = ctx->current;
    if( ( (struct wp_chart *)win->layout->chart )->slot < slot )
        return wp_false;
    switch( ( (struct wp_chart *)win->layout->chart )->slots[slot].type )
    {
    case WORKPHONE_CHART_LINES:
        flags = wp_wp_c8t_push_line( ctx, win, (struct wp_chart *)win->layout->chart, value, slot );
        break;
    case WORKPHONE_CHART_COLUMN:
        flags = wp_wp_c8t_push_column( ctx, win, (struct wp_chart *)win->layout->chart, value, slot );
        break;
    default:
    case WORKPHONE_CHART_MAX:
        flags = 0;
    }
    return flags;
}
WORKPHONE_API wp_flags wp_wp_c8t_push( struct wp_context *ctx, wp_f32 value )
{
    return wp_wp_c8t_push_slot( ctx, value, 0 );
}
WORKPHONE_API void wp_wp_c8t_end( struct wp_context *ctx )
{
    struct wp_window *win;
    struct wp_chart *wp_c8t;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return;

    win = ctx->current;
    wp_c8t = (struct wp_chart *)win->layout->chart;
    WORKPHONE_MEMSET( wp_c8t, 0, sizeof( *wp_c8t ) );
    return;
}
WORKPHONE_API void wp_plot( struct wp_context *ctx, enum chart_type type, const wp_f32 *values,
                            wp_s32 count, wp_s32 offset )
{
    wp_s32 i = 0;
    wp_f32 min_value;
    wp_f32 max_value;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( values );
    if( !ctx || !values || !count )
        return;

    min_value = values[offset];
    max_value = values[offset];
    for( i = 0; i < count; ++i )
    {
        min_value = WORKPHONE_MIN( values[i + offset], min_value );
        max_value = WORKPHONE_MAX( values[i + offset], max_value );
    }

    if( wp_wp_c8t_begin( ctx, type, count, min_value, max_value ) )
    {
        for( i = 0; i < count; ++i )
            wp_wp_c8t_push( ctx, values[i + offset] );
        wp_wp_c8t_end( ctx );
    }
}
WORKPHONE_API void wp_plot_function( struct wp_context *ctx, enum chart_type type, void *userdata,
                                     wp_f32 ( *value_getter )( void *user, wp_s32 index ), wp_s32 count,
                                     wp_s32 offset )
{
    wp_s32 i = 0;
    wp_f32 min_value;
    wp_f32 max_value;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( value_getter );
    if( !ctx || !value_getter || !count )
        return;

    max_value = min_value = value_getter( userdata, offset );
    for( i = 0; i < count; ++i )
    {
        wp_f32 value = value_getter( userdata, i + offset );
        min_value = WORKPHONE_MIN( value, min_value );
        max_value = WORKPHONE_MAX( value, max_value );
    }

    if( wp_wp_c8t_begin( ctx, type, count, min_value, max_value ) )
    {
        for( i = 0; i < count; ++i )
            wp_wp_c8t_push( ctx, value_getter( userdata, i + offset ) );
        wp_wp_c8t_end( ctx );
    }
}
