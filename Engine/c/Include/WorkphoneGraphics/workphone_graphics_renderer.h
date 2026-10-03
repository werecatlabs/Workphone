#ifndef WORKPHONE_RENDERER_H
#define WORKPHONE_RENDERER_H

#include <stdint.h>
#include "workphone_vector.h"
#include "workphone_matrix.h"
#include "workphone_graphics_vertex.h"
#include "workphone_graphics_viewport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wp_renderer wp_renderer;
typedef struct wp_renderer_software wp_renderer_software;
typedef struct wp_renderer_dx11 wp_renderer_dx11;
typedef struct wp_renderer_dx12 wp_renderer_dx12;
typedef wp_s32 ( *wp_renderer_program_reset_func )( wp_renderer *renderer, void *user_data );

typedef enum wp_renderer_type
{
    WORKPHONE_RENDERER_TYPE_SOFTWARE = 0,
    WORKPHONE_RENDERER_TYPE_DX11 = 1,
    WORKPHONE_RENDERER_TYPE_DX12 = 2
} wp_renderer_type;

wp_renderer *wp_renderer_create_software( wp_s32 width, wp_s32 height, wp_pixel_format format );

wp_renderer *wp_renderer_create_dx11( void *hwnd, wp_s32 width, wp_s32 height );

wp_renderer *wp_renderer_create_dx12( void *hwnd, wp_s32 width, wp_s32 height );

void wp_renderer_destroy( wp_renderer *renderer );

wp_renderer_type wp_renderer_get_type( const wp_renderer *renderer );

void wp_renderer_set_program_reset_func( wp_renderer *renderer,
                                         wp_renderer_program_reset_func reset_func, void *user_data );
void wp_renderer_mark_program_bound( wp_renderer *renderer );
wp_s32 wp_renderer_reset_program( wp_renderer *renderer );
wp_s32 wp_renderer_has_program_bound( const wp_renderer *renderer );

wp_renderer_software *wp_renderer_get_software( const wp_renderer *renderer );

wp_renderer_dx11 *wp_renderer_get_dx11( const wp_renderer *renderer );

void *wp_renderer_get_dx11_device( const wp_renderer *renderer );

wp_s32 wp_renderer_resize( wp_renderer *renderer, wp_s32 width, wp_s32 height );

void wp_renderer_begin_frame( wp_renderer *renderer );

void wp_renderer_end_frame( wp_renderer *renderer );

void wp_renderer_set_clear_color( wp_renderer *renderer, wp_f32 r, wp_f32 g, wp_f32 b, wp_f32 a );

void wp_renderer_set_clear_depth( wp_renderer *renderer, wp_f32 depth );

void wp_renderer_clear( wp_renderer *renderer, wp_u32 flags );

void wp_renderer_set_viewport( wp_renderer *renderer, wp_viewport_i viewport );

wp_viewport_i wp_renderer_get_viewport( const wp_renderer *renderer );

void wp_renderer_set_scissor_enabled( wp_renderer *renderer, wp_s32 enabled );

void wp_renderer_set_scissor_rect( wp_renderer *renderer, wp_viewport_i scissor );

const void *wp_renderer_get_framebuffer( const wp_renderer *renderer );

wp_s32 wp_renderer_get_width( const wp_renderer *renderer );

wp_s32 wp_renderer_get_height( const wp_renderer *renderer );

void wp_renderer_set_blend_mode( wp_renderer *renderer, wp_blend_mode mode );

wp_blend_mode wp_renderer_get_blend_mode( const wp_renderer *renderer );

void wp_renderer_set_fill_mode( wp_renderer *renderer, wp_fill_mode mode );

wp_fill_mode wp_renderer_get_fill_mode( const wp_renderer *renderer );

void wp_renderer_set_cull_mode( wp_renderer *renderer, wp_cull_mode mode );

wp_cull_mode wp_renderer_get_cull_mode( const wp_renderer *renderer );

void wp_renderer_set_depth_test_enabled( wp_renderer *renderer, wp_s32 enabled );

wp_s32 wp_renderer_get_depth_test_enabled( const wp_renderer *renderer );

void wp_renderer_set_depth_write_enabled( wp_renderer *renderer, wp_s32 enabled );

wp_s32 wp_renderer_get_depth_write_enabled( const wp_renderer *renderer );

void wp_renderer_set_depth_func( wp_renderer *renderer, wp_depth_func func );

wp_depth_func wp_renderer_get_depth_func( const wp_renderer *renderer );

void wp_renderer_set_world_matrix( wp_renderer *renderer, const wp_mat4f *mat );

void wp_renderer_set_view_matrix( wp_renderer *renderer, const wp_mat4f *mat );

void wp_renderer_set_projection_matrix( wp_renderer *renderer, const wp_mat4f *mat );

/**
 * @brief Binds pixel data for subsequent PTC draw calls.
 *
 * Passing NULL unbinds the texture and makes PTC draws use an opaque white
 * texel. DX11 copies the supplied pixels during this call.
 */
void wp_renderer_set_texture( wp_renderer *renderer, const void *pixels, wp_s32 width, wp_s32 height,
                              wp_pixel_format format );

void wp_renderer_set_texture_native( wp_renderer *renderer, void *texture_view );

void wp_renderer_set_render_target_native( wp_renderer *renderer, void *render_target_view );

void wp_renderer_draw_triangles_pc( wp_renderer *renderer, const wp_vertex_pc *vertices,
                                    wp_s32 vertex_count );

void wp_renderer_draw_indexed_triangles_pc( wp_renderer *renderer, const wp_vertex_pc *vertices,
                                            wp_s32 vertex_count, const uint16_t *indices,
                                            wp_s32 index_count );

void wp_renderer_draw_lines_pc( wp_renderer *renderer, const wp_vertex_pc *vertices,
                                wp_s32 vertex_count );

void wp_renderer_draw_points_pc( wp_renderer *renderer, const wp_vertex_pc *vertices,
                                 wp_s32 vertex_count );

void wp_renderer_draw_triangles_ptc( wp_renderer *renderer, const wp_vertex_ptc *vertices,
                                     wp_s32 vertex_count );

void wp_renderer_draw_indexed_triangles_ptc( wp_renderer *renderer, const wp_vertex_ptc *vertices,
                                             wp_s32 vertex_count, const uint16_t *indices,
                                             wp_s32 index_count );

void wp_renderer_draw_indexed_triangles_ptc_u32( wp_renderer *renderer, const wp_vertex_ptc *vertices,
                                                 wp_s32 vertex_count, const wp_u32 *indices,
                                                 wp_s32 index_count );

void wp_renderer_get_native( const wp_renderer *renderer, void **pp_object );

void wp_renderer_set_native( wp_renderer *renderer, void *native );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_RENDERER_H */
