#include "workphone_graphics_renderer.h"
#include "workphone_graphics_renderer_software.h"
#include "workphone_graphics_renderer_dx11.h"
#include "workphone_graphics_renderer_dx12.h"
#include "workphone_graphics_viewport.h"
#include <stdlib.h>
#include <string.h>

typedef struct wp_renderer
{
    wp_renderer_type type;
    wp_renderer_software *software;
    wp_renderer_dx11 *dx11;
    wp_renderer_dx12 *dx12;
    void *native;
    wp_renderer_program_reset_func program_reset_func;
    void *program_reset_user_data;
    wp_s32 program_bound;
} wp_renderer;

wp_renderer *wp_renderer_create_software( wp_s32 width, wp_s32 height, wp_pixel_format format )
{
    wp_renderer *r;
    wp_renderer_software *sw;

    sw = wp_renderer_software_create( width, height, format );
    if( !sw )
    {
        return NULL;
    }

    r = (wp_renderer *)malloc( sizeof( wp_renderer ) );
    if( !r )
    {
        wp_renderer_software_destroy( sw );
        return NULL;
    }

    memset( r, 0, sizeof( wp_renderer ) );
    r->type = WORKPHONE_RENDERER_TYPE_SOFTWARE;
    r->software = sw;
    wp_renderer_set_native( r, sw );
    return r;
}

wp_renderer *wp_renderer_create_dx11( void *hwnd, wp_s32 width, wp_s32 height )
{
    wp_renderer *r;
    wp_renderer_dx11 *dx;

    dx = wp_renderer_dx11_create( hwnd, width, height );
    if( !dx )
    {
        return NULL;
    }

    r = (wp_renderer *)malloc( sizeof( wp_renderer ) );
    if( !r )
    {
        wp_renderer_dx11_destroy( dx );
        return NULL;
    }

    memset( r, 0, sizeof( wp_renderer ) );
    r->type = WORKPHONE_RENDERER_TYPE_DX11;
    r->dx11 = dx;
    wp_renderer_set_native( r, dx );
    return r;
}

wp_renderer *wp_renderer_create_dx12( void *hwnd, wp_s32 width, wp_s32 height )
{
    wp_renderer *r;
    wp_renderer_dx12 *dx;

    dx = wp_renderer_dx12_create( hwnd, width, height );
    if( !dx )
    {
        return NULL;
    }

    r = (wp_renderer *)malloc( sizeof( wp_renderer ) );
    if( !r )
    {
        wp_renderer_dx12_destroy( dx );
        return NULL;
    }

    memset( r, 0, sizeof( wp_renderer ) );
    r->type = WORKPHONE_RENDERER_TYPE_DX12;
    r->dx12 = dx;
    wp_renderer_set_native( r, dx );
    return r;
}

void wp_renderer_destroy( wp_renderer *renderer )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_destroy( renderer->software );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_destroy( renderer->dx11 );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
    {
        wp_renderer_dx12_destroy( renderer->dx12 );
    }

    free( renderer );
}

wp_renderer_type wp_renderer_get_type( const wp_renderer *renderer )
{
    if( !renderer )
    {
        return WORKPHONE_RENDERER_TYPE_SOFTWARE;
    }
    return renderer->type;
}

void wp_renderer_set_program_reset_func( wp_renderer *renderer,
                                         wp_renderer_program_reset_func reset_func, void *user_data )
{
    if( !renderer )
        return;
    renderer->program_reset_func = reset_func;
    renderer->program_reset_user_data = reset_func ? user_data : NULL;
}

void wp_renderer_mark_program_bound( wp_renderer *renderer )
{
    if( renderer )
        renderer->program_bound = 1;
}

wp_s32 wp_renderer_reset_program( wp_renderer *renderer )
{
    if( !renderer )
        return 0;
    if( !renderer->program_bound )
        return 1;
    if( !renderer->program_reset_func ||
        !renderer->program_reset_func( renderer, renderer->program_reset_user_data ) )
        return 0;
    renderer->program_bound = 0;
    return 1;
}

wp_s32 wp_renderer_has_program_bound( const wp_renderer *renderer )
{
    return renderer ? renderer->program_bound : 0;
}

wp_renderer_software *wp_renderer_get_software( const wp_renderer *renderer )
{
    if( !renderer || renderer->type != WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        return NULL;
    }
    return renderer->software;
}

wp_renderer_dx11 *wp_renderer_get_dx11( const wp_renderer *renderer )
{
    if( !renderer || renderer->type != WORKPHONE_RENDERER_TYPE_DX11 )
    {
        return NULL;
    }
    return renderer->dx11;
}

