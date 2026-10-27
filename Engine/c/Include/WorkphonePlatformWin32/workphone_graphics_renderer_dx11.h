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

#include "workphone_types.h"

#ifdef _WIN32
#    include <stdint.h>
#    include "workphone_vector.h"
#    include "workphone_matrix.h"
#    include "workphone_graphics_vertex.h"
#    include "workphone_graphics_viewport.h"
#    include "workphone_graphics_rtt_job.h"

#    ifdef __cplusplus
extern "C" {
#    endif

/* -------------------------------------------------------------------------
 * Opaque type
 * ---------------------------------------------------------------------- */

typedef struct wp_renderer_dx11 wp_renderer_dx11;
typedef struct wp_render_texture_dx11 wp_render_texture_dx11;
typedef struct wp_geometry_dx11 wp_geometry_dx11;

/* GPU scene-linear RGBA16F staging for a single non-MSAA 2D UNORM/sRGB output.
 * Render-owner calls only. Begin rejects invalid/nested requests without changing
 * bindings. Positive finite manual exposure is clamped to .01..64. End composites
 * the native HDR filmic policy once and restores the saved output/depth/viewport.
 * The returned SRV is borrowed until a successful resize or renderer destruction;
 * clients must not leave it bound when beginning another scene. No CPU readback. */
wp_s32 wp_renderer_dx11_begin_hdr_scene( wp_renderer_dx11 *renderer, wp_f32 exposure );
void wp_renderer_dx11_set_output_options( wp_renderer_dx11 *renderer, wp_s32 hdr_enabled,
                                         wp_s32 linear_workflow, wp_s32 tonemapping );
void wp_renderer_dx11_set_scene_sampling( wp_renderer_dx11 *renderer, wp_s32 samples, wp_f32 scale );
void wp_renderer_dx11_set_post_options( wp_renderer_dx11 *renderer, wp_s32 anti_aliasing,
                                       wp_s32 bloom, wp_s32 motion_blur, wp_u64 temporal_key );
/* Up to four view-local SH9 probes: each is position float4 followed by nine
 * convolved irradiance RGB float4 coefficients (40 floats per probe). */
wp_s32 wp_renderer_dx11_set_irradiance_probes( wp_renderer_dx11 *renderer,
                                             const void *probes, wp_u32 count, wp_s32 mode );
/* Applies to subsequent device creation and built-in shader compilation. */
void wp_renderer_dx11_set_startup_options( wp_s32 mode, wp_s32 shader_model,
                                          wp_s32 validation, wp_s32 allow_fallback );
/* Optional compiler returns 1 and an owned ID3DBlob, 0 for the built-in HLSL
 * compiler, or -1 on an actionable provider error. Feature defines are already
 * prepended to source. The bridge retains the full project profile itself. */
typedef wp_s32 (*wp_dx11_shader_compiler_callback)(void *user, const wp_c8 *source,
    const wp_c8 *entry, const wp_c8 *target, void **native_blob);
void wp_renderer_dx11_set_shader_compiler( wp_dx11_shader_compiler_callback callback, void *user );
wp_u32 wp_renderer_dx11_supported_scene_samples( wp_renderer_dx11 *renderer );
/* Borrowed scene depth, valid until the next scene-target allocation. The
 * resource is R24G8_TYPELESS and supports R24_UNORM_X8_TYPELESS SRV reads. */
void *wp_renderer_dx11_get_scene_depth_texture( wp_renderer_dx11 *renderer );
void wp_renderer_dx11_end_hdr_scene( wp_renderer_dx11 *renderer );
wp_s32 wp_renderer_dx11_is_hdr_scene_active( const wp_renderer_dx11 *renderer );
void *wp_renderer_dx11_get_hdr_scene_view( const wp_renderer_dx11 *renderer );
/* Lifetime successful GPU resolves; independent of resettable frame statistics. */
wp_u64 wp_renderer_dx11_get_hdr_completed_frames( const wp_renderer_dx11 *renderer );

/* Renderer-owned immutable mesh geometry. Triangle lists cache the complete
 * mesh; apply first/count when drawing the returned geometry. Strip ranges are
 * converted to lists starting at zero with submesh-relative winding. Geometry is
 * borrowed until mesh mutation, forget, or renderer destruction. All calls
 * are serialized on the graphics render owner. Native mesh owners must call
 * forget_mesh before destruction to release cached GPU allocations eagerly. */
struct wp_graphics_mesh;
wp_geometry_dx11 *wp_renderer_dx11_prepare_mesh_geometry( wp_renderer_dx11 *renderer,
    struct wp_graphics_mesh *mesh, wp_u32 first, wp_u32 count );
void wp_renderer_dx11_forget_mesh( const struct wp_graphics_mesh *mesh );

/* Optional owner-provided allocation admission. reserve returns a token for the
 * full immutable vertex/index allocation, retain attaches it to each physical
 * D3D resource, and release drops the temporary token. Callbacks never escape
 * this call. Cache hits do not reserve again. Existing terrain/foliage admission
 * continues to use prepare_mesh_geometry without this additional reservation. */
typedef struct wp_gpu_allocation_callbacks
{
    void *user;
    void *( *reserve )( void *user, wp_u64 bytes );
    wp_s32 ( *retain )( void *reservation, void *native_resource );
    void ( *release )( void *reservation );
} wp_gpu_allocation_callbacks;
wp_geometry_dx11 *wp_renderer_dx11_prepare_mesh_geometry_budgeted( wp_renderer_dx11 *renderer,
    struct wp_graphics_mesh *mesh, wp_u32 first, wp_u32 count,
    const wp_gpu_allocation_callbacks *callbacks );

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
    wp_vec4f extra_map_flags;   /* AO, opacity present; albedo/emission samples already linear (draw-owned) */
    wp_vec4f texture_sources;   /* editor metallic, roughness, AO, opacity channel selectors */
    wp_vec4f projection;        /* mode: mesh/world box/object box/world XZ/XY/YZ/screen; scale, unused x2 */
    wp_vec4f ambient_color;     /* rgb ambient radiance; w enables RGB instead of legacy scalar */
    wp_vec4f environment;       /* x enabled, y maximum LOD, z authored probe radiance (otherwise ambient-scaled) */
    wp_vec4f fog_color;         /* linear scene fog radiance */
    wp_vec4f fog_params;        /* mode (0 none, 1 exp, 2 exp2, 3 linear), density, start, end */
} wp_material_dx11;

