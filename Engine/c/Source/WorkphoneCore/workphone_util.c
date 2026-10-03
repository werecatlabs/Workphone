#include "workphone_util.h"
#include "workphone_font.h"
#include "workphone_types.h"
#include "workphone_utf8.h"
#include <stdarg.h>

#ifndef _DEBUG
void *wp_mem_alloc( wp_u32 size )
{
    /* Reject zero allocations */
    if( size == 0 )
        return NULL;

    /* Prevent absurd allocations (DoS protection) */
    if( size > SAFE_MALLOC_MAX )
        return NULL;

    /* Prevent overflow edge cases */
    if( size > SIZE_MAX - 16 )
        return NULL;

    void *p = malloc( size );

    /* Allocation failure check */
    if( p == NULL )
        return NULL;

    return p;
}

void wp_mem_free( void *p )
{
    if( p == NULL )
        return;

    free( p );
}

void *wp_calloc( wp_u32 count, wp_u32 size )
{
    /* Reject zero-sized allocations */
    if( count == 0 || size == 0 )
        return NULL;

    /* Prevent integer overflow */
    if( count > SIZE_MAX / size )
        return NULL;

    wp_u32 total = count * size;

    /* DoS protection */
    if( total > SAFE_CALLOC_MAX )
        return NULL;

    void *p = wp_mem_alloc( total );
    if( !p )
        return NULL;

    /* Zero initialize */
    memset( p, 0, total );

    return p;
}

wp_u32 wp_strnlen( const wp_c8 *s, wp_u32 maxlen )
{
    wp_u32 i = 0;
    while( i < maxlen && s[i] )
        i++;

    return i;
}

wp_c8 *wp_strdup( const wp_c8 *s )
{
    wp_u32 len = (wp_u32)strlen( s ) + 1;
    wp_c8 *p = wp_mem_alloc( len );
    if( p )
        memcpy( p, s, len );

    return p;
}

wp_c8 *wp_strndup( const wp_c8 *s, wp_u32 n )
{
    wp_u32 len = (wp_u32)wp_strnlen( s, n );
    wp_c8 *p = wp_mem_alloc( len + 1 );
    if( !p )
        return 0;

    memcpy( p, s, len );
    p[len] = '\0';
    return p;
}
#else
void *wp_mem_alloc( wp_u32 size )
{
    /* Reject zero allocations */
    if( size == 0 )
        return NULL;

    /* Prevent absurd allocations (DoS protection) */
    if( size > SAFE_MALLOC_MAX )
        return NULL;

    /* Prevent overflow edge cases */
    if( size > SIZE_MAX - 16 )
        return NULL;

    void *p = malloc( size );

    /* Allocation failure check */
    if( p == NULL )
        return NULL;

    return p;
}

void wp_mem_free( void *p )
{
    if( p == NULL )
        return;

    free( p );
}

void *wp_memcpy( void *dest, const void *src, wp_u32 n )
{
    if( dest == NULL || src == NULL )
        return NULL;

    if( dest == src || n == 0 )
        return dest;

    wp_u8 *d = dest;
    const wp_c8 *s = src;

    /* Detect overlap */
    if( ( d < s + n ) && ( s < d + n ) )
        return NULL; /* unsafe for memcpy */

    for( wp_u32 i = 0; i < n; i++ )
        d[i] = s[i];

    return dest;
}

void *wp_calloc( wp_u32 count, wp_u32 size )
{
    /* Reject zero-sized allocations */
    if( count == 0 || size == 0 )
        return NULL;

    /* Prevent integer overflow */
    if( count > SIZE_MAX / size )
        return NULL;

    wp_u32 total = count * size;

    /* DoS protection */
    if( total > SAFE_CALLOC_MAX )
        return NULL;

    void *p = wp_mem_alloc( total );
    if( !p )
        return NULL;

    /* Zero initialize */
    wp_memset( p, 0, total );

    return p;
}

wp_s32 wp_strnlen( const wp_c8 *s, wp_u32 maxlen )
{
    if( s == NULL )
        return 0;

    wp_s32 i = 0;

    while( i < maxlen )
    {
        if( s[i] == '\0' )
            break;

        i++;
    }

    return i;
}

wp_c8 *wp_strdup( const wp_c8 *s )
{
    /* Validate input */
    if( s == NULL )
        return NULL;

    size_t len = 0;

    /* Safe length scan with limit */
    while( len < SAFE_STR_MAX )
    {
        wp_c8 c = s[len];

        if( c == '\0' )
            break;

        len++;
    }

    /* If we hit the limit without finding a terminator,
       treat as suspicious input */
    if( len == SAFE_STR_MAX )
        return NULL;

    /* Overflow protection */
    if( len >= SIZE_MAX - 1 )
        return NULL;

    wp_s32 alloc_size = len + 1;

    wp_c8 *out = (wp_c8 *)wp_mem_alloc( alloc_size );
    if( out == NULL )
        return NULL;

    /* Copy byte-by-byte */
    for( size_t i = 0; i < len; i++ )
        out[i] = s[i];

    /* Ensure null termination */
    out[len] = '\0';

    return out;
}

wp_c8 *wp_strndup( const wp_c8 *s, wp_u32 n )
{
    /* Prevent absurd limits */
    if( n > SAFE_STR_MAX )
        n = SAFE_STR_MAX;

    /* Check for NULL input */
    if( s == NULL )
        return NULL;

    /* If n is zero return empty string */
    if( n == 0 )
    {
        wp_c8 *empty = (wp_c8 *)wp_mem_alloc( 1 );
        if( !empty )
            return NULL;
        empty[0] = '\0';
        return empty;
    }

    /* Find length up to n safely */
    wp_u32 len = 0;
    while( len < n && s[len] != '\0' )
        len++;

    /* Prevent overflow in allocation */
    if( len >= WORKPHONE_UINT_MAX )
        return NULL;

    /* Allocate memory */
    wp_c8 *out = (wp_c8 *)wp_mem_alloc( len + 1 );
    if( !out )
        return NULL;

    /* Copy byte-by-byte (avoids memcpy surprises) */
    for( wp_s32 i = 0; i < (wp_s32)len; i++ )
    {
        out[i] = s[i];
    }

    /* Null terminate */
    out[len] = '\0';

    return out;
}
#endif

