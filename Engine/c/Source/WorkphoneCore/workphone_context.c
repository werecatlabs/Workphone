#include "workphone_pool.h"
#include "workphone_command_buffer.h"
#include "workphone_cursor.h"
#include "workphone_context.h"
#include "workphone_table.h"
#include "workphone_ui.h"

WORKPHONE_INTERN void wp_setup( struct wp_context *ctx, const struct wp_user_font *font )
{
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return;
    wp_zero_struct( *ctx );
    wp_style_default( ctx );
    ctx->seq = 1;
    if( font )
        ctx->style.font = font;
#ifdef WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT
    wp_draw_list_init( &ctx->draw_list );
#endif
}
#ifdef WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR
WORKPHONE_API wp_bool wp_init_default( struct wp_context *ctx, const struct wp_user_font *font )
{
    struct wp_allocator alloc;
    alloc.userdata.ptr = 0;
    alloc.alloc = wp_malloc;
    alloc.free = wp_mfree;
    return wp_init( ctx, &alloc, font );
}
#endif
WORKPHONE_API wp_bool wp_init_fixed( struct wp_context *ctx, void *memory, wp_size size,
                                     const struct wp_user_font *font )
{
    WORKPHONE_ASSERT( memory );
    if( !memory )
        return 0;
    wp_setup( ctx, font );
    wp_buffer_init_fixed( &ctx->memory, memory, size );
    ctx->use_pool = wp_false;
    return 1;
}
WORKPHONE_API wp_bool wp_init_custom( struct wp_context *ctx, struct wp_buffer *cmds,
                                      struct wp_buffer *pool, const struct wp_user_font *font )
{
    WORKPHONE_ASSERT( cmds );
    WORKPHONE_ASSERT( pool );
    if( !cmds || !pool )
        return 0;

