/**
 * @file workphone_graphics_renderer_vulkan.h
 * @brief C API for the Vulkan hardware renderer.
 *
 * wp_renderer_vulkan provides a hardware-accelerated rendering back-end using
 * Vulkan. It mirrors the interface of wp_renderer_software, DX11, DX12, GL, and Metal so that
 * the abstract wp_renderer front-end can dispatch to any implementation.
 *
 * The renderer is created around an existing window/context. Internally it owns the Vulkan
 * instance, device, command queue, swap-chain, render-targets, and all pipeline state objects.
 */

#ifndef WORKPHONE_GRAPHICS_RENDERER_VULKAN_H
#define WORKPHONE_GRAPHICS_RENDERER_VULKAN_H

#include <stdint.h>
#include "workphone_vector.h"
#include "workphone_matrix.h"
#include "workphone_graphics_vertex.h"
#include "workphone_graphics_viewport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Opaque type
 * ---------------------------------------------------------------------- */

typedef struct wp_renderer_vulkan wp_renderer_vulkan;

/* =========================================================================
 * Lifecycle
 * ====================================================================== */
wp_renderer_vulkan *wp_renderer_vulkan_create(void *window, wp_s32 width, wp_s32 height);
void wp_renderer_vulkan_destroy(wp_renderer_vulkan *renderer);
wp_s32 wp_renderer_vulkan_resize(wp_renderer_vulkan *renderer, wp_s32 width, wp_s32 height);

/* =========================================================================
 * Frame lifecycle
 * ====================================================================== */
void wp_renderer_vulkan_begin_frame(wp_renderer_vulkan *renderer);
void wp_renderer_vulkan_end_frame(wp_renderer_vulkan *renderer);
void wp_renderer_vulkan_present(wp_renderer_vulkan *renderer, wp_s32 vsync);

/* =========================================================================
 * Clear
 * ====================================================================== */
void wp_renderer_vulkan_set_clear_color(wp_renderer_vulkan *renderer, wp_f32 r, wp_f32 g, wp_f32 b, wp_f32 a);
void wp_renderer_vulkan_set_clear_depth(wp_renderer_vulkan *renderer, wp_f32 depth);
void wp_renderer_vulkan_clear(wp_renderer_vulkan *renderer, wp_u32 flags);

/* =========================================================================
 * Viewport and scissor
 * ====================================================================== */
void          wp_renderer_vulkan_set_viewport(wp_renderer_vulkan *renderer, wp_viewport_i viewport);
wp_viewport_i wp_renderer_vulkan_get_viewport(const wp_renderer_vulkan *renderer);
void          wp_renderer_vulkan_set_scissor_enabled(wp_renderer_vulkan *renderer, wp_s32 enabled);
void          wp_renderer_vulkan_set_scissor_rect(wp_renderer_vulkan *renderer, wp_viewport_i scissor);

/* =========================================================================
 * Render state
 * ====================================================================== */
void          wp_renderer_vulkan_set_blend_mode(wp_renderer_vulkan *renderer, wp_blend_mode mode);
wp_blend_mode wp_renderer_vulkan_get_blend_mode(const wp_renderer_vulkan *renderer);
void          wp_renderer_vulkan_set_fill_mode(wp_renderer_vulkan *renderer, wp_fill_mode mode);
wp_fill_mode  wp_renderer_vulkan_get_fill_mode(const wp_renderer_vulkan *renderer);
void          wp_renderer_vulkan_set_cull_mode(wp_renderer_vulkan *renderer, wp_cull_mode mode);
wp_cull_mode  wp_renderer_vulkan_get_cull_mode(const wp_renderer_vulkan *renderer);
void          wp_renderer_vulkan_set_depth_test_enabled(wp_renderer_vulkan *renderer, wp_s32 enabled);
wp_s32        wp_renderer_vulkan_get_depth_test_enabled(const wp_renderer_vulkan *renderer);
void          wp_renderer_vulkan_set_depth_write_enabled(wp_renderer_vulkan *renderer, wp_s32 enabled);
wp_s32        wp_renderer_vulkan_get_depth_write_enabled(const wp_renderer_vulkan *renderer);
void          wp_renderer_vulkan_set_depth_func(wp_renderer_vulkan *renderer, wp_depth_func func);
wp_depth_func wp_renderer_vulkan_get_depth_func(const wp_renderer_vulkan *renderer);

/* =========================================================================
 * Transform matrices
 * ====================================================================== */
void wp_renderer_vulkan_set_world_matrix(wp_renderer_vulkan *renderer, const wp_mat4f *mat);
void wp_renderer_vulkan_set_view_matrix(wp_renderer_vulkan *renderer, const wp_mat4f *mat);
void wp_renderer_vulkan_set_projection_matrix(wp_renderer_vulkan *renderer, const wp_mat4f *mat);

/* =========================================================================
 * Draw calls
 * ====================================================================== */
void wp_renderer_vulkan_draw_triangles_pc(wp_renderer_vulkan *renderer, const wp_vertex_pc *vertices, wp_s32 vertex_count);
void wp_renderer_vulkan_draw_indexed_triangles_pc(wp_renderer_vulkan *renderer, const wp_vertex_pc *vertices, wp_s32 vertex_count, const uint16_t *indices, wp_s32 index_count);
void wp_renderer_vulkan_draw_lines_pc(wp_renderer_vulkan *renderer, const wp_vertex_pc *vertices, wp_s32 vertex_count);
void wp_renderer_vulkan_draw_points_pc(wp_renderer_vulkan *renderer, const wp_vertex_pc *vertices, wp_s32 vertex_count);
void wp_renderer_vulkan_draw_triangles_ptc(wp_renderer_vulkan *renderer, const wp_vertex_ptc *vertices, wp_s32 vertex_count);
void wp_renderer_vulkan_draw_indexed_triangles_ptc(wp_renderer_vulkan *renderer, const wp_vertex_ptc *vertices, wp_s32 vertex_count, const uint16_t *indices, wp_s32 index_count);

/* =========================================================================
 * Dimension queries
 * ====================================================================== */
wp_s32 wp_renderer_vulkan_get_width(const wp_renderer_vulkan *renderer);
wp_s32 wp_renderer_vulkan_get_height(const wp_renderer_vulkan *renderer);

/* =========================================================================
 * Native object access
 * ====================================================================== */
void wp_renderer_vulkan_get_native(const wp_renderer_vulkan *renderer, void **pp_object);
void wp_renderer_vulkan_set_native(wp_renderer_vulkan *renderer, void *native);

/* =========================================================================
 * Vulkan-specific accessors
 * ====================================================================== */
void *wp_renderer_vulkan_get_instance(const wp_renderer_vulkan *renderer);
void *wp_renderer_vulkan_get_device(const wp_renderer_vulkan *renderer);
void *wp_renderer_vulkan_get_window(const wp_renderer_vulkan *renderer);

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GRAPHICS_RENDERER_VULKAN_H */
