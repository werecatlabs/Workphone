#pragma once
#include <WPAudio/AudioResourceStream.hpp>
#include <WPAudio/AudioResourceCompiler.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <Workphone/System/ResourcePayloadStream.hpp>
#include <Workphone/System/ResourceLeafProvider.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <filesystem>
#include <fstream>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <workphone_audio_core.h>

namespace audio_runtime_stream_contracts
{
    using namespace workphone;using namespace workphone::resource;
    inline void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
    inline u8 pattern(u64 byte){return u8((byte*13+17)&255);}
    inline void run(const std::filesystem::path &root)
    {
        namespace fs=std::filesystem;constexpr u32 pcmBytes=2u<<20;
        auto source=root/"source",cooked=root/"cooked",package=root/"package";fs::create_directories(source);
        Array<u8> wav(44+pcmBytes);std::memcpy(wav.data(),"RIFF",4);std::memcpy(wav.data()+8,"WAVEfmt ",8);std::memcpy(wav.data()+36,"data",4);
        auto integer=[&](size_t at,u32 value){for(size_t i=0;i<4;++i)wav[at+i]=u8(value>>(8*i));};
        integer(4,pcmBytes+36);integer(16,16);wav[20]=1;wav[22]=2;integer(24,48000);integer(28,192000);wav[32]=4;wav[34]=16;integer(40,pcmBytes);
        for(u64 i=0;i<pcmBytes;++i)wav[44+i]=pattern(i);
        {std::ofstream file(source/"long.wav",std::ios::binary);file.write(reinterpret_cast<const char*>(wav.data()),wav.size());require(bool(file),"Cannot write PCM range fixture");}wav.clear();
        String error;auto registry=std::make_shared<ResourceCompilerRegistry>();require(registry->registerCompiler(std::make_shared<audio::AudioResourceCompiler>(),&error),error.c_str());
        ResourceSystem author(registry,std::make_shared<ResourceCompilationDatabase>());ResourceSystemConfig config;config.sourceRoot=source.u8string().c_str();config.compiledRoot=cooked.u8string().c_str();
        require(author.initialize(config,error),error.c_str());const ResourceID id("data://long.wav");CompilationOptions options;options.packagedBuild=true;require(author.compile(id,options).succeeded(),"Long PCM fixture must compile");
        require(author.writeRuntimePackage(package.u8string().c_str(),{id},{},error),error.c_str());author.shutdown();
        fs::remove_all(source);fs::remove_all(cooked);
        RuntimeResourceConfig runtimeConfig;runtimeConfig.compiledRoot=package.u8string().c_str();runtimeConfig.manifestPath=(package/"runtime.wprm").u8string().c_str();
        ResourceSystem runtime(nullptr,nullptr);require(runtime.initializeRuntime(runtimeConfig,error),error.c_str());
        auto scheduler=WorldStreamingScheduler::shared();WorldStreamProducerLimits limits;limits.maximumPending=64;limits.maximumReady=64;
        auto mount=ResourcePayloadMount::create(runtimeConfig,scheduler,limits,error);require(bool(mount),error.c_str());RuntimeResourceRangeInfo info;
        require(mount->resourceInfo(id,info,error)&&info.independentlyVerified,"Long PCM streaming requires real independent manifest chunk pins");
        auto factory=audio::AudioResourceStream::leafProviderFactory(mount,error);require(factory&&factory->handles(id),"Large v4 WAV must select its native cold leaf provider");
        auto cancelled=factory->create(mount,id,info,100,error);require(bool(cancelled),error.c_str());
        cancelled->cancel();std::shared_ptr<const RuntimeResource> cancelledResult;
        require(cancelled->poll(cancelledResult,error)==ResourceLeafStatus::Error&&!cancelledResult,"Cancelled cold provider must not publish a resource");
        cancelled->drain();cancelled.reset();
        auto provider=factory->create(mount,id,info,100,error);require(bool(provider),error.c_str());std::shared_ptr<const RuntimeResource> sparse;
        const auto coldDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);ResourceLeafStatus status=ResourceLeafStatus::Pending;
        while(status==ResourceLeafStatus::Pending&&std::chrono::steady_clock::now()<coldDeadline){status=provider->poll(sparse,error);if(status==ResourceLeafStatus::Pending)std::this_thread::sleep_for(std::chrono::milliseconds(1));}
        require(status==ResourceLeafStatus::Ready&&sparse&&sparse->payload.empty()&&sparse->streamData,error.empty()?"Cold PCM provider must finish a sparse resource":error.c_str());
        const auto capture=audio::AudioResourceStream::captureStats(*sparse);require(capture.cold&&capture.bootstrapBytes<=128*1024&&capture.initialVerifiedReadBytes<pcmBytes/2,"Cold PCM metadata/bootstrap must use verified partial reads before any whole-payload load");provider.reset();factory.reset();
        // Separately prove that the optional full-input conversion also sheds
        // its raw owner. That conversion is never counted as cold admission.
        auto raw=runtime.load(id,error);require(bool(raw),error.c_str());auto converted=audio::AudioResourceStream::capture(raw,mount,error);
        require(converted&&converted!=raw&&converted->payload.empty()&&!audio::AudioResourceStream::captureStats(*converted).cold,"Full-input conversion must remain distinguishable from cold range loading");converted.reset();
        // Render a buffered reference through the actual common core, then
        // compare every streamed PCM sample against it, excluding underrun
        // silence (which deliberately does not advance the content cursor).
        Array<float> buffered(pcmBytes/2);
        {
            wp_audio_clip *decoded=nullptr;
            require(wp_audio_clip_decode(raw->payload.data(),raw->payload.size(),&decoded)==WP_AUDIO_OK,"Buffered reference decode");
            std::unique_ptr<wp_audio_clip,decltype(&wp_audio_clip_destroy)> clip(decoded,wp_audio_clip_destroy);
            wp_audio_context_desc description{sizeof(description),48000,1,256};wp_audio_context *created=nullptr;
            require(wp_audio_context_create(&description,&created)==WP_AUDIO_OK,"Buffered reference context");
            std::unique_ptr<wp_audio_context,decltype(&wp_audio_context_destroy)> context(created,wp_audio_context_destroy);
            wp_audio_voice_desc voiceDescription{sizeof(voiceDescription),clip.get(),0,1,0,1,0};wp_audio_voice_handle voice{};
            require(wp_audio_voice_create(context.get(),&voiceDescription,&voice)==WP_AUDIO_OK,"Buffered reference voice");
            for(size_t at=0;at<buffered.size();at+=512)
                require(wp_audio_render(context.get(),buffered.data()+at,256)==WP_AUDIO_OK,"Buffered reference render");
        }
        std::weak_ptr<const RuntimeResource> old=raw;raw.reset();runtime.clearRuntimeCache();runtime.shutdown();require(old.expired(),"Sparse native PCM metadata must not retain the original whole resource");old.reset();
        auto stream=audio::AudioResourceStream::create(sparse,mount,error);require(bool(stream),error.c_str());require(stream->frames()==pcmBytes/4&&stream->residentBytes()<(1u<<20),"PCM ring/bootstrap must remain bounded below one MiB for a multi-MiB clip");
        std::atomic_bool stop{false},done{false},valid{true};std::atomic<u64> consumed{0};
        // This thread calls only the exact SDK-free callback methods used by the
        // native XAudio2 adapter. The owner alone processes disk/chunk demand.
        std::thread callback([&]{while(!stop&&!done){const auto block=stream->ring().acquire(4096);
            if(!block.bytes){done=true;break;}
            if(block.slot==audio::AudioPcmRing::slotCount){for(size_t i=0;i<block.size;++i)if(block.bytes[i])valid=false;std::this_thread::yield();continue;}
            const auto &slot=stream->ring().slot(block.slot);for(size_t i=0;i<block.size;++i)if(block.bytes[i]!=pattern(slot.offset+i))valid=false;
            for(size_t i=0;i<block.size;i+=2){const u32 sample=u32(block.bytes[i])|(u32(block.bytes[i+1])<<8);
                const auto signedSample=sample>=32768?int(sample)-65536:int(sample);
                if(buffered[(slot.offset+i)/2]!=float(signedSample)/32768.0f)valid=false;}
            stream->ring().submitted(block,true);consumed+=block.size;const bool end=block.dataEnd;stream->ring().complete(block.slot,end);if(end)done=true;
        }});
        struct Join {std::atomic_bool &stop;std::thread &thread;~Join(){stop=true;if(thread.joinable())thread.join();}} join{stop,callback};
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(15);
        while(!done&&std::chrono::steady_clock::now()<deadline){stream->process(false);std::this_thread::sleep_for(std::chrono::milliseconds(1));}
        stop=true;callback.join();require(done&&valid&&consumed==pcmBytes,"Concurrent native callback ring must deliver every verified PCM byte in sequence");
        // An endpoint seek is terminal even while loop demand is enabled. Only
        // an explicit restart may return playback to the beginning.
        auto seek=[&](u64 frame){const bool accepted=stream->reset(frame,error);require(accepted,error.c_str());};
        seek(stream->frames());stream->process(true);
        require(stream->ring().stats().finished&&stream->ring().stats().position==pcmBytes&&!stream->ring().acquire(4096).bytes,
            "Seeking the exact PCM endpoint must retain the endpoint without replay or silence");
        seek(0);stream->process(false);
        auto restarted=stream->ring().acquire(4096);require(restarted.slot<audio::AudioPcmRing::slotCount&&stream->ring().slot(restarted.slot).offset==0,
            "Explicit PCM restart must restore the beginning after a terminal seek");
        stream->ring().submitted(restarted,true);stream->ring().complete(restarted.slot,false);
        // Queue the last data block while looping; the callback observes the
        // current loop policy at completion, without relying on a queued EOS.
        seek(stream->frames()-audio::AudioPcmRing::blockBytes/4);
        const auto finalDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);audio::AudioPcmRing::Buffer finalBlock;
        while(std::chrono::steady_clock::now()<finalDeadline){stream->process(true);finalBlock=stream->ring().acquire(4096);
            if(finalBlock.slot<audio::AudioPcmRing::slotCount)break;std::this_thread::sleep_for(std::chrono::milliseconds(1));}
        require(finalBlock.slot<audio::AudioPcmRing::slotCount&&finalBlock.dataEnd,"Looped PCM endpoint must be available before completing the callback");
        stream->ring().submitted(finalBlock,true);stream->ring().complete(finalBlock.slot,true);stream->process(false);
        require(stream->ring().stats().finished&&stream->ring().stats().position==pcmBytes&&!stream->ring().acquire(4096).bytes,
            "Disabling loop after final PCM submission must finish without consuming prefetched loop data");
        // Retire/drain this requester before measuring pressure. A worker may
        // still be dropping its final result alias just after publication.
        stream.reset();
        const auto resultClass=size_t(StreamMemoryClass::ResultCpu);const auto originalBudget=scheduler->stats().limits[resultClass];
        struct Restore {std::shared_ptr<WorldStreamingScheduler> scheduler;u64 limit;~Restore(){String ignored;scheduler->setBudget(StreamMemoryClass::ResultCpu,limit,ignored);}} restore{scheduler,originalBudget};
        require(scheduler->setBudget(StreamMemoryClass::ResultCpu,scheduler->stats().used[resultClass],error),error.c_str());
        stream=audio::AudioResourceStream::create(sparse,mount,error);require(bool(stream),error.c_str());
        const u64 seekByte=5*audio::AudioPcmRing::blockBytes;require(stream->reset(seekByte/4,error),error.c_str());stream->process(false);
        const auto silent=stream->ring().acquire(4096);require(silent.slot==audio::AudioPcmRing::slotCount&&silent.bytes,"Missing PCM range must use preallocated silence");
        for(size_t i=0;i<silent.size;++i)require(silent.bytes[i]==0,"Underrun policy must emit silence");
        require(stream->ring().stats().position==seekByte&&stream->admissionFailures()>0,"Residency pressure must preserve the sought content cursor and report refusal");
        require(scheduler->setBudget(StreamMemoryClass::ResultCpu,originalBudget,error),error.c_str());
        const auto retryDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);bool recovered=false;
        while(!recovered&&std::chrono::steady_clock::now()<retryDeadline){stream->process(false);const auto block=stream->ring().acquire(4096);
            if(block.slot<audio::AudioPcmRing::slotCount){require(stream->ring().slot(block.slot).offset==seekByte,"Seek generation must discard older range completions");for(size_t i=0;i<block.size;++i)require(block.bytes[i]==pattern(seekByte+i),"Recovered seek PCM differs from its sealed payload");stream->ring().submitted(block,true);stream->ring().complete(block.slot,false);recovered=true;}
            else std::this_thread::sleep_for(std::chrono::milliseconds(1));}
        require(recovered,"PCM range retry must recover after pressure is released");
        for(u64 iteration=0;iteration<12;++iteration)
        {
            // Supersede each outstanding demand before its result is consumed.
            const u64 rejectedByte=(iteration%8+3)*audio::AudioPcmRing::blockBytes;
            const u64 acceptedByte=(iteration%7+12)*audio::AudioPcmRing::blockBytes;
            require(stream->reset(rejectedByte/4,error)&&stream->reset(acceptedByte/4,error),error.c_str());
            const auto seekDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);bool ready=false;
            while(!ready&&std::chrono::steady_clock::now()<seekDeadline)
            {
                stream->process(false);const auto block=stream->ring().acquire(4096);
                if(block.slot<audio::AudioPcmRing::slotCount)
                {
                    require(stream->ring().slot(block.slot).offset==acceptedByte,"Cancelled seek must never publish stale offsets");
                    for(size_t i=0;i<block.size;++i)require(block.bytes[i]==pattern(acceptedByte+i),"Repeated sought bytes must agree with buffered source");
                    stream->ring().submitted(block,true);stream->ring().complete(block.slot,false);ready=true;
                }
                else std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            require(ready,"Repeated seek must recover after cancellation");
        }
        require(stream->reset(7*audio::AudioPcmRing::blockBytes/4,error),error.c_str());require(stream->reset(3*audio::AudioPcmRing::blockBytes/4,error),error.c_str());
        const auto before=scheduler->stats().used[size_t(StreamMemoryClass::SourceCpu)];stream.reset();const auto after=scheduler->stats().used[size_t(StreamMemoryClass::SourceCpu)];
        require(before>=after+audio::AudioPcmRing::slotCount*audio::AudioPcmRing::blockBytes,"Native PCM retirement must release its bounded source ring admission");
        auto corrupt=std::make_shared<RuntimeResource>(*sparse);corrupt->header.payloadHash^=1;require(!audio::AudioResourceStream::isSnapshot(*corrupt),"Sparse metadata cannot be rebound to another payload generation");
        sparse.reset();mount.reset();
    }
}
