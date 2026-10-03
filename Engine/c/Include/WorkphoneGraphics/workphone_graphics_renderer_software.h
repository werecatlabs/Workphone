/**
 * @file wp_renderer_software.h
 * @brief C API for the CPU-based software renderer.
 *
 * The software renderer rasterises geometry directly into a CPU-side
 * framebuffer (colour + depth).  It is self-contained: no GPU, no windowing
 * system dependency.  The caller allocates a renderer, drives the per-frame
 * lifecycle (begin / draw / end), and reads back the finished pixel data via
 * wp_renderer_software_get_framebuffer().
 */

#ifndef WORKPHONE_RENDERER_SOFTWARE_H
#define WORKPHONE_RENDERER_SOFTWARE_H

#include <stdint.h>
#include "workphone_types.h"
#include "workphone_vector.h"
#include "workphone_matrix.h"
#include "workphone_graphics_vertex.h"
#include "workphone_graphics_viewport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wp_renderer_software wp_renderer_software;

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Creates a software renderer targeting a CPU-side framebuffer.
 *
 * @param width  Initial framebuffer width in pixels. Must be > 0.
 * @param height Initial framebuffer height in pixels. Must be > 0.
 * @param format Pixel layout of the colour buffer.
 * @return Pointer to the created renderer, or NULL on allocation failure.
 */
wp_renderer_software *wp_renderer_software_create( wp_s32 width, wp_s32 height, wp_pixel_format format );

/**
 * @brief Destroys the renderer and frees all associated resources.
 * @param renderer Pointer to the renderer to destroy. Ignored if NULL.
 */
void wp_renderer_software_destroy( wp_renderer_software *renderer );

/* -------------------------------------------------------------------------
 * Resize
 * ---------------------------------------------------------------------- */

/**
 * @brief Resizes the colour and depth framebuffers.
 *
 * Existing framebuffer contents are discarded. The active viewport is
 * reset to cover the full new dimensions.
 *
 * @param renderer Pointer to the renderer.
 * @param width    New framebuffer width in pixels. Must be > 0.
 * @param height   New framebuffer height in pixels. Must be > 0.
 * @return Non-zero on success; zero if the allocation failed.
 */
wp_s32 wp_renderer_software_resize( wp_renderer_software *renderer, wp_s32 width, wp_s32 height );

/* -------------------------------------------------------------------------
 * Frame lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Marks the beginning of a new frame.
 *
 * Resets per-frame statistics and prepares internal state for draw calls.
 * Must be called before any draw or clear operations in a frame.
 *
 * @param renderer Pointer to the renderer.
 */
void wp_renderer_software_begin_frame( wp_renderer_software *renderer );

/**
 * @brief Marks the end of the current frame.
 *
 * Flushes any deferred work and finalises the colour framebuffer so that
 * it can be safely read via wp_renderer_software_get_framebuffer().
 *
 * @param renderer Pointer to the renderer.
 */
void wp_renderer_software_end_frame( wp_renderer_software *renderer );

/* -------------------------------------------------------------------------
 * Clear
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the colour value used by subsequent clear operations.
 *
 * @param renderer Pointer to the renderer.
 * @param r        Red channel [0, 1].
 * @param g        Green channel [0, 1].
 * @param b        Blue channel [0, 1].
 * @param a        Alpha channel [0, 1].
 */
void wp_renderer_software_set_clear_color( wp_renderer_software *renderer, wp_f32 r, wp_f32 g, wp_f32 b,
                                           wp_f32 a );

/**
 * @brief Sets the depth value written by subsequent depth-buffer clear operations.
 *
 * @param renderer Pointer to the renderer.
 * @param depth    Clear depth value in [0, 1]. Defaults to 1.0.
 */
void wp_renderer_software_set_clear_depth( wp_renderer_software *renderer, wp_f32 depth );

/**
 * @brief Clears the selected buffers within the active scissor region.
 *
 * @param renderer Pointer to the renderer.
 * @param flags    Bitwise OR of WORKPHONE_CLEAR_FLAG_* values.
 */
void wp_renderer_software_clear( wp_renderer_software *renderer, wp_u32 flags );

