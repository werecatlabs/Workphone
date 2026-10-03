#ifndef workphone_draw_h__
#define workphone_draw_h__

#include "workphone_vector.h"

struct wp_draw_null_texture
{
    wp_handle texture; /**!< texture handle to a texture with a white pixel */
    struct wp_vec2f uv; /**!< coordinates to a white pixel in the texture  */
};

struct wp_convert_config
{
    wp_f32 global_alpha; /**!< global alpha value */
    enum wp_anti_aliasing
        line_AA; /**!< line anti-aliasing flag can be turned off if you are tight on memory */
    enum wp_anti_aliasing
        shape_AA; /**!< shape anti-aliasing flag can be turned off if you are tight on memory */
    unsigned circle_segment_count;        /**!< number of segments used for circles: default to 22 */
    unsigned arc_segment_count;           /**!< number of segments used for arcs: default to 22 */
    unsigned curve_segment_count;         /**!< number of segments used for curves: default to 22 */
    struct wp_draw_null_texture tex_null; /**!< handle to texture with a white pixel for shape drawing */
    const struct wp_draw_vertex_layout_element
        *vertex_layout;       /**!< describes the vertex output format and packing */
    wp_size vertex_size;      /**!< sizeof one vertex for vertex packing */
    wp_size vertex_alignment; /**!< vertex alignment: Can be obtained by WORKPHONE_ALIGNOF */
};

#ifdef WORKPHONE_UINT_DRAW_INDEX
typedef wp_u32 wp_draw_index;
#else
typedef wp_u16 wp_draw_index;
#endif

enum wp_draw_list_stroke
{
    WORKPHONE_STROKE_OPEN = wp_false, /***< build up path has no connection back to the beginning */
    WORKPHONE_STROKE_CLOSED = wp_true /***< build up path has a connection back to the beginning */
};

enum wp_draw_vertex_layout_attribute
{
    WORKPHONE_VERTEX_POSITION,
    WORKPHONE_VERTEX_COLOR,
    WORKPHONE_VERTEX_TEXCOORD,
    WORKPHONE_VERTEX_ATTRIBUTE_COUNT
};

enum wp_draw_vertex_layout_format
{
    WORKPHONE_FORMAT_SCHAR,
    WORKPHONE_FORMAT_SSHORT,
    WORKPHONE_FORMAT_SINT,
    WORKPHONE_FORMAT_UCHAR,
    WORKPHONE_FORMAT_USHORT,
    WORKPHONE_FORMAT_UINT,
    WORKPHONE_FORMAT_FLOAT,
    WORKPHONE_FORMAT_DOUBLE,

    WORKPHONE_FORMAT_COLOR_BEGIN,
    WORKPHONE_FORMAT_R8G8B8 = WORKPHONE_FORMAT_COLOR_BEGIN,
    WORKPHONE_FORMAT_R16G15B16,
    WORKPHONE_FORMAT_R32G32B32,

    WORKPHONE_FORMAT_R8G8B8A8,
    WORKPHONE_FORMAT_B8G8R8A8,
    WORKPHONE_FORMAT_R16G15B16A16,
    WORKPHONE_FORMAT_R32G32B32A32,
    WORKPHONE_FORMAT_R32G32B32A32_FLOAT,
    WORKPHONE_FORMAT_R32G32B32A32_DOUBLE,

    WORKPHONE_FORMAT_RGB32,
    WORKPHONE_FORMAT_RGBA32,
    WORKPHONE_FORMAT_COLOR_END = WORKPHONE_FORMAT_RGBA32,
    WORKPHONE_FORMAT_COUNT
};

#define WORKPHONE_VERTEX_LAYOUT_END WORKPHONE_VERTEX_ATTRIBUTE_COUNT, WORKPHONE_FORMAT_COUNT, 0

struct wp_draw_vertex_layout_element
{
    enum wp_draw_vertex_layout_attribute attribute;
    enum wp_draw_vertex_layout_format format;
    wp_size offset;
};

struct wp_draw_command
{
    unsigned int elem_count;  /**< number of elements in the current draw batch */
    struct wp_rect clip_rect; /**< current screen clipping rectangle */
    wp_handle texture;        /**< current texture to set */
#ifdef WORKPHONE_INCLUDE_COMMAND_USERDATA
    wp_handle userdata;
#endif
};

#ifdef WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT

struct wp_draw_list
{
    struct wp_rect clip_rect;
    struct wp_vec2f circle_vtx[12];
    struct wp_convert_config config;

    struct wp_buffer *buffer;
    struct wp_buffer *vertices;
    struct wp_buffer *elements;

    unsigned int element_count;
    unsigned int vertex_count;
    unsigned int cmd_count;
    wp_size cmd_offset;

