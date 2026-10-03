#ifndef WP_SCENE_BUILDER_BINDINGS_HPP
#define WP_SCENE_BUILDER_BINDINGS_HPP

#include <EditorPrerequisites.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>

namespace workphone
{
    namespace scene  { class IGameScene; class IGameActor; class IComponent; }
    namespace procedural { class IRoadNetwork; }
    namespace editor
    {

        /**
         * @class SceneBuilderBindings
         * @brief C++ bindings exposing scene / world / prefab / procedural scene operations to Lua.
         *
         * This is the single C++ entry point for all Lua-callable scene-building operations.
         * It wraps IGameSceneBuilder, IGameScene, IGameActor, IComponentSystem, IGamePrefab,
         * IProceduralManager, IWorldGenerator, ICityGenerator, and ITerrainGenerator so that
         * SceneBuilderEditor.lua can drive scene/world authoring without directly including
         * Interface headers.
         *
         * Operations that need runtime implementation (scene save/load, world generator params,
         * city generator params, road → mesh) are stubbed with the binding contract in place
         * and marked deferred.
         *
         * @ref Esoterica/Code/EngineTools/MapEditor/* — scene/map editor reference
         */
        class SceneBuilderBindings : public IScriptReceiver
        {
        public:
            WP_CLASS_REGISTER_DECL;

            SceneBuilderBindings();
            ~SceneBuilderBindings() override;

            // --- Scene lifecycle ------------------------------------------------

            /** Create or reuse the current scene. */
            SmartPtr<scene::IGameScene> createScene( const String &name );
            SmartPtr<scene::IGameScene> getCurrentScene();

            /** Save the scene to a file path. Deferred to runtime serialiser. */
            void saveScene( SmartPtr<scene::IGameScene> scene, const String &path );

            /** Load a scene from a file path. Deferred to runtime. */
            SmartPtr<scene::IGameScene> loadScene( const String &path );

            // --- Actor management ----------------------------------------------

            SmartPtr<scene::IGameActor> addActor(
                SmartPtr<scene::IGameScene> scene,
                const String &actorName,
                const String &actorType );

            void removeActor( SmartPtr<scene::IGameScene> scene,
                            SmartPtr<scene::IGameActor> actor );

            SmartPtr<scene::IGameActor> duplicateActor( SmartPtr<scene::IGameActor> actor );

            // --- Component management -------------------------------------------

            SmartPtr<scene::IComponent> addComponent(
                SmartPtr<scene::IGameActor> actor,
                const String &componentClassName );

            void removeComponent( SmartPtr<scene::IGameActor> actor,
                               SmartPtr<scene::IComponent> component );

            // --- Transform ------------------------------------------------------

            void setActorTransform( SmartPtr<scene::IGameActor> actor,
                                 const Vector3F &position,
                                 const QuaternionF &rotation,
                                 const Vector3F &scale );

            Vector3F    getActorPosition( SmartPtr<scene::IGameActor> actor );
            QuaternionF getActorRotation( SmartPtr<scene::IGameActor> actor );
            Vector3F    getActorScale( SmartPtr<scene::IGameActor> actor );

            // --- Prefab ---------------------------------------------------------

            SmartPtr<scene::IGamePrefab> createPrefab(
                SmartPtr<scene::IGameActor> actor,
                const String &prefabName );

            SmartPtr<scene::IGameActor> instantiatePrefab(
                SmartPtr<scene::IGameScene> scene,
                SmartPtr<scene::IGamePrefab> prefab,
                const Vector3F &position );

            // --- Procedural world ----------------------------------------------

            /** Generate a procedural world (terrain, biome, water). Deferred to runtime. */
            void generateWorld( const String &seed, int worldSize,
                               real_Num seaLevel, real_Num heightScale,
                               const String &biomeParams );

            /** Generate a city. Deferred to runtime. */
            void generateCity( int seed, int radius, int density,
                              int blockSize, int roadWidth );

            /** Build a road network into the mesh generator. Deferred to runtime. */
            void buildRoadNetwork( SmartPtr<procedural::IRoadNetwork> network );

        private:
            s32 setProperty( hash_type, const String & ) override { return 0; }
            s32 getProperty( hash_type, String & ) const override { return 0; }
            s32 setProperty( hash_type, const Parameter & ) override { return 0; }
            s32 setProperty( hash_type, const Parameters & ) override { return 0; }
            s32 setProperty( hash_type, void * ) override { return 0; }
            s32 getProperty( hash_type, Parameter & ) const override { return 0; }
            s32 getProperty( hash_type, Parameters & ) const override { return 0; }
            s32 getProperty( hash_type, void * ) const override { return 0; }
            s32 callFunction( hash_type, const Parameters &, Parameters & ) override { return 0; }
            s32 callFunction( hash_type, SmartPtr<ISharedObject>, Parameters & ) override { return 0; }
        };

    }  // namespace editor
}  // namespace workphone

#endif  // WP_SCENE_BUILDER_BINDINGS_HPP
