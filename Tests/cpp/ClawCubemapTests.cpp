#include <WPGraphics/ClawCubemap.hpp>
#include <windows.h>
#include <d3d11.h>
#include "DX11TestEvidence.hpp"
#include "ClawTextureMipContracts.hpp"
#include <workphone_graphics_renderer_dx11.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <limits>

using workphone::render::ClawCubemap;
static void require( bool condition, const char *message )
{
    if( !condition )
        throw std::runtime_error( message );
}
static std::array<float, 4> cubePixel( ID3D11Device *device, ID3D11DeviceContext *context,
                                       ID3D11ShaderResourceView *view, unsigned face, unsigned mip,
                                       unsigned x, unsigned y )
{
    ID3D11Resource *resource = nullptr;
    view->GetResource( &resource );
    ID3D11Texture2D *source = nullptr;
    require( SUCCEEDED( resource->QueryInterface( __uuidof( ID3D11Texture2D ),
                                                  reinterpret_cast<void **>( &source ) ) ),
             "cube texture" );
    resource->Release();
    D3D11_TEXTURE2D_DESC desc;
    source->GetDesc( &desc );
    const unsigned subresource = face * desc.MipLevels + mip;
    desc.Width >>= mip;
    desc.Height >>= mip;
    desc.MipLevels = desc.ArraySize = 1;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = desc.MiscFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D *staging = nullptr;
    require( SUCCEEDED( device->CreateTexture2D( &desc, nullptr, &staging ) ), "staging cube" );
    context->CopySubresourceRegion( staging, 0, 0, 0, 0, source, subresource, nullptr );
    source->Release();
    D3D11_MAPPED_SUBRESOURCE mapped;
    require( SUCCEEDED( context->Map( staging, 0, D3D11_MAP_READ, 0, &mapped ) ), "read cube" );
    const auto p = reinterpret_cast<const float *>( static_cast<const unsigned char *>( mapped.pData ) +
                                                    y * mapped.RowPitch ) +
                   x * 4;
    const std::array<float, 4> value = { p[0], p[1], p[2], p[3] };
    context->Unmap( staging, 0 );
    staging->Release();
    return value;
}
static void floatTextureContracts()
{
    using namespace workphone;
    using namespace workphone::render;
    claw_texture_mip_contracts::Fixture fixture;
    fixture.initialize();
    auto native = wp_renderer_get_dx11( fixture.renderer->getNativeRenderer() );
    auto device = static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( native ) );
    auto context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( native ) );
    ClawTexture texture;
    Array<f32> radiance{ 4.0f, 2.0f, 0.25f, 1.0f, 8.0f, 0.5f, 3.0f, 1.0f,
                         0.1f, 6.0f, 1.5f, 1.0f, 12.0f, 0.125f, 2.0f, 1.0f };
    String error;
    // Exercise the public ITexture virtual bridge used by the sample sky.
    ITexture &textureInterface = texture;
    require( textureInterface.copyLinearFloatData( radiance, { 2, 2 }, error ), "HDR texture bridge must accept finite linear radiance" );
    auto *view = static_cast<ID3D11ShaderResourceView *>( texture.getNativeTextureView( native ) );
    require( view != nullptr, "HDR texture bridge must upload on the explicit render owner" );
    D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription{};
    view->GetDesc( &viewDescription );
    require( viewDescription.Format == DXGI_FORMAT_R32G32B32A32_FLOAT,
             "HDR texture bridge must retain an RGBA32F native view" );
    const auto checkRadiance = [&]( ID3D11ShaderResourceView *uploaded ) {
        for( unsigned y = 0; y < 2; ++y ) for( unsigned x = 0; x < 2; ++x )
        {
            const auto value = cubePixel( device, context, uploaded, 0, 0, x, y );
            for( unsigned c = 0; c < 4; ++c )
                require( std::abs( value[c] - radiance[( y * 2 + x ) * 4 + c] ) < 0.000001f,
                         "HDR upload/reupload must retain exact unclipped linear radiance" );
        }
    };
    checkRadiance( view );
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> retained = view;
    const auto reject = [&]( const Array<f32> &invalid, const Vector2I &size ) {
        require( !textureInterface.copyLinearFloatData( invalid, size, error ) && !error.empty(),
                 "Malformed HDR input must fail with a diagnostic" );
        require( texture.getNativeTextureView( native ) == retained.Get(),
                 "Malformed HDR input must preserve the last good GPU view" );
        checkRadiance( retained.Get() );
    };
    auto invalid = radiance;
    invalid.pop_back();
    reject( invalid, { 2, 2 } );
    reject( radiance, { 0, 2 } );
    invalid = radiance;
    invalid[3] = std::numeric_limits<f32>::quiet_NaN();
    reject( invalid, { 2, 2 } );
    invalid = radiance;
    invalid[5] = std::numeric_limits<f32>::infinity();
    reject( invalid, { 2, 2 } );
    invalid = radiance;
    invalid[0] = -0.01f;
    reject( invalid, { 2, 2 } );
    texture.unload( nullptr );
    texture.load( nullptr );
    auto *reloaded = static_cast<ID3D11ShaderResourceView *>( texture.getNativeTextureView( native ) );
    require( reloaded && reloaded != retained.Get(), "HDR unload/load must recreate the retained float texture" );
    checkRadiance( reloaded );
    std::printf( "HDR sample texture bridge: RGBA32F radiance >1, invalid-input retention and unload/load reupload PASS\n" );
}

