/**
 * @file workphone_graphics_renderer_dx11.h
 * @brief C API for the Direct3D 11 hardware renderer.
 *
 * wp_renderer_dx11 provides a hardware-accelerated rendering back-end using
 * Direct3D 11.  It mirrors the interface of wp_renderer_software so that
 * the abstract wp_renderer front-end can dispatch to either implementation.
 *
 * The renderer is created around an existing Win32 HWND (typically obtained
 * from wp_platform_window_win32_get_hwnd()).  Internally it owns the D3D11
 * device, device-context, swap-chain, render-target, depth-stencil buffer,
 * shaders, and all pipeline state objects.
 */

#ifndef WORKPHONE_GRAPHICS_RENDERER_DX11_H
#define WORKPHONE_GRAPHICS_RENDERER_DX11_H

#ifdef _WIN32
#    include <stdint.h>
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

typedef struct wp_renderer_dx11 wp_renderer_dx11;
typedef struct wp_render_texture_dx11 wp_render_texture_dx11;
typedef struct wp_geometry_dx11 wp_geometry_dx11;

/** Parameters consumed by the built-in forward PBR mesh shader. */
typedef struct wp_material_dx11
{
    wp_vec4f base_color;
    wp_vec4f emissive_color;
    wp_vec4f specular_color;    /* rgb dielectric F0 reflectance (default 0.04) */
    wp_vec4f light_color;       /* rgb colour, w intensity */
    wp_vec4f light_direction;   /* xyz direction light travels */
    wp_vec4f camera_position;   /* xyz world position, w ambient strength */
    wp_vec4f surface;           /* metalness, roughness, uv scale x/y */
    wp_vec4f uv_transform;      /* uv offset x/y, lighting enabled, rotation degrees */
    wp_vec4f controls;          /* normal strength, AO strength, alpha cutoff (-1 disables), premultiply */
    wp_vec4f map_flags;         /* normal, metallic, roughness, emission textures present */
    wp_vec4f extra_map_flags;   /* AO, opacity textures present, unused, unused */
    wp_vec4f texture_sources;   /* editor metallic, roughness, AO, opacity channel selectors */
    wp_vec4f projection;        /* mode: mesh/world box/object box/world XZ/XY/YZ/screen; scale, unused x2 */
    wp_vec4f ambient_color;     /* rgb ambient radiance; w enables RGB instead of legacy scalar */
    wp_vec4f environment;       /* x enabled, y maximum LOD, z authored probe radiance (otherwise ambient-scaled) */
} wp_material_dx11;

/** Directional depth pass. light_matrix uses the mesh shader's OpenGL clip convention.
 * End restores output targets, viewport and scissor. Caller submits caster geometry
 * with light_matrix, then enables shadow receiving for each colour draw. */
wp_s32 wp_renderer_dx11_begin_shadow_map( wp_renderer_dx11 *renderer, const wp_mat4f *light_matrix, wp_s32 size );
void wp_renderer_dx11_end_shadow_map( wp_renderer_dx11 *renderer );
void wp_renderer_dx11_enable_shadow_receiving( wp_renderer_dx11 *renderer, wp_s32 enabled );

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

/**
 * @brief Creates a DX11 renderer targeting the given Win32 window handle.
 *
 * @param hwnd   Win32 HWND to bind the swap-chain to.
 * @param width  Initial back-buffer width in pixels.  Must be > 0.
 * @param height Initial back-buffer height in pixels.  Must be > 0.
 * @return Pointer to the created renderer, or NULL on failure.
 */
wp_renderer_dx11 *wp_renderer_dx11_create( void *hwnd, wp_s32 width, wp_s32 height );

/**
 * @brief Destroys the renderer and releases all D3D11 resources.
 * @param renderer Pointer to the renderer.  Ignored if NULL.
 */
void wp_renderer_dx11_destroy( wp_renderer_dx11 *renderer );

