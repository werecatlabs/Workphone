#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/Particle/CParticleSystem.hpp>
#include <Workphone/Graphics/GraphicsWindow.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <workphone_graphics_mesh.h>
#include <workphone_graphics_renderer.h>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <cstdio>
#include <cstring>
#include <cmath>

using namespace workphone;
using namespace workphone::render;

namespace
{
    bool check( bool condition, const char *message )
    {
        if( !condition ) std::fprintf( stderr, "FAIL: %s\n", message );
        return condition;
    }

    class TestWindow : public GraphicsWindow
    {
    public:
        HWND handle = nullptr;
        Vector2I getSize() const override { return {128,128}; }
        void getWindowHandle( void *data ) override { *static_cast<HWND *>(data) = handle; }
        void _getObject( void **data ) const override { *data = nullptr; }
    };

    class TestMesh : public ClawMesh
    {
    public:
        TestMesh()
        {
            m_mesh = wp_graphics_mesh_create();
            const wp_graphics_mesh_vertex_pnt vertices[] = {
                { {-0.2f,-0.2f,0.5f}, {0,0,-1}, {0,0} },
                { {0.2f,-0.2f,0.5f}, {0,0,-1}, {1,0} },
                { {0,0.2f,0.5f}, {0,0,-1}, {0.5f,1} }
            };
            wp_graphics_mesh_set_vertices( m_mesh, WORKPHONE_VERTEX_FORMAT_PNT, vertices, 3 );
            const wp_u16 indices[] = {0,1,2};
            wp_graphics_mesh_set_indices_u16( m_mesh, indices, 3 );
        }
    };

