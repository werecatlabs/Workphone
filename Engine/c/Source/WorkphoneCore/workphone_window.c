#include "workphone.h"
#include "workphone_font.h"
#include "workphone_context.h"
#include "workphone_page_element.h"
#include "workphone_pool.h"
#include "workphone_table.h"
#include "workphone_util.h"
#include "workphone_widget.h"

void *wp_create_window( struct wp_context *ctx )
{
    struct wp_page_element *elem;
    elem = wp_create_page_element( ctx );
    if( !elem )
        return 0;
    elem->data.win.seq = ctx->seq;
    return &elem->data.win;
}
void wp_free_window( struct wp_context *ctx, struct wp_window *win )
{
    /* unlink windows from list */
    struct wp_table *it = win->tables;
    if( win->popup.win )
    {
        wp_free_window( ctx, win->popup.win );
        win->popup.win = 0;
    }
    win->next = 0;
    win->prev = 0;

    while( it )
    {
        /*free window state tables */
        struct wp_table *n = it->next;
        wp_remove_table( win, it );
        wp_free_table( ctx, it );
        if( it == win->tables )
            win->tables = n;
        it = n;
    }

    /* link windows into freelist */
    {
        union wp_page_data *pd = WORKPHONE_CONTAINER_OF( win, union wp_page_data, win );
        struct wp_page_element *pe = WORKPHONE_CONTAINER_OF( pd, struct wp_page_element, data );
        wp_free_page_element( ctx, pe );
    }
}

struct wp_window *wp_find_window( const struct wp_context *ctx, wp_hash hash, const wp_c8 *name )
{
    struct wp_window *iter;
    iter = ctx->begin;
    while( iter )
    {
        WORKPHONE_ASSERT( iter != iter->next );
        if( iter->name == hash )
        {
            wp_s32 max_len = wp_strlen( iter->name_string );
            if( !wp_stricmpn( iter->name_string, name, max_len ) )
                return iter;
        }
        iter = iter->next;
    }
    return 0;
}

void wp_insert_window( struct wp_context *ctx, struct wp_window *win,
                       enum wp_window_insert_location loc )
{
    const struct wp_window *iter;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( win );
    if( !win || !ctx )
        return;

    iter = ctx->begin;
    while( iter )
    {
        WORKPHONE_ASSERT( iter != iter->next );
        WORKPHONE_ASSERT( iter != win );
        if( iter == win )
            return;
        iter = iter->next;
    }

    if( !ctx->begin )
    {
        win->next = 0;
        win->prev = 0;
        ctx->begin = win;
        ctx->end = win;
        ctx->count = 1;
        return;
    }
    if( loc == WORKPHONE_INSERT_BACK )
    {
        struct wp_window *end;
        end = ctx->end;
        end->flags |= WORKPHONE_WINDOW_ROM;
        end->next = win;
        win->prev = ctx->end;
        win->next = 0;
        ctx->end = win;
        ctx->active = ctx->end;
        ctx->end->flags &= ~(wp_flags)WORKPHONE_WINDOW_ROM;
    }
    else
    {
        /*ctx->end->flags |= WORKPHONE_WINDOW_ROM;*/
        ctx->begin->prev = win;
        win->next = ctx->begin;
        win->prev = 0;
        ctx->begin = win;
        ctx->begin->flags &= ~(wp_flags)WORKPHONE_WINDOW_ROM;
    }
    ctx->count++;
}

void wp_remove_window( struct wp_context *ctx, struct wp_window *win )
{
    if( win == ctx->begin || win == ctx->end )
    {
        if( win == ctx->begin )
        {
            ctx->begin = win->next;
            if( win->next )
                win->next->prev = 0;
        }
        if( win == ctx->end )
        {
            ctx->end = win->prev;
            if( win->prev )
                win->prev->next = 0;
        }
    }
    else
    {
        if( win->next )
            win->next->prev = win->prev;
        if( win->prev )
            win->prev->next = win->next;
    }
    if( win == ctx->active || !ctx->active )
    {
        ctx->active = ctx->end;
        if( ctx->end )
            ctx->end->flags &= ~(wp_flags)WORKPHONE_WINDOW_ROM;
    }
    win->next = 0;
    win->prev = 0;
    ctx->count--;
}

