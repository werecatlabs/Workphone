#ifndef __WORKPHONE_UI_H_
#define __WORKPHONE_UI_H_

#include "workphone_prerequisites.h"
#include "workphone_color.h"
#include "workphone_context.h"
#include "workphone_vector.h"
#include "workphone_style.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WORKPHONE_INCLUDE_STANDARD_VARARGS
#    include <stdarg.h>
#    if defined( _MSC_VER ) && ( _MSC_VER >= 1600 ) /* VS 2010 and above */
#        include <sal.h>
#        define WORKPHONE_PRINTF_FORMAT_STRING _Printf_format_string_
#    else
#        define WORKPHONE_PRINTF_FORMAT_STRING
#    endif
#    if defined( __GNUC__ )
#        define WORKPHONE_PRINTF_VARARG_FUNC( fmtargnumber ) \
            __attribute__( ( format( __printf__, fmtargnumber, fmtargnumber + 1 ) ) )
#        define WORKPHONE_PRINTF_VALIST_FUNC( fmtargnumber ) \
            __attribute__( ( format( __printf__, fmtargnumber, 0 ) ) )
#    else
#        define WORKPHONE_PRINTF_VARARG_FUNC( fmtargnumber )
#        define WORKPHONE_PRINTF_VALIST_FUNC( fmtargnumber )
#    endif
#endif

#ifdef WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR
WORKPHONE_API wp_bool wp_init_default( struct wp_context *, const struct wp_user_font * );
#endif

struct wp_command
{
    wp_command_type type;
    wp_size next;
#ifdef WORKPHONE_INCLUDE_COMMAND_USERDATA
    wp_handle userdata;
#endif
};

/* Scissor command */
struct wp_command_scissor
{
    struct wp_command header;
    short x;
    short y;
    unsigned short w;
    unsigned short h;
};

/* Line command */
struct wp_command_line
{
    struct wp_command header;
    unsigned short line_thickness;
    struct wp_vec2i begin;
    struct wp_vec2i end;
    struct wp_color color;
};

/* Curve command */
struct wp_command_curve
{
    struct wp_command header;
    unsigned short line_thickness;
    struct wp_vec2i begin;
    struct wp_vec2i end;
    struct wp_vec2i ctrl[2];
    struct wp_color color;
};

/* Rectangle (stroke) command */
struct wp_command_rect
{
    struct wp_command header;
    unsigned short rounding;
    unsigned short line_thickness;
    short x;
    short y;
    unsigned short w;
    unsigned short h;
    struct wp_color color;
};

/* Rectangle filled command */
struct wp_command_rect_filled
{
    struct wp_command header;
    unsigned short rounding;
    short x;
    short y;
    unsigned short w;
    unsigned short h;
    struct wp_color color;
};

/* Rectangle multi-color command */
struct wp_command_rect_multi_color
{
    struct wp_command header;
    short x;
    short y;
    unsigned short w;
    unsigned short h;
    struct wp_color left;
    struct wp_color top;
    struct wp_color right;
    struct wp_color bottom;
};

/* Circle (stroke) command */
struct wp_command_circle
{
    struct wp_command header;
    short x;
    short y;
    unsigned short line_thickness;
    unsigned short w;
    unsigned short h;
    struct wp_color color;
};

/* Circle filled command */
struct wp_command_circle_filled
{
    struct wp_command header;
    short x;
    short y;
    unsigned short w;
    unsigned short h;
    struct wp_color color;
};

/* Arc (stroke) command */
struct wp_command_arc
{
    struct wp_command header;
    short cx;
    short cy;
    unsigned short r;
    unsigned short line_thickness;
    wp_f32 a[2];
    struct wp_color color;
};

/* Arc filled command */
struct wp_command_arc_filled
{
    struct wp_command header;
    short cx;
    short cy;
    unsigned short r;
    wp_f32 a[2];
    struct wp_color color;
};

/* Triangle (stroke) command */
struct wp_command_triangle
{
    struct wp_command header;
    unsigned short line_thickness;
    struct wp_vec2i a;
    struct wp_vec2i b;
    struct wp_vec2i c;
    struct wp_color color;
};

/* Triangle filled command */
struct wp_command_triangle_filled
{
    struct wp_command header;
    struct wp_vec2i a;
    struct wp_vec2i b;
    struct wp_vec2i c;
    struct wp_color color;
};

