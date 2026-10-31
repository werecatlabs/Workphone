#include <WPAudio/WPAudioPCH.hpp>
#include <WPAudio/AudioResourceCompiler.hpp>
#include <workphone_audio_core.h>
#include <filesystem>
#include <fstream>

namespace workphone::audio
{
    namespace
    {
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
    { return {{resource::ResourceTypeID("wav"),1}}; }
    resource::CompilationStatus AudioResourceCompiler::compile(const resource::CompileContext &context,
        std::ostream &output,Array<String> &messages) const
    {
        std::ifstream input(std::filesystem::u8path(context.sourcePath.c_str()),std::ios::binary|std::ios::ate);
        const auto size=input?input.tellg():std::streampos(-1);
        if(size<12 || size>64u*1024u*1024u) { messages.push_back("Invalid audio source size"); return resource::CompilationStatus::Failure; }
        Array<u8> bytes(static_cast<size_t>(size)); input.seekg(0);
        input.read(reinterpret_cast<char*>(bytes.data()),bytes.size()); String error;
        if(!input || !validateBytes(bytes,error)) { messages.push_back(error.empty()?"Cannot read audio source":error); return resource::CompilationStatus::Failure; }
        output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
        return output?resource::CompilationStatus::Success:resource::CompilationStatus::Failure;
    }
    bool AudioResourceCompiler::validate(const resource::RuntimeResource &value,String &error)
    {
        error.clear();
        const AudioResourceCompiler compiler;
        if(value.header.resourceType!=resource::ResourceTypeID("wav") ||
            value.header.resourceId.type()!=resource::ResourceTypeID("wav") ||
            value.header.compilerVersion!=compiler.versionFor(resource::ResourceTypeID("wav")))
        { error="Unsupported cooked audio type or version"; return false; }
        return validateBytes(value.payload,error);
    }
}