wp_bool wp_begin( struct wp_context *ctx, const wp_c8 *title, struct wp_rect bounds, wp_flags flags )
{
    return wp_begin_titled( ctx, title, title, bounds, flags );
}

wp_bool wp_begin_titled( struct wp_context *ctx, const wp_c8 *name, const wp_c8 *title,
                         struct wp_rect bounds, wp_flags flags )
{
    struct wp_window *win;
    struct wp_style *style;
    wp_hash name_hash;
    wp_s32 name_len;
    wp_s32 ret = 0;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( name );
    WORKPHONE_ASSERT( title );
    WORKPHONE_ASSERT( ctx->style.font && ctx->style.font->width &&
                      "if this triggers you forgot to add a font" );
    WORKPHONE_ASSERT( !ctx->current && "if this triggers you missed a `wp_end` call" );
    if( !ctx || ctx->current || !title || !name )
        return 0;

    /* find or create window */
    style = &ctx->style;
    name_len = (wp_s32)wp_strlen( name );
    name_hash = wp_murmur_hash( name, (wp_s32)name_len, WORKPHONE_WINDOW_TITLE );
    win = wp_find_window( ctx, name_hash, name );
    if( !win )
    {
        /* create new window */
        wp_size name_length = (wp_size)name_len;
        win = (struct wp_window *)wp_create_window( ctx );
        WORKPHONE_ASSERT( win );
        if( !win )
            return 0;

        if( flags & WORKPHONE_WINDOW_BACKGROUND )
            wp_insert_window( ctx, win, WORKPHONE_INSERT_FRONT );
        else
            wp_insert_window( ctx, win, WORKPHONE_INSERT_BACK );
        wp_command_buffer_init( &win->buffer, &ctx->memory, WORKPHONE_CLIPPING_ON );

        win->flags = flags;
        win->bounds = bounds;
        win->name = name_hash;
        name_length = WORKPHONE_MIN( name_length, WORKPHONE_WINDOW_MAX_NAME - 1 );
        WORKPHONE_MEMCPY( win->name_string, name, name_length );
        win->name_string[name_length] = 0;
        win->popup.win = 0;
        win->widgets_disabled = wp_false;
        if( !ctx->active )
            ctx->active = win;
    }
    else
    {
        /* update window */
        win->flags &= ~(wp_flags)( WORKPHONE_WINDOW_PRIVATE - 1 );
        win->flags |= flags;
        if( !( win->flags & ( WORKPHONE_WINDOW_MOVABLE | WORKPHONE_WINDOW_SCALABLE ) ) )
            win->bounds = bounds;
        /* If this assert triggers you either:
         *
         * I.) Have more than one window with the same name or
         * II.) You forgot to actually draw the window.
         *      More specific you did not call `wp_clear` (wp_clear will be
         *      automatically called for you if you are using one of the
         *      provided demo backends). */
        //WP_ASSERT( win->seq != ctx->seq );
        win->seq = ctx->seq;
        if( !ctx->active && !( win->flags & WORKPHONE_WINDOW_HIDDEN ) )
        {
            ctx->active = win;
            ctx->end = win;
        }
    }
    if( win->flags & WORKPHONE_WINDOW_HIDDEN )
    {
        ctx->current = win;
        win->layout = 0;
        return 0;
    }
    else
        wp_start( ctx, win );

