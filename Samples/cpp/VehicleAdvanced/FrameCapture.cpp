#include "FrameCapture.h"
#include "ReviewEvidence.h"
#include <Workphone/Workphone.hpp>
#include <cmath>
#include <limits>
#if WP_GRAPHICS_SYSTEM_CLAW && defined( _WIN32 )
#    include <WPGraphics/ClawRendererDX11.hpp>
#    include <workphone_graphics_renderer.h>
#    include <d3d11.h>
#    include <wrl/client.h>
#    include <fstream>
#endif

namespace workphone::advanced
{
    std::string captureDeviceEvidence()
    {
#if WP_GRAPHICS_SYSTEM_CLAW && defined( _WIN32 )
        using Microsoft::WRL::ComPtr;
        auto graphics = core::IApplicationManager::instance()->getGraphicsSystem();
        auto renderer = dynamic_cast<render::ClawRendererDX11 *>( graphics->getRendererPtr() );
        if( !renderer ) return "null";
        auto device = static_cast<ID3D11Device *>( wp_renderer_get_dx11_device( renderer->getNativeRenderer() ) );
        if( !device ) return "null";
        ComPtr<IDXGIDevice> dxgi;
        ComPtr<IDXGIAdapter> adapter;
        DXGI_ADAPTER_DESC desc{};
        if( FAILED( device->QueryInterface( IID_PPV_ARGS( &dxgi ) ) ) ||
            FAILED( dxgi->GetAdapter( &adapter ) ) || FAILED( adapter->GetDesc( &desc ) ) ) return "null";
        const auto length = WideCharToMultiByte( CP_UTF8, 0, desc.Description, -1, nullptr, 0, nullptr, nullptr );
        std::string name( size_t( std::max( length, 1 ) ), '\0' );
        if( length > 0 ) WideCharToMultiByte( CP_UTF8, 0, desc.Description, -1, &name[0], length, nullptr, nullptr );
        if( !name.empty() && name.back() == '\0' ) name.pop_back();
        LARGE_INTEGER driver{};
        const bool hasDriver = SUCCEEDED( adapter->CheckInterfaceSupport( __uuidof( IDXGIDevice ), &driver ) );
        std::ostringstream out;
        out << "{\"adapter\": " << jsonString( name ) << ", \"vendorId\": " << desc.VendorId
            << ", \"deviceId\": " << desc.DeviceId << ", \"dedicatedVideoMemory\": " << desc.DedicatedVideoMemory
            << ", \"featureLevel\": " << unsigned( device->GetFeatureLevel() ) << ", \"driverVersion\": ";
        if( hasDriver ) out << jsonString( std::to_string( HIWORD( driver.HighPart ) ) + "." +
            std::to_string( LOWORD( driver.HighPart ) ) + "." + std::to_string( HIWORD( driver.LowPart ) ) + "." +
            std::to_string( LOWORD( driver.LowPart ) ) );
        else out << "null";
        ComPtr<ID3D11DeviceContext> context;
        device->GetImmediateContext( &context );
        ComPtr<ID3D11RenderTargetView> target;
        context->OMGetRenderTargets( 1, &target, nullptr );
        if( target )
        {
            ComPtr<ID3D11Resource> resource;
            target->GetResource( &resource );
            ComPtr<ID3D11Texture2D> texture;
            if( SUCCEEDED( resource.As( &texture ) ) )
            {
                D3D11_TEXTURE2D_DESC actual{};
                texture->GetDesc( &actual );
                out << ", \"captureTarget\": {\"width\": " << actual.Width << ", \"height\": " << actual.Height
                    << ", \"samples\": " << actual.SampleDesc.Count << ", \"format\": " << unsigned( actual.Format ) << "}";
            }
        }
        out << "}";
        return out.str();
#else
        return "null";
#endif
    }

    std::string captureRadianceEvidence( void *textureView, bool &valid )
    {
        valid = false;
#if WP_GRAPHICS_SYSTEM_CLAW && defined( _WIN32 )
        using Microsoft::WRL::ComPtr;
        auto view = static_cast<ID3D11ShaderResourceView *>( textureView );
        if( !view ) return "{\"available\":false}";
        D3D11_SHADER_RESOURCE_VIEW_DESC srv{};
        view->GetDesc( &srv );
        ComPtr<ID3D11Resource> resource;
        view->GetResource( &resource );
        ComPtr<ID3D11Texture2D> source;
        if( FAILED( resource.As( &source ) ) || srv.ViewDimension != D3D11_SRV_DIMENSION_TEXTURECUBE ||
            srv.Format != DXGI_FORMAT_R32G32B32A32_FLOAT || srv.TextureCube.MostDetailedMip != 0 ) return "{\"available\":false}";
        D3D11_TEXTURE2D_DESC desc{};
        source->GetDesc( &desc );
        if( desc.ArraySize != 6 || desc.Width > 2048 || desc.Height > 2048 ) return "{\"available\":false}";
        ComPtr<ID3D11Device> device;
        source->GetDevice( &device );
        auto graphics = core::IApplicationManager::instance()->getGraphicsSystem();
        auto renderer = dynamic_cast<render::ClawRendererDX11 *>( graphics->getRendererPtr() );
        if( !renderer || device.Get() != wp_renderer_get_dx11_device( renderer->getNativeRenderer() ) ) return "{\"available\":false}";
        ComPtr<ID3D11DeviceContext> context;
        device->GetImmediateContext( &context );
        const auto sourceMips = desc.MipLevels;
        desc.MipLevels = 1;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = desc.MiscFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ComPtr<ID3D11Texture2D> staging;
        if( FAILED( device->CreateTexture2D( &desc, nullptr, &staging ) ) ) return "{\"available\":false}";
        float peak = 0;
        bool finite = true;
        for( UINT face = 0; face < 6; ++face )
        {
            context->CopySubresourceRegion( staging.Get(), face, 0, 0, 0, source.Get(), face * sourceMips, nullptr );
            D3D11_MAPPED_SUBRESOURCE mapped{};
            if( FAILED( context->Map( staging.Get(), face, D3D11_MAP_READ, 0, &mapped ) ) ) return "{\"available\":false}";
            for( UINT y = 0; y < desc.Height; ++y )
            {
                const auto row = reinterpret_cast<const float *>( static_cast<const unsigned char *>( mapped.pData ) + y * mapped.RowPitch );
                for( UINT x = 0; x < desc.Width; ++x ) for( UINT channel = 0; channel < 3; ++channel )
                {
                    const auto value = row[x * 4 + channel];
                    finite = finite && std::isfinite( value ) && value >= 0;
                    if( std::isfinite( value ) ) peak = std::max( peak, value );
                }
            }
            context->Unmap( staging.Get(), face );
        }
        valid = finite && peak > 1;
        std::ostringstream report;
        report << "{\"available\":true,\"format\":" << unsigned( srv.Format ) << ",\"width\":" << desc.Width
               << ",\"height\":" << desc.Height << ",\"mips\":" << sourceMips << ",\"finite\":"
               << ( finite ? "true" : "false" ) << ",\"peakRadiance\":" << peak << ",\"hdrPresent\":"
               << ( valid ? "true" : "false" ) << "}";
        return report.str();
#else
        return "null";
#endif
    }

