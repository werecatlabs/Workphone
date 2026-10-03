/**
 * @file wp_graphics_contact_shadows.h
 * @brief Contact Shadows - Screen-space depth march shadows for prop contact.
 *
 * A cascaded shadow map loses the last few centimetres of contact:
 * the texel is bigger than the gap between a crate and the floor.
 * This marches a short ray through the depth buffer toward the sun and
 * puts that contact back, which is what stops props looking like stickers.
 *
 * Consumed inside the material, multiplied onto the sun term only.
 *
 * Reference: Next Generation Post Processing in Call of Duty: Advanced Warfare
 * (Jimenez et al. 2014)
 */

#ifndef WP_GRAPHICS_CONTACT_SHADOWS_H
#define WP_GRAPHICS_CONTACT_SHADOWS_H

#include "workphone_graphics_types.h"

/* =========================================================================
 * Constants
 * ====================================================================== */

/** Maximum ray march steps */
#define WP_CONTACT_SHADOW_STEPS 14

/** Default ray length in world units (meters) */
#define WP_CONTACT_SHADOW_DEFAULT_LENGTH 0.40f

/** Default thickness threshold in world units (meters) */
#define WP_CONTACT_SHADOW_DEFAULT_THICKNESS 0.42f

/** Default shadow strength (0-1) */
#define WP_CONTACT_SHADOW_DEFAULT_STRENGTH 1.0f

/** Blur kernel radius for bilateral filter */
#define WP_CONTACT_SHADOW_BLUR_RADIUS 2

/* =========================================================================
 * Types
 * ====================================================================== */

/**
 * Contact shadows configuration
 */
typedef struct wp_contact_shadows_config
{
    /** Ray length in world units at 1x distance scaling */
    wp_f32 length;
    /** Thickness threshold for valid occlusion */
    wp_f32 thickness;
    /** Shadow strength (0-1, how much of sun term a full hit removes) */
    wp_f32 strength;
    /** Quality level (0=low, 1=medium, 2=high) */
    wp_s32 quality;
} wp_contact_shadows_config;

/**
 * Contact shadows context
 */
typedef struct wp_contact_shadows wp_contact_shadows;

/**
 * Ray march result at a pixel
 */
typedef struct wp_contact_shadow_result
{
    /** Shadow value (0-1, 0=fully occluded, 1=no shadow) */
    wp_f32 shadow;
    /** Linear depth at this pixel */
    wp_f32 depth;
    /** Validity flag */
    wp_f32 valid;
    /** Padding */
    wp_f32 pad;
} wp_contact_shadow_result;

/* =========================================================================
 * Functions
 * ====================================================================== */

/**
 * Create a new contact shadows context
 *
 * @param width  Render target width
 * @param height Render target height
 * @param config Configuration (can be NULL for defaults)
 * @return New context or NULL on failure
 */
wp_contact_shadows *wp_contact_shadows_create( wp_s32 width, wp_s32 height,
                                               const wp_contact_shadows_config *config );

/**
 * Destroy a contact shadows context
 *
 * @param ctx Context to destroy
 */
void wp_contact_shadows_destroy( wp_contact_shadows *ctx );

/**
 * Render contact shadows
 *
 * Renders the contact shadow pass and stores the result. The result
 * should be passed to materials to multiply onto the sun shadow term.
 *
 * @param ctx           Context
 * @param depth_buffer  Linear depth buffer (R32F, view-space meters, positive)
 * @param normal_buffer Octahedral normal buffer (RG32F or packed in RGBA)
 * @param camera        Camera for projection matrices
 * @param sun_dir_view  Sun direction in view space (normalized)
 * @param frame         Frame counter for stochastic variation
 */
void wp_contact_shadows_render( wp_contact_shadows *ctx, const wp_f32 *depth_buffer,
                                const wp_f32 *normal_buffer, const wp_mat4f *proj_matrix,
                                const wp_mat4f *proj_inv_matrix, const wp_vec3f *sun_dir_view,
                                wp_s32 frame );

/**
 * Get the shadow result buffer
 *
 * @param ctx Context
 * @return Pointer to shadow buffer (RG32F: x=shadow, y=depth)
 */
const wp_f32 *wp_contact_shadows_get_buffer( const wp_contact_shadows *ctx );

/**
 * Get buffer dimensions
 *
 * @param ctx      Context
 * @param width    Output width (can be NULL)
 * @param height   Output height (can be NULL)
 */
void wp_contact_shadows_get_size( const wp_contact_shadows *ctx, wp_s32 *width, wp_s32 *height );

/**
 * Set ray length
 *
 * @param ctx   Context
 * @param len   Ray length in world units at 1x distance scaling
 */
void wp_contact_shadows_set_length( wp_contact_shadows *ctx, wp_f32 len );

/**
 * Set shadow strength
 *
 * @param ctx   Context
 * @param s     Shadow strength (0-1)
 */
void wp_contact_shadows_set_strength( wp_contact_shadows *ctx, wp_f32 s );

/**
 * Resize the contact shadows buffers
 *
 * @param ctx    Context
 * @param width  New width
 * @param height New height
 */
void wp_contact_shadows_resize( wp_contact_shadows *ctx, wp_s32 width, wp_s32 height );

/**
 * Sample shadow at a UV coordinate
 *
 * Bilinearly samples the shadow buffer at the given UV coordinate.
 *
 * @param ctx    Context
 * @param uv    UV coordinate (0-1)
 * @param out   Output shadow value (can be NULL)
 * @param depth Output depth (can be NULL)
 */
void wp_contact_shadows_sample( const wp_contact_shadows *ctx, wp_f32 u, wp_f32 v, wp_f32 *out_shadow,
                                wp_f32 *out_depth );

/**
 * Get the GLSL shader code for contact shadow sampling
 *
 * Returns the GLSL function that samples the contact shadow in materials.
 *
 * @return GLSL shader code string (embedded null-terminated)
 */
const wp_c8 *wp_contact_shadows_get_glsl_chunk( void );

#endif /* WP_GRAPHICS_CONTACT_SHADOWS_H */
