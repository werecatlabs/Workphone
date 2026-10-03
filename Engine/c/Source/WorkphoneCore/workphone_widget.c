#include "workphone_widget.h"
#include "workphone_context.h"
#include "workphone_layout.h"
#include "workphone_style.h"

struct wp_rect wp_widget_bounds( const struct wp_context *ctx )
{
    struct wp_rect bounds;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return wp_make_rect( 0, 0, 0, 0 );
    wp_layout_peek( &bounds, ctx );
    return bounds;
}

struct wp_vec2f wp_widget_position( const struct wp_context *ctx )
{
    struct wp_rect bounds;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return wp_make_vec2f( 0, 0 );

    wp_layout_peek( &bounds, ctx );
    return wp_make_vec2f( bounds.x, bounds.y );
}

struct wp_vec2f wp_widget_size( const struct wp_context *ctx )
{
    struct wp_rect bounds;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return wp_make_vec2f( 0, 0 );

    wp_layout_peek( &bounds, ctx );
    return wp_make_vec2f( bounds.w, bounds.h );
}

wp_f32 wp_widget_width( const struct wp_context *ctx )
{
    struct wp_rect bounds;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return 0;

    wp_layout_peek( &bounds, ctx );
    return bounds.w;
}

wp_f32 wp_widget_height( const struct wp_context *ctx )
{
    struct wp_rect bounds;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return 0;

    wp_layout_peek( &bounds, ctx );
    return bounds.h;
}

wp_bool wp_widget_is_hovered( const struct wp_context *ctx )
{
    struct wp_rect c, v;
    struct wp_rect bounds;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout ||
        ( ctx->active != ctx->current &&
          !( (wp_s32)ctx->current->layout->type & (wp_s32)WORKPHONE_PANEL_SET_POPUP ) ) )
        return 0;

    c = ctx->current->layout->clip;
    c.x = (wp_f32)( (wp_s32)c.x );
    c.y = (wp_f32)( (wp_s32)c.y );
    c.w = (wp_f32)( (wp_s32)c.w );
    c.h = (wp_f32)( (wp_s32)c.h );

    wp_layout_peek( &bounds, ctx );
    wp_unify( &v, &c, bounds.x, bounds.y, bounds.x + bounds.w, bounds.y + bounds.h );
    if( !WORKPHONE_INTERSECT( c.x, c.y, c.w, c.h, bounds.x, bounds.y, bounds.w, bounds.h ) )
        return 0;
    return wp_input_is_mouse_hovering_rect( &ctx->input, bounds );
}

wp_bool wp_widget_is_mouse_clicked( const struct wp_context *ctx, enum wp_buttons btn )
{
    struct wp_rect c, v;
    struct wp_rect bounds;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout ||
        ( ctx->active != ctx->current &&
          !( (wp_s32)ctx->current->layout->type & (wp_s32)WORKPHONE_PANEL_SET_POPUP ) ) )
        return 0;

    c = ctx->current->layout->clip;
    c.x = (wp_f32)( (wp_s32)c.x );
    c.y = (wp_f32)( (wp_s32)c.y );
    c.w = (wp_f32)( (wp_s32)c.w );
    c.h = (wp_f32)( (wp_s32)c.h );

    wp_layout_peek( &bounds, ctx );
    wp_unify( &v, &c, bounds.x, bounds.y, bounds.x + bounds.w, bounds.y + bounds.h );
    if( !WORKPHONE_INTERSECT( c.x, c.y, c.w, c.h, bounds.x, bounds.y, bounds.w, bounds.h ) )
        return 0;
    return wp_input_mouse_clicked( &ctx->input, btn, bounds );
}

