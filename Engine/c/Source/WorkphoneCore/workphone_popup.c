#include "workphone_popup.h"
#include "workphone_util.h"
#include "workphone.h"

WORKPHONE_API wp_bool wp_popup_begin( struct wp_context *ctx, enum wp_popup_type type,
                                      const wp_c8 *title, wp_flags flags, struct wp_rect rect )
{
    struct wp_window *popup;
    struct wp_window *win;
    struct wp_panel *panel;

    wp_s32 title_len;
    wp_hash title_hash;
    wp_size allocated;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( title );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    win = ctx->current;
    panel = win->layout;
    WORKPHONE_ASSERT( !( (wp_s32)panel->type & (wp_s32)WORKPHONE_PANEL_SET_POPUP ) &&
                      "popups are not allowed to have popups" );
    (void)panel;
    title_len = (wp_s32)wp_strlen( title );
    title_hash = wp_murmur_hash( title, (wp_s32)title_len, WORKPHONE_PANEL_POPUP );

    popup = win->popup.win;
    if( !popup )
    {
        popup = (struct wp_window *)wp_create_window( ctx );
        popup->parent = win;
        win->popup.win = popup;
        win->popup.active = 0;
        win->popup.type = WORKPHONE_PANEL_POPUP;
    }

    /* make sure we have correct popup */
    if( win->popup.name != title_hash )
    {
        if( !win->popup.active )
        {
            wp_zero( popup, sizeof( *popup ) );
            win->popup.name = title_hash;
            win->popup.active = 1;
            win->popup.type = WORKPHONE_PANEL_POPUP;
        }
        else
            return 0;
    }

    /* popup position is local to window */
    ctx->current = popup;
    rect.x += win->layout->clip.x;
    rect.y += win->layout->clip.y;

    /* setup popup data */
    popup->parent = win;
    popup->bounds = rect;
    popup->seq = ctx->seq;
    popup->layout = (struct wp_panel *)wp_create_panel( ctx );
    popup->flags = flags;
    popup->flags |= WORKPHONE_WINDOW_BORDER;
    if( type == WORKPHONE_POPUP_DYNAMIC )
        popup->flags |= WORKPHONE_WINDOW_DYNAMIC;

    popup->buffer = win->buffer;
    wp_start_popup( ctx, win );
    allocated = ctx->memory.allocated;
    wp_push_scissor( &popup->buffer, wp_null_rect );

    if( wp_panel_begin( ctx, title, WORKPHONE_PANEL_POPUP ) )
    {
        /* popup is running therefore invalidate parent panels */
        struct wp_panel *root;
        root = win->layout;
        while( root )
        {
            root->flags |= WORKPHONE_WINDOW_ROM;
            root->flags &= ~(wp_flags)WORKPHONE_WINDOW_REMOVE_ROM;
            root = root->parent;
        }
        win->popup.active = 1;
        popup->layout->offset_x = &popup->scrollbar.x;
        popup->layout->offset_y = &popup->scrollbar.y;
        popup->layout->parent = win->layout;
        return 1;
    }
    else
    {
        /* popup was closed/is invalid so cleanup */
        struct wp_panel *root;
        root = win->layout;
        while( root )
        {
            root->flags |= WORKPHONE_WINDOW_REMOVE_ROM;
            root = root->parent;
        }
        win->popup.buf.active = 0;
        win->popup.active = 0;
        ctx->memory.allocated = allocated;
        ctx->current = win;
        wp_free_panel( ctx, popup->layout );
        popup->layout = 0;
        return 0;
    }
}
WORKPHONE_LIB wp_bool wp_nonblock_begin( struct wp_context *ctx, wp_flags flags, struct wp_rect body,
                                         struct wp_rect header, enum wp_panel_type panel_type )
{
    struct wp_window *popup;
    struct wp_window *win;
    struct wp_panel *panel;
    wp_s32 is_active = wp_true;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return 0;

