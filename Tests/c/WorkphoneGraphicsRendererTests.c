#include "workphone_graphics_renderer.h"
#include "workphone_graphics_system.h"
#include "workphone_graphics_scene.h"
#include "workphone_graphics_object.h"
#include "workphone_graphics_camera.h"
#include "workphone_graphics_mesh.h"
#include "workphone_graphics_material.h"
#include "workphone_graphics_scenenode.h"

#include <stdio.h>
#include <string.h>

#define TEST_WIDTH 64
#define TEST_HEIGHT 64

static int g_render_calls;
static int g_frame_calls;
static wp_f32 g_frame_delta_time;
static wp_graphics_object *g_order_objects[3];
static int g_render_order[3];
static int g_render_order_count;
static int g_material_sync_calls;
static int g_material_bind_calls;
static int g_submit_calls;

static wp_s32 sync_test_material( const wp_graphics_material *material, void *native,
                                  void *user_data )
{
    (void)material;
    (void)native;
    (void)user_data;
    ++g_material_sync_calls;
    return 1;
}

static wp_s32 bind_test_material( const wp_graphics_material *material, wp_renderer *renderer,
                                  void *native, void *user_data )
{
    (void)material;
    (void)renderer;
    (void)native;
    (void)user_data;
    ++g_material_bind_calls;
    return 1;
}

static int check( int condition, const char *message )
{
    if( !condition )
    {
        fprintf( stderr, "FAIL: %s\n", message );
        return 0;
    }
    return 1;
}

static void render_test_triangle( wp_graphics_object *object, wp_renderer *renderer )
{
    const wp_vertex_pc vertices[] = {
        { { -0.75f, -0.75f, 0.0f }, 0xff0000ffu },
        { { 0.75f, -0.75f, 0.0f }, 0xff0000ffu },
        { { 0.0f, 0.75f, 0.0f }, 0xff0000ffu },
    };
    (void)object;
    ++g_render_calls;
    wp_renderer_draw_triangles_pc( renderer, vertices, 3 );
}

static void render_test_frame( wp_graphics_system *system, wp_f32 delta_time, void *user_data )
{
    (void)user_data;
    ++g_frame_calls;
    g_frame_delta_time = delta_time;
    wp_graphics_system_render( system );
}

static void record_render_order( wp_graphics_object *object, wp_renderer *renderer )
{
    int i;
    (void)renderer;
    for( i = 0; i < 3; ++i )
    {
        if( object == g_order_objects[i] && g_render_order_count < 3 )
        {
            g_render_order[g_render_order_count++] = i;
            return;
        }
    }
}

static wp_s32 submit_test_object( wp_graphics_object *object, wp_renderer *renderer, void *user_data )
{
    ++g_submit_calls;
    if( object == (wp_graphics_object *)user_data )
    {
        record_render_order( object, renderer );
        return 1;
    }
    return 0;
}

static int framebuffer_contains_non_black_pixel( const unsigned char *pixels )
{
    int i;
    for( i = 0; i < TEST_WIDTH * TEST_HEIGHT; ++i )
    {
        const unsigned char *pixel = pixels + i * 4;
        if( pixel[0] != 0 || pixel[1] != 0 || pixel[2] != 0 )
        {
            return 1;
        }
    }
    return 0;
}

static wp_s32 count_culled_submission( wp_graphics_object *object, wp_renderer *renderer,
                                       void *user_data )
{
    (void)object;
    (void)renderer;
    (void)user_data;
    ++g_submit_calls;
    return 1;
}

static int expect_mesh_submission( wp_graphics_scene *scene, wp_renderer *renderer,
                                    int expected, const char *message )
{
    g_submit_calls = 0;
    wp_graphics_scene_render_with_submit( scene, renderer, count_culled_submission, NULL );
    return check( g_submit_calls == expected, message );
}

