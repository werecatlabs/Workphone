#include "workphone.h"
#include "workphone_page_element.h"

WORKPHONE_LIB struct wp_table *wp_create_table( struct wp_context *ctx )
{
    struct wp_page_element *elem;
    elem = wp_create_page_element( ctx );
    if( !elem )
        return 0;
    wp_zero_struct( *elem );
    return &elem->data.tbl;
}
WORKPHONE_LIB void wp_free_table( struct wp_context *ctx, struct wp_table *tbl )
{
    union wp_page_data *pd = WORKPHONE_CONTAINER_OF( tbl, union wp_page_data, tbl );
    struct wp_page_element *pe = WORKPHONE_CONTAINER_OF( pd, struct wp_page_element, data );
    wp_free_page_element( ctx, pe );
}
WORKPHONE_LIB void wp_push_table( struct wp_window *win, struct wp_table *tbl )
{
    if( !win->tables )
    {
        win->tables = tbl;
        tbl->next = 0;
        tbl->prev = 0;
        tbl->size = 0;
        win->table_count = 1;
        return;
    }
    win->tables->prev = tbl;
    tbl->next = win->tables;
    tbl->prev = 0;
    tbl->size = 0;
    win->tables = tbl;
    win->table_count++;
}
WORKPHONE_LIB void wp_remove_table( struct wp_window *win, struct wp_table *tbl )
{
    if( win->tables == tbl )
        win->tables = tbl->next;
    if( tbl->next )
        tbl->next->prev = tbl->prev;
    if( tbl->prev )
        tbl->prev->next = tbl->next;
    tbl->next = 0;
    tbl->prev = 0;
}
WORKPHONE_LIB wp_u32 *wp_add_value( struct wp_context *ctx, struct wp_window *win, wp_hash name,
                                    wp_u32 value )
{
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( win );
    if( !win || !ctx )
        return 0;
    if( !win->tables || win->tables->size >= WORKPHONE_VALUE_PAGE_CAPACITY )
    {
        struct wp_table *tbl = wp_create_table( ctx );
        WORKPHONE_ASSERT( tbl );
        if( !tbl )
            return 0;
        wp_push_table( win, tbl );
    }
    win->tables->seq = win->seq;
    win->tables->keys[win->tables->size] = name;
    win->tables->values[win->tables->size] = value;
    return &win->tables->values[win->tables->size++];
}
WORKPHONE_LIB wp_u32 *wp_find_value( const struct wp_window *win, wp_hash name )
{
    struct wp_table *iter = win->tables;
    while( iter )
    {
        wp_u32 i = 0;
        wp_u32 size = iter->size;
        for( i = 0; i < size; ++i )
        {
            if( iter->keys[i] == name )
            {
                iter->seq = win->seq;
                return &iter->values[i];
            }
        }
        size = WORKPHONE_VALUE_PAGE_CAPACITY;
        iter = iter->next;
    }
    return 0;
}