/* Owner-thread upload policy for built-in PNTC/terrain/foliage shaders. Combined
 * remains the default. Split lazily creates surface/view/binding buffers and a
 * shader variant; failure preserves the previous policy. Changes reject during
 * transactional RTT, HDR or shadow passes. Equal assignments are harmless. */
enum { WP_DX11_MATERIAL_UPLOAD_COMBINED = 0, WP_DX11_MATERIAL_UPLOAD_SPLIT = 1 };
wp_s32 wp_renderer_dx11_set_material_upload_mode( wp_renderer_dx11 *, wp_s32 mode );
wp_s32 wp_renderer_dx11_get_material_upload_mode( const wp_renderer_dx11 * );

/* Successful CPU writes into dynamic constant buffers, not GPU traffic/cache
 * counters. Reset with the existing statistics reset at the next frame. The
 * legacy material_uploads counter still counts completed logical flushes.
 * Initialize struct_size. Too-small/null inputs reject without modifying output;
 * larger callers receive this known prefix only, with struct_size set to it. */
typedef struct wp_material_upload_statistics_dx11
{
    wp_u32 struct_size;
    wp_u64 combined_uploads, combined_bytes;
    wp_u64 surface_uploads, surface_bytes;
    wp_u64 view_uploads, view_bytes;
    wp_u64 binding_uploads, binding_bytes;
    wp_u64 map_failures;
} wp_material_upload_statistics_dx11;
wp_s32 wp_renderer_dx11_get_material_upload_statistics(
    const wp_renderer_dx11 *, wp_material_upload_statistics_dx11 * );