    bool hasPixel( wp_renderer_dx11 *native, int x, int y, bool redOnly )
    {
        auto *device = static_cast<ID3D11Device *>( wp_renderer_dx11_get_device(native) );
        auto *context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context(native) );
        ID3D11RenderTargetView *rtv = nullptr;
        context->OMGetRenderTargets( 1, &rtv, nullptr );
        if( !rtv ) return check(false, "render target must be bound");
        ID3D11Resource *resource = nullptr;
        rtv->GetResource(&resource);
        ID3D11Texture2D *source = nullptr;
        resource->QueryInterface( IID_PPV_ARGS(&source) );
        resource->Release(); rtv->Release();
        if( !source ) return false;
        D3D11_TEXTURE2D_DESC desc{};
        source->GetDesc(&desc);
        desc.Usage = D3D11_USAGE_STAGING; desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ID3D11Texture2D *staging = nullptr;
        const bool allocated = SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &staging));
        bool found = false;
        if( allocated )
        {
            context->CopyResource(staging, source);
            D3D11_MAPPED_SUBRESOURCE mapped{};
            if( SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)) )
            {
                const auto *pixel = static_cast<const unsigned char *>(mapped.pData) + y * mapped.RowPitch + x * 4;
                const int red = desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM ? 2 : 0;
                found = redOnly ? pixel[red] > 150 && pixel[1] < 20 : pixel[0] + pixel[1] + pixel[2] > 30;
                context->Unmap(staging, 0);
            }
            staging->Release();
        }
        source->Release();
        return found;
    }

    bool testParticleLifecycle()
    {
        CParticleSystem effect;
        effect.setSeed(42); effect.setPoolSize(32);
        wp_particle_simulation_settings settings;
        wp_particle_simulation_default_settings(&settings);
        settings.rate = 20; settings.lifetime_min = settings.lifetime_max = 2;
        bool ok = check(effect.setSimulationSettings(settings), "effect settings must validate");
        effect.load( SmartPtr<ISharedObject>() );
        ok &= check(effect.isLoaded() && effect.getNumEmitters() == 1, "virtual load must initialize the real Claw simulation");
        effect.setState(ParticleSystemState::Started);
        ok &= check(effect.simulate(0.5f) && effect.getNumParticles() == 10, "Claw wrapper must emit ten particles");
        const auto snapshot = effect.getRenderSnapshot();
        effect.setState(ParticleSystemState::Paused);
        effect.simulate(0.5f);
        const auto paused = effect.getRenderSnapshot();
        ok &= check(snapshot.size() == paused.size() &&
            std::memcmp(snapshot.data(), paused.data(), snapshot.size()*sizeof(wp_particle_sample)) == 0,
            "paused effects must retain an identical snapshot");
        effect.setState(ParticleSystemState::StoppedFade);
        effect.simulate(1); effect.simulate(1); effect.simulate(1);
        ok &= check(effect.getNumParticles() == 0, "stop-fade must drain");
        effect.unload( SmartPtr<ISharedObject>() );
        ok &= check(!effect.isLoaded() && effect.getNumEmitters() == 0 && !effect.simulate(0.1f), "unload must release simulation ownership");
        return ok;
    }

    bool testRenderedFeatures( ClawRendererDX11 &renderer )
    {
        auto target = make_ptr<ClawRenderTarget>(); target->setSize({64,64});
        renderer.setRenderTarget(target); renderer.setViewport(nullptr); renderer.setCamera(nullptr);
        auto *native = wp_renderer_get_dx11(renderer.getNativeRenderer());
        Array<wp_particle_sample> samples(1);
        std::memset(samples.data(),0,sizeof(wp_particle_sample));
        samples[0].position = {0,0,0.5f}; samples[0].size = 0.5f;
        samples[0].color[0] = samples[0].color[3] = 1;
        renderer.clear(ColourF::Black);
        renderer.renderParticles(samples, Matrix4F::identity(), Vector3F(1,1,1));
        bool ok = check(hasPixel(native,32,32,true), "particle snapshot must produce red pixels on DX11");
        TestMesh mesh;
        Array<wp_skin_vertex> vertices(3);
        std::memset(vertices.data(),0,vertices.size()*sizeof(wp_skin_vertex));
        const auto *source = static_cast<const wp_graphics_mesh_vertex_pnt *>(wp_graphics_mesh_get_vertices(mesh.getNativeMesh()));
        for( u32 i=0; i<3; ++i )
        {
            vertices[i].position=source[i].position; vertices[i].normal=source[i].normal;
            vertices[i].weights[0]=1; vertices[i].joints[0]=1;
        }
        ok &= check(mesh.setSkinningData(vertices), "Claw mesh must accept validated skinning data");
        Array<wp_mat4f> palette(2);
        for( auto &matrix : palette )
        {
            std::memset(&matrix,0,sizeof(matrix));
            for( u32 i=0; i<4; ++i ) matrix.m[i][i]=1;
        }
        palette[1].m[0][3]=0.5f;
        ok &= check(mesh.applySkinningPalette(palette), "pose palette must deform native mesh vertices");
        source=static_cast<const wp_graphics_mesh_vertex_pnt *>(wp_graphics_mesh_get_vertices(mesh.getNativeMesh()));
        ok &= check(std::abs(source[0].position.x-0.3f)<1e-5f, "joint translation must reach the native mesh");
        renderer.setSceneLighting(ColourF::White, Vector3F(0,-1,0), ColourF::White, 0);
        renderer.clear(ColourF::Black);
        renderer.renderMesh(&mesh, Matrix4F::identity());
        ok &= check(hasPixel(native,48,32,false), "deformed mesh must draw at its translated position");
        renderer.setRenderTarget(nullptr);
        return ok;
    }
}

int main()
{
    TypeManager types; types.load(); TypeManager::setInstance(&types);
    auto application = make_ptr<core::ApplicationManager>();
    core::IApplicationManager::setInstance(application);
    bool ok = testParticleLifecycle();
    auto window = make_ptr<TestWindow>();
    window->handle = CreateWindowExW(0,L"STATIC",L"Claw production regression",WS_OVERLAPPEDWINDOW,
        0,0,128,128,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if( window->handle )
    {
        ClawRendererDX11 renderer; renderer.load(window);
        ok &= check(renderer.isLoaded(), "required DX11 backend must initialize; this test cannot silently skip");
        if( renderer.isLoaded() ) ok &= testRenderedFeatures(renderer);
        renderer.unload(nullptr);
        DestroyWindow(window->handle);
    }
    else ok = check(false,"hidden window must initialize");
    window=nullptr;
    core::IApplicationManager::setInstance(nullptr); application=nullptr;
    TypeManager::setInstance(nullptr); types.unload();
    return ok ? 0 : 1;
}
