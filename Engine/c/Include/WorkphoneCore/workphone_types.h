#ifndef workphone_types_h__
#define workphone_types_h__

#include "workphone_config.h"

#ifndef NULL
#    define NULL ( (void *)0 )
#endif

#define WORKPHONE_UNDEFINED ( -1.0f )
#define WORKPHONE_UTF_INVALID 0xFFFD /**< internal invalid utf8 rune */
#define WORKPHONE_UTF_SIZE 4         /**< describes the number of bytes a glyph consists of*/

#ifndef WORKPHONE_STATIC_ASSERT
#    define WORKPHONE_STATIC_ASSERT( exp ) \
        typedef wp_c8 WORKPHONE_UNIQUE_NAME( _dummy_array )[( exp ) ? 1 : -1]
#endif

#ifdef WORKPHONE_INCLUDE_FIXED_TYPES
#    include <stdint.h>
#    define WORKPHONE_INT8 int8_t
#    define WORKPHONE_UINT8 uint8_t
#    define WORKPHONE_INT16 int16_t
#    define WORKPHONE_UINT16 uint16_t
#    define WORKPHONE_INT32 int32_t
#    define WORKPHONE_UINT32 uint32_t
#    define WORKPHONE_SIZE_TYPE uintptr_t
#    define WORKPHONE_POINTER_TYPE uintptr_t
#else
#    ifndef WORKPHONE_INT8
#        define WORKPHONE_INT8 signed char
#    endif
#    ifndef WORKPHONE_UINT8
#        define WORKPHONE_UINT8 unsigned char
#    endif
#    ifndef WORKPHONE_INT16
#        define WORKPHONE_INT16 signed short
#    endif
#    ifndef WORKPHONE_UINT16
#        define WORKPHONE_UINT16 unsigned short
#    endif
#    ifndef WORKPHONE_INT32
#        if defined( _MSC_VER )
#            define WORKPHONE_INT32 __int32
#        else
#            define WORKPHONE_INT32 signed int
#        endif
#    endif
#    ifndef WORKPHONE_UINT32
#        if defined( _MSC_VER )
#            define WORKPHONE_UINT32 unsigned __int32
#        else
#            define WORKPHONE_UINT32 unsigned int
#        endif
#    endif
#    ifndef WORKPHONE_SIZE_TYPE
#        if defined( _WIN64 ) && defined( _MSC_VER )
#            define WORKPHONE_SIZE_TYPE unsigned __int64
#        elif defined( _WIN64 ) && ( defined( __MINGW64__ ) || defined( __clang__ ) )
#            define WORKPHONE_SIZE_TYPE unsigned long long
#        elif ( defined( _WIN32 ) || defined( WIN32 ) ) && defined( _MSC_VER )
#            define WORKPHONE_SIZE_TYPE unsigned __int32
#        elif ( defined( _WIN32 ) || defined( WIN32 ) ) && \
            ( defined( __MINGW32__ ) || defined( __clang__ ) )
#            define WORKPHONE_SIZE_TYPE unsigned long
#        elif defined( __GNUC__ ) || defined( __clang__ )
#            if defined( __x86_64__ ) || defined( __ppc64__ ) || defined( __PPC64__ ) || \
                defined( __aarch64__ )
#                define WORKPHONE_SIZE_TYPE unsigned long
#            else
#                define WORKPHONE_SIZE_TYPE unsigned int
#            endif
#        else
#            define WORKPHONE_SIZE_TYPE unsigned long
#        endif
#    endif
#    ifndef WORKPHONE_POINTER_TYPE
#        if defined( _WIN64 ) && defined( _MSC_VER )
#            define WORKPHONE_POINTER_TYPE unsigned __int64
#        elif defined( _WIN64 ) && ( defined( __MINGW64__ ) || defined( __clang__ ) )
#            define WORKPHONE_POINTER_TYPE unsigned long long
#        elif ( defined( _WIN32 ) || defined( WIN32 ) ) && defined( _MSC_VER )
#            define WORKPHONE_POINTER_TYPE unsigned __int32
#        elif ( defined( _WIN32 ) || defined( WIN32 ) ) && \
            ( defined( __MINGW32__ ) || defined( __clang__ ) )
#            define WORKPHONE_POINTER_TYPE unsigned long
#        elif defined( __GNUC__ ) || defined( __clang__ )
#            if defined( __x86_64__ ) || defined( __ppc64__ ) || defined( __PPC64__ ) || \
                defined( __aarch64__ )