/* =========================================================================
 * Resize
 * ====================================================================== */

/**
 * @brief Resizes the back-buffer and depth-stencil to new dimensions.
 *
 * @param renderer Pointer to the renderer.
 * @param width    New width in pixels.  Must be > 0.
 * @param height   New height in pixels.  Must be > 0.
 * @return Non-zero on success; zero on failure.
 */
wp_s32 wp_renderer_dx11_resize( wp_renderer_dx11 *renderer, wp_s32 width, wp_s32 height );

/* =========================================================================
 * Frame lifecycle
 * ====================================================================== */

void wp_renderer_dx11_begin_frame( wp_renderer_dx11 *renderer );
void wp_renderer_dx11_end_frame( wp_renderer_dx11 *renderer );

/* =========================================================================
 * Present
 * ====================================================================== */

/**
 * @brief Presents the current back-buffer to the screen.
 * @param renderer Pointer to the renderer.
 * @param vsync    Non-zero to synchronise to vertical blank.
 */
void wp_renderer_dx11_present( wp_renderer_dx11 *renderer, wp_s32 vsync );

/** Optional benchmark counters. Reset is requested from any thread and applied
 * at the next render frame. Read statistics on the render thread or under the
 * graphics system lock. GPU queries are polled without forcing a flush.
 * Counts include offscreen, window and UI passes. CPU frame time spans the
 * first begin_frame through Present, including the separately reported wait.
 * Intervals measure consecutive completed Presents; p95 uses at most the last
 * 8192 intervals. Other means and counters span the complete measurement. */
typedef struct wp_render_statistics_dx11
{
    uint64_t frames, interval_samples, gpu_samples, draws, triangles;
    uint64_t material_uploads, transform_uploads, geometry_creations, state_bindings;
    double interval_ms, interval_p95_ms, cpu_frame_ms, present_ms, gpu_frame_ms;
    wp_s32 flip_model;
} wp_render_statistics_dx11;
/** Call after external code (e.g. ImGui) changes the immediate context state. */
void wp_renderer_dx11_invalidate_state( wp_renderer_dx11 *renderer );
void wp_renderer_dx11_reset_statistics( wp_renderer_dx11 *renderer );
void wp_renderer_dx11_get_statistics( const wp_renderer_dx11 *renderer,
                                      wp_render_statistics_dx11 *statistics );

/* =========================================================================
 * Clear
 * ====================================================================== */

void wp_renderer_dx11_set_clear_color( wp_renderer_dx11 *renderer, wp_f32 r, wp_f32 g, wp_f32 b,
                                       wp_f32 a );
void wp_renderer_dx11_set_clear_depth( wp_renderer_dx11 *renderer, wp_f32 depth );
void wp_renderer_dx11_clear( wp_renderer_dx11 *renderer, wp_u32 flags );

/* =========================================================================
 * Viewport and scissor
 * ====================================================================== */

void wp_renderer_dx11_set_viewport( wp_renderer_dx11 *renderer, wp_viewport_i viewport );
wp_viewport_i wp_renderer_dx11_get_viewport( const wp_renderer_dx11 *renderer );
void wp_renderer_dx11_set_scissor_enabled( wp_renderer_dx11 *renderer, wp_s32 enabled );
void wp_renderer_dx11_set_scissor_rect( wp_renderer_dx11 *renderer, wp_viewport_i scissor );

/* =========================================================================
 * Render state
 * ====================================================================== */

void wp_renderer_dx11_set_blend_mode( wp_renderer_dx11 *renderer, wp_blend_mode mode );
wp_blend_mode wp_renderer_dx11_get_blend_mode( const wp_renderer_dx11 *renderer );

void wp_renderer_dx11_set_fill_mode( wp_renderer_dx11 *renderer, wp_fill_mode mode );
wp_fill_mode wp_renderer_dx11_get_fill_mode( const wp_renderer_dx11 *renderer );

