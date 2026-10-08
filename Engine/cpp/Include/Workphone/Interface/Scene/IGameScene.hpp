#ifndef IScene_h__
#define IScene_h__

#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @file IScene.hpp
         * @brief Defines the interface for a scene, which manages actors, their components, and scene
         * state.
         *
         * The IScene interface provides methods for loading, saving, and managing the contents and state
         * of a scene. It supports actor management, component queries, and registration for threaded
         * updates.
         *
         * @author Zane Desir
         * @version 1.0
         */
        class WPCore_API IGameScene : public IResource
        {
        public:
            /**
             * @brief Represents the state of the scene.
             */
            enum class State
            {
                None,   ///< The scene is in an undefined state.
                Edit,   ///< The scene is in edit mode.
                Play,   ///< The scene is in play mode.
                Reset,  ///< The scene is resetting to its initial state.
                Count   ///< The number of states in the enumeration.
            };

            /**
             * @brief Represents the loading state of the scene.
             */
            enum class SceneLoadingState
            {
                None,       ///< No loading state.
                Loaded,     ///< The scene is loaded.
                Unloaded,   ///< The scene is unloaded.
                Loading,    ///< Preparation or graph commit is pending.
                Failed,     ///< Loading failed; no completion event was emitted.
                Cancelled,  ///< A pending load was superseded or cleared.
                Count       ///< The number of states in the enumeration.

            };

            /**
             * @brief Spatial partitioning methods available for the scene.
             */
            enum class SpatialPartitioningMethod
            {
                None,         ///< No spatial partitioning (linear search).
                UniformGrid,  ///< Simple grid-based partitioning.
                Octree,       ///< Hierarchical partitioning using an octree.
                Count         ///< Number of available methods.
            };

            /**
             * @brief Configuration for spatial partitioning update logic.
             */
            struct SpatialPartitioningConfig
            {
                real_Num nearDistance = 50.0f;  ///< Distance for high-frequency updates.
                real_Num midDistance = 150.0f;  ///< Distance for mid-frequency updates.
                real_Num farDistance = 300.0f;  ///< Distance for low-frequency updates.

                float nearUpdateRate = 1.0f;  ///< Update every frame (1.0 = every frame).
                float midUpdateRate = 0.33f;  ///< Update every ~3 frames.
                float farUpdateRate = 0.1f;   ///< Update every ~10 frames.

                real_Num sleepDistance = 500.0f;  ///< Distance beyond which cells are put to sleep.
            };

            IGameScene();

            IGameScene( u32 poolTypeId );

            /**
             * @brief Virtual destructor for safe polymorphic destruction.
             */
            ~IGameScene() override;

            /**
             * @brief Loads a scene from the specified file path.
             * @param path The path to the file containing the scene data.
             * @param async Whether to load the scene asynchronously.
             *
             * This method should load all actors, components, and scene data from the given file.
             */
            virtual void loadScene( const String &path, bool async = true ) = 0;

            /**
             * Loads a scene from a file.
             * @param data A string containing the data.
             * @param async Whether to load the scene asynchronously.
             */
            virtual void loadSceneDataStr( const String &data, bool async = true ) = 0;

            /**
             * @brief Saves the scene to the specified file path.
             * @param path The path to save the scene data to.
             *
             * This method should serialize all actors, components, and scene data to the given file.
             */
            virtual void saveScene( const String &path ) = 0;

            /**
             * @brief Saves the scene to its current file path.
             *
             * This method should serialize the scene to its internally stored file path.
             */
            virtual void saveScene() = 0;

            /**
             * @brief Clears the scene, removing all actors and resetting state.
             */
            virtual void clear( bool clearNow = true ) = 0;

            /**
             * @brief Gets the name/label of the scene.
             * @return The label of the scene.
             */
            virtual String getLabel() const = 0;

            /**
             * @brief Sets the name/label of the scene.
             * @param label The new name for the scene.
             */
            virtual void setLabel( const String &label ) = 0;

            /**
             * @brief Adds an actor to the scene.
             * @param actor A smart pointer to the actor to add.
             *
             * The actor will be managed by the scene and included in updates and queries.
             */
            virtual void addActor( SmartPtr<IGameActor> actor ) = 0;

            /**
             * @brief Removes an actor from the scene.
             * @param actor A smart pointer to the actor to remove.
             *
             * The actor will be detached from the scene and no longer updated or queried.
             */
            virtual void removeActor( SmartPtr<IGameActor> actor ) = 0;

            /**
             * @brief Removes all actors from the scene.
             */
            virtual void removeAllActors() = 0;

            /**
             * @brief Finds an actor in the scene by name.
             * @param name The name of the actor to find.
             * @return A smart pointer to the found actor, or null if no actor was found.
             */
            virtual SmartPtr<IGameActor> findActorByName( const String &name ) const = 0;

            /**
             * @brief Finds an actor in the scene by ID.
             * @param id The ID of the actor to find.
             * @return A smart pointer to the found actor, or null if no actor was found.
             */
            virtual SmartPtr<IGameActor> findActorById( s32 id ) const = 0;

            /**
             * @brief Gets all actors currently in the scene.
             * @return An array of smart pointers to all actors in the scene.
             */
            virtual Array<SmartPtr<IGameActor>> getActors() const = 0;

            /**
             * @brief Sets the actors in the scene.
             * @param actors An array of smart pointers to the actors to set in the scene.
             */
            virtual void setActors( const Array<SmartPtr<IGameActor>> &actors ) = 0;

            /**
             * @brief Registers an actor for updates on a specific task.
             * @param taskId The ID of the task for which the actor is registered for updates.
             * @param actor The actor object to register.
             */
            virtual void registerUpdates( TaskId taskId, SmartPtr<IGameActor> actor ) = 0;

            /**
             * @brief Registers an actor for updates on a specific task and update type.
             * @param taskId The ID of the task for which the actor is registered for updates.
             * @param updateType The update type used to update the actor object.
             * @param actor The actor object to register.
             */
            virtual void registerUpdate( TaskId taskId, Thread::UpdateState updateType,
                                         SmartPtr<IGameActor> actor ) = 0;

            /**
             * @brief Registers the actor for updates on all tasks.
             * @param actor The actor object to register.
             */
            virtual void registerAllUpdates( SmartPtr<IGameActor> actor ) = 0;

            /**
             * @brief Unregisters the actor from updates on a specific task and update type.
             * @param taskId The ID of the task.
             * @param updateType The update type.
             * @param object The actor object to unregister.
             */
            virtual void unregisterUpdate( TaskId taskId, Thread::UpdateState updateType,
                                           SmartPtr<IGameActor> object ) = 0;

            /**
             * @brief Unregisters the actor from all tasks.
             * @param actor The actor object to unregister.
             */
            virtual void unregisterAll( SmartPtr<IGameActor> actor ) = 0;

            /**
             * @brief Gets the registered objects for a given update state and task.
             * @param updateState The update state.
             * @param task The task.
             * @return A shared pointer to a concurrent array of smart pointers to actors.
             */
            virtual ConcurrentArray<SmartPtr<IGameActor>> &getRegisteredObjects(
                Thread::UpdateState updateState, TaskId task ) = 0;

            /**
             * @brief Gets the registered objects for a given update state and task.
             * @param updateState The update state.
             * @param task The task.
             * @return A shared pointer to a concurrent array of smart pointers to actors.
             */
            virtual const ConcurrentArray<SmartPtr<IGameActor>> &getRegisteredObjects(
                Thread::UpdateState updateState, TaskId task ) const = 0;

            /**
             * @brief Sets the registered objects for a given update state and task.
             * @param updateState The update state.
             * @param task The task.
             * @param objects The shared pointer to the concurrent array of actors to set.
             */
            virtual void setRegisteredObjects(
                Thread::UpdateState updateState, TaskId task,
                const ConcurrentArray<SmartPtr<IGameActor>> &objects ) = 0;

            /**
             * @brief Sets the current state of the scene.
             * @param state The scene state to set.
             */
            virtual void setState( State state ) = 0;

            /**
             * @brief Gets the current state of the scene.
             * @return The current scene state.
             */
            virtual State getState() const = 0;

            /**
             * @brief Sets the loading state of the scene.
             * @param state The scene loading state to set.
             */
            virtual void setSceneLoadingState( SceneLoadingState state ) = 0;

            /**
             * @brief Gets the loading state of the scene.
             * @return The current scene loading state.
             */
            virtual SceneLoadingState getSceneLoadingState() const = 0;

            /**
             * @brief Handles an event dispatched to the scene.
             * @param eventType The type of the event.
             * @param eventValue The value associated with the event.
             * @param arguments The arguments for the event.
             * @param sender The sender of the event.
             * @param object The object associated with the event.
             * @param event The event object.
             * @return A parameter representing the result of the event handling.
             */
            virtual Parameter handleEvent( EventType eventType, hash_type eventValue,
                                           const Array<Parameter> &arguments,
                                           SmartPtr<ISharedObject> sender,
                                           SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) = 0;

            /**
             * @brief Gets a smart pointer to the lighting director for the scene.
             * @return A smart pointer to the lighting director.
             */
            virtual SmartPtr<LightingDirector> getLightingDirector() const = 0;

            /**
             * @brief Sets the lighting director for the scene.
             * @param lightingDirector A smart pointer to the lighting director to set.
             */
            virtual void setLightingDirector( SmartPtr<LightingDirector> lightingDirector ) = 0;

            /**
             * @brief Gets the global string pool.
             */

            /**
             * @brief Sets the spatial partitioning method for the scene.
             * @param method The partitioning method to use.
             */
            virtual void setSpatialPartitioningMethod( SpatialPartitioningMethod method ) = 0;

            /**
             * @brief Gets the current spatial partitioning method of the scene.
             * @return The current partitioning method.
             */
            virtual SpatialPartitioningMethod getSpatialPartitioningMethod() const = 0;

            /**
             * @brief Sets the spatial partitioning configuration.
             * @param config The configuration to use.
             */
            virtual void setSpatialPartitioningConfig( const SpatialPartitioningConfig &config ) = 0;

            /**
             * @brief Gets the current spatial partitioning configuration.
             * @return The current configuration.
             */
            virtual SpatialPartitioningConfig getSpatialPartitioningConfig() const = 0;

            virtual StringPool<c8> *getStringPool() const = 0;

            /**
             * @brief Sets the global string pool.
             */
            virtual void setStringPool( StringPool<c8> *pool ) = 0;

            /**
             * @brief Gets a smart pointer to the first component of the specified type attached to the
             * given actor or its children.
             * @tparam T The type of component to get.
             * @param actor The actor to search for the component.
             * @return A smart pointer to the first component of the specified type, or nullptr if not
             * found.
             */
            template <class T>
            SmartPtr<T> getComponentFromActor( SmartPtr<IGameActor> actor ) const;

            /**
             * @brief Gets a smart pointer to the first component of the specified type attached to any
             * actor in the scene.
             * @tparam T The type of component to get.
             * @return A smart pointer to the first component of the specified type attached to any
             * actor, or nullptr if not found.
             */
            template <class T>
            SmartPtr<T> getComponent() const;

            /**
             * @brief Gets an array of smart pointers to all components of the specified type attached to
             * any actor in the scene.
             * @tparam T The type of component to get.
             * @return An array of smart pointers to all components of the specified type attached to any
             * actor in the scene.
             */
            template <class T>
            Array<SmartPtr<T>> getComponents() const;

            WP_CLASS_REGISTER_DECL;
        };  // namespace scene

        /**
         * @brief Gets a smart pointer to the first component of the specified type attached to the given
         * actor or its children.
         * @tparam T The type of component to get.
         * @param actor The actor to search for the component.
         * @return A smart pointer to the first component of the specified type, or nullptr if not found.
         */
        template <class T>
        SmartPtr<T> IGameScene::getComponentFromActor( SmartPtr<IGameActor> actor ) const
        {
            if( actor )
            {
                if( auto actorComponent = actor->getComponent<T>() )
                {
                    return actorComponent;
                }

                const auto children = actor->getChildren();
                for( auto child : children )
                {
                    if( child )
                    {
                        if( auto childComponent = getComponentFromActor<T>( child ) )
                        {
                            return childComponent;
                        }
                    }
                }
            }

            return nullptr;
        }

        /**
         * @brief Gets a smart pointer to the first component of the specified type attached to any actor
         * in the scene.
         * @tparam T The type of component to get.
         * @return A smart pointer to the first component of the specified type attached to any actor, or
         * nullptr if not found.
         */
        template <class T>
        SmartPtr<T> IGameScene::getComponent() const
        {
            WP_ASSERT( isValid() );

            const auto actors = getActors();
            for( auto actor : actors )
            {
                if( actor )
                {
                    if( auto actorComponent = getComponentFromActor<T>( actor ) )
                    {
                        return actorComponent;
                    }
                }
            }

            return nullptr;
        }

        /**
         * @brief Gets an array of smart pointers to all components of the specified type attached to any
         * actor in the scene.
         * @tparam T The type of component to get.
         * @return An array of smart pointers to all components of the specified type attached to any
         * actor in the scene.
         */
        template <class T>
        Array<SmartPtr<T>> IGameScene::getComponents() const
        {
            WP_ASSERT( isValid() );

            Array<SmartPtr<T>> components;
            components.reserve( 10 );

            const auto actors = getActors();
            for( auto actor : actors )
            {
                if( actor )
                {
                    auto actorComponents = actor->getAllComponentsAndInChildren<T>();
                    if( !actorComponents.empty() )
                    {
                        components.insert( components.cend(), actorComponents.begin(),
                                           actorComponents.end() );
                    }
                }
            }

            return components;
        }
    }  // namespace scene
}  // namespace workphone

#endif  // IScene_h__