/* -------------------------------------------------------------------------
 * Viewport and scissor
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the active viewport used for the NDC-to-screen transform.
 *
 * @param renderer Pointer to the renderer.
 * @param viewport Viewport rectangle in pixels. Clamped to the framebuffer.
 */
void wp_renderer_software_set_viewport( wp_renderer_software *renderer, wp_viewport_i viewport );

/**
 * @brief Gets the currently active viewport.
 *
 * @param renderer Pointer to the renderer.
 * @return Current viewport rectangle. Returns a zeroed struct if renderer is NULL.
 */
wp_viewport_i wp_renderer_software_get_viewport( const wp_renderer_software *renderer );

/**
 * @brief Enables or disables scissor testing.
 *
 * When enabled, only fragments inside the scissor rectangle are written.
 *
 * @param renderer Pointer to the renderer.
 * @param enabled  Non-zero to enable scissor testing; zero to disable.
 */
void wp_renderer_software_set_scissor_enabled( wp_renderer_software *renderer, wp_s32 enabled );

/**
 * @brief Sets the scissor rectangle (active only when scissor test is enabled).
 *
 * @param renderer Pointer to the renderer.
 * @param scissor  Scissor rectangle in pixels.
 */
void wp_renderer_software_set_scissor_rect( wp_renderer_software *renderer, wp_viewport_i scissor );

/* -------------------------------------------------------------------------
 * Framebuffer access
 * ---------------------------------------------------------------------- */

/**
 * @brief Returns a read-only pointer to the colour framebuffer.
 *
 * The buffer is tightly packed; stride equals width * bytes-per-pixel.
 * Valid only after wp_renderer_software_end_frame() has been called.
 *
 * @param renderer Pointer to the renderer.
 * @return Pointer to the first pixel, or NULL if renderer is NULL.
 */
const void *wp_renderer_software_get_framebuffer( const wp_renderer_software *renderer );

/** Replace the color framebuffer with pixels in the renderer's current format. */
wp_s32 wp_renderer_software_set_framebuffer( wp_renderer_software *renderer, const void *pixels,
                                             wp_pixel_format format );

/**
 * @brief Returns a read-only pointer to the depth buffer.
 *
 * One wp_f32 per pixel, row-major.  Values are in [0, 1].
 *
 * @param renderer Pointer to the renderer.
 * @return Pointer to the first depth value, or NULL if renderer is NULL.
 */
const wp_f32 *wp_renderer_software_get_depth_buffer( const wp_renderer_software *renderer );

/**
 * @brief Returns the framebuffer width in pixels.
 * @param renderer Pointer to the renderer.
 * @return Width in pixels, or 0 if renderer is NULL.
 */
wp_s32 wp_renderer_software_get_width( const wp_renderer_software *renderer );

/**
 * @brief Returns the framebuffer height in pixels.
 * @param renderer Pointer to the renderer.
 * @return Height in pixels, or 0 if renderer is NULL.
 */
wp_s32 wp_renderer_software_get_height( const wp_renderer_software *renderer );

/**
 * @brief Returns the pixel format of the colour framebuffer.
 * @param renderer Pointer to the renderer.
 * @return Active pixel format.
 */
wp_pixel_format wp_renderer_software_get_pixel_format( const wp_renderer_software *renderer );

/* -------------------------------------------------------------------------
 * Render state
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the blending mode applied during rasterisation.
 * @param renderer Pointer to the renderer.
 * @param mode     Blending mode to apply.
 */
void wp_renderer_software_set_blend_mode( wp_renderer_software *renderer, wp_blend_mode mode );

/**
 * @brief Gets the current blending mode.
 * @param renderer Pointer to the renderer.
 * @return Active blend mode.
 */
wp_blend_mode wp_renderer_software_get_blend_mode( const wp_renderer_software *renderer );

/**
 * @brief Sets the triangle fill mode.
 * @param renderer Pointer to the renderer.
 * @param mode     Fill mode to apply.
 */
void wp_renderer_software_set_fill_mode( wp_renderer_software *renderer, wp_fill_mode mode );

/**
 * @brief Gets the current fill mode.
 * @param renderer Pointer to the renderer.
 * @return Active fill mode.
 */
