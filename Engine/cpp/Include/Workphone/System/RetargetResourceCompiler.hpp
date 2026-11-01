#pragma once
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>
#include <Workphone/System/CompiledResource.hpp>
#include <Workphone/Mesh/MeshSkeleton.hpp>
#include <Workphone/Interface/Animation/IAnimation.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>

namespace workphone::resource
{
    struct WPCore_API RetargetAnimationResource
    {
        SmartPtr<MeshSkeleton> skeleton;
        std::shared_ptr<const RuntimeResource> targetRig;
        Array<SmartPtr<IAnimation>> clips=makeSourceBudgetSnapshot<SmartPtr<IAnimation>>();
    };
    /** Explicit one-to-one local-bind-space key transfer through the existing
     * skeleton/animation track types. This is not a humanoid or IK solver. */
    class WPCore_API RetargetResourceCompiler final : public IResourceCompiler
    {
    public:
        String name() const override;
        Array<CompilerOutput> outputs() const override;
        bool getDependencies(const CompileContext &,DependencySet &,String &) const override;
        CompilationStatus compile(const CompileContext &,std::ostream &,Array<String> &) const override;
        static bool decode(const RuntimeResource &,RetargetAnimationResource &,String &);
        /** Bounded typed metadata for scene/mesh rig compatibility checks. */
        static bool targetRigIdentity(const RuntimeResource &,ResourceID &,String &);
    };
}