    unsigned int path_count;
    unsigned int path_offset;

    enum wp_anti_aliasing line_AA;
    enum wp_anti_aliasing shape_AA;

#    ifdef WORKPHONE_INCLUDE_COMMAND_USERDATA
    wp_handle userdata;
#    endif
};
#endif

/* triangle */
WORKPHONE_API void wp_triangle_from_direction( struct wp_vec2f *result, struct wp_rect r, wp_f32 pad_x,
                                               wp_f32 pad_y, enum wp_heading direction );

/* draw */
WORKPHONE_API void wp_command_buffer_init( struct wp_command_buffer *cb, struct wp_buffer *b,
                                           enum wp_command_clipping clip );
WORKPHONE_API void wp_command_buffer_reset( struct wp_command_buffer *b );
WORKPHONE_API void *wp_command_buffer_push( struct wp_command_buffer *b, enum wp_command_type t,
                                            wp_size size );
WORKPHONE_API void wp_draw_symbol( struct wp_command_buffer *out, enum wp_symbol_type type,
                                   struct wp_rect content, struct wp_color background,
                                   struct wp_color foreground, wp_f32 border_width,
                                   const struct wp_user_font *font );

/* buffering */
WORKPHONE_API void wp_start_buffer( struct wp_context *ctx, struct wp_command_buffer *b );
WORKPHONE_API void wp_start( struct wp_context *ctx, struct wp_window *win );
WORKPHONE_API void wp_start_popup( struct wp_context *ctx, struct wp_window *win );
WORKPHONE_API void wp_finish_popup( struct wp_context *ctx, struct wp_window * );
WORKPHONE_API void wp_finish_buffer( struct wp_context *ctx, struct wp_command_buffer *b );
WORKPHONE_API void wp_finish( struct wp_context *ctx, struct wp_window *w );
WORKPHONE_API void wp_build( struct wp_context *ctx );

/* draw list */
WORKPHONE_API void wp_draw_list_init( struct wp_draw_list * );
WORKPHONE_API void wp_draw_list_setup( struct wp_draw_list *, const struct wp_convert_config *,
                                       struct wp_buffer *cmds, struct wp_buffer *vertices,
                                       struct wp_buffer *elements, enum wp_anti_aliasing line_aa,
                                       enum wp_anti_aliasing shape_aa );

/* drawing */
#define wp_draw_list_foreach( cmd, can, b ) \
    for( ( cmd ) = wp__draw_list_begin( can, b ); ( cmd ) != 0; \
         ( cmd ) = wp__draw_list_next( cmd, b, can ) )

#define wp_draw_foreach( cmd, ctx, b ) \
    for( ( cmd ) = wp__draw_begin( ctx, b ); ( cmd ) != 0; ( cmd ) = wp__draw_next( cmd, b, ctx ) )

WORKPHONE_API const struct wp_draw_command *wp__draw_list_begin( const struct wp_draw_list *,
                                                                 const struct wp_buffer * );
WORKPHONE_API const struct wp_draw_command *wp__draw_list_next( const struct wp_draw_command *,
                                                                const struct wp_buffer *,
                                                                const struct wp_draw_list * );
WORKPHONE_API const struct wp_draw_command *wp__draw_list_end( const struct wp_draw_list *,
                                                               const struct wp_buffer * );

WORKPHONE_API const struct wp_draw_command *wp__draw_begin( const struct wp_context *,
                                                            const struct wp_buffer * );
WORKPHONE_API const struct wp_draw_command *wp__draw_next( const struct wp_draw_command *,
                                                           const struct wp_buffer *,
                                                           const struct wp_context * );
WORKPHONE_API const struct wp_draw_command *wp__draw_end( const struct wp_context *,
                                                          const struct wp_buffer * );

/* ===============================================================
 *
 *                          DRAW LIST
 *
 * ===============================================================*/
#ifdef WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT
/**
 * \page "Draw List"
 * The optional vertex buffer draw list provides a 2D drawing context
 * with antialiasing functionality which takes basic filled or outlined shapes
 * or a path and outputs vertexes, elements and draw commands.
 * The actual draw list API is not required to be used directly while using this
 * library since converting the default library draw command output is done by
 * just calling `wp_convert` but I decided to still make this library accessible
 * since it can be useful.
 *
 * The draw list is based on a path buffering and polygon and polyline
 * rendering API which allows a lot of ways to draw 2D content to screen.
 * In fact it is probably more powerful than needed but allows even more crazy
 * things than this library provides by default.
 */

