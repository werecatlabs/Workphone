/**
 * @file workphone_graphics_scene.c
 * @brief Implementation of the C graphics scene API.
 */

#include "workphone_graphics_scene.h"
#include "workphone_graphics_camera.h"
#include "workphone_graphics_scenenode.h"
#include "workphone_graphics_object.h"
#include "workphone_graphics_mesh.h"
#include "workphone_graphics_renderer.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static void scene_remove_node( wp_graphics_scene *scene, wp_scenenode *node )
{
    wp_s32 i;

    for( i = 0; i < scene->node_count; ++i )
    {
        if( scene->nodes[i] == node )
        {
            scene->nodes[i] = scene->nodes[--scene->node_count];
            return;
        }
    }
}

static void scene_remove_object( wp_graphics_scene *scene, wp_graphics_object *obj )
{
    wp_s32 i;
    wp_s32 j;

    for( i = 0; i < scene->object_count; ++i )
    {
        if( scene->objects[i] == obj )
        {
            for( j = i + 1; j < scene->object_count; ++j )
            {
                scene->objects[j - 1] = scene->objects[j];
            }
            --scene->object_count;
            return;
        }
    }
}

static void scene_detach_all_objects( wp_graphics_scene *scene )
{
    wp_s32 i;
    wp_scenenode *node;
    wp_graphics_object *obj;

    for( i = 0; i < scene->node_count; ++i )
    {
        node = scene->nodes[i];
        while( wp_scenenode_get_object_count( node ) > 0 )
        {
            obj = wp_scenenode_get_object( node, wp_scenenode_get_object_count( node ) - 1 );
            wp_scenenode_detach_object( node, obj );
        }
    }
}

static wp_s32 scene_render_object_before( const wp_graphics_object *left,
                                          const wp_graphics_object *right )
{
    wp_u32 left_queue;
    wp_u32 right_queue;
    wp_u32 left_z;
    wp_u32 right_z;

    left_queue = wp_graphics_object_get_render_queue_group( left );
    right_queue = wp_graphics_object_get_render_queue_group( right );
    if( left_queue != right_queue )
    {
        return left_queue < right_queue;
    }

    left_z = wp_graphics_object_get_z_order( left );
    right_z = wp_graphics_object_get_z_order( right );
    return left_z < right_z;
}

static wp_s32 scene_mesh_outside_frustum( const wp_graphics_object *object,
                                          const wp_mat4f *view_projection )
{
    wp_graphics_mesh *mesh;
    wp_aabb3f bounds;
    wp_mat4f world;
    wp_mat4f clip;
    wp_s32 axis;
    wp_s32 sign;
    wp_f32 a, b, c, d;
    wp_f32 x, y, z;
    wp_f32 distance;
    wp_f32 tolerance;

    mesh = wp_graphics_object_get_mesh( object );
    if( !mesh )
        return 0;
    /* Read current mesh bounds: the object's copy can predate a mesh reload. */
    bounds = wp_graphics_mesh_get_local_aabb( mesh );
    if( !( bounds.min.x <= bounds.max.x && bounds.min.y <= bounds.max.y &&
           bounds.min.z <= bounds.max.z ) ||
        ( bounds.min.x == bounds.max.x && bounds.min.y == bounds.max.y &&
          bounds.min.z == bounds.max.z ) )
        return 0;

    wp_scenenode_get_world_matrix( wp_graphics_object_get_owner( object ), &world );
    wp_mat4f_mul( &clip, view_projection, &world );
    /* Camera matrices use -w <= x,y,z <= w (DX11 remaps z in its vertex shader).
     * Extract the planes in local space and test each box's furthest vertex.
     * This also handles parent transforms, rotations and negative scales. */
    for( axis = 0; axis < 3; ++axis )
    {
        for( sign = -1; sign <= 1; sign += 2 )
        {
            a = clip.m[3][0] + sign * clip.m[axis][0];
            b = clip.m[3][1] + sign * clip.m[axis][1];
            c = clip.m[3][2] + sign * clip.m[axis][2];
            d = clip.m[3][3] + sign * clip.m[axis][3];
            x = a >= 0 ? bounds.max.x : bounds.min.x;
            y = b >= 0 ? bounds.max.y : bounds.min.y;
            z = c >= 0 ? bounds.max.z : bounds.min.z;
            distance = a * x + b * y + c * z + d;
            tolerance = (wp_f32)( 1.0e-5 * ( fabs( a * x ) + fabs( b * y ) +
                                            fabs( c * z ) + fabs( d ) + 1.0 ) );
            if( distance < -tolerance )
                return 1;
        }
    }
    return 0;
}