int main()
{
    HWND window = CreateWindowExW( 0, L"STATIC", L"PBR test", WS_OVERLAPPED, 0, 0, 32, 32, nullptr,
                                   nullptr, GetModuleHandleW( nullptr ), nullptr );
    auto renderer = wp_renderer_dx11_create( window, 32, 32 );
    if( !renderer )
    {
        DestroyWindow( window );
        std::fprintf( stderr, "DX11 shader/device creation failed\n" );
        return 1;
    }
    int result = 0;
    try
    {
        claw_texture_mip_contracts::run();
        floatTextureContracts();
        auto device = static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( renderer ) );
        require( recordDX11TestDevice( device ), "DX11 device evidence must be available" );
        std::array<wp_texture_cube_mip_dx11, 48> invalidCube{};
        require( !wp_dx11_create_cubemap_rgba32f_native( device, invalidCube.data(), 47 ) &&
                     !wp_dx11_create_cubemap_rgba32f_native( device, invalidCube.data(), 48 ),
                 "native cubemap upload must reject incomplete face and mip data" );
        auto context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( renderer ) );
        ClawCubemap cube;
        std::array<ID3D11ShaderResourceView *, 6> views{};
        // Every sky face has a unique red value and an asymmetric green horizontal ramp.
        for( unsigned f = 0; f < 6; ++f )
        {
            unsigned char pixels[4 * 4 * 4];
            for( unsigned y = 0; y < 4; ++y )
                for( unsigned x = 0; x < 4; ++x )
                {
                    const unsigned i = ( y * 4 + x ) * 4;
                    pixels[i] = 32 + f * 32;
                    pixels[i + 1] = x * 85;
                    pixels[i + 2] = y * 85;
                    pixels[i + 3] = 255;
                }
            views[f] = static_cast<ID3D11ShaderResourceView *>( wp_renderer_dx11_create_texture_native(
                renderer, pixels, 4, 4, WORKPHONE_PIXEL_FORMAT_RGBA8 ) );
            require( views[f] != nullptr, "source face" );
        }
        wp_texture_face_readback_dx11 faceReadback{};
        require( wp_dx11_read_texture_face_native( device, context, views[0], &faceReadback ) &&
                     faceReadback.width == 4 && faceReadback.height == 4 &&
                     faceReadback.row_pitch == 16 && faceReadback.format == WP_DX11_FACE_RGBA8,
                 "native source-face readback must expose packed RGBA rows" );
        const auto *firstFacePixel = static_cast<const unsigned char *>( faceReadback.pixels );
        require( firstFacePixel[0] == 32 && firstFacePixel[1] == 0,
                 "native source-face readback must preserve encoded pixels" );
        wp_dx11_release_texture_face_readback_native( &faceReadback );
        require( !faceReadback.pixels && !wp_dx11_read_texture_face_native(
                     device, context, nullptr, &faceReadback ),
                 "native source-face readback must clear rejected output" );
        require( cube.update( device, context, views ), "build filtered cube" );
        const unsigned skyFace[] = { 3, 2, 4, 5, 1, 0 };
        for( unsigned f = 0; f < 6; ++f )
        {
            const auto center = cubePixel( device, context, cube.getView(), f, 0, 64, 64 );
            require(
                std::abs( center[0] - std::pow( ( ( 32 + skyFace[f] * 32 ) / 255.0f + .055f ) / 1.055f, 2.4f ) ) < 0.001f,
                "face order / linear color" );
            const auto left = cubePixel( device, context, cube.getView(), f, 0, 0, 64 );
            const auto right = cubePixel( device, context, cube.getView(), f, 0, 127, 64 );
            require( f == 2 || f == 3 ? left[1] < right[1] : left[1] > right[1],
                     "sky horizontal orientation" );
            const auto top = cubePixel( device, context, cube.getView(), f, 0, 64, 0 );
            const auto bottom = cubePixel( device, context, cube.getView(), f, 0, 64, 127 );
            require( top[2] < bottom[2], "sky vertical orientation" );
            const auto rough = cubePixel( device, context, cube.getView(), f, 7, 0, 0 );
            require( std::isfinite( rough[0] ) && rough[1] > 0 && rough[1] < 1,
                     "finite filtered levels" );
        }
        auto oldView = cube.getView();
        require( cube.update( device, context, views ) && cube.getView() == oldView,
                 "unchanged faces reuse GPU cube" );
        auto incomplete = views;
        incomplete[0] = nullptr;
        require( !cube.update( device, context, incomplete ) && cube.getView() == oldView,
                 "partial sky preserves complete cube" );

        // Render through the production shader, with a red environment and no direct light.
        wp_material_dx11 material{};
        material.base_color = { 1, 1, 1, 1 };
        material.specular_color = { 0.04f, 0.04f, 0.04f, 1 };
        material.camera_position = { 0, 0, -2, 1 };
        material.ambient_color = { 1, 0, 0, 1 };
        material.surface = { 1, 0.1f, 1, 1 };
        material.uv_transform = { 0, 0, 1, 0 };
        material.controls = { 1, 1, -1, 0 };
        wp_renderer_dx11_set_environment( renderer, cube.getView(), cube.getMaxLod() );
        wp_renderer_dx11_set_material( renderer, &material );
        wp_renderer_dx11_set_cull_mode( renderer, WORKPHONE_CULL_MODE_NONE );
        wp_renderer_dx11_set_depth_test_enabled( renderer, 0 );
        wp_renderer_dx11_set_depth_write_enabled( renderer, 0 );
        const wp_vertex_pntc vertices[] = { { { -1, -1, 0 }, { 0, 0, -1 }, { 0, 1 }, 0xFFFFFFFF },
                                            { { 1, -1, 0 }, { 0, 0, -1 }, { 1, 1 }, 0xFFFFFFFF },
                                            { { 0, 1, 0 }, { 0, 0, -1 }, { 0.5f, 0 }, 0xFFFFFFFF } };
        // Match the renderer's front-face winding so SV_IsFrontFace preserves
        // the authored normal pointing toward the fixture camera.
        const unsigned short indices[] = { 0, 1, 2 };
        auto geometry =
            wp_renderer_dx11_create_indexed_geometry_pntc( renderer, vertices, 3, indices, 3, 0 );
        require( geometry != nullptr, "PBR geometry" );
        auto target = wp_renderer_dx11_create_render_texture( renderer, 32, 32 );
        require( target != nullptr, "PBR target" );
        wp_renderer_dx11_begin_frame( renderer );
        wp_renderer_dx11_set_render_texture( renderer, target );
        wp_renderer_dx11_draw_geometry_pntc( renderer, geometry, 0, 3, 0 );
        wp_render_statistics_dx11 cachedBefore{}, cachedAfter{};
        wp_renderer_dx11_get_statistics( renderer, &cachedBefore );
        for( int repeat = 0; repeat < 3; ++repeat )
        {
            wp_renderer_dx11_set_environment( renderer, cube.getView(), cube.getMaxLod() );
            wp_renderer_dx11_set_material( renderer, &material );
            wp_renderer_dx11_draw_geometry_pntc( renderer, geometry, 0, 3, 0 );
        }
        wp_renderer_dx11_get_statistics( renderer, &cachedAfter );
        require( cachedAfter.draws == cachedBefore.draws + 3 &&
                 cachedAfter.material_uploads == cachedBefore.material_uploads &&
                 cachedAfter.state_bindings == cachedBefore.state_bindings,
                 "unchanged environment and resolved material reuse constant buffer" );
        // External context changes invalidate bindings without forcing a material upload.
        context->PSSetShader( nullptr, nullptr, 0 );
        wp_renderer_dx11_invalidate_state( renderer );
        wp_renderer_dx11_draw_geometry_pntc( renderer, geometry, 0, 3, 0 );
        wp_renderer_dx11_get_statistics( renderer, &cachedAfter );
        require( cachedAfter.state_bindings > cachedBefore.state_bindings &&
                 cachedAfter.material_uploads == cachedBefore.material_uploads,
                 "external context invalidation restores shader bindings" );
        material.emissive_color.x = 0.1f;
        wp_renderer_dx11_set_material( renderer, &material );
        wp_renderer_dx11_draw_geometry_pntc( renderer, geometry, 0, 3, 0 );
        wp_renderer_dx11_get_statistics( renderer, &cachedAfter );
        require( cachedAfter.material_uploads == cachedBefore.material_uploads + 1,
                 "editing existing material invalidates upload cache" );
        material.emissive_color.x = 0;
        wp_renderer_dx11_set_material( renderer, &material );
        wp_renderer_dx11_draw_geometry_pntc( renderer, geometry, 0, 3, 0 );
        auto texture =
            static_cast<ID3D11Texture2D *>( wp_renderer_dx11_get_render_texture_resource( target ) );
        D3D11_TEXTURE2D_DESC desc;
        texture->GetDesc( &desc );
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = desc.MiscFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ID3D11Texture2D *staging = nullptr;
        require( SUCCEEDED( device->CreateTexture2D( &desc, nullptr, &staging ) ), "PBR readback" );
        context->CopyResource( staging, texture );
        D3D11_MAPPED_SUBRESOURCE mapped;
        require( SUCCEEDED( context->Map( staging, 0, D3D11_MAP_READ, 0, &mapped ) ),
                 "PBR readback map" );
        const auto p =
            static_cast<const unsigned char *>( mapped.pData ) + 16 * mapped.RowPitch + 16 * 4;
        require( p[0] > 20 && p[1] < 2 && p[2] < 2,
                 "metal reflects environment and preserves ambient tint" );
        context->Unmap( staging, 0 );
        // Authored material probes carry radiance independently of the diffuse ambient.
        material.ambient_color = { 0, 0, 0, 1 };
        material.environment.z = 1;
        wp_renderer_dx11_set_material( renderer, &material );
        wp_renderer_dx11_draw_geometry_pntc( renderer, geometry, 0, 3, 0 );
        context->CopyResource( staging, texture );
        require( SUCCEEDED( context->Map( staging, 0, D3D11_MAP_READ, 0, &mapped ) ),
                 "authored probe readback" );
        const auto authored =
            static_cast<const unsigned char *>( mapped.pData ) + 16 * mapped.RowPitch + 16 * 4;
        require( authored[1] > 20 && authored[2] > 20,
                 "authored probe stays visible with black diffuse ambient" );
        context->Unmap( staging, 0 );
        material.ambient_color = { 1, 0, 0, 1 };
        material.environment.z = 0;
        wp_renderer_dx11_set_environment( renderer, nullptr, 0 );
        wp_renderer_dx11_set_material( renderer, &material );
        wp_renderer_dx11_draw_geometry_pntc( renderer, geometry, 0, 3, 0 );
        context->CopyResource( staging, texture );
        require( SUCCEEDED( context->Map( staging, 0, D3D11_MAP_READ, 0, &mapped ) ),
                 "unlit metal readback" );
        const auto dark =
            static_cast<const unsigned char *>( mapped.pData ) + 16 * mapped.RowPitch + 16 * 4;
        require( dark[0] < 2 && dark[1] < 2 && dark[2] < 2,
                 "metal has no diffuse ambient contribution" );
        context->Unmap( staging, 0 );
        material.surface.x = 0;
        wp_renderer_dx11_set_material( renderer, &material );
        wp_renderer_dx11_draw_geometry_pntc( renderer, geometry, 0, 3, 0 );
        context->CopyResource( staging, texture );
        require( SUCCEEDED( context->Map( staging, 0, D3D11_MAP_READ, 0, &mapped ) ),
                 "dielectric readback" );
        const auto diffuse =
            static_cast<const unsigned char *>( mapped.pData ) + 16 * mapped.RowPitch + 16 * 4;
        require( diffuse[0] > 100 && diffuse[1] < 2 && diffuse[2] < 2,
                 "metallic slider restores tinted diffuse response at zero" );
        context->Unmap( staging, 0 );
        staging->Release();
        wp_renderer_dx11_set_render_texture( renderer, nullptr );
        wp_renderer_dx11_destroy_render_texture( target );
        wp_renderer_dx11_destroy_geometry( geometry );
        wp_renderer_dx11_set_environment( renderer, nullptr, 0 );
        cube.reset();
        for( auto view : views )
            wp_renderer_dx11_destroy_texture_native( view );
        require( cube.update( device, context, {} ), "studio fallback" );
        const auto smooth = cubePixel( device, context, cube.getView(), 2, 0, 64, 64 );
        const auto rough = cubePixel( device, context, cube.getView(), 2, 7, 0, 0 );
        require( std::isfinite( rough[0] ) && std::abs( smooth[0] - rough[0] ) > 0.02f,
                 "fallback varies with roughness" );
        // GPU readback returns encoded bytes for both UNORM and sRGB views;
        // floating-point faces are already linear radiance and may exceed one.
        for( const auto format : { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                                  DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT } )
        {
            const bool hdr = format == DXGI_FORMAT_R16G16B16A16_FLOAT || format == DXGI_FORMAT_R32G32B32A32_FLOAT;
            const unsigned char encoded[] = { 128, 64, 192, 255 };
            const unsigned short half[] = { 0x4000, 0x3800, 0x3000, 0x3c00 }; // 2, .5, .125, 1
            const float linear[] = { 2, .5f, .125f, 1 };
            D3D11_TEXTURE2D_DESC faceDesc{};
            faceDesc.Width = faceDesc.Height = faceDesc.MipLevels = faceDesc.ArraySize = 1;
            faceDesc.SampleDesc.Count = 1;
            faceDesc.Format = format;
            faceDesc.Usage = D3D11_USAGE_IMMUTABLE;
            faceDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            D3D11_SUBRESOURCE_DATA source{};
            source.pSysMem = format == DXGI_FORMAT_R16G16B16A16_FLOAT ? static_cast<const void *>( half )
                : format == DXGI_FORMAT_R32G32B32A32_FLOAT ? static_cast<const void *>( linear ) : encoded;
            source.SysMemPitch = hdr ? format == DXGI_FORMAT_R16G16B16A16_FLOAT ? 8 : 16 : 4;
            std::array<ID3D11ShaderResourceView *, 6> colourViews{};
            for( auto &view : colourViews )
            {
                ID3D11Texture2D *faceTexture = nullptr;
                require( SUCCEEDED( device->CreateTexture2D( &faceDesc, &source, &faceTexture ) ), "radiance face texture" );
                const auto created = device->CreateShaderResourceView( faceTexture, nullptr, &view );
                faceTexture->Release();
                require( SUCCEEDED( created ), "radiance face view" );
            }
            require( cube.update( device, context, colourViews ), "sRGB/HDR radiance faces must build" );
            const float expected[] = { hdr ? 2.f : .2158605f, hdr ? .5f : .0512695f, hdr ? .125f : .5271151f };
            for( const unsigned mip : { 0u, 7u } )
            {
                const auto value = cubePixel( device, context, cube.getView(), 0, mip, 0, 0 );
                for( unsigned channel = 0; channel < 3; ++channel )
                    require( std::abs( value[channel] - expected[channel] ) < .001f,
                             "environment prefilter must preserve exact linear HDR/decoded sRGB radiance" );
            }
            if( format == DXGI_FORMAT_R32G32B32A32_FLOAT )
            {
                const float invalid[] = { std::numeric_limits<float>::quiet_NaN(), 1, 1, 1 };
                source.pSysMem = invalid;
                ID3D11Texture2D *invalidTexture = nullptr;
                ID3D11ShaderResourceView *invalidView = nullptr;
                require( SUCCEEDED( device->CreateTexture2D( &faceDesc, &source, &invalidTexture ) ), "invalid radiance fixture" );
                require( SUCCEEDED( device->CreateShaderResourceView( invalidTexture, nullptr, &invalidView ) ), "invalid radiance view" );
                invalidTexture->Release();
                auto invalidFaces = colourViews;
                invalidFaces[0] = invalidView;
                const auto retained = cube.getView();
                require( !cube.update( device, context, invalidFaces ) && cube.getView() == retained,
                         "Nonfinite environment must preserve previous filtered cube" );
                invalidView->Release();
            }
            for( auto view : colourViews ) view->Release();
        }
        // Respect the SRV's selected source mip rather than always reading resource mip zero.
        {
            const float base[] = { 16,16,16,1, 16,16,16,1, 16,16,16,1, 16,16,16,1 };
            const float selected[] = { 2,.5f,.125f,1 };
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = desc.Height = desc.MipLevels = 2;
            desc.ArraySize = desc.SampleDesc.Count = 1;
            desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            desc.Usage = D3D11_USAGE_IMMUTABLE;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            const D3D11_SUBRESOURCE_DATA data[] = { {base,32,64}, {selected,16,16} };
            ID3D11Texture2D *texture = nullptr;
            require( SUCCEEDED( device->CreateTexture2D( &desc, data, &texture ) ), "selected mip source" );
            D3D11_SHADER_RESOURCE_VIEW_DESC srv{};
            srv.Format = desc.Format;
            srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srv.Texture2D.MostDetailedMip = srv.Texture2D.MipLevels = 1;
            ID3D11ShaderResourceView *view = nullptr;
            const auto created = device->CreateShaderResourceView( texture, &srv, &view );
            texture->Release();
            require( SUCCEEDED( created ), "selected mip view" );
            std::array<ID3D11ShaderResourceView *, 6> selectedFaces{};
            selectedFaces.fill( view );
            require( cube.update( device, context, selectedFaces ), "selected mip cube" );
            const auto value = cubePixel( device, context, cube.getView(), 0, 0, 0, 0 );
            require( std::abs( value[0] - 2.f ) < .001f && std::abs( value[1] - .5f ) < .001f,
                     "Environment must prefilter the view's selected mip" );
            view->Release();
        }
        std::puts( "Cubemap orientation, filtering, caching, linear HDR/sRGB, selected mip and PBR GPU checks passed." );
    }
    catch( const std::exception &e )
    {
        std::fprintf( stderr, "%s\n", e.what() );
        result = 1;
    }
    wp_renderer_dx11_destroy( renderer );
    DestroyWindow( window );
    return result;
}
