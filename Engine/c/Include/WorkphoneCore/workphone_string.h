/**
 * @file wp_string.h
 * @brief C API for dynamic string operations.
 */

#ifndef WORKPHONE_STRING_H
#define WORKPHONE_STRING_H

#include "workphone_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    wp_c8 *data;
    wp_u32 length;
    wp_u32 capacity;
} wp_string;

struct wp_str
{
    struct wp_buffer buffer;
    wp_s32 len; /**!< in codepoints/runes/glyphs */
};

wp_string wp_string_make( const wp_c8 *src );
wp_string wp_string_make_len( const wp_c8 *src, wp_u32 len );
wp_string wp_string_make_empty( void );
wp_string wp_string_copy( wp_string s );
void wp_string_free( wp_string *s );
void wp_string_clear( wp_string *s );
void wp_string_reserve( wp_string *s, wp_u32 capacity );

wp_c8 *wp_strstr( const wp_c8 *s, const wp_c8 *needle );

wp_u32 wp_string_length( const wp_string *s );
wp_s32 wp_string_is_empty( const wp_string *s );

wp_s32 wp_string_equals( const wp_string *a, const wp_string *b );
wp_s32 wp_string_equals_cstr( const wp_string *a, const wp_c8 *b );
wp_s32 wp_string_compare( const wp_string *a, const wp_string *b );

wp_s32 wp_string_find( const wp_string *s, const wp_c8 *needle, wp_u32 from );
wp_s32 wp_string_rfind( const wp_string *s, const wp_c8 *needle );
wp_s32 wp_string_contains( const wp_string *s, const wp_c8 *needle );
wp_s32 wp_string_starts_with( const wp_string *s, const wp_c8 *prefix );
wp_s32 wp_string_ends_with( const wp_string *s, const wp_c8 *suffix );

void wp_string_append( wp_string *s, const wp_c8 *src );
void wp_string_append_len( wp_string *s, const wp_c8 *src, wp_u32 len );
void wp_string_append_wp_c8( wp_string *s, wp_c8 c );
void wp_string_prepend( wp_string *s, const wp_c8 *src );
void wp_string_insert( wp_string *s, wp_u32 pos, const wp_c8 *src );
void wp_string_erase( wp_string *s, wp_u32 pos, wp_u32 count );
void wp_string_replace_first( wp_string *s, const wp_c8 *from, const wp_c8 *to );
void wp_string_replace_all( wp_string *s, const wp_c8 *from, const wp_c8 *to );

void wp_string_to_upper( wp_string *s );
void wp_string_to_lower( wp_string *s );
void wp_string_trim( wp_string *s );
void wp_string_trim_left( wp_string *s );
void wp_string_trim_right( wp_string *s );
wp_string wp_string_substr( const wp_string *s, wp_u32 pos, wp_u32 count );

wp_u32 wp_string_hash( const wp_string *s );

WORKPHONE_API wp_s32 wp_strlen( const wp_c8 *str );
WORKPHONE_API wp_s32 wp_stricmp( const wp_c8 *s1, const wp_c8 *s2 );
WORKPHONE_API wp_s32 wp_stricmpn( const wp_c8 *s1, const wp_c8 *s2, wp_s32 n );
WORKPHONE_API wp_s32 wp_strtoi( const wp_c8 *str, wp_c8 **endptr );
WORKPHONE_API wp_f32 wp_strtof( const wp_c8 *str, wp_c8 **endptr );

#ifdef WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR
WORKPHONE_API void wp_str_init_default( struct wp_str * );
#endif

WORKPHONE_API void wp_str_init( struct wp_str *, const struct wp_allocator *, wp_size size );
WORKPHONE_API void wp_str_init_fixed( struct wp_str *, void *memory, wp_size size );
WORKPHONE_API void wp_str_clear( struct wp_str * );
WORKPHONE_API void wp_str_free( struct wp_str * );

WORKPHONE_API wp_s32 wp_str_append_text_wp_c8( struct wp_str *, const wp_c8 *, wp_s32 );
WORKPHONE_API wp_s32 wp_str_append_str_wp_c8( struct wp_str *, const wp_c8 * );
WORKPHONE_API wp_s32 wp_str_append_text_utf8( struct wp_str *, const wp_c8 *, wp_s32 );
WORKPHONE_API wp_s32 wp_str_append_str_utf8( struct wp_str *, const wp_c8 * );
WORKPHONE_API wp_s32 wp_str_append_text_runes( struct wp_str *, const wp_rune *, wp_s32 );
WORKPHONE_API wp_s32 wp_str_append_str_runes( struct wp_str *, const wp_rune * );

WORKPHONE_API wp_s32 wp_str_insert_at_wp_c8( struct wp_str *, wp_s32 pos, const wp_c8 *, wp_s32 );
WORKPHONE_API wp_s32 wp_str_insert_at_rune( struct wp_str *, wp_s32 pos, const wp_c8 *, wp_s32 );

WORKPHONE_API wp_s32 wp_str_insert_text_wp_c8( struct wp_str *, wp_s32 pos, const wp_c8 *, wp_s32 );
WORKPHONE_API wp_s32 wp_str_insert_str_wp_c8( struct wp_str *, wp_s32 pos, const wp_c8 * );
WORKPHONE_API wp_s32 wp_str_insert_text_utf8( struct wp_str *, wp_s32 pos, const wp_c8 *, wp_s32 );
WORKPHONE_API wp_s32 wp_str_insert_str_utf8( struct wp_str *, wp_s32 pos, const wp_c8 * );
WORKPHONE_API wp_s32 wp_str_insert_text_runes( struct wp_str *, wp_s32 pos, const wp_rune *, wp_s32 );
WORKPHONE_API wp_s32 wp_str_insert_str_runes( struct wp_str *, wp_s32 pos, const wp_rune * );

WORKPHONE_API void wp_str_remove_wp_c8s( struct wp_str *, wp_s32 len );
WORKPHONE_API void wp_str_remove_runes( struct wp_str *str, wp_s32 len );
WORKPHONE_API void wp_str_delete_wp_c8s( struct wp_str *, wp_s32 pos, wp_s32 len );
WORKPHONE_API void wp_str_delete_runes( struct wp_str *, wp_s32 pos, wp_s32 len );

WORKPHONE_API wp_c8 *wp_str_at_wp_c8( struct wp_str *, wp_s32 pos );
WORKPHONE_API wp_c8 *wp_str_at_rune( struct wp_str *, wp_s32 pos, wp_rune *unicode, wp_s32 *len );
WORKPHONE_API wp_rune wp_str_rune_at( const struct wp_str *, wp_s32 pos );
WORKPHONE_API const wp_c8 *wp_str_at_wp_c8_const( const struct wp_str *, wp_s32 pos );
WORKPHONE_API const wp_c8 *wp_str_at_const( const struct wp_str *, wp_s32 pos, wp_rune *unicode,
                                            wp_s32 *len );

WORKPHONE_API wp_c8 *wp_str_get( struct wp_str * );
WORKPHONE_API const wp_c8 *wp_str_get_const( const struct wp_str * );
WORKPHONE_API wp_s32 wp_str_len( const struct wp_str * );
WORKPHONE_API wp_s32 wp_str_len_wp_c8( const struct wp_str * );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_STRING_H */