/* Polygon (stroke) command */
struct wp_command_polygon
{
    struct wp_command header;
    struct wp_color color;
    unsigned short line_thickness;
    unsigned short point_count;
    struct wp_vec2i points[1];
};

/* Polygon filled command */
struct wp_command_polygon_filled
{
    struct wp_command header;
    struct wp_color color;
    unsigned short point_count;
    struct wp_vec2i points[1];
};

/* Polyline command */
struct wp_command_polyline
{
    struct wp_command header;
    struct wp_color color;
    unsigned short line_thickness;
    unsigned short point_count;
    struct wp_vec2i points[1];
};

/* Image command */
struct wp_command_image
{
    struct wp_command header;
    short x;
    short y;
    unsigned short w;
    unsigned short h;
    struct wp_image img;
    struct wp_color col;
};

/* Custom callback type */
typedef void ( *wp_command_custom_callback )( void *canvas, short x, short y, unsigned short w,
                                              unsigned short h, wp_handle callback_data );

/* Custom command */
struct wp_command_custom
{
    struct wp_command header;
    short x;
    short y;
    unsigned short w;
    unsigned short h;
    wp_handle callback_data;
    wp_command_custom_callback callback;
};

/* Text command */
struct wp_command_text
{
    struct wp_command header;
    const struct wp_user_font *font;
    struct wp_color background;
    struct wp_color foreground;
    short x;
    short y;
    unsigned short w;
    unsigned short h;
    wp_f32 height;
    int length;
    char string[2];
};

/** shape outlines */
WORKPHONE_API void wp_stroke_line( struct wp_command_buffer *b, wp_f32 x0, wp_f32 y0, wp_f32 x1,
                                   wp_f32 y1, wp_f32 line_thickness, struct wp_color );
WORKPHONE_API void wp_stroke_curve( struct wp_command_buffer *, wp_f32, wp_f32, wp_f32, wp_f32, wp_f32,
                                    wp_f32, wp_f32, wp_f32, wp_f32 line_thickness, struct wp_color );
WORKPHONE_API void wp_stroke_rect( struct wp_command_buffer *, struct wp_rect, wp_f32 rounding,
                                   wp_f32 line_thickness, struct wp_color );
WORKPHONE_API void wp_stroke_circle( struct wp_command_buffer *, struct wp_rect, wp_f32 line_thickness,
                                     struct wp_color );
WORKPHONE_API void wp_stroke_arc( struct wp_command_buffer *, wp_f32 cx, wp_f32 cy, wp_f32 radius,
                                  wp_f32 a_min, wp_f32 a_max, wp_f32 line_thickness, struct wp_color );
WORKPHONE_API void wp_stroke_triangle( struct wp_command_buffer *, wp_f32, wp_f32, wp_f32, wp_f32,
                                       wp_f32, wp_f32, wp_f32 line_thichness, struct wp_color );
WORKPHONE_API void wp_stroke_polyline( struct wp_command_buffer *, const wp_f32 *points, int point_count,
                                       wp_f32 line_thickness, struct wp_color col );
WORKPHONE_API void wp_stroke_polygon( struct wp_command_buffer *, const wp_f32 *points, int point_count,
                                      wp_f32 line_thickness, struct wp_color );

/** filled shades */
WORKPHONE_API void wp_fill_rect( struct wp_command_buffer *, struct wp_rect, wp_f32 rounding,
                                 struct wp_color );
WORKPHONE_API void wp_fill_rect_multi_color( struct wp_command_buffer *, struct wp_rect,
                                             struct wp_color left, struct wp_color top,
                                             struct wp_color bottom, struct wp_color right );
WORKPHONE_API void wp_fill_circle( struct wp_command_buffer *, struct wp_rect, struct wp_color );
WORKPHONE_API void wp_fill_arc( struct wp_command_buffer *, wp_f32 cx, wp_f32 cy, wp_f32 radius,
                                wp_f32 a_min, wp_f32 a_max, struct wp_color );
WORKPHONE_API void wp_fill_triangle( struct wp_command_buffer *, wp_f32 x0, wp_f32 y0, wp_f32 x1,
                                     wp_f32 y1, wp_f32 x2, wp_f32 y2, struct wp_color );
WORKPHONE_API void wp_fill_polygon( struct wp_command_buffer *, const wp_f32 *points, int point_count,
                                    struct wp_color );

