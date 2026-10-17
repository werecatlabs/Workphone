/**
 * @file workphone_graphics_renderer_dx12.h
 * @brief C API for the Direct3D 12 hardware renderer.
 *
 * wp_renderer_dx12 provides a hardware-accelerated rendering back-end using
 * Direct3D 12. It mirrors the interface of wp_renderer_software and DX11 so that
 * the abstract wp_renderer front-end can dispatch to any implementation.
 *
 * The renderer is created around an existing Win32 HWND. Internally it owns the D3D12
 * device, command queue, swap-chain, render-targets, depth-stencil buffer, and all pipeline state objects.
 */

#ifndef WORKPHONE_GRAPHICS_RENDERER_DX12_H
#define WORKPHONE_GRAPHICS_RENDERER_DX12_H

#ifdef _WIN32

#    include <stdint.h>
#    include "workphone_types.h"
#    include "workphone_vector.h"
#    include "workphone_matrix.h"
#    include "workphone_graphics_vertex.h"
#    include "workphone_graphics_viewport.h"

#    ifdef __cplusplus
extern "C" {
#    endif

/* -------------------------------------------------------------------------
 * Opaque type
 * ---------------------------------------------------------------------- */

typedef struct wp_renderer_dx12 wp_renderer_dx12;

/* =========================================================================
 * Lifecycle
 * ====================================================================== */
wp_renderer_dx12 *wp_renderer_dx12_create( void *hwnd, wp_s32 width, wp_s32 height );
void wp_renderer_dx12_destroy( wp_renderer_dx12 *renderer );
wp_s32 wp_renderer_dx12_resize( wp_renderer_dx12 *renderer, wp_s32 width, wp_s32 height );

/* =========================================================================
 * Frame lifecycle
 * ====================================================================== */
void wp_renderer_dx12_begin_frame( wp_renderer_dx12 *renderer );
void wp_renderer_dx12_end_frame( wp_renderer_dx12 *renderer );
void wp_renderer_dx12_present( wp_renderer_dx12 *renderer, wp_s32 vsync );
wp_s32 wp_renderer_dx12_try_present( wp_renderer_dx12 *renderer, void *expected_hwnd,
                                     wp_s32 vsync );
wp_s32 wp_renderer_dx12_copy_back_buffer_rgba8( wp_renderer_dx12 *renderer,
                                                void *expected_hwnd, void *output,
                                                wp_u64 output_bytes );

typedef struct wp_render_statistics_dx12
{
    uint64_t frames, interval_samples, gpu_samples, draws, triangles;
    uint64_t material_uploads, transform_uploads, geometry_creations, state_bindings;
    uint64_t instanced_draws, instances, instance_uploads, instance_upload_bytes;
    uint64_t instance_buffer_creations;
    double interval_ms, interval_p95_ms, cpu_frame_ms, present_ms, gpu_frame_ms;
    wp_s32 flip_model;
    uint64_t present_attempts, present_occluded, present_failures;
    wp_s32 last_present_hresult;
} wp_render_statistics_dx12;

void wp_renderer_dx12_invalidate_state( wp_renderer_dx12 *renderer );
void wp_renderer_dx12_reset_statistics( wp_renderer_dx12 *renderer );
void wp_renderer_dx12_get_statistics( const wp_renderer_dx12 *renderer,
                                      wp_render_statistics_dx12 *statistics );

/* =========================================================================
 * Clear
 * ====================================================================== */
void wp_renderer_dx12_set_clear_color( wp_renderer_dx12 *renderer, wp_f32 r, wp_f32 g, wp_f32 b,
                                       wp_f32 a );
void wp_renderer_dx12_set_clear_depth( wp_renderer_dx12 *renderer, wp_f32 depth );
void wp_renderer_dx12_clear( wp_renderer_dx12 *renderer, wp_u32 flags );

/* =========================================================================
 * Viewport and scissor
 * ====================================================================== */
void wp_renderer_dx12_set_viewport( wp_renderer_dx12 *renderer, wp_viewport_i viewport );
wp_viewport_i wp_renderer_dx12_get_viewport( const wp_renderer_dx12 *renderer );
void wp_renderer_dx12_set_scissor_enabled( wp_renderer_dx12 *renderer, wp_s32 enabled );
void wp_renderer_dx12_set_scissor_rect( wp_renderer_dx12 *renderer, wp_viewport_i scissor );

/* =========================================================================
 * Render state
 * ====================================================================== */
void wp_renderer_dx12_set_blend_mode( wp_renderer_dx12 *renderer, wp_blend_mode mode );
wp_blend_mode wp_renderer_dx12_get_blend_mode( const wp_renderer_dx12 *renderer );
void wp_renderer_dx12_set_fill_mode( wp_renderer_dx12 *renderer, wp_fill_mode mode );
wp_fill_mode wp_renderer_dx12_get_fill_mode( const wp_renderer_dx12 *renderer );
void wp_renderer_dx12_set_cull_mode( wp_renderer_dx12 *renderer, wp_cull_mode mode );
wp_cull_mode wp_renderer_dx12_get_cull_mode( const wp_renderer_dx12 *renderer );
void wp_renderer_dx12_set_depth_test_enabled( wp_renderer_dx12 *renderer, wp_s32 enabled );
wp_s32 wp_renderer_dx12_get_depth_test_enabled( const wp_renderer_dx12 *renderer );
void wp_renderer_dx12_set_depth_write_enabled( wp_renderer_dx12 *renderer, wp_s32 enabled );
wp_s32 wp_renderer_dx12_get_depth_write_enabled( const wp_renderer_dx12 *renderer );
void wp_renderer_dx12_set_depth_func( wp_renderer_dx12 *renderer, wp_depth_func func );
wp_depth_func wp_renderer_dx12_get_depth_func( const wp_renderer_dx12 *renderer );

/* =========================================================================
 * Transform matrices
 * ====================================================================== */
void wp_renderer_dx12_set_world_matrix( wp_renderer_dx12 *renderer, const wp_mat4f *mat );
void wp_renderer_dx12_set_view_matrix( wp_renderer_dx12 *renderer, const wp_mat4f *mat );
void wp_renderer_dx12_set_projection_matrix( wp_renderer_dx12 *renderer, const wp_mat4f *mat );

/* =========================================================================
 * Draw calls
 * ====================================================================== */
void wp_renderer_dx12_draw_triangles_pc( wp_renderer_dx12 *renderer, const wp_vertex_pc *vertices,
                                         wp_s32 vertex_count );
void wp_renderer_dx12_draw_indexed_triangles_pc( wp_renderer_dx12 *renderer,
                                                 const wp_vertex_pc *vertices, wp_s32 vertex_count,
                                                 const uint16_t *indices, wp_s32 index_count );
void wp_renderer_dx12_draw_lines_pc( wp_renderer_dx12 *renderer, const wp_vertex_pc *vertices,
                                     wp_s32 vertex_count );
void wp_renderer_dx12_draw_points_pc( wp_renderer_dx12 *renderer, const wp_vertex_pc *vertices,
                                      wp_s32 vertex_count );
void wp_renderer_dx12_draw_triangles_ptc( wp_renderer_dx12 *renderer, const wp_vertex_ptc *vertices,
                                          wp_s32 vertex_count );
void wp_renderer_dx12_draw_indexed_triangles_ptc( wp_renderer_dx12 *renderer,
                                                  const wp_vertex_ptc *vertices, wp_s32 vertex_count,
                                                  const uint16_t *indices, wp_s32 index_count );
void wp_renderer_dx12_draw_indexed_triangles_ptc_u32( wp_renderer_dx12 *renderer,
                                                      const wp_vertex_ptc *vertices,
                                                      wp_s32 vertex_count, const uint32_t *indices,
                                                      wp_s32 index_count );
wp_s32 wp_renderer_dx12_prepare_indexed_triangles_ptc( wp_renderer_dx12 *renderer,
                                                       const wp_vertex_ptc *vertices,
                                                       wp_s32 vertex_count, const uint16_t *indices,
                                                       wp_s32 index_count );
wp_s32 wp_renderer_dx12_prepare_indexed_triangles_ptc_u32( wp_renderer_dx12 *renderer,
                                                           const wp_vertex_ptc *vertices,
                                                           wp_s32 vertex_count, const uint32_t *indices,
                                                           wp_s32 index_count );
void wp_renderer_dx12_draw_prepared_indexed_triangles_ptc( wp_renderer_dx12 *renderer,
                                                           wp_s32 index_start, wp_s32 index_count,
                                                           wp_s32 base_vertex );

/* =========================================================================
 * Textures (used by the PTC draw calls)
 * ====================================================================== */
void wp_renderer_dx12_set_texture( wp_renderer_dx12 *renderer, const void *pixels, wp_s32 width,
                                   wp_s32 height, wp_pixel_format format );
void wp_renderer_dx12_set_texture_native( wp_renderer_dx12 *renderer, void *texture );
void *wp_renderer_dx12_create_texture_native( wp_renderer_dx12 *renderer, const void *pixels,
                                              wp_s32 width, wp_s32 height, wp_pixel_format format );
void wp_renderer_dx12_destroy_texture_native( void *texture );

/* =========================================================================
 * Dimension queries
 * ====================================================================== */
wp_s32 wp_renderer_dx12_get_width( const wp_renderer_dx12 *renderer );
wp_s32 wp_renderer_dx12_get_height( const wp_renderer_dx12 *renderer );
void wp_renderer_dx12_get_projection_matrix( wp_s32 width, wp_s32 height, wp_f32 *result );

/* =========================================================================
 * Native object access
 * ====================================================================== */
void wp_renderer_dx12_get_native( const wp_renderer_dx12 *renderer, void **pp_object );
void wp_renderer_dx12_set_native( wp_renderer_dx12 *renderer, void *native );

/* =========================================================================
 * DX12-specific accessors
 * ====================================================================== */
void *wp_renderer_dx12_get_device( const wp_renderer_dx12 *renderer );
void *wp_renderer_dx12_get_command_queue( const wp_renderer_dx12 *renderer );
void *wp_renderer_dx12_get_swap_chain( const wp_renderer_dx12 *renderer );

#    ifdef __cplusplus
}
#    endif

#endif /* _WIN32 */

#endif /* WORKPHONE_GRAPHICS_RENDERER_DX12_H */
