#ifndef workphone_prerequisites_h__
#define workphone_prerequisites_h__

#include "workphone_config.h"
#include "workphone_types.h"
//#include "Core/ArraySize.h"
//#include "Core/Casts.h"

/* Forward declarations */
struct wp_allocator;
struct wp_buffer;
struct wp_command_buffer;
struct wp_convert_config;
struct wp_context;
struct wp_checkbox_label;
struct wp_checkbox_text;
struct wp_draw_command;
struct wp_draw_list;
struct wp_draw_null_texture;
struct wp_draw_vertex_layout_element;
struct wp_edit_state;
struct wp_font_atlas;
struct wp_image;
struct wp_rect;
struct wp_page_element;
struct wp_panel;
struct wp_property_state;
struct wp_style_item;
struct wp_style_button;
struct wp_style_toggle;
struct wp_style_selectable;
struct wp_style_slide;
struct wp_style_progress;
struct wp_style_scrollbar;
struct wp_style_edit;
struct wp_style_property;
struct wp_style_chart;
struct wp_style_combo;
struct wp_scroll;
struct wp_style_tab;
struct wp_style_window_header;
struct wp_style_window;
struct wp_table;
struct wp_text_edit;
struct wp_user_font;

#ifndef WORKPHONE_POOL_DEFAULT_CAPACITY
#    define WORKPHONE_POOL_DEFAULT_CAPACITY 16
#endif

#ifndef WORKPHONE_DEFAULT_COMMAND_BUFFER_SIZE
#    define WORKPHONE_DEFAULT_COMMAND_BUFFER_SIZE ( 4 * 1024 )
#endif

#ifndef WORKPHONE_BUFFER_DEFAULT_INITIAL_SIZE
#    define WORKPHONE_BUFFER_DEFAULT_INITIAL_SIZE ( 4 * 1024 )
#endif

/* standard library headers */
#ifdef WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR
#    include <stdlib.h> /* malloc, free */
#endif
#ifdef WORKPHONE_INCLUDE_STANDARD_IO
#    include <stdio.h> /* fopen, fclose,... */
#endif
#ifdef WORKPHONE_INCLUDE_STANDARD_VARARGS
#    include <stdarg.h> /* valist, va_start, va_end, ... */
#endif

#ifndef WORKPHONE_ASSERT
#    include <assert.h>
#    define WORKPHONE_ASSERT( expr ) assert( expr )
#endif

#define WORKPHONE_DEFAULT ( -1 )

#ifndef WORKPHONE_VSNPRINTF
/* If your compiler does support `vsnprintf` I would highly recommend
 * defining this to vsnprintf instead since `vsprintf` is basically
 * unbelievable unsafe and should *NEVER* be used. But I have to support
 * it since C89 only provides this unsafe version. */
#    if ( defined( __STDC_VERSION__ ) && ( __STDC_VERSION__ >= 199901L ) ) || \
        ( defined( __cplusplus ) && ( __cplusplus >= 201103L ) ) || \
        ( defined( _POSIX_C_SOURCE ) && ( _POSIX_C_SOURCE >= 200112L ) ) || \
        ( defined( _XOPEN_SOURCE ) && ( _XOPEN_SOURCE >= 500 ) ) || defined( _ISOC99_SOURCE ) || \
        defined( _BSD_SOURCE )
#        define WORKPHONE_VSNPRINTF( s, n, f, a ) vsnprintf( s, n, f, a )
#    else
#        define WORKPHONE_VSNPRINTF( s, n, f, a ) vsprintf( s, f, a )
#    endif
#endif

/* util */
enum
{
    WORKPHONE_DO_NOT_STOP_ON_NEW_LINE,
    WORKPHONE_STOP_ON_NEW_LINE
};

WORKPHONE_LIB wp_bool wp_is_lower( wp_s32 c );
WORKPHONE_LIB wp_bool wp_is_upper( wp_s32 c );
WORKPHONE_LIB wp_s32 wp_to_upper( wp_s32 c );
WORKPHONE_LIB wp_s32 wp_to_lower( wp_s32 c );

#ifndef WORKPHONE_MEMCPY
#    define WORKPHONE_MEMCPY wp_memcopy
#    define WORKPHONE_MEMCPY_NEEDED
WORKPHONE_LIB void *wp_memcopy( void *dst, const void *src, wp_size n );
#endif

#ifndef WORKPHONE_MEMSET
#    define WORKPHONE_MEMSET wp_memset
#    define WORKPHONE_MEMSET_NEEDED
WORKPHONE_LIB void *wp_memset( void *ptr, wp_s32 c0, wp_size size );
#endif