static wp_s32 scene_ensure_render_capacity( wp_graphics_scene *scene, wp_s32 required )
{
    wp_graphics_object **new_list;
    wp_graphics_object **new_scratch;
    wp_s32 new_capacity;
    size_t allocation_size;

    if( required <= scene->render_capacity )
    {
        return 1;
    }

    new_capacity = scene->render_capacity > 0 ? scene->render_capacity : 16;
    while( new_capacity < required )
    {
        if( new_capacity > 0x3FFFFFFF )
        {
            new_capacity = required;
            break;
        }
        new_capacity *= 2;
    }

    if( (size_t)new_capacity > (size_t)-1 / sizeof( wp_graphics_object * ) )
    {
        return 0;
    }

    allocation_size = (size_t)new_capacity * sizeof( wp_graphics_object * );
    new_list = (wp_graphics_object **)malloc( allocation_size );
    new_scratch = (wp_graphics_object **)malloc( allocation_size );
    if( !new_list || !new_scratch )
    {
        free( new_list );
        free( new_scratch );
        return 0;
    }

    free( scene->render_list );
    free( scene->render_scratch );
    scene->render_list = new_list;
    scene->render_scratch = new_scratch;
    scene->render_capacity = new_capacity;
    return 1;
}

static wp_s32 scene_add_clamped( wp_s32 value, wp_s32 increment, wp_s32 limit )
{
    return increment > limit - value ? limit : value + increment;
}

