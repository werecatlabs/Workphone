/**
 * @file wp_string.c
 * @brief Implementation of the C string API.
 */
#include "workphone.h"
#include "workphone_prerequisites.h"
#include "workphone_string.h"
#include "workphone_ui.h"
#include "workphone_utf8.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define WORKPHONE_STRING_MIN_CAPACITY 16

static wp_u32 wp_str_next_capacity( wp_u32 needed )
{
    wp_u32 cap = WORKPHONE_STRING_MIN_CAPACITY;
    while( cap < needed )
        cap *= 2;
    return cap;
}

static void wp_str_ensure( wp_string *s, wp_u32 needed )
{
    wp_u32 cap;
    if( needed <= s->capacity )
        return;
    cap = wp_str_next_capacity( needed );
    s->data = (wp_c8 *)realloc( s->data, cap + 1 );
    s->capacity = cap;
}

wp_string wp_string_make( const wp_c8 *src )
{
    wp_string s;
    wp_u32 len = src ? wp_strlen( src ) : 0;
    wp_u32 cap = wp_str_next_capacity( len );
    s.data = (wp_c8 *)malloc( cap + 1 );
    s.length = len;
    s.capacity = cap;
    if( len > 0 )
        memcpy( s.data, src, len );

    s.data[len] = '\0';
    return s;
}

wp_string wp_string_make_len( const wp_c8 *src, wp_u32 len )
{
    wp_string s;
    wp_u32 cap = wp_str_next_capacity( len );
    s.data = (wp_c8 *)malloc( cap + 1 );
    s.length = len;
    s.capacity = cap;
    if( src && len > 0 )
        memcpy( s.data, src, len );

    s.data[len] = '\0';
    return s;
}

wp_string wp_string_make_empty( void )
{
    wp_string s;
    s.data = (wp_c8 *)malloc( WORKPHONE_STRING_MIN_CAPACITY + 1 );
    s.data[0] = '\0';
    s.length = 0;
    s.capacity = WORKPHONE_STRING_MIN_CAPACITY;
    return s;
}

wp_string wp_string_copy( wp_string s )
{
    return wp_string_make_len( s.data, s.length );
}

void wp_string_free( wp_string *s )
{
    if( s->data )
    {
        free( s->data );
        s->data = NULL;
    }

    s->length = 0;
    s->capacity = 0;
}

void wp_string_clear( wp_string *s )
{
    s->length = 0;

    if( s->data )
        s->data[0] = '\0';
}

void wp_string_reserve( wp_string *s, wp_u32 capacity )
{
    if( capacity <= s->capacity )
        return;

    s->data = (wp_c8 *)realloc( s->data, capacity + 1 );
    s->capacity = capacity;
}

wp_c8 *wp_strstr( const wp_c8 *s, const wp_c8 *needle )
{
    const wp_c8 *p;
    if( !s || !needle )
        return NULL;

    p = strstr( s, needle );

    if( !p )
        return NULL;

    return (wp_c8 *)( p );
}

wp_u32 wp_string_length( const wp_string *s )
{
    return s->length;
}

wp_s32 wp_string_is_empty( const wp_string *s )
{
    return s->length == 0;
}

wp_s32 wp_string_equals( const wp_string *a, const wp_string *b )
{
    if( a->length != b->length )
        return 0;

    return memcmp( a->data, b->data, a->length ) == 0;
}

wp_s32 wp_string_equals_cstr( const wp_string *a, const wp_c8 *b )
{
    if( !b )
        return a->length == 0;

    return strcmp( a->data, b ) == 0;
}

wp_s32 wp_string_compare( const wp_string *a, const wp_string *b )
{
    return strcmp( a->data, b->data );
}

wp_s32 wp_string_find( const wp_string *s, const wp_c8 *needle, wp_u32 from )
{
    const wp_c8 *p;
    if( !needle || from >= s->length )
        return -1;

    p = strstr( s->data + from, needle );

    if( !p )
        return -1;

    return (wp_s32)( p - s->data );
}

