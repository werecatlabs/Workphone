/**
 * @file wp_graphics_gtao.h
 * @brief Ground-Truth Ambient Occlusion (GTAO) - Horizon-based AO with temporal accumulation.
 *
 * Jimenez GTAO implements the true visibility-arc integral, not a
 * hemisphere-sampling approximation. Two slices x eight steps per frame,
 * with temporal accumulation turning that into the equivalent of ~16 slices
 * without the cost.
 *
 * Reference: Real-Time Rendering, 4th Edition; Jimenez et al. 2016
 */

#ifndef WP_GRAPHICS_GTAO_H
#define WP_GRAPHICS_GTAO_H

#include "workphone_graphics_types.h"

/* =========================================================================
 * Constants
 * ====================================================================== */

/** Number of slices per frame (expandable for quality) */
#define WP_GTAO_SLICES 3

/** Number of steps per slice */
#define WP_GTAO_STEPS 8

/** Minimum radius in pixels */
#define WP_GTAO_MIN_RADIUS_PX 6.0f

/** Maximum radius in pixels */
#define WP_GTAO_MAX_RADIUS_PX 128.0f

/** Default world-space radius */
#define WP_GTAO_DEFAULT_RADIUS 0.9f

/** Default intensity */
#define WP_GTAO_DEFAULT_INTENSITY 1.35f

/** Default thickness */
#define WP_GTAO_DEFAULT_THICKNESS 0.4f

/** Temporal feedback */
#define WP_GTAO_DEFAULT_FEEDBACK 0.92f

/** Blur passes */
#define WP_GTAO_BLUR_RADIUS 3

/* =========================================================================
 * Types
 * ====================================================================== */

/**
 * GTAO configuration
 */
typedef struct wp_gtao_config
{
    /** World-space radius for AO */
    wp_f32 radius;
    /** AO intensity (contrast) */
    wp_f32 intensity;
    /** Thickness parameter for edge detection */
    wp_f32 thickness;
    /** Quality level (0=low, 1=medium, 2=high) */
    wp_s32 quality;
} wp_gtao_config;

/**
 * GTAO context
 */
typedef struct wp_gtao wp_gtao;

/* =========================================================================
 * Functions
 * ====================================================================== */

/**
 * Create a new GTAO context
 *
 * @param width  Render target width
 * @param height Render target height
 * @param config Configuration (can be NULL for defaults)
 * @return New context or NULL on failure
 */
wp_gtao *wp_gtao_create( wp_s32 width, wp_s32 height, const wp_gtao_config *config );

/**
 * Destroy a GTAO context
 *
 * @param ctx Context to destroy
 */
void wp_gtao_destroy( wp_gtao *ctx );

/**
 * Render GTAO
 *
 * Renders the GTAO pass with temporal accumulation.
 *
 * @param ctx          Context
 * @param depth_buffer Linear depth buffer (R32F, view-space meters, positive)
 * @param normal_buffer Octahedral normal buffer (RGB32F)
 * @param proj_inv     Inverse projection matrix
 * @param proj_matrix  Projection matrix (for P11)
 * @param frame        Frame counter
 * @param temporal_on  Enable temporal accumulation
 */
void wp_gtao_render( wp_gtao *ctx, const wp_f32 *depth_buffer, const wp_f32 *normal_buffer,
                     const wp_mat4f *proj_inv, const wp_mat4f *proj_matrix, wp_s32 frame,
                     wp_s32 temporal_on );

/**
 * Get the AO result buffer
 *
 * @param ctx Context
 * @return Pointer to AO buffer (R32F: visibility value)
 */
const wp_f32 *wp_gtao_get_buffer( const wp_gtao *ctx );

/**
 * Get buffer dimensions
 *
 * @param ctx      Context
 * @param width    Output width (can be NULL)
 * @param height   Output height (can be NULL)
 */
void wp_gtao_get_size( const wp_gtao *ctx, wp_s32 *width, wp_s32 *height );

/**
 * Set world-space radius
 *
 * @param ctx    Context
 * @param radius World-space radius in meters
 */
void wp_gtao_set_radius( wp_gtao *ctx, wp_f32 radius );

/**
 * Set AO intensity
 *
 * @param ctx      Context
 * @param intensity AO intensity (contrast curve power)
 */
void wp_gtao_set_intensity( wp_gtao *ctx, wp_f32 intensity );

/**
 * Resize buffers
 *
 * @param ctx    Context
 * @param width  New width
 * @param height New height
 */
void wp_gtao_resize( wp_gtao *ctx, wp_s32 width, wp_s32 height );

/**
 * Sample AO at a UV coordinate
 *
 * @param ctx    Context
 * @param u      U coordinate (0-1)
 * @param v      V coordinate (0-1)
 * @return AO visibility value (0=fully occluded, 1=fully visible)
 */
wp_f32 wp_gtao_sample( const wp_gtao *ctx, wp_f32 u, wp_f32 v );

/**
 * Get the GLSL shader code for GTAO sampling
 *
 * Returns the GLSL functions for GTAO multi-bounce and specular occlusion.
 *
 * @return GLSL shader code string
 */
const wp_c8 *wp_gtao_get_glsl_chunk( void );

/**
 * Reset temporal history
 *
 * Call this when the camera moves significantly.
 *
 * @param ctx Context
 */
void wp_gtao_reset_history( wp_gtao *ctx );

#endif /* WP_GRAPHICS_GTAO_H */
