#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/ClawCamera.hpp>
#include <WPGraphics/ClawDepthReadback.hpp>
#if defined WP_TEST_IMGUI
#include <WPGraphics/ClawImguiManager.hpp>
#include <WPImGui/ImGuiManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/IO/FileSystem.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <imgui.h>
#endif
#include <Workphone/Graphics/GraphicsWindow.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>
#include <WorkphoneGraphics/workphone_graphics_renderer.h>
#include <WorkphoneGraphics/workphone_graphics_object.h>
#include <WorkphoneGraphics/workphone_graphics_mesh.h>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include "DX11TestEvidence.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <vector>


#include "ClawRTTJobContracts.hpp"
#include "ClawUIRTTContracts.hpp"
#include "ClawSceneJobContracts.hpp"

namespace
{
    class TestWindow : public workphone::render::GraphicsWindow
    {
    public:
        HWND handle = nullptr;
        workphone::Vector2I size{ 1280, 720 };
        workphone::Vector2I getSize() const override { return size; }
        void getWindowHandle( void *data ) override { *static_cast<HWND *>( data ) = handle; }
        void _getObject( void **data ) const override { *data = nullptr; }
    };

    bool check( bool value, const char *message )
    {
        if( !value )
            std::fprintf( stderr, "FAIL: %s\n", message );
        return value;
    }

    bool sameStatisticsCounters( const wp_render_statistics_dx11 &actual,
                                 const wp_render_statistics_dx11 &expected )
    {
        bool ok = true;
        // Compare public values, not padding bytes or the intentionally omitted
        // history percentile. Both getters observe one unchanged native frame.
#define WP_CHECK_COUNTER( field ) \
        ok &= check( actual.field == expected.field, "numeric statistics must preserve " #field )
        WP_CHECK_COUNTER( frames );
        WP_CHECK_COUNTER( interval_samples );
        WP_CHECK_COUNTER( gpu_samples );
        WP_CHECK_COUNTER( draws );
        WP_CHECK_COUNTER( triangles );
        WP_CHECK_COUNTER( material_uploads );
        WP_CHECK_COUNTER( transform_uploads );
        WP_CHECK_COUNTER( geometry_creations );
        WP_CHECK_COUNTER( state_bindings );
        WP_CHECK_COUNTER( instanced_draws );
        WP_CHECK_COUNTER( instances );
        WP_CHECK_COUNTER( instance_uploads );
        WP_CHECK_COUNTER( instance_upload_bytes );
        WP_CHECK_COUNTER( instance_buffer_creations );
        WP_CHECK_COUNTER( interval_ms );
        WP_CHECK_COUNTER( cpu_frame_ms );
        WP_CHECK_COUNTER( present_ms );
        WP_CHECK_COUNTER( gpu_frame_ms );
        WP_CHECK_COUNTER( flip_model );
        WP_CHECK_COUNTER( present_attempts );
        WP_CHECK_COUNTER( present_occluded );
        WP_CHECK_COUNTER( present_failures );
        WP_CHECK_COUNTER( last_present_hresult );
#undef WP_CHECK_COUNTER
        return ok;
    }

    bool testOcclusionDepthReadback( workphone::render::ClawRendererDX11 &wrapper )
    {
        auto native = wp_renderer_get_dx11( wrapper.getNativeRenderer() );
        auto context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( native ) );
        workphone::render::ClawDepthReadback reader;
        bool ok = true;
        const auto supported = wp_renderer_dx11_supported_scene_samples( native );
        for( int samples : { 1, 4 } )
        {
            if( !( supported & static_cast<unsigned>( samples ) ) ) continue;
            wp_renderer_dx11_set_scene_sampling( native, samples, .5f );
            ok &= check( wp_renderer_dx11_begin_hdr_scene( native, 1.f ),
                         "occlusion depth fixture must begin a native scene" );
            ID3D11RenderTargetView *beforeTarget = nullptr;
            ID3D11DepthStencilView *beforeDepth = nullptr;
            context->OMGetRenderTargets( 1, &beforeTarget, &beforeDepth );
            ok &= check( beforeDepth != nullptr, "native scene must bind sampleable depth" );
            if( beforeDepth ) context->ClearDepthStencilView( beforeDepth, D3D11_CLEAR_DEPTH, .375f, 0 );
            std::vector<float> depth;
            unsigned width = 0, height = 0;
            ok &= check( reader.read( wrapper.getNativeRenderer(), depth, width, height ),
                         "single and multisample scene depth must execute GPU resolve and staging readback" );
            ok &= check( width > 0 && width <= 256 && height > 0 && height <= 256 &&
                         depth.size() == size_t( width ) * height &&
                         std::all_of( depth.begin(), depth.end(), []( float z ) { return std::abs( z - .375f ) < .00001f; } ),
                         "bounded depth snapshots must retain conventional Z values" );
            ID3D11RenderTargetView *afterTarget = nullptr;
            ID3D11DepthStencilView *afterDepth = nullptr;
            context->OMGetRenderTargets( 1, &afterTarget, &afterDepth );
            ok &= check( beforeTarget == afterTarget && beforeDepth == afterDepth,
                         "occlusion readback must restore renderer output bindings" );
            if( beforeTarget ) beforeTarget->Release(); if( beforeDepth ) beforeDepth->Release();
            if( afterTarget ) afterTarget->Release(); if( afterDepth ) afterDepth->Release();
            wp_renderer_dx11_end_hdr_scene( native );
            std::printf( "Occlusion GPU depth: %d samples, %ux%u snapshot\n", samples, width, height );
        }
        wp_renderer_dx11_set_scene_sampling( native, 1, 1.f );
        return ok;
    }

    bool testStatisticsReadback( wp_renderer_dx11 *renderer )
    {
        wp_render_statistics_dx11 empty{}, missing;
        std::memset( &missing, 0xA5, sizeof( missing ) );
        wp_renderer_dx11_get_statistics_counters( nullptr, &missing );
        bool ok = sameStatisticsCounters( missing, empty );
        ok &= check( missing.interval_p95_ms == 0,
                     "missing numeric statistics must clear the percentile" );
        wp_renderer_dx11_get_statistics_counters( renderer, nullptr );
        wp_renderer_dx11_get_statistics( renderer, nullptr );

        wp_render_statistics_dx11 full{}, counters, unchanged{};
        wp_renderer_dx11_get_statistics( renderer, &full );
        for( int read = 0; read < 64; ++read )
        {
            std::memset( &counters, 0xA5, sizeof( counters ) );
            wp_renderer_dx11_get_statistics_counters( renderer, &counters );
            ok &= sameStatisticsCounters( counters, full );
            ok &= check( counters.interval_p95_ms == 0,
                         "numeric readback must leave p95 unavailable even with retained intervals" );
        }
        wp_renderer_dx11_get_statistics( renderer, &unchanged );
        ok &= sameStatisticsCounters( unchanged, full );
        ok &= check( unchanged.interval_p95_ms == full.interval_p95_ms,
                     "repeated numeric readback must preserve percentile history" );
        if( full.interval_samples > 0 )
            ok &= check( full.interval_p95_ms > 0,
                         "the full getter must retain a positive percentile for completed intervals" );
        return ok;
    }

    ID3D11DepthStencilView *depthTarget( ID3D11DeviceContext *context )
    {
        ID3D11DepthStencilView *view = nullptr;
        context->OMGetRenderTargets( 0, nullptr, &view );
        return view;
    }