    /* window overlapping */
    if( !( win->flags & WORKPHONE_WINDOW_HIDDEN ) && !( win->flags & WORKPHONE_WINDOW_NO_INPUT ) )
    {
        wp_s32 inpanel, ishovered;
        struct wp_window *iter = win;
        wp_f32 h = ctx->style.font->height + 2.0f * style->window.header.padding.y +
                   ( 2.0f * style->window.header.label_padding.y );
        struct wp_rect win_bounds = ( !( win->flags & WORKPHONE_WINDOW_MINIMIZED ) )
                                        ? win->bounds
                                        : wp_make_rect( win->bounds.x, win->bounds.y, win->bounds.w, h );

        /* activate window if hovered and no other window is overlapping this window */
        inpanel = wp_input_has_mouse_click_down_in_rect( &ctx->input, WORKPHONE_BUTTON_LEFT, win_bounds,
                                                         wp_true );
        inpanel = inpanel && ctx->input.mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked;
        ishovered = wp_input_is_mouse_hovering_rect( &ctx->input, win_bounds );
        if( ( win != ctx->active ) && ishovered &&
            !ctx->input.mouse.buttons[WORKPHONE_BUTTON_LEFT].down )
        {
            iter = win->next;
            while( iter )
            {
                struct wp_rect iter_bounds =
                    ( !( iter->flags & WORKPHONE_WINDOW_MINIMIZED ) )
                        ? iter->bounds
                        : wp_make_rect( iter->bounds.x, iter->bounds.y, iter->bounds.w, h );
                if( WORKPHONE_INTERSECT( win_bounds.x, win_bounds.y, win_bounds.w, win_bounds.h,
                                         iter_bounds.x, iter_bounds.y, iter_bounds.w, iter_bounds.h ) &&
                    ( !( iter->flags & WORKPHONE_WINDOW_HIDDEN ) ) )
                    break;

                if( iter->popup.win && iter->popup.active &&
                    !( iter->flags & WORKPHONE_WINDOW_HIDDEN ) &&
                    WORKPHONE_INTERSECT( win->bounds.x, win_bounds.y, win_bounds.w, win_bounds.h,
                                         iter->popup.win->bounds.x, iter->popup.win->bounds.y,
                                         iter->popup.win->bounds.w, iter->popup.win->bounds.h ) )
                    break;
                iter = iter->next;
            }
        }

        /* activate window if clicked */
        if( iter && inpanel && ( win != ctx->end ) )
        {
            iter = win->next;
            while( iter )
            {
                /* try to find a panel with higher priority in the same position */
                struct wp_rect iter_bounds =
                    ( !( iter->flags & WORKPHONE_WINDOW_MINIMIZED ) )
                        ? iter->bounds
                        : wp_make_rect( iter->bounds.x, iter->bounds.y, iter->bounds.w, h );
                if( WORKPHONE_INBOX( ctx->input.mouse.pos.x, ctx->input.mouse.pos.y, iter_bounds.x,
                                     iter_bounds.y, iter_bounds.w, iter_bounds.h ) &&
                    !( iter->flags & WORKPHONE_WINDOW_HIDDEN ) )
                    break;
                if( iter->popup.win && iter->popup.active &&
                    !( iter->flags & WORKPHONE_WINDOW_HIDDEN ) &&
                    WORKPHONE_INTERSECT( win_bounds.x, win_bounds.y, win_bounds.w, win_bounds.h,
                                         iter->popup.win->bounds.x, iter->popup.win->bounds.y,
                                         iter->popup.win->bounds.w, iter->popup.win->bounds.h ) )
                    break;
                iter = iter->next;
            }
        }
        if( iter && !( win->flags & WORKPHONE_WINDOW_ROM ) &&
            ( win->flags & WORKPHONE_WINDOW_BACKGROUND ) )
        {
            win->flags |= (wp_flags)WORKPHONE_WINDOW_ROM;
            iter->flags &= ~(wp_flags)WORKPHONE_WINDOW_ROM;
            ctx->active = iter;
            if( !( iter->flags & WORKPHONE_WINDOW_BACKGROUND ) )
            {
                /* current window is active in that position so transfer to top
                 * at the highest priority in stack */
                wp_remove_window( ctx, iter );
                wp_insert_window( ctx, iter, WORKPHONE_INSERT_BACK );
            }
        }
        else
        {
            if( !iter && ctx->end != win )
            {
                if( !( win->flags & WORKPHONE_WINDOW_BACKGROUND ) )
                {
                    /* current window is active in that position so transfer to top
                     * at the highest priority in stack */
                    wp_remove_window( ctx, win );
                    wp_insert_window( ctx, win, WORKPHONE_INSERT_BACK );
                }
                win->flags &= ~(wp_flags)WORKPHONE_WINDOW_ROM;
                ctx->active = win;
            }
            if( ctx->end != win && !( win->flags & WORKPHONE_WINDOW_BACKGROUND ) )
                win->flags |= WORKPHONE_WINDOW_ROM;
        }
    }
    win->layout = (struct wp_panel *)wp_create_panel( ctx );
    ctx->current = win;
    ret = wp_panel_begin( ctx, title, WORKPHONE_PANEL_WINDOW );
    win->layout->offset_x = &win->scrollbar.x;
    win->layout->offset_y = &win->scrollbar.y;
    return ret;
}

