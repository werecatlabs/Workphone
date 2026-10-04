#include <WPGraphics/ClawCubemap.hpp>
#include <windows.h>
#include <d3d11.h>
#include <workphone_graphics_renderer_dx11.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>

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
        auto device = static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( renderer ) );
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
        require( cube.update( device, context, views ), "build filtered cube" );
        const unsigned skyFace[] = { 3, 2, 4, 5, 1, 0 };
        for( unsigned f = 0; f < 6; ++f )
        {
            const auto center = cubePixel( device, context, cube.getView(), f, 0, 64, 64 );
            require(
                std::abs( center[0] - std::pow( ( 32 + skyFace[f] * 32 ) / 255.0f, 2.2f ) ) < 0.001f,
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
        const unsigned short indices[] = { 0, 2, 1 };
        auto geometry =
            wp_renderer_dx11_create_indexed_geometry_pntc( renderer, vertices, 3, indices, 3, 0 );
        require( geometry != nullptr, "PBR geometry" );
        auto target = wp_renderer_dx11_create_render_texture( renderer, 32, 32 );
        require( target != nullptr, "PBR target" );
        wp_renderer_dx11_begin_frame( renderer );
        wp_renderer_dx11_set_render_texture( renderer, target );
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
        std::puts( "Cubemap orientation, filtering, caching and PBR GPU checks passed." );
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
