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
            std::printf( "%s depth readback: %u samples, conservative 513x259 -> 256x256: %s\n",
                warp ? "WARP" : "hardware", samples, ok ? "passed" : "FAILED" );
            native.context->OMSetRenderTargets( 0, nullptr, nullptr );
            release( dsv ); release( native.depth );
        }
    }
    release( native.context ); release( native.device );
    return ok ? 0 : 1;
}