#                define WORKPHONE_POINTER_TYPE unsigned long
#            else
#                define WORKPHONE_POINTER_TYPE unsigned int
#            endif
#        else
#            define WORKPHONE_POINTER_TYPE unsigned long
#        endif
#    endif
#endif

/**< could be any type with semantic of standard bool, either equal or smaller than wp_s32 */
#ifndef WORKPHONE_BOOL
#    ifdef WORKPHONE_INCLUDE_STANDARD_BOOL
#        include <stdbool.h>
#        define WORKPHONE_BOOL bool
#    else
#        define WORKPHONE_BOOL int
#    endif
#endif

typedef WORKPHONE_INT8 wp_c8;
typedef WORKPHONE_UINT8 wp_u8;
typedef WORKPHONE_UINT8 wp_byte;
typedef WORKPHONE_INT16 wp_s16;
typedef WORKPHONE_UINT16 wp_u16;
typedef WORKPHONE_INT32 wp_s32;
typedef WORKPHONE_UINT32 wp_u32;
typedef WORKPHONE_SIZE_TYPE wp_size;
typedef WORKPHONE_POINTER_TYPE wp_ptr;
typedef WORKPHONE_BOOL wp_bool;

typedef float wp_f32;
typedef double wp_f64;

typedef wp_u32 wp_hash;
typedef wp_u32 wp_flags;
typedef wp_u32 wp_rune;
typedef char wp_glyph[WORKPHONE_UTF_SIZE];

typedef union
{
    void *ptr;
    wp_s32 id;
} wp_handle;

typedef void ( *wp_command_custom_callback )( void *canvas, short x, short y, unsigned short w,
                                              unsigned short h, wp_handle callback_data );

/*
WORKPHONE_STATIC_ASSERT( !( (wp_bool)0 ) == !( wp_false ) );
WORKPHONE_STATIC_ASSERT( !( (wp_bool)1 ) == !( wp_true ) );

WORKPHONE_STATIC_ASSERT( sizeof( wp_short ) == 2 );
WORKPHONE_STATIC_ASSERT( sizeof( wp_u16 ) == 2 );
WORKPHONE_STATIC_ASSERT( sizeof( wp_u32 ) == 4 );
WORKPHONE_STATIC_ASSERT( sizeof( wp_wp_s32 ) == 4 );
WORKPHONE_STATIC_ASSERT( sizeof( wp_byte ) == 1 );
WORKPHONE_STATIC_ASSERT( sizeof( wp_flags ) >= 4 );
WORKPHONE_STATIC_ASSERT( sizeof( wp_rune ) >= 4 );
WORKPHONE_STATIC_ASSERT( sizeof( wp_size ) >= sizeof( void * ) );
WORKPHONE_STATIC_ASSERT( sizeof( wp_ptr ) >= sizeof( void * ) );
WORKPHONE_STATIC_ASSERT( sizeof( wp_bool ) <= sizeof( wp_s32 ) );

WORKPHONE_STATIC_ASSERT( sizeof( wp_size ) >= sizeof( void * ) );
WORKPHONE_STATIC_ASSERT( sizeof( wp_ptr ) == sizeof( void * ) );
WORKPHONE_STATIC_ASSERT( sizeof( wp_flags ) >= 4 );
WORKPHONE_STATIC_ASSERT( sizeof( wp_rune ) >= 4 );
WORKPHONE_STATIC_ASSERT( sizeof( wp_u16 ) == 2 );
WORKPHONE_STATIC_ASSERT( sizeof( wp_short ) == 2 );
WORKPHONE_STATIC_ASSERT( sizeof( wp_u32 ) == 4 );
WORKPHONE_STATIC_ASSERT( sizeof( wp_wp_s32 ) == 4 );
WORKPHONE_STATIC_ASSERT( sizeof( wp_byte ) == 1 );
WORKPHONE_STATIC_ASSERT( sizeof( wp_bool ) <= sizeof( wp_s32 ) );
*/

#define WORKPHONE_SCHAR_MIN ( -127 )
#define WORKPHONE_SCHAR_MAX 127
#define WORKPHONE_UCHAR_MIN 0
#define WORKPHONE_UCHAR_MAX 256
#define WORKPHONE_SSHORT_MIN ( -32767 )
#define WORKPHONE_SSHORT_MAX 32767
#define WORKPHONE_USHORT_MIN 0
#define WORKPHONE_USHORT_MAX 65535
#define WORKPHONE_SINT_MIN ( -2147483647 )
#define WORKPHONE_SINT_MAX 2147483647
#define WORKPHONE_UINT_MIN 0
#define WORKPHONE_UINT_MAX 4294967295u
#define WORKPHONE_SIZE_MAX ( (wp_size)( -1 ) )