    std::string captureLinearSceneEvidence( void *textureView, bool &valid )
    {
        valid = false;
#if WP_GRAPHICS_SYSTEM_CLAW && defined( _WIN32 )
        using Microsoft::WRL::ComPtr;
        if( !textureView ) return "{\"available\":false}";
        auto view = static_cast<ID3D11ShaderResourceView *>( textureView );
        D3D11_SHADER_RESOURCE_VIEW_DESC srv{}; view->GetDesc( &srv );
        if( srv.ViewDimension != D3D11_SRV_DIMENSION_TEXTURE2D ||
            srv.Format != DXGI_FORMAT_R16G16B16A16_FLOAT || srv.Texture2D.MostDetailedMip != 0 )
            return "{\"available\":false}";
        ComPtr<ID3D11Resource> resource; view->GetResource( &resource );
        ComPtr<ID3D11Texture2D> source;
        if( FAILED( resource.As( &source ) ) ) return "{\"available\":false}";
        D3D11_TEXTURE2D_DESC desc{}; source->GetDesc( &desc );
        if( !desc.Width || !desc.Height || desc.Width > 8192 || desc.Height > 8192 ||
            desc.ArraySize != 1 || desc.SampleDesc.Count != 1 ) return "{\"available\":false}";
        ComPtr<ID3D11Device> device; source->GetDevice( &device );
        auto graphics = core::IApplicationManager::instance()->getGraphicsSystem();
        auto renderer = dynamic_cast<render::ClawRendererDX11 *>( graphics->getRendererPtr() );
        if( !renderer || device.Get() != wp_renderer_get_dx11_device( renderer->getNativeRenderer() ) )
            return "{\"available\":false}";
        ComPtr<ID3D11DeviceContext> context; device->GetImmediateContext( &context );
        desc.Usage = D3D11_USAGE_STAGING; desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ; desc.MiscFlags = 0;
        ComPtr<ID3D11Texture2D> staging;
        if( FAILED( device->CreateTexture2D( &desc, nullptr, &staging ) ) ) return "{\"available\":false}";
        context->CopyResource( staging.Get(), source.Get() );
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if( FAILED( context->Map( staging.Get(), 0, D3D11_MAP_READ, 0, &mapped ) ) )
            return "{\"available\":false}";
        const auto half = []( u16 bits ) {
            const int exponent = ( bits >> 10 ) & 31, fraction = bits & 1023;
            const float value = exponent == 0 ? std::ldexp( float( fraction ), -24 ) :
                exponent == 31 ? ( fraction ? std::numeric_limits<float>::quiet_NaN() :
                    std::numeric_limits<float>::infinity() ) : std::ldexp( float( fraction + 1024 ), exponent - 25 );
            return bits & 0x8000 ? -value : value;
        };
        bool finite = true; float peak = 0;
        for( UINT y = 0; y < desc.Height; ++y )
        {
            const auto row = reinterpret_cast<const u16 *>( static_cast<const u8 *>( mapped.pData ) + y * mapped.RowPitch );
            for( UINT x = 0; x < desc.Width; ++x ) for( UINT channel = 0; channel < 3; ++channel )
            {
                const auto value = half( row[x * 4 + channel] );
                finite = finite && std::isfinite( value ) && value >= 0;
                if( std::isfinite( value ) ) peak = std::max( peak, value );
            }
        }
        context->Unmap( staging.Get(), 0 );
        valid = finite;
        std::ostringstream report;
        report << "{\"available\":true,\"format\":\"RGBA16F\",\"width\":" << desc.Width
               << ",\"height\":" << desc.Height << ",\"finite\":" << ( finite ? "true" : "false" )
               << ",\"peakRadiance\":" << peak << ",\"radianceAboveOne\":" << ( peak > 1 ? "true" : "false" ) << "}";
        return report.str();
#else
        return "null";
#endif
    }

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