wp_bool wp_is_lower( wp_s32 c )
{
    return ( c >= 'a' && c <= 'z' ) || ( c >= 0xE0 && c <= 0xFF );
}
wp_bool wp_is_upper( wp_s32 c )
{
    return ( c >= 'A' && c <= 'Z' ) || ( c >= 0xC0 && c <= 0xDF );
}

wp_s32 wp_to_upper( wp_s32 c )
{
    return ( c >= 'a' && c <= 'z' ) ? ( c - ( 'a' - 'A' ) ) : c;
}

wp_s32 wp_to_lower( wp_s32 c )
{
    return ( c >= 'A' && c <= 'Z' ) ? ( c - ( 'a' + 'A' ) ) : c;
}

void *wp_memcopy( void *dst0, const void *src0, wp_size length )
{
    wp_ptr t;
    wp_c8 *dst = (wp_c8 *)dst0;
    const wp_c8 *src = (const wp_c8 *)src0;
    if( length == 0 || dst == src )
        goto done;

#define wp_word int
#define wp_wsize sizeof( wp_word )
#define wp_wmask ( wp_wsize - 1 )
#define WORKPHONE_TLOOP( s ) \
    if( t ) \
    WORKPHONE_TLOOP1( s )
#define WORKPHONE_TLOOP1( s ) \
    do \
    { \
        s; \
    } while( --t )

    if( dst < src )
    {
        t = (wp_ptr)src; /* only need low bits */
        if( ( t | (wp_ptr)dst ) & wp_wmask )
        {
            if( ( t ^ (wp_ptr)dst ) & wp_wmask || length < wp_wsize )
                t = length;
            else
                t = wp_wsize - ( t & wp_wmask );
            length -= t;
            WORKPHONE_TLOOP1( *dst++ = *src++ );
        }
        t = length / wp_wsize;
        WORKPHONE_TLOOP( *(wp_word *)(void *)dst = *(const wp_word *)(const void *)src; src += wp_wsize;
                         dst += wp_wsize );
        t = length & wp_wmask;
        WORKPHONE_TLOOP( *dst++ = *src++ );
    }
    else
    {
        src += length;
        dst += length;
        t = (wp_ptr)src;
        if( ( t | (wp_ptr)dst ) & wp_wmask )
        {
            if( ( t ^ (wp_ptr)dst ) & wp_wmask || length <= wp_wsize )
                t = length;
            else
                t &= wp_wmask;
            length -= t;
            WORKPHONE_TLOOP1( *--dst = *--src );
        }
        t = length / wp_wsize;
        WORKPHONE_TLOOP( src -= wp_wsize; dst -= wp_wsize;
                         *(wp_word *)(void *)dst = *(const wp_word *)(const void *)src );
        t = length & wp_wmask;
        WORKPHONE_TLOOP( *--dst = *--src );
    }
#undef wp_word
#undef wp_wsize
#undef wp_wmask
#undef WORKPHONE_TLOOP
#undef WORKPHONE_TLOOP1
done:
    return ( dst0 );
}

void *wp_memset( void *ptr, wp_s32 c0, wp_size size )
{
#define wp_word unsigned
#define wp_wsize sizeof( wp_word )
#define wp_wmask ( wp_wsize - 1 )
    wp_byte *dst = (wp_byte *)ptr;
    unsigned c = 0;
    wp_size t = 0;

    if( !ptr )
        return ptr;

    if( ( c = (wp_byte)c0 ) != 0 )
    {
        c = ( c << 8 ) | c; /* at least 16-bits  */
        if( sizeof( wp_u32 ) > 2 )
            c = ( c << 16 ) | c; /* at least 32-bits*/
    }

    /* too small of a word count */
    dst = (wp_byte *)ptr;
    if( size < 3 * wp_wsize )
    {
        while( size-- )
            *dst++ = (wp_byte)c0;
        return ptr;
    }

    /* align destination */
    if( ( t = WORKPHONE_PTR_TO_UINT( dst ) & wp_wmask ) != 0 )
    {
        t = wp_wsize - t;
        size -= t;
        do
        {
            *dst++ = (wp_byte)c0;
        } while( --t != 0 );
    }

    /* fill word */
    t = size / wp_wsize;
    if( t != 0 )
    {
        do
        {
            *(wp_word *)( (void *)dst ) = c;
            dst += wp_wsize;
        } while( --t != 0 );
    }

    /* fill trailing bytes */
    t = ( size & wp_wmask );
    if( t != 0 )
    {
        do
        {
            *dst++ = (wp_byte)c0;
        } while( --t != 0 );
    }

#undef wp_word
#undef wp_wsize
#undef wp_wmask
    return ptr;
}

void wp_zero( void *ptr, wp_size size )
{
    WORKPHONE_ASSERT( ptr );
    WORKPHONE_MEMSET( ptr, 0, size );
}

wp_u32 wp_strlen( const wp_c8 *str )
{
    WORKPHONE_ASSERT( str );

    wp_u32 count = 0;
    while( str[count] != '\0' )
        count++;
    return count;
}

wp_u32 wp_strlen_s( const wp_c8 *str, wp_u32 maxlen )
{
    WORKPHONE_ASSERT( str );

    wp_u32 count = 0;
    while( str[count++] != '\0' && count < maxlen )
        ;
    return count;
}

wp_s32 wp_strtoi( const wp_c8 *str, wp_c8 **endptr )
{
    wp_s32 neg = 1;
    const wp_c8 *p = str;
    wp_s32 value = 0;

    WORKPHONE_ASSERT( str );
    if( !str )
        return 0;

    /* skip whitespace */
    while( *p == ' ' )
        p++;
    if( *p == '-' )
    {
        neg = -1;
        p++;
    }
    while( *p && *p >= '0' && *p <= '9' )
    {
        value = value * 10 + (wp_s32)( *p - '0' );
        p++;
    }
    if( endptr )
        *endptr = (wp_c8 *)p;
    return neg * value;
}

