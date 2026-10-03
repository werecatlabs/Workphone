/**
 * @file workphone_game_component_renderer.c
 * @brief Implementation of the renderer game component.
 */

#include "workphone_game_component_renderer.h"
#include "workphone_game_component.h"
#include "workphone_game_actor.h"
#include "workphone_matrix.h"

#include <string.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

/**
 * @brief Builds a column-vector TRS world matrix from a wp_transform3f.
 *
 * Layout (row-major storage, column-vector convention):
 *   m[row][col]
 *   col 0-2 = (rotation * scale) basis vectors
 *   col 3   = translation
 */
static void s_transform_to_mat4( const wp_transform3f *t, wp_mat4f *out )
{
    wp_f32 qw = t->orientation.w;
    wp_f32 qx = t->orientation.x;
    wp_f32 qy = t->orientation.y;
    wp_f32 qz = t->orientation.z;
    wp_f32 sx = t->scale.x;
    wp_f32 sy = t->scale.y;
    wp_f32 sz = t->scale.z;

    wp_f32 x2 = qx + qx;
    wp_f32 y2 = qy + qy;
    wp_f32 z2 = qz + qz;

    wp_f32 xx = qx * x2;
    wp_f32 xy = qx * y2;
    wp_f32 xz = qx * z2;
    wp_f32 yy = qy * y2;
    wp_f32 yz = qy * z2;
    wp_f32 zz = qz * z2;
    wp_f32 wx = qw * x2;
    wp_f32 wy = qw * y2;
    wp_f32 wz = qw * z2;

    out->m[0][0] = ( 1.0f - ( yy + zz ) ) * sx;
    out->m[1][0] = ( xy + wz ) * sx;
    out->m[2][0] = ( xz - wy ) * sx;
    out->m[3][0] = 0.0f;

    out->m[0][1] = ( xy - wz ) * sy;
    out->m[1][1] = ( 1.0f - ( xx + zz ) ) * sy;
    out->m[2][1] = ( yz + wx ) * sy;
    out->m[3][1] = 0.0f;

    out->m[0][2] = ( xz + wy ) * sz;
    out->m[1][2] = ( yz - wx ) * sz;
    out->m[2][2] = ( 1.0f - ( xx + yy ) ) * sz;
    out->m[3][2] = 0.0f;

    out->m[0][3] = t->position.x;
    out->m[1][3] = t->position.y;
    out->m[2][3] = t->position.z;
    out->m[3][3] = 1.0f;
}

static void s_sync_from_actor( wp_renderer_component *rc )
{
    if( rc->base.actor )
        s_transform_to_mat4( &rc->base.actor->world_transform, &rc->world_matrix );
}

static void s_apply_to_graphics_object( wp_renderer_component *rc )
{
    if( !rc->graphics_object )
        return;

    wp_graphics_object_set_visible( rc->graphics_object, wp_game_component_is_enabled( &rc->base ) );
    wp_graphics_object_set_cast_shadows( rc->graphics_object, rc->cast_shadows );
    wp_graphics_object_set_receive_shadows( rc->graphics_object, rc->receive_shadows );
    wp_graphics_object_set_visibility_flags( rc->graphics_object, rc->visibility_flags );
    wp_graphics_object_set_render_queue_group( rc->graphics_object, rc->render_queue );
}

/* =========================================================================
 * Component callbacks
 * ====================================================================== */

static void s_on_create( wp_game_component *comp )
{
    wp_renderer_component *rc = (wp_renderer_component *)comp;

    if( !rc->graphics_object )
        rc->graphics_object = wp_graphics_object_create();

    s_sync_from_actor( rc );
    s_apply_to_graphics_object( rc );
}

static void s_on_destroy( wp_game_component *comp )
{
    wp_renderer_component *rc = (wp_renderer_component *)comp;

    if( rc->graphics_object )
    {
        wp_graphics_object_destroy( rc->graphics_object );
        rc->graphics_object = NULL;
    }
}

static void s_on_update( wp_game_component *comp, wp_f64 dt )
{
    wp_renderer_component *rc = (wp_renderer_component *)comp;
    (void)dt;

    if( wp_game_component_is_dirty( comp ) )
    {
        s_sync_from_actor( rc );
        wp_game_component_set_dirty( comp, 0 );
    }
}

static void s_on_enable( wp_game_component *comp )
{
    wp_renderer_component *rc = (wp_renderer_component *)comp;

    if( rc->graphics_object )
        wp_graphics_object_set_visible( rc->graphics_object, 1 );
}

static void s_on_disable( wp_game_component *comp )
{
    wp_renderer_component *rc = (wp_renderer_component *)comp;

    if( rc->graphics_object )
        wp_graphics_object_set_visible( rc->graphics_object, 0 );
}

