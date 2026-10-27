#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawDepthReadback.hpp>
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_renderer_software.h>
#include <algorithm>
#include <cstring>
#include <cmath>
#if defined WP_PLATFORM_WIN32 && WP_BUILD_RENDERER_DX11
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#endif

namespace workphone::render
{
    struct ClawDepthReadback::Native
    {
#if defined WP_PLATFORM_WIN32 && WP_BUILD_RENDERER_DX11
        ID3D11Device *device = nullptr;
        ID3D11ComputeShader *single = nullptr, *multiple = nullptr;
        ID3D11Texture2D *output = nullptr, *staging = nullptr;
        ID3D11UnorderedAccessView *uav = nullptr;
        ID3D11Buffer *constants = nullptr;
        unsigned width = 0, height = 0;
        template<class T> static void release( T *&value ) { if( value ) value->Release(); value = nullptr; }
        ~Native()
        {
            release( constants ); release( uav ); release( output ); release( staging );
            release( single ); release( multiple ); release( device );
        }
        bool shader( bool multisample, ID3D11ComputeShader **result )
        {
            auto &cached = multisample ? multiple : single;
            if( cached ) { *result = cached; return true; }
            static const char source[] = R"(
                cbuffer Sizes : register(b0) { uint sourceWidth, sourceHeight, width, height; }
                #if MULTISAMPLE
                Texture2DMS<float> source : register(t0);
                #else
                Texture2D<float> source : register(t0);
                #endif
                RWTexture2D<float> output : register(u0);
                [numthreads(8,8,1)] void main(uint3 thread : SV_DispatchThreadID) {
                    if(thread.x>=width || thread.y>=height) return;
                    uint2 begin=uint2(thread.xy*uint2(sourceWidth,sourceHeight)/uint2(width,height));
                    uint2 end=uint2(((thread.xy+1)*uint2(sourceWidth,sourceHeight)+uint2(width,height)-1)/uint2(width,height));
                    float farthest=0;
                    #if MULTISAMPLE
                    uint sw,sh,samples; source.GetDimensions(sw,sh,samples);
                    #endif
                    for(uint y=begin.y;y<min(end.y,sourceHeight);++y)
                    for(uint x=begin.x;x<min(end.x,sourceWidth);++x) {
                        #if MULTISAMPLE
                        for(uint sampleIndex=0;sampleIndex<samples;++sampleIndex) farthest=max(farthest,source.Load(int2(x,y),sampleIndex));
                        #else
                        farthest=max(farthest,source.Load(int3(x,y,0)));
                        #endif
                    }
                    output[thread.xy]=farthest;
                }
            )";
            const D3D_SHADER_MACRO macros[] = { { "MULTISAMPLE", multisample ? "1" : "0" }, { nullptr, nullptr } };
            ID3DBlob *code = nullptr, *errors = nullptr;
            const auto compiled = D3DCompile( source, sizeof( source ) - 1, "OcclusionDepth", macros,
                nullptr, "main", "cs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &code, &errors );
            release( errors );
            if( FAILED( compiled ) ) { release( code ); return false; }
            const auto created = device->CreateComputeShader( code->GetBufferPointer(), code->GetBufferSize(), nullptr, &cached );
            release( code );
            *result = cached;
            return SUCCEEDED( created );
        }
        bool targets( unsigned w, unsigned h )
        {
            if( w == width && h == height && output && staging && uav && constants ) return true;
            release( uav ); release( output ); release( staging ); release( constants );
            width = height = 0;
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = w; desc.Height = h; desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
            desc.Format = DXGI_FORMAT_R32_FLOAT; desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
            if( FAILED( device->CreateTexture2D( &desc, nullptr, &output ) ) ||
                FAILED( device->CreateUnorderedAccessView( output, nullptr, &uav ) ) ) return false;
            desc.Usage = D3D11_USAGE_STAGING; desc.BindFlags = 0; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            if( FAILED( device->CreateTexture2D( &desc, nullptr, &staging ) ) ) return false;
            D3D11_BUFFER_DESC buffer = {};
            buffer.ByteWidth = 16; buffer.Usage = D3D11_USAGE_DEFAULT; buffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            if( FAILED( device->CreateBuffer( &buffer, nullptr, &constants ) ) ) return false;
            width = w; height = h;
            return true;
        }
#endif
    };
    ClawDepthReadback::ClawDepthReadback() = default;
    ClawDepthReadback::~ClawDepthReadback() = default;

    ClawDepthReadback::Surface ClawDepthReadback::surface( wp_renderer *renderer ) const
    {
        if( !renderer ) return {};
        if( auto software = wp_renderer_get_software( renderer ) )
        {
            const auto w = wp_renderer_software_get_width( software ), h = wp_renderer_software_get_height( software );
            const auto viewport = wp_renderer_get_viewport( renderer );
            if( viewport.x < 0 || viewport.y < 0 || viewport.width <= 0 || viewport.height <= 0 ||
                viewport.x + viewport.width > w || viewport.y + viewport.height > h ) return {};
            return { unsigned( w ), unsigned( h ), unsigned( viewport.x ), unsigned( viewport.y ),
                     unsigned( viewport.width ), unsigned( viewport.height ), 1 };
        }
#if defined WP_PLATFORM_WIN32 && WP_BUILD_RENDERER_DX11
        if( auto dx11 = wp_renderer_get_dx11( renderer ) )
            if( auto texture = static_cast<ID3D11Texture2D *>( wp_renderer_dx11_get_scene_depth_texture( dx11 ) ) )
            {
                D3D11_TEXTURE2D_DESC desc; texture->GetDesc( &desc );
                return { desc.Width, desc.Height, 0, 0, desc.Width, desc.Height, desc.SampleDesc.Count };
            }
#endif
        return {};
    }

