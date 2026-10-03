#ifndef workphone_graphics_viewport_h__
#define workphone_graphics_viewport_h__

#include "workphone_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Viewport
 * ---------------------------------------------------------------------- */

/**
 * @brief Integer-precision viewport rectangle.
 */
typedef struct
{
    wp_s32 x;      /**< Left edge in pixels.   */
    wp_s32 y;      /**< Top edge in pixels.    */
    wp_s32 width;  /**< Width in pixels.       */
    wp_s32 height; /**< Height in pixels.      */
} wp_viewport_i;

/**
 * @brief Floating-point precision rectangle.
 */
typedef struct
{
    wp_f32 left;
    wp_f32 top;
    wp_f32 right;
    wp_f32 bottom;
} wp_float_rect;

/**
 * @brief Color representation.
 */
typedef struct
{
    wp_f32 r, g, b, a;
} wp_colour_value;

/**
 * @brief Graphics Viewport structure.
 */
typedef struct wp_graphics_viewport
{
    void *camera;
    void *target;
    wp_float_rect rel_rect;
    wp_viewport_i act_rect;
    wp_s32 z_order;
    wp_colour_value back_colour;
    wp_f32 depth_clear_value;
    wp_bool clear_every_frame;
    wp_u32 clear_buffers;
    wp_bool updated;
    wp_bool show_overlays;
    wp_bool show_skies;
    wp_bool show_shadows;
    wp_u32 visibility_mask;
    char *material_scheme_name;
    wp_bool is_auto_updated;
    wp_u32 colour_buffer;
} wp_graphics_viewport;

wp_graphics_viewport *wp_graphics_viewport_create( void *camera, void *target, wp_f32 left, wp_f32 top,
                                                   wp_f32 width, wp_f32 height, wp_s32 z_order );
void wp_graphics_viewport_destroy( wp_graphics_viewport *viewport );
void wp_graphics_viewport_update_dimensions( wp_graphics_viewport *viewport );
void wp_graphics_viewport_update( wp_graphics_viewport *viewport );
void wp_graphics_viewport_clear( wp_graphics_viewport *viewport, wp_u32 buffers, wp_colour_value col,
                                 wp_f32 depth, wp_u16 stencil );
void wp_graphics_viewport_set_camera( wp_graphics_viewport *viewport, void *camera );
void wp_graphics_viewport_set_dimensions( wp_graphics_viewport *viewport, wp_f32 left, wp_f32 top,
                                          wp_f32 width, wp_f32 height );

#ifdef __cplusplus
}
#endif

#endif  // workphone_graphics_viewport_h__
