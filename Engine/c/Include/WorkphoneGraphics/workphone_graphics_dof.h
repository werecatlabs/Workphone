/**
 * @file wp_graphics_dof.h
 * @brief Depth of Field - Bokeh DOF with scatter-as-gather
 */

#ifndef WP_GRAPHICS_DOF_H
#define WP_GRAPHICS_DOF_H

#include "workphone_graphics_types.h"

#define WP_DOF_TAPS 32
#define WP_DOF_DEFAULT_COC 5.0f
#define WP_DOF_DEFAULT_FAR_START 1.2f
#define WP_DOF_DEFAULT_NEAR_RATIO 0.6f

typedef struct wp_dof wp_dof;

wp_dof *wp_dof_create( wp_s32 width, wp_s32 height );
void wp_dof_destroy( wp_dof *ctx );
void wp_dof_render( wp_dof *ctx, const wp_f32 *color, const wp_f32 *depth, wp_s32 frame,
                    wp_f32 focus_dist );
const wp_f32 *wp_dof_get_texture( const wp_dof *ctx );
void wp_dof_resize( wp_dof *ctx, wp_s32 width, wp_s32 height );

#endif
