#pragma once
#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>
#include <Workphone/System/CompiledResource.hpp>

namespace workphone::render
{
    /** Portable static WPM1 geometry. Material IDs must be zero until authored
     * material references have explicit typed install-dependency bindings. */
    class WPGraphics_API MeshResourceCompiler final : public resource::IResourceCompiler
    {
    public:
        String name() const override;
        Array<resource::CompilerOutput> outputs() const override;
        resource::CompilationStatus compile(const resource::CompileContext &, std::ostream &,
                                             Array<String> &) const override;
        static bool validate(const resource::RuntimeResource &, String &error);
    };
}
