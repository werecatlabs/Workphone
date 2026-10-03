#ifndef WP_GRAPHICS_HDR_H
#define WP_GRAPHICS_HDR_H

#include "workphone_graphics_types.h"

#define WP_BLOOM_LEVELS 6
#define WP_BLOOM_DEFAULT_THRESHOLD 1.0f
#define WP_BLOOM_DEFAULT_KNEE 0.6f
#define WP_AE_DEFAULT_SKY_WEIGHT 0.15f
#define WP_AE_DEFAULT_FAR_DISTANCE 400.0f
#define WP_AE_SPEED_UP 3.2f
#define WP_AE_SPEED_DOWN 1.4f
#define WP_AE_MIN_EV -4.0f
#define WP_AE_MAX_EV 16.0f
#define WP_LUT_SIZE 33

typedef struct wp_hdr wp_hdr;

wp_hdr *wp_hdr_create( wp_s32 width, wp_s32 height );
void wp_hdr_destroy( wp_hdr *ctx );
const wp_f32 *wp_hdr_bloom_render( wp_hdr *ctx, const wp_f32 *source, wp_s32 width, wp_s32 height );
const wp_f32 *wp_hdr_exposure_update( wp_hdr *ctx, const wp_f32 *source, const wp_f32 *depth,
                                      wp_s32 width, wp_s32 height, wp_f32 dt );
void wp_hdr_composite( wp_hdr *ctx, const wp_f32 *color, const wp_f32 *bloom, const wp_f32 *exposure,
                       wp_s32 width, wp_s32 height, wp_f32 *output );
void wp_hdr_tonemap_agx( wp_f32 r, wp_f32 g, wp_f32 b, wp_f32 *out_r, wp_f32 *out_g, wp_f32 *out_b );
wp_f32 wp_hdr_srgb_to_linear( wp_f32 c );
wp_f32 wp_hdr_linear_to_srgb( wp_f32 c );
void wp_hdr_resize( wp_hdr *ctx, wp_s32 width, wp_s32 height );
void wp_hdr_bloom_set( wp_hdr *ctx, wp_f32 threshold, wp_f32 strength );

#endif
