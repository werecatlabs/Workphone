#pragma once

#include <WPGraphics/ClawHammerSystem.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPGraphics/ClawCamera.hpp>
#include <Workphone/Graphics/GraphicsPipeline.hpp>
#include <Workphone/Core/Properties.hpp>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <WorkphoneGraphics/workphone_graphics_renderer.h>
#include <stdexcept>
#include <vector>
#include <cmath>
#include <memory>

namespace claw_legacy_output_contracts
{
    inline void require( bool value, const char *message )
    {
        if( !value ) throw std::runtime_error( message );
    }

    class Graphics : public workphone::render::ClawHammerSystem
    {
    public:
        using ClawHammerSystem::beginGraphicsPipelineFrame;
        using ClawHammerSystem::endGraphicsPipelineFrame;
        void setPipeline( workphone::SmartPtr<workphone::render::IGraphicsPipeline> pipeline )
        {
            m_graphicsPipeline = pipeline;
        }
    };

    inline bool run( workphone::SmartPtr<workphone::render::IGraphicsWindow> window )
    {
        using namespace workphone;
        using namespace workphone::render;
        auto renderer = make_ptr<ClawRendererDX11>();
        renderer->load( window );
        require( renderer->isLoaded(), "legacy viewport output renderer must load" );
        auto pipeline = make_ptr<GraphicsPipeline>();
        Graphics graphics;
        graphics.setDefaultWindow( window );
        graphics.setPipeline( pipeline );
        auto camera = make_ptr<ClawCamera>();
        auto target = make_ptr<ClawRenderTarget>();
        target->setSize( { 32, 32 } );
        renderer->setCamera( camera );
        renderer->setRenderTarget( target );
        renderer->beginRender();
        auto *native = wp_renderer_get_dx11( renderer->getNativeRenderer() );
        require( !graphics.beginGraphicsPipelineFrame( renderer, false ) &&
                     !wp_renderer_dx11_is_hdr_scene_active( native ),
                 "legacy scene viewport must preserve direct display-space rendering" );
        renderer->clear( ColourF( .25f, .5f, .75f, 1.f ) );
        std::vector<unsigned char> legacy( 32 * 32 * 4 );
        auto *output = target->getNativeRenderTexture( renderer->getNativeRenderer() );
        require( wp_renderer_dx11_copy_render_texture_rgba8( output, legacy.data(), legacy.size() ) &&
                     std::abs( int( legacy[0] ) - 64 ) <= 1 &&
                     std::abs( int( legacy[1] ) - 128 ) <= 1 &&
                     std::abs( int( legacy[2] ) - 191 ) <= 1 && legacy[3] == 255,
                 "legacy scene viewport must retain its original colour values" );

        auto hdr = pipeline->getHdrSettings();
        hdr.m_enableAgxToneMapping = false;
        pipeline->setHdrSettings( hdr );
        renderer->setRenderTarget( window );
        require( !graphics.beginGraphicsPipelineFrame( renderer, false ),
                 "legacy default window must honour its disabled tone mapping gate" );

        // Selecting only the backend is not an implicit output-colour change.
        auto backendOnly = make_ptr<Properties>();
        backendOnly->setProperty( "graphics.renderer.pc", 1 );
        pipeline->setProperties( backendOnly );
        renderer->setRenderTarget( target );
        require( !graphics.beginGraphicsPipelineFrame( renderer, false ),
                 "backend selection alone must preserve legacy viewport colours" );

        // An explicit project output policy must still run on scene textures,
        // even if the older friendly-name pipeline has tone mapping disabled.
        auto selected = make_ptr<Properties>();
        selected->setProperty( "graphics.hdr", true );
        selected->setProperty( "graphics.colorSpace", 1 );
        selected->setProperty( "graphics.tonemapping", 0 );
        pipeline->setProperties( selected );
        require( graphics.beginGraphicsPipelineFrame( renderer, false ) &&
                     wp_renderer_dx11_is_hdr_scene_active( native ),
                 "explicit project output policy must activate the scene viewport resolve" );
        renderer->clear( ColourF( .25f, .5f, .75f, 1.f ) );
        graphics.endGraphicsPipelineFrame( renderer );
        std::vector<unsigned char> selectedPixels( legacy.size() );
        require( !wp_renderer_dx11_is_hdr_scene_active( native ) &&
                     wp_renderer_dx11_copy_render_texture_rgba8(
                         output, selectedPixels.data(), selectedPixels.size() ) &&
                     selectedPixels[0] > legacy[0] + 20 && selectedPixels[3] == 255,
                 "explicit linear output policy must reach scene texture pixels" );

        // Returning to a legacy project must also discard the previous project's
        // GPU light grid. Compare actual lit geometry, not just profile values.
        auto replacement = make_ptr<GraphicsPipeline>()->getProperties();
        replacement->setProperty( "Replace project settings", true );
        pipeline->setProperties( replacement );
        require( !graphics.beginGraphicsPipelineFrame( renderer, false ),
                 "replacement legacy profile must bypass scene resolve" );
        wp_mat4f identity{};
        for( unsigned axis = 0; axis < 4; ++axis ) identity.m[axis][axis] = 1;
        wp_renderer_dx11_set_world_matrix( native, &identity );
        wp_renderer_dx11_set_view_matrix( native, &identity );
        wp_renderer_dx11_set_projection_matrix( native, &identity );
        wp_renderer_dx11_set_cull_mode( native, WORKPHONE_CULL_MODE_NONE );
        wp_renderer_dx11_set_blend_mode( native, WORKPHONE_BLEND_MODE_NONE );
        wp_material_dx11 material{};
        material.base_color = { 1, 1, 1, 1 };
        material.surface = { 0, .5f, 1, 1 };
        material.uv_transform.z = 1;
        material.camera_position = { 0, 0, 3, 0 };
        material.ambient_color = { .025f, .025f, .025f, 1 };
        wp_renderer_dx11_set_material( native, &material );
        const wp_vertex_pntc vertices[] = {
            { { -1, -1, 0 }, { 0, 0, 1 }, { 0, 0 }, 0xffffffff },
            { { -1, 1, 0 }, { 0, 0, 1 }, { 0, 0 }, 0xffffffff },
            { { 1, -1, 0 }, { 0, 0, 1 }, { 0, 0 }, 0xffffffff }
        };
        const unsigned short indices[] = { 0, 1, 2 };
        std::unique_ptr<wp_geometry_dx11, decltype( &wp_renderer_dx11_destroy_geometry )> geometry(
            wp_renderer_dx11_create_indexed_geometry_pntc( native, vertices, 3, indices, 3, 0 ),
            &wp_renderer_dx11_destroy_geometry );
        require( geometry != nullptr, "legacy lighting geometry must allocate" );
        const auto drawPixels = [&] {
            renderer->clear( ColourF::Black );
            wp_renderer_dx11_draw_geometry_pntc( native, geometry.get(), 0, 3, 0 );
            std::vector<unsigned char> pixels( legacy.size() );
            require( wp_renderer_dx11_copy_render_texture_rgba8( output, pixels.data(), pixels.size() ),
                     "legacy lighting pixels must be readable" );
            return pixels;
        };
        const auto unlitGrid = drawPixels();
        const float lights[] = { 0, 0, -1, 4, 0, 0, 8, 0, 0, 0, 1, 4, 0, 0, 8, 0 };
        const unsigned cells[] = { 0, 2 }, list[] = { 0, 1 };
        const float forward[] = { 0, 0, -1 };
        require( wp_renderer_dx11_set_local_lights(
                     native, lights, 2, cells, list, 2, 1, 1, 1, .1f, 100, forward ),
                 "previous project light grid must upload" );
        wp_renderer_dx11_set_pipeline_mode( native, 3, 1 );
        require( drawPixels() != unlitGrid, "previous project lights must visibly affect the fixture" );
        require( !graphics.beginGraphicsPipelineFrame( renderer, false ) && drawPixels() == unlitGrid,
                 "legacy project pixels must discard the previous project's GPU lights and pipeline" );
        geometry.reset();
        renderer->setRenderTarget( nullptr );
        renderer->endRender();
        renderer->setCamera( nullptr );
        renderer->unload( nullptr );
        return true;
    }
}
