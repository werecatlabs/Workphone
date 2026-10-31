#pragma once
#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>
#include <Workphone/System/CompiledResource.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include <array>

namespace workphone::render
{
    struct CookedSkinInfluence
    {
        std::array<u16,4> joints{};
        std::array<f32,4> weights{};
    };
    struct CookedSkinnedMeshData
    {
        resource::ResourceID mesh,skeleton;
        String target;
        bool packagedBuild=false;
        Array<CookedSkinInfluence> vertices=makeSourceBudgetSnapshot<CookedSkinInfluence>();
    };
    /** version:1, mesh/skeleton typed ResourceIDs and a vertices array. Each
     * vertex has exactly joints:[u16,u16,u16,u16] and weights:[f,f,f,f]. */
    class WPGraphics_API SkinnedMeshResourceCompiler final : public resource::IResourceCompiler
    {
    public:
        String name() const override;
        Array<resource::CompilerOutput> outputs() const override;
        bool getDependencies(const resource::CompileContext &,resource::DependencySet &,String &) const override;
        resource::CompilationStatus compile(const resource::CompileContext &,std::ostream &,Array<String> &) const override;
        static bool decode(const resource::RuntimeResource &,CookedSkinnedMeshData &,String &error);
    };
}