void *wp_renderer_get_dx11_device( const wp_renderer *renderer )
{
    if( !renderer || renderer->type != WORKPHONE_RENDERER_TYPE_DX11 )
    {
        return NULL;
    }

    return wp_renderer_dx11_get_device( renderer->dx11 );
}

wp_s32 wp_renderer_resize( wp_renderer *renderer, wp_s32 width, wp_s32 height )
{
    if( !renderer )
    {
        return 0;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        return wp_renderer_software_resize( renderer->software, width, height );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        return wp_renderer_dx11_resize( renderer->dx11, width, height );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
    {
        return wp_renderer_dx12_resize( renderer->dx12, width, height );
    }

    return 0;
}

/* =========================================================================
 * Frame lifecycle
 * ====================================================================== */

void wp_renderer_begin_frame( wp_renderer *renderer )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_begin_frame( renderer->software );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_begin_frame( renderer->dx11 );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
    {
        wp_renderer_dx12_begin_frame( renderer->dx12 );
    }
}

void wp_renderer_end_frame( wp_renderer *renderer )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_end_frame( renderer->software );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_end_frame( renderer->dx11 );
        wp_renderer_dx11_present( renderer->dx11, 1 );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
    {
        wp_renderer_dx12_end_frame( renderer->dx12 );
        wp_renderer_dx12_present( renderer->dx12, 1 );
    }
}

/* =========================================================================
 * Clear
 * ====================================================================== */

void wp_renderer_set_clear_color( wp_renderer *renderer, wp_f32 r, wp_f32 g, wp_f32 b, wp_f32 a )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_set_clear_color( renderer->software, r, g, b, a );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_set_clear_color( renderer->dx11, r, g, b, a );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
    {
        wp_renderer_dx12_set_clear_color( renderer->dx12, r, g, b, a );
    }
}

void wp_renderer_set_clear_depth( wp_renderer *renderer, wp_f32 depth )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_set_clear_depth( renderer->software, depth );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_set_clear_depth( renderer->dx11, depth );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
    {
        wp_renderer_dx12_set_clear_depth( renderer->dx12, depth );
    }
}

void wp_renderer_clear( wp_renderer *renderer, wp_u32 flags )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_clear( renderer->software, flags );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_clear( renderer->dx11, flags );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
    {
        wp_renderer_dx12_clear( renderer->dx12, flags );
    }
}

void wp_renderer_set_viewport( wp_renderer *renderer, wp_viewport_i viewport )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_set_viewport( renderer->software, viewport );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_set_viewport( renderer->dx11, viewport );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
    {
        wp_renderer_dx12_set_viewport( renderer->dx12, viewport );
    }
}

wp_viewport_i wp_renderer_get_viewport( const wp_renderer *renderer )
{
    wp_viewport_i zero;
    memset( &zero, 0, sizeof( zero ) );

    if( !renderer )
    {
        return zero;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        return wp_renderer_software_get_viewport( renderer->software );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        return wp_renderer_dx11_get_viewport( renderer->dx11 );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
    {
        return wp_renderer_dx12_get_viewport( renderer->dx12 );
    }

    return zero;
}

void wp_renderer_set_scissor_enabled( wp_renderer *renderer, wp_s32 enabled )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_set_scissor_enabled( renderer->software, enabled );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_set_scissor_enabled( renderer->dx11, enabled );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
    {
        wp_renderer_dx12_set_scissor_enabled( renderer->dx12, enabled );
    }
}

void wp_renderer_set_scissor_rect( wp_renderer *renderer, wp_viewport_i scissor )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_set_scissor_rect( renderer->software, scissor );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_set_scissor_rect( renderer->dx11, scissor );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
    {
        wp_renderer_dx12_set_scissor_rect( renderer->dx12, scissor );
    }
}

const void *wp_renderer_get_framebuffer( const wp_renderer *renderer )
{
    if( !renderer )
    {
        return NULL;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        return wp_renderer_software_get_framebuffer( renderer->software );
    }

    return NULL;
}

