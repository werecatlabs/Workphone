#pragma once
#include <Workphone/System/AnimationResourceCompiler.hpp>
#include <Workphone/Animation/IK/AnimationIKSystem.hpp>

namespace workphone::resource
{
    struct WPCore_API RigAnimationResource
    {
        SmartPtr<MeshSkeleton> skeleton;
        std::shared_ptr<const RuntimeResource> rig;
        animation::AnimationGraphDefinition graph;
        Array<AnimationGraphParameter> parameters=makeSourceBudgetSnapshot<AnimationGraphParameter>();
        Array<SmartPtr<IAnimation>> clips=makeSourceBudgetSnapshot<SmartPtr<IAnimation>>();
        animation::AnimationIKSystem ik;
    };
    /** Existing native post-animation two-bone IK, independently owned by each
     * Animator. No recursive rig resources or humanoid solver are supported. */
    class WPCore_API RigResourceCompiler final : public IResourceCompiler
    {
    public:
        String name() const override;
        Array<CompilerOutput> outputs() const override;
        bool getDependencies(const CompileContext &,DependencySet &,String &) const override;
        CompilationStatus compile(const CompileContext &,std::ostream &,Array<String> &) const override;
        static bool decode(const RuntimeResource &,RigAnimationResource &,String &);
        static bool animationIdentity(const RuntimeResource &,ResourceID &,String &);
    };
}