wp_s32 wp_string_rfind( const wp_string *s, const wp_c8 *needle )
{
    wp_u32 nlen;
    wp_s32 last = -1;
    wp_s32 pos = 0;

    if( !needle || s->length == 0 )
        return -1;

    nlen = wp_strlen( needle );

    if( nlen == 0 || nlen > s->length )
        return -1;

    while( pos <= (wp_s32)( s->length - nlen ) )
    {
        const wp_c8 *p = wp_strstr( s->data + pos, needle );
        if( !p )
            break;

        last = (wp_s32)( p - s->data );
        pos = last + 1;
    }
    return last;
}

wp_s32 wp_string_contains( const wp_string *s, const wp_c8 *needle )
{
    return wp_string_find( s, needle, 0 ) >= 0;
}

wp_s32 wp_string_starts_with( const wp_string *s, const wp_c8 *prefix )
{
    wp_u32 plen;

    if( !prefix )
        return 1;

    plen = (wp_u32)strlen( prefix );

    if( plen > s->length )
        return 0;

    return memcmp( s->data, prefix, plen ) == 0;
}

wp_s32 wp_string_ends_with( const wp_string *s, const wp_c8 *suffix )
{
    wp_u32 slen;
    if( !suffix )
        return 1;
    slen = (wp_u32)strlen( suffix );
    if( slen > s->length )
        return 0;
    return memcmp( s->data + s->length - slen, suffix, slen ) == 0;
}

void wp_string_append( wp_string *s, const wp_c8 *src )
{
    wp_u32 src_len;
    if( !src )
        return;
    src_len = (wp_u32)strlen( src );
    wp_str_ensure( s, s->length + src_len );
    memcpy( s->data + s->length, src, src_len + 1 );
    s->length += src_len;
}

void wp_string_append_len( wp_string *s, const wp_c8 *src, wp_u32 len )
{
    if( !src || len == 0 )
        return;
    wp_str_ensure( s, s->length + len );
    memcpy( s->data + s->length, src, len );
    s->length += len;
    s->data[s->length] = '\0';
}

void wp_string_append_wp_c8( wp_string *s, wp_c8 c )
{
    wp_str_ensure( s, s->length + 1 );
    s->data[s->length++] = c;
    s->data[s->length] = '\0';
}

void wp_string_prepend( wp_string *s, const wp_c8 *src )
{
    wp_u32 src_len;
    if( !src )
        return;
    src_len = (wp_u32)strlen( src );
    if( src_len == 0 )
        return;
    wp_str_ensure( s, s->length + src_len );
    memmove( s->data + src_len, s->data, s->length + 1 );
    memcpy( s->data, src, src_len );
    s->length += src_len;
}

void wp_string_insert( wp_string *s, wp_u32 pos, const wp_c8 *src )
{
    wp_u32 src_len;
    if( !src || pos > s->length )
        return;
    src_len = (wp_u32)strlen( src );
    if( src_len == 0 )
        return;
    wp_str_ensure( s, s->length + src_len );
    memmove( s->data + pos + src_len, s->data + pos, s->length - pos + 1 );
    memcpy( s->data + pos, src, src_len );
    s->length += src_len;
}

void wp_string_erase( wp_string *s, wp_u32 pos, wp_u32 count )
{
    if( pos >= s->length )
        return;
    if( pos + count > s->length )
        count = s->length - pos;
    memmove( s->data + pos, s->data + pos + count, s->length - pos - count + 1 );
    s->length -= count;
}

void wp_string_replace_first( wp_string *s, const wp_c8 *from, const wp_c8 *to )
{
    wp_u32 from_len;
    wp_s32 pos;
    if( !from || !to )
        return;
    from_len = (wp_u32)strlen( from );
    pos = wp_string_find( s, from, 0 );
    if( pos < 0 )
        return;
    wp_string_erase( s, (wp_u32)pos, from_len );
    wp_string_insert( s, (wp_u32)pos, to );
}

