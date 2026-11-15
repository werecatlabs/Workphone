#pragma once

#include <WPGraphics/ClawHammerSystem.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPGraphics/ClawCamera.hpp>
#include <WPGraphics/ClawSky.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <Workphone/Graphics/GraphicsPipeline.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Core/Properties.hpp>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <WorkphoneGraphics/workphone_graphics_renderer.h>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
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

    class SkyFixtureCamera final : public workphone::render::ClawCamera
    {
    public:
        workphone::Matrix4<workphone::real_Num> getViewMatrix() const override
        {
            return workphone::Matrix4<workphone::real_Num>::identity();
        }

        workphone::Matrix4<workphone::real_Num> getProjectionMatrix() const override
        {
            return workphone::Matrix4<workphone::real_Num>::identity();
        }
    };

    class SkyFixtureTexture final : public workphone::render::ClawTexture
    {
    public:
        ~SkyFixtureTexture() override
        {
            if( view ) wp_renderer_dx11_destroy_texture_native( view );
        }

        void getTextureFinal( void **result ) const override { *result = view; }
        void *view = nullptr;
    };

    inline void runSkyFallback( workphone::SmartPtr<workphone::render::ClawRendererDX11> &renderer,
                                Graphics &graphics,
                                workphone::render::GraphicsPipeline &pipeline,
                                workphone::SmartPtr<workphone::Properties> legacy,
                                workphone::SmartPtr<workphone::Properties> hdr,
                                const wp_render_texture_dx11 *output )
    {
        using namespace workphone;
        using namespace workphone::render;
        auto *native = wp_renderer_get_dx11( renderer->getNativeRenderer() );
        const auto camera = renderer->getCamera();
        renderer->setCamera( make_ptr<SkyFixtureCamera>() );
        auto byte = make_ptr<SkyFixtureTexture>();
        auto radiance = make_ptr<SkyFixtureTexture>();
        constexpr std::array<unsigned char, 4> byteColour{ 56, 143, 201, 255 };
        constexpr std::array<float, 4> hdrColour{ .22f, 2.75f, .55f, 1.f };
        byte->view = wp_renderer_dx11_create_texture_native(
            native, byteColour.data(), 1, 1, WORKPHONE_PIXEL_FORMAT_RGBA8 );
        radiance->view = wp_renderer_dx11_create_texture_rgba32f_native(
            native, hdrColour.data(), 1, 1, hdrColour.size() );
        require( byte->view && radiance->view, "legacy and HDR sky fixtures must upload" );

        Array<SmartPtr<ITexture>> byteFaces( 6, byte );
        Array<SmartPtr<ITexture>> radianceFaces( 6, radiance );
        auto ordinary = make_ptr<ClawSky>();
        ordinary->setTextures( byteFaces );
        ordinary->setDistance( 1 );
        ordinary->setVisible( true );
        auto paired = make_ptr<ClawSky>();
        paired->setTextures( radianceFaces );
        paired->setDistance( 1 );
        paired->setVisible( true );
        auto material = make_ptr<Material>();
        material->setName( "legacy-output-test/sky-display-fallback" );
        material->setCubicTextures( byteFaces );
        paired->setMaterial( material );

        const auto drawDisplay = [&]( const SmartPtr<ClawSky> &sky, bool resolved = false ) {
            require( graphics.beginGraphicsPipelineFrame( renderer, false ) == resolved &&
                     !wp_renderer_dx11_is_radiance_scene_active( native ), "sky requires a display-space pass" );
            renderer->clear( ColourF::Black );
            renderer->renderSky( sky );
            if( resolved ) graphics.endGraphicsPipelineFrame( renderer );
            std::vector<unsigned char> pixels( 32 * 32 * 4 );
            require( wp_renderer_dx11_copy_render_texture_rgba8( output, pixels.data(), pixels.size() ),
                     "legacy sky pixels must be readable" );
            return pixels;
        };
        pipeline.setProperties( legacy );
        const auto reference = drawDisplay( ordinary );
        constexpr size_t centre = ( 16 * 32 + 16 ) * 4;
        require( std::equal( byteColour.begin(), byteColour.end(), reference.begin() + centre ),
                 "ordinary sky must render its byte face at the viewport centre" );
        require( drawDisplay( paired ) == reference, "marked legacy fallback must match the ordinary byte sky" );
        material->setName( "legacy-output-test/ordinary-sky" );
        require( drawDisplay( paired ) != reference, "ordinary material must not replace primary sky faces" );
        material->setName( "legacy-output-test/sky-display-fallback" );

        auto scale = make_ptr<Properties>();
        scale->setProperty( "graphics.renderScale", .5f );
        pipeline.setProperties( scale );
        const auto scaled = drawDisplay( ordinary, true );
        require( scaled == reference && drawDisplay( paired, true ) == scaled,
                 "scale-only display resolve must retain the byte sky" );

        const auto drawRadiance = [&]( const SmartPtr<Properties> &profile ) {
            pipeline.setProperties( legacy );
            pipeline.setProperties( profile );
            require( graphics.beginGraphicsPipelineFrame( renderer, false ) &&
                     wp_renderer_dx11_is_radiance_scene_active( native ), "HDR sky requires a radiance scene target" );
            renderer->clear( ColourF::Black );
            renderer->renderSky( paired );
            wp_texture_face_readback_dx11 data{};
            require( wp_dx11_read_texture_face_native( wp_renderer_dx11_get_device( native ),
                wp_renderer_dx11_get_context( native ), wp_renderer_dx11_get_hdr_scene_view( native ), &data ),
                "HDR sky pixels must be readable" );
            u16 green = 0;
            if( data.format == WP_DX11_FACE_RGBA16F && data.width == 32 && data.height == 32 )
            {
                const auto *row = static_cast<const unsigned char *>( data.pixels ) + 16 * data.row_pitch;
                std::memcpy( &green, row + ( 16 * 4 + 1 ) * sizeof( green ), sizeof( green ) );
            }
            wp_dx11_release_texture_face_readback_native( &data );
            graphics.endGraphicsPipelineFrame( renderer );
            return green;
        };
        require( drawRadiance( hdr ) == 0x4180, "sky HDR draw must retain the 2.75 linear sample before tone mapping" );

        auto gammaHdr = make_ptr<Properties>();
        gammaHdr->setProperty( "graphics.hdr", true );
        gammaHdr->setProperty( "graphics.colorSpace", 0 );
        gammaHdr->setProperty( "graphics.tonemapping", 0 );
        const auto encoded = drawRadiance( gammaHdr );
        require( encoded > 0x3c00 && encoded < 0x4180, "gamma-space HDR sky must encode and retain values above one" );

        pipeline.setProperties( legacy );
        require( drawDisplay( paired ) == reference, "returning to legacy must restore the same sky fallback" );
        renderer->setCamera( camera );
    }

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
        const ColourF clearColour( .25f, .5f, .75f, .5f );
        renderer->clear( clearColour );
        std::vector<unsigned char> legacy( 32 * 32 * 4 );
        auto *output = target->getNativeRenderTexture( renderer->getNativeRenderer() );
        require( wp_renderer_dx11_copy_render_texture_rgba8( output, legacy.data(), legacy.size() ) &&
                     std::abs( int( legacy[0] ) - 64 ) <= 1 &&
                     std::abs( int( legacy[1] ) - 128 ) <= 1 &&
                     std::abs( int( legacy[2] ) - 191 ) <= 1 && legacy[3] == 128,
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

        auto sampling = make_ptr<Properties>();
        sampling->setProperty( "graphics.renderScale", .5f );
        pipeline->setProperties( sampling );
        require( graphics.beginGraphicsPipelineFrame( renderer, false ),
                 "render scale must activate the scene viewport resolve" );
        renderer->clear( clearColour );
        graphics.endGraphicsPipelineFrame( renderer );
        std::vector<unsigned char> scaled( legacy.size() );
        require( wp_renderer_dx11_copy_render_texture_rgba8( output, scaled.data(), scaled.size() ) &&
                     scaled == legacy,
                 "render scale alone must preserve legacy viewport colour and alpha" );

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
        renderer->clear( clearColour );
        graphics.endGraphicsPipelineFrame( renderer );
        std::vector<unsigned char> selectedPixels( legacy.size() );
        require( !wp_renderer_dx11_is_hdr_scene_active( native ) &&
                     wp_renderer_dx11_copy_render_texture_rgba8(
                         output, selectedPixels.data(), selectedPixels.size() ) &&
                     selectedPixels[0] > legacy[0] + 20 && selectedPixels[3] == legacy[3],
                 "explicit linear output policy must reach scene texture pixels" );

        // Returning to a legacy project must also discard the previous project's
        // GPU light grid. Compare actual lit geometry, not just profile values.
        auto replacement = make_ptr<GraphicsPipeline>()->getProperties();
        replacement->setProperty( "Replace project settings", true );
        runSkyFallback( renderer, graphics, *pipeline, replacement, selected, output );
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