/** misc */
WORKPHONE_API void wp_draw_image( struct wp_command_buffer *, struct wp_rect, const struct wp_image *,
                                  struct wp_color );
WORKPHONE_API void wp_draw_nine_slice( struct wp_command_buffer *, struct wp_rect,
                                       const struct wp_nine_slice *, struct wp_color );
WORKPHONE_API void wp_draw_text( struct wp_command_buffer *, struct wp_rect, const char *text, int len,
                                 const struct wp_user_font *, struct wp_color, struct wp_color );
WORKPHONE_API void wp_push_scissor( struct wp_command_buffer *, struct wp_rect );
WORKPHONE_API void wp_push_custom( struct wp_command_buffer *, struct wp_rect,
                                   wp_command_custom_callback, wp_handle usr );

WORKPHONE_API void wp_style_default( struct wp_context * );
WORKPHONE_API void wp_style_from_table( struct wp_context *, const struct wp_color * );
WORKPHONE_API void wp_style_load_cursor( struct wp_context *, wp_cursor_type, const struct wp_cursor * );
WORKPHONE_API void wp_style_load_all_cursors( struct wp_context *, const struct wp_cursor * );
WORKPHONE_API const char *wp_style_get_color_by_name( wp_style_colors );
WORKPHONE_API void wp_style_set_font( struct wp_context *, const struct wp_user_font * );
WORKPHONE_API wp_bool wp_style_set_cursor( struct wp_context *, wp_cursor_type );
WORKPHONE_API void wp_style_show_cursor( struct wp_context * );
WORKPHONE_API void wp_style_hide_cursor( struct wp_context * );

WORKPHONE_API wp_bool wp_style_push_font( struct wp_context *, const struct wp_user_font * );
WORKPHONE_API wp_bool wp_style_push_wp_f32( struct wp_context *, wp_f32 *, wp_f32 );
WORKPHONE_API wp_bool wp_style_push_vec2( struct wp_context *, struct wp_vec2f *, struct wp_vec2f );
WORKPHONE_API wp_bool wp_style_push_style_item( struct wp_context *, struct wp_style_item *,
                                                struct wp_style_item );
WORKPHONE_API wp_bool wp_style_push_flags( struct wp_context *, wp_flags *, wp_flags );
WORKPHONE_API wp_bool wp_style_push_color( struct wp_context *, struct wp_color *, struct wp_color );

WORKPHONE_API wp_bool wp_style_pop_font( struct wp_context * );
WORKPHONE_API wp_bool wp_style_pop_wp_f32( struct wp_context * );
WORKPHONE_API wp_bool wp_style_pop_vec2( struct wp_context * );
WORKPHONE_API wp_bool wp_style_pop_style_item( struct wp_context * );
WORKPHONE_API wp_bool wp_style_pop_flags( struct wp_context * );
WORKPHONE_API wp_bool wp_style_pop_color( struct wp_context * );

WORKPHONE_API wp_flags wp_edit_string( struct wp_context *, wp_flags, char *buffer, wp_s32 *len,
                                       wp_s32 max, wp_plugin_filter );
WORKPHONE_API wp_flags wp_edit_string_zero_terminated( struct wp_context *, wp_flags, char *buffer,
                                                       wp_s32 max, wp_plugin_filter );
WORKPHONE_API wp_flags wp_edit_buffer( struct wp_context *, wp_flags, struct wp_text_edit *,
                                       wp_plugin_filter );
WORKPHONE_API void wp_edit_focus( struct wp_context *, wp_flags flags );
WORKPHONE_API void wp_edit_unfocus( struct wp_context * );

WORKPHONE_API wp_bool wp_chart_begin( struct wp_context *, wp_chart_type, wp_s32 num, wp_f32 min,
                                      wp_f32 max );
WORKPHONE_API wp_bool wp_chart_begin_colored( struct wp_context *, wp_chart_type, struct wp_color,
                                              struct wp_color active, wp_s32 num, wp_f32 min,
                                              wp_f32 max );
WORKPHONE_API void wp_chart_add_slot( struct wp_context *ctx, const wp_chart_type, wp_s32 count,
                                      wp_f32 min_value, wp_f32 max_value );
WORKPHONE_API void wp_chart_add_slot_colored( struct wp_context *ctx, const wp_chart_type,
                                              struct wp_color, struct wp_color active, wp_s32 count,
                                              wp_f32 min_value, wp_f32 max_value );
