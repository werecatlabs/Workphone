/**
 * @file wp_graphics_pipeline.h
 * @brief Experimental CPU post-processing pipeline.
 *
 * This module provides a deterministic CPU reference implementation of modern
 * post-processing techniques. It is not a replacement for a GPU frame graph;
 * callers must supply real frame buffers and present the returned result.
 *
 * Material lighting, including PBR, belongs to the renderer that supplies the
 * input color buffer; this module does not shade scene geometry or materials.
 *
 * Features:
 * - Cascaded Shadow Maps (CSM) with PCSS
 * - Ground-Truth Ambient Occlusion (GTAO)
 * - Screen Space Reflections (SSR)
 * - Contact Shadows
 * - Temporal Anti-Aliasing (TAA)
 * - Motion Blur
 * - Depth of Field
 * - HDR Bloom with Karis pyramid
 * - Auto Exposure with EV100
 * - AgX Filmic Tone Mapping
 * - Procedural Color Grading LUT
 *
 * Reference: G:\Claude-of-Duty-main\Claude-of-Duty-main\src\render\*
 */

#ifndef WP_GRAPHICS_PIPELINE_H
#define WP_GRAPHICS_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "workphone_graphics_types.h"

/* =========================================================================
 * Include all pipeline subsystems
 * ====================================================================== */

#include "workphone_graphics_gbuffer.h"
#include "workphone_graphics_csm.h"
#include "workphone_graphics_gtao.h"
#include "workphone_graphics_ssr.h"
#include "workphone_graphics_contact_shadows.h"
#include "workphone_graphics_taa.h"
#include "workphone_graphics_motion_blur.h"
#include "workphone_graphics_dof.h"
#include "workphone_graphics_hdr.h"

/* =========================================================================
 * Master Pipeline
 * ====================================================================== */

/**
 * CPU post-processing pipeline context.
 */
typedef struct wp_render_pipeline wp_render_pipeline;

/**
 * Quality presets
 */
typedef enum
{
    WP_PIPELINE_QUALITY_LOW = 0,
    WP_PIPELINE_QUALITY_MEDIUM = 1,
    WP_PIPELINE_QUALITY_HIGH = 2,
    WP_PIPELINE_QUALITY_ULTRA = 3,
    WP_PIPELINE_QUALITY_CINEMATIC = 4
} wp_pipeline_quality;

/**
 * Create render pipeline
 *
 * @param width Screen width
 * @param height Screen height
 * @param quality Quality preset
 * @return Pipeline context or NULL
 */
wp_render_pipeline *wp_pipeline_create( wp_s32 width, wp_s32 height, wp_pipeline_quality quality );

/**
 * Destroy render pipeline
 *
 * @param pipeline Pipeline to destroy
 */
void wp_pipeline_destroy( wp_render_pipeline *pipeline );

/**
 * Render complete frame through pipeline
 *
 * This is the main entry point for pipeline rendering.
 *
 * @param pipeline pipeline
 * @param scene Scene to render
 * @param camera Camera parameters
 * @param lights Light array
 * @param light_count Number of lights
 * @param out_color Output color buffer (HDR RGB32F)
 */
void wp_pipeline_render_frame( wp_render_pipeline *pipeline, void *scene, const wp_mat4f *view,
                          const wp_mat4f *proj, void *lights, wp_s32 light_count, wp_f32 *out_color );

/**
 * Get final post-processed output
 *
 * @param pipeline pipeline
 * @param out_width Output width
 * @param out_height Output height
 * @return Final color buffer (sRGB LDR)
 */
const wp_f32 *wp_pipeline_get_output( const wp_render_pipeline *pipeline, wp_s32 *out_width,
                                 wp_s32 *out_height );

/**
 * Resize pipeline
 *
 * @param pipeline pipeline
 * @param width New width
 * @param height New height
 */
void wp_pipeline_resize( wp_render_pipeline *pipeline, wp_s32 width, wp_s32 height );

/**
 * Set view-projection matrices for TAA
 *
 * @param pipeline pipeline
 * @param curr_vp Current view-projection matrix
 * @param prev_vp Previous view-projection matrix
 * @param inv_vp Inverse view-projection matrix
 */
