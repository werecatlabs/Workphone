#undef WP_USE_PRECOMPILED_HEADERS
#define WP_USE_PRECOMPILED_HEADERS 0
// Compile the production reader in this device-only executable without the
// application PCH or DLL startup; the engine target compiles the same source.
#include "../../../Engine/cpp/Source/WPGraphics/ClawDepthReadback.cpp"
#include <WorkphoneGraphics/workphone_graphics_renderer.h>
#include <WorkphoneGraphics/workphone_graphics_renderer_software.h>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

// The renderer adapter is a borrowed device/context/depth resource. All shader
// compilation, multisample resolve, GPU execution and staging map are production
// ClawDepthReadback code against the actual D3D11 device.
struct wp_renderer_dx11 { ID3D11Device *device; ID3D11DeviceContext *context; ID3D11Texture2D *depth; };
struct wp_renderer { wp_renderer_dx11 *native; };
extern "C" {
wp_renderer_software *wp_renderer_get_software( const wp_renderer * ) { return nullptr; }
wp_renderer_dx11 *wp_renderer_get_dx11( const wp_renderer *r ) { return r ? r->native : nullptr; }
wp_viewport_i wp_renderer_get_viewport( const wp_renderer * ) { return {}; }
wp_s32 wp_renderer_software_get_width( const wp_renderer_software * ) { return 0; }
wp_s32 wp_renderer_software_get_height( const wp_renderer_software * ) { return 0; }
const wp_f32 *wp_renderer_software_get_depth_buffer( const wp_renderer_software * ) { return nullptr; }
void *wp_renderer_dx11_get_device( const wp_renderer_dx11 *r ) { return r->device; }
void *wp_renderer_dx11_get_context( const wp_renderer_dx11 *r ) { return r->context; }
void *wp_renderer_dx11_get_scene_depth_texture( wp_renderer_dx11 *r ) { return r->depth; }
}
template<class T> void release( T *&p ) { if( p ) p->Release(); p = nullptr; }
bool drawDepthPattern( wp_renderer_dx11 &native, unsigned width, unsigned height, unsigned samples )
{
    static const char shader[] = R"(
        float4 vs(uint index:SV_VertexID):SV_Position {
            return float4(index==2?3:-1,index==1?3:-1,0,1);
        }
        float ps(float4 pixel:SV_Position):SV_Depth {
            return pixel.x>=256 ? .95 : .75;
        }
    )";
    ID3DBlob *vsCode = nullptr, *psCode = nullptr, *errors = nullptr;
    ID3D11VertexShader *vs = nullptr;
    ID3D11PixelShader *ps = nullptr;
    ID3D11DepthStencilState *depth = nullptr;
    ID3D11RasterizerState *raster = nullptr;
    D3D11_DEPTH_STENCIL_DESC depthDesc{};
    depthDesc.DepthEnable = TRUE; depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    D3D11_RASTERIZER_DESC rasterDesc{};
    rasterDesc.FillMode = D3D11_FILL_SOLID; rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthClipEnable = TRUE; rasterDesc.MultisampleEnable = TRUE;
    const bool ok = SUCCEEDED( D3DCompile( shader, sizeof( shader ) - 1, nullptr, nullptr, nullptr,
        "vs", "vs_5_0", 0, 0, &vsCode, &errors ) ) &&
        SUCCEEDED( D3DCompile( shader, sizeof( shader ) - 1, nullptr, nullptr, nullptr,
        "ps", "ps_5_0", 0, 0, &psCode, &errors ) ) &&
        SUCCEEDED( native.device->CreateVertexShader( vsCode->GetBufferPointer(), vsCode->GetBufferSize(), nullptr, &vs ) ) &&
        SUCCEEDED( native.device->CreatePixelShader( psCode->GetBufferPointer(), psCode->GetBufferSize(), nullptr, &ps ) ) &&
        SUCCEEDED( native.device->CreateDepthStencilState( &depthDesc, &depth ) ) &&
        SUCCEEDED( native.device->CreateRasterizerState( &rasterDesc, &raster ) );
    if( ok )
    {
        D3D11_VIEWPORT viewport{ 0, 0, float( width ), float( height ), 0, 1 };
        native.context->RSSetViewports( 1, &viewport );
        native.context->RSSetState( raster );
        native.context->OMSetDepthStencilState( depth, 0 );
        native.context->OMSetBlendState( nullptr, nullptr, samples > 1 ? 2u : 1u );
        native.context->VSSetShader( vs, nullptr, 0 );
        native.context->PSSetShader( ps, nullptr, 0 );
        native.context->IASetInputLayout( nullptr );
        native.context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
        native.context->Draw( 3, 0 );
    }
    if( !ok ) std::fprintf( stderr, "Depth pattern setup failed: %s\n", errors ? static_cast<const char *>( errors->GetBufferPointer() ) : "D3D state creation" );
    release( errors ); release( vsCode ); release( psCode ); release( vs ); release( ps ); release( depth ); release( raster );
    return ok;
}
int main()
{
    wp_renderer_dx11 native{};
    const bool warp = std::getenv( "WP_DEPTH_TEST_WARP" ) != nullptr;
    auto driver = warp ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE;
    D3D_FEATURE_LEVEL level;
    if( FAILED( D3D11CreateDevice( nullptr, driver, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &native.device, &level, &native.context ) ) ) return 77;
    if( level < D3D_FEATURE_LEVEL_11_0 ) { release( native.context ); release( native.device ); return 77; }
    bool ok = true;
    {
        workphone::render::ClawDepthReadback reader;
        wp_renderer renderer{ &native };
        for( unsigned samples : { 1u, 4u } )
        {
            UINT quality = 0;
            native.device->CheckMultisampleQualityLevels( DXGI_FORMAT_D24_UNORM_S8_UINT, samples, &quality );
            if( !quality ) continue;
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = 513; desc.Height = 259; desc.MipLevels = desc.ArraySize = 1;
            desc.SampleDesc.Count = samples; desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
            desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
            ID3D11DepthStencilView *dsv = nullptr;
            D3D11_DEPTH_STENCIL_VIEW_DESC view{};
            view.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            view.ViewDimension = samples > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
            ok = ok && SUCCEEDED( native.device->CreateTexture2D( &desc, nullptr, &native.depth ) ) &&
                SUCCEEDED( native.device->CreateDepthStencilView( native.depth, &view, &dsv ) );
            if( !dsv ) break;
            native.context->OMSetRenderTargets( 0, nullptr, dsv );
            for( float expected : { .375f, 1.f } )
            {
                native.context->ClearDepthStencilView( dsv, D3D11_CLEAR_DEPTH, expected, 0 );
                std::vector<float> depth;
                unsigned width = 0, height = 0;
                ok = reader.read( &renderer, depth, width, height ) && ok;
                ok = ok && width == 256 && height == 256 && depth.size() == size_t( width ) * height &&
                    std::all_of( depth.begin(), depth.end(), [=]( float z ) { return std::abs( z - expected ) < .00001f; } );
                ID3D11DepthStencilView *restored = nullptr;
                native.context->OMGetRenderTargets( 0, nullptr, &restored );
                ok = ok && restored == dsv;
                release( restored );
            }
            native.context->ClearDepthStencilView( dsv, D3D11_CLEAR_DEPTH, .25f, 0 );
            ok = drawDepthPattern( native, desc.Width, desc.Height, samples ) && ok;
            std::vector<float> patterned;
            unsigned width = 0, height = 0;
            ok = reader.read( &renderer, patterned, width, height ) && ok;
            if( width != 256 || height != 256 ) ok = false;
            else for( unsigned y = 0; y < height; ++y ) for( unsigned x = 0; x < width; ++x )
            {
                const auto right = ( ( x + 1 ) * desc.Width + width - 1 ) / width;
                const float expected = right > 256 ? .95f : .75f;
                if( std::abs( patterned[size_t( y ) * width + x] - expected ) >= .00001f )
                {
                    if( ok ) std::fprintf( stderr, "Depth mismatch %u,%u: %.6f expected %.6f\n", x, y,
                        patterned[size_t( y ) * width + x], expected );
                    ok = false;
                }
            }
            std::printf( "%s depth readback: %u samples, conservative 513x259 -> 256x256: %s\n",
                warp ? "WARP" : "hardware", samples, ok ? "passed" : "FAILED" );
            native.context->OMSetRenderTargets( 0, nullptr, nullptr );
            native.context->OMSetBlendState( nullptr, nullptr, ~0u );
            release( dsv ); release( native.depth );
        }
    }
    release( native.context ); release( native.device );
    return ok ? 0 : 1;
}
