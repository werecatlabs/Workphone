#pragma once
#include "RuntimeSceneMembershipContracts.hpp"
#include "RuntimeComponentMetadataContracts.hpp"
#include <Workphone/Scene/Components/ParticleSystem.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/System/RuntimeAssetContext.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Thread/ScopedLock.hpp>
#include <WPGraphics/Resources/ParticleResource.hpp>
#include <WPGraphics/Particle/CParticleSystem.hpp>
#include <WPGraphics/ClawScene.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <WorkphoneGraphics/workphone_graphics_renderer.h>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <stdexcept>

namespace runtime_particle_component_contracts
{
    using namespace workphone;
    inline void require(bool value,const String &message){if(!value)throw std::runtime_error(message.c_str());}
    template<class F>bool rejects(F action){try{action();}catch(const std::exception &){return true;}return false;}
    inline void writeSources(const std::filesystem::path &folder)
    {
        const String data=R"({"version":1,"capacity":128,"seed":7,"rate":32,"lifetime":[1,1],"size":[0.5,0.5],"velocity":[0,0,0],"gravity":[0,0,0],"colourStart":[1,1,1,1],"colourEnd":[1,1,1,1],"duration":0,"looping":true,"material":"data://surface.matres"})";
        const auto path=folder/"effect.partres";
        const auto write=[&](const String &value){std::ofstream file(path,std::ios::binary);file.write(value.data(),value.size());require(bool(file),"Cannot write semantic particle fixture");};
        write(data);
        String error,capturedRoot;require(canonicalAssetCatalogRoot(folder.generic_u8string().c_str(),capturedRoot,error),error);
        resource::CompileContext context;context.sourceRoot=capturedRoot;context.sourcePath=path.generic_u8string().c_str();context.resourceId=resource::ResourceID("data://effect.partres");
        render::ParticleResourceCompiler compiler;resource::DependencySet dependencies;
        require(compiler.getDependencies(context,dependencies,error),error);require(dependencies.installDependencies==Array<resource::ResourceID>{resource::ResourceID("data://surface.matres")},"Particle source must expose its typed material dependency");
        write(data.substr(0,data.size()-1)+",\"rate\":1}");require(!compiler.getDependencies(context,dependencies,error)&&error.find("Duplicate")!=String::npos,"Particle descriptors must reject duplicate semantic fields");
        write(data);
    }
    inline size_t greenPixels(wp_renderer_dx11 *native)
    {
        using Microsoft::WRL::ComPtr;auto device=static_cast<ID3D11Device *>(wp_renderer_dx11_get_device(native));auto context=static_cast<ID3D11DeviceContext *>(wp_renderer_dx11_get_context(native));
        ComPtr<ID3D11RenderTargetView> view;context->OMGetRenderTargets(1,view.GetAddressOf(),nullptr);require(view!=nullptr,"Particle colour target must be bound");
        ComPtr<ID3D11Resource> resource;view->GetResource(resource.GetAddressOf());ComPtr<ID3D11Texture2D> texture;require(SUCCEEDED(resource.As(&texture)),"Particle target must be a Texture2D");
        D3D11_TEXTURE2D_DESC description{};texture->GetDesc(&description);require(description.Format==DXGI_FORMAT_R8G8B8A8_UNORM,"Particle fixture requires RGBA8");
        description.Usage=D3D11_USAGE_STAGING;description.BindFlags=0;description.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> staging;
        require(SUCCEEDED(device->CreateTexture2D(&description,nullptr,staging.GetAddressOf())),"Particle readback allocation failed");context->CopyResource(staging.Get(),texture.Get());D3D11_MAPPED_SUBRESOURCE mapped{};
        require(SUCCEEDED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped)),"Particle pixels cannot be mapped");size_t green=0;
        for(UINT y=0;y<description.Height;++y)for(UINT x=0;x<description.Width;++x){const auto pixel=static_cast<const unsigned char *>(mapped.pData)+y*mapped.RowPitch+x*4;green+=pixel[1]>32&&pixel[1]>pixel[0]*2&&pixel[1]>pixel[2]*2;}
        context->Unmap(staging.Get(),0);return green;
    }
    inline void run(SmartPtr<core::IApplicationManager> app,std::shared_ptr<const resource::RuntimeAssetContext> context,
                    const String &particleUuid,const String &wrongUuid,const String &scriptUuid={})
    {
        require(app&&context&&context->resolve&&context->mount,"Particle contracts require the captured Editor package context");
        auto manager=app->getGameManager();auto graphics=app->getGraphicsSystem();auto renderer=dynamic_pointer_cast<render::ClawRendererDX11>(graphics->getRenderer());auto nativeScene=dynamic_pointer_cast<render::ClawScene>(graphics->getGraphicsScene());
        require(manager&&renderer&&nativeScene,"Particle contracts require the existing native Editor renderer");ScopedLock managerLock(manager);ScopedLock graphicsLock(graphics);String error;
        auto resource=context->resolve(particleUuid,error);require(resource!=nullptr,error);render::CookedParticleData decoded;require(render::decodeCookedParticle(*resource,decoded,error),error);
        require(decoded.capacity==128&&decoded.seed==7&&decoded.settings.rate==32&&decoded.material==resource::ResourceID("data://surface.matres"),"Particle payload must carry actual bounded numeric simulation semantics");
        auto damaged=std::make_shared<resource::RuntimeResource>(*resource);damaged->payload.back()^=1;require(!render::decodeCookedParticle(*damaged,decoded,error),"Particle payload corruption must reject before native allocation");
        damaged=std::make_shared<resource::RuntimeResource>(*resource);const size_t rate=16+context->target.size()+4+4+4;const u32 infinity=0x7f800000;
        for(unsigned i=0;i<4;++i)damaged->payload[rate+i]=u8(infinity>>(8*i));damaged->header.payloadHash=resource::hashBytes(damaged->payload.data(),damaged->payload.size());
        require(!render::decodeCookedParticle(*damaged,decoded,error),"Sealed particle payload must still reject nonfinite simulation settings");damaged.reset();
        const auto beforeObjects=nativeScene->getGraphicsObjects().size();
        auto actor=manager->createActor();require(actor!=nullptr,"Particle fixture actor must allocate");actor->setName("CapturedParticleComponent");
        struct Cleanup {SmartPtr<scene::IGameManager> manager;SmartPtr<scene::IGameActor> actor;~Cleanup(){manager->destroyActor(actor);}} cleanup{manager,actor};
        auto component=make_ptr<scene::ParticleSystem>();component->configureRuntimeAssets(context);actor->addComponentInstance(component);auto data=make_ptr<Properties>();data->setProperty("particleAssetUuid",particleUuid);data->setProperty("playOnLoad",true);data->setProperty("state",static_cast<s32>(scene::IComponent::State::Edit));component->load(data);
        auto particles=dynamic_pointer_cast<render::CParticleSystem>(component->getParticleSystem());require(particles&&particles->isLoaded()&&particles->usesRuntimeResources()&&particles->getRuntimeMaterial(),"Captured particles must own their native simulator and direct pinned material");
        require(nativeScene->getGraphicsObjects().size()==beforeObjects+1&&particles->getCreator()==nativeScene,"Captured particles must register the exact native system with their owning scene");
        require(!particles->isVisible()&&particles->simulate(.25f)&&particles->getNumParticles()==0,"Unpublished staged particle systems must never tick or become visible");
        require(rejects([&]{particles->setTemplateName("must-not-open.particle");})&&rejects([&]{particles->setMaterialName("must-not-look-up");}),"Captured native particles must reject template and named-material fallback");
        manager->getCurrentScene()->addActor(actor);require(component->isPlaying()&&particles->isVisible(),"Scene publication must activate the requested native particle playback");
        runtime_scene_membership_contracts::nested(manager,actor,[&](bool visible){require(particles->isVisible()==visible&&component->isPlaying()==visible,"Nested particle visibility/playback must follow its real scene ancestor without a child InScene bit");});
        auto settings=app->getPlayerSettings();const bool oldGpu=settings&&settings->getPropertyAsBool("performance.gpuParticles");
        struct RestoreGpu {SmartPtr<Properties> settings;bool value;~RestoreGpu(){if(settings)settings->setProperty("performance.gpuParticles",value);}} restoreGpu{settings,oldGpu};if(settings)settings->setProperty("performance.gpuParticles",true);
        require(particles->simulate(.25f)&&particles->getNumParticles()>=4,"Captured simulation rate/lifetime must produce actual native particles");
        require(particles->getGpuSimulationDispatchCount()>0,"Enabled WARP/DX11 particle compute must consume admitted native buffers");
        const auto oldTarget=renderer->getRenderTarget();const auto oldCamera=renderer->getCamera();const auto oldViewport=renderer->getViewport();
        struct RestoreRenderer {SmartPtr<render::ClawRendererDX11> renderer;SmartPtr<render::IRenderTarget> target;SmartPtr<render::IGraphicsCamera> camera;SmartPtr<render::IViewport> viewport;~RestoreRenderer(){renderer->setRenderTarget(target);renderer->setViewport(viewport);renderer->setCamera(camera);}} restoreRenderer{renderer,oldTarget,oldCamera,oldViewport};
        auto target=make_ptr<render::ClawRenderTarget>();target->setSize({64,64});renderer->setRenderTarget(target);renderer->setViewport(nullptr);renderer->setCamera(nullptr);renderer->disableShadows();auto native=wp_renderer_get_dx11(renderer->getNativeRenderer());require(native!=nullptr,"Particle fixture requires native DX11");
        const auto viewport=wp_renderer_dx11_get_viewport(native);require(viewport.x==0&&viewport.y==0&&viewport.width==64&&viewport.height==64,"Particle fixture viewport must cover its own colour target");
        wp_mat4f identity{};for(int i=0;i<4;++i)identity.m[i][i]=1;wp_renderer_dx11_set_world_matrix(native,&identity);wp_renderer_dx11_set_view_matrix(native,&identity);wp_renderer_dx11_set_projection_matrix(native,&identity);wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);
        auto texture=particles->getRuntimeMaterial()->getTexture(0);void *textureView=nullptr;if(texture)texture->getTextureFinal(&textureView);
        require(texture&&textureView,"Captured particle material must retain a loaded native texture binding");
        wp_render_statistics_dx11 beforeDraw{},afterDraw{};wp_renderer_dx11_get_statistics_counters(native,&beforeDraw);
        nativeScene->render(static_cast<render::IRenderer *>(renderer.get()));wp_renderer_dx11_get_statistics_counters(native,&afterDraw);const auto green=greenPixels(native);
        require(afterDraw.draws>beforeDraw.draws&&afterDraw.triangles>beforeDraw.triangles,"Scene particle consumption must submit actual native triangles");
        require(green>32,"Scene particle draws must visibly sample their source-free captured green material (green="+StringUtil::toString(green)+", samples="+StringUtil::toString(particles->getNumParticles())+", draws="+StringUtil::toString(afterDraw.draws-beforeDraw.draws)+", triangles="+StringUtil::toString(afterDraw.triangles-beforeDraw.triangles)+")");
        const auto count=particles->getNumParticles();const auto material=particles->getRuntimeMaterial();void *simulation=nullptr;particles->_getObject(&simulation);
        auto scheduler=WorldStreamingScheduler::shared();const auto before=scheduler->stats();
        struct RestoreBudget {std::shared_ptr<WorldStreamingScheduler> scheduler;WorldStreamStats state;~RestoreBudget(){String ignored;scheduler->setBudget(StreamMemoryClass::SourceCpu,state.limits[size_t(StreamMemoryClass::SourceCpu)],ignored);scheduler->setBudget(StreamMemoryClass::Gpu,state.limits[size_t(StreamMemoryClass::Gpu)],ignored);}} restoreBudget{scheduler,before};
        require(scheduler->setBudget(StreamMemoryClass::SourceCpu,before.used[size_t(StreamMemoryClass::SourceCpu)],error),error);
        require(!particles->loadRuntimeResource(resource,context->mount,context->target,true,error),"Native particle capacity must admit retained SourceCpu before allocation");
        require(scheduler->setBudget(StreamMemoryClass::SourceCpu,before.limits[size_t(StreamMemoryClass::SourceCpu)],error),error);
        require(scheduler->setBudget(StreamMemoryClass::Gpu,scheduler->stats().used[size_t(StreamMemoryClass::Gpu)],error),error);
        require(!particles->loadRuntimeResource(resource,context->mount,context->target,true,error),"Particle material staging must reject GPU pressure before replacing the live simulator");
        require(scheduler->setBudget(StreamMemoryClass::Gpu,before.limits[size_t(StreamMemoryClass::Gpu)],error),error);void *retained=nullptr;particles->_getObject(&retained);
        require(retained==simulation&&particles->getNumParticles()==count&&particles->getRuntimeMaterial()==material,"Failed replacement must preserve the exact native simulation, particles and material");
        auto invalid=make_ptr<Properties>();invalid->setProperty("particleAssetUuid",wrongUuid);require(rejects([&]{component->setProperties(invalid);})&&component->getParticleSystem()==particles,"Wrong typed particle UUID must preserve the ready system");
        invalid=make_ptr<Properties>();invalid->setProperty("templateName","must-not-load");require(rejects([&]{component->setProperties(invalid);}),"Captured component must reject authoring template properties");
        if(!scriptUuid.empty()){
            auto staged=make_ptr<Properties>();staged->setName("actors");staged->setProperty("uuid",StringUtil::getUUID());staged->setProperty("name","RejectedParticleAppend");
            auto particleData=make_ptr<Properties>();particleData->setName("component");particleData->setProperty("uuid",StringUtil::getUUID());particleData->setProperty("componentType","workphone::scene::ParticleSystem");particleData->setProperty("particleAssetUuid",particleUuid);particleData->setProperty("playOnLoad",true);staged->addChild(particleData);
            auto scriptData=make_ptr<Properties>();scriptData->setName("component");scriptData->setProperty("uuid",StringUtil::getUUID());scriptData->setProperty("componentType","workphone::scene::Script");scriptData->setProperty("scriptAssetUuid",scriptUuid);scriptData->setProperty("className","MissingParticleRollbackClass");staged->addChild(scriptData);
            require(rejects([&]{scene::GameActorUtil::loadSceneActors({staged},manager->getCurrentScene(),context);})&&particles->getNumParticles()==count&&particles->getRuntimeMaterial()==material,"Later Script failure must retire only the staged particle system");
        }
        component->setEnabled(false);auto roundtrip=runtime_component_metadata_contracts::cook(component,"workphone::scene::ParticleSystem",context,{particleUuid});
        texture=nullptr;component->unload(nullptr);particles->_getObject(&retained);require(!retained&&!particles->getRuntimeMaterial()&&nativeScene->getGraphicsObjects().size()==beforeObjects,"Component retirement must destroy native simulation/compute/material and unregister its scene inventory entry");
        require(rejects([&]{component->load(data);}),"Plain runtime reload must not downgrade to the authoring particle path");component->setActor(actor);component->configureRuntimeAssets(context);component->load(roundtrip);require(component->getParticleSystem()&&component->getParticleSystem()->isLoaded()&&!component->isEnabled()&&!component->getParticleSystem()->isVisible(),"Actual toData/cook/reload must consume Enabled=false and recreate hidden source-free native particles");
    }
}
