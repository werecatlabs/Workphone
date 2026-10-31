#pragma once
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>
#include <Workphone/System/CompiledResource.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace workphone
{
    class SceneLoadJob;
    namespace scene { class IGameScene; class IGamePrefab; class GamePrefabManager; }
    namespace resource
    {
        /** Existing actor Properties documents compiled into the existing binary
         * serializer. This first supported set is transform/identity-only graphs;
         * components and external references are rejected until their typed
         * runtime installers are integrated. */
        class WPCore_API SceneResourceCompiler final : public IResourceCompiler
        {
        public:
            String name() const override;
            Array<CompilerOutput> outputs() const override;
            bool getDependencies(const CompileContext &,DependencySet &,String &) const override;
            CompilationStatus compile(const CompileContext &,std::ostream &,Array<String> &) const override;
        };

        class CookedActorGraph;
        WPCore_API std::shared_ptr<const CookedActorGraph> decodeActorGraphResource(
            std::shared_ptr<const RuntimeResource>,String &error);
        /** Owner-thread, synchronous commit through the existing generation-aware
         * SceneLoadJob rollback path. Existing scene-load append semantics apply. */
        WPCore_API bool installCookedSceneGraph(std::shared_ptr<const CookedActorGraph>,
            SmartPtr<scene::IGameScene>,String &error);
        /** Creates an ordinary GamePrefab; createActor continues to remap internal
         * identities through the existing createInstanceData/loadSceneActors path. */
        WPCore_API SmartPtr<scene::IGamePrefab> installCookedPrefabGraph(
            std::shared_ptr<const CookedActorGraph>,SmartPtr<scene::GamePrefabManager>,String &error);
    }
}