WORKPHONE_LIB void wp_zero( void *ptr, wp_size size );
WORKPHONE_LIB wp_c8 *wp_itoa( wp_c8 *s, long n );
WORKPHONE_LIB wp_s32 wp_string_wp_f32_limit( wp_c8 *string, wp_s32 prec );

#ifndef WORKPHONE_DTOA
#    define WORKPHONE_DTOA wp_dtoa
#    define WORKPHONE_DTOA_NEEDED
WORKPHONE_LIB wp_c8 *wp_dtoa( wp_c8 *s, wp_f64 n );
#endif

#ifndef WORKPHONE_STRTOD
#    define WORKPHONE_STRTOD wp_strtod
#endif

WORKPHONE_LIB wp_s32 wp_text_clamp( const struct wp_user_font *font, const wp_c8 *text, wp_s32 text_len,
                                    wp_f32 space, wp_s32 *glyphs, wp_f32 *text_width, wp_rune *sep_list,
                                    wp_s32 sep_count );
WORKPHONE_LIB struct wp_vec2f wp_text_calculate_text_bounds( const struct wp_user_font *font,
                                                            const wp_c8 *begin, wp_s32 byte_len,
                                                            wp_f32 row_height, const wp_c8 **remaining,
                                                            struct wp_vec2f *out_offset, wp_s32 *glyphs,
                                                            wp_s32 op );
#ifdef WORKPHONE_INCLUDE_STANDARD_VARARGS
WORKPHONE_LIB wp_s32 wp_strfmt( wp_c8 *buf, wp_s32 buf_size, const wp_c8 *fmt, va_list args );
#endif

#ifdef WORKPHONE_INCLUDE_STANDARD_IO
WORKPHONE_LIB wp_c8 *wp_file_load( const wp_c8 *path, wp_size *siz, const struct wp_allocator *alloc );
#endif

/* math helpers that are only used by wp_dtoa */
#ifdef WORKPHONE_DTOA_NEEDED
WORKPHONE_LIB wp_f64 wp_pow( wp_f64 x, wp_s32 n );
WORKPHONE_LIB wp_s32 wp_ifloord( wp_f64 x );
WORKPHONE_LIB wp_s32 wp_log10( wp_f64 n );
#endif

/* buffer */
#ifdef WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR
WORKPHONE_LIB void *wp_malloc( wp_handle unused, void *old, wp_size size );
WORKPHONE_LIB void wp_mfree( wp_handle unused, void *ptr );
#endif

WORKPHONE_LIB void *wp_buffer_align( void *unaligned, wp_size align, wp_size *alignment,
                                     enum wp_buffer_allocation_type type );
WORKPHONE_LIB void *wp_buffer_alloc( struct wp_buffer *b, enum wp_buffer_allocation_type type,
                                     wp_size size, wp_size align );
WORKPHONE_LIB void *wp_buffer_realloc( struct wp_buffer *b, wp_size capacity, wp_size *size );

/* Pointer to Integer type conversion for pointer alignment */
#if defined( __PTRDIFF_TYPE__ ) /* This case should work for GCC*/
#    define WORKPHONE_UINT_TO_PTR( x ) ( (void *)(__PTRDIFF_TYPE__)( x ) )
#    define WORKPHONE_PTR_TO_UINT( x ) ( (wp_size)(__PTRDIFF_TYPE__)( x ) )
#elif !defined( __GNUC__ ) /* works for compilers other than LLVM */
#    define WORKPHONE_UINT_TO_PTR( x ) ( (void *)&( (wp_c8 *)0 )[x] )
#    define WORKPHONE_PTR_TO_UINT( x ) ( (wp_size)( ( (wp_c8 *)x ) - (wp_c8 *)0 ) )
#elif defined( WORKPHONE_USE_FIXED_TYPES ) /* used if we have <stdint.h> */
#    define WORKPHONE_UINT_TO_PTR( x ) ( (void *)(uintptr_t)( x ) )
#    define WORKPHONE_PTR_TO_UINT( x ) ( (uintptr_t)( x ) )
#else /* generates warning but works */
#    define WORKPHONE_UINT_TO_PTR( x ) ( (void *)( x ) )
#    define WORKPHONE_PTR_TO_UINT( x ) ( (wp_size)( x ) )
#endif

#define WORKPHONE_ALIGN_PTR( x, mask ) \
    ( WORKPHONE_UINT_TO_PTR( \
        ( WORKPHONE_PTR_TO_UINT( (wp_byte *)( x ) + ( mask - 1 ) ) & ~( mask - 1 ) ) ) )
#define WORKPHONE_ALIGN_PTR_BACK( x, mask ) \
    ( WORKPHONE_UINT_TO_PTR( ( WORKPHONE_PTR_TO_UINT( (wp_byte *)( x ) ) & ~( mask - 1 ) ) ) )

