#include <WPAudio/WPAudioPCH.hpp>
#include <WPAudio/AudioResourceCompiler.hpp>
#include <workphone_audio_core.h>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace workphone::audio
{
    namespace
    {
        constexpr u64 maximumBytes=64ull*1024*1024;
        double sample(const u8 *value,u16 tag,u16 bits)
        {
            if(tag==3)
            {
                const u32 raw=u32(value[0])|(u32(value[1])<<8)|(u32(value[2])<<16)|(u32(value[3])<<24);
                float result;std::memcpy(&result,&raw,sizeof(result));return result;
            }
            if(bits==8)return (int(value[0])-128)/128.0;
            u32 raw=0;for(u16 i=0;i<bits/8;++i)raw|=u32(value[i])<<(8*i);
            const auto signedValue=(raw&(u32(1)<<(bits-1)))?s64(raw)-(s64(1)<<bits):s64(raw);
            return double(signedValue)/double(u64(1)<<(bits-1));
        }
        void integer(u8 *out,u32 value,size_t count)
        {for(size_t i=0;i<count;++i)out[i]=u8(value>>(8*i));}
        bool sourceInfo(const Array<u8> &bytes,wp_audio_wav_info &info,String &error)
        {
            info={};info.size=sizeof(info);
            if(bytes.size()>maximumBytes||wp_audio_wav_inspect(bytes.data(),bytes.size(),&info)!=WP_AUDIO_OK)
            {error="Audio requires valid mono/stereo PCM8/16/24/32 or float32 WAVE, 8-192 kHz, at most 64 MiB";return false;}
            return true;
        }
        bool validateBytes(const Array<u8> &bytes,String &error)
        {
            wp_audio_wav_info info{}; info.size=sizeof(info);
            if(bytes.size()>64u*1024u*1024u || wp_audio_wav_inspect(bytes.data(),bytes.size(),&info)!=WP_AUDIO_OK ||
                info.format_tag!=1 || info.bits_per_sample!=16 || (info.channels!=1&&info.channels!=2) ||
                info.sample_rate<8000 || info.sample_rate>192000 || !info.data_bytes)
            { error="Cooked audio requires nonempty mono/stereo PCM16 WAVE, 8-192 kHz, at most 64 MiB"; return false; }
            return true;
        }
    }
    String AudioResourceCompiler::name() const { return "WorkphonePcmWave"; }
    Array<resource::CompilerOutput> AudioResourceCompiler::outputs() const
    { return {{resource::ResourceTypeID("wav"),2}}; }
    resource::CompilationStatus AudioResourceCompiler::compile(const resource::CompileContext &context,
        std::ostream &output,Array<String> &messages) const
    {
        std::ifstream input(std::filesystem::u8path(context.sourcePath.c_str()),std::ios::binary|std::ios::ate);
        const auto size=input?input.tellg():std::streampos(-1);
        if(size<12 || size>64u*1024u*1024u) { messages.push_back("Invalid audio source size"); return resource::CompilationStatus::Failure; }
        String admissionError;auto working=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,static_cast<u64>(size)+65536,admissionError);
        if(!working){messages.push_back(admissionError);return resource::CompilationStatus::Failure;}
        Array<u8> bytes(static_cast<size_t>(size)); input.seekg(0);
        input.read(reinterpret_cast<char*>(bytes.data()),bytes.size()); String error;wp_audio_wav_info info{};
        if(!input || !sourceInfo(bytes,info,error)) { messages.push_back(error.empty()?"Cannot read audio source":error); return resource::CompilationStatus::Failure; }
        const u64 sampleCount=info.data_bytes/(info.bits_per_sample/8),pcmBytes=sampleCount*2;
        if(pcmBytes>maximumBytes-44){messages.push_back("Canonical PCM16 audio exceeds the 64 MiB runtime limit");return resource::CompilationStatus::Failure;}
        std::array<u8,44> header{};
        std::memcpy(header.data(),"RIFF",4);integer(header.data()+4,u32(pcmBytes+36),4);
        std::memcpy(header.data()+8,"WAVEfmt ",8);integer(header.data()+16,16,4);
        integer(header.data()+20,1,2);integer(header.data()+22,info.channels,2);
        integer(header.data()+24,info.sample_rate,4);integer(header.data()+28,info.sample_rate*info.channels*2,4);
        integer(header.data()+32,info.channels*2,2);integer(header.data()+34,16,2);
        std::memcpy(header.data()+36,"data",4);integer(header.data()+40,u32(pcmBytes),4);
        output.write(reinterpret_cast<const char*>(header.data()),header.size());
        std::array<u8,65536> block{};const auto *source=bytes.data()+info.data_offset;u64 clipped=0;
        for(u64 at=0;at<sampleCount;)
        {
            const auto count=std::min<u64>(block.size()/2,sampleCount-at);
            for(u64 i=0;i<count;++i)
            {
                const double value=sample(source+(at+i)*(info.bits_per_sample/8),info.format_tag,info.bits_per_sample);
                if(value< -1.0||value>32767.0/32768.0)++clipped;
                const auto quantized=static_cast<s32>(std::round(std::clamp(value,-1.0,32767.0/32768.0)*32768.0));
                integer(block.data()+i*2,static_cast<u16>(quantized),2);
            }
            output.write(reinterpret_cast<const char*>(block.data()),count*2);if(!output)break;at+=count;
        }
        if(!output){messages.push_back("Cannot write canonical PCM16 audio payload");return resource::CompilationStatus::Failure;}
        if(clipped){messages.push_back("Audio source exceeded PCM16 full scale; affected samples were clipped");return resource::CompilationStatus::SuccessWithWarnings;}
        return resource::CompilationStatus::Success;
    }
    bool AudioResourceCompiler::analyze(const Array<u8> &wave,u32 maximumBins,Waveform &out,String &error)
    {
        error.clear();wp_audio_wav_info info{};
        if(!maximumBins||maximumBins>4096){error="Waveform analysis requires 1-4096 bins";return false;}
        if(!sourceInfo(wave,info,error))return false;
        Waveform result;result.sampleRate=info.sample_rate;result.channels=info.channels;result.frames=info.data_bytes/info.block_align;
        const u64 framesPerBin=(result.frames+maximumBins-1)/maximumBins;
        result.bins.reserve(static_cast<size_t>((result.frames+framesPerBin-1)/framesPerBin));
        for(u64 first=0;first<result.frames;first+=framesPerBin)
        {
            WaveformBin bin;bin.firstFrame=first;bin.frames=std::min(framesPerBin,result.frames-first);
            double sums[2]{};
            for(u64 frame=0;frame<bin.frames;++frame)for(u32 channel=0;channel<info.channels;++channel)
            {
                const auto value=sample(wave.data()+info.data_offset+(first+frame)*info.block_align+channel*(info.bits_per_sample/8),info.format_tag,info.bits_per_sample);
                if(!frame){bin.minimum[channel]=float(value);bin.maximum[channel]=float(value);}
                else {bin.minimum[channel]=std::min(bin.minimum[channel],float(value));bin.maximum[channel]=std::max(bin.maximum[channel],float(value));}
                sums[channel]+=value*value;
            }
            for(u32 channel=0;channel<info.channels;++channel)bin.rms[channel]=float(std::sqrt(sums[channel]/bin.frames));
            result.bins.push_back(bin);
        }
        out=std::move(result);return true;
    }
    bool AudioResourceCompiler::validate(const resource::RuntimeResource &value,String &error)
    {
        error.clear();
        const AudioResourceCompiler compiler;
        if(value.header.resourceType!=resource::ResourceTypeID("wav") ||
            value.header.resourceId.type()!=resource::ResourceTypeID("wav") ||
            value.header.resourceId.isSubResource() || !value.header.installDependencies.empty() ||
            !value.header.installArtifacts.empty() || !value.dependencies.empty() ||
            value.header.payloadSize!=value.payload.size() ||
            value.header.payloadHash!=resource::hashBytes(value.payload.data(),value.payload.size()) ||
            value.header.compilerVersion!=compiler.versionFor(resource::ResourceTypeID("wav")))
        { error="Unsupported cooked audio type or version"; return false; }
        return validateBytes(value.payload,error);
    }
}
