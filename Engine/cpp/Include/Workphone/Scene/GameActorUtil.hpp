#ifndef GameActorUtil_h__
#define GameActorUtil_h__

#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Interface/UI/IUIMenuItem.hpp>
#include <Workphone/System/FactoryTemplate.hpp>
#include <Workphone/Math/AABB2.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @brief Utility functions for working with `IActor` instances.
         *
         * The `ActorUtil` class is a collection of static helpers used across the
         * engine for actor serialization, bounding box computation and for creating
         * physics (rigid) meshes from scene actors.
         *
         * All functions operate on `SmartPtr<IGameActor>` instances and commonly
         * interact with `Properties` for (de)serialization. These helpers centralize
         * common actor-related tasks so other code does not duplicate logic.
         *
         * @note None of the functions in this utility class require instantiation —
         * they are all static.
         *
         * @see `IActor`, `Properties`
         */
        class WPCore_API GameActorUtil
        {
        public:
            // Deep-copy a prefab graph with fresh actor/component IDs and remapped local references.
            static SmartPtr<Properties> createInstanceData( SmartPtr<Properties> data );
            // Creates all shells before loading any component properties. Caller owns scene lock.
            static Array<SmartPtr<IGameActor>> loadSceneActors(
                const Array<SmartPtr<Properties>> &data, SmartPtr<IGameScene> target = nullptr );
            /**
             * @name Common property key strings
             * @{
             *
             * Constant string keys used by actor serialization and UI code.
             * These identifiers are typically used when converting an actor to/from
             * a `Properties` object or when building UI representations.
             */
            static const String childStr;
            static const String nameStr;
            static const String staticStr;
            static const String enabledStr;
            static const String smoothMotionStr;
            static const String collisionMaskStr;
            static const String layerStr;
            static const String tagsStr;
            static const String updateTransformStr;
            static const String labelStr;
            static const String uuidStr;
            static const String localTransformStr;
            static const String worldTransformStr;
            static const String componentsStr;
            static const String componentStr;
            static const String componentTypeStr;
            static const String childrenStr;
            /** @} */

            /**
             * @brief Create a static rigid mesh from the currently selected actor(s).
             *
             * This convenience overload is intended for editor/GUI usage where the
             * target actor(s) are inferred from the current selection or context.
             *
             * The created rigid mesh will be configured as static (immovable).
             *
             * @note Implementation details (selection lookup) are engine specific.
             */
            static void createRigidStaticMesh();

            /**
             * @brief Create a dynamic rigid mesh from the currently selected actor(s).
             *
             * Similar to `createRigidStaticMesh()` but produces a dynamic (movable)
             * rigid body suitable for physics simulation.
             */
            static void createRigidDynamicMesh();

            /**
             * @brief Create a static rigid mesh for the provided actor.
             *
             * This function inspects the given `actor` and creates any necessary
             * collision/physics meshes so the actor participates as a static body in
             * the physics system.
             *
             * @param actor The actor to create physics for. If `actor` is null the
             *              function is a no-op.
             * @param recursive If true, also creates static rigid meshes for all
             *                  descendant child actors.
             */
            static void createRigidStaticMesh( SmartPtr<IGameActor> actor, bool recursive );

            /**
             * @brief Create a dynamic rigid mesh for the provided actor.
             *
             * This function inspects the given `actor` and creates any necessary
             * collision/physics meshes so the actor participates as a dynamic body in
             * the physics system (i.e. affected by forces and collisions).
             *
             * @param actor The actor to create physics for. If `actor` is null the
             *              function is a no-op.
             * @param recursive If true, also creates dynamic rigid meshes for all
             *                  descendant child actors.
             */
            static void createRigidDynamicMesh( SmartPtr<IGameActor> actor, bool recursive );

            /**
             * @brief Compute the actor's local axis-aligned bounding box (AABB).
             *
             * The returned AABB is expressed in the actor's local coordinate space and
             * encapsulates the geometry and components attached to the actor.
             *
             * @param actor The actor to compute the local AABB for. If `actor` is
             *              null or contains no geometry, a default-constructed
             *              `AABB3<real_Num>` (typically empty/invalid) is returned.
             * @return The axis-aligned bounding box in local space.
             */
            static AABB3<real_Num> getActorLocalAABB( SmartPtr<IGameActor> actor );

            /**
             * @brief Compute the actor's world axis-aligned bounding box (AABB).
             *
             * This computes the AABB of the actor transformed into world space,
             * including any children or attached geometry as appropriate.
             *
             * @param actor The actor to compute the world AABB for. If `actor` is
             *              null or contains no geometry, a default-constructed
             *              `AABB3<real_Num>` (typically empty/invalid) is returned.
             * @return The axis-aligned bounding box in world space.
             */
            static AABB3<real_Num> getActorAABB( SmartPtr<IGameActor> actor );

            /**
             * @brief Populate an actor from a `Properties` object.
             *
             * This method reads actor data from `actorData` and applies it to the
             * provided `actor` instance. Typical data includes name, uuid, transform,
             * component list and component parameters.
             *
             * @param actor The actor to update. If null the function does nothing.
             * @param actorData The serialized actor data to load from.
             * @param cascade When true, child actor data present in `actorData` will
             *                also be loaded and applied recursively.
             *
             * @see `toData` for the inverse operation.
             */
            static void loadFromData( SmartPtr<IGameActor> actor, SmartPtr<Properties> actorData,
                                      bool cascade );

            /** Detect the editor camera saved as a game actor by older scene files. */
            static bool isEditorCameraData( SmartPtr<IGameActor> editorCamera,
                                            SmartPtr<Properties> actorData );

            /** Restore editor camera settings without creating duplicate components. */
            static void restoreEditorCameraData( SmartPtr<IGameActor> editorCamera,
                                                SmartPtr<Properties> actorData );

            /**
             * @brief Serialize an actor to a `Properties` object.
             *
             * The returned `Properties` contains the actor's metadata required to
             * recreate or inspect the actor later (for example `name`, `uuid`,
             * transform, components and optionally children).
             *
             * Implementations typically include component information and may
             * include child actor entries depending on engine needs.
             *
             * @param actor The actor to serialize. If null an empty `Properties` is
             *              returned.
             * @return A `SmartPtr<Properties>` representing the serialized actor.
             *
             * @see `loadFromData`
             */
            static SmartPtr<Properties> toData( SmartPtr<IGameActor> actor );
        };
    }  // namespace scene
}  // namespace workphone

#endif  // ActorUtil_h__