static int test_frustum_culling( wp_renderer *renderer )
{
    wp_graphics_scene *scene = wp_graphics_scene_create();
    wp_graphics_mesh *mesh = wp_graphics_mesh_create();
    wp_graphics_object *object = wp_graphics_scene_create_object( scene );
    wp_scenenode *node = wp_graphics_scene_create_node( scene );
    wp_scenenode *parent = wp_graphics_scene_create_node( scene );
    wp_camera *camera = wp_camera_create();
    wp_camera *second_camera = wp_camera_create();
    wp_aabb3f bounds = { { -0.25f, -0.25f, -0.25f }, { 0.25f, 0.25f, 0.25f } };
    wp_vec3f position = { 0, 0, -5 };
    wp_vec3f scale = { -2, 1, 1 };
    wp_quatf orientation = { 0.70710678f, 0, 0, 0.70710678f };
    int ok = check( scene && mesh && object && node && parent && camera && second_camera,
                    "frustum fixtures should be created" );
    if( ok )
    {
        wp_graphics_mesh_set_local_aabb( mesh, bounds );
        wp_graphics_object_set_mesh( object, mesh );
        wp_scenenode_attach_object( node, object );
        wp_camera_set_near_clip_distance( camera, 0.5f );
        wp_camera_set_far_clip_distance( camera, 50.0f );
        wp_camera_set_aspect_ratio( camera, 1.0f );
        wp_camera_set_fov_y( camera, 1.57079633f );
        wp_graphics_scene_set_active_camera( scene, camera );
        wp_scenenode_set_position( node, position );
        ok &= expect_mesh_submission( scene, renderer, 1, "mesh inside perspective frustum must draw" );
        position.x = 20;
        wp_scenenode_set_position( node, position );
        ok &= expect_mesh_submission( scene, renderer, 0, "mesh outside right plane must be culled" );
        position.x = -20;
        wp_scenenode_set_position( node, position );
        ok &= expect_mesh_submission( scene, renderer, 0, "mesh outside left plane must be culled" );
        position.x = 0;
        position.y = 20;
        wp_scenenode_set_position( node, position );
        ok &= expect_mesh_submission( scene, renderer, 0, "mesh above frustum must be culled" );
        position.y = -20;
        wp_scenenode_set_position( node, position );
        ok &= expect_mesh_submission( scene, renderer, 0, "mesh below frustum must be culled" );
        position.y = 0;
        position.z = 5;
        wp_scenenode_set_position( node, position );
        ok &= expect_mesh_submission( scene, renderer, 0, "mesh behind camera must be culled" );
        position.z = -0.1f;
        wp_scenenode_set_position( node, position );
        ok &= expect_mesh_submission( scene, renderer, 0, "mesh before near plane must be culled" );
        position.z = -60;
        wp_scenenode_set_position( node, position );
        ok &= expect_mesh_submission( scene, renderer, 0, "mesh beyond far plane must be culled" );

        wp_camera_set_projection_type( camera, WORKPHONE_PROJECTION_ORTHOGRAPHIC );
        wp_camera_set_ortho_width( camera, 1.0f );
        position.x = 1.2f;
        position.z = -5;
        wp_scenenode_set_position( node, position );
        ok &= expect_mesh_submission( scene, renderer, 1, "box intersecting an ortho plane must draw" );
        position.x = 2;
        wp_scenenode_set_position( node, position );
        ok &= expect_mesh_submission( scene, renderer, 0, "box outside ortho frustum must be culled" );
        position.x = 0;
        wp_scenenode_set_position( parent, position );
        position.z = 0;
        wp_scenenode_set_position( node, position );
        wp_scenenode_set_parent( node, parent );
        wp_scenenode_set_scale( node, scale );
        wp_scenenode_set_orientation( node, orientation );
        ok &= expect_mesh_submission( scene, renderer, 1, "parented rotated negative-scale bounds must draw" );

        position.x = 20;
        position.z = -5;
        wp_scenenode_set_position( parent, position );
        ok &= expect_mesh_submission( scene, renderer, 0, "parent transform must participate in culling" );
        position.z = 0;
        wp_camera_set_position( second_camera, position );
        wp_graphics_scene_set_active_camera( scene, second_camera );
        ok &= expect_mesh_submission( scene, renderer, 1, "visibility must be recomputed for a second camera" );
        ok &= check( wp_graphics_object_is_visible( object ), "culling must not change object visibility" );
        wp_graphics_scene_set_active_camera( scene, NULL );
        ok &= expect_mesh_submission( scene, renderer, 1, "scene without a camera must retain its meshes" );
        wp_graphics_scene_set_active_camera( scene, camera );
        memset( &bounds, 0, sizeof( bounds ) );
        wp_graphics_mesh_set_local_aabb( mesh, bounds );
        ok &= expect_mesh_submission( scene, renderer, 1, "mesh without known bounds must draw conservatively" );
        bounds.min.x = bounds.min.y = bounds.min.z = -100;
        bounds.max.x = bounds.max.y = bounds.max.z = 100;
        wp_graphics_mesh_set_local_aabb( mesh, bounds );
        ok &= expect_mesh_submission( scene, renderer, 1, "updated mesh bounds must be used even after attachment" );
        wp_graphics_scene_set_active_camera( scene, NULL );
        wp_graphics_object_set_mesh( object, NULL );
    }
    wp_graphics_scene_destroy( scene );
    wp_graphics_mesh_destroy( mesh );
    wp_camera_destroy( camera );
    wp_camera_destroy( second_camera );
    return ok;
}

