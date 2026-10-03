/**
 * @file wp_graphics_gbuffer.h
 * @brief GBuffer system for deferred rendering prepass.
 *
 * Implements the depth/normal/velocity prepass that outputs:
 * - MRT 0: RGBA16F - Octahedral view-space normal (xy), coverage (z), material_id (w)
 * - MRT 1: RG16F   - Screen-space velocity as UV delta
 * - MRT 2: R32F    - Linear view depth in meters
 *
 * Coverage distinguishes static geometry (1.0) from skinned/morphed (0.7).
 * TAA uses this to reject history on pixels whose motion cannot be described
 * by matrix difference alone.
 *
 * Reference: G:\Claude-of-Duty-main\Claude-of-Duty-main\src\render\prepass.js
 */

#ifndef WP_GRAPHICS_GBUFFER_H
#define WP_GRAPHICS_GBUFFER_H

#include "workphone_graphics_types.h"

/* =========================================================================
 * Constants
 * ====================================================================== */

/** Coverage value for skinned/morphed geometry (vertex-deforming) */
#define WP_GBUFFER_COVERAGE_DYNAMIC 0.7f

/** Coverage value for static geometry */
#define WP_GBUFFER_COVERAGE_STATIC 1.0f

/** Coverage threshold for dynamic detection */
#define WP_GBUFFER_COVERAGE_THRESHOLD 0.5f

/** Maximum objects tracked for velocity */
#define WP_GBUFFER_MAX_OBJECTS 8192

/** Object tracking cleanup threshold multiplier */
#define WP_GBUFFER_CLEANUP_MULTIPLIER 2

/** Cleanup buffer margin */
#define WP_GBUFFER_CLEANUP_MARGIN 64

/* =========================================================================
 * Types
 * ====================================================================== */

/**
 * GBuffer render targets
 */
typedef struct wp_gbuffer
{
    /** Render width */
    wp_s32 width;
    /** Render height */
    wp_s32 height;

    /** Normal buffer (RGBA16F) - xy: octahedral normal, z: coverage, w: material_id */
    wp_f32 *normal_buffer;
    /** Velocity buffer (RG16F) - xy: screen-space UV velocity delta */
    wp_f32 *velocity_buffer;
    /** Depth buffer (R32F) - linear view depth in meters */
    wp_f32 *depth_buffer;

    /** Previous frame object matrices */
    wp_mat4f *prev_matrices;
    /** Current frame object matrices */
    wp_mat4f *curr_matrices;
    /** Object IDs for tracking */
    wp_s32 *object_ids;
    /** Number of tracked objects */
    wp_s32 object_count;

    /** Previous view-projection matrix */
    wp_mat4f prev_vp;
    /** Current view-projection matrix */
    wp_mat4f curr_vp;

    /** Internal tracking set */
    wp_s32 *seen_ids;
    wp_s32 seen_count;
} wp_gbuffer;

/* =========================================================================
 * Functions
 * ====================================================================== */

/**
 * Create a new GBuffer context
 *
 * @param width  Render target width
 * @param height Render target height
 * @return New context or NULL on failure
 */
wp_gbuffer *wp_gbuffer_create( wp_s32 width, wp_s32 height );

/**
 * Destroy a GBuffer context
 *
 * @param ctx Context to destroy
 */
void wp_gbuffer_destroy( wp_gbuffer *ctx );

/**
 * Resize GBuffer textures
 *
 * @param ctx    Context
 * @param width  New width
 * @param height New height
 */
void wp_gbuffer_resize( wp_gbuffer *ctx, wp_s32 width, wp_s32 height );

/**
 * Begin recording object matrices for velocity tracking
 *
 * Call this at the start of each frame.
 *
 * @param ctx GBuffer context
 */
void wp_gbuffer_begin_record( wp_gbuffer *ctx );

/**
 * Record an object's current world matrix
 *
 * Call for each visible object. The previous matrix will be stored
 * and used for velocity computation next frame.
 *
 * @param ctx       GBuffer context
 * @param object_id Unique object identifier
 * @param matrix    Current world matrix
 */
void wp_gbuffer_record_matrix( wp_gbuffer *ctx, wp_s32 object_id, const wp_mat4f *matrix );

/**
 * End recording phase
 *
 * Cleans up stale object entries.
 *
 * @param ctx GBuffer context
 */
