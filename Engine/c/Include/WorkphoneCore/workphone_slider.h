#ifndef workphone_slider_h__
#define workphone_slider_h__

#include "workphone_prerequisites.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Slider widget — floating-point variant.
 *
 * Emits and updates *value in-place. Returns wp_true if the value changed this frame.
 *
 * @param ctx        Active context (must be inside a window/panel).
 * @param min_value  Minimum slider value.
 * @param value      In/out pointer to the current value.
 * @param max_value  Maximum slider value.
 * @param value_step Step size per drag unit.
 */
WORKPHONE_API wp_bool wp_slider_float( struct wp_context *ctx, wp_f32 min_value, wp_f32 *value,
                                       wp_f32 max_value, wp_f32 value_step );

/**
 * Slider widget — floating-point, returns new value directly.
 */
WORKPHONE_API wp_f32 wp_slide_wp_f32( struct wp_context *ctx, wp_f32 min, wp_f32 val, wp_f32 max,
                                      wp_f32 step );

/**
 * Slider widget — integer, returns new value directly.
 */
WORKPHONE_API wp_s32 wp_slide_int( struct wp_context *ctx, wp_s32 min, wp_s32 val, wp_s32 max,
                                   wp_s32 step );

/**
 * Slider widget — integer variant.
 *
 * Emits and updates *val in-place. Returns wp_true if the value changed this frame.
 */
WORKPHONE_API wp_bool wp_slider_int( struct wp_context *ctx, wp_s32 min, wp_s32 *val, wp_s32 max,
                                     wp_s32 step );

#ifdef __cplusplus
}
#endif

#endif  // workphone_slider_h__
