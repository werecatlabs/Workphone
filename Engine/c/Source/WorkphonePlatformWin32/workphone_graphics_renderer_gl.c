/**
 * @file workphone_graphics_renderer_gl.c
 * @brief OpenGL renderer implementation.
 *
 * Mirrors the software and DX11/DX12 renderer interface but drives a hardware OpenGL
 * pipeline. Uses the C API so that the file compiles as plain C.
 */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#include <stdlib.h>
#include <string.h>
#include "workphone_graphics_renderer_gl.h"

typedef struct wp_renderer_gl {
    // OpenGL core objects (stubs)
    void *context;
    void *window;
    wp_s32 width;
    wp_s32 height;
    wp_f32 clear_r, clear_g, clear_b, clear_a, clear_depth;
    wp_viewport_i viewport;
    wp_s32 scissor_enabled;
    wp_viewport_i scissor;
    wp_blend_mode blend_mode;
    wp_fill_mode fill_mode;
    wp_cull_mode cull_mode;
    wp_s32 depth_test_enabled;
    wp_s32 depth_write_enabled;
    wp_depth_func depth_func;
    wp_mat4f world_matrix, view_matrix, proj_matrix, mvp_matrix;
    wp_s32 mvp_dirty;
    void *native;
} wp_renderer_gl;

wp_renderer_gl *wp_renderer_gl_create(void *window, wp_s32 width, wp_s32 height) {
    if (!window || width <= 0 || height <= 0) return NULL;
    wp_renderer_gl *r = (wp_renderer_gl *)malloc(sizeof(wp_renderer_gl));
    if (!r) return NULL;
    memset(r, 0, sizeof(wp_renderer_gl));
    r->window = window;
    r->width = width;
    r->height = height;
    r->clear_a = 1.0f;
    r->clear_depth = 1.0f;
    r->viewport.x = 0;
    r->viewport.y = 0;
    r->viewport.width = width;
    r->viewport.height = height;
    r->blend_mode = WORKPHONE_BLEND_MODE_NONE;
    r->fill_mode = WORKPHONE_FILL_MODE_SOLID;
    r->cull_mode = WORKPHONE_CULL_MODE_BACK;
    r->depth_test_enabled = 1;
    r->depth_write_enabled = 1;
    r->depth_func = WORKPHONE_DEPTH_FUNC_LESS;
    wp_mat4f_identity(&r->world_matrix);
    wp_mat4f_identity(&r->view_matrix);
    wp_mat4f_identity(&r->proj_matrix);
    wp_mat4f_identity(&r->mvp_matrix);
    r->mvp_dirty = 1;
    return r;
}

void wp_renderer_gl_destroy(wp_renderer_gl *r) {
    if (!r) return;
    free(r);
}

wp_s32 wp_renderer_gl_resize(wp_renderer_gl *r, wp_s32 width, wp_s32 height) {
    if (!r || width <= 0 || height <= 0) return 0;
    r->width = width;
    r->height = height;
    r->viewport.width = width;
    r->viewport.height = height;
    return 1;
}

void wp_renderer_gl_begin_frame(wp_renderer_gl *r) {
    if (!r) return;
    // TODO: Begin OpenGL frame
}

void wp_renderer_gl_end_frame(wp_renderer_gl *r) {
    if (!r) return;
    // TODO: End OpenGL frame and present
}

void wp_renderer_gl_present(wp_renderer_gl *r, wp_s32 vsync) {
    (void)r; (void)vsync;
    // TODO: Present OpenGL buffer
}

void wp_renderer_gl_set_clear_color(wp_renderer_gl *r, wp_f32 cr, wp_f32 cg, wp_f32 cb, wp_f32 ca) {
    if (!r) return;
    r->clear_r = cr;
    r->clear_g = cg;
    r->clear_b = cb;
    r->clear_a = ca;
}

void wp_renderer_gl_set_clear_depth(wp_renderer_gl *r, wp_f32 depth) {
    if (!r) return;
    r->clear_depth = depth;
}

void wp_renderer_gl_clear(wp_renderer_gl *r, wp_u32 flags) {
    if (!r) return;
    // TODO: Clear OpenGL buffers
}

void wp_renderer_gl_set_viewport(wp_renderer_gl *r, wp_viewport_i viewport) {
    if (!r) return;
    r->viewport = viewport;
    // TODO: Set OpenGL viewport
}

wp_viewport_i wp_renderer_gl_get_viewport(const wp_renderer_gl *r) {
    wp_viewport_i zero; memset(&zero, 0, sizeof(zero));
    if (r) return r->viewport;
    return zero;
}

void wp_renderer_gl_set_scissor_enabled(wp_renderer_gl *r, wp_s32 enabled) {
    if (!r) return;
    r->scissor_enabled = enabled;
}

void wp_renderer_gl_set_scissor_rect(wp_renderer_gl *r, wp_viewport_i scissor) {
    if (!r) return;
    r->scissor = scissor;
    // TODO: Set OpenGL scissor rect
}

void wp_renderer_gl_set_blend_mode(wp_renderer_gl *r, wp_blend_mode mode) {
    if (!r) return;
    r->blend_mode = mode;
}