wp_f64 wp_strtod( const wp_c8 *str, wp_c8 **endptr )
{
    wp_f64 m;
    wp_f64 neg = 1.0;
    wp_c8 *p = (wp_c8 *)str;
    wp_f64 value = 0;
    wp_f64 number = 0;

    WORKPHONE_ASSERT( str );
    if( !str )
        return 0;

    /* skip whitespace */
    while( *p == ' ' )
        p++;
    if( *p == '-' )
    {
        neg = -1.0;
        p++;
    }

    while( *p && *p != '.' && *p != 'e' )
    {
        value = value * 10.0 + (wp_f64)( *p - '0' );
        p++;
    }

    if( *p == '.' )
    {
        p++;
        for( m = 0.1; *p && *p != 'e'; p++ )
        {
            value = value + (wp_f64)( *p - '0' ) * m;
            m *= 0.1;
        }
    }
    if( *p == 'e' )
    {
        wp_s32 i, pow, div;
        p++;
        if( *p == '-' )
        {
            div = wp_true;
            p++;
        }
        else if( *p == '+' )
        {
            div = wp_false;
            p++;
        }
        else
            div = wp_false;

        for( pow = 0; *p; p++ )
            pow = pow * 10 + (wp_s32)( *p - '0' );

        for( m = 1.0, i = 0; i < pow; i++ )
            m *= 10.0;

        if( div )
            value /= m;
        else
            value *= m;
    }
    number = value * neg;
    if( endptr )
        *endptr = (wp_c8 *)p;
    return number;
}

wp_f32 wp_strtof( const wp_c8 *str, wp_c8 **endptr )
{
    wp_f32 wp_f32_value;
    wp_f64 wp_f64_value;
    wp_f64_value = WORKPHONE_STRTOD( str, endptr );
    wp_f32_value = (wp_f32)wp_f64_value;
    return wp_f32_value;
}

wp_s32 wp_stricmp( const wp_c8 *s1, const wp_c8 *s2 )
{
    wp_s32 c1, c2, d;
    do
    {
        c1 = *s1++;
        c2 = *s2++;
        d = c1 - c2;
        while( d )
        {
            if( c1 <= 'Z' && c1 >= 'A' )
            {
                d += ( 'a' - 'A' );
                if( !d )
                    break;
            }
            if( c2 <= 'Z' && c2 >= 'A' )
            {
                d -= ( 'a' - 'A' );
                if( !d )
                    break;
            }
            return ( ( d >= 0 ) << 1 ) - 1;
        }
    } while( c1 );
    return 0;
}

wp_s32 wp_stricmpn( const wp_c8 *s1, const wp_c8 *s2, wp_s32 n )
{
    wp_s32 c1, c2, d;
    WORKPHONE_ASSERT( n >= 0 );
    do
    {
        c1 = *s1++;
        c2 = *s2++;
        if( !n-- )
            return 0;

        d = c1 - c2;
        while( d )
        {
            if( c1 <= 'Z' && c1 >= 'A' )
            {
                d += ( 'a' - 'A' );
                if( !d )
                    break;
            }
            if( c2 <= 'Z' && c2 >= 'A' )
            {
                d -= ( 'a' - 'A' );
                if( !d )
                    break;
            }
            return ( ( d >= 0 ) << 1 ) - 1;
        }
    } while( c1 );
    return 0;
}

wp_s32 wp_str_match_here( const wp_c8 *regexp, const wp_c8 *text )
{
    if( regexp[0] == '\0' )
        return 1;
    if( regexp[1] == '*' )
        return wp_str_match_star( regexp[0], regexp + 2, text );
    if( regexp[0] == '$' && regexp[1] == '\0' )
        return *text == '\0';
    if( *text != '\0' && ( regexp[0] == '.' || regexp[0] == *text ) )
        return wp_str_match_here( regexp + 1, text + 1 );
    return 0;
}

wp_s32 wp_str_match_star( wp_s32 c, const wp_c8 *regexp, const wp_c8 *text )
{
    do
    { /* a '* matches zero or more instances */
        if( wp_str_match_here( regexp, text ) )
            return 1;
    } while( *text != '\0' && ( *text++ == c || c == '.' ) );
    return 0;
}

wp_s32 wp_strfilter( const wp_c8 *text, const wp_c8 *regexp )
{
    /*
    c    matches any literal wp_c8acter c
    .    matches any single wp_c8acter
    ^    matches the beginning of the input string
    $    matches the end of the input string
    *    matches zero or more occurrences of the previous wp_c8acter*/
    if( regexp[0] == '^' )
        return wp_str_match_here( regexp + 1, text );
    do
    { /* must look even if string is empty */
        if( wp_str_match_here( regexp, text ) )
            return 1;
    } while( *text++ != '\0' );
    return 0;
}

