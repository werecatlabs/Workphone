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
        struct RuntimeAssetContext;
        /** Existing actor Properties documents compiled into the existing binary
         * serializer. Supported components use captured typed UUID references;
         * unsupported types, authoring paths and nested component data fail closed. */
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
        WPCore_API bool actorGraphRequiresScripts(const std::shared_ptr<const CookedActorGraph> &);
        /** Resolver owns only detached UUID/pinned-resource maps. The returned
         * mutable context can stage private native services before publication. */
        WPCore_API std::shared_ptr<RuntimeAssetContext> createActorGraphRuntimeContext(
            std::shared_ptr<const CookedActorGraph>,const String &target,bool packagedBuild,String &error);
        WPCore_API bool validateCookedActorGraphData(SmartPtr<Properties>,bool scene,
            std::shared_ptr<const RuntimeAssetContext>,String &error);
        /** Owner-thread, synchronous commit through the existing generation-aware
         * SceneLoadJob rollback path. Existing scene-load append semantics apply. */
        WPCore_API bool installCookedSceneGraph(std::shared_ptr<const CookedActorGraph>,
            SmartPtr<scene::IGameScene>,String &error);
        WPCore_API bool installCookedSceneGraph(std::shared_ptr<const CookedActorGraph>,
            SmartPtr<scene::IGameScene>,std::shared_ptr<const RuntimeAssetContext>,String &error);
        /** Creates an ordinary GamePrefab; createActor continues to remap internal
         * identities through the existing createInstanceData/loadSceneActors path. */
        WPCore_API SmartPtr<scene::IGamePrefab> installCookedPrefabGraph(
            std::shared_ptr<const CookedActorGraph>,SmartPtr<scene::GamePrefabManager>,String &error);
        WPCore_API SmartPtr<scene::IGamePrefab> installCookedPrefabGraph(
            std::shared_ptr<const CookedActorGraph>,SmartPtr<scene::GamePrefabManager>,
            std::shared_ptr<const RuntimeAssetContext>,String &error);
    }
}
