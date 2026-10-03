/**
 * @file workphone_game_component_renderer.h
 * @brief C API for the renderer game component.
 *
 * A renderer component binds a mesh, up to WP_RENDERER_COMPONENT_MAX_MATERIALS
 * materials, and a graphics object to a game actor.  It caches the world
 * transform matrix and propagates visibility and shadow state to the
 * underlying graphics object.
 */

#ifndef WORKPHONE_GAME_COMPONENT_RENDERER_H
#define WORKPHONE_GAME_COMPONENT_RENDERER_H

#include <stdint.h>
#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_matrix.h"
#include "workphone_game_component.h"
#include "workphone_graphics_mesh.h"
#include "workphone_graphics_material.h"
#include "workphone_graphics_object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Limits
 * ---------------------------------------------------------------------- */

#ifndef WP_RENDERER_COMPONENT_MAX_MATERIALS
#    define WP_RENDERER_COMPONENT_MAX_MATERIALS 8
#endif

/* -------------------------------------------------------------------------
 * Renderer component
 *
 * The base wp_game_component is the first member so a
 * wp_renderer_component * can be safely cast to wp_game_component * and back.
 * ---------------------------------------------------------------------- */

typedef struct wp_renderer_component
{
    wp_game_component base;

    wp_graphics_mesh *mesh;
    wp_graphics_material *materials[WP_RENDERER_COMPONENT_MAX_MATERIALS];
    wp_u32 num_materials;

    wp_graphics_object *graphics_object;

    wp_mat4f world_matrix;

    wp_s32 cast_shadows;
    wp_s32 receive_shadows;
    wp_u32 visibility_flags;
    wp_u32 render_queue;
} wp_renderer_component;

/* ---- Lifecycle -------------------------------------------------------- */

void wp_renderer_component_init( wp_renderer_component *rc );
void wp_renderer_component_destroy( wp_renderer_component *rc );

/* ---- Update ----------------------------------------------------------- */

void wp_renderer_component_update( wp_renderer_component *rc, wp_f64 dt );

/* ---- Mesh ------------------------------------------------------------- */

void wp_renderer_component_set_mesh( wp_renderer_component *rc, wp_graphics_mesh *mesh );
wp_graphics_mesh *wp_renderer_component_get_mesh( const wp_renderer_component *rc );

/* ---- Materials -------------------------------------------------------- */

wp_s32 wp_renderer_component_set_material( wp_renderer_component *rc, wp_u32 slot,
                                           wp_graphics_material *mat );
wp_graphics_material *wp_renderer_component_get_material( const wp_renderer_component *rc, wp_u32 slot );
wp_u32 wp_renderer_component_get_num_materials( const wp_renderer_component *rc );
void wp_renderer_component_clear_materials( wp_renderer_component *rc );

/* ---- Graphics object -------------------------------------------------- */

void wp_renderer_component_set_graphics_object( wp_renderer_component *rc, wp_graphics_object *obj );
wp_graphics_object *wp_renderer_component_get_graphics_object( const wp_renderer_component *rc );

/* ---- Visibility ------------------------------------------------------- */

void wp_renderer_component_set_visible( wp_renderer_component *rc, wp_s32 visible );
wp_s32 wp_renderer_component_is_visible( const wp_renderer_component *rc );

void wp_renderer_component_set_visibility_flags( wp_renderer_component *rc, wp_u32 flags );
wp_u32 wp_renderer_component_get_visibility_flags( const wp_renderer_component *rc );

/* ---- Shadows ---------------------------------------------------------- */

void wp_renderer_component_set_cast_shadows( wp_renderer_component *rc, wp_s32 cast );
wp_s32 wp_renderer_component_get_cast_shadows( const wp_renderer_component *rc );

void wp_renderer_component_set_receive_shadows( wp_renderer_component *rc, wp_s32 receive );
wp_s32 wp_renderer_component_get_receive_shadows( const wp_renderer_component *rc );

/* ---- Render queue ----------------------------------------------------- */

void wp_renderer_component_set_render_queue( wp_renderer_component *rc, wp_u32 queue );
wp_u32 wp_renderer_component_get_render_queue( const wp_renderer_component *rc );

/* ---- World matrix ----------------------------------------------------- */

const wp_mat4f *wp_renderer_component_get_world_matrix( const wp_renderer_component *rc );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GAME_COMPONENT_RENDERER_H */