/* Maximum string size allowed (DoS protection) */
#ifndef SAFE_STR_MAX
#    define SAFE_STR_MAX ( 1024 * 1024 ) /* 1 MB */
#endif

#ifndef SAFE_MALLOC_MAX
#    define SAFE_MALLOC_MAX ( 1024ULL * 1024ULL * 1024ULL ) /* 1 GB limit */
#endif

#ifndef SAFE_CALLOC_MAX
#    define SAFE_CALLOC_MAX ( 1024ULL * 1024ULL * 1024ULL ) /* 1 GB max allocation */
#endif

#define wp_ptr_add( t, p, i ) ( (t *)( (void *)( (wp_byte *)( p ) + ( i ) ) ) )
#define wp_ptr_add_const( t, p, i ) ( (const t *)( (const void *)( (const wp_byte *)( p ) + ( i ) ) ) )
#define wp_zero_struct( s ) wp_zero( &s, sizeof( s ) )

/** @brief Clear the colour buffer when passed to wp_renderer_software_clear(). */
#define WORKPHONE_CLEAR_FLAG_COLOR ( 1u << 0 )

/** @brief Clear the depth buffer when passed to wp_renderer_software_clear(). */
#define WORKPHONE_CLEAR_FLAG_DEPTH ( 1u << 1 )

/** @brief Convenience mask that clears both colour and depth buffers. */
#define WORKPHONE_CLEAR_FLAG_ALL ( WORKPHONE_CLEAR_FLAG_COLOR | WORKPHONE_CLEAR_FLAG_DEPTH )

#ifndef WP_UI_COMPONENT_MAX_LABEL
#    define WP_UI_COMPONENT_MAX_LABEL 256
#endif

#define WORKPHONE_WIDGET_DISABLED_FACTOR 0.5f

enum
{
    WP_UI_FLAG_CASCADE_INPUT = ( 1u << 1 ),
    WP_UI_FLAG_HANDLE_INPUT_EVENTS = ( 1u << 2 ),
    WP_UI_FLAG_SHOW_LABEL = ( 1u << 3 ),
    WP_UI_FLAG_AUTO_CALCULATE_ORDER = ( 1u << 4 )
};

enum
{
    wp_false,
    wp_true
};

/**
 * @brief Pixel layout of the colour framebuffer.
 */
typedef enum wp_pixel_format
{
    WORKPHONE_PIXEL_FORMAT_RGBA8 = 0, /**< 4 bytes per pixel: R, G, B, A (8 bits each). */
    WORKPHONE_PIXEL_FORMAT_BGRA8 = 1, /**< 4 bytes per pixel: B, G, R, A (8 bits each). */
    WORKPHONE_PIXEL_FORMAT_RGB8 = 2,  /**< 3 bytes per pixel: R, G, B (8 bits each).    */
    WORKPHONE_PIXEL_FORMAT_BGR8 = 3   /**< 3 bytes per pixel: B, G, R (8 bits each).    */
} wp_pixel_format;

/**
 * @brief Back-face culling policy applied per triangle.
 */
typedef enum wp_cull_mode
{
    WORKPHONE_CULL_MODE_NONE = 0, /**< No culling; both faces are rasterised.           */
    WORKPHONE_CULL_MODE_BACK = 1, /**< Cull back-facing triangles (default).            */
    WORKPHONE_CULL_MODE_FRONT = 2 /**< Cull front-facing triangles.                     */
} wp_cull_mode;

/**
 * @brief Comparison function used when writing to the depth buffer.
 */
typedef enum wp_depth_func
{
    WORKPHONE_DEPTH_FUNC_NEVER = 0,    /**< Fragment always fails depth test.         */
    WORKPHONE_DEPTH_FUNC_LESS = 1,     /**< Pass if incoming depth < stored depth.    */
    WORKPHONE_DEPTH_FUNC_EQUAL = 2,    /**< Pass if incoming depth == stored depth.   */
    WORKPHONE_DEPTH_FUNC_LEQUAL = 3,   /**< Pass if incoming depth <= stored depth.   */
    WORKPHONE_DEPTH_FUNC_GREATER = 4,  /**< Pass if incoming depth > stored depth.    */
    WORKPHONE_DEPTH_FUNC_NOTEQUAL = 5, /**< Pass if incoming depth != stored depth.   */
    WORKPHONE_DEPTH_FUNC_GEQUAL = 6,   /**< Pass if incoming depth >= stored depth.   */
    WORKPHONE_DEPTH_FUNC_ALWAYS = 7    /**< Fragment always passes depth test.        */
} wp_depth_func;