wp_s32 wp_strmatch_fuzzy_text( const wp_c8 *str, wp_s32 str_len, const wp_c8 *pattern,
                               wp_s32 *out_score )
{
/* Returns true if each wp_c8acter in pattern is found sequentially within str
 * if found then out_score is also set. Score value has no intrinsic meaning.
 * Range varies with pattern. Can only compare scores with same search pattern. */

/* bonus for adjacent matches */
#define WORKPHONE_ADJACENCY_BONUS 5
/* bonus if match occurs after a separator */
#define WORKPHONE_SEPARATOR_BONUS 10
/* bonus if match is uppercase and prev is lower */
#define WORKPHONE_CAMEL_BONUS 10
/* penalty applied for every letter in str before the first match */
#define WORKPHONE_LEADING_LETTER_PENALTY ( -3 )
/* maximum penalty for leading letters */
#define WORKPHONE_MAX_LEADING_LETTER_PENALTY ( -9 )
/* penalty for every letter that doesn't matter */
#define WORKPHONE_UNMATCHED_LETTER_PENALTY ( -1 )

    /* loop variables */
    wp_s32 score = 0;
    wp_c8 const *pattern_iter = pattern;
    wp_s32 str_iter = 0;
    wp_s32 prev_matched = wp_false;
    wp_s32 prev_lower = wp_false;
    /* true so if first letter match gets separator bonus*/
    wp_s32 prev_separator = wp_true;

    /* use "best" matched letter if multiple string letters match the pattern */
    wp_c8 const *best_letter = 0;
    wp_s32 best_letter_score = 0;

    /* loop over strings */
    WORKPHONE_ASSERT( str );
    WORKPHONE_ASSERT( pattern );
    if( !str || !str_len || !pattern )
        return 0;
    while( str_iter < str_len )
    {
        const wp_c8 pattern_letter = *pattern_iter;
        const wp_c8 str_letter = str[str_iter];

        wp_s32 next_match =
            *pattern_iter != '\0' && wp_to_lower( pattern_letter ) == wp_to_lower( str_letter );
        wp_s32 rematch = best_letter && wp_to_upper( *best_letter ) == wp_to_upper( str_letter );

        wp_s32 advanced = next_match && best_letter;
        wp_s32 pattern_repeat = best_letter && *pattern_iter != '\0';
        pattern_repeat = pattern_repeat && wp_to_lower( *best_letter ) == wp_to_lower( pattern_letter );

        if( advanced || pattern_repeat )
        {
            score += best_letter_score;
            best_letter = 0;
            best_letter_score = 0;
        }

        if( next_match || rematch )
        {
            wp_s32 new_score = 0;
            /* Apply penalty for each letter before the first pattern match */
            if( pattern_iter == pattern )
            {
                wp_s32 count = (wp_s32)( &str[str_iter] - str );
                wp_s32 penalty = WORKPHONE_LEADING_LETTER_PENALTY * count;
                if( penalty < WORKPHONE_MAX_LEADING_LETTER_PENALTY )
                    penalty = WORKPHONE_MAX_LEADING_LETTER_PENALTY;

                score += penalty;
            }

            /* apply bonus for consecutive bonuses */
            if( prev_matched )
                new_score += WORKPHONE_ADJACENCY_BONUS;

            /* apply bonus for matches after a separator */
            if( prev_separator )
                new_score += WORKPHONE_SEPARATOR_BONUS;

            /* apply bonus across camel case boundaries */
            if( prev_lower && wp_is_upper( str_letter ) )
                new_score += WORKPHONE_CAMEL_BONUS;

            /* update pattern iter IFF the next pattern letter was matched */
            if( next_match )
                ++pattern_iter;

            /* update best letter in str which may be for a "next" letter or a rematch */
            if( new_score >= best_letter_score )
            {
                /* apply penalty for now skipped letter */
                if( best_letter != 0 )
                    score += WORKPHONE_UNMATCHED_LETTER_PENALTY;

                best_letter = &str[str_iter];
                best_letter_score = new_score;
            }
            prev_matched = wp_true;
        }
        else
        {
            score += WORKPHONE_UNMATCHED_LETTER_PENALTY;
            prev_matched = wp_false;
        }

        /* separators should be more easily defined */
        prev_lower = wp_is_lower( str_letter ) != 0;
        prev_separator = str_letter == '_' || str_letter == ' ';

        ++str_iter;
    }

    /* apply score for last match */
    if( best_letter )
        score += best_letter_score;

    /* did not match full pattern */
    if( *pattern_iter != '\0' )
        return wp_false;

    if( out_score )
        *out_score = score;
    return wp_true;
}

wp_s32 wp_strmatch_fuzzy_string( wp_c8 const *str, wp_c8 const *pattern, wp_s32 *out_score )
{
    return wp_strmatch_fuzzy_text( str, wp_strlen( str ), pattern, out_score );
}

wp_s32 wp_string_wp_f32_limit( wp_c8 *string, wp_s32 prec )
{
    wp_s32 dot = 0;
    wp_c8 *c = string;
    while( *c )
    {
        if( *c == '.' )
        {
            dot = 1;
            c++;
            continue;
        }
        if( dot == ( prec + 1 ) )
        {
            *c = 0;
            break;
        }
        if( dot > 0 )
            dot++;
        c++;
    }
    return (wp_s32)( c - string );
}

void wp_strrev_ascii( wp_c8 *s )
{
    wp_s32 len = wp_strlen( s );
    wp_s32 end = len / 2;
    wp_s32 i = 0;
    wp_c8 t;
    for( ; i < end; ++i )
    {
        t = s[i];
        s[i] = s[len - 1 - i];
        s[len - 1 - i] = t;
    }
}

wp_c8 *wp_itoa( wp_c8 *s, long n )
{
    long i = 0;
    if( n == 0 )
    {
        s[i++] = '0';
        s[i] = 0;
        return s;
    }
    if( n < 0 )
    {
        s[i++] = '-';
        n = -n;
    }
    while( n > 0 )
    {
        s[i++] = (wp_c8)( '0' + ( n % 10 ) );
        n /= 10;
    }
    s[i] = 0;
    if( s[0] == '-' )
        ++s;

    wp_strrev_ascii( s );
    return s;
}