int main( void )
{
    wp_renderer *renderer =
        wp_renderer_create_software( TEST_WIDTH, TEST_HEIGHT, WORKPHONE_PIXEL_FORMAT_BGRA8 );
    wp_graphics_system *system = wp_graphics_system_create();
    wp_graphics_scene *scene;
    wp_graphics_object *object;
    wp_graphics_object *second_object;
    wp_graphics_object *third_object;
    wp_camera *camera;
    wp_graphics_mesh *mesh;
    wp_graphics_material *material;
    wp_graphics_object *mesh_object;
    wp_scenenode *mesh_node;
    wp_mat4f mesh_world;
    const unsigned char *pixels;
    int ok = 1;

    ok &= check( renderer != NULL, "software renderer should be created" );
    ok &= check( system != NULL, "graphics system should be created" );
    if( !ok )
    {
        wp_renderer_destroy( renderer );
        wp_graphics_system_destroy( system );
        return 1;
    }

    ok &= check( wp_renderer_get_width( renderer ) == TEST_WIDTH,
                 "renderer should report its width" );
    ok &= check( wp_renderer_get_height( renderer ) == TEST_HEIGHT,
                 "renderer should report its height" );

    wp_renderer_begin_frame( renderer );
    wp_renderer_set_clear_color( renderer, 0.0f, 0.0f, 0.0f, 1.0f );
    wp_renderer_set_clear_depth( renderer, 1.0f );
    wp_renderer_clear( renderer, WORKPHONE_CLEAR_FLAG_ALL );
    wp_renderer_end_frame( renderer );

    pixels = (const unsigned char *)wp_renderer_get_framebuffer( renderer );
    ok &= check( pixels != NULL, "software renderer should expose a framebuffer" );
    ok &= check( pixels && !framebuffer_contains_non_black_pixel( pixels ),
                 "clear should write the requested colour" );

    wp_graphics_system_set_renderer( system, renderer );
    ok &= check( wp_graphics_system_get_renderer( system ) == renderer,
                 "graphics system should retain its renderer binding" );

    scene = wp_graphics_system_create_scene( system );
    object = wp_graphics_scene_create_object( scene );
    ok &= check( scene != NULL && object != NULL, "scene and object should be created" );
    ok &= check( wp_graphics_system_get_scene_count( system ) == 1,
                 "system should own the created scene" );

    wp_graphics_object_set_render_func( object, render_test_triangle );
    wp_renderer_begin_frame( renderer );
    wp_renderer_clear( renderer, WORKPHONE_CLEAR_FLAG_ALL );
    g_render_calls = 0;
    wp_graphics_system_render_frame( system, 1.0f / 60.0f );
    wp_renderer_end_frame( renderer );

    pixels = (const unsigned char *)wp_renderer_get_framebuffer( renderer );
    ok &= check( g_render_calls == 1, "visible object should be submitted exactly once" );
    ok &= check( pixels && framebuffer_contains_non_black_pixel( pixels ),
                 "submitted geometry should change framebuffer pixels" );

    wp_graphics_system_set_frame_func( system, render_test_frame, NULL );
    g_render_calls = 0;
    g_frame_calls = 0;
    g_frame_delta_time = 0.0f;
    wp_graphics_system_render_frame( system, 0.02f );
    ok &= check( g_frame_calls == 1, "frame callback should run exactly once" );
    ok &= check( g_render_calls == 1, "frame callback should submit native objects" );
    ok &= check( g_frame_delta_time > 0.019f && g_frame_delta_time < 0.021f,
                 "frame callback should receive delta time" );
    wp_graphics_system_set_frame_func( system, NULL, NULL );

    wp_graphics_object_set_visible( object, 0 );
    g_render_calls = 0;
    wp_graphics_system_render( system );
    ok &= check( g_render_calls == 0, "hidden objects should not be submitted" );

    wp_graphics_object_set_visible( object, 1 );
    wp_graphics_object_set_visibility_flags( object, 0x2u );
    wp_graphics_scene_set_visibility_mask( scene, 0x1u );
    g_render_calls = 0;
    wp_graphics_system_render( system );
    ok &= check( g_render_calls == 0, "scene visibility mask should filter objects" );

    second_object = wp_graphics_scene_create_object( scene );
    third_object = wp_graphics_scene_create_object( scene );
    camera = wp_camera_create();
    ok &= check( second_object != NULL && third_object != NULL && camera != NULL,
                 "scene render ordering fixtures should be created" );
    if( second_object && third_object && camera )
    {
        g_order_objects[0] = object;
        g_order_objects[1] = second_object;
        g_order_objects[2] = third_object;
        wp_graphics_object_set_render_func( object, record_render_order );
        wp_graphics_object_set_render_func( second_object, record_render_order );
        wp_graphics_object_set_render_func( third_object, record_render_order );
        wp_graphics_object_set_visible( object, 1 );
        wp_graphics_object_set_visibility_flags( object, 0x2u );
        wp_graphics_object_set_visibility_flags( second_object, 0x2u );
        wp_graphics_object_set_visibility_flags( third_object, 0x2u );
        wp_graphics_scene_set_visibility_mask( scene, 0xFFFFFFFFu );
        wp_camera_set_visibility_mask( camera, 0x2u );
        wp_graphics_scene_set_active_camera( scene, camera );

        wp_graphics_object_set_render_queue_group( object, WORKPHONE_RENDER_QUEUE_DEFAULT );
        wp_graphics_object_set_z_order( object, 10u );
        wp_graphics_object_set_render_queue_group( second_object,
                                                   WORKPHONE_RENDER_QUEUE_BACKGROUND );
        wp_graphics_object_set_z_order( second_object, 100u );
        wp_graphics_object_set_render_queue_group( third_object,
                                                   WORKPHONE_RENDER_QUEUE_OVERLAY );
        wp_graphics_object_set_z_order( third_object, 0u );

        g_render_order_count = 0;
        wp_graphics_scene_render( scene, renderer );
        ok &= check( g_render_order_count == 3, "scene should submit every visible object" );
        ok &= check( g_render_order[0] == 1 && g_render_order[1] == 0 &&
                         g_render_order[2] == 2,
                     "scene should sort by render queue before Z-order" );

        wp_graphics_object_set_render_queue_group( object, WORKPHONE_RENDER_QUEUE_DEFAULT );
        wp_graphics_object_set_render_queue_group( second_object, WORKPHONE_RENDER_QUEUE_DEFAULT );
        wp_graphics_object_set_render_queue_group( third_object, WORKPHONE_RENDER_QUEUE_DEFAULT );
        wp_graphics_object_set_z_order( object, 10u );
        wp_graphics_object_set_z_order( second_object, 5u );
        wp_graphics_object_set_z_order( third_object, 5u );
        g_render_order_count = 0;
        wp_graphics_scene_render( scene, renderer );
        ok &= check( g_render_order[0] == 1 && g_render_order[1] == 2 &&
                         g_render_order[2] == 0,
                     "scene should sort by Z-order and preserve creation order for ties" );

        wp_camera_set_visibility_mask( camera, 0x1u );
        g_render_order_count = 0;
        g_submit_calls = 0;
        wp_camera_set_visibility_mask( camera, 0x2u );
        wp_graphics_scene_render_with_submit( scene, renderer, submit_test_object, second_object );
        ok &= check( g_submit_calls == 3 && g_render_order_count == 3,
                     "submission override should draw handled objects once and retain fallback" );
        ok &= check( g_render_order[0] == 1 && g_render_order[1] == 2 && g_render_order[2] == 0,
                     "submission override should preserve sorted render order" );
        wp_camera_set_visibility_mask( camera, 0x1u );
        g_render_order_count = 0;
        g_submit_calls = 0;
        wp_graphics_scene_render_with_submit( scene, renderer, submit_test_object, second_object );
        ok &= check( g_render_order_count == 0 && g_submit_calls == 0,
                     "active camera visibility mask should filter scene objects" );
    }

    wp_graphics_scene_set_active_camera( scene, NULL );
    wp_camera_destroy( camera );

    {
        const wp_graphics_mesh_vertex_ptc vertices[] = {
            { { -0.4f, -0.4f, 0.0f }, { 0.0f, 1.0f }, 0x00FF00FFu },
            { { 0.4f, -0.4f, 0.0f }, { 1.0f, 1.0f }, 0x00FF00FFu },
            { { 0.0f, 0.4f, 0.0f }, { 0.5f, 0.0f }, 0x00FF00FFu }
        };
        const wp_u32 indices[] = { 0u, 1u, 2u };
        wp_vec3f position = { 0.1f, 0.2f, 0.0f };
        wp_vec3f camera_position = { 0.0f, 0.0f, 2.0f };
        wp_vec3f camera_target = { 0.0f, 0.0f, 0.0f };
        wp_vec3f camera_up = { 0.0f, 1.0f, 0.0f };
        wp_camera *mesh_camera;

        mesh = wp_graphics_mesh_create();
        material = wp_graphics_material_create();
        mesh_object = wp_graphics_scene_create_object( scene );
        mesh_node = wp_graphics_scene_create_node( scene );
        mesh_camera = wp_camera_create();
        ok &= check( mesh && material && mesh_object && mesh_node && mesh_camera,
                     "native mesh render fixtures should be created" );
        if( mesh && material && mesh_object && mesh_node && mesh_camera )
        {
            wp_graphics_material_desc material_desc;
            wp_graphics_material *material_copy;
            wp_u32 revision;
            wp_graphics_material_desc_init( &material_desc );
            ok &= check( material_desc.diffuse.a == 1.0f &&
                             material_desc.specular.r > 0.039f &&
                             material_desc.specular.r < 0.041f,
                         "material descriptor should have safe PBR defaults" );
            wp_graphics_material_clear_dirty( material );
            revision = wp_graphics_material_get_revision( material );
            wp_graphics_material_set_roughness( material, 0.5f );
            ok &= check( wp_graphics_material_get_revision( material ) == revision &&
                             !wp_graphics_material_is_dirty( material ),
                         "an unchanged material value should not invalidate backend state" );
            wp_graphics_material_set_metalness( material, 2.0f );
            wp_graphics_material_set_normal_scale( material, -2.0f );
            ok &= check( wp_graphics_material_get_metalness( material ) == 1.0f &&
                             wp_graphics_material_get_normal_scale( material ) == 0.0f,
                         "material PBR values should be clamped to valid ranges" );
            wp_graphics_material_set_texture_name(
                material, WORKPHONE_MATERIAL_TEXTURE_BASE_COLOUR, "paint/base_colour.dds" );
            material_copy = wp_graphics_material_clone( material );
            ok &= check( material_copy != NULL &&
                             strcmp( wp_graphics_material_get_texture_name(
                                         material_copy, WORKPHONE_MATERIAL_TEXTURE_BASE_COLOUR ),
                                     "paint/base_colour.dds" ) == 0,
                         "material cloning should preserve texture semantics" );
            wp_graphics_material_destroy( material_copy );
            g_material_sync_calls = 0;
            g_material_bind_calls = 0;
            wp_graphics_material_set_sync_func( material, sync_test_material, NULL );
            wp_graphics_material_set_bind_func( material, bind_test_material, NULL );

            ok &= check( wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_PTC,
                                                        vertices, 3u ) != 0,
                         "native mesh vertices should be accepted" );
            ok &= check( wp_graphics_mesh_set_indices_u32( mesh, indices, 3u ) != 0,
                         "native mesh 32-bit indices should be accepted" );
            wp_graphics_object_set_mesh( mesh_object, mesh );
            wp_graphics_object_set_material( mesh_object, material );
            wp_scenenode_set_position( mesh_node, position );
            wp_scenenode_set_parent( mesh_node, wp_graphics_scene_get_root_node( scene ) );
            wp_scenenode_attach_object( mesh_node, mesh_object );
            wp_scenenode_get_world_matrix( mesh_node, &mesh_world );
            ok &= check( mesh_world.m[0][3] > 0.099f && mesh_world.m[0][3] < 0.101f &&
                             mesh_world.m[1][3] > 0.199f && mesh_world.m[1][3] < 0.201f,
                         "native scene node should calculate its world transform" );

            wp_graphics_object_set_visible( object, 0 );
            wp_graphics_object_set_visible( second_object, 0 );
            wp_graphics_object_set_visible( third_object, 0 );
            wp_camera_set_position( mesh_camera, camera_position );
            wp_camera_look_at( mesh_camera, camera_target, camera_up );
            wp_graphics_scene_set_active_camera( scene, mesh_camera );
            wp_renderer_begin_frame( renderer );
            wp_renderer_clear( renderer, WORKPHONE_CLEAR_FLAG_ALL );
            wp_graphics_scene_render( scene, renderer );
            wp_renderer_end_frame( renderer );
            ok &= check( g_material_sync_calls == 1 && g_material_bind_calls == 1 &&
                             !wp_graphics_material_is_dirty( material ),
                         "scene rendering should synchronize and bind material backend state" );
            pixels = (const unsigned char *)wp_renderer_get_framebuffer( renderer );
            ok &= check( pixels && framebuffer_contains_non_black_pixel( pixels ),
                         "built-in C89 mesh renderable should draw scene geometry" );

            wp_scenenode_detach_object( mesh_node, mesh_object );
            wp_graphics_object_set_mesh( mesh_object, NULL );
            wp_graphics_object_set_material( mesh_object, NULL );
            wp_graphics_scene_set_active_camera( scene, NULL );
        }
        wp_camera_destroy( mesh_camera );
        wp_graphics_material_destroy( material );
        wp_graphics_mesh_destroy( mesh );
    }

    ok &= test_frustum_culling( renderer );
    wp_graphics_system_set_renderer( system, NULL );
    wp_graphics_system_destroy( system );
    wp_renderer_destroy( renderer );

    if( !ok )
    {
        return 1;
    }

    puts( "Workphone graphics renderer contract tests passed." );
    return 0;
}