wp_bool wp_widget_has_mouse_click_down( const struct wp_context *ctx, enum wp_buttons btn, wp_bool down )
{
    struct wp_rect c, v;
    struct wp_rect bounds;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout ||
        ( ctx->active != ctx->current &&
          !( (wp_s32)ctx->current->layout->type & (wp_s32)WORKPHONE_PANEL_SET_POPUP ) ) )
        return 0;

    c = ctx->current->layout->clip;
    c.x = (wp_f32)( (wp_s32)c.x );
    c.y = (wp_f32)( (wp_s32)c.y );
    c.w = (wp_f32)( (wp_s32)c.w );
    c.h = (wp_f32)( (wp_s32)c.h );

    wp_layout_peek( &bounds, ctx );
    wp_unify( &v, &c, bounds.x, bounds.y, bounds.x + bounds.w, bounds.y + bounds.h );
    if( !WORKPHONE_INTERSECT( c.x, c.y, c.w, c.h, bounds.x, bounds.y, bounds.w, bounds.h ) )
        return 0;
    return wp_input_has_mouse_click_down_in_rect( &ctx->input, btn, bounds, down );
}

enum wp_widget_layout_states wp_widget( struct wp_rect *bounds, const struct wp_context *ctx )
{
    struct wp_rect c, v;
    struct wp_window *win;
    struct wp_panel *layout;
    const struct wp_input *in;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return WORKPHONE_WIDGET_INVALID;

    /* allocate space and check if the widget needs to be updated and drawn */
    wp_panel_alloc_space( bounds, ctx );
    win = ctx->current;
    layout = win->layout;
    in = &ctx->input;
    c = layout->clip;

    /*  if one of these triggers you forgot to add an `if` condition around either
        a window, group, popup, combobox or contextual menu `begin` and `end` block.
        Example:
            if (wp_begin(...) {...} wp_end(...); or
            if (wp_group_begin(...) { wp_group_end(...);} */
    WORKPHONE_ASSERT( !( layout->flags & WORKPHONE_WINDOW_MINIMIZED ) );
    WORKPHONE_ASSERT( !( layout->flags & WORKPHONE_WINDOW_HIDDEN ) );
    WORKPHONE_ASSERT( !( layout->flags & WORKPHONE_WINDOW_CLOSED ) );

    /* need to convert to wp_s32 here to remove wp_f32ing powp_s32 errors */
    bounds->x = (wp_f32)( (wp_s32)bounds->x );
    bounds->y = (wp_f32)( (wp_s32)bounds->y );
    bounds->w = (wp_f32)( (wp_s32)bounds->w );
    bounds->h = (wp_f32)( (wp_s32)bounds->h );

    c.x = (wp_f32)( (wp_s32)c.x );
    c.y = (wp_f32)( (wp_s32)c.y );
    c.w = (wp_f32)( (wp_s32)c.w );
    c.h = (wp_f32)( (wp_s32)c.h );

    wp_unify( &v, &c, bounds->x, bounds->y, bounds->x + bounds->w, bounds->y + bounds->h );
    if( !WORKPHONE_INTERSECT( c.x, c.y, c.w, c.h, bounds->x, bounds->y, bounds->w, bounds->h ) )
        return WORKPHONE_WIDGET_INVALID;
    if( win->widgets_disabled )
        return WORKPHONE_WIDGET_DISABLED;
    if( !WORKPHONE_INBOX( in->mouse.pos.x, in->mouse.pos.y, v.x, v.y, v.w, v.h ) )
        return WORKPHONE_WIDGET_ROM;
    return WORKPHONE_WIDGET_VALID;
}

enum wp_widget_layout_states wp_widget_fitting( struct wp_rect *bounds, const struct wp_context *ctx,
                                                struct wp_vec2f item_padding )
{
    /* update the bounds to stand without padding  */
    enum wp_widget_layout_states state;
    WORKPHONE_UNUSED( item_padding );

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return WORKPHONE_WIDGET_INVALID;

    state = wp_widget( bounds, ctx );
    return state;
}