wp_c8 *wp_dtoa( wp_c8 *s, wp_f64 n )
{
    wp_s32 useExp = 0;
    wp_s32 digit = 0, m = 0, m1 = 0;
    wp_c8 *c = s;
    wp_s32 neg = 0;

    WORKPHONE_ASSERT( s );
    if( !s )
        return 0;

    if( n == 0.0 )
    {
        s[0] = '0';
        s[1] = '\0';
        return s;
    }

    neg = ( n < 0 );
    if( neg )
        n = -n;

    /* calculate magnitude */
    m = wp_log10( n );
    useExp = ( m >= 14 || ( neg && m >= 9 ) || m <= -9 );
    if( neg )
        *( c++ ) = '-';

    /* set up for scientific notation */
    if( useExp )
    {
        if( m < 0 )
            m -= 1;
        n = n / (wp_f64)wp_pow( 10.0, m );
        m1 = m;
        m = 0;
    }
    if( m < 1.0 )
    {
        m = 0;
    }

    /* convert the number */
    while( n > WORKPHONE_FLOAT_PRECISION || m >= 0 )
    {
        wp_f64 weight = wp_pow( 10.0, m );
        if( weight > 0 )
        {
            wp_f64 t = (wp_f64)n / weight;
            digit = wp_ifloord( t );
            n -= ( (wp_f64)digit * weight );
            *( c++ ) = (wp_c8)( '0' + (wp_c8)digit );
        }
        if( m == 0 && n > 0 )
            *( c++ ) = '.';
        m--;
    }

    if( useExp )
    {
        /* convert the exponent */
        wp_s32 i, j;
        *( c++ ) = 'e';
        if( m1 > 0 )
        {
            *( c++ ) = '+';
        }
        else
        {
            *( c++ ) = '-';
            m1 = -m1;
        }
        m = 0;
        while( m1 > 0 )
        {
            *( c++ ) = (wp_c8)( '0' + (wp_c8)( m1 % 10 ) );
            m1 /= 10;
            m++;
        }
        c -= m;
        for( i = 0, j = m - 1; i < j; i++, j-- )
        {
            /* swap without temporary */
            c[i] ^= c[j];
            c[j] ^= c[i];
            c[i] ^= c[j];
        }
        c += m;
    }
    *( c ) = '\0';
    return s;
}

