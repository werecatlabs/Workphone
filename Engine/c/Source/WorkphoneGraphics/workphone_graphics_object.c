/**
 * @file wp_graphics_object.c
 * @brief Implementation of the C graphics object API.
 */

#include "workphone_graphics_object.h"
#include "workphone_graphics_material.h"
#include "workphone_graphics_mesh.h"
#include "workphone_graphics_renderer.h"
#include "workphone_graphics_scenenode.h"
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal structure
 * ====================================================================== */

typedef struct wp_graphics_object
{
    wp_u32 flags;
    wp_u32 visibility_flags;
    wp_u32 z_order;
    wp_u32 render_queue_group;
    wp_u32 render_technique;
    wp_aabb3f local_aabb;
    wp_scenenode *owner;
    wp_graphics_scene *creator;
    void *native;
    wp_s32 is_dirty;
    wp_graphics_object_render_func render_func;
    wp_graphics_mesh *mesh;
    wp_graphics_material *material;
} wp_graphics_object;

static void wp_graphics_object_render_mesh( wp_graphics_object *obj, wp_renderer *renderer )
{
    wp_mat4f world;

    if( !obj || !renderer || !obj->mesh )
    {
        return;
    }

    wp_scenenode_get_world_matrix( obj->owner, &world );
    wp_renderer_set_world_matrix( renderer, &world );
    wp_graphics_mesh_render( obj->mesh, renderer, obj->material );
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_graphics_object *wp_graphics_object_create( void )
{
    wp_graphics_object *obj = (wp_graphics_object *)malloc( sizeof( wp_graphics_object ) );
    if( !obj )
    {
        return NULL;
    }

    memset( obj, 0, sizeof( wp_graphics_object ) );

    obj->flags = WORKPHONE_GRAPHICS_OBJECT_FLAG_VISIBLE | WORKPHONE_GRAPHICS_OBJECT_FLAG_CAST_SHADOWS |
                 WORKPHONE_GRAPHICS_OBJECT_FLAG_RECV_SHADOWS | WORKPHONE_GRAPHICS_OBJECT_FLAG_SCENE;
    obj->visibility_flags = 0xFFFFFFFFu;
    obj->render_queue_group = WORKPHONE_RENDER_QUEUE_DEFAULT;

    return obj;
}

void wp_graphics_object_destroy( wp_graphics_object *obj )
{
    if( !obj )
    {
        return;
    }

    free( obj );
}

/* =========================================================================
 * Visibility
 * ====================================================================== */

void wp_graphics_object_set_visible( wp_graphics_object *obj, wp_s32 visible )
{
    if( !obj )
    {
        return;
    }

    if( visible )
    {
        obj->flags |= WORKPHONE_GRAPHICS_OBJECT_FLAG_VISIBLE;
    }
    else
    {
        obj->flags &= ~WORKPHONE_GRAPHICS_OBJECT_FLAG_VISIBLE;
    }
}

wp_s32 wp_graphics_object_is_visible( const wp_graphics_object *obj )
{
    if( !obj )
    {
        return 0;
    }

    return ( obj->flags & WORKPHONE_GRAPHICS_OBJECT_FLAG_VISIBLE ) != 0;
}

/* =========================================================================
 * Shadow casting and receiving
 * ====================================================================== */

void wp_graphics_object_set_cast_shadows( wp_graphics_object *obj, wp_s32 cast )
{
    if( !obj )
    {
        return;
    }

    if( cast )
    {
        obj->flags |= WORKPHONE_GRAPHICS_OBJECT_FLAG_CAST_SHADOWS;
    }
    else
    {
        obj->flags &= ~WORKPHONE_GRAPHICS_OBJECT_FLAG_CAST_SHADOWS;
    }
}

wp_s32 wp_graphics_object_get_cast_shadows( const wp_graphics_object *obj )
{
    if( !obj )
    {
        return 0;
    }

    return ( obj->flags & WORKPHONE_GRAPHICS_OBJECT_FLAG_CAST_SHADOWS ) != 0;
}

void wp_graphics_object_set_receive_shadows( wp_graphics_object *obj, wp_s32 receive )
{
    if( !obj )
    {
        return;
    }

    if( receive )
    {
        obj->flags |= WORKPHONE_GRAPHICS_OBJECT_FLAG_RECV_SHADOWS;
    }
    else
    {
        obj->flags &= ~WORKPHONE_GRAPHICS_OBJECT_FLAG_RECV_SHADOWS;
    }
}

wp_s32 wp_graphics_object_get_receive_shadows( const wp_graphics_object *obj )
{
    if( !obj )
    {
        return 0;
    }

    return ( obj->flags & WORKPHONE_GRAPHICS_OBJECT_FLAG_RECV_SHADOWS ) != 0;
}

/* =========================================================================
 * Visibility flags (bitmask)
 * ====================================================================== */

void wp_graphics_object_set_visibility_flags( wp_graphics_object *obj, wp_u32 flags )
{
    if( !obj )
    {
        return;
    }

    obj->visibility_flags = flags;
}

wp_u32 wp_graphics_object_get_visibility_flags( const wp_graphics_object *obj )
{
    if( !obj )
    {
        return 0;
    }

    return obj->visibility_flags;
}

/* =========================================================================
 * Z-order and render queue
 * ====================================================================== */

void wp_graphics_object_set_z_order( wp_graphics_object *obj, wp_u32 z_order )
{
    if( !obj )
    {
        return;
    }

    obj->z_order = z_order;
}

wp_u32 wp_graphics_object_get_z_order( const wp_graphics_object *obj )
{
    if( !obj )
    {
        return 0;
    }

    return obj->z_order;
}

void wp_graphics_object_set_render_queue_group( wp_graphics_object *obj, wp_u32 queue_id )
{
    if( !obj )
    {
        return;
    }

    obj->render_queue_group = queue_id;
}

wp_u32 wp_graphics_object_get_render_queue_group( const wp_graphics_object *obj )
{
    if( !obj )
    {
        return WORKPHONE_RENDER_QUEUE_DEFAULT;
    }

    return obj->render_queue_group;
}

/* =========================================================================
 * Render technique
 * ====================================================================== */

void wp_graphics_object_set_render_technique( wp_graphics_object *obj, wp_u32 technique )
{
    if( !obj )
    {
        return;
    }

    obj->render_technique = technique;
}

wp_u32 wp_graphics_object_get_render_technique( const wp_graphics_object *obj )
{
    if( !obj )
    {
        return 0;
    }

    return obj->render_technique;
}

/* =========================================================================
 * Object flags (bitmask)
 * ====================================================================== */

void wp_graphics_object_set_flag( wp_graphics_object *obj, wp_u32 flag, wp_s32 value )
{
    if( !obj )
    {
        return;
    }

    if( value )
    {
        obj->flags |= flag;
    }
    else
    {
        obj->flags &= ~flag;
    }
}

wp_s32 wp_graphics_object_get_flag( const wp_graphics_object *obj, wp_u32 flag )
{
    if( !obj )
    {
        return 0;
    }

    return ( obj->flags & flag ) != 0;
}

wp_u32 wp_graphics_object_get_flags( const wp_graphics_object *obj )
{
    if( !obj )
    {
        return 0;
    }

    return obj->flags;
}

void wp_graphics_object_set_flags( wp_graphics_object *obj, wp_u32 flags )
{
    if( !obj )
    {
        return;
    }

    obj->flags = flags;
}

/* =========================================================================
 * Local AABB
 * ====================================================================== */

wp_aabb3f wp_graphics_object_get_local_aabb( const wp_graphics_object *obj )
{
    wp_aabb3f empty;
    memset( &empty, 0, sizeof( wp_aabb3f ) );

    if( !obj )
    {
        return empty;
    }

    return obj->local_aabb;
}

void wp_graphics_object_set_local_aabb( wp_graphics_object *obj, wp_aabb3f aabb )
{
    if( !obj )
    {
        return;
    }

    obj->local_aabb = aabb;
}

/* =========================================================================
 * Scene node attachment
 * ====================================================================== */

void wp_graphics_object_attach_to_parent( wp_graphics_object *obj, wp_scenenode *parent )
{
    if( !obj )
    {
        return;
    }

    obj->owner = parent;
    obj->flags |= WORKPHONE_GRAPHICS_OBJECT_FLAG_ATTACHED;
}

void wp_graphics_object_detach_from_parent( wp_graphics_object *obj, wp_scenenode *parent )
{
    if( !obj )
    {
        return;
    }

    if( obj->owner != parent )
    {
        return;
    }

    obj->owner = NULL;
    obj->flags &= ~WORKPHONE_GRAPHICS_OBJECT_FLAG_ATTACHED;
}

wp_s32 wp_graphics_object_is_attached( const wp_graphics_object *obj )
{
    if( !obj )
    {
        return 0;
    }

    return ( obj->flags & WORKPHONE_GRAPHICS_OBJECT_FLAG_ATTACHED ) != 0;
}

void wp_graphics_object_set_attached( wp_graphics_object *obj, wp_s32 attached )
{
    if( !obj )
    {
        return;
    }

    if( attached )
    {
        obj->flags |= WORKPHONE_GRAPHICS_OBJECT_FLAG_ATTACHED;
    }
    else
    {
        obj->flags &= ~WORKPHONE_GRAPHICS_OBJECT_FLAG_ATTACHED;
    }
}

wp_scenenode *wp_graphics_object_get_owner( const wp_graphics_object *obj )
{
    if( !obj )
    {
        return NULL;
    }

    return obj->owner;
}

void wp_graphics_object_set_owner( wp_graphics_object *obj, wp_scenenode *node )
{
    if( !obj )
    {
        return;
    }

    obj->owner = node;
}

/* =========================================================================
 * Creator / graphics scene
 * ====================================================================== */

wp_graphics_scene *wp_graphics_object_get_creator( const wp_graphics_object *obj )
{
    if( !obj )
    {
        return NULL;
    }

    return obj->creator;
}

void wp_graphics_object_set_creator( wp_graphics_object *obj, wp_graphics_scene *scene )
{
    if( !obj )
    {
        return;
    }

    obj->creator = scene;
}

/* =========================================================================
 * State management
 * ====================================================================== */

void wp_graphics_object_make_dirty( wp_graphics_object *obj )
{
    if( !obj )
    {
        return;
    }

    obj->is_dirty = 1;
}

/* =========================================================================
 * Native object access
 * ====================================================================== */

void wp_graphics_object_get_native( const wp_graphics_object *obj, void **pp_object )
{
    if( !pp_object )
    {
        return;
    }

    *pp_object = obj ? obj->native : NULL;
}

/* =========================================================================
 * Render callback
 * ====================================================================== */

void wp_graphics_object_set_render_func( wp_graphics_object *obj, wp_graphics_object_render_func fn )
{
    if( !obj )
    {
        return;
    }

    obj->render_func = fn;
}

wp_graphics_object_render_func wp_graphics_object_get_render_func( const wp_graphics_object *obj )
{
    if( !obj )
    {
        return NULL;
    }

    return obj->render_func;
}

void wp_graphics_object_set_mesh( wp_graphics_object *obj, wp_graphics_mesh *mesh )
{
    if( !obj )
    {
        return;
    }

    obj->mesh = mesh;
    if( mesh )
    {
        obj->local_aabb = wp_graphics_mesh_get_local_aabb( mesh );
        obj->render_func = wp_graphics_object_render_mesh;
    }
    else if( obj->render_func == wp_graphics_object_render_mesh )
    {
        obj->render_func = NULL;
    }
    obj->is_dirty = 1;
}

wp_graphics_mesh *wp_graphics_object_get_mesh( const wp_graphics_object *obj )
{
    return obj ? obj->mesh : NULL;
}

void wp_graphics_object_set_material( wp_graphics_object *obj, wp_graphics_material *material )
{
    if( !obj )
    {
        return;
    }
    obj->material = material;
    obj->is_dirty = 1;
}

wp_graphics_material *wp_graphics_object_get_material( const wp_graphics_object *obj )
{
    return obj ? obj->material : NULL;
}