wp_s32 wp_renderer_get_width( const wp_renderer *renderer )
{
    if( !renderer )
    {
        return 0;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        return wp_renderer_software_get_width( renderer->software );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        return wp_renderer_dx11_get_width( renderer->dx11 );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
    {
        return wp_renderer_dx12_get_width( renderer->dx12 );
    }

    return 0;
}

wp_s32 wp_renderer_get_height( const wp_renderer *renderer )
{
    if( !renderer )
    {
        return 0;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        return wp_renderer_software_get_height( renderer->software );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        return wp_renderer_dx11_get_height( renderer->dx11 );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
    {
        return wp_renderer_dx12_get_height( renderer->dx12 );
    }

    return 0;
}

void wp_renderer_set_blend_mode( wp_renderer *renderer, wp_blend_mode mode )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_set_blend_mode( renderer->software, mode );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_set_blend_mode( renderer->dx11, mode );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
    {
        wp_renderer_dx12_set_blend_mode( renderer->dx12, mode );
    }
}

wp_blend_mode wp_renderer_get_blend_mode( const wp_renderer *renderer )
{
    if( !renderer || renderer->type != WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        if( renderer && renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
        {
            return wp_renderer_dx11_get_blend_mode( renderer->dx11 );
        }
        if( renderer && renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
        {
            return wp_renderer_dx12_get_blend_mode( renderer->dx12 );
        }
        return WORKPHONE_BLEND_MODE_NONE;
    }
    return wp_renderer_software_get_blend_mode( renderer->software );
}

void wp_renderer_set_fill_mode( wp_renderer *renderer, wp_fill_mode mode )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_set_fill_mode( renderer->software, mode );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_set_fill_mode( renderer->dx11, mode );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
    {
        wp_renderer_dx12_set_fill_mode( renderer->dx12, mode );
    }
}

wp_fill_mode wp_renderer_get_fill_mode( const wp_renderer *renderer )
{
    if( !renderer || renderer->type != WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        if( renderer && renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
        {
            return wp_renderer_dx11_get_fill_mode( renderer->dx11 );
        }
        if( renderer && renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
        {
            return wp_renderer_dx12_get_fill_mode( renderer->dx12 );
        }
        return WORKPHONE_FILL_MODE_SOLID;
    }
    return wp_renderer_software_get_fill_mode( renderer->software );
}

void wp_renderer_set_cull_mode( wp_renderer *renderer, wp_cull_mode mode )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_set_cull_mode( renderer->software, mode );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_set_cull_mode( renderer->dx11, mode );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
    {
        wp_renderer_dx12_set_cull_mode( renderer->dx12, mode );
    }
}

wp_cull_mode wp_renderer_get_cull_mode( const wp_renderer *renderer )
{
    if( !renderer || renderer->type != WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        if( renderer && renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
        {
            return wp_renderer_dx11_get_cull_mode( renderer->dx11 );
        }
        if( renderer && renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
        {
            return wp_renderer_dx12_get_cull_mode( renderer->dx12 );
        }
        return WORKPHONE_CULL_MODE_BACK;
    }

    return wp_renderer_software_get_cull_mode( renderer->software );
}

void wp_renderer_set_depth_test_enabled( wp_renderer *renderer, wp_s32 enabled )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_set_depth_test_enabled( renderer->software, enabled );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_set_depth_test_enabled( renderer->dx11, enabled );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX12 )
    {
        wp_renderer_dx12_set_depth_test_enabled( renderer->dx12, enabled );
    }
}

wp_s32 wp_renderer_get_depth_test_enabled( const wp_renderer *renderer )
{
    if( !renderer || renderer->type != WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        if( renderer && renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
        {
            return wp_renderer_dx11_get_depth_test_enabled( renderer->dx11 );
        }

        return 0;
    }
    return wp_renderer_software_get_depth_test_enabled( renderer->software );
}

void wp_renderer_set_depth_write_enabled( wp_renderer *renderer, wp_s32 enabled )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_set_depth_write_enabled( renderer->software, enabled );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_set_depth_write_enabled( renderer->dx11, enabled );
    }
}

wp_s32 wp_renderer_get_depth_write_enabled( const wp_renderer *renderer )
{
    if( !renderer || renderer->type != WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        if( renderer && renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
        {
            return wp_renderer_dx11_get_depth_write_enabled( renderer->dx11 );
        }

        return 0;
    }

    return wp_renderer_software_get_depth_write_enabled( renderer->software );
}

void wp_renderer_set_depth_func( wp_renderer *renderer, wp_depth_func func )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_set_depth_func( renderer->software, func );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_set_depth_func( renderer->dx11, func );
    }
}

wp_depth_func wp_renderer_get_depth_func( const wp_renderer *renderer )
{
    if( !renderer || renderer->type != WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        if( renderer && renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
        {
            return wp_renderer_dx11_get_depth_func( renderer->dx11 );
        }

        return WORKPHONE_DEPTH_FUNC_LESS;
    }

    return wp_renderer_software_get_depth_func( renderer->software );
}

void wp_renderer_set_world_matrix( wp_renderer *renderer, const wp_mat4f *mat )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_set_world_matrix( renderer->software, mat );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_set_world_matrix( renderer->dx11, mat );
    }
}