/**
 * @brief Controls how triangles are rasterised.
 */
typedef enum wp_fill_mode
{
    WORKPHONE_FILL_MODE_SOLID = 0,     /**< Filled triangles.                    */
    WORKPHONE_FILL_MODE_WIREFRAME = 1, /**< Only triangle edges are drawn.       */
    WORKPHONE_FILL_MODE_POINT = 2      /**< Only triangle vertices are drawn.    */
} wp_fill_mode;

/**
 * @brief Per-fragment colour blending equation applied during rasterisation.
 */
typedef enum wp_blend_mode
{
    WORKPHONE_BLEND_MODE_NONE = 0,     /**< No blending; fragments overwrite the framebuffer. */
    WORKPHONE_BLEND_MODE_ALPHA = 1,    /**< Standard alpha blending: out = src*a + dst*(1-a).  */
    WORKPHONE_BLEND_MODE_ADDITIVE = 2, /**< Additive:  out = src*a + dst.                      */
    WORKPHONE_BLEND_MODE_MULTIPLY = 3, /**< Multiply:  out = src * dst.                        */
    WORKPHONE_BLEND_MODE_PREMULTIPLIED = 4 /**< Premultiplied alpha: src + dst*(1-a).           */
} wp_blend_mode;

typedef enum wp_symbol_type
{
    WORKPHONE_SYMBOL_NONE,
    WORKPHONE_SYMBOL_X,
    WORKPHONE_SYMBOL_UNDERSCORE,
    WORKPHONE_SYMBOL_CIRCLE_SOLID,
    WORKPHONE_SYMBOL_CIRCLE_OUTLINE,
    WORKPHONE_SYMBOL_RECT_SOLID,
    WORKPHONE_SYMBOL_RECT_OUTLINE,
    WORKPHONE_SYMBOL_TRIANGLE_UP,
    WORKPHONE_SYMBOL_TRIANGLE_DOWN,
    WORKPHONE_SYMBOL_TRIANGLE_LEFT,
    WORKPHONE_SYMBOL_TRIANGLE_RIGHT,
    WORKPHONE_SYMBOL_PLUS,
    WORKPHONE_SYMBOL_MINUS,
    WORKPHONE_SYMBOL_TRIANGLE_UP_OUTLINE,
    WORKPHONE_SYMBOL_TRIANGLE_DOWN_OUTLINE,
    WORKPHONE_SYMBOL_TRIANGLE_LEFT_OUTLINE,
    WORKPHONE_SYMBOL_TRIANGLE_RIGHT_OUTLINE,
    WORKPHONE_SYMBOL_MAX
} wp_symbol_type;

enum wp_heading
{
    WORKPHONE_UP,
    WORKPHONE_RIGHT,
    WORKPHONE_DOWN,
    WORKPHONE_LEFT
};

typedef enum wp_button_behavior_enum
{
    WORKPHONE_BUTTON_DEFAULT,
    WORKPHONE_BUTTON_REPEATER
} wp_button_behavior_enum;

typedef enum wp_modify
{
    WORKPHONE_FIXED = wp_false,
    WORKPHONE_MODIFIABLE = wp_true
} wp_modify;

typedef enum wp_orientation
{
    WORKPHONE_VERTICAL,
    WORKPHONE_HORIZONTAL
} wp_orientation;

enum wp_collapse_states
{
    WORKPHONE_MINIMIZED = 0,
    WORKPHONE_MAXIMIZED = 1
};

enum wp_show_states
{
    WORKPHONE_HIDDEN = 0,
    WORKPHONE_SHOWN = 1
};

typedef enum wp_chart_type
{
    WORKPHONE_CHART_LINES,
    WORKPHONE_CHART_COLUMN,
    WORKPHONE_CHART_MAX
} wp_chart_type;

typedef enum wp_chart_event
{
    WORKPHONE_CHART_HOVERING = 0x01,
    WORKPHONE_CHART_CLICKED = 0x02
} wp_chart_event;

typedef enum wp_color_format
{
    WORKPHONE_RGB,
    WORKPHONE_RGBA
} wp_color_format;

typedef enum wp_popup_type
{
    WORKPHONE_POPUP_STATIC,
    WORKPHONE_POPUP_DYNAMIC
} wp_popup_type;

enum wp_layout_format
{
    WORKPHONE_DYNAMIC,
    WORKPHONE_STATIC
};

