#include "ClawFoliageContracts.hpp"
#include "ClawPrimaryLightContracts.hpp"
#include "ClawFoliageSceneContracts.hpp"
#include "ClawSceneLifetimeContracts.hpp"
#include "ClawFoliageWorldContracts.hpp"
#include "ClawAnimationContracts.hpp"
#include "ClawMeshBudgetContracts.hpp"
#include "ClawMaterialReloadContracts.hpp"
#include "ClawShaderFileWatchContracts.hpp"
#include "ClawShaderSchemaContracts.hpp"
#include "ClawShaderDeviceContracts.hpp"
#include "ClawRTTJobContracts.hpp"
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Graphics/MaterialShader.hpp>
#include <Workphone/Graphics/MaterialPublicationScope.hpp>
#include <Workphone/Interface/IBuildDirector.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/ClawLight.hpp>
#include <WPGraphics/ClawCamera.hpp>
#include <WPGraphics/ClawSky.hpp>
#include <WPGraphics/Particle/CParticleSystem.hpp>
#include <Workphone/Graphics/GraphicsWindow.hpp>
#include <Workphone/Graphics/GraphicsSceneNode.hpp>
#include <Workphone/Scene/Components/Light.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <workphone_graphics_mesh.h>
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_light.h>
#include <workphone_graphics_scenenode.h>
#include <workphone_graphics_object.h>
#include <workphone_graphics_camera.h>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include "DX11TestEvidence.hpp"
#include "ClawSceneNodeTransformContracts.hpp"
#include "ClawMaterialPassLifetimeContracts.hpp"
#include "GraphicsCrashEvidence.hpp"
#include "ClawTerrainContracts.hpp"
#include "ClawTerrainMaterialContracts.hpp"
#include "ClawTerrainStreamingContracts.hpp"
#include "ClawTerrainMotionContracts.hpp"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <array>
#include <vector>
#include <algorithm>
#include <chrono>
#include <new>

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
        mutable u64 materialQueries = 0;
        SmartPtr<IMaterial> getMaterial( s32 index = -1 ) const override
        { ++materialQueries; return ClawMesh::getMaterial( index ); }
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

    class MaterialStorageScene : public ClawScene
    {
    public:
        std::shared_ptr<void> retainJobs() const { return m_nativeRenderJobs; }
        u64 retainedBytes() const
        { return m_materialRecordWorkspace ? m_materialRecordWorkspace->getRetainedStorageBytes() : 0; }
        u64 retainedAllocations() const
        { return m_materialRecordWorkspace ? m_materialRecordWorkspace->getStorageAllocationCount() : 0; }
    };

    class ProfileGraphics : public claw_rtt_contracts::Graphics
    {
    public:
        void markLoaded() { setLoadingState( LoadingState::Loaded ); }
        void startPublication() { beginSceneProfileFrame(); }
        void finishPublication() { publishSceneProfileFrame(); }
        void abortPublication() { invalidateSceneProfileFrame(); }
    };

    bool testSceneProfile( SmartPtr<IGraphicsWindow> window )
    {
        claw_foliage_scene_contracts::Fixture services;
        auto renderer = make_ptr<claw_rtt_contracts::Renderer>();
        renderer->load( window );
        if( !check( renderer->isLoaded(), "scene profile renderer must initialize" ) ) return false;
        auto target = make_ptr<ClawRenderTarget>();
        target->setSize( { 32, 32 } ); target->setActive( true );
        auto camera = make_ptr<ClawCamera>();
        for( u32 index = 0; index < 2; ++index )
        {
            auto viewport = target->addViewport( 19 + index, camera );
            if( !check( viewport != nullptr, "scene profile multiview fixture" ) ) return false;
            viewport->setActive( true ); viewport->setEnableSceneRender( true );
            viewport->setEnableUI( false );
            viewport->setClearEveryFrame( true, WORKPHONE_CLEAR_FLAG_ALL );
        }
        auto texture = make_ptr<claw_rtt_contracts::Texture>(); texture->output = target;
        auto textures = make_ptr<claw_rtt_contracts::Textures>(); textures->values.push_back( texture );
        ProfileGraphics graphics; graphics.initialise( renderer, textures ); graphics.markLoaded();
        auto scene = make_ptr<ClawScene>(); scene->setEnableShadows( false );
        auto mesh = make_ptr<claw_rtt_contracts::SnapshotMesh>();
        wp_graphics_mesh_compute_aabb( mesh->getNativeMesh() );
        auto object = wp_graphics_scene_create_object( scene->getNativeScene() );
        auto node = wp_graphics_scene_create_node( scene->getNativeScene() );
        wp_scenenode_attach_object( node, object );
        mesh->bindNativeRenderObject( object ); mesh->setVisibilityFlags( ~u32( 0 ) );
        auto material = make_ptr<ClawMaterial>();
        auto technique = make_ptr<ClawMaterialTechnique>();
        technique->setMaterial( material.get() ); technique->createPass();
        material->setTechniques( { technique } ); material->setMaterialType( MaterialType::UI );
        mesh->setMaterial( material );
        graphics.nativeMesh = mesh; graphics.attachScene( scene );

        ClawHammerSystem::SceneProfileSnapshot snapshot;
        snapshot.serial = 99; snapshot.materialRecords = 99;
        bool ok = check( !graphics.getSceneProfilingEnabled() && !graphics.readSceneProfile( snapshot ) &&
                         !snapshot.serial && !snapshot.materialRecords,
                         "scene telemetry is disabled and clears caller output by default" );
        graphics.update();
        ok &= check( graphics.unprofiledPreparations == 2 && !graphics.readSceneProfile( snapshot ),
                     "disabled graphics updates leave preparation timing and readback unavailable" );
        graphics.setSceneProfilingEnabled( true );
        ok &= check( !graphics.readSceneProfile( snapshot ), "enabling telemetry does not publish old frames" );
        graphics.update();
        ok &= check( graphics.readSceneProfile( snapshot ) && snapshot.serial && snapshot.registeredScenes == 1 &&
                     snapshot.scenePreparations == 2 && snapshot.mainViews == 2 && snapshot.shadowViews == 2 &&
                     snapshot.candidates == 2 && snapshot.mainVisible == 2 && snapshot.transformDispatches == 2 &&
                     snapshot.completedPasses == 2 && snapshot.skippedPasses == 1 && !snapshot.failedPasses,
                     "typed telemetry aggregates actual scene/view pairs and completed native passes once" );
        ok &= check( snapshot.dx11Available && snapshot.dx11DrawsLifetime && snapshot.materialRecords == 2 &&
                     snapshot.materialBindings == 2 && snapshot.materialMeshes == 2 &&
                     snapshot.materialStorageReusedViews == 2 && !snapshot.materialStorageAllocations &&
                     snapshot.materialDenseCapacityBytesSum && snapshot.cacheResident == 1 && snapshot.cacheSlots >= 1,
                     "typed telemetry copies warmed material publications and one persistent renderer table" );
        ok &= check( snapshot.graphicsUpdateNs >= snapshot.graphicsPrepareNs + snapshot.nativeFrameNs &&
                     snapshot.preparationTotalNs >= snapshot.mainPrepareNs + snapshot.shadowDeriveNs + snapshot.materialPrepareNs &&
                     snapshot.nativeFrameNs >= snapshot.preparationTotalNs,
                     "typed nested owner timings fit their measured graphics update" );
        ClawScene::MaterialPreparationStatistics retired;
        ok &= check( !scene->getPreparedMaterialStatistics( retired ) && !retired.records && snapshot.materialRecords == 2,
                     "published material telemetry survives native job retirement without retaining jobs" );
        const auto first = snapshot;
        const auto cache = renderer->getMaterialRecordStatistics();
        graphics.resetSceneProfile();
        ok &= check( graphics.getSceneProfilingEnabled() && !graphics.readSceneProfile( snapshot ) && !snapshot.serial &&
                     renderer->getMaterialRecordStatistics().preparations == cache.preparations,
                     "telemetry reset invalidates readback without resetting material owners" );
        graphics.update();
        ok &= check( graphics.readSceneProfile( snapshot ) && snapshot.serial > first.serial &&
                     snapshot.generation > first.generation && snapshot.nativeUpdateSerial > first.nativeUpdateSerial,
                     "a fresh owner update publishes a later serial in the reset generation" );

        // Exercise the same publication guard used when another thread resets
        // controls while a graphics-owner frame is still in flight.
        graphics.startPublication();
        std::thread resetter( [&] { graphics.resetSceneProfile(); } ); resetter.join();
        graphics.finishPublication();
        ok &= check( !graphics.readSceneProfile( snapshot ), "reset rejects an already captured generation" );
        graphics.startPublication(); graphics.setSceneProfilingEnabled( false );
        graphics.setSceneProfilingEnabled( true ); graphics.finishPublication();
        ok &= check( !graphics.readSceneProfile( snapshot ), "disable/re-enable rejects an in-flight old frame" );
        graphics.update(); graphics.startPublication(); graphics.abortPublication();
        ok &= check( !graphics.readSceneProfile( snapshot ), "aborted owner work clears prior completed readback" );
        graphics.fail = true; graphics.update(); graphics.fail = false;
        ok &= check( graphics.readSceneProfile( snapshot ) && snapshot.failedPasses == 2 &&
                     snapshot.scenePreparations == 2,
                     "failed native passes remain explicit in fresh frame diagnostics" );

        graphics.removeScenes();
        graphics.update();
        ok &= check( graphics.readSceneProfile( snapshot ) && !snapshot.registeredScenes &&
                     !snapshot.scenePreparations && !snapshot.materialRecords && !snapshot.preparationTotalNs &&
                     snapshot.completedPasses == 2,
                     "an empty-scene update publishes fresh zero preparation values" );
        graphics.detach();
        graphics.update();
        ok &= check( graphics.readSceneProfile( snapshot ) && !snapshot.dx11Available &&
                     !snapshot.cacheResident && !snapshot.cacheStorageBytes && !snapshot.dx11DrawsLifetime &&
                     !snapshot.completedPasses && !snapshot.scenePreparations,
                     "detaching the renderer clears unavailable owner counters in the next update" );
        graphics.unload( nullptr );
        snapshot.serial = 99; snapshot.dx11DrawsLifetime = 99;
        ok &= check( !graphics.getSceneProfilingEnabled() && !graphics.readSceneProfile( snapshot ) &&
                     !snapshot.serial && !snapshot.dx11DrawsLifetime,
                     "unload disables telemetry and clears every published counter" );
        graphics.setSceneProfilingEnabled( true ); graphics.update();
        ok &= check( !graphics.readSceneProfile( snapshot ), "unloaded updates cannot republish retired frame values" );
        graphics.setSceneProfilingEnabled( false );
        mesh->setMaterial( nullptr ); mesh = nullptr; material = nullptr; technique = nullptr;
        renderer->unload( nullptr );
        return ok;
    }

    class TestLightNode : public GraphicsSceneNode
    {
    public:
        Quaternion<real_Num> orientation = Quaternion<real_Num>::identity();
        void setPosition( const Vector3<real_Num> & ) override {}
        void setScale( const Vector3<real_Num> & ) override {}
        void setOrientation( const Quaternion<real_Num> &value ) override { orientation = value; }
        Quaternion<real_Num> getWorldOrientation() const override { return orientation; }
    };

    bool testLightDirection()
    {
        auto node = make_ptr<TestLightNode>();
        auto light = make_ptr<ClawLight>();
        light->load( nullptr );

        wp_light *nativeLight = nullptr;
        light->_getObject( reinterpret_cast<void **>( &nativeLight ) );
        if( !check( light->isLoaded() && nativeLight,
                    "light direction fixture must load its native light" ) )
            return false;
        light->setOwner( node );
        scene::Light component;
        component.setLight( light );
        component.setSceneNode( node );
        Transform3<real_Num> transform;
        transform.setOrientation( Quaternion<real_Num>::eulerDegrees( -60, -30, 0 ) );
        component.updateTransform( transform );
        const auto expected = transform.getOrientation() * -Vector3<real_Num>::unitY();
        const auto actual = light->getDerivedDirection();
        bool ok = check( ( actual - expected ).lengthSquared() < 1e-6f,
                         "light component rotation must reach WPGraphics exactly once" );
        ok &= check( actual.Y() < -0.1f, "SamplePhysics sun must illuminate the ground from above" );
        // A subsequent transform update must not accumulate another rotation.
        component.updateTransform( transform );
        ok &= check( ( light->getDerivedDirection() - expected ).lengthSquared() < 1e-6f,
                     "repeated light transforms must preserve the derived direction" );
        auto nativeNode = wp_scenenode_create();
        const auto rotation = transform.getOrientation();
        wp_scenenode_set_orientation( nativeNode, { rotation.w, rotation.x, rotation.y, rotation.z } );
        
        nativeLight =nullptr;
        light->_getObject( reinterpret_cast<void **>( &nativeLight ) );
        wp_light_attach_to_node( nativeLight, nativeNode );
        node->orientation = Quaternion<real_Num>::identity(); // Simulate a stale C++ transform cache.
        ok &= check( ( light->getDerivedDirection() - expected ).lengthSquared() < 1e-6f,
                     "attached light must use the current native scene-node rotation" );
        wp_light_detach_from_node( nativeLight, nativeNode );
        wp_scenenode_destroy( nativeNode );
        light->setOwner( nullptr );
        light->unload( nullptr );

        nativeLight = nullptr;
        light->_getObject( reinterpret_cast<void **>( &nativeLight ) );
        ok &= check( !light->isLoaded() && !nativeLight,
                     "light unload must release the native light" );
        return ok;
    }

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

    class MaterialRoutingTexture : public ClawTexture
    {
    public:
        mutable u32 requests = 0;
        void *currentView = nullptr;
        void getTextureFinal( void **view ) const override
        {
            ++requests;
            *view = currentView; // Completion/replacement can change this without a material edit.
        }
    };

    bool testSkyMaterialPublications( ClawRendererDX11 &renderer )
    {
        class CubeMaterial : public ClawMaterial
        {
        public:
            mutable u32 copies = 0;
            Array<SmartPtr<ITexture>> getCubicTextures() const override
            { ++copies; return Material::getCubicTextures(); }
        };
        auto target = make_ptr<ClawRenderTarget>(); target->setSize({64,64});
        auto camera = make_ptr<ClawCamera>();
        camera->setNearClipDistance(0.1f); camera->setFarClipDistance(100); camera->setAspectRatio(1);
        renderer.setRenderTarget(target); renderer.setViewport(nullptr); renderer.setCamera(camera);
        auto sky = make_ptr<ClawSky>();
        auto material = make_ptr<CubeMaterial>();
        auto first = make_ptr<MaterialRoutingTexture>(), second = make_ptr<MaterialRoutingTexture>();
        material->setCubicTextures(Array<SmartPtr<ITexture>>(6, first)); sky->setMaterial(material);
        renderer.clear(ColourF::Black); renderer.renderSky(sky);
        bool ok = check(first->requests > 0 && second->requests == 0 && material->copies == 0,
            "sky material fallback borrows published faces without the copying getter");
        auto retained = material->getCubicTextureSnapshot();
        const auto oldRequests = first->requests;
        material->setCubicTextures(Array<SmartPtr<ITexture>>(6, second)); renderer.renderSky(sky);
        ok &= check(first->requests == oldRequests && second->requests > 0 && (*retained)[0] == first,
            "sky consumes replacement faces while an older publication remains retained");
        const auto newRequests = second->requests;
        sky->setTextures(Array<SmartPtr<ITexture>>(1, first)); renderer.renderSky(sky);
        ok &= check(first->requests > oldRequests && second->requests == newRequests,
            "explicit sky textures retain precedence over material publications");
        sky->setTextures({}); material->setCubicTextures({}); renderer.renderSky(sky);
        ok &= check(second->requests == newRequests && material->copies == 0,
            "empty material publication makes no stale texture requests");
        sky->setMaterial(nullptr); renderer.setCamera(nullptr); renderer.setRenderTarget(nullptr);
        return ok;
    }

    bool testSkyEnvironmentOwnerRetirement( SmartPtr<IGraphicsWindow> window )
    {
        class RetiredSky final : public ClawSky
        {
        public:
            explicit RetiredSky( unsigned &destroyed ) : destroyed( destroyed ) {}
            ~RetiredSky() override { ++destroyed; }
            static void *operator new( size_t bytes )
            {
                auto storage = VirtualAlloc( nullptr, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
                if( !storage ) throw std::bad_alloc();
                return storage;
            }
            static void operator delete( void *storage ) noexcept
            { if( storage ) VirtualFree( storage, 0, MEM_RELEASE ); }
        private:
            unsigned &destroyed;
        };
        struct Face
        {
            ID3D11Texture2D *texture = nullptr;
            ID3D11ShaderResourceView *view = nullptr;
            ~Face() { if( view ) view->Release(); if( texture ) texture->Release(); }
        };

        bool ok = true;
        for( unsigned cycle = 0; cycle < 3; ++cycle )
        {
            ClawRendererDX11 renderer; renderer.load( window );
            if( !check( renderer.isLoaded(), "sky retirement requires the real loaded DX11 renderer" ) ) return false;
            auto target = make_ptr<ClawRenderTarget>(); target->setSize( { 32, 32 } );
            auto camera = make_ptr<ClawCamera>();
            camera->setNearClipDistance( .1f ); camera->setFarClipDistance( 100 ); camera->setAspectRatio( 1 );
            renderer.setRenderTarget( target ); renderer.setViewport( nullptr ); renderer.setCamera( camera );
            auto *native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
            auto *device = static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( native ) );
            Face face; D3D11_TEXTURE2D_DESC description{};
            description.Width = description.Height = description.MipLevels = description.ArraySize = 1;
            description.Format = DXGI_FORMAT_R8G8B8A8_UNORM; description.SampleDesc.Count = 1;
            description.Usage = D3D11_USAGE_IMMUTABLE; description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            const u32 colour = 0xffc08040u + cycle;
            D3D11_SUBRESOURCE_DATA pixels{ &colour, sizeof( colour ), sizeof( colour ) };
            if( !check( SUCCEEDED( device->CreateTexture2D( &description, &pixels, &face.texture ) ) &&
                        SUCCEEDED( device->CreateShaderResourceView( face.texture, nullptr, &face.view ) ),
                        "sky retirement requires an actual GPU face/view" ) ) return false;
            auto texture = make_ptr<MaterialRoutingTexture>(); texture->currentView = face.view;
            unsigned destroyed = 0;
            for( unsigned replacement = 0; replacement < 2; ++replacement )
            {
                // Physical page release makes the former dangling WeakPtr read
                // deterministic, rather than relying on CRT freed-heap reuse.
                auto sky = SmartPtr<RetiredSky>( new RetiredSky( destroyed ) );
#if WP_GARBAGE_COLLECTION == 1
                sky->removeReference(); // Match make_ptr's initial-reference contract.
#endif
                sky->load( nullptr ); sky->setTextures( Array<SmartPtr<ITexture>>( 6, texture ) );
                const auto references = sky->getReferences();
                const auto requests = texture->requests;
                renderer.clear( ColourF::Black ); renderer.renderSky( sky );
                ok &= check( texture->requests >= requests + 6 && sky->getReferences() == references,
                    "rendered sky cache must consume GPU faces without retaining the sky owner" );
                sky = nullptr;
                ok &= check( destroyed == replacement + 1, "sky must be physically destroyed before renderer retirement" );
            }
            renderer.setCamera( nullptr ); renderer.setRenderTarget( nullptr );
            renderer.unload( nullptr ); // Exercises the actual private destroyRenderer cache erase.
            ok &= check( !renderer.isLoaded() && destroyed == 2, "renderer teardown must release an expired sky cache safely" );
            texture->currentView = nullptr;
        }
        return ok;
    }

    bool testIdleMaterialRetirement( ClawRendererDX11 &renderer )
    {
        claw_foliage_scene_contracts::Fixture fixture;
        auto target = make_ptr<ClawRenderTarget>(); target->setSize({64,64});
        renderer.setRenderTarget(target); renderer.setViewport(nullptr);
        const auto baseline = renderer.getMaterialRecordStatistics();
        auto material = make_ptr<ClawMaterial>();
        auto technique = make_ptr<ClawMaterialTechnique>();
        technique->setMaterial(material.get()); technique->createPass();
        material->setTechniques({technique});
        TestMesh mesh; mesh.setMaterial(material);
        ClawMaterialDrawRecords records;
        for(u32 edit = 0; edit < 129; ++edit)
        {
            material->setRoughness(f32(edit) / 129);
            records.capture(&mesh); renderer.prepareMaterialRecords(records); records.clear();
        }
        mesh.setMaterial(nullptr); material->unload(nullptr); material = nullptr; technique = nullptr;
        const auto pending = renderer.getMaterialRecordStatistics();
        const auto frames = (pending.slots + 31) / 32 + 4;
        for(u32 frame = 0; frame < frames; ++frame)
        {
            renderer.beginRender(); renderer.endRender();
        }
        const auto retired = renderer.getMaterialRecordStatistics();
        const bool ok = check(retired.resident <= baseline.resident && retired.resident < pending.resident &&
            retired.preparations == pending.preparations && retired.storageAllocations == pending.storageAllocations,
            "idle frame boundaries retire expired material records without new draws, preparation or allocation");
        renderer.setRenderTarget(nullptr);
        return ok;
    }

    bool testRuntimeMaterialProgram(ClawRendererDX11 &renderer)
    {
        using Microsoft::WRL::ComPtr;
        claw_foliage_scene_contracts::Fixture fixture;
        auto target = make_ptr<ClawRenderTarget>(); target->setSize({64,64});
        renderer.setRenderTarget(target); renderer.setCamera(nullptr); renderer.setViewport(nullptr);
        renderer.disableShadows();
        struct Restore { ClawRendererDX11 &r; ~Restore() { r.setRenderTarget(nullptr); } } restore{renderer};
        auto native = wp_renderer_get_dx11(renderer.getNativeRenderer());
        auto device = static_cast<ID3D11Device *>(wp_renderer_dx11_get_device(native));
        const auto compile = [&](const char *source, const char *profile) {
            ComPtr<ID3DBlob> code, errors;
            const auto hr = D3DCompile(source, std::strlen(source), "RuntimeMaterialContract", nullptr, nullptr,
                "main", profile, D3DCOMPILE_ENABLE_STRICTNESS, 0, &code, &errors);
            if(FAILED(hr) && errors) std::fprintf(stderr,"%s",static_cast<const char *>(errors->GetBufferPointer()));
            check(SUCCEEDED(hr), "runtime fixture HLSL compiles");
            return code;
        };
        auto pixel = compile(
            "cbuffer Values:register(b4){float4 Tint;} Texture2D Tex:register(t12);"
            "Buffer<float4> Buf:register(t13); SamplerState Samp:register(s5);"
            "float4 main(float4 p:SV_POSITION):SV_TARGET{return Tint*Tex.Sample(Samp,float2(.5,.5))*Buf[0];}","ps_5_0");
        auto vertex = compile(
            "cbuffer Transform:register(b0){row_major float4x4 Mvp;}"
            "cbuffer VertexValues:register(b8){float4 Tint;}"
            "float4 main(float3 p:POSITION):SV_POSITION{return mul(Mvp,float4(p*Tint.a,1));}","vs_5_0");
        if(!pixel || !vertex) return false;
        auto material = make_ptr<ClawMaterial>();
        auto technique = make_ptr<ClawMaterialTechnique>(); technique->setMaterial(material.get()); technique->createPass();
        material->setTechniques({technique}); material->setCullMode(1); material->setBlendMode(0);
        material->setMaterialType(MaterialType::UI);
        using Type = MaterialShaderParameterType;
        material->setShaderParameters({{Type::Colour,"Tint"},{Type::Texture,"Tex"},
            {Type::Buffer,"Buf"},{Type::Sampler,"Samp"}},"RuntimeTest");
        auto &values = material->getShaderParameters();
        values.SetColour(values.FindParameter("Tint"),ColourF(1,0,0,1));
        values.SetTexture(values.FindParameter("Tex"),1);
        values.SetBuffer(values.FindParameter("Buf"),2);
        values.SetSampler(values.FindParameter("Samp"),3);
        std::unique_ptr<wp_shader_program,decltype(&wp_shader_program_destroy)> descriptor(
            wp_shader_program_create(), &wp_shader_program_destroy);
        const auto text = [](const char *p) { return reinterpret_cast<const wp_c8 *>(p); };
        wp_shader_program_register_uniform(descriptor.get(),text("Tint"),WORKPHONE_UNIFORM_TYPE_VEC4);
        wp_shader_program_register_param(descriptor.get(),text("Tex"),WORKPHONE_UNIFORM_TYPE_INT,1,
            WORKPHONE_SHADER_PARAM_VARIABILITY_PER_OBJECT,WORKPHONE_SHADER_AUTO_PARAM_NONE,WORKPHONE_SHADER_RESOURCE_TEXTURE);
        wp_shader_program_register_param(descriptor.get(),text("Buf"),WORKPHONE_UNIFORM_TYPE_INT,1,
            WORKPHONE_SHADER_PARAM_VARIABILITY_PER_OBJECT,WORKPHONE_SHADER_AUTO_PARAM_NONE,WORKPHONE_SHADER_RESOURCE_BUFFER);
        wp_shader_program_register_param(descriptor.get(),text("Samp"),WORKPHONE_UNIFORM_TYPE_INT,1,
            WORKPHONE_SHADER_PARAM_VARIABILITY_PER_OBJECT,WORKPHONE_SHADER_AUTO_PARAM_NONE,WORKPHONE_SHADER_RESOURCE_SAMPLER);
        String error;
        const MaterialProgramDX11::Bytecode ps{pixel->GetBufferPointer(),pixel->GetBufferSize()};
        const MaterialProgramDX11::Bytecode vs{vertex->GetBufferPointer(),vertex->GetBufferSize()};
        MaterialProgramReflectionSelection vertexSelection; // b0 is geometry-owned
        vertexSelection.constantBuffers = {"VertexValues"};
        auto program = MaterialProgramDX11::Create(device,values.GetLayout(),descriptor.get(),1,ps,vs,error,
            nullptr,&vertexSelection);
        if(!check(bool(program),error.c_str())) return false;
        struct Resources : MaterialProgramResourcesDX11
        {
            ComPtr<ID3D11ShaderResourceView> texture, buffer;
            ComPtr<ID3D11SamplerState> sampler;
            bool ready = true;
            unsigned calls = 0;
            bool Resolve(Type type, MaterialResourceHandle handle, void *&native) override
            {
                ++calls;
                if(!ready) return false;
                native = type == Type::Texture && handle == 1 ? texture.Get() :
                    type == Type::Buffer && handle == 2 ? buffer.Get() :
                    type == Type::Sampler && handle == 3 ? static_cast<void *>(sampler.Get()) : nullptr;
                return native != nullptr;
            }
        };
        auto resources = std::make_shared<Resources>();
        const u32 white = 0xffffffff, red = 0xff0000ff, green = 0xff00ff00;
        auto whiteView = claw_terrain_material_contracts::texture(native,1,1,DXGI_FORMAT_R8G8B8A8_UNORM,&white,4);
        auto redView = claw_terrain_material_contracts::texture(native,1,1,DXGI_FORMAT_R8G8B8A8_UNORM,&red,4);
        auto greenView = claw_terrain_material_contracts::texture(native,1,1,DXGI_FORMAT_R8G8B8A8_UNORM,&green,4);
        resources->texture = whiteView;
        D3D11_BUFFER_DESC desc{}; desc.ByteWidth = 16; desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        const float ones[] = {1,1,1,1}; D3D11_SUBRESOURCE_DATA initial{ones,0,0};
        ComPtr<ID3D11Buffer> buffer;
        D3D11_SHADER_RESOURCE_VIEW_DESC view{}; view.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        view.ViewDimension = D3D11_SRV_DIMENSION_BUFFER; view.Buffer.NumElements = 1;
        D3D11_SAMPLER_DESC sampler{}; sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampler.MaxLOD = D3D11_FLOAT32_MAX;
        if(!check(SUCCEEDED(device->CreateBuffer(&desc,&initial,&buffer)) &&
            SUCCEEDED(device->CreateShaderResourceView(buffer.Get(),&view,&resources->buffer)) &&
            SUCCEEDED(device->CreateSamplerState(&sampler,&resources->sampler)),"runtime native resources create")) return false;
        material->setRuntimeProgram(program,resources);
        TestMesh mesh; mesh.setMaterial(material);
        ClawMaterialDrawRecords a, b;
        a.capture(&mesh); renderer.prepareMaterialRecords(a);
        values.SetColour(values.FindParameter("Tint"),ColourF(0,1,0,1));
        b.capture(&mesh); renderer.prepareMaterialRecords(b);
        bool ok = true;
        const auto expect = [&](int channel, const char *message) {
            const auto pixels = claw_foliage_contracts::pixels(native);
            const auto offset = (32u*64u+32u)*4u;
            bool matches = true;
            for(int i = 0; i < 3; ++i) matches &= channel == i ? pixels[offset+i] > 200 : pixels[offset+i] < 10;
            return check(matches,message);
        };
        for(auto records : {&a,&b,&a})
        {
            renderer.clear(ColourF::Black);
            ok &= renderer.renderMeshRecords(&mesh,Matrix4F::identity(),*records,*records->find(&mesh));
            ok &= expect(records == &a ? 0 : 1,"runtime VS/PS + texture/buffer/sampler preserve A/B/A captured values");
        }
        values.SetColour(values.FindParameter("Tint"),ColourF(1,0,0,0));
        renderer.clear(ColourF::Black); renderer.renderMesh(&mesh,Matrix4F::identity());
        ok &= expect(-1,"vertex-stage material constants reach the active native vertex shader");
        values.SetColour(values.FindParameter("Tint"),ColourF::White);
        material->getRuntimePublication(); // Warm latest material revision.
        ClawMaterialDrawRecords current; current.capture(&mesh); renderer.prepareMaterialRecords(current);
        const auto publication = material->getRuntimePublication();
        for(auto texture : {redView,greenView})
        {
            resources->texture = texture;
            renderer.clear(ColourF::Black);
            renderer.renderMeshRecords(&mesh,Matrix4F::identity(),current,*current.find(&mesh));
            ok &= expect(texture == redView ? 0 : 1,"resource replacement resolves on every retained-publication draw");
            ok &= check(material->getRuntimePublication() == publication,"resource replacement does not repack CPU values");
        }
        resources->ready = false;
        wp_render_statistics_dx11 before{}, after{};
        wp_renderer_dx11_get_statistics(native,&before);
        renderer.clear(ColourF::Black);
        renderer.renderMeshRecords(&mesh,Matrix4F::identity(),current,*current.find(&mesh));
        wp_renderer_dx11_get_statistics(native,&after);
        ok &= expect(-1,"unresolved resource defers draw");
        ok &= check(after.draws == before.draws,"denied activation issues no native draw");
        resources->ready = true;
        // The canonical invalid handle clears a previous resource rather than reusing it.
        values.SetBuffer(values.FindParameter("Buf"),InvalidMaterialResourceHandle);
        renderer.clear(ColourF::Black); renderer.renderMesh(&mesh,Matrix4F::identity());
        ok &= expect(-1,"invalid resource handle binds null");
        values.SetBuffer(values.FindParameter("Buf"),2);
        resources->texture = resources->buffer;
        renderer.clear(ColourF::Black); renderer.renderMesh(&mesh,Matrix4F::identity());
        ok &= expect(-1,"buffer SRV cannot satisfy a reflected Texture2D");
        resources->texture = static_cast<ID3D11ShaderResourceView *>(target->getNativeTextureView());
        if(check(bool(resources->texture),"active target exposes an SRV for feedback rejection"))
        {
            wp_renderer_dx11_get_statistics(native,&before);
            renderer.clear(ColourF::Black); renderer.renderMesh(&mesh,Matrix4F::identity());
            wp_renderer_dx11_get_statistics(native,&after);
            ok &= check(after.draws == before.draws,"runtime material cannot sample its active output target");
        }
        else ok = false;
        ComPtr<ID3D11Device> foreignDevice;
        ComPtr<ID3D11DeviceContext> foreignContext;
        D3D_FEATURE_LEVEL feature;
        if(check(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,
            &foreignDevice,&feature,&foreignContext)),"foreign-device fixture creates"))
        {
            D3D11_TEXTURE2D_DESC image{}; image.Width=image.Height=image.MipLevels=image.ArraySize=1;
            image.Format=DXGI_FORMAT_R8G8B8A8_UNORM; image.SampleDesc.Count=1;
            image.Usage=D3D11_USAGE_IMMUTABLE; image.BindFlags=D3D11_BIND_SHADER_RESOURCE;
            D3D11_SUBRESOURCE_DATA pixels{&white,4,4}; ComPtr<ID3D11Texture2D> texture;
            ComPtr<ID3D11ShaderResourceView> view;
            ok &= check(SUCCEEDED(foreignDevice->CreateTexture2D(&image,&pixels,&texture)) &&
                SUCCEEDED(foreignDevice->CreateShaderResourceView(texture.Get(),nullptr,&view)),"foreign SRV creates");
            resources->texture=view;
            wp_renderer_dx11_get_statistics(native,&before);
            renderer.clear(ColourF::Black); renderer.renderMesh(&mesh,Matrix4F::identity());
            wp_renderer_dx11_get_statistics(native,&after);
            ok &= check(after.draws == before.draws,"foreign-device resource defers runtime material draw");
        }
        else ok = false;
        resources->texture = greenView;
        auto engineTexture = make_ptr<MaterialRoutingTexture>(); engineTexture->currentView = redView.Get();
        auto table = std::make_shared<MaterialProgramResourceTableDX11>();
        table->SetTexture(0,engineTexture); // Zero is a valid numeric resource identity.
        table->SetBuffer(2,resources->buffer.Get()); table->SetSampler(3,resources->sampler.Get());
        values.SetTexture(values.FindParameter("Tex"),0);
        material->setRuntimeProgram(program,table);
        const auto enginePublication = material->getRuntimePublication();
        for(auto view : {redView,greenView})
        {
            engineTexture->currentView = view.Get();
            renderer.clear(ColourF::Black); renderer.renderMesh(&mesh,Matrix4F::identity());
            ok &= expect(view == redView ? 0 : 1,"numeric owner table resolves current engine texture view");
            ok &= check(material->getRuntimePublication() == enginePublication,"texture completion preserves packed publication");
        }
        table->SetBuffer(2,resources->sampler.Get());
        wp_renderer_dx11_get_statistics(native,&before);
        renderer.clear(ColourF::Black); renderer.renderMesh(&mesh,Matrix4F::identity());
        wp_renderer_dx11_get_statistics(native,&after);
        ok &= check(after.draws == before.draws,"native interface mismatch rejects without dereferencing an SRV vtable");
        table->SetBuffer(2,resources->buffer.Get());
        // Warm captured draws and numeric lookups run with zero SourceCpu headroom.
        ClawMaterialDrawRecords warm; warm.capture(&mesh); renderer.prepareMaterialRecords(warm);
        auto scheduler = WorldStreamingScheduler::shared();
        const auto source = size_t(StreamMemoryClass::SourceCpu), gpu = size_t(StreamMemoryClass::Gpu);
        const auto baseline = scheduler->stats();
        bool limits = scheduler->setBudget(StreamMemoryClass::SourceCpu,baseline.used[source],error);
        renderer.clear(ColourF::Black);
        renderer.renderMeshRecords(&mesh,Matrix4F::identity(),warm,*warm.find(&mesh));
        const auto afterWarm = scheduler->stats();
        bool attachmentDenied = false;
        try { material->setRuntimeProgram(program,table); }
        catch(const std::bad_alloc &) { attachmentDenied = true; }
        scheduler->setBudget(StreamMemoryClass::SourceCpu,baseline.limits[source],error);
        ok &= check(attachmentDenied && material->getRuntimePublication() == enginePublication &&
            scheduler->stats().used[source] == baseline.used[source],
            "cold attachment CPU denial preserves the old immutable publication and lease count");
        ok &= expect(1,"warm runtime shader draw succeeds at full CPU budget");
        ok &= check(limits && afterWarm.used[source] == baseline.used[source] &&
            afterWarm.rejectedReservations[source] == baseline.rejectedReservations[source],
            "warm runtime binding performs no SourceCpu allocation");
        // Candidate failure preserves the last attachment and releases partial CPU/GPU ownership.
        limits = scheduler->setBudget(StreamMemoryClass::Gpu,baseline.used[gpu],error);
        bool denied = false;
        try { MaterialProgramDX11::Create(device,values.GetLayout(),descriptor.get(),1,ps,{},error); }
        catch(const std::bad_alloc &) { denied = true; }
        const auto afterDenied = scheduler->stats();
        scheduler->setBudget(StreamMemoryClass::Gpu,baseline.limits[gpu],error);
        ok &= check(limits && denied && afterDenied.used[source] == baseline.used[source] &&
            afterDenied.used[gpu] == baseline.used[gpu] && material->getRuntimePublication() == enginePublication,
            "GPU admission denial unwinds candidate while retaining the installed shader");
        engineTexture->currentView = nullptr;
        values.SetTexture(values.FindParameter("Tex"),1);
        // Standard pipeline must recover after both successful and denied overrides.
        material->setRuntimeProgram(nullptr); material->setDiffuse(ColourF(0,0,1,1));
        renderer.clear(ColourF::Black); renderer.renderMesh(&mesh,Matrix4F::identity());
        ok &= expect(2,"ordinary material restores built-in shaders/constants/resources");
        auto pixelOnly = MaterialProgramDX11::Create(device,values.GetLayout(),descriptor.get(),1,ps,{},error);
        material->setRuntimeProgram(pixelOnly,resources);
        wp_instance_pntc_dx11 instance{};
        for(int i=0;i<4;++i) instance.transform.m[i][i]=1;
        instance.tint={1,1,1,1};
        renderer.clear(ColourF::Black);
        ok &= check(renderer.renderMeshInstances(&mesh,&instance,1) == 1,"pixel-only program activates after instanced vertex setup");
        ok &= expect(1,"instanced custom material reaches pixel shader");
        material->setRuntimeProgram(program,resources);
        ok &= check(renderer.renderMeshInstances(&mesh,&instance,1) == 0,"custom scalar vertex program explicitly defers instancing");
        // In-place descriptor edits invalidate both current and retained publications.
        wp_shader_program_set_permutation_mask(descriptor.get(),WP_SHADER_PERMUTATION_ALPHA_TEST);
        renderer.clear(ColourF::Black);
        renderer.renderMeshRecords(&mesh,Matrix4F::identity(),a,*a.find(&mesh));
        ok &= expect(-1,"stale runtime program never draws retained constants with a new generation");
        material->setRuntimeProgram(nullptr);
        std::fprintf(stderr,"Runtime material activation: captured A/B/A, resource refresh/null/denial, built-in recovery, instancing and stale maps passed=%d\n",ok);
        return ok;
    }

    bool testPreparedMaterialEdits( ClawRendererDX11 &renderer )
    {
        claw_foliage_scene_contracts::Fixture fixture;
        auto target = make_ptr<ClawRenderTarget>();
        target->setSize( { 64, 64 } );
        renderer.setRenderTarget( target );
        renderer.setViewport( nullptr );
        renderer.setCamera( nullptr );
        renderer.disableShadows();
        auto native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
        auto material = make_ptr<ClawMaterial>();
        auto technique = make_ptr<ClawMaterialTechnique>();
        technique->setMaterial( material.get() );
        auto pass = technique->createPass();
        material->setTechniques( { technique } );
        material->setMaterialType( MaterialType::UI );
        material->setCullMode( 1u );
        material->setBlendMode( 0u );
        TestMesh mesh;
        mesh.setMaterial( material );
        bool ok = true;
        // Repeated draws hit the prepared record; a new snapshot must replace
        // its surface values, while lighting/fog continue to belong to the view.
        for( int edit = 0; edit < 8; ++edit )
        {
            const bool red = edit % 2 == 0;
            material->setDiffuse( red ? ColourF( 1, 0, 0, 1 ) : ColourF( 0, 1, 0, 1 ) );
            ClawRendererDX11::MaterialRecordStatistics firstView;
            for( int view = 0; view < 2; ++view )
            {
                renderer.setSceneLighting( view ? ColourF::Black : ColourF::White,
                                          Vector3F( 0, -1, 0 ), ColourF::White, f32( view ) );
                renderer.setSceneFog( 0u, ColourF::Black, 0.0f, 0.0f, 1.0f );
                renderer.clear( ColourF::Black );
                renderer.renderMesh( &mesh, Matrix4F::identity() );
                const auto statistics = renderer.getMaterialRecordStatistics();
                if(view == 0) firstView = statistics;
                else ok &= check(statistics.preparations == firstView.preparations &&
                                 statistics.storageAllocations == firstView.storageAllocations &&
                                 statistics.directStorageAllocations == firstView.directStorageAllocations,
                                 "unchanged direct/UI views reuse persistent records and warmed publication storage");
                const auto pixels = claw_foliage_contracts::pixels( native );
                const auto offset = ( 32u * 64u + 32u ) * 4u;
                ok &= check( pixels[offset + ( red ? 0u : 1u )] > 200 &&
                             pixels[offset + ( red ? 1u : 0u )] < 10 && pixels[offset + 2] < 10,
                             "prepared UI material must show current edits across repeated draws/views" );
            }
        }
        {
            auto streamed = make_ptr<MaterialRoutingTexture>();
            const u32 redPixel = 0xff0000ffu, greenPixel = 0xff00ff00u;
            auto redView = claw_terrain_material_contracts::texture(native, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &redPixel, 4);
            auto greenView = claw_terrain_material_contracts::texture(native, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &greenPixel, 4);
            material->setDiffuse(ColourF::White);
            {
                auto data = pass->getStateContext()->invalidateStateDataById<MaterialPassStateData>(pass->getId());
                data->setTextureBinding( 0u, streamed );
            }
            const auto version = material->getRenderState();
            ClawMaterialDrawRecords retained;
            retained.capture(&mesh); renderer.prepareMaterialRecords(retained);
            const auto prepared = renderer.getMaterialRecordStatistics().preparations;
            for(int replacement = 0; replacement < 4; ++replacement)
            {
                streamed->currentView = replacement == 1 ? redView.Get() : replacement == 2 ? greenView.Get() : nullptr;
                renderer.beginRender();
                renderer.clear(ColourF::Black);
                ok &= renderer.renderMeshRecords(&mesh, Matrix4F::identity(), retained, *retained.find(&mesh));
                renderer.endRender();
                const auto pixels = claw_foliage_contracts::pixels(native);
                const auto offset = (32u * 64u + 32u) * 4u;
                ok &= check((replacement == 2 ? pixels[offset] < 10 : pixels[offset] > 200) &&
                            (replacement == 1 ? pixels[offset + 1] < 10 : pixels[offset + 1] > 200) &&
                            material->getRenderState() == version &&
                            renderer.getMaterialRecordStatistics().preparations == prepared,
                            "retained records resolve pending, completed, replaced and removed native texture views without stale bindings");
            }
            auto data = pass->getStateContext()->invalidateStateDataById<MaterialPassStateData>(pass->getId());
            data->setTextureBinding( 0u, nullptr );
        }
        material->setMaterialType( MaterialType::Standard );
        material->setDiffuse( ColourF::White );
        material->setSpecular( ColourF::Black );
        material->setMetalness( 0.0f );
        for( int view = 0; view < 2; ++view )
        {
            renderer.setSceneLighting( view ? ColourF::White : ColourF::Black,
                                      Vector3F( 0, -1, 0 ), ColourF::White, 0.0f );
            renderer.clear( ColourF::Black );
            renderer.renderMesh( &mesh, Matrix4F::identity() );
            const auto pixels = claw_foliage_contracts::pixels( native );
            const auto offset = ( 32u * 64u + 32u ) * 4u;
            const auto brightness = int( pixels[offset] ) + pixels[offset + 1] + pixels[offset + 2];
            ok &= check( view ? brightness > 60 : brightness < 10,
                         "cached lit material must use current view lighting without a material edit" );
        }
        auto metallic = make_ptr<MaterialRoutingTexture>();
        auto roughness = make_ptr<MaterialRoutingTexture>();
        auto packed = make_ptr<MaterialRoutingTexture>();
        {
            auto data = pass->getStateContext()->invalidateStateDataById<MaterialPassStateData>( pass->getId() );
            data->setTextureBinding( 2u, metallic );
            data->setTextureBinding( 3u, roughness );
            data->setTextureBinding( 25u, packed );
        }
        material->setEditorUInt( "metallicSource", 0u );
        constexpr u32 roughnessSlots[] = { 3u, 3u, 2u, 25u, 2u, 3u };
        for( u32 mode = 0; mode < 6; ++mode )
        {
            material->setEditorUInt( "roughnessSource", mode );
            ClawMaterialDrawRecords records;
            records.capture( &mesh ); renderer.prepareMaterialRecords( records );
            const auto preparedStatistics = renderer.getMaterialRecordStatistics();
            for( int repeat = 0; repeat < 4; ++repeat )
            {
                metallic->requests = roughness->requests = packed->requests = 0;
                renderer.clear( ColourF::Black );
                if( repeat < 2 ) renderer.renderMesh( &mesh, Matrix4F::identity() );
                else ok &= renderer.renderMeshRecords( &mesh, Matrix4F::identity(), records, *records.find( &mesh ) );
                ok &= check(renderer.getMaterialRecordStatistics().preparations == preparedStatistics.preparations,
                            "direct and scene adapters share the same persistent material preparation");
                ok &= check( metallic->requests == ( roughnessSlots[mode] == 2u ? 2u : 1u ) &&
                             roughness->requests == ( roughnessSlots[mode] == 3u ? 1u : 0u ) &&
                             packed->requests == ( roughnessSlots[mode] == 25u ? 1u : 0u ),
                             "direct and captured draws must resolve current native views for every roughness mode" );
            }
            const auto staleBinding = *records.find( &mesh );
            records.clear();
            const auto beforeRejected = claw_foliage_contracts::statistics( native );
            ok &= check( !renderer.renderMeshRecords( &mesh, Matrix4F::identity(), records, staleBinding ) &&
                         claw_foliage_contracts::statistics( native ).draws == beforeRejected.draws,
                         "stale material publications must reject before issuing GPU work" );
        }
        MaterialShader shader( "ColorOnly" );
        shader.addParameter( MaterialShaderParameterType::Colour, "Color" )
              .addParameter( MaterialShaderParameterType::Scalar, "Opacity" );
        MaterialShaderRegistry::instance().registerShader( shader );
        material->setShaderParameters( shader.getParameterInfo(), shader.getShaderID() );
        material->routeDataDrivenMaterialType();
        auto &parameters = material->getShaderParameters();
        const auto color = parameters.Resolve<ColourF>( "Color" );
        const auto opacity = parameters.Resolve<f32>( "Opacity" );
        for( int edit = 0; edit < 8; ++edit )
        {
            const bool red = edit % 2 == 0;
            if( edit < 4 )
            {
                auto transaction = parameters.Edit();
                parameters.Set( color, red ? ColourF( 1, 0, 0, 1 ) : ColourF( 0, 1, 0, 1 ) );
                parameters.Set( opacity, 1.0f );
            }
            else if(edit < 6) material->batch( [&]( Material &m ) {
                m.setUVOffset( Vector2F( 0.1f, 0.2f ) );
                parameters.Set( color, red ? ColourF( 1, 0, 0, 1 ) : ColourF( 0, 1, 0, 1 ) );
                parameters.Set( opacity, 1.0f );
            } );
            else
            {
                const auto previous = material->getRenderState();
                MaterialPublicationScope update;
                material->setUVOffset(Vector2F(0.3f, 0.4f));
                parameters.Set(color, red ? ColourF(1, 0, 0, 1) : ColourF(0, 1, 0, 1));
                parameters.Set(opacity, 1.0f);
                ok &= check(material->getRenderState() == previous,
                            "update edits retain the previously captured version until commit");
                update.commit();
                ok &= check(material->getRenderState()->revision == previous->revision + 1,
                            "independent update edits publish one version before rendering");
            }
            renderer.clear( ColourF::Black );
            renderer.renderMesh( &mesh, Matrix4F::identity() );
            const auto pixels = claw_foliage_contracts::pixels( native );
            const auto offset = ( 32u * 64u + 32u ) * 4u;
            ok &= check( pixels[offset + ( red ? 0u : 1u )] > 200 &&
                         pixels[offset + ( red ? 1u : 0u )] < 10,
                         "retained typed parameter edits must reach GPU pixels without manual apply" );
        }
        // Exercise the actual scene capture/submission path, including a main-only
        // mesh and retained jobs reused after a material edit.
        parameters.Set( color, ColourF::Red );
        auto scene = make_ptr<MaterialStorageScene>();
        scene->setEnableShadows( false );
        TestMesh second;
        second.setMaterial( material ); second.setCastShadows( false );
        for( auto current : { &mesh, &second } )
        {
            wp_graphics_mesh_compute_aabb( current->getNativeMesh() );
            current->bindNativeRenderObject( wp_graphics_scene_create_object( scene->getNativeScene() ) );
            current->setVisibilityFlags( ~u32( 0 ) );
        }
        ok &= check( scene->prepareRenderJobs( nullptr, true, &renderer ), "scene material publication must prepare" );
        ClawScene::MaterialPreparationStatistics materialStats;
        ClawScene::PreparationTiming timing;
        ok &= check( scene->getPreparedMaterialStatistics( materialStats ) && materialStats.records == 1 &&
                     materialStats.bindings == 2 && materialStats.meshes == 2 && materialStats.preparations <= 1 &&
                     scene->getPreparedTiming( timing ) && timing.material_prepare_ns > 0 &&
                     timing.total_ns >= timing.main_prepare_ns + timing.shadow_derive_ns + timing.material_prepare_ns,
                     "scene captures one shared record for the union of main/shadow sections and profiles its work" );
        const auto queries = mesh.materialQueries + second.materialQueries;
        std::weak_ptr<const MaterialRenderState> retired = material->getRenderState();
        parameters.Set( color, ColourF::Green );
        // The legacy native descriptor adapter independently retains its last
        // synchronized snapshot. Advance it so this check isolates scene ownership.
        material->getNativeMaterial();
        for( int view = 0; view < 2; ++view )
        {
            renderer.clear( ColourF::Black );
            scene->render( static_cast<IRenderer *>( &renderer ) );
            const auto pixels = claw_foliage_contracts::pixels( native );
            const auto offset = ( 32u * 64u + 32u ) * 4u;
            ok &= check( pixels[offset] > 200 && pixels[offset + 1] < 10 &&
                         mesh.materialQueries + second.materialQueries == queries && !retired.expired(),
                         "scene draws borrow frozen red records without reacquiring mesh materials" );
        }
        auto inFlight = scene->retainJobs();
        ok &= check( scene->prepareRenderJobs( nullptr, false, &renderer ) && !retired.expired() &&
                     scene->getPreparedMaterialStatistics( materialStats ) && !materialStats.reused_storage,
                     "a retained consumer prevents storage reuse and preserves old material owners" );
        inFlight.reset();
        ok &= check( retired.expired(), "releasing the final old consumer retires its material version" );
        renderer.clear( ColourF::Black ); scene->render( static_cast<IRenderer *>( &renderer ) );
        const auto currentPixels = claw_foliage_contracts::pixels( native );
        const auto offset = ( 32u * 64u + 32u ) * 4u;
        ok &= check( currentPixels[offset + 1] > 200 && currentPixels[offset] < 10,
                     "the next scene publication displays the edited green material" );
        ok &= check( scene->prepareRenderJobs( nullptr, false, &renderer ) &&
                     scene->getPreparedMaterialStatistics( materialStats ) && materialStats.reused_storage &&
                     materialStats.storage_allocations == 0 && materialStats.retained_storage_bytes > 0,
                     "the next exclusive publication reuses storage without upstream allocations" );
        scene->discardRenderJobs();
        ok &= check( !scene->getPreparedMaterialStatistics( materialStats ) && materialStats.records == 0 &&
                     scene->retainedBytes() > 0,
                     "retired scene jobs report no retained material publication" );
        const auto warmStorageAllocations = scene->retainedAllocations();
        renderer.clear( ColourF::Black ); scene->render( static_cast<IRenderer *>( &renderer ) );
        ok &= check( !scene->getPreparedMaterialStatistics( materialStats ) &&
                     scene->retainedAllocations() == warmStorageAllocations,
                     "temporary direct scene renders also recycle storage after their borrow ends" );
        mesh.bindNativeRenderObject( nullptr ); second.bindNativeRenderObject( nullptr );
        second.setMaterial( nullptr ); scene->unload( nullptr );
        ok &= check( scene->retainedBytes() == 0, "scene unload releases retired storage capacity" );
        mesh.setMaterial( nullptr );
        material->unload( nullptr );
        renderer.setRenderTarget( nullptr );
        return ok;
    }

    bool testSortedTransparency( ClawRendererDX11 &renderer )
    {
        claw_foliage_scene_contracts::Fixture fixture;
        auto target = make_ptr<ClawRenderTarget>(); target->setSize( {64,64} );
        renderer.setRenderTarget( target ); renderer.setViewport( nullptr );
        auto camera = make_ptr<ClawCamera>();
        camera->setNearClipDistance( 0.1f ); camera->setFarClipDistance( 10 ); camera->setAspectRatio( 1 );
        wp_camera_set_position( camera->getNativeCamera(), {0,0,3} );
        renderer.setCamera( camera );
        auto scene = make_ptr<MaterialStorageScene>(); scene->setEnableShadows( false );
        scene->setAmbientLight( ColourF::White );
        TestMesh nearMesh, farMesh;
        auto makeMaterial = []( ColourF colour ) {
            auto material = make_ptr<ClawMaterial>();
            auto technique = make_ptr<ClawMaterialTechnique>(); technique->setMaterial( material.get() );
            technique->createPass(); material->setTechniques( {technique} );
            material->setCullMode( 1 ); material->setDiffuse( colour ); material->setOpacity( 0.5f );
            material->setDepthWrite( false ); return material;
        };
        auto nearMaterial = makeMaterial( ColourF::Red ), farMaterial = makeMaterial( ColourF::Blue );
        nearMesh.setMaterial( nearMaterial ); farMesh.setMaterial( farMaterial );
        wp_graphics_object *objects[2];
        int index = 0;
        for( auto mesh : { &nearMesh, &farMesh } )
        {
            auto object = wp_graphics_scene_create_object( scene->getNativeScene() ); objects[index] = object;
            mesh->bindNativeRenderObject( object );
            mesh->setVisibilityFlags( ~u32( 0 ) );
            auto node = wp_graphics_scene_create_node( scene->getNativeScene() );
            wp_scenenode_set_position( node, {0,0,index == 0 ? 0.4f : 0.0f} );
            wp_scenenode_attach_object( node, object ); ++index;
        }
        auto native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
        bool ok = true;
        auto draw = [&] {
            ok &= check( scene->prepareRenderJobs( camera, false, &renderer ), "sorted alpha scene prepares" );
            renderer.clear( ColourF::Black ); scene->render( static_cast<IRenderer *>( &renderer ) );
            return claw_foliage_contracts::pixels( native );
        };
        // Explicit z-order supplies an independent far-before-near reference.
        wp_graphics_object_set_z_order( objects[0], 1 );
        const auto reference = draw();
        const auto pixel = ( 32u * 64u + 32u ) * 4u;
        ok &= check( reference[pixel] > 20 && reference[pixel+2] > 10, "alpha reference contains both surfaces" );
        wp_graphics_object_set_z_order( objects[0], 0 );
        ok &= check( draw() != reference, "near-first alpha fixture detects incorrect submission order" );
        for( auto strategy : { ClawDrawSortStrategy::FrontToBack, ClawDrawSortStrategy::MaterialFirst,
                               ClawDrawSortStrategy::DepthBuckets } )
        {
            ok &= check( scene->setDrawSortStrategy( strategy ), "valid scene sort policy accepted" );
            ok &= check( draw() == reference, "every optimized strategy preserves back-to-front alpha pixels" );
            const auto publication = scene->retainJobs();
            ok &= check( scene->setDrawSortStrategy( strategy ) && scene->retainJobs() == publication,
                         "equal sort policy preserves prepared publication" );
            ok &= check( !scene->setDrawSortStrategy( static_cast<ClawDrawSortStrategy>( 99 ) ) &&
                         !scene->setDrawSortStrategy( strategy, 0 ) && scene->retainJobs() == publication,
                         "invalid sort policies preserve publication" );
        }
        nearMesh.bindNativeRenderObject( nullptr ); farMesh.bindNativeRenderObject( nullptr );
        scene->unload( nullptr );
        nearMesh.setMaterial( nullptr ); farMesh.setMaterial( nullptr );
        nearMaterial->unload( nullptr ); farMaterial->unload( nullptr );
        renderer.setCamera( nullptr ); renderer.setRenderTarget( nullptr );
        return ok;
    }

    bool checkMaterialBuffer(wp_renderer_dx11 *native, unsigned slot, const std::vector<wp_vec4f> &expected)
    {
        auto context = static_cast<ID3D11DeviceContext *>(wp_renderer_dx11_get_context(native));
        auto device = static_cast<ID3D11Device *>(wp_renderer_dx11_get_device(native));
        ID3D11Buffer *buffer = nullptr, *staging = nullptr;
        context->PSGetConstantBuffers(slot, 1, &buffer);
        if(!check(buffer != nullptr, "split material buffer is bound")) return false;
        D3D11_BUFFER_DESC desc{}; buffer->GetDesc(&desc);
        bool ok = check(desc.ByteWidth == expected.size() * sizeof(wp_vec4f), "split material buffer has exact packed size");
        desc.Usage = D3D11_USAGE_STAGING; desc.BindFlags = 0; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        if(SUCCEEDED(device->CreateBuffer(&desc, nullptr, &staging))) {
            context->CopyResource(staging, buffer);
            D3D11_MAPPED_SUBRESOURCE mapped{};
            const auto readable = SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped));
            ok &= check(readable, "split material GPU constants can be read back");
            if(readable) {
                ok &= check(desc.ByteWidth == expected.size() * sizeof(wp_vec4f) &&
                    std::memcmp(mapped.pData, expected.data(), expected.size() * sizeof(wp_vec4f)) == 0,
                    "all material vectors retain their exact shader register packing");
                context->Unmap(staging, 0);
            }
            staging->Release();
        } else ok &= check(false, "split material staging buffer can be created");
        buffer->Release();
        return ok;
    }

    bool testMaterialUploadWork( ClawRendererDX11 &renderer )
    {
        claw_foliage_scene_contracts::Fixture fixture;
        auto target = make_ptr<ClawRenderTarget>(); target->setSize( {64,64} );
        renderer.setRenderTarget( target ); renderer.setCamera( nullptr ); renderer.disableShadows();
        auto native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
        TestMesh mesh;
        auto geometry = wp_renderer_dx11_prepare_mesh_geometry( native, mesh.getNativeMesh(), 0, 3 );
        if( !check( geometry != nullptr, "material upload workload prepares native geometry" ) ) return false;
        wp_mat4f identity{};
        for( int axis = 0; axis < 4; ++axis ) identity.m[axis][axis] = 1;
        auto world = identity; world.m[0][3] = 4; // Clip geometry to isolate CPU upload/submission work.
        wp_renderer_dx11_set_world_matrix( native, &world );
        wp_renderer_dx11_set_view_matrix( native, &identity );
        wp_renderer_dx11_set_projection_matrix( native, &identity );
        bool ok = true;
        constexpr int samples = 7, draws = 512;
        std::array<double,samples> timings[8];
        wp_u64 uploads[8]{}, bytes[8]{}, maps[8]{};
        ok &= check(wp_renderer_dx11_get_material_upload_mode(native) == WP_DX11_MATERIAL_UPLOAD_COMBINED, "combined material uploads remain the default");
        ok &= check(!wp_renderer_dx11_set_material_upload_mode(native, -1), "invalid upload policy is rejected");
        for( int sample = 0; sample < samples; ++sample ) for( int order = 0; order < 8; ++order )
        {
            const auto variant = ( sample + order ) % 8;
            const auto policy = variant / 4, mode = variant % 4;
            ok &= check(renderer.setSplitMaterialUploads(policy != 0), "material upload policy can switch between draws");
            wp_material_dx11 material{};
            material.base_color = {0.1f,0.2f,0.3f,1}; material.surface = {0,0.5f,1,1}; material.controls.z = -1;
            const auto set = [&]( float value ) {
                if( mode == 0 || mode == 3 ) material.base_color.x = value;
                if( mode == 1 || mode == 3 ) material.light_color.w = value;
                if( mode == 2 || mode == 3 ) material.map_flags.x = value > 0.5f ? 1.0f : 0.0f;
                wp_renderer_dx11_set_material( native, &material );
            };
            set( 0.6f ); wp_renderer_dx11_draw_geometry_pntc( native, geometry, 0, 3, 0 );
            wp_render_statistics_dx11 before{}, after{};
            wp_material_upload_statistics_dx11 uploadBefore{}, uploadAfter{};
            uploadBefore.struct_size = uploadAfter.struct_size = sizeof(uploadBefore);
            ok &= check(wp_renderer_dx11_get_material_upload_statistics(native, &uploadBefore), "upload counters are available");
            wp_renderer_dx11_get_statistics( native, &before );
            const auto begin = std::chrono::steady_clock::now();
            for( int draw = 0; draw < draws; ++draw )
            {
                set( draw % 2 ? 0.8f : 0.2f );
                wp_renderer_dx11_draw_geometry_pntc( native, geometry, 0, 3, 0 );
            }
            const auto end = std::chrono::steady_clock::now();
            timings[variant][sample] = std::chrono::duration<double,std::milli>( end - begin ).count();
            wp_renderer_dx11_get_statistics( native, &after );
            uploads[variant] = after.material_uploads - before.material_uploads;
            ok &= check(wp_renderer_dx11_get_material_upload_statistics(native, &uploadAfter), "upload counters remain available");
            const auto combined = uploadAfter.combined_uploads - uploadBefore.combined_uploads;
            const auto surface = uploadAfter.surface_uploads - uploadBefore.surface_uploads;
            const auto view = uploadAfter.view_uploads - uploadBefore.view_uploads;
            const auto binding = uploadAfter.binding_uploads - uploadBefore.binding_uploads;
            maps[variant] = combined + surface + view + binding;
            bytes[variant] = uploadAfter.combined_bytes - uploadBefore.combined_bytes +
                uploadAfter.surface_bytes - uploadBefore.surface_bytes + uploadAfter.view_bytes - uploadBefore.view_bytes +
                uploadAfter.binding_bytes - uploadBefore.binding_bytes;
            ok &= check(combined == (policy == 0 ? draws : 0) &&
                surface == (policy && (mode == 0 || mode == 3) ? draws : 0) &&
                view == (policy && (mode == 1 || mode == 3) ? draws : 0) &&
                binding == (policy && (mode == 2 || mode == 3) ? draws : 0) &&
                uploadAfter.map_failures == uploadBefore.map_failures,
                "only changed constant blocks are mapped");
            const auto expectedBytes = policy ? draws * (mode == 0 ? 128 : mode == 1 ? 112 : mode == 2 ? 32 : 272) : draws * 272;
            ok &= check(bytes[variant] == expectedBytes, "material upload byte counts match block packing");
            ok &= check( after.draws - before.draws == draws && uploads[variant] == draws,
                         "material upload workload changes constants for each native draw" );
        }
        for( int variant = 0; variant < 8; ++variant )
        {
            std::sort( timings[variant].begin(), timings[variant].end() );
            std::printf( "Material upload workload policy=%d mode=%d draws=%d samples=%d cpu_median_ms=%.6f maps=%llu bytes=%llu\n",
                variant / 4, variant % 4, draws, samples, timings[variant][samples/2],
                static_cast<unsigned long long>( maps[variant] ), static_cast<unsigned long long>( bytes[variant] ) );
        }
        ok &= check(renderer.setSplitMaterialUploads(true), "remaining rendering contracts use split constants");
        wp_material_upload_statistics_dx11 invalid{};
        invalid.struct_size = 1; invalid.surface_bytes = 1234;
        ok &= check(!wp_renderer_dx11_get_material_upload_statistics(native, &invalid) &&
            invalid.struct_size == 1 && invalid.surface_bytes == 1234 &&
            !wp_renderer_dx11_get_material_upload_statistics(nullptr, &invalid) &&
            !wp_renderer_dx11_get_material_upload_statistics(native, nullptr), "upload statistics reject invalid callers without writing");
        struct ExtendedStats { wp_material_upload_statistics_dx11 known; wp_u64 tail; } extended{};
        extended.known.struct_size = sizeof(extended); extended.tail = 5678;
        ok &= check(wp_renderer_dx11_get_material_upload_statistics(native, &extended.known) &&
            extended.known.struct_size == sizeof(extended.known) && extended.tail == 5678,
            "upload statistics preserve future caller extensions");
        wp_material_upload_statistics_dx11 stableBefore{}, stableAfter{};
        stableBefore.struct_size = stableAfter.struct_size = sizeof(stableBefore);
        wp_material_dx11 stable{}; stable.base_color = {.8f,.2f,.3f,1}; stable.surface = {0,.5f,1,1}; stable.controls.z = -1;
        wp_renderer_dx11_set_material(native, &stable);
        wp_renderer_dx11_draw_geometry_pntc(native, geometry, 0, 3, 0);
        wp_renderer_dx11_get_material_upload_statistics(native, &stableBefore);
        for(int repeat = 0; repeat < 32; ++repeat) {
            auto transient = stable; transient.base_color.x = .1f;
            wp_renderer_dx11_set_material(native, &transient);
            wp_renderer_dx11_set_material(native, &stable);
            wp_renderer_dx11_invalidate_state(native);
            wp_renderer_dx11_draw_geometry_pntc(native, geometry, 0, 3, 0);
        }
        wp_renderer_dx11_get_material_upload_statistics(native, &stableAfter);
        ok &= check(stableAfter.surface_uploads == stableBefore.surface_uploads &&
            stableAfter.view_uploads == stableBefore.view_uploads && stableAfter.binding_uploads == stableBefore.binding_uploads,
            "cancelled edits and binding invalidation reuse uploaded split constants");
        // Every vector has distinct data so offset swaps cannot pass through similar defaults.
        std::array<wp_vec4f,17> vectors{};
        for(unsigned i = 0; i < vectors.size(); ++i) vectors[i] = {float(i*4+1),float(i*4+2),float(i*4+3),float(i*4+4)};
        vectors[10].z = vectors[10].w = 0; vectors[14].x = vectors[14].y = 0;
        wp_material_dx11 packed{}; static_assert(sizeof(packed) == sizeof(vectors));
        std::memcpy(&packed, vectors.data(), sizeof(packed));
        wp_renderer_dx11_set_environment(native, nullptr, 0);
        wp_renderer_dx11_set_texture_native(native, nullptr);
        wp_renderer_dx11_set_material_textures(native, nullptr);
        wp_renderer_dx11_set_material(native, &packed);
        wp_renderer_dx11_draw_geometry_pntc(native, geometry, 0, 3, 0);
        ok &= checkMaterialBuffer(native, 1, {vectors[0],vectors[1],vectors[2],vectors[6],vectors[7],vectors[8],vectors[11],vectors[12]});
        ok &= checkMaterialBuffer(native, 6, {vectors[3],vectors[4],vectors[5],vectors[13],vectors[14],vectors[15],vectors[16]});
        ok &= checkMaterialBuffer(native, 7, {vectors[9],vectors[10]});
        if(wp_renderer_dx11_begin_shadow_map(native, &identity, 64)) {
            ok &= check(!renderer.setSplitMaterialUploads(false) && renderer.setSplitMaterialUploads(true),
                "shadow passes reject upload policy changes but allow equal assignments");
            wp_renderer_dx11_end_shadow_map(native);
        } else ok &= check(false, "upload policy shadow guard fixture begins");
        wp_renderer_dx11_set_world_matrix( native, &identity );
        renderer.setRenderTarget( nullptr );
        return ok;
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

    class ShadowReceiver : public ClawMesh
    {
    public:
        ShadowReceiver()
        {
            m_mesh = wp_graphics_mesh_create();
            const wp_graphics_mesh_vertex_pnt vertices[] = {
                { {-1,-1,0}, {0,0,1}, {0,0} }, { {1,-1,0}, {0,0,1}, {1,0} },
                { {1,1,0}, {0,0,1}, {1,1} }, { {-1,1,0}, {0,0,1}, {0,1} }
            };
            const wp_u16 indices[] = {0,1,2,0,2,3};
            wp_graphics_mesh_set_vertices( m_mesh, WORKPHONE_VERTEX_FORMAT_PNT, vertices, 4 );
            wp_graphics_mesh_set_indices_u16( m_mesh, indices, 6 );
        }
    };

    bool testMeshShadows( ClawRendererDX11 &renderer )
    {
        auto target = make_ptr<ClawRenderTarget>(); target->setSize({64,64});
        renderer.setRenderTarget(target);
        auto camera = make_ptr<ClawCamera>();
        camera->setNearClipDistance(0.1f); camera->setFarClipDistance(10); camera->setAspectRatio(1);
        wp_camera_set_position(camera->getNativeCamera(), {0,0,3});
        renderer.setCamera(camera);
        renderer.setSceneLighting(ColourF::White * 0.02f, Vector3F(0,0,-1), ColourF::White, 3);
        ShadowReceiver receiver, caster;
        auto casterWorld = Matrix4F::identity();
        casterWorld[0][0] = 0.25f; casterWorld[0][3] = -0.5f; casterWorld[2][3] = 1;
        auto *native = wp_renderer_get_dx11(renderer.getNativeRenderer());
        auto *device = static_cast<ID3D11Device *>(wp_renderer_dx11_get_device(native));
        auto *context = static_cast<ID3D11DeviceContext *>(wp_renderer_dx11_get_context(native));
        bool ok = true;
        for( int pass = 0; pass < 3; ++pass )
        {
            caster.setCastShadows(pass != 2); receiver.setReceiveShadows(pass != 1);
            const bool begun = renderer.beginShadowMap(512);
            ok &= check(begun, "C++ mesh shadow pass must begin");
            if( !begun ) break;
            if( caster.getCastShadows() ) renderer.renderMesh(&caster, casterWorld);
            renderer.endShadowMap();
            renderer.clear(ColourF::Black);
            renderer.renderMesh(&receiver, Matrix4F::identity());
            ID3D11RenderTargetView *view = nullptr;
            context->OMGetRenderTargets(1, &view, nullptr);
            if( !view ) { ok &= check(false,"mesh receiver target must be bound"); break; }
            ID3D11Resource *resource = nullptr; view->GetResource(&resource); view->Release();
            ID3D11Texture2D *source = nullptr, *staging = nullptr;
            resource->QueryInterface(IID_PPV_ARGS(&source)); resource->Release();
            if( !source ) { ok &= check(false,"mesh receiver must have a texture"); break; }
            D3D11_TEXTURE2D_DESC desc{}; source->GetDesc(&desc);
            desc.Usage = D3D11_USAGE_STAGING; desc.BindFlags = 0; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            const bool allocated = SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&staging));
            ok &= check(allocated,"mesh shadow readback must allocate");
            if( allocated )
            {
                context->CopyResource(staging,source);
                D3D11_MAPPED_SUBRESOURCE mapped{};
                const bool readable = SUCCEEDED(context->Map(staging,0,D3D11_MAP_READ,0,&mapped));
                ok &= check(readable,"mesh shadow pixels must map");
                if( readable )
                {
                    const auto *row = static_cast<const unsigned char *>(mapped.pData) + 32 * mapped.RowPitch;
                    const int shadow = row[19 * 4 + 1], lit = row[45 * 4 + 1];
                    std::printf("C++ mesh shadow pass %d: shadow=%d lit=%d\n",pass,shadow,lit);
                    ok &= check(lit > 100,"C++ mesh receiver must have direct lighting");
                    ok &= check(pass == 0 ? shadow + 40 < lit : std::abs(shadow-lit) < 15,
                        "C++ meshes must cast and receive shadows, and honor both shadow toggles");
                    context->Unmap(staging,0);
                }
                staging->Release();
            }
            source->Release();
        }
        renderer.disableShadows(); renderer.setCamera(nullptr); renderer.setRenderTarget(nullptr);
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
        ok &= check( mesh.getCastShadows() && mesh.getReceiveShadows(),
                     "Claw meshes must cast and receive shadows without a state context" );
        mesh.setCastShadows( false ); mesh.setReceiveShadows( false );
        ok &= check( !mesh.getCastShadows() && !mesh.getReceiveShadows(),
                     "Claw mesh shadow settings must persist before native object attachment" );
        auto object = wp_graphics_object_create();
        mesh.bindNativeRenderObject( object );
        ok &= check( !wp_graphics_object_get_cast_shadows( object ) && !wp_graphics_object_get_receive_shadows( object ),
                     "native mesh object must inherit disabled shadow settings" );
        mesh.setCastShadows( true ); mesh.setReceiveShadows( true );
        ok &= check( wp_graphics_object_get_cast_shadows( object ) && wp_graphics_object_get_receive_shadows( object ),
                     "shadow toggles must reach the attached native mesh object" );
        mesh.bindNativeRenderObject( nullptr ); wp_graphics_object_destroy( object );
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