void wp_pipeline_set_view_proj( wp_render_pipeline *pipeline, const wp_mat4f *curr_vp, const wp_mat4f *prev_vp,
                           const wp_mat4f *inv_vp );

/**
 * Supply the CPU frame buffers consumed by the post-processing pipeline.
 * Every non-NULL buffer is copied, so the caller retains ownership.  Color is
 * linear HDR RGB32F, normals are XYZA32F, depth is R32F and velocity is RG32F.
 */
void wp_pipeline_set_frame_inputs( wp_render_pipeline *pipeline, const wp_f32 *hdr_color, const wp_f32 *normal,
                              const wp_f32 *depth, const wp_f32 *velocity );

/** Read-only access to the frame inputs used by diagnostics and adapters. */
const wp_f32 *wp_pipeline_get_gbuffer_normal( const wp_render_pipeline *pipeline );
const wp_f32 *wp_pipeline_get_gbuffer_depth( const wp_render_pipeline *pipeline );
const wp_f32 *wp_pipeline_get_gbuffer_velocity( const wp_render_pipeline *pipeline );

/* =========================================================================
 * Individual effect toggles
 * ====================================================================== */

/**
 * Enable/disable individual post-processing effects
 */
void wp_pipeline_enable_shadows( wp_render_pipeline *pipeline, wp_s32 enabled );
void wp_pipeline_enable_ao( wp_render_pipeline *pipeline, wp_s32 enabled );
void wp_pipeline_enable_ssr( wp_render_pipeline *pipeline, wp_s32 enabled );
void wp_pipeline_enable_contact_shadows( wp_render_pipeline *pipeline, wp_s32 enabled );
void wp_pipeline_enable_taa( wp_render_pipeline *pipeline, wp_s32 enabled );
void wp_pipeline_enable_motion_blur( wp_render_pipeline *pipeline, wp_s32 enabled );
void wp_pipeline_enable_dof( wp_render_pipeline *pipeline, wp_s32 enabled );
void wp_pipeline_enable_bloom( wp_render_pipeline *pipeline, wp_s32 enabled );
void wp_pipeline_enable_exposure( wp_render_pipeline *pipeline, wp_s32 enabled );

/* =========================================================================
 * Quality settings
 * ====================================================================== */

/**
 * Set shadow quality
 *
 * @param pipeline pipeline
 * @param cascades Number of cascades (1-4)
 * @param map_size Shadow map size (512-4096)
 */
void wp_pipeline_set_shadow_quality( wp_render_pipeline *pipeline, wp_s32 cascades, wp_s32 map_size );

/**
 * Set SSAO parameters
 *
 * @param pipeline pipeline
 * @param radius AO radius in meters
 * @param intensity AO intensity
 */
void wp_pipeline_set_ao_params( wp_render_pipeline *pipeline, wp_f32 radius, wp_f32 intensity );

/**
 * Set TAA parameters
 *
 * @param pipeline pipeline
 * @param feedback History blend factor
 */
void wp_pipeline_set_taa_params( wp_render_pipeline *pipeline, wp_f32 feedback );

/**
 * Set bloom parameters
 *
 * @param pipeline pipeline
 * @param threshold Bloom threshold
 * @param strength Bloom strength
 */
void wp_pipeline_set_bloom_params( wp_render_pipeline *pipeline, wp_f32 threshold, wp_f32 strength );

/* =========================================================================
 * Debug views
 * ====================================================================== */

/**
 * Debug visualization modes
 */
typedef enum
{
    WP_PIPELINE_DEBUG_NONE = 0,
    WP_PIPELINE_DEBUG_NORMALS,
    WP_PIPELINE_DEBUG_DEPTH,
    WP_PIPELINE_DEBUG_VELOCITY,
    WP_PIPELINE_DEBUG_AO,
    WP_PIPELINE_DEBUG_SSR,
    WP_PIPELINE_DEBUG_BLOOM,
    WP_PIPELINE_DEBUG_EXPOSURE
} wp_render_debug_view;

/**
 * Set debug view
 *
 * @param pipeline pipeline
 * @param mode Debug mode
 */
void wp_pipeline_set_debug_view( wp_render_pipeline *pipeline, wp_render_debug_view mode );

#ifdef __cplusplus
}
#endif

#endif /* WP_GRAPHICS_PIPELINE_H */
