#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/ClawCamera.hpp>
#include <Workphone/Graphics/GraphicsWindow.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>
#include <WorkphoneGraphics/workphone_graphics_renderer.h>
#include <WorkphoneGraphics/workphone_graphics_object.h>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include "DX11TestEvidence.hpp"
#include <chrono>
#include <cstdio>
#include <cmath>

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

    ID3D11DepthStencilView *depthTarget( ID3D11DeviceContext *context )
    {
        ID3D11DepthStencilView *view = nullptr;
        context->OMGetRenderTargets( 0, nullptr, &view );
        return view;
    }

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
        auto *geometry = wp_renderer_dx11_create_indexed_geometry_pntc( renderer, vertices, 4, indices, 6, 0 );
        bool ok = check( geometry != nullptr, "shadow geometry must be created" );
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
        wp_renderer_dx11_destroy_geometry( geometry );
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
        renderer.setRenderTarget( window );
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

        window->size = { 1024, 640 };
        renderer.setRenderTarget( window );
        renderer.setViewport( nullptr );
        auto *resizedDepth = depthTarget( context );
        ok &= check( resizedDepth && resizedDepth != originalDepth,
                     "a real window resize must recreate the depth buffer" );
        const auto resizedViewport = wp_renderer_dx11_get_viewport( native );
        ok &= check( resizedViewport.width == 1024 && resizedViewport.height == 640,
                     "window viewport must follow a real resize" );

        texture->setSize( { 320, 180 } );
        renderer.setRenderTarget( texture );
        renderer.setViewport( nullptr );
        const auto textureViewport = wp_renderer_dx11_get_viewport( native );
        ok &= check( textureViewport.width == 320 && textureViewport.height == 180,
                     "resized scene texture must update its viewport" );
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
        for( int frame = 0; frame < 3; ++frame )
        {
            wp_renderer_dx11_begin_frame( native );
            ID3D11RenderTargetView *view = nullptr;
            context->OMGetRenderTargets( 1, &view, nullptr );
            ok &= check( view != nullptr, "beginFrame must rebind the back buffer after Present" );
            if( view )
                view->Release();
            renderer.clear( workphone::ColourF::Black );
            wp_renderer_dx11_end_frame( native );
            ok &= check( SUCCEEDED( swapChain->Present( 0, 0 ) ), "consecutive presentations must succeed" );
        }
        window->size = { 800, 600 };
        renderer.setRenderTarget( window );
        auto *afterPresentResize = depthTarget( context );
        ok &= check( afterPresentResize != nullptr, "resize after Present must bind a new depth target" );
        if( afterPresentResize )
            afterPresentResize->Release();
        renderer.unload( nullptr );
    }
    DestroyWindow( window->handle );
    window = nullptr;
    workphone::TypeManager::setInstance( nullptr );
    types.unload();
    return ok ? 0 : 1;
}
