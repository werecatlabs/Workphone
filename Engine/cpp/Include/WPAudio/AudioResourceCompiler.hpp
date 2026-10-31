#pragma once
#include <WPAudio/WPAudioPrerequisites.hpp>
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>
#include <Workphone/System/CompiledResource.hpp>

namespace workphone::audio
{
    /** Version 1: validated, source-free mono/stereo PCM16 WAVE payload. */
    class WPAudio_API AudioResourceCompiler final : public resource::IResourceCompiler
    {
    public:
        String name() const override;
        Array<resource::CompilerOutput> outputs() const override;
        resource::CompilationStatus compile(const resource::CompileContext &,std::ostream &,
            Array<String> &) const override;
        static bool validate(const resource::RuntimeResource &,String &error);
    };
}