/** Four-layer terrain constants. Texture slots are albedo/normal/ORM per layer,
 * followed by a linear RGBA control map. UV scales are cycles per local metre;
 * control_transform maps local XZ to texel-centred coordinates over the full grid.
 * ORM channels are occlusion, roughness, metallic. Missing maps use unit defaults.
 * A zero control sum selects layer zero. Binding borrows views until unbound. */
typedef struct wp_terrain_material_dx11
{
    wp_vec4f control_transform; /* x/z offset, inverse extent (with texel centres) */
    wp_vec4f controls; /* enabled, control map present, reserved, reserved */
    wp_vec4f tint[4];
    wp_vec4f surface[4]; /* UV scale x/y, roughness multiplier, metallic multiplier */
    wp_vec4f maps[4]; /* albedo present, normal present, ORM present, normal strength */
    wp_vec4f height[4]; /* ORM-alpha height scale, bias; view-owned linear albedo samples; reserved */
    wp_vec4f appearance; /* projection 0 planar/1 triplanar, sharpness, height blend, transition */
    wp_vec4f detail; /* UV scale x/y, normal strength, detail texture present */
    wp_vec4f macro; /* local cycles/metre, strength, seed low/high 16-bit halves */
    wp_vec4f macro_tint;
    wp_vec4f weather; /* wetness, wet darkening, wet roughness, snow */
    wp_vec4f snow; /* snow roughness, surface mask present, reserved, reserved */
    wp_vec4f snow_tint;
} wp_terrain_material_dx11;

enum { WP_TERRAIN_MATERIAL_VIEWS = 15 };
/** Atomically validate/install 12 layer views, control, detail normal and wet/snow mask.
 * NULL disables terrain.
 * Invalid constants/views leave the previous complete binding unchanged. */
wp_s32 wp_renderer_dx11_set_terrain_material( wp_renderer_dx11 *renderer,
    const wp_terrain_material_dx11 *material, void *const *views );
wp_s32 wp_renderer_dx11_validate_terrain_material( wp_renderer_dx11 *renderer,
    const wp_terrain_material_dx11 *material, void *const *views );

/** Directional depth pass. light_matrix uses the mesh shader's OpenGL clip convention.
 * End restores output targets, viewport and scissor. Caller submits caster geometry
 * with light_matrix, then enables shadow receiving for each colour draw. */
wp_s32 wp_renderer_dx11_begin_shadow_map( wp_renderer_dx11 *renderer, const wp_mat4f *light_matrix, wp_s32 size );
void wp_renderer_dx11_end_shadow_map( wp_renderer_dx11 *renderer );
void wp_renderer_dx11_enable_shadow_receiving( wp_renderer_dx11 *renderer, wp_s32 enabled );
void wp_renderer_dx11_set_shadow_filter_quality( wp_renderer_dx11 *renderer, wp_s32 quality );

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
/** Checked presentation on the render owner. Rejects a different window,
 * offscreen target, or unfinished HDR/shadow pass without calling DXGI.
 * Returns 1 for S_OK, 2 for occlusion, -1 for a DXGI failure, or 0 for rejection.
 * Occlusion is not a completed submitted frame. The legacy entry point uses this
 * same path with the renderer's window. */
wp_s32 wp_renderer_dx11_try_present( wp_renderer_dx11 *renderer,
                                     void *expected_hwnd, wp_s32 vsync );
/** Copies the current window back buffer before Present as top-down, packed RGBA8.
 * expected_hwnd must match the swap chain's window. An invalid request leaves
 * output unchanged. Front-buffer capture after Present is not supported. */
wp_s32 wp_renderer_dx11_copy_back_buffer_rgba8(
    wp_renderer_dx11 *renderer, void *expected_hwnd, void *output, wp_u64 output_bytes );

/** Optional benchmark counters. Reset is requested from any thread and applied
 * at the next render frame. Read statistics on the render thread or under the
 * graphics system lock. GPU queries are polled without forcing a flush.
 * Counts include offscreen, window and UI passes. CPU frame time spans the
 * first begin_frame through Present, including the separately reported wait.
 * Only S_OK Presents complete measured frames. Occlusion/failure breaks the
 * interval sequence. Intervals measure consecutive completed Presents; p95 uses at most the last
 * 8192 intervals. Other means and counters span the complete measurement. */