wp_fill_mode wp_renderer_software_get_fill_mode( const wp_renderer_software *renderer );

/**
 * @brief Sets the back-face culling mode.
 * @param renderer Pointer to the renderer.
 * @param mode     Culling mode to apply.
 */
void wp_renderer_software_set_cull_mode( wp_renderer_software *renderer, wp_cull_mode mode );

/**
 * @brief Gets the current culling mode.
 * @param renderer Pointer to the renderer.
 * @return Active cull mode.
 */
wp_cull_mode wp_renderer_software_get_cull_mode( const wp_renderer_software *renderer );

/**
 * @brief Enables or disables the depth test.
 * @param renderer Pointer to the renderer.
 * @param enabled  Non-zero to enable depth testing; zero to disable.
 */
void wp_renderer_software_set_depth_test_enabled( wp_renderer_software *renderer, wp_s32 enabled );

/**
 * @brief Queries whether the depth test is enabled.
 * @param renderer Pointer to the renderer.
 * @return Non-zero if depth testing is enabled.
 */
wp_s32 wp_renderer_software_get_depth_test_enabled( const wp_renderer_software *renderer );

/**
 * @brief Enables or disables writes to the depth buffer.
 * @param renderer Pointer to the renderer.
 * @param enabled  Non-zero to enable depth writes; zero to disable.
 */
void wp_renderer_software_set_depth_write_enabled( wp_renderer_software *renderer, wp_s32 enabled );

/**
 * @brief Queries whether depth writes are enabled.
 * @param renderer Pointer to the renderer.
 * @return Non-zero if depth writes are enabled.
 */
wp_s32 wp_renderer_software_get_depth_write_enabled( const wp_renderer_software *renderer );

/**
 * @brief Sets the depth comparison function.
 * @param renderer Pointer to the renderer.
 * @param func     Comparison function to use for depth testing.
 */
void wp_renderer_software_set_depth_func( wp_renderer_software *renderer, wp_depth_func func );

/**
 * @brief Gets the current depth comparison function.
 * @param renderer Pointer to the renderer.
 * @return Active depth comparison function.
 */
wp_depth_func wp_renderer_software_get_depth_func( const wp_renderer_software *renderer );

/* -------------------------------------------------------------------------
 * Transform matrices
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the world (model) transform matrix.
 * @param renderer Pointer to the renderer.
 * @param mat      Column-major 4x4 world matrix.
 */
void wp_renderer_software_set_world_matrix( wp_renderer_software *renderer, const wp_mat4f *mat );

/**
 * @brief Sets the view (camera) transform matrix.
 * @param renderer Pointer to the renderer.
 * @param mat      Column-major 4x4 view matrix.
 */
void wp_renderer_software_set_view_matrix( wp_renderer_software *renderer, const wp_mat4f *mat );

/**
 * @brief Sets the projection matrix.
 * @param renderer Pointer to the renderer.
 * @param mat      Column-major 4x4 projection matrix.
 */
void wp_renderer_software_set_projection_matrix( wp_renderer_software *renderer, const wp_mat4f *mat );
void wp_renderer_software_get_world_matrix( const wp_renderer_software *renderer, wp_mat4f *mat );
void wp_renderer_software_get_view_matrix( const wp_renderer_software *renderer, wp_mat4f *mat );
void wp_renderer_software_get_projection_matrix( const wp_renderer_software *renderer, wp_mat4f *mat );

/**
 * @brief Binds a non-owning texture for subsequent PTC draw calls.
 *
 * Passing NULL unbinds the texture and makes PTC draws use a white texel.
 */
void wp_renderer_software_set_texture( wp_renderer_software *renderer, const void *pixels, wp_s32 width,
                                       wp_s32 height, wp_pixel_format format );

/**
 * @brief Executes the rendering process for the software renderer.
 * @param renderer    Pointer to the renderer.
 * @param anti_aliasing The anti-aliasing mode to use.
 */
void wp_renderer_software_render( wp_renderer_software *renderer, enum wp_anti_aliasing anti_aliasing );

/* -------------------------------------------------------------------------
 * Draw calls
 * ---------------------------------------------------------------------- */