enum wp_tree_type
{
    WORKPHONE_TREE_NODE,
    WORKPHONE_TREE_TAB
};

enum wp_widget_layout_states
{
    WORKPHONE_WIDGET_INVALID,
    WORKPHONE_WIDGET_VALID,
    WORKPHONE_WIDGET_ROM,
    WORKPHONE_WIDGET_DISABLED
};

enum wp_widget_states
{
    WORKPHONE_WIDGET_STATE_MODIFIED = ( 1 << 1 ),
    WORKPHONE_WIDGET_STATE_INACTIVE = ( 1 << 2 ),
    WORKPHONE_WIDGET_STATE_ENTERED = ( 1 << 3 ),
    WORKPHONE_WIDGET_STATE_HOVER = ( 1 << 4 ),
    WORKPHONE_WIDGET_STATE_ACTIVED = ( 1 << 5 ),
    WORKPHONE_WIDGET_STATE_LEFT = ( 1 << 6 ),
    WORKPHONE_WIDGET_STATE_HOVERED = ( 1 << 7 ),
    WORKPHONE_WIDGET_STATE_ACTIVE = ( 1 << 8 )
};

/* window */
typedef enum wp_window_insert_location
{
    WORKPHONE_INSERT_BACK, /* inserts window into the back of list (front of screen) */
    WORKPHONE_INSERT_FRONT /* inserts window into the front of list (back of screen) */
} wp_window_insert_location;

/* toggle */
typedef enum wp_toggle_type
{
    WORKPHONE_TOGGLE_CHECK,
    WORKPHONE_TOGGLE_OPTION
} wp_toggle_type;

typedef enum wp_cursor_type
{
    WORKPHONE_CURSOR_ARROW,
    WORKPHONE_CURSOR_TEXT,
    WORKPHONE_CURSOR_MOVE,
    WORKPHONE_CURSOR_RESIZE_VERTICAL,
    WORKPHONE_CURSOR_RESIZE_HORIZONTAL,
    WORKPHONE_CURSOR_RESIZE_TOP_LEFT_DOWN_RIGHT,
    WORKPHONE_CURSOR_RESIZE_TOP_RIGHT_DOWN_LEFT,
    WORKPHONE_CURSOR_COUNT
} wp_cursor_type;

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

enum wp_keys
{
    WORKPHONE_KEY_NONE,
    WORKPHONE_KEY_SHIFT,
    WORKPHONE_KEY_CTRL,
    WORKPHONE_KEY_DEL,
    WORKPHONE_KEY_ENTER,
    WORKPHONE_KEY_TAB,
    WORKPHONE_KEY_BACKSPACE,
    WORKPHONE_KEY_COPY,
    WORKPHONE_KEY_CUT,
    WORKPHONE_KEY_PASTE,
    WORKPHONE_KEY_UP,
    WORKPHONE_KEY_DOWN,
    WORKPHONE_KEY_LEFT,
    WORKPHONE_KEY_RIGHT,
    /* Shortcuts: text field */
    WORKPHONE_KEY_TEXT_INSERT_MODE,
    WORKPHONE_KEY_TEXT_REPLACE_MODE,
    WORKPHONE_KEY_TEXT_RESET_MODE,
    WORKPHONE_KEY_TEXT_LINE_START,
    WORKPHONE_KEY_TEXT_LINE_END,
    WORKPHONE_KEY_TEXT_START,
    WORKPHONE_KEY_TEXT_END,
    WORKPHONE_KEY_TEXT_UNDO,
    WORKPHONE_KEY_TEXT_REDO,
    WORKPHONE_KEY_TEXT_SELECT_ALL,
    WORKPHONE_KEY_TEXT_WORD_LEFT,
    WORKPHONE_KEY_TEXT_WORD_RIGHT,
    /* Shortcuts: scrollbar */
    WORKPHONE_KEY_SCROLL_START,
    WORKPHONE_KEY_SCROLL_END,
    WORKPHONE_KEY_SCROLL_DOWN,
    WORKPHONE_KEY_SCROLL_UP,
    WORKPHONE_KEY_MAX
};

enum wp_buttons
{
    WORKPHONE_BUTTON_LEFT,
    WORKPHONE_BUTTON_MIDDLE,
    WORKPHONE_BUTTON_RIGHT,
    WORKPHONE_BUTTON_DOUBLE,
    WORKPHONE_BUTTON_MAX
};