void wp_renderer_dx11_set_cull_mode( wp_renderer_dx11 *renderer, wp_cull_mode mode );
wp_cull_mode wp_renderer_dx11_get_cull_mode( const wp_renderer_dx11 *renderer );

void wp_renderer_dx11_set_depth_test_enabled( wp_renderer_dx11 *renderer, wp_s32 enabled );
wp_s32 wp_renderer_dx11_get_depth_test_enabled( const wp_renderer_dx11 *renderer );

void wp_renderer_dx11_set_depth_write_enabled( wp_renderer_dx11 *renderer, wp_s32 enabled );
wp_s32 wp_renderer_dx11_get_depth_write_enabled( const wp_renderer_dx11 *renderer );

void wp_renderer_dx11_set_depth_func( wp_renderer_dx11 *renderer, wp_depth_func func );
wp_depth_func wp_renderer_dx11_get_depth_func( const wp_renderer_dx11 *renderer );

/* =========================================================================
 * Transform matrices
 * ====================================================================== */

void wp_renderer_dx11_set_world_matrix( wp_renderer_dx11 *renderer, const wp_mat4f *mat );
void wp_renderer_dx11_set_view_matrix( wp_renderer_dx11 *renderer, const wp_mat4f *mat );
void wp_renderer_dx11_set_projection_matrix( wp_renderer_dx11 *renderer, const wp_mat4f *mat );

/** Sets the material and scene-light constants used by PNTC mesh draws. */
void wp_renderer_dx11_set_material( wp_renderer_dx11 *renderer,
                                    const wp_material_dx11 *material );

/** Borrowed texture views in normal, metallic, roughness, emission, AO, opacity order.
 * Passing NULL clears all six slots. Albedo continues to use set_texture_native. */
void wp_renderer_dx11_set_material_textures( wp_renderer_dx11 *renderer, void *const *views );
/** Borrowed linear HDR TextureCube, filtered from smooth to rough across its mip chain. */
void wp_renderer_dx11_set_environment( wp_renderer_dx11 *renderer, void *view, wp_f32 max_lod );
/** Editor sampler modes: wrap 0..4, filter 0..3, anisotropy 1..16. */
void wp_renderer_dx11_set_material_sampler( wp_renderer_dx11 *renderer, wp_u32 wrap_u,
                                            wp_u32 wrap_v, wp_u32 filter, wp_u32 anisotropy );

/**
 * @brief Uploads a texture for subsequent PTC draw calls.
 *
 * The pixel data is copied into a renderer-owned D3D11 texture. Passing NULL
 * unbinds the texture so PTC draws use an opaque white texel.
 */
void wp_renderer_dx11_set_texture( wp_renderer_dx11 *renderer, const void *pixels, wp_s32 width,
                                   wp_s32 height, wp_pixel_format format );
void wp_renderer_dx11_set_texture_native( wp_renderer_dx11 *renderer, void *texture_view );
void wp_renderer_dx11_set_render_target_native( wp_renderer_dx11 *renderer,
                                                void *render_target_view );

/**
 * @brief Creates a standalone shader-resource view from CPU pixel data.
 *
 * Unlike wp_renderer_dx11_set_texture(), ownership is returned to the caller so
 * resource wrappers can cache the GPU texture across draw calls.
 */
void *wp_renderer_dx11_create_texture_native( wp_renderer_dx11 *renderer, const void *pixels,
                                               wp_s32 width, wp_s32 height,
                                               wp_pixel_format format );
void wp_renderer_dx11_destroy_texture_native( void *texture_view );

/**
 * @brief Creates an off-screen colour/depth target that can also be sampled as a texture.
 */
wp_render_texture_dx11 *wp_renderer_dx11_create_render_texture( wp_renderer_dx11 *renderer,
                                                                 wp_s32 width, wp_s32 height );
