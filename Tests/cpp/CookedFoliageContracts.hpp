#pragma once
#include "FoliagePageStreamContracts.hpp"
#include "ClawTextureMipContracts.hpp"
#include "ClawFoliageContracts.hpp"
#include "GraphicsTextureSourceContracts.hpp"
#include <WPGraphics/Resources/FoliageResource.hpp>
#include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
#include <WPGraphics/ClawFoliageWorld.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>

namespace cooked_foliage_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    using namespace workphone::resource;
    namespace fs=std::filesystem;
    inline void require(bool ok,const String &message){if(!ok)throw std::runtime_error(message.c_str());}
    inline String path(const fs::path &p){return p.generic_u8string().c_str();}
    inline void text(const fs::path &p,const String &value){std::ofstream f(p,std::ios::binary);f.write(value.data(),std::streamsize(value.size()));require(bool(f),"Foliage fixture write");}
    inline String bytes(const fs::path &p){std::ifstream f(p,std::ios::binary);return String(std::istreambuf_iterator<char>(f),{});}
    inline void image(const fs::path &p)
    {
        std::array<u8,18+4*4*4> data{};data[2]=2;data[12]=data[14]=4;data[16]=32;data[17]=0x28;
        for(size_t i=18;i<data.size();i+=4){data[i+1]=173;data[i+3]=255;}
        std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(data.data()),data.size());require(bool(f),"Foliage fixture TGA write");
    }
    inline void consumePackage(const fs::path &package,const String &uuid)
    {
        // Graphics-only consumer: no catalog, compiler registry or authoring DB.
        claw_texture_mip_contracts::Fixture graphics; graphics.initialize();
        auto &renderer=*graphics.renderer;
        String error; auto runtime=std::make_shared<ResourceSystem>(nullptr,nullptr);
        RuntimeResourceConfig config; config.compiledRoot=path(package);
        config.manifestPath=path(package/"runtime.wprm"); config.target="windows-dx11-dds-bgra8-v1";
        require(runtime->initializeRuntime(config,error),error);
        ResourceID id; require(runtime->resolveRuntimeUUID(uuid,id,error),error);
        auto scheduler=WorldStreamingScheduler::shared(); WorldStreamProducerLimits limits; limits.maximumReadyBytes=4096;
        auto producer=scheduler->createProducer(limits,error); require(producer!=nullptr,error);
        require(queueCookedFoliageWorld(*producer,runtime,id,config.target,true,{},error),error);
        WorldStreamResult prepared; const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(15);
        while(!producer->take(prepared)) { require(std::chrono::steady_clock::now()<deadline,"Checked foliage decode timeout"); std::this_thread::yield(); }
        require(prepared.payload&&prepared.error.empty(),prepared.error);
        auto decoded=std::static_pointer_cast<const DecodedFoliageWorld>(prepared.payload);
        require(decoded->source.snapshot->instances.empty()&&decoded->source.cells.size()==3,"Checked foliage mounts a shallow world directory");
        producer->drain(); producer.reset();
        auto candidate=stageCookedFoliageWorld(renderer,decoded,error); require(candidate!=nullptr,error);
        auto target=make_ptr<ClawRenderTarget>(); target->setSize({128,128}); renderer.setRenderTarget(target);
        renderer.setCamera(nullptr); renderer.setViewport(nullptr); renderer.disableShadows();
        auto native=wp_renderer_get_dx11(renderer.getNativeRenderer()); claw_foliage_contracts::setup(native);
        renderer.setSceneLighting(ColourF::White,{0,0,-1},ColourF::White,0);
        ClawFoliageWorld world; require(world.setSource(candidate->source,candidate->resolver,error),error);
        ClawFoliageView camera; camera.orthographic=true; camera.forcedLod=0; camera.cull=false;
        const auto ready=std::chrono::steady_clock::now()+std::chrono::seconds(15);
        while(world.diagnostics().sourceRevision!=candidate->source.snapshot->revision)
        { require(world.prepare(renderer,{camera},0,error),error); require(std::chrono::steady_clock::now()<ready,"Checked foliage publication timeout"); std::this_thread::yield(); }
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL); require(world.render(renderer,camera,error),error);
        require(world.diagnostics().residentInstances==3&&world.diagnostics().actualDraws>0,"Checked package enters native foliage draws");
        auto pixels=claw_foliage_contracts::pixels(native); size_t green=0;
        for(size_t i=0;i<pixels.size();i+=4) green+=pixels[i+1]>20&&pixels[i+1]>pixels[i]*2;
        require(green>100,"Checked foliage package visibly samples its installed texture");
        world.clear(); renderer.setRenderTarget(nullptr); runtime->shutdown();
        std::puts("PASS: checked UUID foliage package, source-free scheduled page reads and native draws");
    }
    inline void run(ClawRendererDX11 &renderer,const fs::path &parent)
    {
        struct RenderScope{ClawRendererDX11 &renderer;~RenderScope(){renderer.setCamera(nullptr);renderer.setViewport(nullptr);renderer.setRenderTarget(nullptr);renderer.disableShadows();}}renderScope{renderer};
        String error;const auto folder=parent/"foliage-package",source=folder/"source",compiled=folder/"compiled",package=folder/"package";
        fs::create_directories(source);image(source/"leaf.tga");
        text(source/"leaf.texres","{\"version\":1,\"source\":\"data://leaf.tga\",\"mipFilter\":\"cutout\",\"atlasColumns\":1,\"alphaCutoff\":0.5}");
        graphics_texture_source_contracts::run(path(source),ResourceID("data://leaf.texres"));
        auto authored=foliage_page_stream_contracts::source();auto species=std::make_shared<FoliageSpecies>(*authored->species[0]);species->materials[0].albedo="data://leaf.texres";authored->species[0]=species;
        authored->instances[0].position={-.4f,-.5f,.1f};authored->instances[1].position={.4f,-.5f,.1f};authored->instances[2].position={129,-.5f,.1f};
        const ResourceID id("data://forest.folworld");require(writeFoliageSources(authored,path(source),id,error),error);
        const auto previous=bytes(source/"forest.folworld");auto invalid=std::make_shared<FoliageWorldData>(*authored);invalid->instances[0].scale.x=0;
        require(!writeFoliageSources(invalid,path(source),id,error)&&bytes(source/"forest.folworld")==previous,"Failed source export retains published descriptor");
        auto registry=std::make_shared<ResourceCompilerRegistry>();require(registry->registerCompiler(std::make_shared<GraphicsResourceCompiler>(),&error),error);
        require(registry->registerCompiler(std::make_shared<FoliageResourceCompiler>(),&error),error);
        ResourceSystem author(registry,std::make_shared<ResourceCompilationDatabase>());ResourceSystemConfig config;config.sourceRoot=path(source);config.compiledRoot=path(compiled);config.target="windows-dx11-dds-bgra8-v1";
        require(author.initialize(config,error),error);CompilationOptions options;options.packagedBuild=true;auto report=author.compile(id,options);
        if(!report.succeeded())for(const auto &message:report.messages)error+=" "+message;
        require(report.succeeded(),"Foliage cook failed: "+error);
        String crowded = "{\"padding\":["; crowded.reserve(2400016);
        for(size_t n=0;n<1200000;++n) crowded += n ? ",0" : "0";
        crowded += "]}"; text(source/"forest.folworld",crowded);
        auto rejected=author.compile(id,options);bool budgetMessage=false;
        for(const auto &message:rejected.messages) budgetMessage|=message.find("DOM/scratch budget")!=String::npos;
        require(!rejected.succeeded()&&budgetMessage,"Small-wire oversized descriptor DOM is rejected before cJSON allocation");
        require(author.load(id,error)!=nullptr,"Rejected descriptor compile retains the previous cooked closure");
        text(source/"forest.folworld",previous);require(author.compile(id,options).succeeded(),"Restored source revalidates the last good closure");
        require(author.writeRuntimeManifest(path(compiled/"foliage.wprs"),{id},error),error);author.shutdown();
        fs::copy(compiled,package,fs::copy_options::recursive);fs::rename(source,folder/"source-offline");
        auto runtime=std::make_shared<ResourceSystem>(nullptr,nullptr);RuntimeResourceConfig mount;mount.compiledRoot=path(package);mount.manifestPath=path(package/"foliage.wprs");mount.target=config.target;
        require(runtime->initializeRuntime(mount,error),error);
        require(!runtime->loadPayload(id,error,1),"Shallow maximum rejects payload before allocation");
        auto shallow=runtime->loadPayload(id,error);require(shallow&&shallow->dependencies.empty()&&shallow->header.installDependencies.size()==4,"Shallow world retains pins without retaining payload closure");
        fs::path pageFile;for(size_t i=0;i<shallow->header.installDependencies.size();++i)if(shallow->header.installDependencies[i].type()==ResourceTypeID("folpage")){pageFile=package/fs::u8path(shallow->header.installArtifacts[i].path.c_str());break;}
        require(!pageFile.empty(),"World exports separately addressable page resources");const auto hidden=fs::path(pageFile.wstring()+L".offline");fs::rename(pageFile,hidden);
        require(!runtime->load(id,error),"Complete closure load sees the absent page");
        auto scheduler=WorldStreamingScheduler::shared();WorldStreamProducerLimits limits;limits.maximumReadyBytes=4096;
        auto producer=scheduler->createProducer(limits,error);require(producer!=nullptr,error);FoliageDecodeOptions decodeOptions;
        require(queueCookedFoliageWorld(*producer,runtime,id,config.target,true,decodeOptions,error),error);
        WorldStreamResult prepared;const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(15);
        while(!producer->take(prepared)){require(std::chrono::steady_clock::now()<deadline,"Foliage world decode timeout");std::this_thread::yield();}
        require(prepared.payload&&prepared.error.empty(),"World directory decodes while page files are offline: "+prepared.error);
        auto decoded=std::static_pointer_cast<const DecodedFoliageWorld>(prepared.payload);
        require(decoded->source.snapshot->instances.empty()&&decoded->source.cells.size()==3&&decoded->source.snapshot->species.size()==1,"Initial world decode loads shared species and page directory only");
        require(decoded->source.maximumGeometryRadius>1&&decoded->source.maximumBendRadius>=0,
            "Cooked directory retains conservative geometry extents without loading instance pages");
        producer->drain();producer.reset(); // The retained page loader must own its allocator service.
        auto candidate=stageCookedFoliageWorld(renderer,decoded,error);require(candidate!=nullptr,error);
        auto texture=candidate->resolver("data://leaf.texres",error);auto concrete=dynamic_pointer_cast<ClawTexture>(texture);require(concrete!=nullptr,error);
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view=claw_texture_mip_contracts::view(*concrete);require(claw_texture_mip_contracts::greenPixel(view.Get(),0)==173,"Source-free DDS bytes reach native foliage texture");
        FoliagePageStream pages;require(pages.setSource(candidate->source,error),error);
        require(pages.setDemand({{-1,0}},error),error);auto failed=foliage_page_stream_contracts::take(pages);
        require(!failed.page&&!failed.error.empty(),"Demand for offline page reports bounded failure");
        fs::rename(hidden,pageFile);require(pages.setDemand({{-1,0}},error),error);auto restored=foliage_page_stream_contracts::take(pages);
        require(restored.page&&restored.page->instances.size()==1&&restored.page->instances[0].id==1,"Page retry verifies restored pinned generation after submitting producer destruction");
        require(runtime->load(id,error)!=nullptr,"Failed deep load did not cache an incomplete closure");
        auto malformed=bytes(pageFile);require(!malformed.empty(),"Page fixture bytes");malformed.back()^=1;text(pageFile,malformed);
        pages.drain();require(pages.setSource(candidate->source,error)&&pages.setDemand({{-1,0}},error),error);auto corrupt=foliage_page_stream_contracts::take(pages);
        require(!corrupt.page&&restored.page->instances[0].id==1,"Corrupt replacement cannot mutate retained page");malformed.back()^=1;text(pageFile,malformed);pages.drain();
        const auto budgets=scheduler->stats();require(scheduler->setBudget(StreamMemoryClass::Gpu,budgets.used[size_t(StreamMemoryClass::Gpu)],error),error);
        struct Restore{std::shared_ptr<WorldStreamingScheduler> s;u64 value;~Restore(){String error;s->setBudget(StreamMemoryClass::Gpu,value,error);}}restore{scheduler,budgets.limits[size_t(StreamMemoryClass::Gpu)]};
        require(!stageCookedFoliageWorld(renderer,decoded,error)&&claw_texture_mip_contracts::greenPixel(view.Get(),0)==173,"Texture staging budget failure retains previous GPU candidate");
        require(scheduler->setBudget(StreamMemoryClass::Gpu,restore.value,error),error);
        auto target=make_ptr<ClawRenderTarget>();target->setSize({128,128});renderer.setRenderTarget(target);renderer.setCamera(nullptr);renderer.setViewport(nullptr);renderer.disableShadows();
        auto native=wp_renderer_get_dx11(renderer.getNativeRenderer());claw_foliage_contracts::setup(native);renderer.setSceneLighting(ColourF::White,{0,0,-1},ColourF::White,0);
        ClawFoliageWorld world;require(world.setSource(candidate->source,candidate->resolver,error),error);ClawFoliageView camera;camera.orthographic=true;camera.forcedLod=0;camera.cull=false;
        const auto readyBy=std::chrono::steady_clock::now()+std::chrono::seconds(15);
        while(world.diagnostics().sourceRevision!=candidate->source.snapshot->revision)
        {require(world.prepare(renderer,{camera},0,error),error);require(std::chrono::steady_clock::now()<readyBy,"Cooked foliage render publication timeout");std::this_thread::yield();}
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);require(world.render(renderer,camera,error),error);
        require(world.diagnostics().residentInstances==3&&world.diagnostics().actualDraws>0,"Cooked pages enter real native instance draws");
        const auto pixels=claw_foliage_contracts::pixels(native);size_t green=0;for(size_t i=0;i<pixels.size();i+=4)green+=pixels[i+1]>20&&pixels[i+1]>pixels[i]*2;
        require(green>100,"Source-free cooked foliage must visibly sample its texture");world.clear();
        require(!decodeCookedFoliageWorld(runtime,id,"different-target",true,error),"Cooked foliage rejects mismatched target");
        runtime->shutdown();
        std::puts("Cooked foliage: source-free shared DDS/species, on-demand pages, failed/corrupt retry, exact pins, GPU budget retention and native draws passed.");
    }
}