    #if defined WP_TEST_IMGUI
    bool testImgui( workphone::render::ClawRendererDX11 &wrapper )
    {
        workphone::render::ClawImguiManager manager;
        manager.setRenderer( wrapper.getNativeRenderer() );
        manager.load( nullptr );
        bool ok = check( manager.isInitialised(), "ImGui must initialise without application services" );
        if( !ok ) return false;
        auto &io = ImGui::GetIO();
        io.IniFilename = nullptr;
        ok &= check( io.Fonts->TexID != nullptr, "font atlas must upload" );
        {
            auto app = workphone::make_ptr<workphone::core::ApplicationManager>();
            workphone::core::IApplicationManager::setInstance( app );
            auto factories = workphone::make_ptr<workphone::FactoryManager>();
            factories->load( nullptr );
            app->setFactoryManager( factories );
            app->setFileSystem( workphone::make_ptr<workphone::FileSystem>() );
            workphone::ui::ImGuiManager ui;
            ok &= check( !ui.loadFont( "missing-icon-font.ttf", "awesome" ), "missing font must fail safely" );
            ok &= check( ui.loadFont( WP_TEST_ICON_FONT, "awesome" ), "bundled icon font must load" );
            const int configs = io.Fonts->ConfigData.Size;
            ok &= check( ui.loadFont( WP_TEST_ICON_FONT, "awesome" ) &&
                         io.Fonts->ConfigData.Size == configs, "repeated icon load must not duplicate merges" );
            const ImFontGlyph *icon = io.Fonts->Fonts[0]->FindGlyphNoFallback( 0xf007 );
            ImFontAtlas reference;
            static const ImWchar range[] = { 0xf007, 0xf007, 0 };
            auto *referenceFont = reference.AddFontFromFileTTF( WP_TEST_ICON_FONT, 12.0f, nullptr, range );
            reference.Build();
            const auto *oldIcon = referenceFont->FindGlyphNoFallback( 0xf007 );
            ok &= check( icon && oldIcon && icon->Y1 - icon->Y0 > oldIcon->Y1 - oldIcon->Y0,
                         "merged icon outline must be larger than the previous 12px glyph" );
            ui.unloadFont();
            io.Fonts->ClearTexData();
            ok &= check( io.Fonts->Build(), "atlas must rebuild after UI font unload" );
            app->setFileSystem( nullptr );
            app->setFactoryManager( nullptr );
            factories->unload( nullptr );
            workphone::core::IApplicationManager::setInstance( nullptr );
        }
        const int sources = io.Fonts->ConfigData.Size;
        for( int i = 0; i < 3; ++i ) manager.createFontTexture();
        ok &= check( io.Fonts->ConfigData.Size == sources, "texture rebuild must not add font sources" );
        manager.newFrame();
        const int frame = ImGui::GetFrameCount();
        manager.newFrame();
        ok &= check( ImGui::GetFrameCount() == frame, "duplicate newFrame must be harmless" );
        auto texture = io.Fonts->TexID;
        manager.createFontTexture();
        ok &= check( texture == io.Fonts->TexID, "locked atlas must not be replaced" );
        int callbacks = 0;
        auto *draw = ImGui::GetForegroundDrawList();
        draw->AddCallback( []( const ImDrawList *, const ImDrawCmd *cmd ) {
            ++*static_cast<int *>( cmd->UserCallbackData );
        }, &callbacks );
        draw->AddCallback( ImDrawCallback_ResetRenderState, nullptr );
        draw->PushClipRect( ImVec2( -1e30f, -1e30f ), ImVec2( 1e30f, 1e30f ) );
        draw->AddRectFilled( ImVec2( 5, 5 ), ImVec2( 20, 20 ), IM_COL32_WHITE );
        draw->PopClipRect();
        manager.render();
        manager.render();
        ok &= check( callbacks == 1, "zero-element callback must run exactly once" );
        manager.shutdown();
        manager.shutdown();
        ok &= check( !manager.isInitialised(), "shutdown must be idempotent" );
        return ok;
    }

    #endif

