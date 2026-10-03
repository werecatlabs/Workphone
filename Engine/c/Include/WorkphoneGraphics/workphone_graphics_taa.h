/**
 * @file wp_graphics_taa.h
 * @brief Temporal Anti-Aliasing (TAA) - Velocity reprojection with YCoCg variance clipping.
 *
 * TAA uses velocity-based history reprojection with:
 * - Halton sequence sub-pixel jitter
 * - Catmull-Rom filtered history resampling
 * - YCoCg color space for chroma-aware variance clipping
 * - Depth-based velocity dilation for better silhouette handling
 *
 * Reference: Jimenez et al. - Filmic SMAA
 */

#ifndef WP_GRAPHICS_TAA_H
#define WP_GRAPHICS_TAA_H

#include "workphone_graphics_types.h"

/* =========================================================================
 * Constants
 * ====================================================================== */

/** Number of Halton sequence samples */
#define WP_TAA_HALTON_SAMPLES 16

/** Neighborhood size for variance clipping */
#define WP_TAA_NEIGHBORHOOD_SIZE 1

/** Default feedback (blend factor) */
#define WP_TAA_DEFAULT_FEEDBACK 0.92f

/** Default clip gamma */
#define WP_TAA_DEFAULT_CLIP_GAMMA 1.25f

/** Dynamic geometry coverage threshold */
#define WP_TAA_DYNAMIC_COVERAGE 0.5f

/** Dynamic geometry feedback cap */
#define WP_TAA_DYNAMIC_FEEDBACK_CAP 0.55f

/** Fast motion feedback reduction */
#define WP_TAA_FAST_MOTION_THRESHOLD 24.0f

/** Fast motion feedback min */
#define WP_TAA_FAST_MOTION_MIN 0.72f

/** Clipping-based feedback min */
#define WP_TAA_CLIP_MIN 0.82f

/* =========================================================================
 * Types
 * ====================================================================== */

/**
 * TAA configuration
 */
typedef struct wp_taa_config
{
    /** Feedback (history blend factor) */
    wp_f32 feedback;
    /** Clip gamma for variance clipping */
    wp_f32 clip_gamma;
    /** Motion scale multiplier */
    wp_f32 motion_scale;
    /** Quality level (0=low, 1=medium, 2=high) */
    wp_s32 quality;
} wp_taa_config;

/**
 * TAA context
 */
typedef struct wp_taa wp_taa;

/**
 * Jitter sample
 */
typedef struct wp_taa_jitter
{
    wp_f32 x;
    wp_f32 y;
} wp_taa_jitter;

/* =========================================================================
 * Functions
 * ====================================================================== */

/**
 * Create a new TAA context
 *
 * @param width  Render target width
 * @param height Render target height
 * @param config Configuration (can be NULL for defaults)
 * @return New context or NULL on failure
 */
wp_taa *wp_taa_create( wp_s32 width, wp_s32 height, const wp_taa_config *config );

/**
 * Destroy a TAA context
 *
 * @param ctx Context to destroy
 */
void wp_taa_destroy( wp_taa *ctx );

/**
 * Render TAA resolve
 *
 * @param ctx              TAA context
 * @param current_buffer   Current frame color (RGB32F HDR)
 * @param history_buffer   Previous resolved frame
 * @param velocity_buffer  Screen-space velocity (RG32F: UV delta)
 * @param normal_buffer    Normal/coverage buffer (RGBA32F)
 * @param depth_buffer    Linear depth (R32F)
 * @param current_vp      Current view-projection matrix
 * @param previous_vp     Previous view-projection matrix
 * @param frame           Frame counter
 */
void wp_taa_resolve( wp_taa *ctx, const wp_f32 *current_buffer, const wp_f32 *history_buffer,
                     const wp_f32 *velocity_buffer, const wp_f32 *normal_buffer,
                     const wp_f32 *depth_buffer, const wp_mat4f *current_vp, const wp_mat4f *previous_vp,
                     const wp_mat4f *inv_vp, wp_s32 frame );

/**
 * Get the resolved color buffer
 *
 * @param ctx TAA context
 * @return Pointer to resolved color buffer (RGB32F)
 */
const wp_f32 *wp_taa_get_buffer( const wp_taa *ctx );

/**
 * Get previous frame buffer (for SSR)
 *
 * @param ctx TAA context
 * @return Pointer to previous resolved buffer (RGB32F)
 */
const wp_f32 *wp_taa_get_previous_buffer( const wp_taa *ctx );

/**
 * Get next jitter sample
 *
 * @param ctx TAA context
 * @return Jitter offset in pixels
 */
wp_taa_jitter wp_taa_next_jitter( wp_taa *ctx );

/**
 * Reset TAA history
 *
 * Call this after camera cuts or teleports.
 *
 * @param ctx TAA context
 */
void wp_taa_reset( wp_taa *ctx );

/**
 * Resize TAA buffers
 *
 * @param ctx    TAA context
 * @param width  New width
 * @param height New height
 */
void wp_taa_resize( wp_taa *ctx, wp_s32 width, wp_s32 height );

/**
 * Get buffer dimensions
 *
 * @param ctx      Context
 * @param width    Output width (can be NULL)
 * @param height   Output height (can be NULL)
 */
void wp_taa_get_size( const wp_taa *ctx, wp_s32 *width, wp_s32 *height );

/**
 * Set feedback value
 *
 * @param ctx       TAA context
 * @param feedback  New feedback value (0-1)
 */
void wp_taa_set_feedback( wp_taa *ctx, wp_f32 feedback );

/**
 * Sample a color buffer at UV with Catmull-Rom filtering
 *
 * @param buffer  Color buffer
 * @param width   Buffer width
 * @param height  Buffer height
 * @param u       U coordinate
 * @param v       V coordinate
 * @param out_r   Output R value
 * @param out_g   Output G value
 * @param out_b   Output B value
 */
void wp_taa_sample_catmull_rom( const wp_f32 *buffer, wp_s32 width, wp_s32 height, wp_f32 u, wp_f32 v,
                                wp_f32 *out_r, wp_f32 *out_g, wp_f32 *out_b );

/**
 * Convert RGB to YCoCg
 *
 * @param r Input R
 * @param g Input G
 * @param b Input B
 * @param out_y   Output Y
 * @param out_co  Output Co
 * @param out_cg  Output Cg
 */
void wp_taa_rgb_to_ycocg( wp_f32 r, wp_f32 g, wp_f32 b, wp_f32 *out_y, wp_f32 *out_co, wp_f32 *out_cg );

/**
 * Convert YCoCg to RGB
 *
 * @param y   Input Y
 * @param co  Input Co
 * @param cg  Input Cg
 * @param out_r Output R
 * @param out_g Output G
 * @param out_b Output B
 */
void wp_taa_ycocg_to_rgb( wp_f32 y, wp_f32 co, wp_f32 cg, wp_f32 *out_r, wp_f32 *out_g, wp_f32 *out_b );

#endif /* WP_GRAPHICS_TAA_H */