void wp_string_replace_all( wp_string *s, const wp_c8 *from, const wp_c8 *to )
{
    wp_u32 from_len;
    wp_u32 to_len;
    wp_u32 search_from;
    wp_s32 pos;
    if( !from || !to )
        return;
    from_len = (wp_u32)strlen( from );
    to_len = (wp_u32)strlen( to );
    if( from_len == 0 )
        return;
    search_from = 0;
    while( ( pos = wp_string_find( s, from, search_from ) ) >= 0 )
    {
        wp_string_erase( s, (wp_u32)pos, from_len );
        wp_string_insert( s, (wp_u32)pos, to );
        search_from = (wp_u32)pos + to_len;
    }
}

void wp_string_to_upper( wp_string *s )
{
    wp_u32 i;
    for( i = 0; i < s->length; ++i )
        s->data[i] = (wp_c8)toupper( (wp_u8)s->data[i] );
}

void wp_string_to_lower( wp_string *s )
{
    wp_u32 i;
    for( i = 0; i < s->length; ++i )
        s->data[i] = (wp_c8)tolower( (wp_u8)s->data[i] );
}

void wp_string_trim_left( wp_string *s )
{
    wp_u32 i = 0;
    while( i < s->length && isspace( (wp_u8)s->data[i] ) )
        ++i;
    if( i > 0 )
    {
        memmove( s->data, s->data + i, s->length - i + 1 );
        s->length -= i;
    }
}

void wp_string_trim_right( wp_string *s )
{
    while( s->length > 0 && isspace( (wp_u8)s->data[s->length - 1] ) )
        --s->length;
    s->data[s->length] = '\0';
}

void wp_string_trim( wp_string *s )
{
    wp_string_trim_right( s );
    wp_string_trim_left( s );
}

wp_string wp_string_substr( const wp_string *s, wp_u32 pos, wp_u32 count )
{
    if( pos >= s->length )
        return wp_string_make_empty();
    if( pos + count > s->length )
        count = s->length - pos;
    return wp_string_make_len( s->data + pos, count );
}

wp_u32 wp_string_hash( const wp_string *s )
{
    /* FNV-1a 32-bit */
    wp_u32 hash = 2166136261u;
    wp_u32 i;
    for( i = 0; i < s->length; ++i )
    {
        hash ^= (wp_u8)s->data[i];
        hash *= 16777619u;
    }

    return hash;
}

#ifdef WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR
WORKPHONE_API void wp_str_init_default( struct wp_str *str )
{
    struct wp_allocator alloc;
    alloc.userdata.ptr = 0;
    alloc.alloc = wp_malloc;
    alloc.free = wp_mfree;
    wp_buffer_init( &str->buffer, &alloc, 32 );
    str->len = 0;
}
#endif