/**
 * @brief Draws a list of independent triangles (position + colour vertices).
 *
 * Vertices are consumed in groups of three; each group forms one triangle.
 *
 * @param renderer     Pointer to the renderer.
 * @param vertices     Array of vertices. Must not be NULL.
 * @param vertex_count Number of vertices. Must be a multiple of 3.
 */
void wp_renderer_software_draw_triangles_pc( wp_renderer_software *renderer,
                                             const wp_vertex_pc *vertices, wp_s32 vertex_count );

/**
 * @brief Draws an indexed list of triangles (position + colour vertices).
 *
 * @param renderer     Pointer to the renderer.
 * @param vertices     Array of vertices. Must not be NULL.
 * @param vertex_count Number of entries in the vertex array.
 * @param indices      Array of 16-bit indices. Must not be NULL.
 * @param index_count  Number of indices. Must be a multiple of 3.
 */
void wp_renderer_software_draw_indexed_triangles_pc( wp_renderer_software *renderer,
                                                     const wp_vertex_pc *vertices, wp_s32 vertex_count,
                                                     const uint16_t *indices, wp_s32 index_count );

/**
 * @brief Draws a list of independent lines (position + colour vertices).
 *
 * Vertices are consumed in pairs; each pair forms one line segment.
 *
 * @param renderer     Pointer to the renderer.
 * @param vertices     Array of vertices. Must not be NULL.
 * @param vertex_count Number of vertices. Must be a multiple of 2.
 */
void wp_renderer_software_draw_lines_pc( wp_renderer_software *renderer, const wp_vertex_pc *vertices,
                                         wp_s32 vertex_count );

/**
 * @brief Draws a list of points (position + colour vertices).
 *
 * @param renderer     Pointer to the renderer.
 * @param vertices     Array of vertices. Must not be NULL.
 * @param vertex_count Number of vertices.
 */
void wp_renderer_software_draw_points_pc( wp_renderer_software *renderer, const wp_vertex_pc *vertices,
                                          wp_s32 vertex_count );

/**
 * @brief Draws a list of independent triangles (position + UV + colour vertices).
 *
 * @param renderer     Pointer to the renderer.
 * @param vertices     Array of vertices. Must not be NULL.
 * @param vertex_count Number of vertices. Must be a multiple of 3.
 */
void wp_renderer_software_draw_triangles_ptc( wp_renderer_software *renderer,
                                              const wp_vertex_ptc *vertices, wp_s32 vertex_count );

/**
 * @brief Draws an indexed list of triangles (position + UV + colour vertices).
 *
 * @param renderer     Pointer to the renderer.
 * @param vertices     Array of vertices. Must not be NULL.
 * @param vertex_count Number of entries in the vertex array.
 * @param indices      Array of 16-bit indices. Must not be NULL.
 * @param index_count  Number of indices. Must be a multiple of 3.
 */
void wp_renderer_software_draw_indexed_triangles_ptc( wp_renderer_software *renderer,
                                                      const wp_vertex_ptc *vertices, wp_s32 vertex_count,
                                                      const uint16_t *indices, wp_s32 index_count );

void wp_renderer_software_draw_indexed_triangles_ptc_u32( wp_renderer_software *renderer,
                                                          const wp_vertex_ptc *vertices,
                                                          wp_s32 vertex_count, const wp_u32 *indices,
                                                          wp_s32 index_count );

/* -------------------------------------------------------------------------
 * Native object access
 * ---------------------------------------------------------------------- */

/**
 * @brief Retrieves the underlying native implementation pointer.
 * @param renderer  Pointer to the renderer.
 * @param pp_object Output pointer that receives the native object pointer.
 *                  Set to NULL if renderer is NULL or no native object is set.
 */
void wp_renderer_software_get_native( const wp_renderer_software *renderer, void **pp_object );

/**
 * @brief Sets the underlying native implementation pointer.
 * @param renderer Pointer to the renderer.
 * @param native   Pointer to the native object to associate with this renderer.
 */
void wp_renderer_software_set_native( wp_renderer_software *renderer, void *native );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_RENDERER_SOFTWARE_H */