typedef struct wp_render_statistics_dx11
{
    uint64_t frames, interval_samples, gpu_samples, draws, triangles;
    uint64_t material_uploads, transform_uploads, geometry_creations, state_bindings;
    uint64_t instanced_draws, instances, instance_uploads, instance_upload_bytes;
    uint64_t instance_buffer_creations;
    double interval_ms, interval_p95_ms, cpu_frame_ms, present_ms, gpu_frame_ms;
    wp_s32 flip_model;
    uint64_t present_attempts, present_occluded, present_failures;
    wp_s32 last_present_hresult; /* Raw DXGI HRESULT; render-thread diagnostics. */
} wp_render_statistics_dx11;
/** Call after external code (e.g. ImGui) changes the immediate context state. */
void wp_renderer_dx11_invalidate_state( wp_renderer_dx11 *renderer );
void wp_renderer_dx11_reset_statistics( wp_renderer_dx11 *renderer );
void wp_renderer_dx11_set_gpu_profiling( wp_renderer_dx11 *renderer, wp_s32 enabled );
/** Fixed-cost numeric readback: copies counters and computes aggregate means,
 * without allocating, sorting or traversing frame history. interval_p95_ms is
 * zero/unavailable in this path; use get_statistics for the history percentile.
 * The same render-thread/graphics-lock ownership requirement applies. */
void wp_renderer_dx11_get_statistics_counters( const wp_renderer_dx11 *renderer,
                                               wp_render_statistics_dx11 *statistics );
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
typedef struct wp_texture_mip_dx11
{
    const void *bgra;
    wp_u32 width, height;
} wp_texture_mip_dx11;
/** Immutable BGRA8 mip chain; dimensions halve at each level. */
void *wp_renderer_dx11_create_texture_mips_native( wp_renderer_dx11 *renderer,
    const wp_texture_mip_dx11 *levels, wp_u32 level_count );

/** Immutable RGBA32F sampled texture. The exact float count is required and
 * upload is rejected above the 256 MiB texture budget. Returns an owned SRV.
 */
void *wp_renderer_dx11_create_texture_rgba32f_native( wp_renderer_dx11 *renderer,
    const wp_f32 *rgba, wp_u32 width, wp_u32 height, wp_u64 float_count );

typedef struct wp_texture_cube_mip_dx11
{
    const wp_f32 *rgba;
    wp_u32 width, height;
    wp_u64 float_count;
} wp_texture_cube_mip_dx11;
/** Immutable RGBA32F environment cube with six faces and eight mips per face.
 * Levels are face-major and every mip must contain its exact pixel count.
 * The returned SRV is caller-owned; release with destroy_texture_native().
 */
void *wp_dx11_create_cubemap_rgba32f_native( void *device,
    const wp_texture_cube_mip_dx11 *levels, wp_u32 level_count );

typedef struct wp_dx11_cubemap_owner wp_dx11_cubemap_owner;
/** Publishes a complete filtered cube and retains each non-NULL source view.
 * Sources must be all present or all absent and belong to the supplied device.
 * The owner keeps the cube and source identities alive until destruction.
 */
wp_dx11_cubemap_owner *wp_dx11_cubemap_owner_create( void *device,
    const wp_texture_cube_mip_dx11 *levels, wp_u32 level_count,
    void *const source_views[6] );
void wp_dx11_cubemap_owner_destroy( wp_dx11_cubemap_owner *owner );
void *wp_dx11_cubemap_owner_get_view( const wp_dx11_cubemap_owner *owner );
wp_s32 wp_dx11_cubemap_owner_matches( const wp_dx11_cubemap_owner *owner,
    void *device, void *const source_views[6] );
/** Returns whether a shader view belongs to the supplied DX11 device. */
wp_s32 wp_dx11_texture_view_matches_device( void *view, void *device );
wp_s32 wp_dx11_device_matches_context( void *device, void *context );

