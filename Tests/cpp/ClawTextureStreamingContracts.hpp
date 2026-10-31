#pragma once
#include <WPGraphics/Resources/ClawTextureStream.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <Workphone/System/ResourcePayloadStream.hpp>
#include <Workphone/System/ResourceClosureStream.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/Thread/ScopedLock.hpp>
#include <Workphone/Graphics/ProjectAssetPolicy.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <d3d11.h>
#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>

namespace claw_texture_streaming_contracts
{
    using namespace workphone;using namespace workphone::resource;using namespace workphone::render;
    namespace fs=std::filesystem;
    inline void require(bool value,const String &message){if(!value)throw std::runtime_error(message.c_str());}
    inline void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
    inline String text(const fs::path &path){return path.generic_u8string().c_str();}
    inline D3D11_TEXTURE2D_DESC description(void *view)
    {
        using Microsoft::WRL::ComPtr;require(view!=nullptr,"Runtime texture must retain a usable fallback view");
        ComPtr<ID3D11Resource> resource;static_cast<ID3D11ShaderResourceView *>(view)->GetResource(resource.GetAddressOf());
        ComPtr<ID3D11Texture2D> texture;require(SUCCEEDED(resource.As(&texture)),"Streamed native view must be a Texture2D");
        D3D11_TEXTURE2D_DESC descriptor{};texture->GetDesc(&descriptor);return descriptor;
    }
    inline u32 width(void *view){return description(view).Width;}
    template<class Fixture> void run(Fixture &fixture)
    {
        auto scheduler=WorldStreamingScheduler::shared();auto native=wp_renderer_get_dx11(fixture.graphics.renderer->getNativeRenderer());
        require(native!=nullptr,"Mip streaming contracts require the existing DX11 fixture");String error;
        unsigned caseIndex=0;
        for(const String target:{String("texture-stream-contract"),String("windows-dx11-dds-bc1-v1"),String("pc-texture-legacy-stream-contract"),String("texture-unmipped-stream-contract")})
        {
            const bool compressedContainer=target=="pc-texture-legacy-stream-contract";
            const bool legacy=compressedContainer||target=="texture-unmipped-stream-contract";
            const bool blockCompressed=target=="windows-dx11-dds-bc1-v1";
            const u32 coarseMip=blockCompressed?6u:8u,coarseWidth=blockCompressed?4u:1u;
            // Cooked artifact names are deliberately long. Keep owned fixture
            // directories short enough for Windows filesystem traversal.
            const auto base=fixture.folder/("mts"+std::to_string(caseIndex++));
            const auto source=base/"authoring"/"source",compiled=base/"authoring"/"compiled",package=base/"package";fs::create_directories(source);
            std::vector<u8> image(18+256*256*4);image[2]=2;image[12]=image[14]=0;image[13]=image[15]=1;image[16]=32;image[17]=0x28;
            for(size_t p=18;p<image.size();p+=4){image[p]=20;image[p+1]=220;image[p+2]=40;image[p+3]=255;}
            {std::ofstream out(source/"leaf.tga",std::ios::binary);out.write(reinterpret_cast<const char *>(image.data()),image.size());require(out.good(),"Mip fixture image write failed");}
            {std::ofstream out(source/"leaf.texres",std::ios::binary);out<<"{\"version\":1,\"source\":\"data://leaf.tga\",\"mipFilter\":\""<<(target=="texture-unmipped-stream-contract"?"none":"colour")<<"\",\"atlasColumns\":1,\"alphaCutoff\":0.5}";require(out.good(),"Mip fixture recipe write failed");}
            if(compressedContainer){auto settings=make_ptr<Properties>();settings->setProperty("assets.compression",1);require(writeProjectAssetPolicy(text(source),settings,error),error);}
            auto author=std::make_shared<ResourceSystem>(fixture.resources->registry(),std::make_shared<ResourceCompilationDatabase>());
            ResourceSystemConfig config;config.sourceRoot=text(source);config.compiledRoot=text(compiled);config.target=target;
            require(author->initialize(config,error),error);CompilationOptions options;options.packagedBuild=true;const ResourceID id("data://leaf.texres");
            const auto report=author->compile(id,options);
            if(!report.succeeded()){String diagnostic="Runtime mip fixture cook failed for "+target;for(const auto &message:report.messages)diagnostic+="; "+message;throw std::runtime_error(diagnostic.c_str());}
            auto complete=author->load(id,error);require(complete!=nullptr,error);CookedTextureData validated;require(decodeCookedTexture(*complete,validated,error),error);validated=CookedTextureData{};
            if(compressedContainer)require(complete->payload.size()>=4&&!std::memcmp(complete->payload.data(),"WPZ1",4),"Legacy container fixture must actually produce whole-payload WPZ1 compression");
            const String identity="f43e54bc-09bd-4d71-b33a-000000000001";
            require(author->writeRuntimePackage(text(package),{id},{{identity,id}},error),error);
            RuntimeResourceConfig runtime;runtime.compiledRoot=text(package);runtime.manifestPath=text(package/"runtime.wprm");runtime.target=target;
            WorldStreamProducerLimits limits;limits.maximumPending=64;limits.maximumActive=2;limits.maximumReady=16;limits.maximumReadyBytes=8ull<<20;
            auto mount=ResourcePayloadMount::create(runtime,scheduler,limits,error);require(mount!=nullptr,error);
            auto snapshot=snapshotRuntimeTexture(complete,mount,error);require(snapshot&&(legacy?!snapshot->payload.empty():isRuntimeTextureSnapshot(*snapshot)&&snapshot->payload.empty()),"Validated mip snapshots must distinguish sparse residency from explicit legacy whole-payload fallback");
            auto mismatched=std::make_shared<RuntimeResource>(*snapshot);++mismatched->header.payloadHash;
            ClawTexture invalid;require(!invalid.loadRuntimeStream(mismatched,mount,target,true,native,error),"A forged sparse texture header must reject before native installation");
            mismatched.reset();
            complete.reset();author->shutdown();author.reset();fs::remove_all(base/"authoring");require(!fs::exists(source)&&!fs::exists(compiled),"Native mip consumption must run with all authoring storage removed");
            auto factory=createRuntimeTextureLeafProviderFactory(mount,error);require(factory!=nullptr,error);
            auto cold=ResourceClosureStream::create(mount,error,factory);require(cold!=nullptr,error);ResourceClosureLimits coldLimits;
            coldLimits.maximumResources=1;coldLimits.maximumDepth=1;coldLimits.maximumResourcePayloadBytes=128ull<<10;coldLimits.maximumClosurePayloadBytes=128ull<<10;
            if(legacy)coldLimits.maximumResourcePayloadBytes=coldLimits.maximumClosurePayloadBytes=8ull<<20;
            snapshot.reset(); // The cold provider cannot reuse any complete input node.
            require(cold->request(1,id,coldLimits,10,error),error);ResourcePayloadResult coldResult;bool coldReady=false;
            const auto coldDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
            while(std::chrono::steady_clock::now()<coldDeadline){if(cold->take(coldResult)){coldReady=true;break;}std::this_thread::sleep_for(std::chrono::milliseconds(1));}
            require(coldReady&&coldResult.resource&&(legacy?!coldResult.resource->payload.empty():coldResult.resource->payload.empty()&&isRuntimeTextureSnapshot(*coldResult.resource)),
                "Cold closure must admit only verified mip metadata/bootstrap even when the logical portable texture exceeds its whole-payload limit");
            snapshot=std::move(coldResult.resource);cold->drain();cold.reset();factory.reset();
            if(legacy){CookedTextureData description,data;require(describeRuntimeTextureSnapshot(*snapshot,description,error)&&decodeCookedTexture(*snapshot,data,error),error);
                require(!isRuntimeTextureSnapshot(*snapshot)&&description.target==target&&description.packagedBuild,"Checked whole-payload fallback must retain strict private semantic identity");
                ClawTexture ordinary;require(data.ddsBytes.empty()?ordinary.uploadCookedMips(data.levels,data.mipSettings,native,error):uploadCookedDds(&ordinary,data,native,error),error);
                require(width(ordinary.getNativeTextureView(native))==256,"Legacy v4 compressed/unmipped texture must retain its existing actual native consumer");continue;}
            ClawTexture texture;require(texture.loadRuntimeStream(snapshot,mount,target,true,native,error),error);
            const auto initial=texture.runtimeMipState();require(initial.mipCount==9&&initial.firstMip==coarseMip&&initial.residentCpuBytes<=65536,"Native texture must bootstrap only its bounded smallest legal native mip tail");
            require(texture.setRuntimeMipDemand(8,0,-10,error),error);auto fallback=texture.getNativeTextureView(native);require(width(fallback)==coarseWidth,"Initial native texture fallback must use its smallest legal native mip base");
            if(blockCompressed)require(description(fallback).MipLevels==3&&initial.residentCpuBytes==24&&initial.residentGpuBytes==24,
                "BC1 fallback must retain and upload exactly the block-aligned 4x4/2x2/1x1 tail");
            const auto coarseLedger=scheduler->stats();
            struct Budgets {std::shared_ptr<WorldStreamingScheduler> scheduler;WorldStreamStats before;~Budgets(){String ignored;scheduler->setBudget(StreamMemoryClass::SourceCpu,before.limits[size_t(StreamMemoryClass::SourceCpu)],ignored);scheduler->setBudget(StreamMemoryClass::Gpu,before.limits[size_t(StreamMemoryClass::Gpu)],ignored);}} budgets{scheduler,scheduler->stats()};
            require(scheduler->setBudget(StreamMemoryClass::SourceCpu,scheduler->stats().used[size_t(StreamMemoryClass::SourceCpu)],error),error);
            require(texture.setRuntimeMipDemand(0,10,10,error),error);require(!texture.processRuntimeStreaming(native,error),"Source pressure must refuse fine-mip staging");
            require(texture.getNativeTextureView(native)==fallback&&width(fallback)==coarseWidth,"Source admission failure must preserve the native bootstrap");
            require(scheduler->setBudget(StreamMemoryClass::SourceCpu,budgets.before.limits[size_t(StreamMemoryClass::SourceCpu)],error),error);
            require(scheduler->setBudget(StreamMemoryClass::Gpu,scheduler->stats().used[size_t(StreamMemoryClass::Gpu)],error),error);
            auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);bool gpuRejected=false;
            while(std::chrono::steady_clock::now()<deadline){if(!texture.processRuntimeStreaming(native,error)&&!texture.runtimeMipState().pending){gpuRejected=true;break;}std::this_thread::sleep_for(std::chrono::milliseconds(1));}
            require(gpuRejected&&texture.runtimeMipState().firstMip==coarseMip&&texture.getNativeTextureView(native)==fallback,"GPU admission failure must keep the previous native tail usable");
            require(scheduler->setBudget(StreamMemoryClass::Gpu,budgets.before.limits[size_t(StreamMemoryClass::Gpu)],error),error);
            const auto await=[&](u32 first){const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(5);while(std::chrono::steady_clock::now()<until){texture.processRuntimeStreaming(native,error);if(texture.runtimeMipState().firstMip==first)return true;std::this_thread::sleep_for(std::chrono::milliseconds(1));}return false;};
            require(await(0),"Asynchronous verified ranges must promote a native texture after pressure clears");require(width(texture.getNativeTextureView(native))==256,"Fine native mip promotion must reach the actual DX11 Texture2D");
            const auto promoted=texture.runtimeMipState();require(promoted.residentCpuBytes>initial.residentCpuBytes&&promoted.residentGpuBytes>initial.residentGpuBytes,"Fine residency must account its CPU tail and GPU allocation");
            require(texture.setRuntimeMipDemand(8,-10,-10,error),error);require(texture.processRuntimeStreaming(native,error),error);const auto demoted=texture.runtimeMipState();
            require(texture.getNativeTextureView(native)==fallback&&demoted.residentCpuBytes==initial.residentCpuBytes&&demoted.residentGpuBytes==initial.residentGpuBytes,"Demotion must release high CPU/GPU tails and reuse the persistent bootstrap");
            texture.unload(nullptr);texture.load(nullptr);require(texture.setRuntimeMipDemand(8,0,0,error),error);fallback=texture.getNativeTextureView(native);require(width(fallback)==coarseWidth,"Unload/load must recreate from retained bootstrap without source IO");
            fs::path artifact;for(const auto &entry:fs::recursive_directory_iterator(package))if(entry.is_regular_file()){CompiledResourceHeader header;String ignored;if(CompiledResourceIO::readHeader(text(entry.path()),header,ignored)&&header.resourceId==id){artifact=entry.path();break;}}
            require(!artifact.empty(),"Mip corruption fixture must locate its single pinned texture artifact");
            const auto at=fs::file_size(artifact)-snapshot->header.payloadSize+1000;
            std::fstream bytes(artifact,std::ios::in|std::ios::out|std::ios::binary);bytes.seekg(at);char original=0;bytes.read(&original,1);require(bytes.good(),"Cannot read owned mip corruption byte");
            struct Restore {std::fstream &file;std::streamoff offset;char original;~Restore(){file.clear();file.seekp(offset);file.write(&original,1);file.flush();}} restore{bytes,static_cast<std::streamoff>(at),original};
            bytes.seekp(at);char corrupt=original^0x40;bytes.write(&corrupt,1);bytes.flush();
            require(texture.setRuntimeMipDemand(0,10,10,error),error);deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
            while(std::chrono::steady_clock::now()<deadline&&!texture.runtimeMipState().failed){texture.processRuntimeStreaming(native,error);std::this_thread::sleep_for(std::chrono::milliseconds(1));}
            require(texture.runtimeMipState().failed&&texture.getNativeTextureView(native)==fallback&&width(fallback)==coarseWidth,"Corrupt async high mip must never replace the last valid native fallback");
            bytes.clear();bytes.seekp(at);bytes.write(&original,1);bytes.flush();texture.unload(nullptr);texture.load(nullptr);require(texture.setRuntimeMipDemand(0,10,10,error),error);
            bool promotedOnRenderOwner=false;
            std::thread renderOwner([&]{ScopedLock lock(fixture.graphics.graphics);const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(5);while(std::chrono::steady_clock::now()<until){texture.getNativeTextureView(native);if(texture.runtimeMipState().firstMip==0){promotedOnRenderOwner=true;break;}std::this_thread::sleep_for(std::chrono::milliseconds(1));}});renderOwner.join();
            require(promotedOnRenderOwner&&width(texture.getNativeTextureView(native))==256,"A distinct render OS owner must stream the same captured mount generation without reopening authoring");
            texture.unload(nullptr);
            texture.load(nullptr);require(texture.setRuntimeMipDemand(0,10,10,error),error);
            std::thread pendingOwner([&]{ScopedLock lock(fixture.graphics.graphics);texture.getNativeTextureView(native);});pendingOwner.join();
            // Joining here is the fixture's render-owner fence. Product actor
            // teardown uses the existing paused Render task plus graphics lock.
            {ScopedLock lock(fixture.graphics.graphics);texture.unload(nullptr);}
            const auto retired=texture.runtimeMipState();const auto ledger=scheduler->stats();
            require(!retired.pending&&retired.residentCpuBytes==initial.residentCpuBytes&&retired.residentGpuBytes==0,
                "Primary teardown after a distinct render-owner fence must release in-flight/high CPU and native GPU residency");
            require(ledger.used[size_t(StreamMemoryClass::WorkingCpu)]<=coarseLedger.used[size_t(StreamMemoryClass::WorkingCpu)]&&
                ledger.used[size_t(StreamMemoryClass::ResultCpu)]<=coarseLedger.used[size_t(StreamMemoryClass::ResultCpu)]&&
                ledger.used[size_t(StreamMemoryClass::SourceCpu)]<=coarseLedger.used[size_t(StreamMemoryClass::SourceCpu)]+16384,
                "Quiesced off-owner texture retirement must drain worker/result leases and release high mip Source storage");
        }
        std::puts("PASS: native portable/DDS mip ranges bootstrap, promote, demote, survive pressure/corruption and transfer render owner");
    }
}