WORKPHONE_API void wp_str_init( struct wp_str *str, const struct wp_allocator *alloc, wp_size size )
{
    wp_buffer_init( &str->buffer, alloc, size );
    str->len = 0;
}
WORKPHONE_API void wp_str_init_fixed( struct wp_str *str, void *memory, wp_size size )
{
    wp_buffer_init_fixed( &str->buffer, memory, size );
    str->len = 0;
}
WORKPHONE_API wp_s32 wp_str_append_text_wp_c8( struct wp_str *s, const wp_c8 *str, wp_s32 len )
{
    wp_c8 *mem;

    WORKPHONE_ASSERT( s );
    WORKPHONE_ASSERT( str );

    if( !s || !str || !len )
        return 0;

    mem = (wp_c8 *)wp_buffer_alloc( &s->buffer, WORKPHONE_BUFFER_FRONT, (wp_size)len * sizeof( wp_c8 ),
                                    0 );

    if( !mem )
        return 0;

    WORKPHONE_MEMCPY( mem, str, (wp_size)len * sizeof( wp_c8 ) );
    s->len += wp_utf_len( str, len );
    return len;
}
WORKPHONE_API wp_s32 wp_str_append_str_wp_c8( struct wp_str *s, const wp_c8 *str )
{
    return wp_str_append_text_wp_c8( s, str, wp_strlen( str ) );
}
WORKPHONE_API wp_s32 wp_str_append_text_utf8( struct wp_str *str, const wp_c8 *text, wp_s32 len )
{
    wp_s32 i = 0;
    wp_s32 byte_len = 0;
    wp_rune unicode;
    if( !str || !text || !len )
        return 0;
    for( i = 0; i < len; ++i )
        byte_len += wp_utf_decode( text + byte_len, &unicode, 4 );
    wp_str_append_text_wp_c8( str, text, byte_len );
    return len;
}
WORKPHONE_API wp_s32 wp_str_append_str_utf8( struct wp_str *str, const wp_c8 *text )
{
    wp_s32 byte_len = 0;
    wp_s32 num_runes = 0;
    wp_s32 glyph_len = 0;
    wp_rune unicode;
    if( !str || !text )
        return 0;

    glyph_len = byte_len = wp_utf_decode( text + byte_len, &unicode, 4 );
    while( unicode != '\0' && glyph_len )
    {
        glyph_len = wp_utf_decode( text + byte_len, &unicode, 4 );
        byte_len += glyph_len;
        num_runes++;
    }
    wp_str_append_text_wp_c8( str, text, byte_len );
    return num_runes;
}
WORKPHONE_API wp_s32 wp_str_append_text_runes( struct wp_str *str, const wp_rune *text, wp_s32 len )
{
    wp_s32 i = 0;
    wp_s32 byte_len = 0;
    wp_glyph glyph;

    WORKPHONE_ASSERT( str );
    if( !str || !text || !len )
        return 0;
    for( i = 0; i < len; ++i )
    {
        byte_len = wp_utf_encode( text[i], glyph, WORKPHONE_UTF_SIZE );
        if( !byte_len )
            break;
        wp_str_append_text_wp_c8( str, glyph, byte_len );
    }
    return len;
}
WORKPHONE_API wp_s32 wp_str_append_str_runes( struct wp_str *str, const wp_rune *runes )
{
    wp_s32 i = 0;
    wp_glyph glyph;
    wp_s32 byte_len;
    WORKPHONE_ASSERT( str );
    if( !str || !runes )
        return 0;
    while( runes[i] != '\0' )
    {
        byte_len = wp_utf_encode( runes[i], glyph, WORKPHONE_UTF_SIZE );
        wp_str_append_text_wp_c8( str, glyph, byte_len );
        i++;
    }
    return i;
}
WORKPHONE_API wp_s32 wp_str_insert_at_wp_c8( struct wp_str *s, wp_s32 pos, const wp_c8 *str, wp_s32 len )
{
    wp_s32 i;
    void *mem;
    wp_c8 *src;
    wp_c8 *dst;

    wp_s32 copylen;
    WORKPHONE_ASSERT( s );
    WORKPHONE_ASSERT( str );
    WORKPHONE_ASSERT( len >= 0 );
    if( !s || !str || !len || (wp_size)pos > s->buffer.allocated )
        return 0;
    if( ( s->buffer.allocated + (wp_size)len >= s->buffer.memory.size ) &&
        ( s->buffer.type == WORKPHONE_BUFFER_FIXED ) )
        return 0;

    copylen = (wp_s32)s->buffer.allocated - pos;
    if( !copylen )
    {
        wp_str_append_text_wp_c8( s, str, len );
        return 1;
    }
    mem = wp_buffer_alloc( &s->buffer, WORKPHONE_BUFFER_FRONT, (wp_size)len * sizeof( wp_c8 ), 0 );
    if( !mem )
        return 0;

    /* memmove */
    WORKPHONE_ASSERT( ( (wp_s32)pos + (wp_s32)len + ( (wp_s32)copylen - 1 ) ) >= 0 );
    WORKPHONE_ASSERT( ( (wp_s32)pos + ( (wp_s32)copylen - 1 ) ) >= 0 );
    dst = wp_ptr_add( wp_c8, s->buffer.memory.ptr, pos + len + ( copylen - 1 ) );
    src = wp_ptr_add( wp_c8, s->buffer.memory.ptr, pos + ( copylen - 1 ) );
    for( i = 0; i < copylen; ++i )
        *dst-- = *src--;
    mem = wp_ptr_add( void, s->buffer.memory.ptr, pos );
    WORKPHONE_MEMCPY( mem, str, (wp_size)len * sizeof( wp_c8 ) );
    s->len = wp_utf_len( (wp_c8 *)s->buffer.memory.ptr, (wp_s32)s->buffer.allocated );
    return 1;
}
WORKPHONE_API wp_s32 wp_str_insert_at_rune( struct wp_str *str, wp_s32 pos, const wp_c8 *cstr,
                                            wp_s32 len )
{
    wp_s32 glyph_len;
    wp_rune unicode;
    const wp_c8 *begin;
    const wp_c8 *buffer;

    WORKPHONE_ASSERT( str );
    WORKPHONE_ASSERT( cstr );
    WORKPHONE_ASSERT( len );
    if( !str || !cstr || !len )
        return 0;
    begin = wp_str_at_rune( str, pos, &unicode, &glyph_len );
    if( !str->len )
        return wp_str_append_text_wp_c8( str, cstr, len );
    buffer = wp_str_get_const( str );
    if( !begin )
        return 0;
    return wp_str_insert_at_wp_c8( str, (wp_s32)( begin - buffer ), cstr, len );
}
WORKPHONE_API wp_s32 wp_str_insert_text_wp_c8( struct wp_str *str, wp_s32 pos, const wp_c8 *text,
                                               wp_s32 len )
{
    return wp_str_insert_text_utf8( str, pos, text, len );
}
WORKPHONE_API wp_s32 wp_str_insert_str_wp_c8( struct wp_str *str, wp_s32 pos, const wp_c8 *text )
{
    return wp_str_insert_text_utf8( str, pos, text, wp_strlen( text ) );
}
WORKPHONE_API wp_s32 wp_str_insert_text_utf8( struct wp_str *str, wp_s32 pos, const wp_c8 *text,
                                              wp_s32 len )
{
    wp_s32 i = 0;
    wp_s32 byte_len = 0;
    wp_rune unicode;

    WORKPHONE_ASSERT( str );
    WORKPHONE_ASSERT( text );
    if( !str || !text || !len )
        return 0;
    for( i = 0; i < len; ++i )
        byte_len += wp_utf_decode( text + byte_len, &unicode, 4 );
    wp_str_insert_at_rune( str, pos, text, byte_len );
    return len;
}
WORKPHONE_API wp_s32 wp_str_insert_str_utf8( struct wp_str *str, wp_s32 pos, const wp_c8 *text )
{
    wp_s32 byte_len = 0;
    wp_s32 num_runes = 0;
    wp_s32 glyph_len = 0;
    wp_rune unicode;
    if( !str || !text )
        return 0;

    glyph_len = byte_len = wp_utf_decode( text + byte_len, &unicode, 4 );
    while( unicode != '\0' && glyph_len )
    {
        glyph_len = wp_utf_decode( text + byte_len, &unicode, 4 );
        byte_len += glyph_len;
        num_runes++;
    }
    wp_str_insert_at_rune( str, pos, text, byte_len );
    return num_runes;
}
WORKPHONE_API wp_s32 wp_str_insert_text_runes( struct wp_str *str, wp_s32 pos, const wp_rune *runes,
                                               wp_s32 len )
{
    wp_s32 i = 0;
    wp_s32 byte_len = 0;
    wp_glyph glyph;

    WORKPHONE_ASSERT( str );
    if( !str || !runes || !len )
        return 0;
    for( i = 0; i < len; ++i )
    {
        byte_len = wp_utf_encode( runes[i], glyph, WORKPHONE_UTF_SIZE );
        if( !byte_len )
            break;
        wp_str_insert_at_rune( str, pos + i, glyph, byte_len );
    }
    return len;
}
WORKPHONE_API wp_s32 wp_str_insert_str_runes( struct wp_str *str, wp_s32 pos, const wp_rune *runes )
{
    wp_s32 i = 0;
    wp_glyph glyph;
    wp_s32 byte_len;
    WORKPHONE_ASSERT( str );
    if( !str || !runes )
        return 0;
    while( runes[i] != '\0' )
    {
        byte_len = wp_utf_encode( runes[i], glyph, WORKPHONE_UTF_SIZE );
        wp_str_insert_at_rune( str, pos + i, glyph, byte_len );
        i++;
    }
    return i;
}