enum wp_panel_type
{
    WORKPHONE_PANEL_NONE = 0,
    WORKPHONE_PANEL_WINDOW = WORKPHONE_FLAG( 0 ),
    WORKPHONE_PANEL_GROUP = WORKPHONE_FLAG( 1 ),
    WORKPHONE_PANEL_POPUP = WORKPHONE_FLAG( 2 ),
    WORKPHONE_PANEL_CONTEXTUAL = WORKPHONE_FLAG( 4 ),
    WORKPHONE_PANEL_COMBO = WORKPHONE_FLAG( 5 ),
    WORKPHONE_PANEL_MENU = WORKPHONE_FLAG( 6 ),
    WORKPHONE_PANEL_TOOLTIP = WORKPHONE_FLAG( 7 )
};

enum wp_panel_set
{
    WORKPHONE_PANEL_SET_NONBLOCK = (int)( (int)WORKPHONE_PANEL_CONTEXTUAL | (int)WORKPHONE_PANEL_COMBO |
                                          (int)WORKPHONE_PANEL_MENU | (int)WORKPHONE_PANEL_TOOLTIP ),
    WORKPHONE_PANEL_SET_POPUP = (int)( (int)WORKPHONE_PANEL_SET_NONBLOCK | (int)WORKPHONE_PANEL_POPUP ),
    WORKPHONE_PANEL_SET_SUB = (int)( (int)WORKPHONE_PANEL_SET_POPUP | (int)WORKPHONE_PANEL_GROUP )
};

enum wp_allocation_type
{
    WORKPHONE_BUFFER_FIXED,
    WORKPHONE_BUFFER_DYNAMIC
};

enum wp_buffer_allocation_type
{
    WORKPHONE_BUFFER_FRONT,
    WORKPHONE_BUFFER_BACK,
    WORKPHONE_BUFFER_MAX
};

enum wp_command_clipping
{
    WORKPHONE_CLIPPING_OFF = 0,
    WORKPHONE_CLIPPING_ON = 1
};

enum wp_style_header_align
{
    WORKPHONE_HEADER_LEFT,
    WORKPHONE_HEADER_RIGHT
};

/* Style item types */
enum wp_style_item_type
{
    WORKPHONE_STYLE_ITEM_COLOR,
    WORKPHONE_STYLE_ITEM_IMAGE,
    WORKPHONE_STYLE_ITEM_NINE_SLICE
};

enum wp_text_align
{
    WORKPHONE_TEXT_ALIGN_LEFT = 0x01,
    WORKPHONE_TEXT_ALIGN_CENTERED = 0x02,
    WORKPHONE_TEXT_ALIGN_RIGHT = 0x04,
    WORKPHONE_TEXT_ALIGN_TOP = 0x08,
    WORKPHONE_TEXT_ALIGN_MIDDLE = 0x10,
    WORKPHONE_TEXT_ALIGN_BOTTOM = 0x20
};

enum wp_text_alignment
{
    WORKPHONE_TEXT_LEFT = WORKPHONE_TEXT_ALIGN_MIDDLE | WORKPHONE_TEXT_ALIGN_LEFT,
    WORKPHONE_TEXT_CENTERED = WORKPHONE_TEXT_ALIGN_MIDDLE | WORKPHONE_TEXT_ALIGN_CENTERED,
    WORKPHONE_TEXT_RIGHT = WORKPHONE_TEXT_ALIGN_MIDDLE | WORKPHONE_TEXT_ALIGN_RIGHT
};

enum wp_text_edit_type
{
    WORKPHONE_TEXT_EDIT_SINGLE_LINE,
    WORKPHONE_TEXT_EDIT_MULTI_LINE
};

enum wp_text_edit_mode
{
    WORKPHONE_TEXT_EDIT_MODE_VIEW,
    WORKPHONE_TEXT_EDIT_MODE_INSERT,
    WORKPHONE_TEXT_EDIT_MODE_REPLACE
};

enum wp_panel_row_layout_type
{
    WORKPHONE_LAYOUT_DYNAMIC_FIXED = 0,
    WORKPHONE_LAYOUT_DYNAMIC_ROW,
    WORKPHONE_LAYOUT_DYNAMIC_FREE,
    WORKPHONE_LAYOUT_DYNAMIC,
    WORKPHONE_LAYOUT_STATIC_FIXED,
    WORKPHONE_LAYOUT_STATIC_ROW,
    WORKPHONE_LAYOUT_STATIC_FREE,
    WORKPHONE_LAYOUT_STATIC,
    WORKPHONE_LAYOUT_TEMPLATE,
    WORKPHONE_LAYOUT_COUNT
};

