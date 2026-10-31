#pragma once
#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>
#include <Workphone/System/CompiledResource.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>

namespace workphone::render
{
    struct CookedMeshData
    {
        Array<u8> geometry;
        Array<resource::ResourceID> materials;
        String target;
        bool packagedBuild=false;
        StreamBudget budget;
    };
    /** WPM1 geometry plus optional .mesh.materials JSON array of matres IDs,
     * ordered by submesh. Each bound slot becomes a typed install dependency. */
    class WPGraphics_API MeshResourceCompiler final : public resource::IResourceCompiler
    {
    public:
        String name() const override;
        Array<resource::CompilerOutput> outputs() const override;
        bool getDependencies(const resource::CompileContext &,resource::DependencySet &,String &) const override;
        resource::CompilationStatus compile(const resource::CompileContext &, std::ostream &,
                                             Array<String> &) const override;
        static bool validate(const resource::RuntimeResource &, String &error);
        static bool decode(const resource::RuntimeResource &, CookedMeshData &, String &error);
    };
}