    /* popups cannot have popups */
    win = ctx->current;
    panel = win->layout;
    WORKPHONE_ASSERT( !( (wp_s32)panel->type & (wp_s32)WORKPHONE_PANEL_SET_POPUP ) );
    (void)panel;
    popup = win->popup.win;
    if( !popup )
    {
        /* create window for nonblocking popup */
        popup = (struct wp_window *)wp_create_window( ctx );
        popup->parent = win;
        win->popup.win = popup;
        win->popup.type = panel_type;
        wp_command_buffer_init( &popup->buffer, &ctx->memory, WORKPHONE_CLIPPING_ON );
    }
    else
    {
        /* close the popup if user pressed outside or in the header */
        wp_s32 pressed, in_body, in_header;
#ifdef WORKPHONE_BUTTON_TRIGGER_ON_RELEASE
        pressed = wp_input_is_mouse_released( &ctx->input, WORKPHONE_BUTTON_LEFT );
#else
        pressed = wp_input_is_mouse_pressed( &ctx->input, WORKPHONE_BUTTON_LEFT );
#endif
        in_body = wp_input_is_mouse_hovering_rect( &ctx->input, body );
        in_header = wp_input_is_mouse_hovering_rect( &ctx->input, header );
        if( pressed && ( !in_body || in_header ) )
            is_active = wp_false;
    }
    win->popup.header = header;

    if( !is_active )
    {
        /* remove read only mode from all parent panels */
        struct wp_panel *root = win->layout;
        while( root )
        {
            root->flags |= WORKPHONE_WINDOW_REMOVE_ROM;
            root = root->parent;
        }
        return is_active;
    }
    popup->bounds = body;
    popup->parent = win;
    popup->layout = (struct wp_panel *)wp_create_panel( ctx );
    popup->flags = flags;
    popup->flags |= WORKPHONE_WINDOW_BORDER;
    popup->flags |= WORKPHONE_WINDOW_DYNAMIC;
    popup->seq = ctx->seq;
    win->popup.active = 1;
    WORKPHONE_ASSERT( popup->layout );

    wp_start_popup( ctx, win );
    popup->buffer = win->buffer;
    wp_push_scissor( &popup->buffer, wp_null_rect );
    ctx->current = popup;

    wp_panel_begin( ctx, 0, panel_type );
    win->buffer = popup->buffer;
    popup->layout->parent = win->layout;
    popup->layout->offset_x = &popup->scrollbar.x;
    popup->layout->offset_y = &popup->scrollbar.y;

    /* set read only mode to all parent panels */
    {
        struct wp_panel *root;
        root = win->layout;
        while( root )
        {
            root->flags |= WORKPHONE_WINDOW_ROM;
            root = root->parent;
        }
    }
    return is_active;
}
WORKPHONE_API void wp_popup_close( struct wp_context *ctx )
{
    struct wp_window *popup;
    WORKPHONE_ASSERT( ctx );
    if( !ctx || !ctx->current )
        return;

    popup = ctx->current;
    WORKPHONE_ASSERT( popup->parent );
    WORKPHONE_ASSERT( (wp_s32)popup->layout->type & (wp_s32)WORKPHONE_PANEL_SET_POPUP );
    popup->flags |= WORKPHONE_WINDOW_HIDDEN;
}
WORKPHONE_API void wp_popup_end( struct wp_context *ctx )
{
    struct wp_window *win;
    struct wp_window *popup;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    popup = ctx->current;
    if( !popup->parent )
        return;
    win = popup->parent;
    if( popup->flags & WORKPHONE_WINDOW_HIDDEN )
    {
        struct wp_panel *root;
        root = win->layout;
        while( root )
        {
            root->flags |= WORKPHONE_WINDOW_REMOVE_ROM;
            root = root->parent;
        }
        win->popup.active = 0;
    }
    wp_push_scissor( &popup->buffer, wp_null_rect );
    wp_end( ctx );

    win->buffer = popup->buffer;
    wp_finish_popup( ctx, win );
    ctx->current = win;
    wp_push_scissor( &win->buffer, win->layout->clip );
}
WORKPHONE_API void wp_popup_get_scroll( const struct wp_context *ctx, wp_u32 *offset_x,
                                        wp_u32 *offset_y )
{
    struct wp_window *popup;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    popup = ctx->current;
    if( offset_x )
        *offset_x = popup->scrollbar.x;
    if( offset_y )
        *offset_y = popup->scrollbar.y;
}
WORKPHONE_API void wp_popup_set_scroll( struct wp_context *ctx, wp_u32 offset_x, wp_u32 offset_y )
{
    struct wp_window *popup;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    popup = ctx->current;
    popup->scrollbar.x = offset_x;
    popup->scrollbar.y = offset_y;
}
