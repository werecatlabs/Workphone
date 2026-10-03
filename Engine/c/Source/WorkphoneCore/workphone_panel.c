#include "workphone_panel.h"
#include "workphone_button.h"
#include "workphone_context.h"
#include "workphone_font.h"
#include "workphone_layout.h"
#include "workphone_menu.h"
#include "workphone_page_element.h"
#include "workphone_scrollbar.h"
#include "workphone_style.h"
#include "workphone_window.h"

WORKPHONE_LIB void *wp_create_panel( struct wp_context *ctx )
{
    struct wp_page_element *elem;
    elem = wp_create_page_element( ctx );
    if( !elem )
        return 0;
    wp_zero_struct( *elem );
    return &elem->data.pan;
}
WORKPHONE_LIB void wp_free_panel( struct wp_context *ctx, struct wp_panel *pan )
{
    union wp_page_data *pd = WORKPHONE_CONTAINER_OF( pan, union wp_page_data, pan );
    struct wp_page_element *pe = WORKPHONE_CONTAINER_OF( pd, struct wp_page_element, data );
    wp_free_page_element( ctx, pe );
}
WORKPHONE_LIB wp_bool wp_panel_has_header( wp_flags flags, const wp_c8 *title )
{
    wp_bool active = 0;
    active = ( flags & ( WORKPHONE_WINDOW_CLOSABLE | WORKPHONE_WINDOW_MINIMIZABLE ) );
    active = active || ( flags & WORKPHONE_WINDOW_TITLE );
    active = active && !( flags & WORKPHONE_WINDOW_HIDDEN ) && title;
    return active;
}
WORKPHONE_LIB struct wp_vec2f wp_panel_get_padding( const struct wp_style *style,
                                                   enum wp_panel_type type )
{
    switch( type )
    {
    default:
    case WORKPHONE_PANEL_WINDOW:
        return style->window.padding;
    case WORKPHONE_PANEL_GROUP:
        return style->window.group_padding;
    case WORKPHONE_PANEL_POPUP:
        return style->window.popup_padding;
    case WORKPHONE_PANEL_CONTEXTUAL:
        return style->window.contextual_padding;
    case WORKPHONE_PANEL_COMBO:
        return style->window.combo_padding;
    case WORKPHONE_PANEL_MENU:
        return style->window.menu_padding;
    case WORKPHONE_PANEL_TOOLTIP:
        return style->window.menu_padding;
    }
}
WORKPHONE_LIB wp_f32 wp_panel_get_border( const struct wp_style *style, wp_flags flags,
                                          enum wp_panel_type type )
{
    if( flags & WORKPHONE_WINDOW_BORDER )
    {
        switch( type )
        {
        default:
        case WORKPHONE_PANEL_WINDOW:
            return style->window.border;
        case WORKPHONE_PANEL_GROUP:
            return style->window.group_border;
        case WORKPHONE_PANEL_POPUP:
            return style->window.popup_border;
        case WORKPHONE_PANEL_CONTEXTUAL:
            return style->window.contextual_border;
        case WORKPHONE_PANEL_COMBO:
            return style->window.combo_border;
        case WORKPHONE_PANEL_MENU:
            return style->window.menu_border;
        case WORKPHONE_PANEL_TOOLTIP:
            return style->window.menu_border;
        }
    }
    else
        return 0;
}
WORKPHONE_LIB struct wp_color wp_panel_get_border_color( const struct wp_style *style,
                                                         enum wp_panel_type type )
{
    switch( type )
    {
    default:
    case WORKPHONE_PANEL_WINDOW:
        return style->window.border_color;
    case WORKPHONE_PANEL_GROUP:
        return style->window.group_border_color;
    case WORKPHONE_PANEL_POPUP:
        return style->window.popup_border_color;
    case WORKPHONE_PANEL_CONTEXTUAL:
        return style->window.contextual_border_color;
    case WORKPHONE_PANEL_COMBO:
        return style->window.combo_border_color;
    case WORKPHONE_PANEL_MENU:
        return style->window.menu_border_color;
    case WORKPHONE_PANEL_TOOLTIP:
        return style->window.menu_border_color;
    }
}
WORKPHONE_LIB wp_bool wp_panel_is_sub( enum wp_panel_type type )
{
    return ( (wp_s32)type & (wp_s32)WORKPHONE_PANEL_SET_SUB ) ? 1 : 0;
}
WORKPHONE_LIB wp_bool wp_panel_is_nonblock( enum wp_panel_type type )
{
    return ( (wp_s32)type & (wp_s32)WORKPHONE_PANEL_SET_NONBLOCK ) ? 1 : 0;
}
WORKPHONE_LIB wp_bool wp_panel_begin( struct wp_context *ctx, const wp_c8 *title,
                                      enum wp_panel_type panel_type )
{
    struct wp_input *in;
    struct wp_window *win;
    struct wp_panel *layout;
    struct wp_command_buffer *out;
    const struct wp_style *style;
    const struct wp_user_font *font;