#if ( ( defined( __GNUC__ ) && __GNUC__ >= 4 ) || defined( __clang__ ) ) && !defined( EMSCRIPTEN )
#    define WORKPHONE_OFFSETOF( st, m ) ( __builtin_offsetof( st, m ) )
#else
#    define WORKPHONE_OFFSETOF( st, m ) ( ( wp_ptr ) & ( ( (st *)0 )->m ) )
#endif

#define WORKPHONE_UNDEFINED ( -1.0f )
#define WORKPHONE_UTF_INVALID 0xFFFD /**< internal invalid utf8 rune */
#define WORKPHONE_UTF_SIZE 4         /**< describes the number of bytes a glyph consists of*/

#ifndef WORKPHONE_INPUT_MAX
#    define WORKPHONE_INPUT_MAX 16
#endif

#ifndef WORKPHONE_MAX_NUMBER_BUFFER
#    define WORKPHONE_MAX_NUMBER_BUFFER 64
#endif

#ifndef WORKPHONE_SCROLLBAR_HIDING_TIMEOUT
#    define WORKPHONE_SCROLLBAR_HIDING_TIMEOUT 4.0f
#endif

#define WORKPHONE_FLAG( x ) ( 1 << ( x ) )
#define WORKPHONE_STRINGIFY( x ) #x
#define WORKPHONE_MACRO_STRINGIFY( x ) WORKPHONE_STRINGIFY( x )
#define WORKPHONE_STRING_JOIN_IMMEDIATE( arg1, arg2 ) arg1##arg2
#define WORKPHONE_STRING_JOIN_DELAY( arg1, arg2 ) WORKPHONE_STRING_JOIN_IMMEDIATE( arg1, arg2 )
#define WORKPHONE_STRING_JOIN( arg1, arg2 ) WORKPHONE_STRING_JOIN_DELAY( arg1, arg2 )

#ifdef _MSC_VER
#    define WORKPHONE_UNIQUE_NAME( name ) WORKPHONE_STRING_JOIN( name, __COUNTER__ )
#else
#    define WORKPHONE_UNIQUE_NAME( name ) WORKPHONE_STRING_JOIN( name, __LINE__ )
#endif

#ifndef WORKPHONE_FILE_LINE
#    ifdef _MSC_VER
#        define WORKPHONE_FILE_LINE __FILE__ ":" WORKPHONE_MACRO_STRINGIFY( __COUNTER__ )
#    else
#        define WORKPHONE_FILE_LINE __FILE__ ":" WORKPHONE_MACRO_STRINGIFY( __LINE__ )
#    endif
#endif

#define WORKPHONE_MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define WORKPHONE_MAX( a, b ) ( ( a ) < ( b ) ? ( b ) : ( a ) )
#define WORKPHONE_CLAMP( i, v, x ) ( WORKPHONE_MAX( WORKPHONE_MIN( v, x ), i ) )

#ifndef WORKPHONE_WINDOW_MAX_NAME
#    define WORKPHONE_WINDOW_MAX_NAME 64
#endif

#ifndef WORKPHONE_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS
#    define WORKPHONE_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS 16
#endif

#define WORKPHONE_PANEL_SET_POPUP \
    ( WORKPHONE_PANEL_POPUP | WORKPHONE_PANEL_CONTEXTUAL | WORKPHONE_PANEL_COMBO | \
      WORKPHONE_PANEL_MENU | WORKPHONE_PANEL_TOOLTIP )

#define WORKPHONE_FLOAT_PRECISION 0.00000000000001

/* Constants */
#ifndef WORKPHONE_WINDOW_MAX_NAME
#    define WORKPHONE_WINDOW_MAX_NAME 64
#endif

#ifndef WORKPHONE_CHART_MAX_SLOT
#    define WORKPHONE_CHART_MAX_SLOT 4
#endif

#ifndef WORKPHONE_VALUE_PAGE_CAPACITY
#    define WORKPHONE_VALUE_PAGE_CAPACITY 32
#endif

#ifndef WORKPHONE_BUTTON_BEHAVIOR_STACK_SIZE
#    define WORKPHONE_BUTTON_BEHAVIOR_STACK_SIZE 8
#endif

#ifndef WORKPHONE_FONT_STACK_SIZE
#    define WORKPHONE_FONT_STACK_SIZE 8
#endif

#ifndef WORKPHONE_STYLE_ITEM_STACK_SIZE
#    define WORKPHONE_STYLE_ITEM_STACK_SIZE 16
#endif

#ifndef WORKPHONE_FLOAT_STACK_SIZE
#    define WORKPHONE_FLOAT_STACK_SIZE 32
#endif

#ifndef WORKPHONE_VECTOR_STACK_SIZE
#    define WORKPHONE_VECTOR_STACK_SIZE 16
#endif