void wp_spacing( struct wp_context *ctx, wp_s32 cols )
{
    struct wp_window *win;
    struct wp_panel *layout;
    struct wp_rect none;
    wp_s32 i, index, rows;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    /* spacing over row boundaries */
    win = ctx->current;
    layout = win->layout;
    index = ( layout->row.index + cols ) % layout->row.columns;
    rows = ( layout->row.index + cols ) / layout->row.columns;
    if( rows )
    {
        for( i = 0; i < rows; ++i )
            wp_panel_alloc_row( ctx, win );
        cols = index;
    }
    /* non table layout need to allocate space */
    if( layout->row.type != WORKPHONE_LAYOUT_DYNAMIC_FIXED &&
        layout->row.type != WORKPHONE_LAYOUT_STATIC_FIXED )
    {
        for( i = 0; i < cols; ++i )
            wp_panel_alloc_space( &none, ctx );
    }
    layout->row.index = index;
}

void wp_widget_state_reset( wp_flags *state )
{
    if( *state & WORKPHONE_WIDGET_STATE_MODIFIED )
        *state = WORKPHONE_WIDGET_STATE_INACTIVE | WORKPHONE_WIDGET_STATE_MODIFIED;
    else
        *state = WORKPHONE_WIDGET_STATE_INACTIVE;
}

void wp_widget_disable_begin( struct wp_context *ctx )
{
    struct wp_window *win;
    struct wp_style *style;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );

    if( !ctx || !ctx->current )
        return;

    win = ctx->current;
    style = &ctx->style;

    win->widgets_disabled = wp_true;

    style->button.color_factor_text = style->button.disabled_factor;
    style->button.color_factor_background = style->button.disabled_factor;
    style->chart.color_factor = style->chart.disabled_factor;
    style->checkbox.color_factor = style->checkbox.disabled_factor;
    style->combo.color_factor = style->combo.disabled_factor;
    style->combo.button.color_factor_text = style->combo.button.disabled_factor;
    style->combo.button.color_factor_background = style->combo.button.disabled_factor;
    style->contextual_button.color_factor_text = style->contextual_button.disabled_factor;
    style->contextual_button.color_factor_background = style->contextual_button.disabled_factor;
    style->edit.color_factor = style->edit.disabled_factor;
    style->edit.scrollbar.color_factor = style->edit.scrollbar.disabled_factor;
    style->menu_button.color_factor_text = style->menu_button.disabled_factor;
    style->menu_button.color_factor_background = style->menu_button.disabled_factor;
    style->option.color_factor = style->option.disabled_factor;
    style->progress.color_factor = style->progress.disabled_factor;
    style->property.color_factor = style->property.disabled_factor;
    style->property.inc_button.color_factor_text = style->property.inc_button.disabled_factor;
    style->property.inc_button.color_factor_background = style->property.inc_button.disabled_factor;
    style->property.dec_button.color_factor_text = style->property.dec_button.disabled_factor;
    style->property.dec_button.color_factor_background = style->property.dec_button.disabled_factor;
    style->property.edit.color_factor = style->property.edit.disabled_factor;
    style->scrollh.color_factor = style->scrollh.disabled_factor;
    style->scrollh.inc_button.color_factor_text = style->scrollh.inc_button.disabled_factor;
    style->scrollh.inc_button.color_factor_background = style->scrollh.inc_button.disabled_factor;
    style->scrollh.dec_button.color_factor_text = style->scrollh.dec_button.disabled_factor;
    style->scrollh.dec_button.color_factor_background = style->scrollh.dec_button.disabled_factor;
    style->scrollv.color_factor = style->scrollv.disabled_factor;
    style->scrollv.inc_button.color_factor_text = style->scrollv.inc_button.disabled_factor;
    style->scrollv.inc_button.color_factor_background = style->scrollv.inc_button.disabled_factor;
    style->scrollv.dec_button.color_factor_text = style->scrollv.dec_button.disabled_factor;
    style->scrollv.dec_button.color_factor_background = style->scrollv.dec_button.disabled_factor;
    style->selectable.color_factor = style->selectable.disabled_factor;
    style->slider.color_factor = style->slider.disabled_factor;
    style->slider.inc_button.color_factor_text = style->slider.inc_button.disabled_factor;
    style->slider.inc_button.color_factor_background = style->slider.inc_button.disabled_factor;
    style->slider.dec_button.color_factor_text = style->slider.dec_button.disabled_factor;
    style->slider.dec_button.color_factor_background = style->slider.dec_button.disabled_factor;
    style->tab.color_factor = style->tab.disabled_factor;
    style->tab.node_maximize_button.color_factor_text = style->tab.node_maximize_button.disabled_factor;
    style->tab.node_minimize_button.color_factor_text = style->tab.node_minimize_button.disabled_factor;
    style->tab.tab_maximize_button.color_factor_text = style->tab.tab_maximize_button.disabled_factor;
    style->tab.tab_maximize_button.color_factor_background =
        style->tab.tab_maximize_button.disabled_factor;
    style->tab.tab_minimize_button.color_factor_text = style->tab.tab_minimize_button.disabled_factor;
    style->tab.tab_minimize_button.color_factor_background =
        style->tab.tab_minimize_button.disabled_factor;
    style->text.color_factor = style->text.disabled_factor;
}