    bool ClawDepthReadback::read( wp_renderer *renderer, std::vector<float> &depth,
                                 unsigned &width, unsigned &height )
    {
        const auto dimensions = surface( renderer );
        if( !dimensions[4] || !dimensions[5] ) return false;
        width = std::min( dimensions[4], 256u ); height = std::min( dimensions[5], 256u );
        depth.assign( size_t( width ) * height, 1.0f );
        if( auto software = wp_renderer_get_software( renderer ) )
        {
            const auto source = wp_renderer_software_get_depth_buffer( software );
            if( !source ) return false;
            for( unsigned y = 0; y < height; ++y ) for( unsigned x = 0; x < width; ++x )
            {
                float farthest = 0;
                const auto yEnd = ( ( y + 1 ) * dimensions[5] + height - 1 ) / height;
                const auto xEnd = ( ( x + 1 ) * dimensions[4] + width - 1 ) / width;
                for( auto sy = y * dimensions[5] / height; sy < yEnd; ++sy )
                    for( auto sx = x * dimensions[4] / width; sx < xEnd; ++sx )
                    {
                        const auto sample = source[size_t( sy + dimensions[3] ) * dimensions[0] + sx + dimensions[2]];
                        farthest = std::max( farthest, std::isfinite( sample ) && sample >= 0 && sample <= 1 ? sample : 1.0f );
                    }
                depth[size_t( y ) * width + x] = farthest;
            }
            return true;
        }
#if defined WP_PLATFORM_WIN32 && WP_BUILD_RENDERER_DX11
        auto dx11 = wp_renderer_get_dx11( renderer );
        auto device = dx11 ? static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( dx11 ) ) : nullptr;
        auto context = dx11 ? static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( dx11 ) ) : nullptr;
        auto texture = dx11 ? static_cast<ID3D11Texture2D *>( wp_renderer_dx11_get_scene_depth_texture( dx11 ) ) : nullptr;
        if( !device || !context || !texture ) return false;
        if( !m_native || m_native->device != device )
        {
            m_native = std::make_unique<Native>(); m_native->device = device; device->AddRef();
        }
        ID3D11ComputeShader *shader = nullptr;
        if( !m_native->targets( width, height ) || !m_native->shader( dimensions[6] > 1, &shader ) ) return false;
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        srvDesc.ViewDimension = dimensions[6] > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        ID3D11ShaderResourceView *srv = nullptr;
        if( FAILED( device->CreateShaderResourceView( texture, &srvDesc, &srv ) ) ) return false;
        ID3D11RenderTargetView *oldTargets[8] = {};
        ID3D11DepthStencilView *oldDepth = nullptr;
        ID3D11ComputeShader *oldShader = nullptr;
        ID3D11ClassInstance *instances[256] = {};
        UINT instanceCount = 256;
        ID3D11ShaderResourceView *oldSrv = nullptr;
        ID3D11UnorderedAccessView *oldUav = nullptr;
        ID3D11Buffer *oldConstants = nullptr;
        context->OMGetRenderTargets( 8, oldTargets, &oldDepth );
        context->CSGetShader( &oldShader, instances, &instanceCount );
        context->CSGetShaderResources( 0, 1, &oldSrv );
        context->CSGetUnorderedAccessViews( 0, 1, &oldUav );
        context->CSGetConstantBuffers( 0, 1, &oldConstants );
        context->OMSetRenderTargets( 0, nullptr, nullptr );
        const unsigned sizes[] = { dimensions[0], dimensions[1], width, height };
        context->UpdateSubresource( m_native->constants, 0, nullptr, sizes, 0, 0 );
        context->CSSetShader( shader, nullptr, 0 );
        context->CSSetShaderResources( 0, 1, &srv );
        context->CSSetUnorderedAccessViews( 0, 1, &m_native->uav, nullptr );
        context->CSSetConstantBuffers( 0, 1, &m_native->constants );
        context->Dispatch( ( width + 7 ) / 8, ( height + 7 ) / 8, 1 );
        ID3D11UnorderedAccessView *none = nullptr;
        context->CSSetUnorderedAccessViews( 0, 1, &none, nullptr );
        context->CopyResource( m_native->staging, m_native->output );
        context->CSSetShaderResources( 0, 1, &oldSrv );
        context->CSSetUnorderedAccessViews( 0, 1, &oldUav, nullptr );
        context->CSSetConstantBuffers( 0, 1, &oldConstants );
        context->CSSetShader( oldShader, instances, instanceCount );
        context->OMSetRenderTargets( 8, oldTargets, oldDepth );
        Native::release( srv ); Native::release( oldSrv ); Native::release( oldUav );
        Native::release( oldShader ); Native::release( oldConstants ); Native::release( oldDepth );
        for( auto &target : oldTargets ) Native::release( target );
        for( UINT i = 0; i < instanceCount; ++i ) Native::release( instances[i] );
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if( FAILED( context->Map( m_native->staging, 0, D3D11_MAP_READ, 0, &mapped ) ) ) return false;
        for( unsigned y = 0; y < height; ++y )
            std::memcpy( depth.data() + size_t( y ) * width, static_cast<const char *>( mapped.pData ) + size_t( y ) * mapped.RowPitch, width * sizeof( float ) );
        context->Unmap( m_native->staging, 0 );
        return true;
#else
        return false;
#endif
    }
}
