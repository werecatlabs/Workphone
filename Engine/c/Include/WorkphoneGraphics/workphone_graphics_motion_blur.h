/**
 * @file wp_graphics_motion_blur.h
 * @brief Motion Blur - Velocity-based tile blur with depth weighting
 */

#ifndef WP_GRAPHICS_MOTION_BLUR_H
#define WP_GRAPHICS_MOTION_BLUR_H

#include "workphone_graphics_types.h"

#define WP_MB_TAPS 12
#define WP_MB_DEFAULT_SHUTTER 0.5f
#define WP_MB_DEFAULT_MAX_RADIUS 48.0f
#define WP_MB_DEFAULT_INTENSITY 1.0f

typedef struct wp_motion_blur wp_motion_blur;

wp_motion_blur *wp_mb_create( wp_s32 width, wp_s32 height );
void wp_mb_destroy( wp_motion_blur *ctx );
void wp_mb_render( wp_motion_blur *ctx, const wp_f32 *color, const wp_f32 *velocity, const wp_f32 *depth,
                   const wp_f32 *normal, wp_s32 frame, wp_f32 shutter );
const wp_f32 *wp_mb_get_texture( const wp_motion_blur *ctx );
void wp_mb_resize( wp_motion_blur *ctx, wp_s32 width, wp_s32 height );

#endif