void wp_renderer_set_view_matrix( wp_renderer *renderer, const wp_mat4f *mat )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_set_view_matrix( renderer->software, mat );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_set_view_matrix( renderer->dx11, mat );
    }
}

void wp_renderer_set_projection_matrix( wp_renderer *renderer, const wp_mat4f *mat )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_set_projection_matrix( renderer->software, mat );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_set_projection_matrix( renderer->dx11, mat );
    }
}

void wp_renderer_set_texture( wp_renderer *renderer, const void *pixels, wp_s32 width, wp_s32 height,
                              wp_pixel_format format )
{
    if( !renderer )
        return;

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_set_texture( renderer->software, pixels, width, height, format );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_set_texture( renderer->dx11, pixels, width, height, format );
    }
}

void wp_renderer_set_texture_native( wp_renderer *renderer, void *texture_view )
{
    if( renderer && renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_set_texture_native( renderer->dx11, texture_view );
    }
}

void wp_renderer_set_render_target_native( wp_renderer *renderer, void *render_target_view )
{
    if( renderer && renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_set_render_target_native( renderer->dx11, render_target_view );
    }
}

void wp_renderer_draw_triangles_pc( wp_renderer *renderer, const wp_vertex_pc *vertices,
                                    wp_s32 vertex_count )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_draw_triangles_pc( renderer->software, vertices, vertex_count );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_draw_triangles_pc( renderer->dx11, vertices, vertex_count );
    }
}

void wp_renderer_draw_indexed_triangles_pc( wp_renderer *renderer, const wp_vertex_pc *vertices,
                                            wp_s32 vertex_count, const uint16_t *indices,
                                            wp_s32 index_count )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_draw_indexed_triangles_pc( renderer->software, vertices, vertex_count,
                                                        indices, index_count );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_draw_indexed_triangles_pc( renderer->dx11, vertices, vertex_count, indices,
                                                    index_count );
    }
}

void wp_renderer_draw_lines_pc( wp_renderer *renderer, const wp_vertex_pc *vertices,
                                wp_s32 vertex_count )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_draw_lines_pc( renderer->software, vertices, vertex_count );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_draw_lines_pc( renderer->dx11, vertices, vertex_count );
    }
}

void wp_renderer_draw_points_pc( wp_renderer *renderer, const wp_vertex_pc *vertices,
                                 wp_s32 vertex_count )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_draw_points_pc( renderer->software, vertices, vertex_count );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_draw_points_pc( renderer->dx11, vertices, vertex_count );
    }
}

void wp_renderer_draw_triangles_ptc( wp_renderer *renderer, const wp_vertex_ptc *vertices,
                                     wp_s32 vertex_count )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_draw_triangles_ptc( renderer->software, vertices, vertex_count );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_draw_triangles_ptc( renderer->dx11, vertices, vertex_count );
    }
}

void wp_renderer_draw_indexed_triangles_ptc( wp_renderer *renderer, const wp_vertex_ptc *vertices,
                                             wp_s32 vertex_count, const uint16_t *indices,
                                             wp_s32 index_count )
{
    if( !renderer )
    {
        return;
    }

    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_draw_indexed_triangles_ptc( renderer->software, vertices, vertex_count,
                                                         indices, index_count );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_draw_indexed_triangles_ptc( renderer->dx11, vertices, vertex_count, indices,
                                                     index_count );
    }
}

void wp_renderer_draw_indexed_triangles_ptc_u32( wp_renderer *renderer, const wp_vertex_ptc *vertices,
                                                 wp_s32 vertex_count, const wp_u32 *indices,
                                                 wp_s32 index_count )
{
    if( !renderer )
    {
        return;
    }

    wp_renderer_reset_program( renderer );
    if( renderer->type == WORKPHONE_RENDERER_TYPE_SOFTWARE )
    {
        wp_renderer_software_draw_indexed_triangles_ptc_u32( renderer->software, vertices, vertex_count,
                                                             indices, index_count );
    }
    else if( renderer->type == WORKPHONE_RENDERER_TYPE_DX11 )
    {
        wp_renderer_dx11_draw_indexed_triangles_ptc_u32( renderer->dx11, vertices, vertex_count, indices,
                                                         index_count );
    }
}

void wp_renderer_get_native( const wp_renderer *renderer, void **pp_object )
{
    if( !pp_object )
    {
        return;
    }

    if( !renderer )
    {
        *pp_object = NULL;
        return;
    }

    *pp_object = renderer->native;
}

void wp_renderer_set_native( wp_renderer *renderer, void *native )
{
    if( !renderer )
    {
        return;
    }

    renderer->native = native;
}
