#ifndef WPGraphicsResourceCompiler_h__
#define WPGraphicsResourceCompiler_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>

namespace workphone::render
{
    struct CookedTextureData;
    /** Decode an authored texres through the compiler's strict descriptor and
     * bounded image path, producing portable BGRA mips without writing a cache.
     * Shared working reservations precede encoded/pixel allocations. When supplied,
     * retainedBudget receives the source reservation that must outlive output.
     * Both output and retainedBudget are unchanged on failure.
     */
    WPGraphics_API bool decodeGraphicsTextureSource( const String &sourceRoot,
                                                     const resource::ResourceID &id,
                                                     CookedTextureData &output, String &error,
                                                     u64 maximumOutputBytes = 32ull * 1024 * 1024,
                                                     StreamBudget *retainedBudget = nullptr );

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