int main( int argc, char **argv )
{
    graphics_crash_evidence::install();
    TypeManager types; types.load(); TypeManager::setInstance(&types);
    auto application = make_ptr<core::ApplicationManager>();
    core::IApplicationManager::setInstance(application);
    if( argc > 1 )
    {
        char *end = nullptr;
        const auto iterations = argc == 3 ? std::strtol( argv[2], &end, 10 ) : 0;
        if( argc != 3 || std::strcmp( argv[1], "--lifecycle-soak" ) ||
            !end || *end || iterations < 1 || iterations > 10000 )
        {
            std::fprintf( stderr, "Usage: --lifecycle-soak iterations[1..10000]\n" );
            core::IApplicationManager::setInstance(nullptr); application=nullptr;
            TypeManager::setInstance(nullptr); types.unload();
            return 2;
        }
        bool ok = true;
        for( long iteration = 0; iteration < iterations && ok; ++iteration )
        {
            std::fprintf( stderr, "Lifecycle soak iteration %ld\n", iteration + 1 );
            ok = testParticleLifecycle() && claw_scene_node_transform_contracts::run() &&
                 claw_material_pass_lifetime_contracts::run() && claw_scene_lifetime_contracts::run();
        }
        core::IApplicationManager::setInstance(nullptr); application=nullptr;
        TypeManager::setInstance(nullptr); types.unload();
        return ok ? 0 : 1;
    }
    bool ok = claw_mesh_budget_contracts::run();
    ok &= testParticleLifecycle();
    ok &= claw_scene_node_transform_contracts::run();
    ok &= claw_material_pass_lifetime_contracts::run();
    ok &= claw_scene_lifetime_contracts::run();
    ok &= testLightDirection();
    ok &= claw_primary_light_contracts::run();
    auto window = make_ptr<TestWindow>();
    window->handle = CreateWindowExW(0,L"STATIC",L"Claw production regression",WS_OVERLAPPEDWINDOW,
        0,0,128,128,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if( window->handle )
    {
        ClawRendererDX11 renderer; renderer.load(window);
        ok &= check(renderer.isLoaded(), "required DX11 backend must initialize; this test cannot silently skip");
        if( renderer.isLoaded() )
        {
            auto *native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
            ok &= check( recordDX11TestDevice( static_cast<ID3D11Device *>(
                             wp_renderer_dx11_get_device( native ) ) ),
                         "DX11 device evidence must be available" );
            ok &= claw_mesh_budget_contracts::runRendered(renderer);
            ok &= testRenderedFeatures(renderer);
            ok &= testMaterialUploadWork(renderer);
            ok &= testRenderedFeatures(renderer); // Run the same pixel contracts with split constants.
            ok &= testRuntimeMaterialProgram(renderer);
            ok &= claw_material_reload_contracts::run(renderer);
            ok &= claw_shader_file_watch_contracts::run(renderer);
            ok &= claw_shader_schema_contracts::run(renderer);
            ok &= claw_shader_device_contracts::run(renderer);
            ok &= testPreparedMaterialEdits(renderer);
            ok &= testSkyMaterialPublications(renderer);
            ok &= testIdleMaterialRetirement(renderer);
            ok &= testSortedTransparency(renderer);
            ok &= testMeshShadows(renderer);
            ok &= claw_terrain_contracts::run(renderer);
            ok &= claw_terrain_material_contracts::run(renderer);
            ok &= claw_terrain_streaming_contracts::run(renderer);
            ok &= claw_terrain_motion_contracts::run(renderer);
            ok &= claw_foliage_contracts::run(renderer);
            ok &= claw_foliage_scene_contracts::run(renderer);
            ok &= claw_foliage_world_contracts::run(renderer);
            ok &= claw_animation_contracts::run(renderer);
            try {
                ok &= testSceneProfile(window);
                claw_foliage_scene_contracts::Fixture services;
                ok &= claw_rtt_contracts::integration(window, true, true);
            } catch(const std::exception &e) { ok &= check(false,e.what()); }
        }
        renderer.unload(nullptr);
        ok &= testSkyEnvironmentOwnerRetirement( window );
        DestroyWindow(window->handle);
    }
    else ok = check(false,"hidden window must initialize");
    window=nullptr;
    core::IApplicationManager::setInstance(nullptr); application=nullptr;
    TypeManager::setInstance(nullptr); types.unload();
    return ok ? 0 : 1;
}
