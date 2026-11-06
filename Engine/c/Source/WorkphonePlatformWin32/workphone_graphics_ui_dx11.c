#include "workphone_graphics_ui_dx11.h"
#include <limits.h>

/* The transient buffers grow by doubling their signed byte capacities. */
#define WP_UI_MAX_BUFFER_BYTES ( INT_MAX / 2 )

enum { WP_UI_UPLOAD_PENDING, WP_UI_UPLOAD_READY, WP_UI_UPLOAD_FALLBACK };

wp_s32 wp_renderer_dx11_draw_ui_range( wp_renderer_dx11 *renderer, wp_ui_draw_list_dx11 *list,
                                      size_t first_index, size_t index_count, size_t base_vertex,
                                      wp_viewport_i scissor, void *texture_view )
{
    size_t available_vertices, i;
    const uint16_t *indices16 = NULL;
    const uint32_t *indices32 = NULL;
    wp_s32 uploaded;

    if( !renderer || !list || !list->vertices || !list->indices || !index_count ||
        scissor.width <= 0 || scissor.height <= 0 ||
        scissor.x > INT_MAX - scissor.width || scissor.y > INT_MAX - scissor.height ||
        ( list->index_stride != sizeof( uint16_t ) && list->index_stride != sizeof( uint32_t ) ) ||
        list->vertex_count > WP_UI_MAX_BUFFER_BYTES / sizeof( wp_vertex_ptc ) ||
        list->index_count > WP_UI_MAX_BUFFER_BYTES / list->index_stride ||
        base_vertex >= list->vertex_count ||
        first_index >= list->index_count || index_count > list->index_count - first_index )
    {
        return 0;
    }

    available_vertices = list->vertex_count - base_vertex;
    if( list->index_stride == sizeof( uint16_t ) )
    {
        indices16 = (const uint16_t *)list->indices + first_index;
        for( i = 0; i < index_count; ++i )
            if( indices16[i] >= available_vertices )
                return 0;
    }
    else
    {
        indices32 = (const uint32_t *)list->indices + first_index;
        for( i = 0; i < index_count; ++i )
            if( indices32[i] >= available_vertices )
                return 0;
    }

    wp_renderer_dx11_set_texture_native( renderer, texture_view );
    wp_renderer_dx11_set_scissor_enabled( renderer, 1 );
    wp_renderer_dx11_set_scissor_rect( renderer, scissor );

    if( list->upload_state == WP_UI_UPLOAD_PENDING )
    {
        if( indices16 )
            uploaded = wp_renderer_dx11_prepare_indexed_triangles_ptc(
                renderer, list->vertices, (wp_s32)list->vertex_count, (const uint16_t *)list->indices,
                (wp_s32)list->index_count );
        else
            uploaded = wp_renderer_dx11_prepare_indexed_triangles_ptc_u32(
                renderer, list->vertices, (wp_s32)list->vertex_count, (const uint32_t *)list->indices,
                (wp_s32)list->index_count );
        list->upload_state = uploaded ? WP_UI_UPLOAD_READY : WP_UI_UPLOAD_FALLBACK;
    }

    if( list->upload_state == WP_UI_UPLOAD_READY )
        wp_renderer_dx11_draw_prepared_indexed_triangles_ptc(
            renderer, (wp_s32)first_index, (wp_s32)index_count, (wp_s32)base_vertex );
    else if( indices16 )
        wp_renderer_dx11_draw_indexed_triangles_ptc(
            renderer, list->vertices + base_vertex, (wp_s32)available_vertices, indices16,
            (wp_s32)index_count );
    else
        wp_renderer_dx11_draw_indexed_triangles_ptc_u32(
            renderer, list->vertices + base_vertex, (wp_s32)available_vertices, indices32,
            (wp_s32)index_count );
    return 1;
}
