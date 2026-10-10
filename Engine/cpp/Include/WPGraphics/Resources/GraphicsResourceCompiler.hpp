#ifndef WPGraphicsResourceCompiler_h__
#define WPGraphicsResourceCompiler_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>

namespace workphone::render
{
    /** First-party texres/matres descriptor compiler for the existing registry.
     * Both JSON descriptors require version:1. texres requires source:data://...,
     * mipFilter (none/colour/data/normal/cutout/roughness), atlasColumns and
     * alphaCutoff. matres requires baseColour:[r,g,b,a], metalness, roughness and
     * baseColourTexture:data://...texres. Unknown/duplicate fields are rejected.
     */
    class WPGraphics_API GraphicsResourceCompiler final : public resource::IResourceCompiler
    {
    public:
        String name() const override;
        Array<resource::CompilerOutput> outputs() const override;
        bool getDependencies( const resource::CompileContext &context,
                              resource::DependencySet &dependencies, String &error ) const override;
        resource::CompilationStatus compile( const resource::CompileContext &context,
                                             std::ostream &output,
                                             Array<String> &messages ) const override;
    };
}  // namespace workphone::render

#endif