    struct wp_vec2f scrollbar_size;
    struct wp_vec2f panel_padding;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;
    wp_zero( ctx->current->layout, sizeof( *ctx->current->layout ) );
    if( ( ctx->current->flags & WORKPHONE_WINDOW_HIDDEN ) ||
        ( ctx->current->flags & WORKPHONE_WINDOW_CLOSED ) )
    {
        wp_zero( ctx->current->layout, sizeof( struct wp_panel ) );
        ctx->current->layout->type = panel_type;
        return 0;
    }
    /* pull state into local stack */
    style = &ctx->style;
    font = style->font;
    win = ctx->current;
    layout = win->layout;
    out = &win->buffer;
    in = ( win->flags & WORKPHONE_WINDOW_NO_INPUT ) ? 0 : &ctx->input;
#ifdef WORKPHONE_INCLUDE_COMMAND_USERDATA
    win->buffer.userdata = ctx->userdata;
#endif
    /* pull style configuration into local stack */
    scrollbar_size = style->window.scrollbar_size;
    panel_padding = wp_panel_get_padding( style, panel_type );

    /* window movement */
    if( ( win->flags & WORKPHONE_WINDOW_MOVABLE ) && !( win->flags & WORKPHONE_WINDOW_ROM ) )
    {
        wp_bool left_mouse_down;
        wp_u32 left_mouse_clicked;
        wp_s32 left_mouse_click_in_cursor;

        /* calculate draggable window space */
        struct wp_rect header;
        header.x = win->bounds.x;
        header.y = win->bounds.y;
        header.w = win->bounds.w;
        if( wp_panel_has_header( win->flags, title ) )
        {
            header.h = font->height + 2.0f * style->window.header.padding.y;
            header.h += 2.0f * style->window.header.label_padding.y;
        }
        else
            header.h = panel_padding.y;

        /* window movement by dragging */
        left_mouse_down = in->mouse.buttons[WORKPHONE_BUTTON_LEFT].down;
        left_mouse_clicked = in->mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked;
        left_mouse_click_in_cursor =
            wp_input_has_mouse_click_down_in_rect( in, WORKPHONE_BUTTON_LEFT, header, wp_true );
        if( left_mouse_down && left_mouse_click_in_cursor && !left_mouse_clicked )
        {
            win->bounds.x = win->bounds.x + in->mouse.delta.x;
            win->bounds.y = win->bounds.y + in->mouse.delta.y;
            in->mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked_pos.x += in->mouse.delta.x;
            in->mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked_pos.y += in->mouse.delta.y;
            ctx->style.cursor_active = ctx->style.cursors[WORKPHONE_CURSOR_MOVE];
        }
    }

    /* setup panel */
    layout->type = panel_type;
    layout->flags = win->flags;
    layout->bounds = win->bounds;
    layout->bounds.x += panel_padding.x;
    layout->bounds.w -= 2 * panel_padding.x;
    if( win->flags & WORKPHONE_WINDOW_BORDER )
    {
        layout->border = wp_panel_get_border( style, win->flags, panel_type );
        layout->bounds = wp_shrink_make_rect( layout->bounds, layout->border );
    }
    else
        layout->border = 0;
    layout->at_y = layout->bounds.y;
    layout->at_x = layout->bounds.x;
    layout->max_x = 0;
    layout->header_height = 0;
    layout->footer_height = 0;
    wp_layout_reset_min_row_height( ctx );
    layout->row.index = 0;
    layout->row.columns = 0;
    layout->row.ratio = 0;
    layout->row.item_width = 0;
    layout->row.tree_depth = 0;
    layout->row.height = panel_padding.y;
    layout->has_scrolling = wp_true;
    if( !( win->flags & WORKPHONE_WINDOW_NO_SCROLLBAR ) )
        layout->bounds.w -= scrollbar_size.x;
    if( !wp_panel_is_nonblock( panel_type ) )
    {
        layout->footer_height = 0;
        if( !( win->flags & WORKPHONE_WINDOW_NO_SCROLLBAR ) || win->flags & WORKPHONE_WINDOW_SCALABLE )
            layout->footer_height = scrollbar_size.y;
        layout->bounds.h -= layout->footer_height;
    }