    bool testShadows( workphone::render::ClawRendererDX11 &wrapper )
    {
        auto *renderer = wp_renderer_get_dx11( wrapper.getNativeRenderer() );
        auto *device = static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( renderer ) );
        auto *context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( renderer ) );
        auto *target = wp_renderer_dx11_create_render_texture( renderer, 64, 64 );
        if( !check( target != nullptr, "shadow receiver target must be created" ) ) return false;
        const wp_vertex_pntc vertices[] = {
            { {-1,-1,0}, {0,0,1}, {0,0}, 0xFFFFFFFFu },
            { { 1,-1,0}, {0,0,1}, {1,0}, 0xFFFFFFFFu },
            { { 1, 1,0}, {0,0,1}, {1,1}, 0xFFFFFFFFu },
            { {-1, 1,0}, {0,0,1}, {0,1}, 0xFFFFFFFFu }
        };
        const wp_u16 indices[] = {0,1,2,0,2,3};
        auto *nativeMesh = wp_graphics_mesh_create();
        const bool prepared = nativeMesh && wp_graphics_mesh_set_vertices( nativeMesh,
            WORKPHONE_VERTEX_FORMAT_PNTC, vertices, 4 ) &&
            wp_graphics_mesh_set_indices_u16( nativeMesh, indices, 6 );
        auto *geometry = prepared
            ? wp_renderer_dx11_prepare_mesh_geometry( renderer, nativeMesh, 0, 6 ) : nullptr;
        bool ok = check( geometry != nullptr, "native cached shadow geometry must be created" );
        wp_mat4f identity{};
        for( int i = 0; i < 4; ++i ) identity.m[i][i] = 1;
        wp_renderer_dx11_set_render_texture( renderer, target );
        wp_renderer_dx11_set_viewport( renderer, {0,0,64,64} );
        wp_renderer_dx11_set_view_matrix( renderer, &identity );
        wp_renderer_dx11_set_projection_matrix( renderer, &identity );
        wp_renderer_dx11_set_cull_mode( renderer, WORKPHONE_CULL_MODE_NONE );
        wp_renderer_dx11_set_depth_test_enabled( renderer, 1 );
        wp_renderer_dx11_set_depth_write_enabled( renderer, 1 );
        wp_renderer_dx11_set_depth_func( renderer, WORKPHONE_DEPTH_FUNC_LESS );
        wp_renderer_dx11_set_blend_mode( renderer, WORKPHONE_BLEND_MODE_NONE );
        wp_material_dx11 material{};
        material.base_color = {1,1,1,1};
        material.specular_color = {0.04f,0.04f,0.04f,1};
        material.light_color = {1,1,1,3};
        material.light_direction = {0,0,-1,0};
        material.camera_position = {0,0,2,0.02f};
        material.surface = {0,0.8f,1,1};
        material.uv_transform.z = 1;
        material.controls.z = -1;
        wp_renderer_dx11_set_material_textures( renderer, nullptr );
        wp_renderer_dx11_set_texture_native( renderer, nullptr );
        auto *originalDepth = depthTarget( context );
        ID3D11Texture2D *readback = nullptr;
        auto *resource = static_cast<ID3D11Texture2D *>( wp_renderer_dx11_get_render_texture_resource( target ) );
        D3D11_TEXTURE2D_DESC desc{};
        resource->GetDesc( &desc );
        desc.Usage = D3D11_USAGE_STAGING; desc.BindFlags = 0; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ok &= check( SUCCEEDED( device->CreateTexture2D( &desc, nullptr, &readback ) ), "shadow readback must be created" );
        // Repeated passes also cover resize, stale SRV unbinding and clearing previous casters.
        for( int pass = 0; geometry && readback && pass < 4; ++pass )
        {
            wp_renderer_dx11_set_scissor_enabled( renderer, 1 );
            wp_renderer_dx11_set_scissor_rect( renderer, {0,0,64,64} );
            const bool begun = wp_renderer_dx11_begin_shadow_map( renderer, &identity, pass == 0 ? 128 : 256 ) != 0;
            ok &= check( begun, "shadow depth pass must begin" );
            if( !begun ) break;
            ok &= check( !wp_renderer_dx11_begin_shadow_map( renderer, &identity, 128 ), "nested shadow passes must be rejected" );
            auto caster = identity;
            caster.m[0][0] = 0.25f; caster.m[0][3] = -0.5f; caster.m[2][3] = -0.5f;
            wp_renderer_dx11_set_world_matrix( renderer, &caster );
            material.base_color.w = pass == 2 ? 0.0f : 1.0f;
            material.controls.z = 0.5f;
            wp_renderer_dx11_set_material( renderer, &material );
            if( pass != 3 ) wp_renderer_dx11_draw_geometry_pntc( renderer, geometry, 0, 6, 0 );
            wp_renderer_dx11_end_shadow_map( renderer );
            auto *restoredDepth = depthTarget( context );
            ok &= check( restoredDepth == originalDepth, "shadow pass must restore receiver depth target" );
            if( restoredDepth ) restoredDepth->Release();
            const auto viewport = wp_renderer_dx11_get_viewport( renderer );
            ok &= check( viewport.width == 64 && viewport.height == 64, "shadow pass must restore viewport" );
            ID3D11RasterizerState *raster = nullptr;
            // Draw flushes the restored scissor state through the native state cache.
            auto receiver = identity; receiver.m[2][3] = 0.5f;
            wp_renderer_dx11_set_world_matrix( renderer, &receiver );
            material.base_color.w = 1; material.controls.z = -1;
            wp_renderer_dx11_set_material( renderer, &material );
            wp_renderer_dx11_enable_shadow_receiving( renderer, pass != 1 );
            wp_renderer_dx11_clear( renderer, WORKPHONE_CLEAR_FLAG_ALL );
            wp_renderer_dx11_draw_geometry_pntc( renderer, geometry, 0, 6, 0 );
            context->RSGetState( &raster );
            if( raster ) { D3D11_RASTERIZER_DESC rd{}; raster->GetDesc( &rd );
                ok &= check( rd.ScissorEnable, "shadow pass must restore scissor enable" ); raster->Release(); }
            context->CopyResource( readback, resource );
            D3D11_MAPPED_SUBRESOURCE mapped{};
            const bool mappedOK = SUCCEEDED( context->Map( readback, 0, D3D11_MAP_READ, 0, &mapped ) );
            ok &= check( mappedOK, "shadow pixels must be readable" );
            if( mappedOK )
            {
                const auto *pixels = static_cast<const unsigned char *>( mapped.pData ) + 32 * mapped.RowPitch;
                const int left = pixels[16 * 4 + 1], right = pixels[48 * 4 + 1];
                std::printf( "Shadow pass %d: occluded=%d lit=%d\n", pass, left, right );
                ok &= check( right > 100, "unoccluded receiver must stay lit" );
                ok &= check( pass == 0 ? left + 50 < right : std::abs( left - right ) < 10,
                    "shadows must darken receivers; disabling, cutouts and empty maps must stay lit" );
                context->Unmap( readback, 0 );
            }
        }
        // Exercise camera fitting with a diagonal light, where a component-wise
        // matrix/vector multiply would fail to enclose the receiver frustum.
        auto camera = workphone::make_ptr<workphone::render::ClawCamera>();
        camera->setNearClipDistance( 0.1f ); camera->setFarClipDistance( 100.0f ); camera->setAspectRatio( 1.0f );
        wrapper.setCamera( camera );
        wrapper.setSceneLighting( workphone::ColourF::White, workphone::Vector3F( 0.3f,-0.7f,0.5f ), workphone::ColourF::White, 3 );
        const bool fitted = wrapper.beginShadowMap( 512 );
        ok &= check( fitted, "C++ renderer must fit a directional shadow map" );
        if( fitted && geometry )
        {
            wp_renderer_dx11_draw_geometry_pntc( renderer, geometry, 0, 6, 0 );
            ID3D11Buffer *constants = nullptr, *staging = nullptr;
            context->PSGetConstantBuffers( 2, 1, &constants );
            if( constants )
            {
                D3D11_BUFFER_DESC bd{}; constants->GetDesc( &bd );
                bd.Usage = D3D11_USAGE_STAGING; bd.BindFlags = 0; bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                if( SUCCEEDED( device->CreateBuffer( &bd, nullptr, &staging ) ) )
                {
                    context->CopyResource( staging, constants );
                    D3D11_MAPPED_SUBRESOURCE mapped{};
                    if( SUCCEEDED( context->Map( staging, 0, D3D11_MAP_READ, 0, &mapped ) ) )
                    {
                        const auto &light = *static_cast<const wp_mat4f *>( mapped.pData );
                        const auto inverse = ( camera->getProjectionMatrix() * camera->getViewMatrix() ).inverse();
                        for( int z : {-1,1} ) for( int y : {-1,1} ) for( int x : {-1,1} )
                        {
                            const float w = inverse[3][0]*x + inverse[3][1]*y + inverse[3][2]*z + inverse[3][3];
                            float point[3];
                            for( int i = 0; i < 3; ++i ) point[i] = ( inverse[i][0]*x + inverse[i][1]*y + inverse[i][2]*z + inverse[i][3] ) / w;
                            for( int i = 0; i < 3; ++i )
                            {
                                const float clip = light.m[i][0]*point[0] + light.m[i][1]*point[1] + light.m[i][2]*point[2] + light.m[i][3];
                                ok &= check( std::isfinite( clip ) && std::abs( clip ) <= 1.001f, "diagonal light map must contain the camera frustum" );
                            }
                        }
                        context->Unmap( staging, 0 );
                    }
                    else ok &= check( false, "shadow camera constants must map" );
                    staging->Release();
                }
                else ok &= check( false, "shadow camera staging buffer must allocate" );
                constants->Release();
            }
            else ok &= check( false, "shadow camera constants must bind" );
            wrapper.endShadowMap();
        }
        else if( fitted ) wrapper.endShadowMap();
        wrapper.disableShadows(); wrapper.setCamera( nullptr );
        if( originalDepth ) originalDepth->Release();
        if( readback ) readback->Release();
        wp_renderer_dx11_forget_mesh( nativeMesh );
        wp_graphics_mesh_destroy( nativeMesh );
        wp_renderer_dx11_enable_shadow_receiving( renderer, 0 );
        wp_renderer_dx11_set_scissor_enabled( renderer, 0 );
        wp_renderer_dx11_set_render_texture( renderer, nullptr );
        wp_renderer_dx11_destroy_render_texture( target );
        return ok;
    }

    bool testMipsAndFog( wp_renderer_dx11 *renderer )
    {
        using namespace workphone::render;
        const std::uint8_t checker[] = { 0,   0,   0,   255, 255, 255, 255, 255,
                                         255, 255, 255, 255, 0,   0,   0,   255 };
        const auto colour = generateTextureMips( checker, 2, 2, { TextureMipFilter::Colour } );
        const auto data = generateTextureMips( checker, 2, 2, { TextureMipFilter::Data } );
        bool ok = check( colour.size() == 2 && colour[1].bgra[0] > 180 && colour[1].bgra[0] < 190,
                         "colour mip averages must be linear-light, not dark encoded averages" );
        ok &= check( data[1].bgra[0] == 128, "data mip averages must remain linear" );
        const auto roughness = generateTextureMips( checker, 2, 2, { TextureMipFilter::Roughness } );
        ok &= check( roughness[1].bgra[0] >= 214 && roughness[1].bgra[0] <= 215,
                     "roughness mips must average the fourth moment used by GGX" );
        std::vector<std::uint8_t> odd( 3 * 3 * 4, 0 );
        odd[8 * 4] = 255;
        const auto oddMips = generateTextureMips( odd.data(), 3, 3, { TextureMipFilter::Data } );
        ok &= check( oddMips.size() == 2 && oddMips[1].bgra[0] == 28,
                     "odd mip dimensions must include the final row and column" );
        const std::uint8_t normals[] = { 255, 128, 128, 255, 128, 128, 255, 255,
                                         255, 128, 128, 255, 128, 128, 255, 255 };
        const auto normal = generateTextureMips( normals, 2, 2, { TextureMipFilter::Normal } );
        float norm = 0;
        for( int c = 0; c < 3; ++c )
        {
            const float v = normal[1].bgra[c] / 255.f * 2 - 1;
            norm += v * v;
        }
        ok &= check( std::abs( norm - 1 ) < .025f, "averaged normals must remain unit length" );
        std::vector<std::uint8_t> atlas( 12 * 4 * 4, 0 );
        for( int y = 0; y < 4; ++y )
            for( int x = 0; x < 12; ++x )
            {
                auto *p = atlas.data() + ( y * 12 + x ) * 4;
                p[x / 4] = 255;
                p[3] = ( x % 4 < 2 ) ? 255 : 0;
            }
        const auto cards =
            generateTextureMips( atlas.data(), 12, 4, { TextureMipFilter::Cutout, 3, .5f } );
        ok &= check( cards.size() == 3 && cards.back().width == 3,
                     "atlas mip chain must stop before views merge" );
        for( int tile = 0; tile < 3; ++tile )
        {
            const auto *p = cards[1].bgra.data() + tile * 2 * 4;
            ok &= check( p[tile] == 255 && p[( tile + 1 ) % 3] == 0 && p[3] == 255 && p[7] == 0,
                         "atlas tiles must keep their colour, alpha coverage and transparent gutters" );
        }
        const std::uint8_t edge[] = { 0, 0, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        const auto border = generateTextureMips( edge, 2, 2, { TextureMipFilter::Colour } );
        ok &= check( border[1].bgra[2] == 255, "transparent black must not darken cutout edge colour" );
        std::vector<wp_texture_mip_dx11> upload;
        for( const auto &level : colour )
            upload.push_back( { level.bgra.data(), level.width, level.height } );
        auto *view =
            static_cast<ID3D11ShaderResourceView *>( wp_renderer_dx11_create_texture_mips_native(
                renderer, upload.data(), wp_u32( upload.size() ) ) );
        ok &= check( view != nullptr, "GPU mip texture must allocate" );
        auto *device = static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( renderer ) );
        auto *context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( renderer ) );
        if( view )
        {
            ID3D11Resource *resource = nullptr;
            ID3D11Texture2D *texture = nullptr, *staging = nullptr;
            view->GetResource( &resource );
            resource->QueryInterface( __uuidof( ID3D11Texture2D ),
                                      reinterpret_cast<void **>( &texture ) );
            if( texture )
            {
                D3D11_TEXTURE2D_DESC desc{};
                texture->GetDesc( &desc );
                ok &= check( desc.MipLevels == 2, "GPU resource must expose the mip chain" );
                desc.Usage = D3D11_USAGE_STAGING;
                desc.BindFlags = 0;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                if( SUCCEEDED( device->CreateTexture2D( &desc, nullptr, &staging ) ) )
                {
                    context->CopyResource( staging, texture );
                    D3D11_MAPPED_SUBRESOURCE mapped{};
                    if( SUCCEEDED( context->Map( staging, 1, D3D11_MAP_READ, 0, &mapped ) ) )
                    {
                        ok &= check(
                            static_cast<const std::uint8_t *>( mapped.pData )[0] == colour[1].bgra[0],
                            "uploaded GPU mip must contain the filtered pixels" );
                        context->Unmap( staging, 1 );
                    }
                    else
                        ok &= check( false, "GPU mip readback must map" );
                    staging->Release();
                }
                else
                    ok &= check( false, "GPU mip staging must allocate" );
                texture->Release();
            }
            else
                ok &= check( false, "GPU mip texture must be queryable" );
            resource->Release();
            view->Release();
        }
        auto invalid = upload;
        invalid[1].width = 2;
        ok &= check( !wp_renderer_dx11_create_texture_mips_native( renderer, invalid.data(),
                                                                   wp_u32( invalid.size() ) ),
                     "invalid GPU mip dimensions must be rejected" );
        const float linearPixels[] = { 2.0f, 0.25f, 0.5f, 1.0f,
                                       0.125f, 4.0f, 0.75f, 0.5f };
        ok &= check( !wp_renderer_dx11_create_texture_rgba32f_native(
                         renderer, linearPixels, 2, 1, 7 ),
                     "RGBA32F upload must reject an incomplete pixel buffer" );
        ok &= check( !wp_renderer_dx11_create_texture_rgba32f_native(
                         renderer, linearPixels, 0, 1, 8 ),
                     "RGBA32F upload must reject invalid dimensions" );
        auto *linearView = static_cast<ID3D11ShaderResourceView *>(
            wp_renderer_dx11_create_texture_rgba32f_native( renderer, linearPixels, 2, 1, 8 ) );
        ok &= check( linearView != nullptr, "native RGBA32F texture must allocate" );
        if( linearView )
        {
            ID3D11Resource *linearResource = nullptr;
            ID3D11Texture2D *linearTexture = nullptr;
            ID3D11Texture2D *staging = nullptr;
            linearView->GetResource( &linearResource );
            linearResource->QueryInterface( __uuidof( ID3D11Texture2D ),
                                            reinterpret_cast<void **>( &linearTexture ) );
            ok &= check( linearTexture != nullptr, "RGBA32F resource must be a texture" );
            if( linearTexture )
            {
                D3D11_TEXTURE2D_DESC desc{};
                linearTexture->GetDesc( &desc );
                ok &= check( desc.Format == DXGI_FORMAT_R32G32B32A32_FLOAT &&
                                 desc.Width == 2 && desc.Height == 1,
                             "native RGBA32F resource must preserve its format and dimensions" );
                desc.Usage = D3D11_USAGE_STAGING;
                desc.BindFlags = 0;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                if( SUCCEEDED( device->CreateTexture2D( &desc, nullptr, &staging ) ) )
                {
                    context->CopyResource( staging, linearTexture );
                    D3D11_MAPPED_SUBRESOURCE mapped{};
                    if( SUCCEEDED( context->Map( staging, 0, D3D11_MAP_READ, 0, &mapped ) ) )
                    {
                        ok &= check( std::memcmp( mapped.pData, linearPixels, sizeof( linearPixels ) ) == 0,
                                     "native RGBA32F upload must preserve HDR pixel values" );
                        context->Unmap( staging, 0 );
                    }
                    else ok &= check( false, "RGBA32F readback must map" );
                    staging->Release();
                }
                else ok &= check( false, "RGBA32F readback must allocate" );
                linearTexture->Release();
            }
            linearResource->Release();
            linearView->Release();
        }
        auto *target = wp_renderer_dx11_create_render_texture( renderer, 16, 16 );
        if( !check( target != nullptr, "fog test target must allocate" ) )
            return false;
        const wp_vertex_pntc vertices[] = { { { -1, -1, 0 }, { 0, 0, 1 }, { 0, 0 }, 0xFFFFFFFFu },
                                            { { 1, -1, 0 }, { 0, 0, 1 }, { 1, 0 }, 0xFFFFFFFFu },
                                            { { 1, 1, 0 }, { 0, 0, 1 }, { 1, 1 }, 0xFFFFFFFFu },
                                            { { -1, 1, 0 }, { 0, 0, 1 }, { 0, 1 }, 0xFFFFFFFFu } };
        const wp_u16 indices[] = { 0, 1, 2, 0, 2, 3 };
        auto *geometry =
            wp_renderer_dx11_create_indexed_geometry_pntc( renderer, vertices, 4, indices, 6, 0 );
        ok &= check( geometry != nullptr, "fog geometry must allocate" );
        auto *resource =
            static_cast<ID3D11Texture2D *>( wp_renderer_dx11_get_render_texture_resource( target ) );
        D3D11_TEXTURE2D_DESC desc{};
        resource->GetDesc( &desc );
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ID3D11Texture2D *staging = nullptr;
        ok &= check( SUCCEEDED( device->CreateTexture2D( &desc, nullptr, &staging ) ),
                     "fog staging must allocate" );
        wp_mat4f identity{};
        for( int i = 0; i < 4; ++i )
            identity.m[i][i] = 1;
        wp_renderer_dx11_set_render_texture( renderer, target );
        wp_renderer_dx11_set_viewport( renderer, { 0, 0, 16, 16 } );
        wp_renderer_dx11_set_world_matrix( renderer, &identity );
        wp_renderer_dx11_set_view_matrix( renderer, &identity );
        wp_renderer_dx11_set_projection_matrix( renderer, &identity );
        wp_renderer_dx11_set_cull_mode( renderer, WORKPHONE_CULL_MODE_NONE );
        wp_renderer_dx11_set_depth_test_enabled( renderer, 0 );
        wp_renderer_dx11_set_blend_mode( renderer, WORKPHONE_BLEND_MODE_NONE );
        wp_renderer_dx11_set_scissor_enabled( renderer, 0 );
        wp_renderer_dx11_set_texture_native( renderer, nullptr );
        wp_renderer_dx11_set_material_textures( renderer, nullptr );
        wp_renderer_dx11_set_environment( renderer, nullptr, 0 );
        for( int mode = 0; geometry && staging && mode < 4; ++mode )
        {
            wp_material_dx11 material{};
            material.base_color = { 0, 0, 1, 1 };
            material.surface = { 0, 1, 1, 1 };
            material.uv_transform.z = 1;
            material.camera_position = { 0, 0, 2, 1 };
            material.ambient_color = { 1, 1, 1, 1 };
            material.light_direction = { 0, 0, -1, 0 };
            material.controls.z = -1;
            material.fog_color = { 1, 0, 0, 1 };
            material.fog_params = { float( mode ), 8, 0, 1 };
            wp_renderer_dx11_set_material( renderer, &material );
            wp_renderer_dx11_clear( renderer, WORKPHONE_CLEAR_FLAG_ALL );
            wp_renderer_dx11_draw_geometry_pntc( renderer, geometry, 0, 6, 0 );
            context->CopyResource( staging, resource );
            D3D11_MAPPED_SUBRESOURCE mapped{};
            if( SUCCEEDED( context->Map( staging, 0, D3D11_MAP_READ, 0, &mapped ) ) )
            {
                const auto *pixel =
                    static_cast<const std::uint8_t *>( mapped.pData ) + 8 * mapped.RowPitch + 8 * 4;
                ok &= check( mode ? pixel[0] > 170 && pixel[2] < 5 : pixel[2] > 100 && pixel[0] < 5,
                             "linear, exponential and squared fog must change rendered pixels; disabled "
                             "fog must preserve them" );
                context->Unmap( staging, 0 );
            }
            else
                ok &= check( false, "fog pixels must map" );
        }
        if ( staging )
            staging->Release();
        wp_renderer_dx11_destroy_geometry( geometry );
        wp_renderer_dx11_set_render_texture( renderer, nullptr );
        wp_renderer_dx11_destroy_render_texture( target );
        return ok;
    }

    bool testDrawConstants( wp_renderer_dx11 *renderer )
    {
        auto *device = static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( renderer ) );
        auto *context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( renderer ) );
        auto *target = wp_renderer_dx11_create_render_texture( renderer, 64, 64 );
        if ( !check( target != nullptr, "constant-buffer test target must be created" ) )
            return false;
        const wp_vertex_pntc vertices[] = {
            { { -0.2f, -0.3f, 0.0f }, { 0, 0, 1 }, { 0, 0 }, 0xFFFFFFFFu },
            { { 0.2f, -0.3f, 0.0f }, { 0, 0, 1 }, { 0, 0 }, 0xFFFFFFFFu },
            { { 0.0f, 0.3f, 0.0f }, { 0, 0, 1 }, { 0, 0 }, 0xFFFFFFFFu } };
        const wp_u16 indices[] = { 0, 1, 2 };
        auto *geometry = wp_renderer_dx11_create_indexed_geometry_pntc( renderer, vertices, 3, indices, 3, 0 );
        bool ok = check( geometry != nullptr, "constant-buffer test geometry must be created" );
        if( geometry )
        {
            wp_renderer_dx11_set_render_texture( renderer, target );
            wp_renderer_dx11_set_viewport( renderer, { 0, 0, 64, 64 } );
            wp_renderer_dx11_set_depth_test_enabled( renderer, 0 );
            wp_renderer_dx11_set_cull_mode( renderer, WORKPHONE_CULL_MODE_NONE );
            wp_renderer_dx11_set_clear_color( renderer, 0, 0, 0, 1 );
            wp_renderer_dx11_clear( renderer, WORKPHONE_CLEAR_FLAG_ALL );
            wp_mat4f identity{};
            for( int i = 0; i < 4; ++i )
                identity.m[i][i] = 1;
            wp_renderer_dx11_set_view_matrix( renderer, &identity );
            wp_renderer_dx11_set_projection_matrix( renderer, &identity );
            const auto started = std::chrono::steady_clock::now();
            // Alternate two transforms/materials without waiting between draws.
            // Both triangles must survive later writes to the same logical buffers.
            for( int draw = 0; draw < 1000; ++draw )
            {
                const bool right = draw % 2 != 0 || draw >= 998;
                // Model the state changes/restoration around Editor mesh draws.
                wp_renderer_dx11_set_blend_mode( renderer, WORKPHONE_BLEND_MODE_ALPHA );
                wp_renderer_dx11_set_blend_mode( renderer, WORKPHONE_BLEND_MODE_NONE );
                wp_renderer_dx11_set_cull_mode( renderer, WORKPHONE_CULL_MODE_BACK );
                wp_renderer_dx11_set_cull_mode( renderer, WORKPHONE_CULL_MODE_NONE );
                wp_renderer_dx11_set_depth_write_enabled( renderer, 1 );
                wp_renderer_dx11_set_depth_write_enabled( renderer, 0 );
                auto world = identity;
                world.m[0][3] = right ? 0.5f : -0.5f;
                wp_material_dx11 material{};
                // The final three draws reuse constants; earlier draws change both.
                material.base_color = right ? wp_vec4f{ 0, 1, 0, 1 } : wp_vec4f{ 1, 0, 0, 1 };
                material.surface.z = material.surface.w = 1;
                wp_renderer_dx11_set_world_matrix( renderer, &world );
                wp_renderer_dx11_set_material( renderer, &material );
                wp_renderer_dx11_draw_geometry_pntc( renderer, geometry, 0, 3, 0 );
            }
            ID3D11Texture2D *readback = nullptr;
            auto *resource = static_cast<ID3D11Texture2D *>( wp_renderer_dx11_get_render_texture_resource( target ) );
            D3D11_TEXTURE2D_DESC desc{};
            resource->GetDesc( &desc );
            desc.Usage = D3D11_USAGE_STAGING;
            desc.BindFlags = 0;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            ok &= check( SUCCEEDED( device->CreateTexture2D( &desc, nullptr, &readback ) ),
                         "readback texture must be created" );
            if( readback )
            {
                context->CopyResource( readback, resource );
                D3D11_MAPPED_SUBRESOURCE mapped{};
                const bool mappedOK = SUCCEEDED( context->Map( readback, 0, D3D11_MAP_READ, 0, &mapped ) );
                ok &= check( mappedOK, "rendered constants must be readable" );
                if( mappedOK )
                {
                    const auto *left = static_cast<const unsigned char *>( mapped.pData ) + 32 * mapped.RowPitch + 16 * 4;
                    const auto *right = static_cast<const unsigned char *>( mapped.pData ) + 32 * mapped.RowPitch + 48 * 4;
                    const int red = desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM ? 2 : 0;
                    ok &= check( left[red] > 200 && left[1] < 10,
                                 "earlier draw must retain its left transform and red material" );
                    ok &= check( right[1] > 200 && right[red] < 10,
                                 "later draw must use its right transform and green material" );
                    context->Unmap( readback, 0 );
                }
                readback->Release();
            }
            const auto elapsed = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - started ).count();
            std::printf( "1000 alternating mesh draws including readback: %.3f ms\n", elapsed );
            wp_renderer_dx11_destroy_geometry( geometry );
        }
        wp_renderer_dx11_set_render_texture( renderer, nullptr );
        wp_renderer_dx11_destroy_render_texture( target );
        return ok;
    }
    bool testNativeMeshPixels( wp_renderer_dx11 *renderer )
    {
        auto *device = static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( renderer ) );
        auto *context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( renderer ) );
        auto *target = wp_renderer_dx11_create_render_texture( renderer, 32, 32 );
        if( !check( target != nullptr, "cache equivalence target allocation" ) ) return false;
        auto *resource = static_cast<ID3D11Texture2D *>( wp_renderer_dx11_get_render_texture_resource( target ) );
        D3D11_TEXTURE2D_DESC desc{};
        resource->GetDesc( &desc );
        desc.Usage = D3D11_USAGE_STAGING; desc.BindFlags = 0; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ID3D11Texture2D *staging = nullptr;
        bool ok = check( SUCCEEDED( device->CreateTexture2D( &desc, nullptr, &staging ) ), "cache equivalence staging allocation" );
        const wp_vertex_pntc authored[] = {
            {{-0.8f,-0.8f,0}, {0,0,1}, {0,0}, 0xff8040ffu},
            {{ 0.8f,-0.8f,0}, {0,0.6f,0.8f}, {1,0}, 0x40ff80ffu},
            {{-0.8f, 0.8f,0}, {0.6f,0,0.8f}, {0,1}, 0x8040ffffu},
            {{ 0.8f, 0.8f,0}, {0,0,1}, {1,1}, 0xffffffffu}
        };
        const uint16_t strip[] = {0,1,2,3};
        const uint16_t triangles[] = {0,1,2,2,1,3};
        const uint16_t range[] = {1,2,3};
        wp_mat4f identity{};
        for( int i = 0; i < 4; ++i ) identity.m[i][i] = 1;
        wp_renderer_dx11_set_render_texture( renderer, target );
        wp_renderer_dx11_set_viewport( renderer, {0,0,32,32} );
        wp_renderer_dx11_set_scissor_enabled( renderer, 0 );
        wp_renderer_dx11_set_world_matrix( renderer, &identity );
        wp_renderer_dx11_set_view_matrix( renderer, &identity );
        wp_renderer_dx11_set_projection_matrix( renderer, &identity );
        wp_renderer_dx11_set_depth_test_enabled( renderer, 0 );
        wp_renderer_dx11_set_blend_mode( renderer, WORKPHONE_BLEND_MODE_NONE );
        wp_renderer_dx11_set_fill_mode( renderer, WORKPHONE_FILL_MODE_SOLID );
        wp_renderer_dx11_set_texture_native( renderer, nullptr );
        wp_renderer_dx11_set_material_textures( renderer, nullptr );
        wp_renderer_dx11_enable_shadow_receiving( renderer, 0 );
        wp_renderer_dx11_set_environment( renderer, nullptr, 0 );
        wp_material_dx11 material{};
        material.base_color = {1,1,1,1}; material.specular_color = {0.04f,0.04f,0.04f,1};
        material.light_color = {1,1,1,2}; material.light_direction = {0,0,-1,0};
        material.camera_position = {0,0,2,0.1f}; material.surface = {0,0.8f,1,1};
        material.uv_transform.z = 1; material.controls.z = -1;
        wp_renderer_dx11_set_material( renderer, &material );
        wp_renderer_dx11_set_clear_color( renderer, 0, 0, 0, 1 );
        for( int mode = 0; staging && mode < 5; ++mode )
        {
            auto *mesh = wp_graphics_mesh_create();
            wp_vertex_pntc reference[4];
            std::memcpy( reference, authored, sizeof( reference ) );
            if( mode >= 3 )
            {
                wp_graphics_mesh_vertex_pt source[4];
                for( int i = 0; i < 4; ++i )
                {
                    source[i] = {authored[i].position, authored[i].uv};
                    reference[i].normal = {0,0,1}; reference[i].color = 0xffffffffu;
                }
                ok &= wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_PT, source, 4 ) != 0;
            }
            else ok &= wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_PNTC, authored, 4 ) != 0;
            const bool isStrip = mode > 0 && mode < 4;
            if( mode != 4 ) ok &= wp_graphics_mesh_set_indices_u16( mesh, isStrip ? strip : triangles, isStrip ? 4 : 6 ) != 0;
            wp_graphics_mesh_set_primitive_type( mesh, isStrip ? WORKPHONE_PRIMITIVE_TRIANGLE_STRIP : WORKPHONE_PRIMITIVE_TRIANGLE_LIST );
            const wp_u32 first = mode == 2 ? 1u : 0u;
            const wp_u32 elements = mode == 2 ? 3u : isStrip ? 4u : mode == 4 ? 4u : 6u;
            const int drawCount = mode == 2 || mode == 4 ? 3 : 6;
            auto *cached = wp_renderer_dx11_prepare_mesh_geometry( renderer, mesh, first, elements );
            auto *raw = wp_renderer_dx11_create_indexed_geometry_pntc( renderer, reference, 4,
                mode == 2 ? range : triangles, drawCount, 0 );
            ok &= check( cached && raw, "cached and reference geometry allocation" );
            for( int cull = 0; cached && raw && cull < 3; ++cull )
            {
                wp_renderer_dx11_set_cull_mode( renderer, cull == 0 ? WORKPHONE_CULL_MODE_NONE :
                    cull == 1 ? WORKPHONE_CULL_MODE_BACK : WORKPHONE_CULL_MODE_FRONT );
                std::vector<unsigned char> pixels[2];
                for( int pass = 0; pass < 2; ++pass )
                {
                    wp_renderer_dx11_clear( renderer, WORKPHONE_CLEAR_FLAG_ALL );
                    wp_renderer_dx11_draw_geometry_pntc( renderer, pass == 0 ? raw : cached, 0, drawCount, 0 );
                    context->CopyResource( staging, resource );
                    D3D11_MAPPED_SUBRESOURCE mapped{};
                    if( SUCCEEDED( context->Map( staging, 0, D3D11_MAP_READ, 0, &mapped ) ) )
                    {
                        pixels[pass].resize( 32 * 32 * 4 );
                        for( int y = 0; y < 32; ++y ) std::memcpy( pixels[pass].data() + y * 32 * 4,
                            static_cast<const unsigned char *>( mapped.pData ) + y * mapped.RowPitch, 32 * 4 );
                        context->Unmap( staging, 0 );
                    }
                    else ok &= check( false, "cache equivalence pixel readback" );
                }
                ok &= check( !pixels[0].empty() && pixels[0] == pixels[1], "native cache must match raw geometry pixels including winding and submesh ranges" );
                if( cull == 0 && !pixels[0].empty() )
                {
                    bool visible = false;
                    for( size_t i = 0; i < pixels[0].size(); i += 4 ) visible |= pixels[0][i] || pixels[0][i+1] || pixels[0][i+2];
                    ok &= check( visible, "cache equivalence reference must draw visible pixels" );
                }
            }
            wp_renderer_dx11_destroy_geometry( raw );
            wp_renderer_dx11_forget_mesh( mesh ); wp_graphics_mesh_destroy( mesh );
        }
        if( staging ) staging->Release();
        wp_renderer_dx11_set_render_texture( renderer, nullptr );
        wp_renderer_dx11_destroy_render_texture( target );
        return ok;
    }

    bool testNativeMeshCache( wp_renderer_dx11 *renderer )
    {
        const wp_graphics_mesh_vertex_p vertices[] = {
            {{-1,-1,0}}, {{1,-1,0}}, {{-1,1,0}}, {{1,1,0}}
        };
        const uint16_t strip[] = {0,1,2,3};
        const wp_u32 invalid[] = {0,1,9};
        auto *mesh = wp_graphics_mesh_create();
        if( !check( mesh != nullptr, "native mesh allocation" ) ) return false;
        bool ok = wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_P, vertices, 4 ) &&
            wp_graphics_mesh_set_indices_u16( mesh, strip, 4 );
        wp_graphics_mesh_set_primitive_type( mesh, WORKPHONE_PRIMITIVE_TRIANGLE_STRIP );
        auto *whole = wp_renderer_dx11_prepare_mesh_geometry( renderer, mesh, 0, 4 );
        ok &= check( whole && wp_renderer_dx11_prepare_mesh_geometry( renderer, mesh, 0, 4 ) == whole,
            "native strip geometry is retained across frames" );
        auto *range = wp_renderer_dx11_prepare_mesh_geometry( renderer, mesh, 1, 3 );
        ok &= check( range && range != whole &&
            wp_renderer_dx11_prepare_mesh_geometry( renderer, mesh, 1, 3 ) == range,
            "submesh-relative strip conversion retains independent geometry" );
        ok &= check( !wp_renderer_dx11_prepare_mesh_geometry( renderer, mesh, 3, 3 ),
            "native mesh cache rejects out-of-bounds draw ranges" );
        wp_graphics_mesh_set_primitive_type( mesh, WORKPHONE_PRIMITIVE_TRIANGLE_LIST );
        wp_graphics_mesh_set_indices_u32( mesh, invalid, 3 );
        ok &= check( !wp_renderer_dx11_prepare_mesh_geometry( renderer, mesh, 0, 3 ),
            "index-only mutation retires cached data and rejects invalid geometry" );
        wp_graphics_mesh_set_indices_u16( mesh, strip, 3 );
        ok &= check( wp_renderer_dx11_prepare_mesh_geometry( renderer, mesh, 0, 3 ) != nullptr,
            "native cache recovers after valid geometry replacement" );
        wp_renderer_dx11_forget_mesh( mesh );
        wp_graphics_mesh_destroy( mesh );
        return ok;
    }

}

