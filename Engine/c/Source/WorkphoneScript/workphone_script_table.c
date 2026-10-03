#include <string.h>
#include <stdlib.h>
#include "workphone_script_table.h"

static wp_c8 *wp_script_table_strdup( const wp_c8 *source )
{
    size_t length;
    wp_c8 *copy;

    if( !source )
        return NULL;

    length = strlen( source ) + 1u;
    copy = (wp_c8 *)malloc( length );
    if( copy )
        memcpy( copy, source, length );
    return copy;
}

WP_SCRIPT_LIB wp_u32 wp_script_table_hash( const wp_c8 *s )
{
    wp_u32 h = 0;

    if( !s )
        return 0;

    while( *s )
        h = h * 31u + (wp_u8)*s++;

    return h;
}

WP_SCRIPT_LIB wp_s32 wp_script_table_set( wp_script_table *t, const wp_c8 *key,
                                          wp_script_value value )
{
    wp_u32 start;
    wp_u32 probe;
    wp_u32 index;
    wp_c8 *copy;

    if( !t || !key || !key[0] )
        return 0;

    start = wp_script_table_hash( key ) % TABLE_SIZE;
    for( probe = 0; probe < TABLE_SIZE; ++probe )
    {
        index = ( start + probe ) % TABLE_SIZE;
        if( t->entries[index].key )
        {
            if( strcmp( t->entries[index].key, key ) == 0 )
            {
                t->entries[index].value = value;
                return 1;
            }
        }
        else
        {
            copy = wp_script_table_strdup( key );
            if( !copy )
                return 0;
            t->entries[index].key = copy;
            t->entries[index].value = value;
            return 1;
        }
    }

    return 0;
}

WP_SCRIPT_LIB wp_script_value wp_script_table_get( const wp_script_table *t, const wp_c8 *key )
{
    wp_u32 start;
    wp_u32 probe;
    wp_u32 index;

    if( !t || !key || !key[0] )
        return wp_script_make_nil();

    start = wp_script_table_hash( key ) % TABLE_SIZE;
    for( probe = 0; probe < TABLE_SIZE; ++probe )
    {
        index = ( start + probe ) % TABLE_SIZE;
        if( !t->entries[index].key )
            return wp_script_make_nil();
        if( strcmp( t->entries[index].key, key ) == 0 )
            return t->entries[index].value;
    }

    return wp_script_make_nil();
}

WP_SCRIPT_LIB wp_s32 wp_script_table_contains( const wp_script_table *t, const wp_c8 *key )
{
    return wp_script_table_get( t, key ).type != VAL_NIL;
}

WP_SCRIPT_LIB void wp_script_table_clear( wp_script_table *t )
{
    wp_s32 i;

    if( !t )
        return;

    for( i = 0; i < TABLE_SIZE; ++i )
    {
        free( t->entries[i].key );
        t->entries[i].key = NULL;
        t->entries[i].value = wp_script_make_nil();
    }
}