/* path */
WORKPHONE_API void wp_draw_list_path_clear( struct wp_draw_list * );
WORKPHONE_API void wp_draw_list_path_line_to( struct wp_draw_list *, struct wp_vec2f pos );
WORKPHONE_API void wp_draw_list_path_arc_to_fast( struct wp_draw_list *, struct wp_vec2f center,
                                                  wp_f32 radius, int a_min, int a_max );
WORKPHONE_API void wp_draw_list_path_arc_to( struct wp_draw_list *, struct wp_vec2f center, wp_f32 radius,
                                             wp_f32 a_min, wp_f32 a_max, unsigned int segments );
WORKPHONE_API void wp_draw_list_path_rect_to( struct wp_draw_list *, struct wp_vec2f a, struct wp_vec2f b,
                                              wp_f32 rounding );
WORKPHONE_API void wp_draw_list_path_curve_to( struct wp_draw_list *, struct wp_vec2f p2,
                                               struct wp_vec2f p3, struct wp_vec2f p4,
                                               unsigned int num_segments );
WORKPHONE_API void wp_draw_list_path_fill( struct wp_draw_list *, struct wp_color );
WORKPHONE_API void wp_draw_list_path_stroke( struct wp_draw_list *, struct wp_color,
                                             enum wp_draw_list_stroke closed, wp_f32 thickness );

/* stroke */
WORKPHONE_API void wp_draw_list_stroke_line( struct wp_draw_list *, struct wp_vec2f a, struct wp_vec2f b,
                                             struct wp_color, wp_f32 thickness );
WORKPHONE_API void wp_draw_list_stroke_rect( struct wp_draw_list *, struct wp_rect rect, struct wp_color,
                                             wp_f32 rounding, wp_f32 thickness );
WORKPHONE_API void wp_draw_list_stroke_triangle( struct wp_draw_list *, struct wp_vec2f a,
                                                 struct wp_vec2f b, struct wp_vec2f c, struct wp_color,
                                                 wp_f32 thickness );
WORKPHONE_API void wp_draw_list_stroke_circle( struct wp_draw_list *, struct wp_vec2f center,
                                               wp_f32 radius, struct wp_color, unsigned int segs,
                                               wp_f32 thickness );
WORKPHONE_API void wp_draw_list_stroke_curve( struct wp_draw_list *, struct wp_vec2f p0,
                                              struct wp_vec2f cp0, struct wp_vec2f cp1, struct wp_vec2f p1,
                                              struct wp_color, unsigned int segments, wp_f32 thickness );
WORKPHONE_API void wp_draw_list_stroke_poly_line( struct wp_draw_list *, const struct wp_vec2f *pnts,
                                                  const unsigned int cnt, struct wp_color,
                                                  enum wp_draw_list_stroke, wp_f32 thickness,
                                                  enum wp_anti_aliasing );

/* fill */
WORKPHONE_API void wp_draw_list_fill_rect( struct wp_draw_list *, struct wp_rect rect, struct wp_color,
                                           wp_f32 rounding );
WORKPHONE_API void wp_draw_list_fill_rect_multi_color( struct wp_draw_list *, struct wp_rect rect,
                                                       struct wp_color left, struct wp_color top,
                                                       struct wp_color bottom, struct wp_color right );
WORKPHONE_API void wp_draw_list_fill_triangle( struct wp_draw_list *, struct wp_vec2f a, struct wp_vec2f b,
                                               struct wp_vec2f c, struct wp_color );
WORKPHONE_API void wp_draw_list_fill_circle( struct wp_draw_list *, struct wp_vec2f center, wp_f32 radius,
                                             struct wp_color col, unsigned int segs );
WORKPHONE_API void wp_draw_list_fill_poly_convex( struct wp_draw_list *, const struct wp_vec2f *points,
                                                  const unsigned int count, struct wp_color,
                                                  enum wp_anti_aliasing );

/* misc */
WORKPHONE_API void wp_draw_list_add_image( struct wp_draw_list *, struct wp_image texture,
                                           struct wp_rect rect, struct wp_color );
WORKPHONE_API void wp_draw_list_add_text( struct wp_draw_list *, const struct wp_user_font *,
                                          struct wp_rect, const char *text, int len, wp_f32 font_height,
                                          struct wp_color );
#    ifdef WORKPHONE_INCLUDE_COMMAND_USERDATA
WORKPHONE_API void wp_draw_list_push_userdata( struct wp_draw_list *, wp_handle userdata );
#    endif

#endif

/* Convert UI commands to vertex/index buffers */
WORKPHONE_API wp_flags wp_convert( struct wp_context *, struct wp_buffer *cmds,
                                   struct wp_buffer *vertices, struct wp_buffer *elements,
                                   const struct wp_convert_config * );

#endif  // workphone_draw_h__