WORKPHONE_API wp_flags wp_chart_push( struct wp_context *, wp_f32 );
WORKPHONE_API wp_flags wp_chart_push_slot( struct wp_context *, wp_f32, wp_s32 );
WORKPHONE_API void wp_chart_end( struct wp_context * );
WORKPHONE_API void wp_plot( struct wp_context *, wp_chart_type, const wp_f32 *values, wp_s32 count,
                            wp_s32 offset );
WORKPHONE_API void wp_plot_function( struct wp_context *, wp_chart_type, void *userdata,
                                     wp_f32 ( *value_getter )( void *user, wp_s32 index ), wp_s32 count,
                                     wp_s32 offset );
WORKPHONE_API wp_bool wp_popup_begin( struct wp_context *, wp_popup_type, const wp_c8 *, wp_flags,
                                      struct wp_rect bounds );
WORKPHONE_API void wp_popup_close( struct wp_context * );
WORKPHONE_API void wp_popup_end( struct wp_context * );
WORKPHONE_API void wp_popup_get_scroll( const struct wp_context *, wp_u32 *offset_x, wp_u32 *offset_y );
WORKPHONE_API void wp_popup_set_scroll( struct wp_context *, wp_u32 offset_x, wp_u32 offset_y );

WORKPHONE_API wp_s32 wp_combo( struct wp_context *, const wp_c8 *const *items, wp_s32 count,
                               wp_s32 selected, wp_s32 item_height, struct wp_vec2f size );
WORKPHONE_API wp_s32 wp_combo_separator( struct wp_context *, const wp_c8 *items_separated_by_separator,
                                         wp_s32 separator, wp_s32 selected, wp_s32 count,
                                         wp_s32 item_height, struct wp_vec2f size );
WORKPHONE_API wp_s32 wp_combo_string( struct wp_context *, const wp_c8 *items_separated_by_zeros,
                                      wp_s32 selected, wp_s32 count, wp_s32 item_height,
                                      struct wp_vec2f size );
WORKPHONE_API wp_s32 wp_combo_callback( struct wp_context *,
                                        void ( *item_getter )( void *, int, const wp_c8 ** ),
                                        void *userdata, wp_s32 selected, wp_s32 count,
                                        wp_s32 item_height, struct wp_vec2f size );
WORKPHONE_API void wp_combobox( struct wp_context *, const wp_c8 *const *items, wp_s32 count,
                                wp_s32 *selected, wp_s32 item_height, struct wp_vec2f size );
WORKPHONE_API void wp_combobox_string( struct wp_context *, const wp_c8 *items_separated_by_zeros,
                                       wp_s32 *selected, wp_s32 count, wp_s32 item_height,
                                       struct wp_vec2f size );
WORKPHONE_API void wp_combobox_separator( struct wp_context *, const wp_c8 *items_separated_by_separator,
                                          wp_s32 separator, wp_s32 *selected, wp_s32 count,
                                          wp_s32 item_height, struct wp_vec2f size );
WORKPHONE_API void wp_combobox_callback( struct wp_context *,
                                         void ( *item_getter )( void *, int, const wp_c8 ** ), void *,
                                         wp_s32 *selected, wp_s32 count, wp_s32 item_height,
                                         struct wp_vec2f size );

WORKPHONE_API wp_bool wp_combo_begin_text( struct wp_context *, const wp_c8 *selected, int,
                                           struct wp_vec2f size );
WORKPHONE_API wp_bool wp_combo_begin_label( struct wp_context *, const wp_c8 *selected,
                                            struct wp_vec2f size );
WORKPHONE_API wp_bool wp_combo_begin_color( struct wp_context *, struct wp_color color,
                                            struct wp_vec2f size );
WORKPHONE_API wp_bool wp_combo_begin_symbol( struct wp_context *, wp_symbol_type, struct wp_vec2f size );
WORKPHONE_API wp_bool wp_combo_begin_symbol_label( struct wp_context *, const wp_c8 *selected,
                                                   wp_symbol_type, struct wp_vec2f size );
WORKPHONE_API wp_bool wp_combo_begin_symbol_text( struct wp_context *, const wp_c8 *selected, int,
                                                  wp_symbol_type, struct wp_vec2f size );
WORKPHONE_API wp_bool wp_combo_begin_image( struct wp_context *, struct wp_image img,
                                            struct wp_vec2f size );