enum wp_property_status
{
    WORKPHONE_PROPERTY_DEFAULT,
    WORKPHONE_PROPERTY_EDIT,
    WORKPHONE_PROPERTY_DRAG
};

enum wp_property_filter
{
    WORKPHONE_FILTER_INT,
    WORKPHONE_FILTER_FLOAT
};

enum wp_property_kind
{
    WORKPHONE_PROPERTY_INT,
    WORKPHONE_PROPERTY_FLOAT,
    WORKPHONE_PROPERTY_DOUBLE
};

enum wp_widget_align
{
    WORKPHONE_WIDGET_ALIGN_LEFT = 0x01,
    WORKPHONE_WIDGET_ALIGN_CENTERED = 0x02,
    WORKPHONE_WIDGET_ALIGN_RIGHT = 0x04,
    WORKPHONE_WIDGET_ALIGN_TOP = 0x08,
    WORKPHONE_WIDGET_ALIGN_MIDDLE = 0x10,
    WORKPHONE_WIDGET_ALIGN_BOTTOM = 0x20
};

enum wp_widget_alignment
{
    WORKPHONE_WIDGET_LEFT = WORKPHONE_WIDGET_ALIGN_MIDDLE | WORKPHONE_WIDGET_ALIGN_LEFT,
    WORKPHONE_WIDGET_CENTERED = WORKPHONE_WIDGET_ALIGN_MIDDLE | WORKPHONE_WIDGET_ALIGN_CENTERED,
    WORKPHONE_WIDGET_RIGHT = WORKPHONE_WIDGET_ALIGN_MIDDLE | WORKPHONE_WIDGET_ALIGN_RIGHT
};

typedef enum wp_command_type
{
    WORKPHONE_COMMAND_NOP,
    WORKPHONE_COMMAND_SCISSOR,
    WORKPHONE_COMMAND_LINE,
    WORKPHONE_COMMAND_CURVE,
    WORKPHONE_COMMAND_RECT,
    WORKPHONE_COMMAND_RECT_FILLED,
    WORKPHONE_COMMAND_RECT_MULTI_COLOR,
    WORKPHONE_COMMAND_CIRCLE,
    WORKPHONE_COMMAND_CIRCLE_FILLED,
    WORKPHONE_COMMAND_ARC,
    WORKPHONE_COMMAND_ARC_FILLED,
    WORKPHONE_COMMAND_TRIANGLE,
    WORKPHONE_COMMAND_TRIANGLE_FILLED,
    WORKPHONE_COMMAND_POLYGON,
    WORKPHONE_COMMAND_POLYGON_FILLED,
    WORKPHONE_COMMAND_POLYLINE,
    WORKPHONE_COMMAND_TEXT,
    WORKPHONE_COMMAND_IMAGE,
    WORKPHONE_COMMAND_CUSTOM
} wp_command_type;

enum wp_edit_flags
{
    WORKPHONE_EDIT_DEFAULT = 0,
    WORKPHONE_EDIT_READ_ONLY = WORKPHONE_FLAG( 0 ),
    WORKPHONE_EDIT_AUTO_SELECT = WORKPHONE_FLAG( 1 ),
    WORKPHONE_EDIT_SIG_ENTER = WORKPHONE_FLAG( 2 ),
    WORKPHONE_EDIT_ALLOW_TAB = WORKPHONE_FLAG( 3 ),
    WORKPHONE_EDIT_NO_CURSOR = WORKPHONE_FLAG( 4 ),
    WORKPHONE_EDIT_SELECTABLE = WORKPHONE_FLAG( 5 ),
    WORKPHONE_EDIT_CLIPBOARD = WORKPHONE_FLAG( 6 ),
    WORKPHONE_EDIT_CTRL_ENTER_NEWLINE = WORKPHONE_FLAG( 7 ),
    WORKPHONE_EDIT_NO_HORIZONTAL_SCROLL = WORKPHONE_FLAG( 8 ),
    WORKPHONE_EDIT_ALWAYS_INSERT_MODE = WORKPHONE_FLAG( 9 ),
    WORKPHONE_EDIT_MULTILINE = WORKPHONE_FLAG( 10 ),
    WORKPHONE_EDIT_GOTO_END_ON_ACTIVATE = WORKPHONE_FLAG( 11 )
};

