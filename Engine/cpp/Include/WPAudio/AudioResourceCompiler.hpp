#pragma once
#include <WPAudio/WPAudioPrerequisites.hpp>
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>
#include <Workphone/System/CompiledResource.hpp>

namespace workphone::audio
{
    /** Version 2: canonical source-free PCM16 WAVE, cooked from validated
     * PCM8/16/24/32 or float32 source. Rate/channels are preserved; quantization
     * rounds to nearest, clips to [-1, 32767/32768], and applies no dither. */
    class WPAudio_API AudioResourceCompiler final : public resource::IResourceCompiler
    {
    public:
        String name() const override;
        Array<resource::CompilerOutput> outputs() const override;
        resource::CompilationStatus compile(const resource::CompileContext &,std::ostream &,
            Array<String> &) const override;
        static bool validate(const resource::RuntimeResource &,String &error);
        struct WaveformBin
        {
            float minimum[2]{}, maximum[2]{}, rms[2]{};
            u64 firstFrame=0, frames=0;
        };
        struct Waveform
        {
            u32 sampleRate=0, channels=0;
            u64 frames=0;
            Array<WaveformBin> bins;
        };
        /** Control/tool thread only; <=4096 bins and <=64 MiB input. The
         * output is replaced only on success. Accepts the source WAV profile. */
        static bool analyze(const Array<u8> &wave,u32 maximumBins,Waveform &,String &error);
    };
}