wp_blend_mode wp_renderer_gl_get_blend_mode(const wp_renderer_gl *r) {
    return r ? r->blend_mode : WORKPHONE_BLEND_MODE_NONE;
}

void wp_renderer_gl_set_fill_mode(wp_renderer_gl *r, wp_fill_mode mode) {
    if (!r) return;
    r->fill_mode = mode;
}

wp_fill_mode wp_renderer_gl_get_fill_mode(const wp_renderer_gl *r) {
    return r ? r->fill_mode : WORKPHONE_FILL_MODE_SOLID;
}

void wp_renderer_gl_set_cull_mode(wp_renderer_gl *r, wp_cull_mode mode) {
    if (!r) return;
    r->cull_mode = mode;
}

wp_cull_mode wp_renderer_gl_get_cull_mode(const wp_renderer_gl *r) {
    return r ? r->cull_mode : WORKPHONE_CULL_MODE_BACK;
}

void wp_renderer_gl_set_depth_test_enabled(wp_renderer_gl *r, wp_s32 enabled) {
    if (!r) return;
    r->depth_test_enabled = enabled;
}

wp_s32 wp_renderer_gl_get_depth_test_enabled(const wp_renderer_gl *r) {
    return r ? r->depth_test_enabled : 0;
}

void wp_renderer_gl_set_depth_write_enabled(wp_renderer_gl *r, wp_s32 enabled) {
    if (!r) return;
    r->depth_write_enabled = enabled;
}

wp_s32 wp_renderer_gl_get_depth_write_enabled(const wp_renderer_gl *r) {
    return r ? r->depth_write_enabled : 0;
}

void wp_renderer_gl_set_depth_func(wp_renderer_gl *r, wp_depth_func func) {
    if (!r) return;
    r->depth_func = func;
}

wp_depth_func wp_renderer_gl_get_depth_func(const wp_renderer_gl *r) {
    return r ? r->depth_func : WORKPHONE_DEPTH_FUNC_LESS;
}

void wp_renderer_gl_set_world_matrix(wp_renderer_gl *r, const wp_mat4f *mat) {
    if (!r || !mat) return;
    r->world_matrix = *mat;
    r->mvp_dirty = 1;
}

void wp_renderer_gl_set_view_matrix(wp_renderer_gl *r, const wp_mat4f *mat) {
    if (!r || !mat) return;
    r->view_matrix = *mat;
    r->mvp_dirty = 1;
}

void wp_renderer_gl_set_projection_matrix(wp_renderer_gl *r, const wp_mat4f *mat) {
    if (!r || !mat) return;
    r->proj_matrix = *mat;
    r->mvp_dirty = 1;
}

void wp_renderer_gl_draw_triangles_pc(wp_renderer_gl *r, const wp_vertex_pc *vertices, wp_s32 vertex_count) {
    (void)r; (void)vertices; (void)vertex_count;
    // TODO: Implement OpenGL draw
}

void wp_renderer_gl_draw_indexed_triangles_pc(wp_renderer_gl *r, const wp_vertex_pc *vertices, wp_s32 vertex_count, const uint16_t *indices, wp_s32 index_count) {
    (void)r; (void)vertices; (void)vertex_count; (void)indices; (void)index_count;
    // TODO: Implement OpenGL indexed draw
}

void wp_renderer_gl_draw_lines_pc(wp_renderer_gl *r, const wp_vertex_pc *vertices, wp_s32 vertex_count) {
    (void)r; (void)vertices; (void)vertex_count;
    // TODO: Implement OpenGL line draw
}

void wp_renderer_gl_draw_points_pc(wp_renderer_gl *r, const wp_vertex_pc *vertices, wp_s32 vertex_count) {
    (void)r; (void)vertices; (void)vertex_count;
    // TODO: Implement OpenGL powp_s32 draw
}

void wp_renderer_gl_draw_triangles_ptc(wp_renderer_gl *r, const wp_vertex_ptc *vertices, wp_s32 vertex_count) {
    (void)r; (void)vertices; (void)vertex_count;
    // TODO: Implement OpenGL draw
}

void wp_renderer_gl_draw_indexed_triangles_ptc(wp_renderer_gl *r, const wp_vertex_ptc *vertices, wp_s32 vertex_count, const uint16_t *indices, wp_s32 index_count) {
    (void)r; (void)vertices; (void)vertex_count; (void)indices; (void)index_count;
    // TODO: Implement OpenGL indexed draw
}

wp_s32 wp_renderer_gl_get_width(const wp_renderer_gl *r) {
    return r ? r->width : 0;
}

wp_s32 wp_renderer_gl_get_height(const wp_renderer_gl *r) {
    return r ? r->height : 0;
}

void wp_renderer_gl_get_native(const wp_renderer_gl *r, void **pp) {
    if (!pp) return;
    *pp = r ? r->native : NULL;
}

void wp_renderer_gl_set_native(wp_renderer_gl *r, void *native) {
    if (r) r->native = native;
}

void *wp_renderer_gl_get_context(const wp_renderer_gl *r) {
    return r ? r->context : NULL;
}

void *wp_renderer_gl_get_window(const wp_renderer_gl *r) {
    return r ? r->window : NULL;
}
