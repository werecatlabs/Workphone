#include "workphone_pool.h"
#include "workphone.h"

void wp_pool_init( struct wp_pool *pool, const struct wp_allocator *alloc, wp_u32 capacity )
{
    WORKPHONE_ASSERT( capacity >= 1 );
    wp_zero( pool, sizeof( *pool ) );
    pool->alloc = *alloc;
    pool->capacity = capacity;
    pool->type = WORKPHONE_BUFFER_DYNAMIC;
    pool->pages = 0;
}

void wp_pool_free( struct wp_pool *pool )
{
    struct wp_page *iter;
    if( !pool )
        return;
    iter = pool->pages;
    if( pool->type == WORKPHONE_BUFFER_FIXED )
        return;
    while( iter )
    {
        struct wp_page *next = iter->next;
        pool->alloc.free( pool->alloc.userdata, iter );
        iter = next;
    }
}

void wp_pool_init_fixed( struct wp_pool *pool, void *memory, wp_size size )
{
    wp_zero( pool, sizeof( *pool ) );
    WORKPHONE_ASSERT( size >= sizeof( struct wp_page ) );
    if( size < sizeof( struct wp_page ) )
        return;
    /* first wp_page_element is embedded in wp_page, additional elements follow in adjacent space */
    pool->capacity =
        (unsigned)( 1 + ( size - sizeof( struct wp_page ) ) / sizeof( struct wp_page_element ) );
    pool->pages = (struct wp_page *)memory;
    pool->type = WORKPHONE_BUFFER_FIXED;
    pool->size = size;
}

struct wp_page_element *wp_pool_alloc( struct wp_pool *pool )
{
    if( !pool->pages || pool->pages->size >= pool->capacity )
    {
        /* allocate new page */
        struct wp_page *page;
        if( pool->type == WORKPHONE_BUFFER_FIXED )
        {
            WORKPHONE_ASSERT( pool->pages );
            if( !pool->pages )
                return 0;
            WORKPHONE_ASSERT( pool->pages->size < pool->capacity );
            return 0;
        }
        else
        {
            wp_size size = sizeof( struct wp_page );
            size += ( pool->capacity - 1 ) * sizeof( struct wp_page_element );
            page = (struct wp_page *)pool->alloc.alloc( pool->alloc.userdata, 0, size );
            page->next = pool->pages;
            pool->pages = page;
            page->size = 0;
        }
    }
    return &pool->pages->win[pool->pages->size++];
}