WORKPHONE_API void wp_str_remove_wp_c8s( struct wp_str *s, wp_s32 len )
{
    WORKPHONE_ASSERT( s );
    WORKPHONE_ASSERT( len >= 0 );

    if( !s || len < 0 || (wp_size)len > s->buffer.allocated )
        return;

    WORKPHONE_ASSERT( ( (wp_s32)s->buffer.allocated - (wp_s32)len ) >= 0 );
    s->buffer.allocated -= (wp_size)len;
    s->len = wp_utf_len( (wp_c8 *)s->buffer.memory.ptr, (wp_s32)s->buffer.allocated );
}

WORKPHONE_API void wp_str_remove_runes( struct wp_str *str, wp_s32 len )
{
    wp_s32 index;
    const wp_c8 *begin;
    const wp_c8 *end;
    wp_rune unicode;

    WORKPHONE_ASSERT( str );
    WORKPHONE_ASSERT( len >= 0 );

    if( !str || len < 0 )
        return;

    if( len >= str->len )
    {
        str->len = 0;
        return;
    }

    index = str->len - len;
    begin = wp_str_at_rune( str, index, &unicode, &len );
    end = (const wp_c8 *)str->buffer.memory.ptr + str->buffer.allocated;
    wp_str_remove_wp_c8s( str, (wp_s32)( end - begin ) + 1 );
}
WORKPHONE_API void wp_str_delete_wp_c8s( struct wp_str *s, wp_s32 pos, wp_s32 len )
{
    WORKPHONE_ASSERT( s );
    if( !s || !len || (wp_size)pos > s->buffer.allocated ||
        (wp_size)( pos + len ) > s->buffer.allocated )
        return;

    if( (wp_size)( pos + len ) < s->buffer.allocated )
    {
        /* memmove */
        wp_c8 *dst = wp_ptr_add( wp_c8, s->buffer.memory.ptr, pos );
        wp_c8 *src = wp_ptr_add( wp_c8, s->buffer.memory.ptr, pos + len );
        WORKPHONE_MEMCPY( dst, src, s->buffer.allocated - (wp_size)( pos + len ) );
        WORKPHONE_ASSERT( ( (wp_s32)s->buffer.allocated - (wp_s32)len ) >= 0 );
        s->buffer.allocated -= (wp_size)len;
    }
    else
        wp_str_remove_wp_c8s( s, len );
    s->len = wp_utf_len( (wp_c8 *)s->buffer.memory.ptr, (wp_s32)s->buffer.allocated );
}

