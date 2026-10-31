#pragma once
#include <WPAudio/WPAudioPrerequisites.hpp>
#include <WPAudio/AudioPcmRing.hpp>
#include <Workphone/System/CompiledResource.hpp>
#include <memory>

namespace workphone::resource { class ResourcePayloadMount; class ResourceLeafProviderFactory; }
namespace workphone::audio
{
    /** Private-to-the-audio-backend native voice data. Owner pump performs
     * range transport; only ring()'s nonblocking consumer reaches callbacks. */
    class WPAudio_API AudioResourceStream final
    {
    public:
        static std::shared_ptr<const resource::RuntimeResource> capture(
            std::shared_ptr<const resource::RuntimeResource>,
            std::shared_ptr<resource::ResourcePayloadMount>,String &error);
        static bool isSnapshot(const resource::RuntimeResource &);
        struct CaptureStats {bool cold=false;u64 initialVerifiedReadBytes=0,bootstrapBytes=0;};
        static CaptureStats captureStats(const resource::RuntimeResource &);
        static std::shared_ptr<resource::ResourceLeafProviderFactory> leafProviderFactory(
            std::shared_ptr<resource::ResourcePayloadMount>,String &error);
        static std::unique_ptr<AudioResourceStream> create(
            std::shared_ptr<const resource::RuntimeResource>,
            std::shared_ptr<resource::ResourcePayloadMount>,String &error);
        ~AudioResourceStream();
        AudioResourceStream(const AudioResourceStream&)=delete;
        AudioResourceStream &operator=(const AudioResourceStream&)=delete;
        void process(bool loop);
        /** Requires a quiesced native voice/callback. No file IO. */
        bool reset(u64 frame,String &error);
        AudioPcmRing &ring();
        const AudioPcmRing &ring() const;
        const u8 *format() const;
        u32 blockAlign() const;
        u64 frames() const;
        u64 readFailures() const;
        u64 admissionFailures() const;
        u64 residentBytes() const;
        bool owner() const;
    private:
        AudioResourceStream();
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}
