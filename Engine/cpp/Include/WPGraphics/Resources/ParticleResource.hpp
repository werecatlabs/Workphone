#pragma once
#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>
#include <Workphone/System/CompiledResource.hpp>
#include <workphone_graphics_particle_simulation.h>

namespace workphone::render
{
    /** Explicit native simulation semantics. Runtime never interprets a template. */
    struct CookedParticleData
    {
        u32 capacity = 0, seed = 0;
        wp_particle_simulation_settings settings{};
        String target;
        bool packagedBuild = false;
        resource::ResourceID material;
    };

    class WPGraphics_API ParticleResourceCompiler final : public resource::IResourceCompiler
    {
    public:
        String name() const override;
        Array<resource::CompilerOutput> outputs() const override;
        bool getDependencies( const resource::CompileContext &, resource::DependencySet &,
                              String & ) const override;
        resource::CompilationStatus compile( const resource::CompileContext &, std::ostream &,
                                             Array<String> & ) const override;
    };

    WPGraphics_API bool decodeCookedParticle( const resource::RuntimeResource &, CookedParticleData &,
                                              String &error );
}  // namespace workphone::render