WORKPHONE_API wp_bool wp_combo_begin_image_label( struct wp_context *, const wp_c8 *selected,
                                                  struct wp_image, struct wp_vec2f size );
WORKPHONE_API wp_bool wp_combo_begin_image_text( struct wp_context *, const wp_c8 *selected, int,
                                                 struct wp_image, struct wp_vec2f size );
WORKPHONE_API wp_bool wp_combo_item_label( struct wp_context *, const wp_c8 *, wp_flags alignment );
WORKPHONE_API wp_bool wp_combo_item_text( struct wp_context *, const wp_c8 *, int, wp_flags alignment );
WORKPHONE_API wp_bool wp_combo_item_image_label( struct wp_context *, struct wp_image, const wp_c8 *,
                                                 wp_flags alignment );
WORKPHONE_API wp_bool wp_combo_item_image_text( struct wp_context *, struct wp_image, const wp_c8 *, int,
                                                wp_flags alignment );
WORKPHONE_API wp_bool wp_combo_item_symbol_label( struct wp_context *, wp_symbol_type, const wp_c8 *,
                                                  wp_flags alignment );
WORKPHONE_API wp_bool wp_combo_item_symbol_text( struct wp_context *, wp_symbol_type, const wp_c8 *, int,
                                                 wp_flags alignment );
WORKPHONE_API void wp_combo_close( struct wp_context * );
WORKPHONE_API void wp_combo_end( struct wp_context * );

WORKPHONE_API wp_bool wp_contextual_begin( struct wp_context *, wp_flags, struct wp_vec2f,
                                           struct wp_rect trigger_bounds );
WORKPHONE_API wp_bool wp_contextual_item_text( struct wp_context *, const wp_c8 *, int, wp_flags align );
WORKPHONE_API wp_bool wp_contextual_item_label( struct wp_context *, const wp_c8 *, wp_flags align );
WORKPHONE_API wp_bool wp_contextual_item_image_label( struct wp_context *, struct wp_image,
                                                      const wp_c8 *, wp_flags alignment );
WORKPHONE_API wp_bool wp_contextual_item_image_text( struct wp_context *, struct wp_image, const wp_c8 *,
                                                     wp_s32 len, wp_flags alignment );
WORKPHONE_API wp_bool wp_contextual_item_symbol_label( struct wp_context *, wp_symbol_type,
                                                       const wp_c8 *, wp_flags alignment );
WORKPHONE_API wp_bool wp_contextual_item_symbol_text( struct wp_context *, wp_symbol_type, const wp_c8 *,
                                                      int, wp_flags alignment );
WORKPHONE_API void wp_contextual_close( struct wp_context * );
WORKPHONE_API void wp_contextual_end( struct wp_context * );

WORKPHONE_API void wp_tooltip( struct wp_context *, const wp_c8 * );

/* =============================================================================
 *
 *                                  RADIO BUTTON
 *
 * ============================================================================= */
WORKPHONE_API wp_bool wp_radio_label( struct wp_context *, const wp_c8 *, wp_bool *active );
WORKPHONE_API wp_bool wp_radio_label_align( struct wp_context *ctx, const wp_c8 *label, wp_bool *active,
                                            wp_flags widget_alignment, wp_flags text_alignment );
WORKPHONE_API wp_bool wp_radio_text( struct wp_context *, const wp_c8 *, int, wp_bool *active );
WORKPHONE_API wp_bool wp_radio_text_align( struct wp_context *ctx, const wp_c8 *text, wp_s32 len,
                                           wp_bool *active, wp_flags widget_alignment,
                                           wp_flags text_alignment );
WORKPHONE_API wp_bool wp_option_label( struct wp_context *, const wp_c8 *, wp_bool active );
WORKPHONE_API wp_bool wp_option_label_align( struct wp_context *ctx, const wp_c8 *label, wp_bool active,
                                             wp_flags widget_alignment, wp_flags text_alignment );
WORKPHONE_API wp_bool wp_option_text( struct wp_context *, const wp_c8 *, int, wp_bool active );
WORKPHONE_API wp_bool wp_option_text_align( struct wp_context *ctx, const wp_c8 *text, wp_s32 len,
                                            wp_bool is_active, wp_flags widget_alignment,
                                            wp_flags text_alignment );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_UI_H_ */