WORKPHONE_API void wp_str_delete_runes( struct wp_str *s, wp_s32 pos, wp_s32 len )
{
    wp_c8 *temp;
    wp_rune unicode;
    wp_c8 *begin;
    wp_c8 *end;
    wp_s32 unused;

    WORKPHONE_ASSERT( s );
    WORKPHONE_ASSERT( s->len >= pos + len );

    if( s->len < pos + len )
        len = WORKPHONE_CLAMP( 0, ( s->len - pos ), s->len );

    if( !len )
        return;

    temp = (wp_c8 *)s->buffer.memory.ptr;
    begin = wp_str_at_rune( s, pos, &unicode, &unused );
    if( !begin )
        return;
    s->buffer.memory.ptr = begin;
    end = wp_str_at_rune( s, len, &unicode, &unused );
    s->buffer.memory.ptr = temp;

    if( !end )
        return;

    wp_str_delete_wp_c8s( s, (wp_s32)( begin - temp ), (wp_s32)( end - begin ) );
}

WORKPHONE_API wp_c8 *wp_str_at_wp_c8( struct wp_str *s, wp_s32 pos )
{
    WORKPHONE_ASSERT( s );
    if( !s || pos > (wp_s32)s->buffer.allocated )
        return 0;
    return wp_ptr_add( wp_c8, s->buffer.memory.ptr, pos );
}