    /* panel header */
    if( wp_panel_has_header( win->flags, title ) )
    {
        struct wp_text text;
        struct wp_rect header;
        const struct wp_style_item *background = 0;

        /* calculate header bounds */
        header.x = win->bounds.x;
        header.y = win->bounds.y;
        header.w = win->bounds.w;
        header.h = font->height + 2.0f * style->window.header.padding.y;
        header.h += ( 2.0f * style->window.header.label_padding.y );

        /* shrink panel by header */
        layout->header_height = header.h;
        layout->bounds.y += header.h;
        layout->bounds.h -= header.h;
        layout->at_y += header.h;

        /* select correct header background and text color */
        if( ctx->active == win )
        {
            background = &style->window.header.active;
            text.text = style->window.header.label_active;
        }
        else if( wp_input_is_mouse_hovering_rect( &ctx->input, header ) )
        {
            background = &style->window.header.hover;
            text.text = style->window.header.label_hover;
        }
        else
        {
            background = &style->window.header.normal;
            text.text = style->window.header.label_normal;
        }

        /* draw header background */
        header.h += 1.0f;

        switch( background->type )
        {
        case WORKPHONE_STYLE_ITEM_IMAGE:
            text.background = wp_rgba( 0, 0, 0, 0 );
            wp_draw_image( &win->buffer, header, &background->data.image, wp_white );
            break;
        case WORKPHONE_STYLE_ITEM_NINE_SLICE:
            text.background = wp_rgba( 0, 0, 0, 0 );
            wp_draw_nine_slice( &win->buffer, header, &background->data.slice, wp_white );
            break;
        case WORKPHONE_STYLE_ITEM_COLOR:
            text.background = background->data.color;
            wp_fill_rect( out, header, 0, background->data.color );
            break;
        }

        /* window close button */
        {
            struct wp_rect button;
            button.y = header.y + style->window.header.padding.y;
            button.h = header.h - 2 * style->window.header.padding.y;
            button.w = button.h;
            if( win->flags & WORKPHONE_WINDOW_CLOSABLE )
            {
                wp_flags ws = 0;
                if( style->window.header.align == WORKPHONE_HEADER_RIGHT )
                {
                    button.x = ( header.w + header.x ) - ( button.w + style->window.header.padding.x );
                    header.w -=
                        button.w + style->window.header.spacing.x + style->window.header.padding.x;
                }
                else
                {
                    button.x = header.x + style->window.header.padding.x;
                    header.x +=
                        button.w + style->window.header.spacing.x + style->window.header.padding.x;
                }

                if( wp_do_button_symbol( &ws, &win->buffer, button, style->window.header.close_symbol,
                                         WORKPHONE_BUTTON_DEFAULT, &style->window.header.close_button,
                                         in, style->font ) &&
                    !( win->flags & WORKPHONE_WINDOW_ROM ) )
                {
                    layout->flags |= WORKPHONE_WINDOW_HIDDEN;
                    layout->flags &= (wp_flags)~WORKPHONE_WINDOW_MINIMIZED;
                }
            }

            /* window minimize button */
            if( win->flags & WORKPHONE_WINDOW_MINIMIZABLE )
            {
                wp_flags ws = 0;
                if( style->window.header.align == WORKPHONE_HEADER_RIGHT )
                {
                    button.x = ( header.w + header.x ) - button.w;
                    if( !( win->flags & WORKPHONE_WINDOW_CLOSABLE ) )
                    {
                        button.x -= style->window.header.padding.x;
                        header.w -= style->window.header.padding.x;
                    }
                    header.w -= button.w + style->window.header.spacing.x;
                }
                else
                {
                    button.x = header.x;
                    header.x +=
                        button.w + style->window.header.spacing.x + style->window.header.padding.x;
                }
                if( wp_do_button_symbol( &ws, &win->buffer, button,
                                         ( layout->flags & WORKPHONE_WINDOW_MINIMIZED )
                                             ? style->window.header.maximize_symbol
                                             : style->window.header.minimize_symbol,
                                         WORKPHONE_BUTTON_DEFAULT, &style->window.header.minimize_button,
                                         in, style->font ) &&
                    !( win->flags & WORKPHONE_WINDOW_ROM ) )
                    layout->flags = ( layout->flags & WORKPHONE_WINDOW_MINIMIZED )
                                        ? layout->flags & (wp_flags)~WORKPHONE_WINDOW_MINIMIZED
                                        : layout->flags | WORKPHONE_WINDOW_MINIMIZED;
            }
        }

        { /* window header title */
            wp_s32 text_len = wp_strlen( title );
            struct wp_rect label = { 0, 0, 0, 0 };
            wp_f32 t = font->width( font->userdata, font->height, title, text_len );
            text.padding = wp_make_vec2f( 0, 0 );

            label.x = header.x + style->window.header.padding.x;
            label.x += style->window.header.label_padding.x;
            label.y = header.y + style->window.header.label_padding.y;
            label.h = font->height + 2 * style->window.header.label_padding.y;
            label.w = t + 2 * style->window.header.spacing.x;
            label.w = WORKPHONE_CLAMP( 0, label.w, header.x + header.w - label.x );
            wp_widget_text( out, label, (const wp_c8 *)title, text_len, &text, WORKPHONE_TEXT_LEFT,
                            font );
        }
    }

