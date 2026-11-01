#pragma once
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>
#include <Workphone/System/CompiledResource.hpp>
#include <Workphone/Animation/AnimationGraph.hpp>
#include <Workphone/Mesh/MeshSkeleton.hpp>

namespace workphone::resource
{
    struct WPCore_API AnimationGraphParameter
    {
        String name;
        bool boolean=false,booleanValue=false;
        f32 value=0;
    };
    /** Independent decoded rig/clip bindings. Parameters and playback state are
     * installed into each Animator, never shared between component instances. */
    struct WPCore_API AnimationGraphResource
    {
        SmartPtr<MeshSkeleton> skeleton;
        std::shared_ptr<const RuntimeResource> rig;
        animation::AnimationGraphDefinition definition;
        Array<AnimationGraphParameter> parameters=makeSourceBudgetSnapshot<AnimationGraphParameter>();
    };
    class WPCore_API AnimationResourceCompiler final : public IResourceCompiler
    {
    public:
        String name() const override;
        Array<CompilerOutput> outputs() const override;
        bool getDependencies(const CompileContext &,DependencySet &,String &error) const override;
        CompilationStatus compile(const CompileContext &,std::ostream &,Array<String> &) const override;
        static bool decodeGraph(const RuntimeResource &,AnimationGraphResource &,String &error);
        static bool decodeMask(const RuntimeResource &,IAnimationState::BoneBlendMask &,String &error);
    };
}