void wp_renderer_dx11_destroy_render_texture( wp_render_texture_dx11 *render_texture );
void wp_renderer_dx11_set_render_texture( wp_renderer_dx11 *renderer,
                                          wp_render_texture_dx11 *render_texture );
void *wp_renderer_dx11_get_render_texture_resource(
    const wp_render_texture_dx11 *render_texture );
void *wp_renderer_dx11_get_render_texture_view( const wp_render_texture_dx11 *render_texture );
void *wp_renderer_dx11_get_render_texture_target_view(
    const wp_render_texture_dx11 *render_texture );

/* =========================================================================
 * Draw calls
 * ====================================================================== */

void wp_renderer_dx11_draw_triangles_pc( wp_renderer_dx11 *renderer, const wp_vertex_pc *vertices,
                                         wp_s32 vertex_count );

void wp_renderer_dx11_draw_indexed_triangles_pc( wp_renderer_dx11 *renderer,
                                                 const wp_vertex_pc *vertices, wp_s32 vertex_count,
                                                 const uint16_t *indices, wp_s32 index_count );

void wp_renderer_dx11_draw_lines_pc( wp_renderer_dx11 *renderer, const wp_vertex_pc *vertices,
                                     wp_s32 vertex_count );

void wp_renderer_dx11_draw_points_pc( wp_renderer_dx11 *renderer, const wp_vertex_pc *vertices,
                                      wp_s32 vertex_count );

void wp_renderer_dx11_draw_triangles_ptc( wp_renderer_dx11 *renderer, const wp_vertex_ptc *vertices,
                                          wp_s32 vertex_count );

void wp_renderer_dx11_draw_indexed_triangles_ptc( wp_renderer_dx11 *renderer,
                                                  const wp_vertex_ptc *vertices, wp_s32 vertex_count,
                                                  const uint16_t *indices, wp_s32 index_count );

void wp_renderer_dx11_draw_indexed_triangles_ptc_u32(
    wp_renderer_dx11 *renderer, const wp_vertex_ptc *vertices, wp_s32 vertex_count,
    const uint32_t *indices, wp_s32 index_count );

/**
 * @brief Uploads and binds PTC geometry once for multiple indexed range draws.
 *
 * Texture, scissor and blend state may be changed between prepared draws without
 * uploading the vertex/index data again.
 */
wp_s32 wp_renderer_dx11_prepare_indexed_triangles_ptc(
    wp_renderer_dx11 *renderer, const wp_vertex_ptc *vertices, wp_s32 vertex_count,
    const uint16_t *indices, wp_s32 index_count );
wp_s32 wp_renderer_dx11_prepare_indexed_triangles_ptc_u32(
    wp_renderer_dx11 *renderer, const wp_vertex_ptc *vertices, wp_s32 vertex_count,
    const uint32_t *indices, wp_s32 index_count );
void wp_renderer_dx11_draw_prepared_indexed_triangles_ptc(
    wp_renderer_dx11 *renderer, wp_s32 index_start, wp_s32 index_count,
    wp_s32 base_vertex );

/** Creates immutable indexed PTC geometry for repeated mesh draws. */
wp_geometry_dx11 *wp_renderer_dx11_create_indexed_geometry_ptc(
    wp_renderer_dx11 *renderer, const wp_vertex_ptc *vertices, wp_s32 vertex_count,
    const void *indices, wp_s32 index_count, wp_s32 indices_are_u32 );
void wp_renderer_dx11_destroy_geometry( wp_geometry_dx11 *geometry );
void wp_renderer_dx11_draw_geometry_ptc( wp_renderer_dx11 *renderer,
                                         const wp_geometry_dx11 *geometry,
                                         wp_s32 index_start, wp_s32 index_count,
                                         wp_s32 base_vertex );