WORKPHONE_API wp_c8 *wp_str_at_rune( struct wp_str *str, wp_s32 pos, wp_rune *unicode, wp_s32 *len )
{
    wp_s32 i = 0;
    wp_s32 src_len = 0;
    wp_s32 glyph_len = 0;
    wp_c8 *text;
    wp_s32 text_len;

    WORKPHONE_ASSERT( str );
    WORKPHONE_ASSERT( unicode );
    WORKPHONE_ASSERT( len );

    if( !str || !unicode || !len )
        return 0;
    if( pos < 0 )
    {
        *unicode = 0;
        *len = 0;
        return 0;
    }

    text = (wp_c8 *)str->buffer.memory.ptr;
    text_len = (wp_s32)str->buffer.allocated;
    glyph_len = wp_utf_decode( text, unicode, text_len );
    while( glyph_len )
    {
        if( i == pos )
        {
            *len = glyph_len;
            break;
        }

        i++;
        src_len = src_len + glyph_len;
        glyph_len = wp_utf_decode( text + src_len, unicode, text_len - src_len );
    }
    if( i != pos )
        return 0;
    return text + src_len;
}
WORKPHONE_API const wp_c8 *wp_str_at_wp_c8_const( const struct wp_str *s, wp_s32 pos )
{
    WORKPHONE_ASSERT( s );
    if( !s || pos > (wp_s32)s->buffer.allocated )
        return 0;
    return wp_ptr_add( wp_c8, s->buffer.memory.ptr, pos );
}
WORKPHONE_API const wp_c8 *wp_str_at_const( const struct wp_str *str, wp_s32 pos, wp_rune *unicode,
                                            wp_s32 *len )
{
    wp_s32 i = 0;
    wp_s32 src_len = 0;
    wp_s32 glyph_len = 0;
    wp_c8 *text;
    wp_s32 text_len;

    WORKPHONE_ASSERT( str );
    WORKPHONE_ASSERT( unicode );
    WORKPHONE_ASSERT( len );

    if( !str || !unicode || !len )
        return 0;
    if( pos < 0 )
    {
        *unicode = 0;
        *len = 0;
        return 0;
    }

    text = (wp_c8 *)str->buffer.memory.ptr;
    text_len = (wp_s32)str->buffer.allocated;
    glyph_len = wp_utf_decode( text, unicode, text_len );
    while( glyph_len )
    {
        if( i == pos )
        {
            *len = glyph_len;
            break;
        }

        i++;
        src_len = src_len + glyph_len;
        glyph_len = wp_utf_decode( text + src_len, unicode, text_len - src_len );
    }
    if( i != pos )
        return 0;
    return text + src_len;
}
WORKPHONE_API wp_rune wp_str_rune_at( const struct wp_str *str, wp_s32 pos )
{
    wp_s32 len;
    wp_rune unicode = 0;
    wp_str_at_const( str, pos, &unicode, &len );
    return unicode;
}
WORKPHONE_API wp_c8 *wp_str_get( struct wp_str *s )
{
    WORKPHONE_ASSERT( s );
    if( !s || !s->len || !s->buffer.allocated )
        return 0;
    return (wp_c8 *)s->buffer.memory.ptr;
}
WORKPHONE_API const wp_c8 *wp_str_get_const( const struct wp_str *s )
{
    WORKPHONE_ASSERT( s );
    if( !s || !s->len || !s->buffer.allocated )
        return 0;
    return (const wp_c8 *)s->buffer.memory.ptr;
}
WORKPHONE_API wp_s32 wp_str_len( const struct wp_str *s )
{
    WORKPHONE_ASSERT( s );
    if( !s || !s->len || !s->buffer.allocated )
        return 0;
    return s->len;
}
WORKPHONE_API wp_s32 wp_str_len_wp_c8( const struct wp_str *s )
{
    WORKPHONE_ASSERT( s );
    if( !s || !s->len || !s->buffer.allocated )
        return 0;
    return (wp_s32)s->buffer.allocated;
}
WORKPHONE_API void wp_str_clear( struct wp_str *str )
{
    WORKPHONE_ASSERT( str );
    wp_buffer_clear( &str->buffer );
    str->len = 0;
}
WORKPHONE_API void wp_str_free( struct wp_str *str )
{
    WORKPHONE_ASSERT( str );
    wp_buffer_free( &str->buffer );
    str->len = 0;
}