    /* draw window background */
    if( !( layout->flags & WORKPHONE_WINDOW_MINIMIZED ) &&
        !( layout->flags & WORKPHONE_WINDOW_DYNAMIC ) )
    {
        struct wp_rect body;
        body.x = win->bounds.x;
        body.w = win->bounds.w;
        body.y = ( win->bounds.y + layout->header_height );
        body.h = ( win->bounds.h - layout->header_height );

        switch( style->window.fixed_background.type )
        {
        case WORKPHONE_STYLE_ITEM_IMAGE:
            wp_draw_image( out, body, &style->window.fixed_background.data.image, wp_white );
            break;
        case WORKPHONE_STYLE_ITEM_NINE_SLICE:
            wp_draw_nine_slice( out, body, &style->window.fixed_background.data.slice, wp_white );
            break;
        case WORKPHONE_STYLE_ITEM_COLOR:
            wp_fill_rect( out, body, style->window.rounding, style->window.fixed_background.data.color );
            break;
        }
    }

    /* set clipping rectangle */
    {
        struct wp_rect clip;
        layout->clip = layout->bounds;
        wp_unify( &clip, &win->buffer.clip, layout->clip.x, layout->clip.y,
                  layout->clip.x + layout->clip.w, layout->clip.y + layout->clip.h );
        wp_push_scissor( out, clip );
        layout->clip = clip;
    }
    return !( layout->flags & WORKPHONE_WINDOW_HIDDEN ) &&
           !( layout->flags & WORKPHONE_WINDOW_MINIMIZED );
}
WORKPHONE_LIB void wp_panel_end( struct wp_context *ctx )
{
    struct wp_input *in;
    struct wp_window *window;
    struct wp_panel *layout;
    const struct wp_style *style;
    struct wp_command_buffer *out;

    struct wp_vec2f scrollbar_size;
    struct wp_vec2f panel_padding;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    window = ctx->current;
    layout = window->layout;
    style = &ctx->style;
    out = &window->buffer;
    in = ( layout->flags & WORKPHONE_WINDOW_ROM || layout->flags & WORKPHONE_WINDOW_NO_INPUT )
             ? 0
             : &ctx->input;
    if( !wp_panel_is_sub( layout->type ) )
        wp_push_scissor( out, wp_null_rect );

    /* cache configuration data */
    scrollbar_size = style->window.scrollbar_size;
    panel_padding = wp_panel_get_padding( style, layout->type );

    /* update the current cursor Y-position to powp_s32 over the last added widget */
    layout->at_y += layout->row.height;

    /* dynamic panels */
    if( layout->flags & WORKPHONE_WINDOW_DYNAMIC && !( layout->flags & WORKPHONE_WINDOW_MINIMIZED ) )
    {
        /* update panel height to fit dynamic growth */
        struct wp_rect empty_space;
        if( layout->at_y < ( layout->bounds.y + layout->bounds.h ) )
            layout->bounds.h = layout->at_y - layout->bounds.y;

        /* fill top empty space */
        empty_space.x = window->bounds.x;
        empty_space.y = layout->bounds.y;
        empty_space.h = panel_padding.y;
        empty_space.w = window->bounds.w;
        wp_fill_rect( out, empty_space, 0, style->window.background );

        /* fill left empty space */
        empty_space.x = window->bounds.x;
        empty_space.y = layout->bounds.y;
        empty_space.w = panel_padding.x + layout->border;
        empty_space.h = layout->bounds.h;
        wp_fill_rect( out, empty_space, 0, style->window.background );

        /* fill right empty space */
        empty_space.x = layout->bounds.x + layout->bounds.w;
        empty_space.y = layout->bounds.y;
        empty_space.w = panel_padding.x + layout->border;
        empty_space.h = layout->bounds.h;
        if( *layout->offset_y == 0 && !( layout->flags & WORKPHONE_WINDOW_NO_SCROLLBAR ) )
            empty_space.w += scrollbar_size.x;
        wp_fill_rect( out, empty_space, 0, style->window.background );

        /* fill bottom empty space */
        if( layout->footer_height > 0 )
        {
            empty_space.x = window->bounds.x;
            empty_space.y = layout->bounds.y + layout->bounds.h;
            empty_space.w = window->bounds.w;
            empty_space.h = layout->footer_height;
            wp_fill_rect( out, empty_space, 0, style->window.background );
        }
    }

    /* scrollbars */
    if( !( layout->flags & WORKPHONE_WINDOW_NO_SCROLLBAR ) &&
        !( layout->flags & WORKPHONE_WINDOW_MINIMIZED ) &&
        window->scrollbar_hiding_timer < WORKPHONE_SCROLLBAR_HIDING_TIMEOUT )
    {
        struct wp_rect scroll;
        wp_s32 scroll_has_scrolling;
        wp_f32 scroll_target;
        wp_f32 scroll_offset;
        wp_f32 scroll_step;
        wp_f32 scroll_inc;

        /* mouse wheel scrolling */
        if( wp_panel_is_sub( layout->type ) )
        {
            /* sub-window mouse wheel scrolling */
            struct wp_window *root_window = window;
            struct wp_panel *root_panel = window->layout;
            while( root_panel->parent )
                root_panel = root_panel->parent;
            while( root_window->parent )
                root_window = root_window->parent;

            /* only allow scrolling if parent window is active */
            scroll_has_scrolling = wp_false;
            if( ( root_window == ctx->active ) && layout->has_scrolling )
            {
                /* and panel is being hovered and inside clip rect*/
                if( wp_input_is_mouse_hovering_rect( in, layout->bounds ) &&
                    WORKPHONE_INTERSECT( layout->bounds.x, layout->bounds.y, layout->bounds.w,
                                         layout->bounds.h, root_panel->clip.x, root_panel->clip.y,
                                         root_panel->clip.w, root_panel->clip.h ) )
                {
                    /* deactivate all parent scrolling */
                    root_panel = window->layout;
                    while( root_panel->parent )
                    {
                        root_panel->has_scrolling = wp_false;
                        root_panel = root_panel->parent;
                    }
                    root_panel->has_scrolling = wp_false;
                    scroll_has_scrolling = wp_true;
                }
            }
        }
        else
        {
            /* window mouse wheel scrolling */
            scroll_has_scrolling = ( window == ctx->active ) && layout->has_scrolling;
            if( in && ( in->mouse.scroll_delta.y > 0 || in->mouse.scroll_delta.x > 0 ) &&
                scroll_has_scrolling )
                window->scrolled = wp_true;
            else
                window->scrolled = wp_false;
        }

        {
            /* vertical scrollbar */
            wp_flags state = 0;
            scroll.x = layout->bounds.x + layout->bounds.w + panel_padding.x;
            scroll.y = layout->bounds.y;
            scroll.w = scrollbar_size.x;
            scroll.h = layout->bounds.h;

            scroll_offset = (wp_f32)*layout->offset_y;
            scroll_step = scroll.h * 0.10f;
            scroll_inc = scroll.h * 0.01f;
            scroll_target = (wp_f32)(wp_s32)( layout->at_y - scroll.y );
            scroll_offset = wp_do_scrollbarv( &state, out, scroll, scroll_has_scrolling, scroll_offset,
                                              scroll_target, scroll_step, scroll_inc,
                                              &ctx->style.scrollv, in, style->font );
            *layout->offset_y = (wp_u32)scroll_offset;
            if( in && scroll_has_scrolling )
                in->mouse.scroll_delta.y = 0;
        }
        {
            /* horizontal scrollbar */
            wp_flags state = 0;
            scroll.x = layout->bounds.x;
            scroll.y = layout->bounds.y + layout->bounds.h;
            scroll.w = layout->bounds.w;
            scroll.h = scrollbar_size.y;

            scroll_offset = (wp_f32)*layout->offset_x;
            scroll_target = (wp_f32)(wp_s32)( layout->max_x - scroll.x );
            scroll_step = layout->max_x * 0.05f;
            scroll_inc = layout->max_x * 0.005f;
            scroll_offset = wp_do_scrollbarh( &state, out, scroll, scroll_has_scrolling, scroll_offset,
                                              scroll_target, scroll_step, scroll_inc,
                                              &ctx->style.scrollh, in, style->font );
            *layout->offset_x = (wp_u32)scroll_offset;
        }
    }

    /* hide scroll if no user input */
    if( window->flags & WORKPHONE_WINDOW_SCROLL_AUTO_HIDE )
    {
        wp_s32 has_input =
            wp_input_is_mouse_moved( &ctx->input ) || ctx->input.mouse.scroll_delta.y != 0;
        wp_s32 is_window_hovered = wp_window_is_hovered( ctx );
        wp_s32 any_item_active = ( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_MODIFIED );
        if( ( !has_input && is_window_hovered ) || ( !is_window_hovered && !any_item_active ) )
            window->scrollbar_hiding_timer += ctx->delta_time_seconds;
        else
            window->scrollbar_hiding_timer = 0;
    }
    else
        window->scrollbar_hiding_timer = 0;

    /* window border */
    if( layout->flags & WORKPHONE_WINDOW_BORDER )
    {
        struct wp_color border_color = wp_panel_get_border_color( style, layout->type );
        const wp_f32 padding_y =
            ( layout->flags & WORKPHONE_WINDOW_MINIMIZED )
                ? ( style->window.border + window->bounds.y + layout->header_height )
                : ( ( layout->flags & WORKPHONE_WINDOW_DYNAMIC )
                        ? ( layout->bounds.y + layout->bounds.h + layout->footer_height )
                        : ( window->bounds.y + window->bounds.h ) );
        struct wp_rect b = window->bounds;
        b.h = padding_y - window->bounds.y;
        wp_stroke_rect( out, b, style->window.rounding, layout->border, border_color );
    }

    /* scaler */
    if( ( layout->flags & WORKPHONE_WINDOW_SCALABLE ) && in &&
        !( layout->flags & WORKPHONE_WINDOW_MINIMIZED ) )
    {
        /* calculate scaler bounds */
        struct wp_rect scaler;
        scaler.w = scrollbar_size.x;
        scaler.h = scrollbar_size.y;
        scaler.y = layout->bounds.y + layout->bounds.h;
        if( layout->flags & WORKPHONE_WINDOW_SCALE_LEFT )
            scaler.x = layout->bounds.x - panel_padding.x * 0.5f;
        else
            scaler.x = layout->bounds.x + layout->bounds.w + panel_padding.x;
        if( layout->flags & WORKPHONE_WINDOW_NO_SCROLLBAR )
            scaler.x -= scaler.w;

        /* draw scaler */
        {
            const struct wp_style_item *item = &style->window.scaler;
            if( item->type == WORKPHONE_STYLE_ITEM_IMAGE )
                wp_draw_image( out, scaler, &item->data.image, wp_white );
            else
            {
                if( layout->flags & WORKPHONE_WINDOW_SCALE_LEFT )
                {
                    wp_fill_triangle( out, scaler.x, scaler.y, scaler.x, scaler.y + scaler.h,
                                      scaler.x + scaler.w, scaler.y + scaler.h, item->data.color );
                }
                else
                {
                    wp_fill_triangle( out, scaler.x + scaler.w, scaler.y, scaler.x + scaler.w,
                                      scaler.y + scaler.h, scaler.x, scaler.y + scaler.h,
                                      item->data.color );
                }
            }
        }

        /* do window scaling */
        if( !( window->flags & WORKPHONE_WINDOW_ROM ) )
        {
            struct wp_vec2f window_size = style->window.min_size;
            wp_s32 left_mouse_down = in->mouse.buttons[WORKPHONE_BUTTON_LEFT].down;
            wp_s32 left_mouse_click_in_scaler =
                wp_input_has_mouse_click_down_in_rect( in, WORKPHONE_BUTTON_LEFT, scaler, wp_true );

            if( left_mouse_down && left_mouse_click_in_scaler )
            {
                wp_f32 delta_x = in->mouse.delta.x;
                if( layout->flags & WORKPHONE_WINDOW_SCALE_LEFT )
                {
                    delta_x = -delta_x;
                    window->bounds.x += in->mouse.delta.x;
                }
                /* dragging in x-direction  */
                if( window->bounds.w + delta_x >= window_size.x )
                {
                    if( ( delta_x < 0 ) || ( delta_x > 0 && in->mouse.pos.x >= scaler.x ) )
                    {
                        window->bounds.w = window->bounds.w + delta_x;
                        scaler.x += in->mouse.delta.x;
                    }
                }
                /* dragging in y-direction (only possible if static window) */
                if( !( layout->flags & WORKPHONE_WINDOW_DYNAMIC ) )
                {
                    if( window_size.y < window->bounds.h + in->mouse.delta.y )
                    {
                        if( ( in->mouse.delta.y < 0 ) ||
                            ( in->mouse.delta.y > 0 && in->mouse.pos.y >= scaler.y ) )
                        {
                            window->bounds.h = window->bounds.h + in->mouse.delta.y;
                            scaler.y += in->mouse.delta.y;
                        }
                    }
                }
                ctx->style.cursor_active =
                    ctx->style.cursors[WORKPHONE_CURSOR_RESIZE_TOP_RIGHT_DOWN_LEFT];
                in->mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked_pos.x = scaler.x + scaler.w / 2.0f;
                in->mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked_pos.y = scaler.y + scaler.h / 2.0f;
            }
        }
    }
    if( !wp_panel_is_sub( layout->type ) )
    {
        /* window is hidden so clear command buffer  */
        if( layout->flags & WORKPHONE_WINDOW_HIDDEN )
            wp_command_buffer_reset( &window->buffer );
        /* window is visible and not tab */
        else
            wp_finish( ctx, window );
    }

    /* WORKPHONE_WINDOW_REMOVE_ROM flag was set so remove WORKPHONE_WINDOW_ROM */
    if( layout->flags & WORKPHONE_WINDOW_REMOVE_ROM )
    {
        layout->flags &= ~(wp_flags)WORKPHONE_WINDOW_ROM;
        layout->flags &= ~(wp_flags)WORKPHONE_WINDOW_REMOVE_ROM;
    }
    window->flags = layout->flags;

    /* property garbage collector */
    if( window->property.active && window->property.old != window->property.seq &&
        window->property.active == window->property.prev )
    {
        wp_zero( &window->property, sizeof( window->property ) );
    }
    else
    {
        window->property.old = window->property.seq;
        window->property.prev = window->property.active;
        window->property.seq = 0;
    }
    /* edit garbage collector */
    if( window->edit.active && window->edit.old != window->edit.seq &&
        window->edit.active == window->edit.prev )
    {
        wp_zero( &window->edit, sizeof( window->edit ) );
    }
    else
    {
        window->edit.old = window->edit.seq;
        window->edit.prev = window->edit.active;
        window->edit.seq = 0;
    }
    /* contextual garbage collector */
    if( window->popup.active_con && window->popup.con_old != window->popup.con_count )
    {
        window->popup.con_count = 0;
        window->popup.con_old = 0;
        window->popup.active_con = 0;
    }
    else
    {
        window->popup.con_old = window->popup.con_count;
        window->popup.con_count = 0;
    }
    window->popup.combo_count = 0;
    /* helper to make sure you have a 'wp_tree_push' for every 'wp_tree_pop' */
    WORKPHONE_ASSERT( !layout->row.tree_depth );
}
