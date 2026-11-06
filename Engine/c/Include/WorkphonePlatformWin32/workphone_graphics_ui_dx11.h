#ifndef WORKPHONE_GRAPHICS_UI_DX11_H
#define WORKPHONE_GRAPHICS_UI_DX11_H

#include <stddef.h>
#include "workphone_graphics_renderer_dx11.h"

#ifdef _WIN32
#ifdef __cplusplus
extern "C" {
#endif

/* Borrowed, immutable PTC and index spans for one ordered UI draw list.
 * Initialise upload_state to zero. Reset it after any callback or draw that
 * can overwrite the renderer's shared transient buffers or their bindings. */
typedef struct wp_ui_draw_list_dx11
{
    const wp_vertex_ptc *vertices;
    const void *indices;
    size_t vertex_count;
    size_t index_count;
    size_t index_stride;
    wp_s32 upload_state;
} wp_ui_draw_list_dx11;

/* Validates a range, binds its texture and scissor, and uploads the list once
 * for subsequent ranges. Index elements are relative to base_vertex. Supports
 * uint16_t and uint32_t indices. Each span must fit within INT_MAX / 2 bytes.
 * The caller owns render state outside the UI pass, as well as the list and
 * texture lifetimes. Returns zero for invalid input and one when the range was
 * passed to the backend. */
wp_s32 wp_renderer_dx11_draw_ui_range( wp_renderer_dx11 *renderer, wp_ui_draw_list_dx11 *list,
                                      size_t first_index, size_t index_count, size_t base_vertex,
                                      wp_viewport_i scissor, void *texture_view );

#ifdef __cplusplus
}
#endif
#endif /* _WIN32 */

#endif