void wp_gbuffer_end_record( wp_gbuffer *ctx );

/**
 * Get the previous world matrix for an object
 *
 * @param ctx       GBuffer context
 * @param object_id Object identifier
 * @param out_mat   Output matrix (filled with identity if not found)
 */
void wp_gbuffer_get_prev_matrix( wp_gbuffer *ctx, wp_s32 object_id, wp_mat4f *out_mat );

/**
 * Update view-projection matrices
 *
 * @param ctx      GBuffer context
 * @param curr_vp  Current view-projection matrix
 * @param prev_vp  Previous view-projection matrix
 */
void wp_gbuffer_set_view_proj( wp_gbuffer *ctx, const wp_mat4f *curr_vp, const wp_mat4f *prev_vp );

/**
 * Render the GBuffer prepass
 *
 * This is a software fallback that fills the buffers with default values.
 * In a real implementation, this would be called from the GPU render path.
 *
 * @param ctx       GBuffer context
 * @param scene     Scene data (geometry, transforms)
 * @param camera    Camera data
 * @param clear     Clear buffers before rendering
 */
void wp_gbuffer_render( wp_gbuffer *ctx, void *scene, void *camera, wp_s32 clear );

/**
 * Clear GBuffer contents
 *
 * @param ctx  GBuffer context
 * @param full Full clear (true) or depth only (false)
 */
void wp_gbuffer_clear( wp_gbuffer *ctx, wp_s32 full );

/**
 * Get the normal buffer
 *
 * Format: RGBA16F
 *   - xy: Octahedral encoded view-space normal
 *   - z:  Coverage (1.0=static, 0.7=skinned)
 *   - w:  Material ID
 *
 * @param ctx GBuffer context
 * @return Pointer to normal buffer (RGBA32F layout)
 */
const wp_f32 *wp_gbuffer_get_normal_buffer( const wp_gbuffer *ctx );

/**
 * Get the velocity buffer
 *
 * Format: RG16F
 *   - xy: Screen-space UV velocity delta (current - previous)
 *
 * @param ctx GBuffer context
 * @return Pointer to velocity buffer (RG32F layout)
 */
const wp_f32 *wp_gbuffer_get_velocity_buffer( const wp_gbuffer *ctx );

/**
 * Get the depth buffer
 *
 * Format: R32F
 *   - x: Linear view depth in meters (positive)
 *
 * @param ctx GBuffer context
 * @return Pointer to depth buffer (R32F layout)
 */
const wp_f32 *wp_gbuffer_get_depth_buffer( const wp_gbuffer *ctx );

/* =========================================================================
 * Utility Functions
 * ====================================================================== */

/**
 * Decode octahedral normal to 3D vector
 *
 * @param encoded  Encoded normal (xy from gbuffer)
 * @param out_x    Output X component
 * @param out_y    Output Y component
 * @param out_z    Output Z component
 */
void wp_gbuffer_decode_normal( wp_f32 encoded_x, wp_f32 encoded_y, wp_f32 *out_x, wp_f32 *out_y,
                               wp_f32 *out_z );

/**
 * Encode 3D normal to octahedral
 *
 * @param nx   Normal X
 * @param ny   Normal Y
 * @param nz   Normal Z
 * @param out_x Output encoded X
 * @param out_y Output encoded Y
 */
void wp_gbuffer_encode_normal( wp_f32 nx, wp_f32 ny, wp_f32 nz, wp_f32 *out_x, wp_f32 *out_y );

/**
 * Get coverage from normal buffer sample
 *
 * @param normal_sample RGBA normal buffer sample
 * @return Coverage value
 */
wp_f32 wp_gbuffer_get_coverage( const wp_f32 *normal_sample );

/**
 * Get material ID from normal buffer sample
 *
 * @param normal_sample RGBA normal buffer sample
 * @return Material ID
 */
wp_f32 wp_gbuffer_get_material_id( const wp_f32 *normal_sample );

/**
 * Check if a pixel is from dynamic (skinned) geometry
 *
 * @param normal_sample RGBA normal buffer sample
 * @return 1 if dynamic, 0 if static
 */
wp_s32 wp_gbuffer_is_dynamic( const wp_f32 *normal_sample );

#endif /* WP_GRAPHICS_GBUFFER_H */