enum wp_edit_types
{
    WORKPHONE_EDIT_SIMPLE = (int)WORKPHONE_EDIT_ALWAYS_INSERT_MODE,
    WORKPHONE_EDIT_FIELD =
        (int)WORKPHONE_EDIT_SIMPLE | (int)WORKPHONE_EDIT_SELECTABLE | (int)WORKPHONE_EDIT_CLIPBOARD,
    WORKPHONE_EDIT_BOX = (int)WORKPHONE_EDIT_ALWAYS_INSERT_MODE | (int)WORKPHONE_EDIT_SELECTABLE |
                         (int)WORKPHONE_EDIT_MULTILINE | (int)WORKPHONE_EDIT_ALLOW_TAB |
                         (int)WORKPHONE_EDIT_CLIPBOARD,
    WORKPHONE_EDIT_EDITOR = (int)WORKPHONE_EDIT_SELECTABLE | (int)WORKPHONE_EDIT_MULTILINE |
                            (int)WORKPHONE_EDIT_ALLOW_TAB | (int)WORKPHONE_EDIT_CLIPBOARD
};

enum wp_edit_events
{
    WORKPHONE_EDIT_ACTIVE = WORKPHONE_FLAG( 0 ), /**!< edit widget is currently being modified */
    WORKPHONE_EDIT_INACTIVE =
        WORKPHONE_FLAG( 1 ), /**!< edit widget is not active and is not being modified */
    WORKPHONE_EDIT_ACTIVATED =
        WORKPHONE_FLAG( 2 ), /**!< edit widget went from state inactive to state active */
    WORKPHONE_EDIT_DEACTIVATED =
        WORKPHONE_FLAG( 3 ), /**!< edit widget went from state active to state inactive */
    WORKPHONE_EDIT_COMMITED =
        WORKPHONE_FLAG( 4 ) /**!< edit widget has received an enter and lost focus */
};

enum wp_anti_aliasing
{
    WORKPHONE_ANTI_ALIASING_OFF,
    WORKPHONE_ANTI_ALIASING_ON
};

enum wp_convert_result
{
    WORKPHONE_CONVERT_SUCCESS = 0,
    WORKPHONE_CONVERT_INVALID_PARAM = 1,
    WORKPHONE_CONVERT_COMMAND_BUFFER_FULL = WORKPHONE_FLAG( 1 ),
    WORKPHONE_CONVERT_VERTEX_BUFFER_FULL = WORKPHONE_FLAG( 2 ),
    WORKPHONE_CONVERT_ELEMENT_BUFFER_FULL = WORKPHONE_FLAG( 3 )
};

enum wp_style_colors
{
    WORKPHONE_COLOR_TEXT,
    WORKPHONE_COLOR_WINDOW,
    WORKPHONE_COLOR_HEADER,
    WORKPHONE_COLOR_BORDER,
    WORKPHONE_COLOR_BUTTON,
    WORKPHONE_COLOR_BUTTON_HOVER,
    WORKPHONE_COLOR_BUTTON_ACTIVE,
    WORKPHONE_COLOR_TOGGLE,
    WORKPHONE_COLOR_TOGGLE_HOVER,
    WORKPHONE_COLOR_TOGGLE_CURSOR,
    WORKPHONE_COLOR_SELECT,
    WORKPHONE_COLOR_SELECT_ACTIVE,
    WORKPHONE_COLOR_SLIDER,
    WORKPHONE_COLOR_SLIDER_CURSOR,
    WORKPHONE_COLOR_SLIDER_CURSOR_HOVER,
    WORKPHONE_COLOR_SLIDER_CURSOR_ACTIVE,
    WORKPHONE_COLOR_PROPERTY,
    WORKPHONE_COLOR_EDIT,
    WORKPHONE_COLOR_EDIT_CURSOR,
    WORKPHONE_COLOR_COMBO,
    WORKPHONE_COLOR_CHART,
    WORKPHONE_COLOR_CHART_COLOR,
    WORKPHONE_COLOR_CHART_COLOR_HIGHLIGHT,
    WORKPHONE_COLOR_SCROLLBAR,
    WORKPHONE_COLOR_SCROLLBAR_CURSOR,
    WORKPHONE_COLOR_SCROLLBAR_CURSOR_HOVER,
    WORKPHONE_COLOR_SCROLLBAR_CURSOR_ACTIVE,
    WORKPHONE_COLOR_TAB_HEADER,
    WORKPHONE_COLOR_KNOB,
    WORKPHONE_COLOR_KNOB_CURSOR,
    WORKPHONE_COLOR_KNOB_CURSOR_HOVER,
    WORKPHONE_COLOR_KNOB_CURSOR_ACTIVE,
    WORKPHONE_COLOR_COUNT
};

#endif  // workphone_types_h__