static void s_on_transform_updated( wp_game_component *comp )
{
    wp_renderer_component *rc = (wp_renderer_component *)comp;
    s_sync_from_actor( rc );
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

void wp_renderer_component_init( wp_renderer_component *rc )
{
    wp_component_callbacks cb;

    if( !rc )
        return;

    memset( rc, 0, sizeof( *rc ) );

    wp_game_component_init_with_type( &rc->base, WP_COMPONENT_TYPE_RENDERER );
    wp_game_component_set_name( &rc->base, "Renderer" );

    rc->mesh = NULL;
    rc->num_materials = 0;
    rc->graphics_object = NULL;
    rc->cast_shadows = 1;
    rc->receive_shadows = 1;
    rc->visibility_flags = 0xFFFFFFFFu;
    rc->render_queue = WORKPHONE_RENDER_QUEUE_DEFAULT;

    wp_mat4f_identity( &rc->world_matrix );

    memset( &cb, 0, sizeof( cb ) );
    cb.on_create = s_on_create;
    cb.on_destroy = s_on_destroy;
    cb.on_update = s_on_update;
    cb.on_enable = s_on_enable;
    cb.on_disable = s_on_disable;
    cb.on_transform_updated = s_on_transform_updated;
    wp_game_component_set_callbacks( &rc->base, cb );
}

void wp_renderer_component_destroy( wp_renderer_component *rc )
{
    if( !rc )
        return;

    wp_game_component_destroy( &rc->base );
}

/* =========================================================================
 * Update
 * ====================================================================== */

void wp_renderer_component_update( wp_renderer_component *rc, wp_f64 dt )
{
    if( !rc )
        return;

    wp_game_component_update( &rc->base, dt );
}

/* =========================================================================
 * Mesh
 * ====================================================================== */

void wp_renderer_component_set_mesh( wp_renderer_component *rc, wp_graphics_mesh *mesh )
{
    if( !rc )
        return;

    rc->mesh = mesh;
    wp_game_component_set_dirty( &rc->base, 1 );
}

wp_graphics_mesh *wp_renderer_component_get_mesh( const wp_renderer_component *rc )
{
    return rc ? rc->mesh : NULL;
}

/* =========================================================================
 * Materials
 * ====================================================================== */

wp_s32 wp_renderer_component_set_material( wp_renderer_component *rc, wp_u32 slot,
                                           wp_graphics_material *mat )
{
    if( !rc || slot >= WP_RENDERER_COMPONENT_MAX_MATERIALS )
        return 0;

    rc->materials[slot] = mat;

    if( mat && slot >= rc->num_materials )
        rc->num_materials = slot + 1;

    wp_game_component_set_dirty( &rc->base, 1 );
    return 1;
}

wp_graphics_material *wp_renderer_component_get_material( const wp_renderer_component *rc, wp_u32 slot )
{
    if( !rc || slot >= WP_RENDERER_COMPONENT_MAX_MATERIALS )
        return NULL;

    return rc->materials[slot];
}

wp_u32 wp_renderer_component_get_num_materials( const wp_renderer_component *rc )
{
    return rc ? rc->num_materials : 0;
}

void wp_renderer_component_clear_materials( wp_renderer_component *rc )
{
    if( !rc )
        return;

    memset( rc->materials, 0, sizeof( rc->materials ) );
    rc->num_materials = 0;
    wp_game_component_set_dirty( &rc->base, 1 );
}

/* =========================================================================
 * Graphics object
 * ====================================================================== */

void wp_renderer_component_set_graphics_object( wp_renderer_component *rc, wp_graphics_object *obj )
{
    if( !rc )
        return;

    if( rc->graphics_object && rc->graphics_object != obj )
        wp_graphics_object_destroy( rc->graphics_object );

    rc->graphics_object = obj;
    s_apply_to_graphics_object( rc );
}

wp_graphics_object *wp_renderer_component_get_graphics_object( const wp_renderer_component *rc )
{
    return rc ? rc->graphics_object : NULL;
}

/* =========================================================================
 * Visibility
 * ====================================================================== */

void wp_renderer_component_set_visible( wp_renderer_component *rc, wp_s32 visible )
{
    if( !rc )
        return;

    wp_game_component_set_visible( &rc->base, visible );

    if( rc->graphics_object )
        wp_graphics_object_set_visible( rc->graphics_object, visible );
}

wp_s32 wp_renderer_component_is_visible( const wp_renderer_component *rc )
{
    return rc ? wp_game_component_is_visible( &rc->base ) : 0;
}

void wp_renderer_component_set_visibility_flags( wp_renderer_component *rc, wp_u32 flags )
{
    if( !rc )
        return;

    rc->visibility_flags = flags;

    if( rc->graphics_object )
        wp_graphics_object_set_visibility_flags( rc->graphics_object, flags );
}

wp_u32 wp_renderer_component_get_visibility_flags( const wp_renderer_component *rc )
{
    return rc ? rc->visibility_flags : 0u;
}

/* =========================================================================
 * Shadows
 * ====================================================================== */

void wp_renderer_component_set_cast_shadows( wp_renderer_component *rc, wp_s32 cast )
{
    if( !rc )
        return;

    rc->cast_shadows = cast;

    if( rc->graphics_object )
        wp_graphics_object_set_cast_shadows( rc->graphics_object, cast );
}

wp_s32 wp_renderer_component_get_cast_shadows( const wp_renderer_component *rc )
{
    return rc ? rc->cast_shadows : 0;
}

void wp_renderer_component_set_receive_shadows( wp_renderer_component *rc, wp_s32 receive )
{
    if( !rc )
        return;

    rc->receive_shadows = receive;

    if( rc->graphics_object )
        wp_graphics_object_set_receive_shadows( rc->graphics_object, receive );
}

wp_s32 wp_renderer_component_get_receive_shadows( const wp_renderer_component *rc )
{
    return rc ? rc->receive_shadows : 0;
}

/* =========================================================================
 * Render queue
 * ====================================================================== */

void wp_renderer_component_set_render_queue( wp_renderer_component *rc, wp_u32 queue )
{
    if( !rc )
        return;

    rc->render_queue = queue;

    if( rc->graphics_object )
        wp_graphics_object_set_render_queue_group( rc->graphics_object, queue );
}

wp_u32 wp_renderer_component_get_render_queue( const wp_renderer_component *rc )
{
    return rc ? rc->render_queue : (wp_u32)WORKPHONE_RENDER_QUEUE_DEFAULT;
}

/* =========================================================================
 * World matrix
 * ====================================================================== */

const wp_mat4f *wp_renderer_component_get_world_matrix( const wp_renderer_component *rc )
{
    return rc ? &rc->world_matrix : NULL;
}