void wp_widget_disable_end( struct wp_context *ctx )
{
    struct wp_window *win;
    struct wp_style *style;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );

    if( !ctx || !ctx->current )
        return;

    win = ctx->current;
    style = &ctx->style;

    win->widgets_disabled = wp_false;

    style->button.color_factor_text = 1.0f;
    style->button.color_factor_background = 1.0f;
    style->chart.color_factor = 1.0f;
    style->checkbox.color_factor = 1.0f;
    style->combo.color_factor = 1.0f;
    style->combo.button.color_factor_text = 1.0f;
    style->combo.button.color_factor_background = 1.0f;
    style->contextual_button.color_factor_text = 1.0f;
    style->contextual_button.color_factor_background = 1.0f;
    style->edit.color_factor = 1.0f;
    style->edit.scrollbar.color_factor = 1.0f;
    style->menu_button.color_factor_text = 1.0f;
    style->menu_button.color_factor_background = 1.0f;
    style->option.color_factor = 1.0f;
    style->progress.color_factor = 1.0f;
    style->property.color_factor = 1.0f;
    style->property.inc_button.color_factor_text = 1.0f;
    style->property.inc_button.color_factor_background = 1.0f;
    style->property.dec_button.color_factor_text = 1.0f;
    style->property.dec_button.color_factor_background = 1.0f;
    style->property.edit.color_factor = 1.0f;
    style->scrollh.color_factor = 1.0f;
    style->scrollh.inc_button.color_factor_text = 1.0f;
    style->scrollh.inc_button.color_factor_background = 1.0f;
    style->scrollh.dec_button.color_factor_text = 1.0f;
    style->scrollh.dec_button.color_factor_background = 1.0f;
    style->scrollv.color_factor = 1.0f;
    style->scrollv.inc_button.color_factor_text = 1.0f;
    style->scrollv.inc_button.color_factor_background = 1.0f;
    style->scrollv.dec_button.color_factor_text = 1.0f;
    style->scrollv.dec_button.color_factor_background = 1.0f;
    style->selectable.color_factor = 1.0f;
    style->slider.color_factor = 1.0f;
    style->slider.inc_button.color_factor_text = 1.0f;
    style->slider.inc_button.color_factor_background = 1.0f;
    style->slider.dec_button.color_factor_text = 1.0f;
    style->slider.dec_button.color_factor_background = 1.0f;
    style->tab.color_factor = 1.0f;
    style->tab.node_maximize_button.color_factor_text = 1.0f;
    style->tab.node_minimize_button.color_factor_text = 1.0f;
    style->tab.tab_maximize_button.color_factor_text = 1.0f;
    style->tab.tab_maximize_button.color_factor_background = 1.0f;
    style->tab.tab_minimize_button.color_factor_text = 1.0f;
    style->tab.tab_minimize_button.color_factor_background = 1.0f;
    style->text.color_factor = 1.0f;
}