enum wp_dx11_face_format
{
    WP_DX11_FACE_BGRA8 = 1,
    WP_DX11_FACE_RGBA8 = 2,
    WP_DX11_FACE_RGBA16F = 3,
    WP_DX11_FACE_RGBA32F = 4
};
typedef struct wp_texture_face_readback_dx11
{
    void *pixels; /* tightly packed rows; release with release_texture_face_readback_native */
    wp_u32 width, height, row_pitch, format;
} wp_texture_face_readback_dx11;
/** Reads the selected mip of a same-device 2D SRV, up to 2048x2048.
 * Pass a zero-initialized output and release it before reuse. On failure the
 * output is empty. This synchronous readback is for infrequent
 * cubemap preparation, not the per-frame render path.
 */
wp_s32 wp_dx11_read_texture_face_native( void *device, void *context, void *view,
    wp_texture_face_readback_dx11 *output );
void wp_dx11_release_texture_face_readback_native( wp_texture_face_readback_dx11 *output );

typedef struct wp_texture_encoded_mip_dx11
{
    const void *bytes;
    wp_u32 width, height, byte_count;
} wp_texture_encoded_mip_dx11;
/* Whitelisted DXGI BC1/BC3/BGRA8 linear/SRGB formats. Exact pitches and byte
 * counts are computed and checked before device creation. Returns an owned SRV. */
void *wp_renderer_dx11_create_texture_encoded_mips_native( wp_renderer_dx11 *renderer,
    const wp_texture_encoded_mip_dx11 *levels, wp_u32 count, wp_u32 dxgi_format );

/**
 * @brief Creates an off-screen colour/depth target that can also be sampled as a texture.
 */
wp_render_texture_dx11 *wp_renderer_dx11_create_render_texture( wp_renderer_dx11 *renderer,
                                                                 wp_s32 width, wp_s32 height );
void wp_renderer_dx11_destroy_render_texture( wp_render_texture_dx11 *render_texture );
/* Synchronous, transactional RGBA8-UNORM offscreen job (legacy LDR draw policy).
 * Scene-linear/HDR output formats are explicitly unsupported in this tier. Requires D3D11.1
 * context-state support. First begin_frame/RTT execution claims its owner thread.
 * Target/renderer stay alive throughout this call; no worker dispatch or queued
 * ownership. Resource generations must match the queries below (including extent).
 * Scene/view generations are checked by the supplied validate callback, if any;
 * a NULL callback selects the existing synchronous scene path, not frozen culling.
 * Dependency is optional but must report immutable publication before OK.
 * No nested RTT, open HDR/shadow scope, sampled destination or mid-pass resize.
 * Copy-on-success preserves prior colour/depth on failure. Scratch storage is
 * reused per target, bounded to 256 MiB colour+depth. Callbacks may draw but must
 * not destroy/rebind output resources or mutate renderer lifetime. Exceptions
 * must be caught in the C++ adapter. No GPU completion wait is implied by OK. */
wp_u64 wp_renderer_dx11_get_rtt_generation(const wp_renderer_dx11 *renderer);
wp_u64 wp_renderer_dx11_get_render_texture_generation(const wp_render_texture_dx11 *target);
wp_rtt_result wp_renderer_dx11_execute_rtt_job(wp_renderer_dx11 *renderer,
    wp_render_texture_dx11 *target, const wp_graphics_rtt_desc *desc,
    wp_rtt_check_func dependency, wp_rtt_check_func validate,
    wp_rtt_check_func draw, void *context);

void wp_renderer_dx11_set_render_texture( wp_renderer_dx11 *renderer,
                                          wp_render_texture_dx11 *render_texture );
void *wp_renderer_dx11_get_render_texture_resource(
    const wp_render_texture_dx11 *render_texture );
void *wp_renderer_dx11_get_render_texture_view( const wp_render_texture_dx11 *render_texture );
void *wp_renderer_dx11_get_render_texture_target_view(
    const wp_render_texture_dx11 *render_texture );
/** Copies the complete offscreen colour texture as tightly packed, top-down RGBA8.
 * Returns 0 without changing output if the target or output capacity is invalid.
 * The target must not be concurrently resized or destroyed. */