void wp_end( struct wp_context *ctx )
{
    struct wp_panel *layout;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current && "if this triggers you forgot to call `wp_begin`" );
    if( !ctx || !ctx->current )
        return;

    layout = ctx->current->layout;
    if( !layout ||
        ( layout->type == WORKPHONE_PANEL_WINDOW && ( ctx->current->flags & WORKPHONE_WINDOW_HIDDEN ) ) )
    {
        ctx->current = 0;
        return;
    }
    wp_panel_end( ctx );
    wp_free_panel( ctx, ctx->current->layout );
    ctx->current = 0;
}

struct wp_rect wp_window_get_bounds( const struct wp_context *ctx )
{
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return wp_make_rect( 0, 0, 0, 0 );
    return ctx->current->bounds;
}

struct wp_vec2f wp_window_get_position( const struct wp_context *ctx )
{
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return wp_make_vec2f( 0, 0 );
    return wp_make_vec2f( ctx->current->bounds.x, ctx->current->bounds.y );
}

struct wp_vec2f wp_window_get_size( const struct wp_context *ctx )
{
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return wp_make_vec2f( 0, 0 );
    return wp_make_vec2f( ctx->current->bounds.w, ctx->current->bounds.h );
}

wp_f32 wp_window_get_width( const struct wp_context *ctx )
{
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return 0;
    return ctx->current->bounds.w;
}

wp_f32 wp_window_get_height( const struct wp_context *ctx )
{
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return 0;
    return ctx->current->bounds.h;
}

struct wp_rect wp_window_get_content_region( const struct wp_context *ctx )
{
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return wp_make_rect( 0, 0, 0, 0 );
    return ctx->current->layout->clip;
}

struct wp_vec2f wp_window_get_content_region_min( const struct wp_context *ctx )
{
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current )
        return wp_make_vec2f( 0, 0 );
    return wp_make_vec2f( ctx->current->layout->clip.x, ctx->current->layout->clip.y );
}

struct wp_vec2f wp_window_get_content_region_max( const struct wp_context *ctx )
{
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current )
        return wp_make_vec2f( 0, 0 );
    return wp_make_vec2f( ctx->current->layout->clip.x + ctx->current->layout->clip.w,
                          ctx->current->layout->clip.y + ctx->current->layout->clip.h );
}

struct wp_vec2f wp_window_get_content_region_size( const struct wp_context *ctx )
{
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current )
        return wp_make_vec2f( 0, 0 );
    return wp_make_vec2f( ctx->current->layout->clip.w, ctx->current->layout->clip.h );
}

struct wp_command_buffer *wp_window_get_canvas( const struct wp_context *ctx )
{
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current )
        return 0;
    return &ctx->current->buffer;
}

struct wp_panel *wp_window_get_panel( const struct wp_context *ctx )
{
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return 0;
    return ctx->current->layout;
}

void wp_window_get_scroll( const struct wp_context *ctx, wp_u32 *offset_x, wp_u32 *offset_y )
{
    struct wp_window *win;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return;
    win = ctx->current;
    if( offset_x )
        *offset_x = win->scrollbar.x;
    if( offset_y )
        *offset_y = win->scrollbar.y;
}

wp_bool wp_window_has_focus( const struct wp_context *ctx )
{
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current )
        return 0;
    return ctx->current == ctx->active;
}

wp_bool wp_window_is_hovered( const struct wp_context *ctx )
{
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current || ( ctx->current->flags & WORKPHONE_WINDOW_HIDDEN ) )
        return 0;
    else
    {
        struct wp_rect actual_bounds = ctx->current->bounds;
        if( ctx->current->flags & WORKPHONE_WINDOW_MINIMIZED )
        {
            actual_bounds.h = ctx->current->layout->header_height;
        }
        return wp_input_is_mouse_hovering_rect( &ctx->input, actual_bounds );
    }
}