wp_s32 wp_vsnprintf( wp_c8 *buf, wp_s32 buf_size, const wp_c8 *fmt, va_list args )
{
    enum wp_arg_type
    {
        WORKPHONE_ARG_TYPE_CHAR,
        WORKPHONE_ARG_TYPE_SHORT,
        WORKPHONE_ARG_TYPE_DEFAULT,
        WORKPHONE_ARG_TYPE_LONG
    };
    enum wp_arg_flags
    {
        WORKPHONE_ARG_FLAG_LEFT = 0x01,
        WORKPHONE_ARG_FLAG_PLUS = 0x02,
        WORKPHONE_ARG_FLAG_SPACE = 0x04,
        WORKPHONE_ARG_FLAG_NUM = 0x10,
        WORKPHONE_ARG_FLAG_ZERO = 0x20
    };

    wp_c8 number_buffer[WORKPHONE_MAX_NUMBER_BUFFER];
    enum wp_arg_type arg_type = WORKPHONE_ARG_TYPE_DEFAULT;
    wp_s32 precision = WORKPHONE_DEFAULT;
    wp_s32 width = WORKPHONE_DEFAULT;
    wp_flags flag = 0;

    wp_s32 len = 0;
    wp_s32 result = -1;
    const wp_c8 *iter = fmt;

    WORKPHONE_ASSERT( buf );
    WORKPHONE_ASSERT( buf_size );
    if( !buf || !buf_size || !fmt )
        return 0;
    for( iter = fmt; *iter && len < buf_size; iter++ )
    {
        /* copy all non-format wp_c8acters */
        while( *iter && ( *iter != '%' ) && ( len < buf_size ) )
            buf[len++] = *iter++;
        if( !( *iter ) || len >= buf_size )
            break;
        iter++;

        /* flag arguments */
        while( *iter )
        {
            if( *iter == '-' )
                flag |= WORKPHONE_ARG_FLAG_LEFT;
            else if( *iter == '+' )
                flag |= WORKPHONE_ARG_FLAG_PLUS;
            else if( *iter == ' ' )
                flag |= WORKPHONE_ARG_FLAG_SPACE;
            else if( *iter == '#' )
                flag |= WORKPHONE_ARG_FLAG_NUM;
            else if( *iter == '0' )
                flag |= WORKPHONE_ARG_FLAG_ZERO;
            else
                break;
            iter++;
        }

        /* width argument */
        width = WORKPHONE_DEFAULT;
        if( *iter >= '1' && *iter <= '9' )
        {
            wp_c8 *end;
            width = wp_strtoi( iter, &end );
            if( end == iter )
                width = -1;
            else
                iter = end;
        }
        else if( *iter == '*' )
        {
            width = va_arg( args, wp_s32 );
            iter++;
        }

        /* precision argument */
        precision = WORKPHONE_DEFAULT;
        if( *iter == '.' )
        {
            iter++;
            if( *iter == '*' )
            {
                precision = va_arg( args, wp_s32 );
                iter++;
            }
            else
            {
                wp_c8 *end;
                precision = wp_strtoi( iter, &end );
                if( end == iter )
                    precision = -1;
                else
                    iter = end;
            }
        }

        /* length modifier */
        if( *iter == 'h' )
        {
            if( *( iter + 1 ) == 'h' )
            {
                arg_type = WORKPHONE_ARG_TYPE_CHAR;
                iter++;
            }
            else
                arg_type = WORKPHONE_ARG_TYPE_SHORT;
            iter++;
        }
        else if( *iter == 'l' )
        {
            arg_type = WORKPHONE_ARG_TYPE_LONG;
            iter++;
        }
        else
            arg_type = WORKPHONE_ARG_TYPE_DEFAULT;

        /* specifier */
        if( *iter == '%' )
        {
            WORKPHONE_ASSERT( arg_type == WORKPHONE_ARG_TYPE_DEFAULT );
            WORKPHONE_ASSERT( precision == WORKPHONE_DEFAULT );
            WORKPHONE_ASSERT( width == WORKPHONE_DEFAULT );
            if( len < buf_size )
                buf[len++] = '%';
        }
        else if( *iter == 's' )
        {
            /* string  */
            const wp_c8 *str = va_arg( args, wp_c8 * );
            WORKPHONE_ASSERT( str != buf && "buffer and argument are not allowed to overlap!" );
            WORKPHONE_ASSERT( arg_type == WORKPHONE_ARG_TYPE_DEFAULT );
            WORKPHONE_ASSERT( precision == WORKPHONE_DEFAULT );
            WORKPHONE_ASSERT( width == WORKPHONE_DEFAULT );
            if( str == buf )
                return -1;
            while( str && *str && len < buf_size )
                buf[len++] = *str++;
        }
        else if( *iter == 'n' )
        {
            /* current length callback */
            wp_s32 *n = va_arg( args, wp_s32 * );
            WORKPHONE_ASSERT( arg_type == WORKPHONE_ARG_TYPE_DEFAULT );
            WORKPHONE_ASSERT( precision == WORKPHONE_DEFAULT );
            WORKPHONE_ASSERT( width == WORKPHONE_DEFAULT );
            if( n )
                *n = len;
        }
        else if( *iter == 'c' || *iter == 'i' || *iter == 'd' )
        {
            /* signed integer */
            long value = 0;
            const wp_c8 *num_iter;
            wp_s32 num_len, num_print, padding;
            wp_s32 cur_precision = WORKPHONE_MAX( precision, 1 );
            wp_s32 cur_width = WORKPHONE_MAX( width, 0 );

            /* retrieve correct value type */
            if( arg_type == WORKPHONE_ARG_TYPE_CHAR )
                value = (wp_c8)va_arg( args, wp_s32 );
            else if( arg_type == WORKPHONE_ARG_TYPE_SHORT )
                value = (signed short)va_arg( args, wp_s32 );
            else if( arg_type == WORKPHONE_ARG_TYPE_LONG )
                value = va_arg( args, signed long );
            else if( *iter == 'c' )
                value = (wp_u8)va_arg( args, wp_s32 );
            else
                value = va_arg( args, wp_s32 );

            /* convert number to string */
            wp_itoa( number_buffer, value );
            num_len = wp_strlen( number_buffer );
            padding = WORKPHONE_MAX( cur_width - WORKPHONE_MAX( cur_precision, num_len ), 0 );
            if( ( flag & WORKPHONE_ARG_FLAG_PLUS ) || ( flag & WORKPHONE_ARG_FLAG_SPACE ) )
                padding = WORKPHONE_MAX( padding - 1, 0 );

            /* fill left padding up to a total of `width` wp_c8acters */
            if( !( flag & WORKPHONE_ARG_FLAG_LEFT ) )
            {
                while( padding-- > 0 && ( len < buf_size ) )
                {
                    if( ( flag & WORKPHONE_ARG_FLAG_ZERO ) && ( precision == WORKPHONE_DEFAULT ) )
                        buf[len++] = '0';
                    else
                        buf[len++] = ' ';
                }
            }

            /* copy string value representation into buffer */
            if( ( flag & WORKPHONE_ARG_FLAG_PLUS ) && value >= 0 && len < buf_size )
                buf[len++] = '+';
            else if( ( flag & WORKPHONE_ARG_FLAG_SPACE ) && value >= 0 && len < buf_size )
                buf[len++] = ' ';

            /* fill up to precision number of digits with '0' */
            num_print = WORKPHONE_MAX( cur_precision, num_len );
            while( precision && ( num_print > num_len ) && ( len < buf_size ) )
            {
                buf[len++] = '0';
                num_print--;
            }

            /* copy string value representation into buffer */
            num_iter = number_buffer;
            while( precision && *num_iter && len < buf_size )
                buf[len++] = *num_iter++;

            /* fill right padding up to width wp_c8acters */
            if( flag & WORKPHONE_ARG_FLAG_LEFT )
            {
                while( ( padding-- > 0 ) && ( len < buf_size ) )
                    buf[len++] = ' ';
            }
        }
        else if( *iter == 'o' || *iter == 'x' || *iter == 'X' || *iter == 'u' )
        {
            /* unsigned integer */
            unsigned long value = 0;
            wp_s32 num_len = 0, num_print, padding = 0;
            wp_s32 cur_precision = WORKPHONE_MAX( precision, 1 );
            wp_s32 cur_width = WORKPHONE_MAX( width, 0 );
            wp_u32 base = ( *iter == 'o' ) ? 8 : ( *iter == 'u' ) ? 10 : 16;

            /* prwp_s32 oct/hex/dec value */
            const wp_c8 *upper_output_format = "0123456789ABCDEF";
            const wp_c8 *lower_output_format = "0123456789abcdef";
            const wp_c8 *output_format = ( *iter == 'x' ) ? lower_output_format : upper_output_format;

            /* retrieve correct value type */
            if( arg_type == WORKPHONE_ARG_TYPE_CHAR )
                value = (wp_u8)va_arg( args, wp_s32 );
            else if( arg_type == WORKPHONE_ARG_TYPE_SHORT )
                value = (unsigned short)va_arg( args, wp_s32 );
            else if( arg_type == WORKPHONE_ARG_TYPE_LONG )
                value = va_arg( args, unsigned long );
            else
                value = va_arg( args, wp_u32 );

            do
            {
                /* convert decimal number into hex/oct number */
                wp_s32 digit = output_format[value % base];
                if( num_len < WORKPHONE_MAX_NUMBER_BUFFER )
                    number_buffer[num_len++] = (wp_c8)digit;
                value /= base;
            } while( value > 0 );

            num_print = WORKPHONE_MAX( cur_precision, num_len );
            padding = WORKPHONE_MAX( cur_width - WORKPHONE_MAX( cur_precision, num_len ), 0 );
            if( flag & WORKPHONE_ARG_FLAG_NUM )
                padding = WORKPHONE_MAX( padding - 1, 0 );

            /* fill left padding up to a total of `width` wp_c8acters */
            if( !( flag & WORKPHONE_ARG_FLAG_LEFT ) )
            {
                while( ( padding-- > 0 ) && ( len < buf_size ) )
                {
                    if( ( flag & WORKPHONE_ARG_FLAG_ZERO ) && ( precision == WORKPHONE_DEFAULT ) )
                        buf[len++] = '0';
                    else
                        buf[len++] = ' ';
                }
            }

            /* fill up to precision number of digits */
            if( num_print && ( flag & WORKPHONE_ARG_FLAG_NUM ) )
            {
                if( ( *iter == 'o' ) && ( len < buf_size ) )
                {
                    buf[len++] = '0';
                }
                else if( ( *iter == 'x' ) && ( ( len + 1 ) < buf_size ) )
                {
                    buf[len++] = '0';
                    buf[len++] = 'x';
                }
                else if( ( *iter == 'X' ) && ( ( len + 1 ) < buf_size ) )
                {
                    buf[len++] = '0';
                    buf[len++] = 'X';
                }
            }
            while( precision && ( num_print > num_len ) && ( len < buf_size ) )
            {
                buf[len++] = '0';
                num_print--;
            }

            /* reverse number direction */
            while( num_len > 0 )
            {
                if( precision && ( len < buf_size ) )
                    buf[len++] = number_buffer[num_len - 1];
                num_len--;
            }

            /* fill right padding up to width wp_c8acters */
            if( flag & WORKPHONE_ARG_FLAG_LEFT )
            {
                while( ( padding-- > 0 ) && ( len < buf_size ) )
                    buf[len++] = ' ';
            }
        }
        else if( *iter == 'f' )
        {
            /* wp_f32ing powp_s32 */
            const wp_c8 *num_iter;
            wp_s32 cur_precision = ( precision < 0 ) ? 6 : precision;
            wp_s32 prefix, cur_width = WORKPHONE_MAX( width, 0 );
            wp_f64 value = va_arg( args, wp_f64 );
            wp_s32 num_len = 0, frac_len = 0, dot = 0;
            wp_s32 padding = 0;

            WORKPHONE_ASSERT( arg_type == WORKPHONE_ARG_TYPE_DEFAULT );
            WORKPHONE_DTOA( number_buffer, value );
            num_len = wp_strlen( number_buffer );

            /* calculate padding */
            num_iter = number_buffer;
            while( *num_iter && *num_iter != '.' )
                num_iter++;

            prefix = ( *num_iter == '.' ) ? (wp_s32)( num_iter - number_buffer ) + 1 : 0;
            padding = WORKPHONE_MAX(
                cur_width - ( prefix + WORKPHONE_MIN( cur_precision, num_len - prefix ) ), 0 );
            if( ( flag & WORKPHONE_ARG_FLAG_PLUS ) || ( flag & WORKPHONE_ARG_FLAG_SPACE ) )
                padding = WORKPHONE_MAX( padding - 1, 0 );

            /* fill left padding up to a total of `width` wp_c8acters */
            if( !( flag & WORKPHONE_ARG_FLAG_LEFT ) )
            {
                while( padding-- > 0 && ( len < buf_size ) )
                {
                    if( flag & WORKPHONE_ARG_FLAG_ZERO )
                        buf[len++] = '0';
                    else
                        buf[len++] = ' ';
                }
            }

            /* copy string value representation into buffer */
            num_iter = number_buffer;
            if( ( flag & WORKPHONE_ARG_FLAG_PLUS ) && ( value >= 0 ) && ( len < buf_size ) )
                buf[len++] = '+';
            else if( ( flag & WORKPHONE_ARG_FLAG_SPACE ) && ( value >= 0 ) && ( len < buf_size ) )
                buf[len++] = ' ';
            while( *num_iter )
            {
                if( dot )
                    frac_len++;
                if( len < buf_size )
                    buf[len++] = *num_iter;
                if( *num_iter == '.' )
                    dot = 1;
                if( frac_len >= cur_precision )
                    break;
                num_iter++;
            }

            /* fill number up to precision */
            while( frac_len < cur_precision )
            {
                if( !dot && len < buf_size )
                {
                    buf[len++] = '.';
                    dot = 1;
                }
                if( len < buf_size )
                    buf[len++] = '0';
                frac_len++;
            }

            /* fill right padding up to width wp_c8acters */
            if( flag & WORKPHONE_ARG_FLAG_LEFT )
            {
                while( ( padding-- > 0 ) && ( len < buf_size ) )
                    buf[len++] = ' ';
            }
        }
        else
        {
            /* Specifier not supported: g,G,e,E,p,z */
            WORKPHONE_ASSERT( 0 && "specifier is not supported!" );
            return result;
        }
    }
    buf[( len >= buf_size ) ? ( buf_size - 1 ) : len] = 0;
    result = ( len >= buf_size ) ? -1 : len;
    return result;
}
wp_s32 wp_strfmt( wp_c8 *buf, wp_s32 buf_size, const wp_c8 *fmt, va_list args )
{
    wp_s32 result = -1;
    WORKPHONE_ASSERT( buf );
    WORKPHONE_ASSERT( buf_size );
    if( !buf || !buf_size || !fmt )
        return 0;
#ifdef WORKPHONE_INCLUDE_STANDARD_IO
    result = WORKPHONE_VSNPRINTF( buf, (wp_size)buf_size, fmt, args );
    result = ( result >= buf_size ) ? -1 : result;
    buf[buf_size - 1] = 0;
#else
    result = wp_vsnprintf( buf, buf_size, fmt, args );
#endif
    return result;
}
wp_hash wp_murmur_hash( const void *key, wp_s32 len, wp_hash seed )
{
/* 32-Bit MurmurHash3: https://code.google.com/p/smhasher/wiki/MurmurHash3*/
#define WORKPHONE_ROTL( x, r ) ( ( x ) << ( r ) | ( ( x ) >> ( 32 - r ) ) )

    wp_u32 h1 = seed;
    wp_u32 k1;
    const wp_byte *data = (const wp_byte *)key;
    const wp_byte *keyptr = data;
    wp_byte *k1ptr;
    const wp_s32 bsize = sizeof( k1 );
    const wp_s32 nblocks = len / 4;

    const wp_u32 c1 = 0xcc9e2d51;
    const wp_u32 c2 = 0x1b873593;
    const wp_byte *tail;
    wp_s32 i;

    /* body */
    if( !key )
        return 0;
    for( i = 0; i < nblocks; ++i, keyptr += bsize )
    {
        k1ptr = (wp_byte *)&k1;
        k1ptr[0] = keyptr[0];
        k1ptr[1] = keyptr[1];
        k1ptr[2] = keyptr[2];
        k1ptr[3] = keyptr[3];

        k1 *= c1;
        k1 = WORKPHONE_ROTL( k1, 15 );
        k1 *= c2;

        h1 ^= k1;
        h1 = WORKPHONE_ROTL( h1, 13 );
        h1 = h1 * 5 + 0xe6546b64;
    }

    /* tail */
    tail = (const wp_byte *)( data + nblocks * 4 );
    k1 = 0;
    switch( len & 3 )
    {
    case 3:
        k1 ^= (wp_u32)( tail[2] << 16 ); /* fallthrough */
    case 2:
        k1 ^= (wp_u32)( tail[1] << 8u ); /* fallthrough */
    case 1:
        k1 ^= tail[0];
        k1 *= c1;
        k1 = WORKPHONE_ROTL( k1, 15 );
        k1 *= c2;
        h1 ^= k1;
        break;
    default:
        break;
    }

    /* finalization */
    h1 ^= (wp_u32)len;
    /* fmix32 */
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

#undef WORKPHONE_ROTL
    return h1;
}
#ifdef WORKPHONE_INCLUDE_STANDARD_IO
wp_c8 *wp_file_load( const wp_c8 *path, wp_size *siz, const struct wp_allocator *alloc )
{
    wp_c8 *buf;
    FILE *fd;
    long ret;

    WORKPHONE_ASSERT( path );
    WORKPHONE_ASSERT( siz );
    WORKPHONE_ASSERT( alloc );
    if( !path || !siz || !alloc )
        return 0;

    fd = fopen( path, "rb" );
    if( !fd )
        return 0;
    fseek( fd, 0, SEEK_END );
    ret = ftell( fd );
    if( ret < 0 )
    {
        fclose( fd );
        return 0;
    }
    *siz = (wp_size)ret;
    fseek( fd, 0, SEEK_SET );
    buf = (wp_c8 *)alloc->alloc( alloc->userdata, 0, *siz );
    WORKPHONE_ASSERT( buf );
    if( !buf )
    {
        fclose( fd );
        return 0;
    }
    *siz = (wp_size)fread( buf, 1, *siz, fd );
    fclose( fd );
    return buf;
}
#endif
WORKPHONE_LIB wp_s32 wp_text_clamp( const struct wp_user_font *font, const wp_c8 *text, wp_s32 text_len,
                                    wp_f32 space, wp_s32 *glyphs, wp_f32 *text_width, wp_rune *sep_list,
                                    wp_s32 sep_count )
{
    wp_s32 i = 0;
    wp_s32 glyph_len = 0;
    wp_f32 last_width = 0;
    wp_rune unicode = 0;
    wp_f32 width = 0;
    wp_s32 len = 0;
    wp_s32 g = 0;
    wp_f32 s;

    wp_s32 sep_len = 0;
    wp_s32 sep_g = 0;
    wp_f32 sep_width = 0;
    sep_count = WORKPHONE_MAX( sep_count, 0 );

    glyph_len = wp_utf_decode( text, &unicode, text_len );
    while( glyph_len && ( width < space ) && ( len < text_len ) )
    {
        len += glyph_len;
        s = font->width( font->userdata, font->height, text, len );
        for( i = 0; i < sep_count; ++i )
        {
            if( unicode != sep_list[i] )
                continue;
            sep_width = last_width = width;
            sep_g = g + 1;
            sep_len = len;
            break;
        }
        if( i == sep_count )
        {
            last_width = sep_width = width;
            sep_g = g + 1;
        }
        width = s;
        glyph_len = wp_utf_decode( &text[len], &unicode, text_len - len );
        g++;
    }
    if( len >= text_len )
    {
        *glyphs = g;
        *text_width = last_width;
        return len;
    }
    else
    {
        *glyphs = sep_g;
        *text_width = sep_width;
        return ( !sep_len ) ? len : sep_len;
    }
}
WORKPHONE_LIB struct wp_vec2f wp_text_calculate_text_bounds( const struct wp_user_font *font,
                                                            const wp_c8 *begin, wp_s32 byte_len,
                                                            wp_f32 row_height, const wp_c8 **remaining,
                                                            struct wp_vec2f *out_offset, wp_s32 *glyphs,
                                                            wp_s32 op )
{
    wp_f32 line_height = row_height;
    struct wp_vec2f text_size = wp_make_vec2f( 0, 0 );
    wp_f32 line_width = 0.0f;

    wp_f32 glyph_width;
    wp_s32 glyph_len = 0;
    wp_rune unicode = 0;
    wp_s32 text_len = 0;
    if( !begin || byte_len <= 0 || !font )
        return wp_make_vec2f( 0, row_height );

    glyph_len = wp_utf_decode( begin, &unicode, byte_len );
    if( !glyph_len )
        return text_size;
    glyph_width = font->width( font->userdata, font->height, begin, glyph_len );

    *glyphs = 0;
    while( ( text_len < byte_len ) && glyph_len )
    {
        if( unicode == '\n' )
        {
            text_size.x = WORKPHONE_MAX( text_size.x, line_width );
            text_size.y += line_height;
            line_width = 0;
            *glyphs += 1;
            if( op == WORKPHONE_STOP_ON_NEW_LINE )
                break;

            text_len++;
            glyph_len = wp_utf_decode( begin + text_len, &unicode, byte_len - text_len );
            continue;
        }

        if( unicode == '\r' )
        {
            text_len++;
            *glyphs += 1;
            glyph_len = wp_utf_decode( begin + text_len, &unicode, byte_len - text_len );
            continue;
        }

        *glyphs = *glyphs + 1;
        text_len += glyph_len;
        line_width += (wp_f32)glyph_width;
        glyph_len = wp_utf_decode( begin + text_len, &unicode, byte_len - text_len );
        glyph_width = font->width( font->userdata, font->height, begin + text_len, glyph_len );
        continue;
    }

    if( text_size.x < line_width )
        text_size.x = line_width;
    if( out_offset )
        *out_offset = wp_make_vec2f( line_width, text_size.y + line_height );
    if( line_width > 0 || text_size.y == 0.0f )
        text_size.y += line_height;
    if( remaining )
        *remaining = begin + text_len;
    return text_size;
}
