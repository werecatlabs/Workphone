#include "FrameCapture.h"
#include <Workphone/Workphone.hpp>
#if WP_GRAPHICS_SYSTEM_CLAW && defined( _WIN32 )
#    include <WPGraphics/ClawRendererDX11.hpp>
#    include <workphone_graphics_renderer.h>
#    include <d3d11.h>
#    include <wrl/client.h>
#    include <fstream>
#endif

namespace workphone::advanced
{
    bool captureFrame( const std::string &path )
    {
#if WP_GRAPHICS_SYSTEM_CLAW && defined( _WIN32 )
        using Microsoft::WRL::ComPtr;
        auto graphics = core::IApplicationManager::instance()->getGraphicsSystem();
        auto renderer = dynamic_cast<render::ClawRendererDX11 *>( graphics->getRendererPtr() );
        if( !renderer )
            return false;
        auto device =
            static_cast<ID3D11Device *>( wp_renderer_get_dx11_device( renderer->getNativeRenderer() ) );
        if( !device )
            return false;
        ComPtr<ID3D11DeviceContext> context;
        device->GetImmediateContext( &context );
        ComPtr<ID3D11RenderTargetView> target;
        context->OMGetRenderTargets( 1, &target, nullptr );
        if( !target )
            return false;
        ComPtr<ID3D11Resource> resource;
        target->GetResource( &resource );
        ComPtr<ID3D11Texture2D> source;
        if( FAILED( resource.As( &source ) ) )
            return false;
        D3D11_TEXTURE2D_DESC desc;
        source->GetDesc( &desc );
        if( desc.SampleDesc.Count != 1 ||
            ( desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM && desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM ) )
            return false;
        const bool rgba = desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags = 0;
        ComPtr<ID3D11Texture2D> staging;
        if( FAILED( device->CreateTexture2D( &desc, nullptr, &staging ) ) )
            return false;
        context->CopyResource( staging.Get(), source.Get() );
        D3D11_MAPPED_SUBRESOURCE data;
        if( FAILED( context->Map( staging.Get(), 0, D3D11_MAP_READ, 0, &data ) ) )
            return false;
        BITMAPFILEHEADER header{};
        header.bfType = 0x4d42;
        header.bfOffBits = sizeof( header ) + sizeof( BITMAPINFOHEADER );
        header.bfSize = header.bfOffBits + desc.Width * desc.Height * 4;
        BITMAPINFOHEADER info{};
        info.biSize = sizeof( info );
        info.biWidth = desc.Width;
        info.biHeight = -static_cast<LONG>( desc.Height );
        info.biPlanes = 1;
        info.biBitCount = 32;
        info.biCompression = BI_RGB;
        std::ofstream out( path, std::ios::binary );
        out.write( reinterpret_cast<const char *>( &header ), sizeof( header ) );
        out.write( reinterpret_cast<const char *>( &info ), sizeof( info ) );
        std::vector<unsigned char> row( desc.Width * 4 );
        for( UINT y = 0; y < desc.Height; ++y )
        {
            auto input = static_cast<const unsigned char *>( data.pData ) + y * data.RowPitch;
            for( UINT x = 0; x < desc.Width; ++x )
            {
                row[x * 4] = input[x * 4 + ( rgba ? 2 : 0 )];
                row[x * 4 + 1] = input[x * 4 + 1];
                row[x * 4 + 2] = input[x * 4 + ( rgba ? 0 : 2 )];
                row[x * 4 + 3] = 255;
            }
            out.write( reinterpret_cast<const char *>( row.data() ), row.size() );
        }
        context->Unmap( staging.Get(), 0 );
        return out.good();
#else
        return false;
#endif
    }
}  // namespace workphone::advanced