int main()
{
    workphone::TypeManager types;
    types.load();
    workphone::TypeManager::setInstance( &types );
    bool ok = true;
    {
        workphone::render::ClawMesh mesh;
        auto *first = wp_graphics_object_create();
        auto *second = wp_graphics_object_create();
        ok &= check( first && second, "submission context test objects must be created" );
        mesh.bindNativeRenderObject( first );
        ok &= check( wp_graphics_object_get_submit_data( first ) == &mesh,
                     "binding must publish the wrapper before mesh resource loading" );
        mesh.bindNativeRenderObject( second );
        ok &= check( wp_graphics_object_get_submit_data( first ) == nullptr &&
                         wp_graphics_object_get_submit_data( second ) == &mesh,
                     "rebinding must clear the old borrowed context" );
        mesh.bindNativeRenderObject( nullptr );
        ok &= check( wp_graphics_object_get_submit_data( second ) == nullptr,
                     "unbinding must clear the borrowed wrapper context" );
        wp_graphics_object_destroy( first );
        wp_graphics_object_destroy( second );
    }
    auto window = workphone::make_ptr<TestWindow>();
    window->handle = CreateWindowExW( 0, L"STATIC", L"DX11 target regression", WS_OVERLAPPEDWINDOW,
                                    0, 0, 1280, 720, nullptr, nullptr, GetModuleHandleW( nullptr ), nullptr );
    if( !check( window->handle != nullptr, "hidden test window must be created" ) )
        return 1;

    {
        workphone::render::ClawRendererDX11 renderer;
        renderer.load( window );
        if( !check( renderer.isLoaded(), "DX11 device must be created (hardware or WARP)" ) )
            return 1;
        auto *native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
        ok &= check( recordDX11TestDevice( static_cast<ID3D11Device *>(
                         wp_renderer_dx11_get_device( native ) ) ),
                     "DX11 device evidence must be available" );
        auto *context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( native ) );
        ok &= testOcclusionDepthReadback( renderer );
        try { ok &= claw_rtt_contracts::native(native); ok &= claw_scene_job_contracts::pixelsMatch(renderer); }
        catch(const std::exception &e) { ok &= check(false, e.what()); }
        renderer.setRenderTarget( window );
#if defined WP_TEST_IMGUI
        ok &= testImgui( renderer );
#endif
        auto *originalDepth = depthTarget( context );
        ok &= check( originalDepth != nullptr, "window depth target must be bound" );
        auto texture = workphone::make_ptr<workphone::render::ClawRenderTarget>();
        texture->setSize( { 960, 540 } );

        const auto started = std::chrono::steady_clock::now();
        bool retained = true;
        for( int frame = 0; frame < 60; ++frame )
        {
            renderer.setRenderTarget( texture );
            renderer.setViewport( nullptr );
            const auto viewport = wp_renderer_dx11_get_viewport( native );
            ok &= check( viewport.width == 960 && viewport.height == 540,
                         "scene viewport must use the render texture size" );
            renderer.clear( workphone::ColourF::Black );
            renderer.setRenderTarget( window );
            renderer.setViewport( nullptr );
            auto *currentDepth = depthTarget( context );
            retained &= currentDepth == originalDepth;
            if( currentDepth )
                currentDepth->Release();
            renderer.clear( workphone::ColourF::Black );
        }
        const auto elapsed = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - started ).count();
        std::printf( "60 scene/window switches and clears: %.3f ms (%.3f ms/frame)\n", elapsed, elapsed / 60 );
        ok &= check( retained, "scene/window switches must retain the window depth buffer" );

        // A retained swap-chain buffer forces real DXGI ResizeBuffers rejection.
        // Keep rendering/capture usable at the last successful extent, then retry.
        ok &= check( wp_renderer_dx11_resize( native, 1280, 720 ),
                     "same-extent native resize must succeed without replacing targets" );
        auto *sameExtentDepth = depthTarget( context );
        ok &= check( sameExtentDepth == originalDepth,
                     "same-extent resize must retain the depth target" );
        if( sameExtentDepth ) sameExtentDepth->Release();
        auto *resizeSwapChain = static_cast<IDXGISwapChain *>(
            wp_renderer_dx11_get_swap_chain( native ) );
        ID3D11Texture2D *retainedBackBuffer = nullptr;
        ok &= check( SUCCEEDED( resizeSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ),
                         reinterpret_cast<void **>( &retainedBackBuffer ) ) ),
                     "resize rejection fixture must retain the current back buffer" );
        ok &= check( !wp_renderer_dx11_resize( native, 1024, 640 ),
                     "retained back buffer must force native ResizeBuffers failure" );
        ID3D11RenderTargetView *restoredWindowOutput = nullptr;
        context->OMGetRenderTargets( 1, &restoredWindowOutput, nullptr );
        ok &= check( restoredWindowOutput != nullptr,
                     "native resize failure must rebind the retained window output" );
        if( restoredWindowOutput ) restoredWindowOutput->Release();
        window->size = { 1024, 640 };
        renderer.setRenderTarget( window );
        renderer.setViewport( nullptr );
        const auto failedResizeViewport = wp_renderer_dx11_get_viewport( native );
        ok &= check( wp_renderer_dx11_get_width( native ) == 1280 &&
                         wp_renderer_dx11_get_height( native ) == 720 &&
                         failedResizeViewport.width == 1280 && failedResizeViewport.height == 720,
                     "rejected window resize must retain actual buffer and viewport dimensions" );
        renderer.beginRender();
        renderer.clear( workphone::ColourF( .25f, .5f, .75f, 1.0f ) );
        std::vector<unsigned char> retainedPixels( 1280u * 720u * 4u );
        ok &= check( wp_renderer_dx11_copy_back_buffer_rgba8(
                         native, window->handle, retainedPixels.data(), retainedPixels.size() ) &&
                         std::abs( static_cast<int>( retainedPixels[0] ) - 64 ) <= 1 &&
                         std::abs( static_cast<int>( retainedPixels[1] ) - 128 ) <= 1 &&
                         std::abs( static_cast<int>( retainedPixels[2] ) - 191 ) <= 1 &&
                         retainedPixels[3] == 255,
                     "rejected resize must leave a drawable and readable window buffer" );
        renderer.endRender();
        if( retainedBackBuffer ) retainedBackBuffer->Release();
        std::puts( "Real DXGI resize rejection: retained extent, viewport, draw and readback passed." );
        window->size = { 1024, 640 };
        renderer.setRenderTarget( window );
        renderer.setViewport( nullptr );
        auto *resizedDepth = depthTarget( context );
        ok &= check( resizedDepth && resizedDepth != originalDepth,
                     "a real window resize must recreate the depth buffer" );
        const auto resizedViewport = wp_renderer_dx11_get_viewport( native );
        ok &= check( resizedViewport.width == 1024 && resizedViewport.height == 640,
                     "window viewport must follow a real resize" );

        texture->setSize( { 319, 179 } );
        renderer.setRenderTarget( texture );
        renderer.setViewport( nullptr );
        const auto textureViewport = wp_renderer_dx11_get_viewport( native );
        ok &= check( textureViewport.width == 319 && textureViewport.height == 179,
                     "resized scene texture must update its viewport" );
        renderer.clear( workphone::ColourF( .25f, .5f, .75f, 1.0f ) );
        constexpr workphone::u32 readbackBytes = 319u * 179u * 4u;
        std::vector<unsigned char> readback( readbackBytes + 4u, 0xA5 );
        texture->copyContentsToMemory( readback.data(), readbackBytes - 1u );
        ok &= check( std::all_of( readback.begin(), readback.end(),
                                  []( unsigned char value ) { return value == 0xA5; } ),
                     "undersized offscreen readback must leave the destination untouched" );
        texture->copyContentsToMemory( readback.data(), readbackBytes );
        const auto pixelMatches = [&]( size_t offset ) {
            return std::abs( static_cast<int>( readback[offset] ) - 64 ) <= 1 &&
                   std::abs( static_cast<int>( readback[offset + 1] ) - 128 ) <= 1 &&
                   std::abs( static_cast<int>( readback[offset + 2] ) - 191 ) <= 1 &&
                   readback[offset + 3] == 255;
        };
        ok &= check( pixelMatches( 0 ) && pixelMatches( 319u * 4u ) &&
                         pixelMatches( 319u * 90u * 4u ) &&
                         pixelMatches( readbackBytes - 4u ) &&
                         std::all_of( readback.begin() + readbackBytes, readback.end(),
                                      []( unsigned char value ) { return value == 0xA5; } ),
                     "offscreen readback must return packed RGBA8 pixels without overrunning capacity" );
        renderer.setRenderTarget( window );
        auto *afterTextureResize = depthTarget( context );
        ok &= check( resizedDepth && afterTextureResize == resizedDepth,
                     "resizing the scene texture must retain the window depth buffer" );
        if( afterTextureResize )
            afterTextureResize->Release();
        if( resizedDepth )
            resizedDepth->Release();
        if( originalDepth )
            originalDepth->Release();
        ok &= testNativeMeshCache( native );
        ok &= testNativeMeshPixels( native );
        ok &= testShadows( renderer );
        ok &= testDrawConstants( native );
        ok &= testMipsAndFog( native );
        renderer.setRenderTarget( window );
        renderer.setViewport( nullptr );
        auto *swapChain = static_cast<IDXGISwapChain *>( wp_renderer_dx11_get_swap_chain( native ) );
        DXGI_SWAP_CHAIN_DESC swapDesc{};
        ok &= check( SUCCEEDED( swapChain->GetDesc( &swapDesc ) ), "swap chain description must be available" );
        std::printf( "Presentation: %s, %u buffers\n",
                     swapDesc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD ? "flip discard" : "legacy discard",
                     swapDesc.BufferCount );
        wp_render_statistics_dx11 beforePresent{}, afterPresent{};
        wp_renderer_dx11_get_statistics( native, &beforePresent );
        ok &= check( !wp_renderer_dx11_try_present( nullptr, window->handle, 0 ) &&
                         !wp_renderer_dx11_try_present( native, nullptr, 0 ) &&
                         !wp_renderer_dx11_try_present( native, GetDesktopWindow(), 0 ),
                     "checked presentation must reject missing renderer/window and foreign HWND" );
        renderer.setRenderTarget( texture );
        ok &= check( !wp_renderer_dx11_try_present( native, window->handle, 0 ),
                     "offscreen rendering must not present the window" );
        renderer.beginRender();
        renderer.endRender();
        renderer.setRenderTarget( window );
        ok &= check( wp_renderer_dx11_begin_hdr_scene( native, 1.0f ),
                     "presentation guard HDR scene must begin" );
        ok &= check( !wp_renderer_dx11_try_present( native, window->handle, 0 ),
                     "unfinished HDR scene must not present" );
        auto *hdrOutput = static_cast<ID3D11ShaderResourceView *>(
            wp_renderer_dx11_get_hdr_scene_view( native ) );
        ok &= check( !wp_renderer_dx11_resize( native, 800, 600 ) &&
                         wp_renderer_dx11_is_hdr_scene_active( native ) &&
                         wp_renderer_dx11_get_hdr_scene_view( native ) == hdrOutput,
                     "resize during HDR must reject without disrupting the active scene" );
        wp_renderer_dx11_end_hdr_scene( native );
        wp_mat4f presentationIdentity{};
        for( int i = 0; i < 4; ++i ) presentationIdentity.m[i][i] = 1.0f;
        ok &= check( wp_renderer_dx11_begin_shadow_map( native, &presentationIdentity, 128 ),
                     "presentation guard shadow pass must begin" );
        ok &= check( !wp_renderer_dx11_try_present( native, window->handle, 0 ),
                     "unfinished shadow pass must not present" );
        auto *shadowDepth = depthTarget( context );
        ok &= check( !wp_renderer_dx11_resize( native, 800, 600 ),
                     "resize during shadow pass must reject" );
        auto *retainedShadowDepth = depthTarget( context );
        ok &= check( shadowDepth && shadowDepth == retainedShadowDepth,
                     "rejected resize must retain the active shadow target" );
        if( shadowDepth ) shadowDepth->Release();
        if( retainedShadowDepth ) retainedShadowDepth->Release();
        wp_renderer_dx11_end_shadow_map( native );
        wp_renderer_dx11_get_statistics( native, &afterPresent );
        ok &= check( beforePresent.present_attempts == afterPresent.present_attempts,
                     "rejected presentation must not call DXGI" );
        wp_renderer_dx11_reset_statistics( native );
        for( int frame = 0; frame < 3; ++frame )
        {
            renderer.beginRender();
            ID3D11RenderTargetView *view = nullptr;
            context->OMGetRenderTargets( 1, &view, nullptr );
            ok &= check( view != nullptr, "beginFrame must rebind the back buffer after Present" );
            if( view ) view->Release();
            renderer.clear( workphone::ColourF::Black );
            renderer.endRender();
            renderer.endRender();
            wp_renderer_dx11_get_statistics( native, &afterPresent );
            ok &= check( afterPresent.present_attempts == static_cast<unsigned>( frame + 1 ) &&
                             afterPresent.present_failures == 0 &&
                             afterPresent.frames + afterPresent.present_occluded ==
                                 afterPresent.present_attempts,
                         "wrapper must present once and count only S_OK as a completed frame" );
        }
        ok &= testStatisticsReadback( native );
        renderer.setRenderTarget( nullptr );
        renderer.beginRender();
        renderer.endRender();
        wp_renderer_dx11_get_statistics( native, &beforePresent );
        ok &= check( beforePresent.present_attempts == afterPresent.present_attempts,
                     "detached wrapper target must end safely without presentation" );
        std::printf( "Checked native presentation: %llu attempts, %llu completed, %llu occluded\n",
                     static_cast<unsigned long long>( afterPresent.present_attempts ),
                     static_cast<unsigned long long>( afterPresent.frames ),
                     static_cast<unsigned long long>( afterPresent.present_occluded ) );
        window->size = { 800, 600 };
        renderer.setRenderTarget( window );
        auto *afterPresentResize = depthTarget( context );
        ok &= check( afterPresentResize != nullptr, "resize after Present must bind a new depth target" );
        if( afterPresentResize )
            afterPresentResize->Release();
        renderer.clear( workphone::ColourF( .75f, .25f, .5f, 1.0f ) );
        constexpr workphone::u32 windowBytes = 800u * 600u * 4u;
        std::vector<unsigned char> windowPixels( windowBytes + 4u, 0xA5 );
        ok &= check( !wp_renderer_dx11_copy_back_buffer_rgba8(
                         native, GetDesktopWindow(), windowPixels.data(), windowBytes ) &&
                         !wp_renderer_dx11_copy_back_buffer_rgba8(
                             native, window->handle, windowPixels.data(), windowBytes - 1u ) &&
                         std::all_of( windowPixels.begin(), windowPixels.end(),
                                      []( unsigned char value ) { return value == 0xA5; } ),
                     "window readback must reject another HWND and undersized output without writes" );
        ok &= check( wp_renderer_dx11_copy_back_buffer_rgba8(
                         native, window->handle, windowPixels.data(), windowBytes ),
                     "current window back buffer must support RGBA8 readback before Present" );
        const auto windowPixelMatches = [&]( size_t offset ) {
            return std::abs( static_cast<int>( windowPixels[offset] ) - 191 ) <= 1 &&
                   std::abs( static_cast<int>( windowPixels[offset + 1] ) - 64 ) <= 1 &&
                   std::abs( static_cast<int>( windowPixels[offset + 2] ) - 128 ) <= 1 &&
                   windowPixels[offset + 3] == 255;
        };
        ok &= check( windowPixelMatches( 0 ) && windowPixelMatches( windowBytes - 4u ) &&
                         std::all_of( windowPixels.begin() + windowBytes, windowPixels.end(),
                                      []( unsigned char value ) { return value == 0xA5; } ),
                     "window readback must copy complete RGBA8 pixels within capacity" );
        try { ok &= claw_rtt_contracts::integration(window); }
        catch(const std::exception &e) { ok &= check(false, e.what()); }
        try { ok &= claw_ui_rtt_contracts::editorPreview(window); }
        catch(const std::exception &e) { ok &= check(false, e.what()); }
        renderer.unload( nullptr );
    }
    DestroyWindow( window->handle );
    window = nullptr;
    workphone::TypeManager::setInstance( nullptr );
    types.unload();
    return ok ? 0 : 1;
}