    wp_setup( ctx, font );
    ctx->memory = *cmds;
    if( pool->type == WORKPHONE_BUFFER_FIXED )
    {
        /* take memory from buffer and alloc fixed pool */
        wp_pool_init_fixed( &ctx->pool, pool->memory.ptr, pool->memory.size );
    }
    else
    {
        /* create dynamic pool from buffer allocator */
        struct wp_allocator *alloc = &pool->pool;
        wp_pool_init( &ctx->pool, alloc, WORKPHONE_POOL_DEFAULT_CAPACITY );
    }
    ctx->use_pool = wp_true;
    return 1;
}
WORKPHONE_API wp_bool wp_init( struct wp_context *ctx, const struct wp_allocator *alloc,
                               const struct wp_user_font *font )
{
    WORKPHONE_ASSERT( alloc );
    if( !alloc )
        return 0;
    wp_setup( ctx, font );
    wp_buffer_init( &ctx->memory, alloc, WORKPHONE_DEFAULT_COMMAND_BUFFER_SIZE );
    wp_pool_init( &ctx->pool, alloc, WORKPHONE_POOL_DEFAULT_CAPACITY );
    ctx->use_pool = wp_true;
    return 1;
}
#ifdef WORKPHONE_INCLUDE_COMMAND_USERDATA
WORKPHONE_API void wp_set_user_data( struct wp_context *ctx, wp_handle handle )
{
    if( !ctx )
        return;
    ctx->userdata = handle;
    if( ctx->current )
        ctx->current->buffer.userdata = handle;
}
#endif
WORKPHONE_API void wp_free( struct wp_context *ctx )
{
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return;
    wp_buffer_free( &ctx->memory );
    if( ctx->use_pool )
        wp_pool_free( &ctx->pool );

    wp_zero( &ctx->input, sizeof( ctx->input ) );
    wp_zero( &ctx->style, sizeof( ctx->style ) );
    wp_zero( &ctx->memory, sizeof( ctx->memory ) );

    ctx->seq = 0;
    ctx->build = 0;
    ctx->begin = 0;
    ctx->end = 0;
    ctx->active = 0;
    ctx->current = 0;
    ctx->freelist = 0;
    ctx->count = 0;
}
WORKPHONE_API void wp_clear( struct wp_context *ctx )
{
    struct wp_window *iter;
    struct wp_window *next;
    WORKPHONE_ASSERT( ctx );

    if( !ctx )
        return;
    if( ctx->use_pool )
        wp_buffer_clear( &ctx->memory );
    else
        wp_buffer_reset( &ctx->memory, WORKPHONE_BUFFER_FRONT );

    ctx->build = 0;
    ctx->memory.calls = 0;
    ctx->last_widget_state = 0;
    ctx->style.cursor_active = ctx->style.cursors[WORKPHONE_CURSOR_ARROW];
    WORKPHONE_MEMSET( &ctx->overlay, 0, sizeof( ctx->overlay ) );

    /* garbage collector */
    iter = ctx->begin;
    while( iter )
    {
        /* make sure valid minimized windows do not get removed */
        if( ( iter->flags & WORKPHONE_WINDOW_MINIMIZED ) && !( iter->flags & WORKPHONE_WINDOW_CLOSED ) &&
            iter->seq == ctx->seq )
        {
            iter = iter->next;
            continue;
        }
        /* remove hotness from hidden or closed windows*/
        if( ( ( iter->flags & WORKPHONE_WINDOW_HIDDEN ) || ( iter->flags & WORKPHONE_WINDOW_CLOSED ) ) &&
            iter == ctx->active )
        {
            ctx->active = iter->prev;
            ctx->end = iter->prev;
            if( !ctx->end )
                ctx->begin = 0;
            if( ctx->active )
                ctx->active->flags &= ~(unsigned)WORKPHONE_WINDOW_ROM;
        }
        /* free unused popup windows */
        if( iter->popup.win && iter->popup.win->seq != ctx->seq )
        {
            wp_free_window( ctx, iter->popup.win );
            iter->popup.win = 0;
        }
        /* remove unused window state tables */
        {
            struct wp_table *n, *it = iter->tables;
            while( it )
            {
                n = it->next;
                if( it->seq != ctx->seq )
                {
                    wp_remove_table( iter, it );
                    wp_zero( it, sizeof( union wp_page_data ) );
                    wp_free_table( ctx, it );
                    if( it == iter->tables )
                        iter->tables = n;
                }
                it = n;
            }
        }
        /* window itself is not used anymore so free */
        if( iter->seq != ctx->seq || iter->flags & WORKPHONE_WINDOW_CLOSED )
        {
            next = iter->next;
            wp_remove_window( ctx, iter );
            wp_free_window( ctx, iter );
            iter = next;
        }
        else
            iter = iter->next;
    }
    ctx->seq++;
}
WORKPHONE_LIB void wp_start_buffer( struct wp_context *ctx, struct wp_command_buffer *buffer )
{
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( buffer );
    if( !ctx || !buffer )
        return;
    buffer->begin = ctx->memory.allocated;
    buffer->end = buffer->begin;
    buffer->last = buffer->begin;
    buffer->clip = wp_null_rect;
}
WORKPHONE_LIB void wp_start( struct wp_context *ctx, struct wp_window *win )
{
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( win );
    wp_start_buffer( ctx, &win->buffer );
}
WORKPHONE_LIB void wp_start_popup( struct wp_context *ctx, struct wp_window *win )
{
    struct wp_popup_buffer *buf;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( win );
    if( !ctx || !win )
        return;

    /* save buffer fill state for popup */
    buf = &win->popup.buf;
    buf->begin = win->buffer.end;
    buf->end = win->buffer.end;
    buf->parent = win->buffer.last;
    buf->last = buf->begin;
    buf->active = wp_true;
}
WORKPHONE_LIB void wp_finish_popup( struct wp_context *ctx, struct wp_window *win )
{
    struct wp_popup_buffer *buf;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( win );
    if( !ctx || !win )
        return;

    buf = &win->popup.buf;
    buf->last = win->buffer.last;
    buf->end = win->buffer.end;
}
WORKPHONE_LIB void wp_finish_buffer( struct wp_context *ctx, struct wp_command_buffer *buffer )
{
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( buffer );
    if( !ctx || !buffer )
        return;
    buffer->end = ctx->memory.allocated;
}
WORKPHONE_LIB void wp_finish( struct wp_context *ctx, struct wp_window *win )
{
    struct wp_popup_buffer *buf;
    struct wp_command *parent_last;
    void *memory;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( win );
    if( !ctx || !win )
        return;
    wp_finish_buffer( ctx, &win->buffer );
    if( !win->popup.buf.active )
        return;

    buf = &win->popup.buf;
    memory = ctx->memory.memory.ptr;
    parent_last = wp_ptr_add( struct wp_command, memory, buf->parent );
    parent_last->next = buf->end;
}
WORKPHONE_LIB void wp_build( struct wp_context *ctx )
{
    struct wp_window *it = 0;
    struct wp_command *cmd = 0;
    wp_byte *buffer = 0;

    /* draw cursor overlay */
    if( !ctx->style.cursor_active )
        ctx->style.cursor_active = ctx->style.cursors[WORKPHONE_CURSOR_ARROW];
    if( ctx->style.cursor_active && !ctx->input.mouse.grabbed && ctx->style.cursor_visible )
    {
        struct wp_rect mouse_bounds;
        const struct wp_cursor *cursor = ctx->style.cursor_active;
        wp_command_buffer_init( &ctx->overlay, &ctx->memory, WORKPHONE_CLIPPING_OFF );
        wp_start_buffer( ctx, &ctx->overlay );

        mouse_bounds.x = ctx->input.mouse.pos.x - cursor->offset.x;
        mouse_bounds.y = ctx->input.mouse.pos.y - cursor->offset.y;
        mouse_bounds.w = cursor->size.x;
        mouse_bounds.h = cursor->size.y;

        wp_draw_image( &ctx->overlay, mouse_bounds, &cursor->img, wp_white );
        wp_finish_buffer( ctx, &ctx->overlay );
    }
    /* build one big draw command list out of all window buffers */
    it = ctx->begin;
    buffer = (wp_byte *)ctx->memory.memory.ptr;
    while( it != 0 )
    {
        struct wp_window *next = it->next;
        if( it->buffer.last == it->buffer.begin || ( it->flags & WORKPHONE_WINDOW_HIDDEN ) ||
            it->seq != ctx->seq )
            goto cont;

        cmd = wp_ptr_add( struct wp_command, buffer, it->buffer.last );
        while( next && ( ( next->buffer.last == next->buffer.begin ) ||
                         ( next->flags & WORKPHONE_WINDOW_HIDDEN ) || next->seq != ctx->seq ) )
            next = next->next; /* skip empty command buffers */

        if( next )
            cmd->next = next->buffer.begin;
    cont:
        it = next;
    }
    /* append all popup draw commands into lists */
    it = ctx->begin;
    while( it != 0 )
    {
        struct wp_window *next = it->next;
        struct wp_popup_buffer *buf;
        if( !it->popup.buf.active )
            goto skip;

        buf = &it->popup.buf;
        cmd->next = buf->begin;
        cmd = wp_ptr_add( struct wp_command, buffer, buf->last );
        buf->active = wp_false;
    skip:
        it = next;
    }
    if( cmd )
    {
        /* append overlay commands */
        if( ctx->overlay.end != ctx->overlay.begin )
            cmd->next = ctx->overlay.begin;
        else
            cmd->next = ctx->memory.allocated;
    }
}
WORKPHONE_API const struct wp_command *wp__begin( struct wp_context *ctx )
{
    struct wp_window *iter;
    wp_byte *buffer;
    WORKPHONE_ASSERT( ctx );
    if( !ctx )
        return 0;
    if( !ctx->count )
        return 0;

    buffer = (wp_byte *)ctx->memory.memory.ptr;
    if( !ctx->build )
    {
        wp_build( ctx );
        ctx->build = wp_true;
    }
    iter = ctx->begin;
    while( iter && ( ( iter->buffer.begin == iter->buffer.end ) ||
                     ( iter->flags & WORKPHONE_WINDOW_HIDDEN ) || iter->seq != ctx->seq ) )
        iter = iter->next;
    if( !iter )
        return 0;
    return wp_ptr_add_const( struct wp_command, buffer, iter->buffer.begin );
}

WORKPHONE_API const struct wp_command *wp__next( struct wp_context *ctx, const struct wp_command *cmd )
{
    wp_byte *buffer;
    const struct wp_command *next;
    WORKPHONE_ASSERT( ctx );
    if( !ctx || !cmd || !ctx->count )
        return 0;
    if( cmd->next >= ctx->memory.allocated )
        return 0;
    buffer = (wp_byte *)ctx->memory.memory.ptr;
    next = wp_ptr_add_const( struct wp_command, buffer, cmd->next );
    return next;
}
