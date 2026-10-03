#include "workphone_utf8.h"
#include "workphone_math.h"

WORKPHONE_GLOBAL const wp_byte wp_utfbyte[WORKPHONE_UTF_SIZE + 1] = { 0x80, 0, 0xC0, 0xE0, 0xF0 };
WORKPHONE_GLOBAL const wp_byte wp_utfmask[WORKPHONE_UTF_SIZE + 1] = { 0xC0, 0x80, 0xE0, 0xF0, 0xF8 };
WORKPHONE_GLOBAL const wp_u32 wp_utfmin[WORKPHONE_UTF_SIZE + 1] = { 0, 0, 0x80, 0x800, 0x10000 };
WORKPHONE_GLOBAL const wp_u32 wp_utfmax[WORKPHONE_UTF_SIZE + 1] = { 0x10FFFF, 0x7F, 0x7FF, 0xFFFF,
                                                                    0x10FFFF };

wp_s32 wp_utf_validate( wp_rune *u, wp_s32 i )
{
    WORKPHONE_ASSERT( u );
    if( !u )
        return 0;
    if( !WORKPHONE_BETWEEN( *u, wp_utfmin[i], wp_utfmax[i] ) || WORKPHONE_BETWEEN( *u, 0xD800, 0xDFFF ) )
        *u = WORKPHONE_UTF_INVALID;
    for( i = 1; *u > wp_utfmax[i]; ++i )
        ;
    return i;
}
wp_rune wp_utf_decode_byte( wp_c8 c, wp_s32 *i )
{
    WORKPHONE_ASSERT( i );
    if( !i )
        return 0;
    for( *i = 0; *i < (wp_s32)WORKPHONE_LEN( wp_utfmask ); ++( *i ) )
    {
        if( ( (wp_byte)c & wp_utfmask[*i] ) == wp_utfbyte[*i] )
            return (wp_byte)( c & ~wp_utfmask[*i] );
    }
    return 0;
}

wp_s32 wp_utf_decode( const wp_c8 *c, wp_rune *u, wp_s32 clen )
{
    wp_s32 i, j, len, type = 0;
    wp_rune udecoded;

    WORKPHONE_ASSERT( c );
    WORKPHONE_ASSERT( u );

    if( !c || !u )
        return 0;
    if( !clen )
        return 0;
    *u = WORKPHONE_UTF_INVALID;

    udecoded = wp_utf_decode_byte( c[0], &len );
    if( !WORKPHONE_BETWEEN( len, 1,
                            WORKPHONE_UTF_SIZE + 1 ) ) /* +1 because WORKPHONE_BETWEEN uses strict upper
                                                          bound ((a) <= (x) && (x) < (b)) */
        return 1;

    for( i = 1, j = 1; i < clen && j < len; ++i, ++j )
    {
        udecoded = ( udecoded << 6 ) | wp_utf_decode_byte( c[i], &type );
        if( type != 0 )
            return j;
    }
    if( j < len )
        return 0;
    *u = udecoded;
    wp_utf_validate( u, len );
    return len;
}

wp_c8 wp_utf_encode_byte( wp_rune u, wp_s32 i )
{
    return (wp_c8)( ( wp_utfbyte[i] ) | ( (wp_byte)u & ~wp_utfmask[i] ) );
}

wp_s32 wp_utf_encode( wp_rune u, wp_c8 *c, wp_s32 clen )
{
    wp_s32 len, i;
    len = wp_utf_validate( &u, 0 );
    if( clen < len || !len || len > WORKPHONE_UTF_SIZE )
        return 0;

    for( i = len - 1; i != 0; --i )
    {
        c[i] = wp_utf_encode_byte( u, 0 );
        u >>= 6;
    }
    c[0] = wp_utf_encode_byte( u, len );
    return len;
}

wp_s32 wp_utf_len( const wp_c8 *str, wp_s32 len )
{
    const wp_c8 *text;
    wp_s32 glyphs = 0;
    wp_s32 text_len;
    wp_s32 glyph_len;
    wp_s32 src_len = 0;
    wp_rune unicode;

    WORKPHONE_ASSERT( str );
    if( !str || !len )
        return 0;

    text = str;
    text_len = len;
    glyph_len = wp_utf_decode( text, &unicode, text_len );
    while( glyph_len && src_len < len )
    {
        glyphs++;
        src_len = src_len + glyph_len;
        glyph_len = wp_utf_decode( text + src_len, &unicode, text_len - src_len );
    }
    return glyphs;
}

const wp_c8 *wp_utf_at( const wp_c8 *buffer, wp_s32 length, wp_s32 index, wp_rune *unicode, wp_s32 *len )
{
    wp_s32 i = 0;
    wp_s32 src_len = 0;
    wp_s32 glyph_len = 0;
    const wp_c8 *text;
    wp_s32 text_len;

    WORKPHONE_ASSERT( buffer );
    WORKPHONE_ASSERT( unicode );
    WORKPHONE_ASSERT( len );

    if( !buffer || !unicode || !len )
        return 0;
    if( index < 0 )
    {
        *unicode = WORKPHONE_UTF_INVALID;
        *len = 0;
        return 0;
    }

    text = buffer;
    text_len = length;
    glyph_len = wp_utf_decode( text, unicode, text_len );
    while( glyph_len )
    {
        if( i == index )
        {
            *len = glyph_len;
            break;
        }

        i++;
        src_len = src_len + glyph_len;
        glyph_len = wp_utf_decode( text + src_len, unicode, text_len - src_len );
    }
    if( i != index )
        return 0;
    return buffer + src_len;
}
