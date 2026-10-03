#include "workphone_context.h"
#include "workphone_page_element.h"
#include "workphone_pool.h"
#include "workphone_window.h"

#pragma warning( push )
#pragma warning( disable : 4116 )

struct wp_page_element *wp_create_page_element( struct wp_context *ctx )
{
    struct wp_page_element *elem;
    if( ctx->freelist )
    {
        /* unlink page element from free list */
        elem = ctx->freelist;
        ctx->freelist = elem->next;
    }
    else if( ctx->use_pool )
    {
        /* allocate page element from memory pool */
        elem = wp_pool_alloc( &ctx->pool );
        WORKPHONE_ASSERT( elem );
        if( !elem )
            return 0;
    }
    else
    {
        /* allocate new page element from back of fixed size memory buffer */
        WORKPHONE_STORAGE const wp_size size = sizeof( struct wp_page_element );
        WORKPHONE_STORAGE const wp_size align = WORKPHONE_ALIGNOF( struct wp_page_element );
        elem = (struct wp_page_element *)wp_buffer_alloc( &ctx->memory, WORKPHONE_BUFFER_BACK, size,
                                                          align );
        WORKPHONE_ASSERT( elem );
        if( !elem )
            return 0;
    }
    wp_zero_struct( *elem );
    elem->next = 0;
    elem->prev = 0;
    return elem;
}
WORKPHONE_LIB void wp_liwp_page_element_into_freelist( struct wp_context *ctx,
                                                       struct wp_page_element *elem )
{
    /* link table into freelist */
    if( !ctx->freelist )
    {
        ctx->freelist = elem;
    }
    else
    {
        elem->next = ctx->freelist;
        ctx->freelist = elem;
    }
}
WORKPHONE_LIB void wp_free_page_element( struct wp_context *ctx, struct wp_page_element *elem )
{
    /* we have a pool so just add to free list */
    if( ctx->use_pool )
    {
        wp_liwp_page_element_into_freelist( ctx, elem );
        return;
    }
    /* if possible remove last element from back of fixed memory buffer */
    {
        void *elem_end = (void *)( elem + 1 );
        void *buffer_end = (wp_byte *)ctx->memory.memory.ptr + ctx->memory.size;
        if( elem_end == buffer_end )
            ctx->memory.size -= sizeof( struct wp_page_element );
        else
            wp_liwp_page_element_into_freelist( ctx, elem );
    }
}

#pragma warning( pop )
