#ifndef workphone_command_buffer_h__
#define workphone_command_buffer_h__

#include "workphone_math.h"

struct wp_command_buffer
{
    struct wp_buffer *base;
    wp_s32 use_clipping;
    wp_size begin;
    wp_size end;
    wp_size last;
    struct wp_rect clip;
#ifdef WORKPHONE_INCLUDE_COMMAND_USERDATA
    wp_handle userdata;
#endif
};

/** shape outlines */
WORKPHONE_API void wp_stroke_line( struct wp_command_buffer *b, float x0, float y0, float x1, float y1,
                                   float line_thickness, struct wp_color );
WORKPHONE_API void wp_stroke_curve( struct wp_command_buffer *, float, float, float, float, float, float,
                                    float, float, float line_thickness, struct wp_color );
WORKPHONE_API void wp_stroke_rect( struct wp_command_buffer *, struct wp_rect, float rounding,
                                   float line_thickness, struct wp_color );
WORKPHONE_API void wp_stroke_circle( struct wp_command_buffer *, struct wp_rect, float line_thickness,
                                     struct wp_color );
WORKPHONE_API void wp_stroke_arc( struct wp_command_buffer *, float cx, float cy, float radius,
                                  float a_min, float a_max, float line_thickness, struct wp_color );
WORKPHONE_API void wp_stroke_triangle( struct wp_command_buffer *, float, float, float, float, float,
                                       float, float line_thichness, struct wp_color );
WORKPHONE_API void wp_stroke_polyline( struct wp_command_buffer *, const float *points, int point_count,
                                       float line_thickness, struct wp_color col );
WORKPHONE_API void wp_stroke_polygon( struct wp_command_buffer *, const float *points, int point_count,
                                      float line_thickness, struct wp_color );

/** filled shades */
WORKPHONE_API void wp_fill_rect( struct wp_command_buffer *, struct wp_rect, float rounding,
                                 struct wp_color );
WORKPHONE_API void wp_fill_rect_multi_color( struct wp_command_buffer *, struct wp_rect,
                                             struct wp_color left, struct wp_color top,
                                             struct wp_color right, struct wp_color bottom );
WORKPHONE_API void wp_fill_circle( struct wp_command_buffer *, struct wp_rect, struct wp_color );
WORKPHONE_API void wp_fill_arc( struct wp_command_buffer *, float cx, float cy, float radius,
                                float a_min, float a_max, struct wp_color );
WORKPHONE_API void wp_fill_triangle( struct wp_command_buffer *, float x0, float y0, float x1, float y1,
                                     float x2, float y2, struct wp_color );
WORKPHONE_API void wp_fill_polygon( struct wp_command_buffer *, const float *points, int point_count,
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

#endif  // workphone_command_buffer_h__