wp_bool wp_window_is_any_hovered( const struct wp_context *ctx )
{
    struct wp_window *iter;
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return 0;
    iter = ctx->begin;
    while( iter )
    {
        /* check if window is being hovered */
        if( !( iter->flags & WORKPHONE_WINDOW_HIDDEN ) )
        {
            /* check if window popup is being hovered */
            if( iter->popup.active && iter->popup.win &&
                wp_input_is_mouse_hovering_rect( &ctx->input, iter->popup.win->bounds ) )
                return 1;

            if( iter->flags & WORKPHONE_WINDOW_MINIMIZED )
            {
                struct wp_rect header = iter->bounds;
                header.h = ctx->style.font->height + 2 * ctx->style.window.header.padding.y;
                if( wp_input_is_mouse_hovering_rect( &ctx->input, header ) )
                    return 1;
            }
            else if( wp_input_is_mouse_hovering_rect( &ctx->input, iter->bounds ) )
            {
                return 1;
            }
        }
        iter = iter->next;
    }
    return 0;
}

wp_bool wp_item_is_any_active( const struct wp_context *ctx )
{
    wp_s32 any_hovered = wp_window_is_any_hovered( ctx );
    wp_s32 any_active = ( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_MODIFIED );
    return any_hovered || any_active;
}

wp_bool wp_window_is_collapsed( const struct wp_context *ctx, const wp_c8 *name )
{
    wp_s32 title_len;
    wp_hash title_hash;
    struct wp_window *win;
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return 0;

    title_len = (wp_s32)wp_strlen( name );
    title_hash = wp_murmur_hash( name, (wp_s32)title_len, WORKPHONE_WINDOW_TITLE );
    win = wp_find_window( ctx, title_hash, name );
    if( !win )
        return 0;
    return win->flags & WORKPHONE_WINDOW_MINIMIZED;
}

wp_bool wp_window_is_closed( const struct wp_context *ctx, const wp_c8 *name )
{
    wp_s32 title_len;
    wp_hash title_hash;
    struct wp_window *win;
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return 1;

    title_len = (wp_s32)wp_strlen( name );
    title_hash = wp_murmur_hash( name, (wp_s32)title_len, WORKPHONE_WINDOW_TITLE );
    win = wp_find_window( ctx, title_hash, name );
    if( !win )
        return 1;
    return ( win->flags & WORKPHONE_WINDOW_CLOSED );
}

wp_bool wp_window_is_hidden( const struct wp_context *ctx, const wp_c8 *name )
{
    wp_s32 title_len;
    wp_hash title_hash;
    struct wp_window *win;
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return 1;

    title_len = (wp_s32)wp_strlen( name );
    title_hash = wp_murmur_hash( name, (wp_s32)title_len, WORKPHONE_WINDOW_TITLE );
    win = wp_find_window( ctx, title_hash, name );
    if( !win )
        return 1;
    return ( win->flags & WORKPHONE_WINDOW_HIDDEN );
}

wp_bool wp_window_is_active( const struct wp_context *ctx, const wp_c8 *name )
{
    wp_s32 title_len;
    wp_hash title_hash;
    struct wp_window *win;
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return 0;

    title_len = (wp_s32)wp_strlen( name );
    title_hash = wp_murmur_hash( name, (wp_s32)title_len, WORKPHONE_WINDOW_TITLE );
    win = wp_find_window( ctx, title_hash, name );
    if( !win )
        return 0;
    return win == ctx->active;
}

struct wp_window *wp_window_find( const struct wp_context *ctx, const wp_c8 *name )
{
    wp_s32 title_len;
    wp_hash title_hash;
    title_len = (wp_s32)wp_strlen( name );
    title_hash = wp_murmur_hash( name, (wp_s32)title_len, WORKPHONE_WINDOW_TITLE );
    return wp_find_window( ctx, title_hash, name );
}

void wp_window_close( struct wp_context *ctx, const wp_c8 *name )
{
    struct wp_window *win;
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return;
    win = wp_window_find( ctx, name );
    if( !win )
        return;
    WORKPHONE_ASSERT( ctx->current != win && "You cannot close a currently active window" );
    if( ctx->current == win )
        return;
    win->flags |= WORKPHONE_WINDOW_HIDDEN;
    win->flags |= WORKPHONE_WINDOW_CLOSED;
}