#ifndef WORKPHONE_FLAGS_STACK_SIZE
#    define WORKPHONE_FLAGS_STACK_SIZE 32
#endif

#ifndef WORKPHONE_TEXTEDIT_UNDOSTATECOUNT
#    define WORKPHONE_TEXTEDIT_UNDOSTATECOUNT 99
#endif

#ifndef WORKPHONE_TEXTEDIT_UNDOCHARCOUNT
#    define WORKPHONE_TEXTEDIT_UNDOCHARCOUNT 999
#endif

#ifndef WORKPHONE_COLOR_STACK_SIZE
#    define WORKPHONE_COLOR_STACK_SIZE 32
#endif

#ifndef WORKPHONE_CONTAINER_OF
#    define WORKPHONE_CONTAINER_OF( ptr, type, member ) \
        ( (type *)( (void *)( (wp_byte *)( ptr ) - WORKPHONE_OFFSETOF( type, member ) ) ) )
#endif

#define WORKPHONE_VALUE_PAGE_CAPACITY \
    ( ( ( WORKPHONE_MAX( sizeof( struct wp_window ), sizeof( struct wp_panel ) ) / \
          sizeof( wp_u32 ) ) ) / \
      2 )

#define WORKPHONE_CONFIGURATION_STACK_TYPE( prefix, name, type ) \
    struct wp_config_stack_##name##_element \
    { \
        prefix##_##type *address; \
        prefix##_##type old_value; \
    }
#define WORKPHONE_CONFIG_STACK( type, size ) \
    struct wp_config_stack_##type \
    { \
        int head; \
        struct wp_config_stack_##type##_element elements[size]; \
    }

#define wp_foreach( c, ctx ) for( ( c ) = wp__begin( ctx ); ( c ) != 0; ( c ) = wp__next( ctx, c ) )

#define wp_draw_foreach( cmd, ctx, b ) \
    for( ( cmd ) = wp__draw_begin( ctx, b ); ( cmd ) != 0; ( cmd ) = wp__draw_next( cmd, b, ctx ) )

enum wp_panel_flags
{
    WORKPHONE_WINDOW_BORDER = WORKPHONE_FLAG( 0 ),
    WORKPHONE_WINDOW_MOVABLE = WORKPHONE_FLAG( 1 ),
    WORKPHONE_WINDOW_SCALABLE = WORKPHONE_FLAG( 2 ),
    WORKPHONE_WINDOW_CLOSABLE = WORKPHONE_FLAG( 3 ),
    WORKPHONE_WINDOW_MINIMIZABLE = WORKPHONE_FLAG( 4 ),
    WORKPHONE_WINDOW_NO_SCROLLBAR = WORKPHONE_FLAG( 5 ),
    WORKPHONE_WINDOW_TITLE = WORKPHONE_FLAG( 6 ),
    WORKPHONE_WINDOW_SCROLL_AUTO_HIDE = WORKPHONE_FLAG( 7 ),
    WORKPHONE_WINDOW_BACKGROUND = WORKPHONE_FLAG( 8 ),
    WORKPHONE_WINDOW_SCALE_LEFT = WORKPHONE_FLAG( 9 ),
    WORKPHONE_WINDOW_NO_INPUT = WORKPHONE_FLAG( 10 )
};

enum wp_window_flags
{
    WORKPHONE_WINDOW_PRIVATE = WORKPHONE_FLAG( 11 ),
    WORKPHONE_WINDOW_DYNAMIC =
        WORKPHONE_WINDOW_PRIVATE, /**< special window type growing up in height while being filled to a certain maximum height */
    WORKPHONE_WINDOW_ROM = WORKPHONE_FLAG(
        12 ), /**< sets window widgets into a read only mode and does not allow input changes */
    WORKPHONE_WINDOW_NOT_INTERACTIVE =
        (int)WORKPHONE_WINDOW_ROM |
        (int)
            WORKPHONE_WINDOW_NO_INPUT, /**< prevents all interaction caused by input to either window or widgets inside */
    WORKPHONE_WINDOW_HIDDEN =
        WORKPHONE_FLAG( 13 ), /**< Hides window and stops any window interaction and drawing */
    WORKPHONE_WINDOW_CLOSED =
        WORKPHONE_FLAG( 14 ), /**< Directly closes and frees the window at the end of the frame */
    WORKPHONE_WINDOW_MINIMIZED = WORKPHONE_FLAG( 15 ), /**< marks the window as minimized */
    WORKPHONE_WINDOW_REMOVE_ROM =
        WORKPHONE_FLAG( 16 ) /**< Removes read only mode at the end of the window */
};

#endif  // workphone_prerequisites_h__