wp_s32 wp_renderer_dx11_copy_render_texture_rgba8(
    const wp_render_texture_dx11 *render_texture, void *output, wp_u64 output_bytes );

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

/** Optional owning-thread material activation, after built-in geometry state.
 * Return zero to defer submission. Must not throw or issue draws. The callback
 * is borrowed only for this call; the next ordinary draw restores built-in state. */
typedef wp_s32 (*wp_material_bind_dx11)(wp_renderer_dx11 *, wp_s32 instanced, void *);
wp_s32 wp_renderer_dx11_draw_geometry_pntc_bound(wp_renderer_dx11 *, const wp_geometry_dx11 *,
    wp_s32 start, wp_s32 count, wp_s32 base, wp_material_bind_dx11, void *);
wp_s32 wp_renderer_dx11_material_view_allowed(wp_renderer_dx11 *, void *view);

/** Instance transforms compose beneath the currently bound world matrix.
 * Positive-determinant affine transforms, including nonuniform scale, are
 * supported. Singular/reflected/nonfinite transforms or nonfinite tints reject
 * the whole request before drawing. Geometry/material resources remain shared. */
typedef struct wp_instance_pntc_dx11
{
    wp_mat4f transform;
    wp_vec4f tint;
} wp_instance_pntc_dx11;

#    define WP_DX11_INSTANCE_CAPACITY 4096
/** CPU-only preflight using precisely the draw path's transform/tint rules.
 * An empty population is valid, including a NULL pointer with count zero. */
wp_s32 wp_renderer_dx11_validate_instances_pntc(
    const wp_instance_pntc_dx11 *instances, wp_s32 instance_count );
/** Issues indexed instanced draws through the same PBR/cutout/shadow pipeline.
 * Upload storage is renderer-owned and bounded to WP_DX11_INSTANCE_CAPACITY.
 * A compatible group may span pages; callers separate mesh/material/pass/LOD
 * groups, while this function splits only at upload capacity. Returns the
 * number actually submitted (possibly partial on GPU upload failure). Zero
 * instances issue no draw. Statistics count successful native submissions. */
wp_s32 wp_renderer_dx11_draw_geometry_pntc_instanced(
    wp_renderer_dx11 *renderer, const wp_geometry_dx11 *geometry,
    wp_s32 index_start, wp_s32 index_count, wp_s32 base_vertex,
    const wp_instance_pntc_dx11 *instances, wp_s32 instance_count );

wp_s32 wp_renderer_dx11_draw_geometry_pntc_instanced_bound(wp_renderer_dx11 *,
    const wp_geometry_dx11 *, wp_s32 start, wp_s32 count, wp_s32 base,
    const wp_instance_pntc_dx11 *, wp_s32 instances, wp_material_bind_dx11, void *);

/** Foliage stream extends ordinary instances without changing their ABI. Wind
 * and interaction are local horizontal displacements in metres; phase is radians.
 * Atlas rectangles transform all material UVs together. Both passes run the same
 * deformation and inverse-Jacobian normal correction. */
typedef struct wp_foliage_instance_dx11
{
    wp_instance_pntc_dx11 instance;
    wp_vec4f uv_rect;
    wp_vec4f wind;        /* local x/z amplitude, phase, flutter amplitude */
    wp_vec4f interaction; /* local x/z bend; reserved z/w */
} wp_foliage_instance_dx11;
typedef struct wp_foliage_parameters_dx11
{
    wp_vec4f time;   /* seconds, frequency, reserved z/w */
    wp_vec4f height; /* bend start, bend end, reserved z/w */
} wp_foliage_parameters_dx11;
wp_s32 wp_renderer_dx11_validate_foliage_instances(
    const wp_foliage_instance_dx11 *, wp_s32 count, const wp_foliage_parameters_dx11 * );
wp_s32 wp_renderer_dx11_draw_foliage_instances(
    wp_renderer_dx11 *, const wp_geometry_dx11 *, wp_s32 index_start,
    wp_s32 index_count, const wp_foliage_instance_dx11 *, wp_s32 count,
    const wp_foliage_parameters_dx11 * );

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
