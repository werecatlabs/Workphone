/**
 * @file wp_graphics_csm.h
 * @brief Cascaded Shadow Maps - Multi-cascade directional shadows with PCSS
 */

#ifndef WP_GRAPHICS_CSM_H
#define WP_GRAPHICS_CSM_H

#include "workphone_graphics_types.h"

#define WP_CSM_CASCADES 4
#define WP_CSM_MAX_SPLITS 4
#define WP_CSM_BLOCKER_TAPS 16
#define WP_CSM_PCF_TAPS 20

typedef struct wp_csm wp_csm;

wp_csm *wp_csm_create( wp_s32 map_size );
void wp_csm_destroy( wp_csm *ctx );
void wp_csm_update( wp_csm *ctx, const wp_mat4f *view, const wp_mat4f *proj, const wp_vec3f *light_dir );
void wp_csm_render( wp_csm *ctx, void *scene );
const wp_f32 *wp_csm_get_texture( const wp_csm *ctx );
void wp_csm_get_matrices( const wp_csm *ctx, wp_mat4f *out_matrices );
void wp_csm_get_splits( const wp_csm *ctx, wp_f32 *out_splits );
void wp_csm_resize( wp_csm *ctx, wp_s32 map_size );

#endif
