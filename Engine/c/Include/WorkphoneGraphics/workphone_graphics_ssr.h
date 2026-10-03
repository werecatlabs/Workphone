/**
 * @file wp_graphics_ssr.h
 * @brief Screen Space Reflections - Ray marched reflections with temporal reprojection
 */

#ifndef WP_GRAPHICS_SSR_H
#define WP_GRAPHICS_SSR_H

#include "workphone_graphics_types.h"

#define WP_SSR_STEPS 28
#define WP_SSR_REFINE 5
#define WP_SSR_DEFAULT_DIST 24.0f
#define WP_SSR_DEFAULT_THICKNESS 0.6f
#define WP_SSR_DEFAULT_STRENGTH 1.0f

typedef struct wp_ssr wp_ssr;

wp_ssr *wp_ssr_create( wp_s32 width, wp_s32 height );
void wp_ssr_destroy( wp_ssr *ctx );
void wp_ssr_render( wp_ssr *ctx, const wp_f32 *color, const wp_f32 *depth, const wp_f32 *normal,
                    const wp_f32 *velocity, const wp_mat4f *proj, const wp_mat4f *proj_inv,
                    wp_s32 frame );
const wp_f32 *wp_ssr_get_texture( const wp_ssr *ctx );
void wp_ssr_resize( wp_ssr *ctx, wp_s32 width, wp_s32 height );

#endif
