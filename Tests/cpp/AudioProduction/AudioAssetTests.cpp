#include <WPAudio/AudioResourceCompiler.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include "../AudioRuntimeStreamContracts.hpp"
#include <workphone_audio_core.h>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <limits>

namespace
{
    using namespace workphone;
    using namespace workphone::resource;
    using audio::AudioResourceCompiler;
    using audio_runtime_stream_contracts::require;
    void integer(Array<u8> &bytes,size_t at,u32 value,size_t count)
    {for(size_t i=0;i<count;++i)bytes[at+i]=u8(value>>(i*8));}
    Array<u8> fixture(u16 bits,u16 tag,u16 channels)
    {
        constexpr size_t frames=6;const auto stride=bits/8;
        const size_t size=frames*channels*stride;
        Array<u8> bytes(44+size+(size&1));
        std::memcpy(bytes.data(),"RIFF",4);integer(bytes,4,u32(bytes.size()-8),4);
        std::memcpy(bytes.data()+8,"WAVEfmt ",8);integer(bytes,16,16,4);
        integer(bytes,20,tag,2);integer(bytes,22,channels,2);integer(bytes,24,48000,4);
        integer(bytes,28,48000*channels*stride,4);integer(bytes,32,channels*stride,2);integer(bytes,34,bits,2);
        std::memcpy(bytes.data()+36,"data",4);integer(bytes,40,u32(size),4);
        const float values[]={-1,-0.5f,0,0.25f,0.5f,0.75f};
        for(size_t frame=0;frame<frames;++frame)for(size_t channel=0;channel<channels;++channel)
        {
            const auto value=values[(frame+channel)%frames];u32 raw=0;
            if(tag==3)std::memcpy(&raw,&value,4);
            else if(bits==8)raw=u32(value*128+128);
            else raw=static_cast<u32>(static_cast<s64>(double(value)*double(u64(1)<<(bits-1))));
            integer(bytes,44+(frame*channels+channel)*stride,raw,stride);
        }
        return bytes;
    }
    void compilerContracts(const std::filesystem::path &root)
    {
        std::filesystem::create_directories(root);
        AudioResourceCompiler compiler;
        for(u16 channels:{u16(1),u16(2)})for(u16 bits:{u16(8),u16(16),u16(24),u16(32)})for(u16 tag:{u16(1),u16(3)})
        {
            if(tag==3&&bits!=32)continue;
            auto wave=fixture(bits,tag,channels);
            const auto path=root/"source.wav";
            {std::ofstream file(path,std::ios::binary|std::ios::trunc);file.write(reinterpret_cast<const char*>(wave.data()),wave.size());require(bool(file),"write source fixture");}
            CompileContext context;context.sourcePath=path.u8string().c_str();context.resourceId=ResourceID("data://source.wav");
            Array<String> messages;std::ostringstream output(std::ios::binary);
            require(compiler.compile(context,output,messages)==CompilationStatus::Success,"PCM/float source must cook");
            const auto cooked=output.str();wp_audio_wav_info info{};info.size=sizeof(info);
            require(wp_audio_wav_inspect(cooked.data(),cooked.size(),&info)==WP_AUDIO_OK&&info.bits_per_sample==16&&info.format_tag==1&&info.channels==channels&&info.sample_rate==48000,"Cooker must produce canonical PCM16 with original layout/rate");
            const auto expected=fixture(16,1,channels);
            require(cooked.size()==expected.size()&&std::memcmp(cooked.data(),expected.data(),cooked.size())==0,"Quantization must match exact representable PCM samples");
            RuntimeResource resource;resource.header.resourceId=context.resourceId;resource.header.resourceType=context.resourceId.type();
            resource.header.compilerVersion=compiler.versionFor(resource.header.resourceType);resource.payload.resize(cooked.size());
            std::memcpy(resource.payload.data(),cooked.data(),cooked.size());
            resource.header.payloadSize=resource.payload.size();resource.header.payloadHash=hashBytes(resource.payload.data(),resource.payload.size());String error;
            require(compiler.validate(resource,error),error.c_str());
            AudioResourceCompiler::Waveform analysis;
            require(compiler.analyze(wave,3,analysis,error),error.c_str());
            require(analysis.channels==channels&&analysis.frames==6&&analysis.sampleRate==48000&&analysis.bins.size()==3,"Bounded analysis metadata");
            require(analysis.bins[0].firstFrame==0&&analysis.bins[0].frames==2&&analysis.bins[2].firstFrame==4,"Waveform bins preserve frame boundaries");
            require(analysis.bins[0].minimum[0]==-1&&analysis.bins[0].maximum[0]==-0.5f&&std::abs(analysis.bins[0].rms[0]-std::sqrt(0.625f))<1e-6f,"Waveform extrema/RMS use samples");
            auto invalid=wave;invalid[0]=0;
            require(!compiler.analyze(invalid,3,analysis,error)&&analysis.frames==6,"Failed analysis retains old result");
            require(!compiler.analyze(wave,0,analysis,error)&&!compiler.analyze(wave,4097,analysis,error),"Waveform bin limits");
        }
        // Float clipping and nonfinite rejection are explicit import policies.
        auto wave=fixture(32,3,1);float high=2,low=-2;u32 raw;
        std::memcpy(&raw,&high,4);integer(wave,44,raw,4);std::memcpy(&raw,&low,4);integer(wave,48,raw,4);
        const float positive=0.1f,negative=-0.1f;
        std::memcpy(&raw,&positive,4);integer(wave,52,raw,4);std::memcpy(&raw,&negative,4);integer(wave,56,raw,4);
        auto path=root/"float.wav";
        auto write=[&]{std::ofstream file(path,std::ios::binary|std::ios::trunc);file.write(reinterpret_cast<const char*>(wave.data()),wave.size());require(bool(file),"write float fixture");};write();
        CompileContext context;context.sourcePath=path.u8string().c_str();context.resourceId=ResourceID("data://float.wav");
        Array<String> messages;std::ostringstream output(std::ios::binary);
        require(compiler.compile(context,output,messages)==CompilationStatus::SuccessWithWarnings&&!messages.empty(),"Finite over-range source is clipped with a diagnostic");
        const auto clipped=output.str();require(u8(clipped[44])==255&&u8(clipped[45])==127&&u8(clipped[46])==0&&u8(clipped[47])==128,"Clip PCM16 full scale");
        for(size_t i=0;i<2;++i){const u32 encoded=u8(clipped[48+i*2])|(u32(u8(clipped[49+i*2]))<<8);
            const auto signedValue=encoded>=32768?int(encoded)-65536:int(encoded);
            require(std::abs(float(signedValue)/32768.0f-(i?negative:positive))<=0.5f/32768,"Float quantization is within one half PCM16 step");}
        integer(wave,44,0x7fc00000u,4);write();std::ostringstream rejected(std::ios::binary);
        require(compiler.compile(context,rejected,messages)==CompilationStatus::Failure&&rejected.str().empty(),"Nonfinite source must fail before publishing any bytes");
    }
    void generationContracts(const std::filesystem::path &root)
    {
        namespace fs=std::filesystem;
        const auto source=root/"source",cooked=root/"cooked";fs::create_directories(source);
        auto registry=std::make_shared<ResourceCompilerRegistry>();String error;
        require(registry->registerCompiler(std::make_shared<AudioResourceCompiler>(),&error),error.c_str());
        ResourceSystem author(registry,std::make_shared<ResourceCompilationDatabase>());
        ResourceSystemConfig config;config.sourceRoot=source.u8string().c_str();config.compiledRoot=cooked.u8string().c_str();
        require(author.initialize(config,error),error.c_str());const ResourceID id("data://reload.wav");CompilationOptions options;options.packagedBuild=true;
        Array<std::shared_ptr<const RuntimeResource>> pinned;
        for(u32 generation=0;generation<3;++generation)
        {
            auto wave=fixture(16,1,1);integer(wave,44,generation*4096,2);
            {std::ofstream file(source/"reload.wav",std::ios::binary|std::ios::trunc);file.write(reinterpret_cast<const char*>(wave.data()),wave.size());require(bool(file),"write reimport fixture");}
            require(author.compile(id,options).succeeded(),"Audio generation recompiles");
            const auto package=root/("package"+std::to_string(generation));
            require(author.writeRuntimePackage(package.u8string().c_str(),{id},{},error),error.c_str());
            RuntimeResourceConfig runtimeConfig;runtimeConfig.compiledRoot=package.u8string().c_str();runtimeConfig.manifestPath=(package/"runtime.wprm").u8string().c_str();
            ResourceSystem runtime(nullptr,nullptr);require(runtime.initializeRuntime(runtimeConfig,error),error.c_str());
            auto resource=runtime.load(id,error);require(bool(resource)&&AudioResourceCompiler::validate(*resource,error),error.c_str());
            pinned.push_back(resource);runtime.shutdown();
            if(generation)require(pinned[generation]->header.sourceHash!=pinned[generation-1]->header.sourceHash&&pinned[generation]->header.payloadHash!=pinned[generation-1]->header.payloadHash,"Reimport must publish a distinct immutable generation");
        }
        {std::ofstream file(source/"reload.wav",std::ios::binary|std::ios::trunc);file<<"broken";}
        require(!author.compile(id,options).succeeded(),"Failed reimport must report failure");author.shutdown();
        fs::remove_all(source);fs::remove_all(cooked);
        for(size_t generation=0;generation<pinned.size();++generation)
        {
            require(AudioResourceCompiler::validate(*pinned[generation],error),"Old pinned generations must survive reimport, malformed replacement and source removal");
            require(pinned[generation]->payload[44]==0&&pinned[generation]->payload[45]==generation*16,"Existing generation sample data must not change");
        }
    }
}
int main()
{
    namespace fs=std::filesystem;
    workphone::TypeManager types;types.load();workphone::TypeManager::setInstance(&types);
    struct EngineCleanup
    {
        workphone::TypeManager &types;
        ~EngineCleanup(){workphone::core::IApplicationManager::setInstance(nullptr);workphone::TypeManager::setInstance(nullptr);types.unload();}
    } engineCleanup{types};
    auto application=workphone::make_ptr<workphone::core::ApplicationManager>();
    workphone::core::IApplicationManager::setInstance(application);
    const auto root=fs::temp_directory_path()/("wp_audio_assets_"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup {fs::path root;~Cleanup(){std::error_code error;fs::remove_all(root,error);}} cleanup{root};
    try
    {
        compilerContracts(root/"compiler");
        generationContracts(root/"generations");
        audio_runtime_stream_contracts::run(root/"streams");
        std::puts("Audio source cooking, analysis and packaged PCM stream contracts passed (no device)");
        return 0;
    }
    catch(const std::exception &error){std::fprintf(stderr,"%s\n",error.what());return 1;}
}