static wp_graphics_object **scene_sort_render_list( wp_graphics_scene *scene, wp_s32 count )
{
    wp_graphics_object **source;
    wp_graphics_object **destination;
    wp_graphics_object **swap;
    wp_s32 width;
    wp_s32 left;
    wp_s32 middle;
    wp_s32 right;
    wp_s32 left_index;
    wp_s32 right_index;
    wp_s32 output_index;

    source = scene->render_list;
    destination = scene->render_scratch;
    width = 1;
    while( width < count )
    {
        left = 0;
        while( left < count )
        {
            middle = scene_add_clamped( left, width, count );
            right = scene_add_clamped( middle, width, count );
            left_index = left;
            right_index = middle;
            output_index = left;

            while( left_index < middle && right_index < right )
            {
                if( scene_render_object_before( source[right_index], source[left_index] ) )
                {
                    destination[output_index++] = source[right_index++];
                }
                else
                {
                    destination[output_index++] = source[left_index++];
                }
            }
            while( left_index < middle )
            {
                destination[output_index++] = source[left_index++];
            }
            while( right_index < right )
            {
                destination[output_index++] = source[right_index++];
            }
            left = right;
        }

        swap = source;
        source = destination;
        destination = swap;
        if( width > count / 2 )
        {
            break;
        }
        width *= 2;
    }

    return source;
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_graphics_scene *wp_graphics_scene_create( void )
{
    wp_graphics_scene *scene = (wp_graphics_scene *)malloc( sizeof( wp_graphics_scene ) );
    if( !scene )
    {
        return NULL;
    }

    memset( scene, 0, sizeof( wp_graphics_scene ) );

    scene->visibility_mask = 0xFFFFFFFFu;
    scene->ambient_r = 0.2f;
    scene->ambient_g = 0.2f;
    scene->ambient_b = 0.2f;

    /* Default hemisphere direction: up */
    scene->hemisphere_dir.x = 0.0f;
    scene->hemisphere_dir.y = 1.0f;
    scene->hemisphere_dir.z = 0.0f;
    scene->envmap_scale = 1.0f;

    /* Fog defaults */
    scene->fog_mode = WP_FOG_NONE;
    scene->fog_colour.r = 1.0f;
    scene->fog_colour.g = 1.0f;
    scene->fog_colour.b = 1.0f;
    scene->fog_colour.a = 1.0f;
    scene->fog_density = 0.001f;
    scene->fog_start = 0.0f;
    scene->fog_end = 1.0f;

    /* Skybox defaults */
    scene->skybox_enabled = 0;
    scene->skybox_distance = 5000.0f;

    /* Shadow defaults */
    scene->shadows_enabled = 0;
    scene->depth_shadows = 1;

    /* Cameras */
    scene->active_camera = NULL;
    scene->default_camera = NULL;

    /* Node array */
    scene->nodes = (wp_scenenode **)malloc( WP_SCENE_INITIAL_NODE_CAPACITY * sizeof( wp_scenenode * ) );
    if( !scene->nodes )
    {
        free( scene );
        return NULL;
    }
    scene->node_capacity = WP_SCENE_INITIAL_NODE_CAPACITY;

    /* Object array */
    scene->objects = (wp_graphics_object **)malloc( WP_SCENE_INITIAL_OBJECT_CAPACITY *
                                                    sizeof( wp_graphics_object * ) );
    if( !scene->objects )
    {
        free( scene->nodes );
        free( scene );
        return NULL;
    }
    scene->object_capacity = WP_SCENE_INITIAL_OBJECT_CAPACITY;

    /* Root node */
    scene->root_node = wp_scenenode_create();
    if( !scene->root_node )
    {
        free( scene->objects );
        free( scene->nodes );
        free( scene );
        return NULL;
    }
    wp_scenenode_set_creator( scene->root_node, scene );
    scene->nodes[scene->node_count++] = scene->root_node;

    return scene;
}

void wp_graphics_scene_destroy( wp_graphics_scene *scene )
{
    wp_s32 i;

    if( !scene )
    {
        return;
    }

    scene_detach_all_objects( scene );

    for( i = 0; i < scene->object_count; ++i )
    {
        wp_graphics_object_destroy( scene->objects[i] );
    }

    free( scene->objects );
    free( scene->render_list );
    free( scene->render_scratch );

    for( i = 0; i < scene->node_count; ++i )
    {
        wp_scenenode_destroy( scene->nodes[i] );
    }

    free( scene->nodes );
    free( scene );
}

void wp_graphics_scene_clear( wp_graphics_scene *scene )
{
    wp_s32 i;

    if( !scene )
    {
        return;
    }

    scene_detach_all_objects( scene );

    /* Destroy all objects */
    for( i = 0; i < scene->object_count; ++i )
    {
        wp_graphics_object_destroy( scene->objects[i] );
    }

    scene->object_count = 0;

    /* Destroy all non-root nodes */
    for( i = scene->node_count - 1; i >= 0; --i )
    {
        if( scene->nodes[i] != scene->root_node )
        {
            wp_scenenode_destroy( scene->nodes[i] );
            scene->nodes[i] = scene->nodes[--scene->node_count];
        }
    }

    scene->active_camera = NULL;
    scene->default_camera = NULL;
}

/* =========================================================================
 * Scene node management
 * ====================================================================== */

wp_scenenode *wp_graphics_scene_create_node( wp_graphics_scene *scene )
{
    wp_scenenode *node;
    wp_scenenode **new_nodes;
    wp_s32 new_cap;

    if( !scene )
    {
        return NULL;
    }

    node = wp_scenenode_create();
    if( !node )
    {
        return NULL;
    }
    wp_scenenode_set_creator( node, scene );

    if( scene->node_count >= scene->node_capacity )
    {
        new_cap = scene->node_capacity * 2;
        new_nodes = (wp_scenenode **)realloc( scene->nodes, (wp_u32)new_cap * sizeof( wp_scenenode * ) );
        if( !new_nodes )
        {
            wp_scenenode_destroy( node );
            return NULL;
        }
        scene->nodes = new_nodes;
        scene->node_capacity = new_cap;
    }

    scene->nodes[scene->node_count++] = node;

    return node;
}

void wp_graphics_scene_destroy_node( wp_graphics_scene *scene, wp_scenenode *node )
{
    if( !scene || !node || node == scene->root_node )
    {
        return;
    }

    scene_remove_node( scene, node );
    wp_scenenode_destroy( node );
}

wp_scenenode *wp_graphics_scene_get_root_node( const wp_graphics_scene *scene )
{
    if( !scene )
    {
        return NULL;
    }

    return scene->root_node;
}

wp_s32 wp_graphics_scene_get_node_count( const wp_graphics_scene *scene )
{
    if( !scene )
    {
        return 0;
    }

    return scene->node_count;
}

/* =========================================================================
 * Graphics object management
 * ====================================================================== */

wp_graphics_object *wp_graphics_scene_create_object( wp_graphics_scene *scene )
{
    wp_graphics_object *obj;
    wp_graphics_object **new_objects;
    wp_s32 new_cap;

    if( !scene )
    {
        return NULL;
    }

    obj = wp_graphics_object_create();
    if( !obj )
    {
        return NULL;
    }
    wp_graphics_object_set_creator( obj, scene );

    if( scene->object_count >= scene->object_capacity )
    {
        new_cap = scene->object_capacity * 2;
        new_objects = (wp_graphics_object **)realloc( scene->objects,
                                                      (wp_u32)new_cap * sizeof( wp_graphics_object * ) );
        if( !new_objects )
        {
            wp_graphics_object_destroy( obj );
            return NULL;
        }
        scene->objects = new_objects;
        scene->object_capacity = new_cap;
    }

    scene->objects[scene->object_count++] = obj;

    return obj;
}

void wp_graphics_scene_destroy_object( wp_graphics_scene *scene, wp_graphics_object *obj )
{
    if( !scene || !obj )
    {
        return;
    }

    if( wp_graphics_object_get_owner( obj ) )
    {
        wp_scenenode_detach_object( wp_graphics_object_get_owner( obj ), obj );
    }
    scene_remove_object( scene, obj );
    wp_graphics_object_destroy( obj );
}

wp_s32 wp_graphics_scene_get_object_count( const wp_graphics_scene *scene )
{
    if( !scene )
    {
        return 0;
    }

    return scene->object_count;
}

/* =========================================================================
 * Visibility mask
 * ====================================================================== */

void wp_graphics_scene_set_visibility_mask( wp_graphics_scene *scene, wp_u32 mask )
{
    if( !scene )
    {
        return;
    }

    scene->visibility_mask = mask;
}

wp_u32 wp_graphics_scene_get_visibility_mask( const wp_graphics_scene *scene )
{
    if( !scene )
    {
        return 0;
    }

    return scene->visibility_mask;
}

/* =========================================================================
 * Ambient light
 * ====================================================================== */

void wp_graphics_scene_set_ambient_light( wp_graphics_scene *scene, wp_f32 r, wp_f32 g, wp_f32 b )
{
    if( !scene )
    {
        return;
    }

    scene->ambient_r = r;
    scene->ambient_g = g;
    scene->ambient_b = b;
}

void wp_graphics_scene_get_ambient_light( const wp_graphics_scene *scene, wp_f32 *r, wp_f32 *g,
                                          wp_f32 *b )
{
    if( !scene )
    {
        if( r )
        {
            *r = 0.0f;
        }
        if( g )
        {
            *g = 0.0f;
        }
        if( b )
        {
            *b = 0.0f;
        }
        return;
    }

    if( r )
    {
        *r = scene->ambient_r;
    }
    if( g )
    {
        *g = scene->ambient_g;
    }
    if( b )
    {
        *b = scene->ambient_b;
    }
}

/* =========================================================================
 * Hemisphere lighting
 * ====================================================================== */

void wp_graphics_scene_set_upper_hemisphere( wp_graphics_scene *scene, wp_colour4f colour )
{
    if( !scene )
    {
        return;
    }

    scene->upper_hemisphere = colour;
}

wp_colour4f wp_graphics_scene_get_upper_hemisphere( const wp_graphics_scene *scene )
{
    wp_colour4f zero;
    memset( &zero, 0, sizeof( wp_colour4f ) );
    if( !scene )
    {
        return zero;
    }
    return scene->upper_hemisphere;
}

void wp_graphics_scene_set_lower_hemisphere( wp_graphics_scene *scene, wp_colour4f colour )
{
    if( !scene )
    {
        return;
    }

    scene->lower_hemisphere = colour;
}

wp_colour4f wp_graphics_scene_get_lower_hemisphere( const wp_graphics_scene *scene )
{
    wp_colour4f zero;
    memset( &zero, 0, sizeof( wp_colour4f ) );
    if( !scene )
    {
        return zero;
    }
    return scene->lower_hemisphere;
}

void wp_graphics_scene_set_hemisphere_dir( wp_graphics_scene *scene, wp_vec3f dir )
{
    if( !scene )
    {
        return;
    }

    scene->hemisphere_dir = dir;
}

wp_vec3f wp_graphics_scene_get_hemisphere_dir( const wp_graphics_scene *scene )
{
    wp_vec3f zero;
    memset( &zero, 0, sizeof( wp_vec3f ) );
    if( !scene )
    {
        return zero;
    }
    return scene->hemisphere_dir;
}

void wp_graphics_scene_set_envmap_scale( wp_graphics_scene *scene, wp_f32 scale )
{
    if( !scene )
    {
        return;
    }

    scene->envmap_scale = scale;
}

wp_f32 wp_graphics_scene_get_envmap_scale( const wp_graphics_scene *scene )
{
    if( !scene )
    {
        return 0.0f;
    }

    return scene->envmap_scale;
}

/* =========================================================================
 * Fog
 * ====================================================================== */

void wp_graphics_scene_set_fog( wp_graphics_scene *scene, wp_fog_mode mode, wp_colour4f colour,
                                wp_f32 density, wp_f32 start, wp_f32 end )
{
    if( !scene )
    {
        return;
    }

    scene->fog_mode = mode;
    scene->fog_colour = colour;
    scene->fog_density = density;
    scene->fog_start = start;
    scene->fog_end = end;
}

wp_fog_mode wp_graphics_scene_get_fog_mode( const wp_graphics_scene *scene )
{
    if( !scene )
    {
        return WP_FOG_NONE;
    }

    return scene->fog_mode;
}

wp_colour4f wp_graphics_scene_get_fog_colour( const wp_graphics_scene *scene )
{
    wp_colour4f zero;
    memset( &zero, 0, sizeof( wp_colour4f ) );
    if( !scene )
    {
        return zero;
    }
    return scene->fog_colour;
}

wp_f32 wp_graphics_scene_get_fog_density( const wp_graphics_scene *scene )
{
    if( !scene )
    {
        return 0.0f;
    }

    return scene->fog_density;
}

wp_f32 wp_graphics_scene_get_fog_start( const wp_graphics_scene *scene )
{
    if( !scene )
    {
        return 0.0f;
    }

    return scene->fog_start;
}

wp_f32 wp_graphics_scene_get_fog_end( const wp_graphics_scene *scene )
{
    if( !scene )
    {
        return 0.0f;
    }

    return scene->fog_end;
}

/* =========================================================================
 * Skybox
 * ====================================================================== */

void wp_graphics_scene_set_skybox( wp_graphics_scene *scene, wp_s32 enable, wp_f32 distance )
{
    if( !scene )
    {
        return;
    }

    scene->skybox_enabled = enable;
    scene->skybox_distance = distance;
}

wp_s32 wp_graphics_scene_get_skybox_enabled( const wp_graphics_scene *scene )
{
    if( !scene )
    {
        return 0;
    }

    return scene->skybox_enabled;
}

wp_f32 wp_graphics_scene_get_skybox_distance( const wp_graphics_scene *scene )
{
    if( !scene )
    {
        return 0.0f;
    }

    return scene->skybox_distance;
}

/* =========================================================================
 * Shadows
 * ====================================================================== */

void wp_graphics_scene_set_shadows( wp_graphics_scene *scene, wp_s32 enable, wp_s32 depth_shadows )
{
    if( !scene )
    {
        return;
    }

    scene->shadows_enabled = enable;
    scene->depth_shadows = depth_shadows;
}

wp_s32 wp_graphics_scene_get_shadows_enabled( const wp_graphics_scene *scene )
{
    if( !scene )
    {
        return 0;
    }

    return scene->shadows_enabled;
}

wp_s32 wp_graphics_scene_get_depth_shadows( const wp_graphics_scene *scene )
{
    if( !scene )
    {
        return 0;
    }

    return scene->depth_shadows;
}

/* =========================================================================
 * Camera
 * ====================================================================== */

wp_camera *wp_graphics_scene_get_active_camera( const wp_graphics_scene *scene )
{
    if( !scene )
    {
        return NULL;
    }

    return scene->active_camera;
}

void wp_graphics_scene_set_active_camera( wp_graphics_scene *scene, wp_camera *camera )
{
    if( !scene )
    {
        return;
    }

    scene->active_camera = camera;
}

wp_camera *wp_graphics_scene_get_default_camera( const wp_graphics_scene *scene )
{
    if( !scene )
    {
        return NULL;
    }

    return scene->default_camera;
}

void wp_graphics_scene_set_default_camera( wp_graphics_scene *scene, wp_camera *camera )
{
    if( !scene )
    {
        return;
    }

    scene->default_camera = camera;
}

void wp_graphics_scene_render( wp_graphics_scene *scene, wp_renderer *renderer )
{
    wp_graphics_scene_render_with_submit( scene, renderer, NULL, NULL );
}

void wp_graphics_scene_render_with_submit( wp_graphics_scene *scene, wp_renderer *renderer,
                                           wp_graphics_scene_submit_func submit, void *user_data )
{
    wp_graphics_object **render_list;
    wp_graphics_object **sorted_list;
    wp_graphics_object *obj;
    wp_graphics_object_render_func render_func;
    wp_camera *camera;
    wp_u32 visibility_mask;
    wp_s32 render_count;
    wp_s32 i;
    wp_mat4f view_matrix;
    wp_mat4f projection_matrix;
    wp_mat4f view_projection;

    if( !scene || !renderer || scene->object_count <= 0 )
    {
        return;
    }

    if( !scene_ensure_render_capacity( scene, scene->object_count ) )
    {
        return;
    }
    render_list = scene->render_list;

    camera = scene->active_camera ? scene->active_camera : scene->default_camera;
    visibility_mask = scene->visibility_mask;
    if( camera )
    {
        visibility_mask &= wp_camera_get_visibility_mask( camera );
        wp_camera_get_view_matrix( camera, &view_matrix );
        wp_camera_get_projection_matrix( camera, &projection_matrix );
        wp_mat4f_mul( &view_projection, &projection_matrix, &view_matrix );
        wp_renderer_set_view_matrix( renderer, &view_matrix );
        wp_renderer_set_projection_matrix( renderer, &projection_matrix );
    }

    render_count = 0;
    for( i = 0; i < scene->object_count; ++i )
    {
        obj = scene->objects[i];
        if( !obj || !wp_graphics_object_is_visible( obj ) )
        {
            continue;
        }

        if( !( wp_graphics_object_get_visibility_flags( obj ) & visibility_mask ) )
        {
            continue;
        }

        if( camera && scene_mesh_outside_frustum( obj, &view_projection ) )
        {
            continue;
        }

        render_func = wp_graphics_object_get_render_func( obj );
        if( !render_func )
        {
            continue;
        }

        render_list[render_count++] = obj;
    }

    sorted_list = scene_sort_render_list( scene, render_count );
    for( i = 0; i < render_count; ++i )
    {
        obj = sorted_list[i];
        if( submit && submit( obj, renderer, user_data ) )
        {
            continue;
        }
        render_func = wp_graphics_object_get_render_func( obj );
        if( render_func )
        {
            render_func( obj, renderer );
        }
    }
}

/* =========================================================================
 * Native access
 * ====================================================================== */

void wp_graphics_scene_get_native( const wp_graphics_scene *scene, void **pp_object )
{
    if( !pp_object )
    {
        return;
    }

    *pp_object = scene ? scene->native : NULL;
}

void wp_graphics_scene_set_native( wp_graphics_scene *scene, void *native )
{
    if( !scene )
    {
        return;
    }

    scene->native = native;
}