void wp_window_set_bounds( struct wp_context *ctx, const wp_c8 *name, struct wp_rect bounds )
{
    struct wp_window *win;
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return;
    win = wp_window_find( ctx, name );
    if( !win )
        return;
    win->bounds = bounds;
}

void wp_window_set_position( struct wp_context *ctx, const wp_c8 *name, struct wp_vec2f pos )
{
    struct wp_window *win = wp_window_find( ctx, name );
    if( !win )
        return;
    win->bounds.x = pos.x;
    win->bounds.y = pos.y;
}

void wp_window_set_size( struct wp_context *ctx, const wp_c8 *name, struct wp_vec2f size )
{
    struct wp_window *win = wp_window_find( ctx, name );
    if( !win )
        return;
    win->bounds.w = size.x;
    win->bounds.h = size.y;
}

void wp_window_set_scroll( struct wp_context *ctx, wp_u32 offset_x, wp_u32 offset_y )
{
    struct wp_window *win;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    if( !ctx || !ctx->current )
        return;
    win = ctx->current;
    win->scrollbar.x = offset_x;
    win->scrollbar.y = offset_y;
}

void wp_window_collapse( struct wp_context *ctx, const wp_c8 *name, enum wp_collapse_states c )
{
    wp_s32 title_len;
    wp_hash title_hash;
    struct wp_window *win;
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return;

    title_len = (wp_s32)wp_strlen( name );
    title_hash = wp_murmur_hash( name, (wp_s32)title_len, WORKPHONE_WINDOW_TITLE );
    win = wp_find_window( ctx, title_hash, name );
    if( !win )
        return;
    if( c == WORKPHONE_MINIMIZED )
        win->flags |= WORKPHONE_WINDOW_MINIMIZED;
    else
        win->flags &= ~(wp_flags)WORKPHONE_WINDOW_MINIMIZED;
}

void wp_window_collapse_if( struct wp_context *ctx, const wp_c8 *name, enum wp_collapse_states c,
                            wp_s32 cond )
{
    WORKPHONE_ASSERT( ctx );
    if( !ctx || !cond )
        return;
    wp_window_collapse( ctx, name, c );
}

void wp_window_show( struct wp_context *ctx, const wp_c8 *name, enum wp_show_states s )
{
    wp_s32 title_len;
    wp_hash title_hash;
    struct wp_window *win;
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return;

    title_len = (wp_s32)wp_strlen( name );
    title_hash = wp_murmur_hash( name, (wp_s32)title_len, WORKPHONE_WINDOW_TITLE );
    win = wp_find_window( ctx, title_hash, name );
    if( !win )
        return;
    if( s == WORKPHONE_HIDDEN )
    {
        win->flags |= WORKPHONE_WINDOW_HIDDEN;
    }
    else
        win->flags &= ~(wp_flags)WORKPHONE_WINDOW_HIDDEN;
}

void wp_window_show_if( struct wp_context *ctx, const wp_c8 *name, enum wp_show_states s, wp_s32 cond )
{
    WORKPHONE_ASSERT( ctx );
    if( !ctx || !cond )
        return;
    wp_window_show( ctx, name, s );
}

void wp_window_set_focus( struct wp_context *ctx, const wp_c8 *name )
{
    wp_s32 title_len;
    wp_hash title_hash;
    struct wp_window *win;
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return;

    title_len = (wp_s32)wp_strlen( name );
    title_hash = wp_murmur_hash( name, (wp_s32)title_len, WORKPHONE_WINDOW_TITLE );
    win = wp_find_window( ctx, title_hash, name );
    if( win && ctx->end != win )
    {
        wp_remove_window( ctx, win );
        wp_insert_window( ctx, win, WORKPHONE_INSERT_BACK );
    }
    ctx->active = win;
}

void wp_rule_horizontal( struct wp_context *ctx, struct wp_color color, wp_bool rounding )
{
    struct wp_rect space;
    enum wp_widget_layout_states state = wp_widget( &space, ctx );
    struct wp_command_buffer *canvas = wp_window_get_canvas( ctx );
    if( !state )
        return;
    wp_fill_rect( canvas, space, rounding && space.h > 1.5f ? space.h / 2.0f : 0, color );
}
