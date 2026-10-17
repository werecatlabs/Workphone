#pragma once
#include <WorkphoneGraphics/workphone_graphics_cull_job.h>
#include <WorkphoneGraphics/workphone_graphics_camera.h>
#include <WorkphoneGraphics/workphone_graphics_scenenode.h>

namespace claw_scene_job_contracts
{
    using claw_rtt_contracts::pixels;
    using claw_rtt_contracts::require;

    struct SnapshotDraw
    {
        wp_graphics_scene *scene;
        wp_graphics_cull_result *result;
        wp_renderer *renderer;
    };

    inline wp_rtt_result ready( void *p, const wp_graphics_rtt_desc * )
    {
        auto &d = *static_cast<SnapshotDraw *>( p );
        return wp_graphics_cull_result_validate(
                   d.result, d.scene,
                   static_cast<wp_u32>( wp_graphics_scene_get_generation( d.scene ) ) ) ==
                       WP_GRAPHICS_CULL_OK
                   ? WP_RTT_OK
                   : WP_RTT_STALE;
    }

    inline wp_rtt_result draw( void *p, const wp_graphics_rtt_desc * )
    {
        auto &d = *static_cast<SnapshotDraw *>( p );
        return wp_graphics_scene_render_cull_result( d.scene, d.renderer, d.result, nullptr, nullptr ) ==
                       WP_GRAPHICS_CULL_OK
                   ? WP_RTT_OK
                   : WP_RTT_STALE;
    }

