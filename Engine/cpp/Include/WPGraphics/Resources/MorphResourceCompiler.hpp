#pragma once
#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>
#include <Workphone/System/MorphResource.hpp>
#include <workphone_graphics_mesh.h>

namespace workphone::render
{
    /** Sparse static PNT/PNTC deformation, pinned to the original native mesh.
     * Skin/morph composition is explicitly unsupported by this format version. */
    class WPGraphics_API MorphResourceCompiler final : public resource::IResourceCompiler
    {
    public:
        String name() const override;
        Array<resource::CompilerOutput> outputs() const override;
        bool getDependencies( const resource::CompileContext &, resource::DependencySet &,
                              String & ) const override;
        resource::CompilationStatus compile( const resource::CompileContext &, std::ostream &,
                                             Array<String> & ) const override;
        static bool signature( wp_graphics_mesh *, resource::MorphClipInfo &, String & );
        static bool validateSamples( wp_graphics_mesh *, const resource::MorphClipData &, String & );
    };
}  // namespace workphone::render
