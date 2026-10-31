#pragma once
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>
#include <Workphone/System/CompiledResource.hpp>
#include <Workphone/Mesh/MeshSkeleton.hpp>

namespace workphone::resource
{
    /** Typed, self-contained skeleton/linear clip resource using the existing
     * XMLSkeletonSerializer schema. Linked rigs, base animations and scale
     * channels unsupported by IBone are rejected, never silently approximated. */
    class WPCore_API SkeletonResourceCompiler final : public IResourceCompiler
    {
    public:
        String name() const override;
        Array<CompilerOutput> outputs() const override;
        CompilationStatus compile(const CompileContext &,std::ostream &,Array<String> &) const override;
        static bool validateSource(const String &,String &error);
        static SmartPtr<MeshSkeleton> decode(const RuntimeResource &,String &error);
    };
}