    inline bool pixelsMatch( workphone::render::ClawRendererDX11 &wrapper )
    {
        auto *generic = wrapper.getNativeRenderer();
        auto *native = wp_renderer_get_dx11( generic );
        std::unique_ptr<wp_graphics_scene, decltype( &wp_graphics_scene_destroy )> scene(
            wp_graphics_scene_create(), wp_graphics_scene_destroy );
        std::unique_ptr<wp_camera, decltype( &wp_camera_destroy )> camera( wp_camera_create(),
                                                                           wp_camera_destroy );
        std::unique_ptr<wp_graphics_mesh, decltype( &wp_graphics_mesh_destroy )> mesh(
            wp_graphics_mesh_create(), wp_graphics_mesh_destroy );
        std::unique_ptr<wp_render_texture_dx11, decltype( &wp_renderer_dx11_destroy_render_texture )>
            target( wp_renderer_dx11_create_render_texture( native, 32, 32 ),
                    wp_renderer_dx11_destroy_render_texture );
        require( scene && camera && mesh && target, "scene snapshot GPU fixtures" );
        const wp_vertex_ptc vertices[] = { { { -.25f, -.25f, -2 }, { 0, 0 }, 0xFFFFFFFFu },
                                           { { .25f, -.25f, -2 }, { 1, 0 }, 0xFFFFFFFFu },
                                           { { 0, .25f, -2 }, { .5f, 1 }, 0xFFFFFFFFu } };
        require( wp_graphics_mesh_set_vertices( mesh.get(), WORKPHONE_VERTEX_FORMAT_PTC, vertices, 3 ),
                 "snapshot bounds fixture" );
        wp_graphics_mesh_compute_aabb( mesh.get() );
        wp_graphics_scene_set_active_camera( scene.get(), camera.get() );
        for( int i = 0; i < 3; ++i )
        {
            auto *node = wp_graphics_scene_create_node( scene.get() );
            auto *object = wp_graphics_scene_create_object( scene.get() );
            wp_graphics_object_set_mesh( object, mesh.get() );
            wp_scenenode_attach_object( node, object );
            wp_scenenode_set_position( node, { i == 2 ? 100.f : i == 1 ? .5f : -.5f, 0, 0 } );
            wp_graphics_object_set_render_func( object, []( wp_graphics_object *o, wp_renderer *r ) {
                auto *mesh = wp_graphics_object_get_mesh( o );
                wp_renderer_set_texture_native( r, nullptr );
                wp_renderer_set_cull_mode( r, WORKPHONE_CULL_MODE_NONE );
                wp_renderer_set_blend_mode( r, WORKPHONE_BLEND_MODE_NONE );
                wp_renderer_set_depth_test_enabled( r, 0 );
                wp_renderer_draw_triangles_ptc(
                    r, static_cast<const wp_vertex_ptc *>( wp_graphics_mesh_get_vertices( mesh ) ), 3 );
            } );
        }
        std::unique_ptr<wp_graphics_cull_result, decltype( &wp_graphics_cull_result_destroy )>
            publication(
                wp_graphics_scene_prepare_cull( scene.get(), camera.get(), WP_GRAPHICS_CULL_PASS_MAIN ),
                wp_graphics_cull_result_destroy ),
            latest( nullptr, wp_graphics_cull_result_destroy );
        require( publication &&
                     wp_graphics_cull_result_get_transform_dispatch_count( publication.get() ) == 1,
                 "shared geometry publication must dispatch transforms once" );
        wp_graphics_rtt_desc lastDesc{};
        std::vector<unsigned char> views[2];
        for( int view = 0; view < 2; ++view )
        {
            wp_camera_set_position( camera.get(), { view ? .25f : 0.f, 0, 0 } );
            std::unique_ptr<wp_graphics_cull_result, decltype( &wp_graphics_cull_result_destroy )>
                result( wp_graphics_cull_result_create_view( publication.get(), camera.get(),
                                                             WP_GRAPHICS_CULL_PASS_MAIN ),
                        wp_graphics_cull_result_destroy );
            require( result && !wp_graphics_cull_result_get_transform_dispatch_count( result.get() ) &&
                         wp_graphics_cull_result_get_visible_count( result.get() ) == 2,
                     "per-view main list excludes offscreen mesh" );
            wp_renderer_dx11_set_render_texture( native, target.get() );
            wp_renderer_dx11_set_viewport( native, { 0, 0, 32, 32 } );
            wp_renderer_dx11_set_clear_color( native, 0, 0, 0, 1 );
            wp_renderer_dx11_clear( native, WORKPHONE_CLEAR_FLAG_ALL );
            wp_graphics_scene_render_with_submit(
                scene.get(), generic,
                []( wp_graphics_object *o, wp_renderer *r, void * ) -> wp_s32 {
                    wp_mat4f world;
                    wp_scenenode_get_world_matrix( wp_graphics_object_get_owner( o ), &world );
                    wp_renderer_set_world_matrix( r, &world );
                    return 0;
                },
                nullptr );
            const auto reference = pixels( target.get() );
            bool visible = false;
            for( size_t i = 0; i < reference.size(); i += 4 )
            {
                visible |= reference[i] != 0;
            }
            require( visible, "scene reference must produce visible geometry" );
            wp_renderer_dx11_set_render_texture( native, nullptr );
            SnapshotDraw d{ scene.get(), result.get(), generic };
            wp_graphics_rtt_desc desc{};
            desc.renderer_generation = wp_renderer_dx11_get_rtt_generation( native );
            desc.target_generation = wp_renderer_dx11_get_render_texture_generation( target.get() );
            desc.width = desc.height = 32;
            desc.viewport = { 0, 0, 32, 32 };
            desc.clear_colour = desc.clear_depth = 1;
            desc.colour[3] = 1;
            desc.depth = 1;
            require( wp_renderer_dx11_execute_rtt_job( native, target.get(), &desc, ready, ready, draw,
                                                       &d ) == WP_RTT_OK,
                     "transform/cull dependency to RTT execution" );
            views[view] = pixels( target.get() );
            require( views[view] == reference,
                     "complete per-view job pixels must equal synchronous scene reference" );
            lastDesc = desc;
            latest = std::move( result );
        }
        require( views[0] != views[1], "independent camera views must produce different images" );
        publication.reset();
        SnapshotDraw retained{ scene.get(), latest.get(), generic };
        require( wp_renderer_dx11_execute_rtt_job( native, target.get(), &lastDesc, ready, ready, draw,
                                                   &retained ) == WP_RTT_OK &&
                     pixels( target.get() ) == views[1],
                 "derived GPU view must survive source publication retirement" );
        wp_scenenode_set_position( wp_graphics_object_get_owner( scene->objects[0] ), { -.6f, 0, 0 } );
        require( wp_renderer_dx11_execute_rtt_job( native, target.get(), &lastDesc, ready, ready, draw,
                                                   &retained ) == WP_RTT_STALE &&
                     pixels( target.get() ) == views[1],
                 "stale shared transform dependency must preserve output" );
        wp_renderer_dx11_set_render_texture( native, nullptr );
        std::puts(
            "Native transform/cull-to-RTT two-camera pixel parity and stale dependency retention "
            "passed." );
        return true;
    }
}  // namespace claw_scene_job_contracts