/** Creates and draws immutable indexed geometry through the lit PNTC pipeline. */
wp_geometry_dx11 *wp_renderer_dx11_create_indexed_geometry_pntc(
    wp_renderer_dx11 *renderer, const wp_vertex_pntc *vertices, wp_s32 vertex_count,
    const void *indices, wp_s32 index_count, wp_s32 indices_are_u32 );
void wp_renderer_dx11_draw_geometry_pntc( wp_renderer_dx11 *renderer,
                                          const wp_geometry_dx11 *geometry,
                                          wp_s32 index_start, wp_s32 index_count,
                                          wp_s32 base_vertex );

/* =========================================================================
 * Dimension queries
 * ====================================================================== */

wp_s32 wp_renderer_dx11_get_width( const wp_renderer_dx11 *renderer );
wp_s32 wp_renderer_dx11_get_height( const wp_renderer_dx11 *renderer );

/* =========================================================================
 * Native object access
 * ====================================================================== */

void wp_renderer_dx11_get_native( const wp_renderer_dx11 *renderer, void **pp_object );
void wp_renderer_dx11_set_native( wp_renderer_dx11 *renderer, void *native );

/* =========================================================================
 * DX11-specific accessors
 * ====================================================================== */

void *wp_renderer_dx11_get_device( const wp_renderer_dx11 *renderer );
void *wp_renderer_dx11_get_context( const wp_renderer_dx11 *renderer );
void *wp_renderer_dx11_get_swap_chain( const wp_renderer_dx11 *renderer );

void wp_renderer_dx11_font_stash_begin( wp_renderer_dx11 *r, struct wp_font_atlas **atlas );
void wp_renderer_dx11_font_stash_end( wp_renderer_dx11 *r );

typedef struct ID3D11DeviceContext ID3D11DeviceContext;
typedef struct ID3D11Device ID3D11Device;
typedef struct HWND__ *HWND;
typedef unsigned int UINT;

#    if defined( _WIN64 )
typedef __int64 INT_PTR, *PINT_PTR;
typedef unsigned __int64 UINT_PTR, *PUINT_PTR;

typedef __int64 LONG_PTR, *PLONG_PTR;
typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;

#        define __int3264 __int64

#    else
typedef _W64 wp_s32 INT_PTR, *PINT_PTR;
typedef _W64 wp_u32 UINT_PTR, *PUINT_PTR;

typedef _W64 long LONG_PTR, *PLONG_PTR;
typedef _W64 unsigned long ULONG_PTR, *PULONG_PTR;

#        define __int3264 __int32

#    endif

typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;

struct wp_d3d11_vertex
{
    float position[2];
    float uv[2];
    wp_byte col[4];
};

WORKPHONE_API void wp_renderer_dx11_render( wp_renderer_dx11 *r, enum wp_anti_aliasing AA );

WORKPHONE_API void wp_renderer_dx11_get_projection_matrix( wp_s32 width, wp_s32 height, wp_f32 *result );

//WORKPHONE_API void wp_renderer_dx11_resize( wp_renderer_dx11 *r, ID3D11DeviceContext *context,
//                                            wp_s32 width, wp_s32 height );

WORKPHONE_API wp_s32 wp_renderer_dx11_handle_event( wp_renderer_dx11 *r, HWND wnd, UINT msg,
                                                    WPARAM wparam, LPARAM lparam );

WORKPHONE_API void wp_renderer_dx11_clipboard_paste( wp_handle usr, struct wp_text_edit *edit );

WORKPHONE_API void wp_renderer_dx11_clipboard_copy( wp_handle usr, const wp_c8 *text, wp_s32 len );

WORKPHONE_API struct wp_context *wp_renderer_dx11_init( wp_renderer_dx11 *r, wp_s32 width, wp_s32 height,
                                                        wp_u32 max_vertex_buffer,
                                                        wp_u32 max_index_buffer );

#    ifdef __cplusplus
}
#    endif

#endif /* _WIN32 */

#endif /* WORKPHONE_GRAPHICS_RENDERER_DX11_H */
