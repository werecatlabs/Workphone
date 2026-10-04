/**
 * @file Scene.hpp
 * @brief Defines the Scene class, the concrete implementation of IScene for managing actors, scene
 * state, and updates.
 *
 * The Scene class provides the core functionality for loading, saving, updating, and managing the
 * contents and state of a scene. It supports actor management, threaded update registration, event
 * handling, and scene state transitions.
 *
 * @author Zane Desir
 * @version 1.0
 */
#ifndef __WP_Scene_h__
#define __WP_Scene_h__

#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Core/ConcurrentHashMap.hpp>
#include <Workphone/Core/FixedArray.hpp>
#include <Workphone/Core/StringPool.hpp>
#include <Workphone/Memory/SharedRWPtr.hpp>
#include <Workphone/System/Resource.hpp>
#include <memory>

namespace workphone
{
    namespace scene
    {
        /**
         * @class Scene
         * @brief Concrete implementation of the IScene interface for scene management.
         *
         * The Scene class is responsible for managing all actors, their updates, and the overall state
         * of a scene. It provides methods for loading and saving scenes, adding and removing actors,
         * handling threaded updates, and managing scene-level properties such as lighting and state. The
         * class is thread-safe and supports concurrent operations on actors and update queues.
         *
         * Key features:
         * - Actor management (add, remove, find, enumerate)
         * - Scene loading and saving
         * - Threaded update registration for actors
         * - Scene state and loading state management
         * - Event handling and property serialization
         * - Lighting director integration
         *
         * @see IScene
         * @see IActor
         */

        /**
         * @brief Configuration for spatial partitioning update logic.
         */

        class WPCore_API GameScene : public Resource<IGameScene>
        {
            /**
             * @brief Spatial partitioning methods available for the scene.
             */

        public:
            /**
             * @brief Default constructor.
             *
             * Initializes the scene and its internal state.
             */
            GameScene();

            /**
             * @brief Destructor.
             *
             * Cleans up resources held by the scene.
             */
            ~GameScene() override;

#ifdef _DEBUG
            s32 addReference();
            bool removeReference();
#endif

            /**
             * @copydoc IScene::loadScene
             * @brief Loads the scene from the specified file path.
             */
            void loadScene( const String &path, bool async = true ) override;

            /**
             * Loads a scene from a file.
             * @param data A string containing the data.
             */
            void loadSceneDataStr( const String &data, bool async = true ) override;

            /**
             * @copydoc IScene::saveScene
             * @brief Saves the scene to the specified file path.
             */
            void saveScene( const String &path ) override;

            /**
             * @copydoc IScene::saveScene
             * @brief Saves the scene to its current file path.
             */
            void saveScene() override;

            /**
             * @copydoc IScene::getLabel
             * @brief Gets the name/label of the scene.
             */
            String getLabel() const override;

            /**
             * @copydoc IScene::setLabel
             * @brief Sets the name/label of the scene.
             */
            void setLabel( const String &label ) override;

            /**
             * @copydoc IScene::load
             * @brief Loads the scene with the specified data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IScene::reload
             * @brief Reloads the scene with the specified data.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IScene::unload
             * @brief Unloads the scene and releases associated resources.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IScene::preUpdate
             * @brief Performs pre-update logic for the scene.
             */
            void preUpdate() override;

            /**
             * @copydoc IScene::update
             * @brief Updates the scene and all managed actors.
             */
            void update() override;

            /**
             * @copydoc IScene::postUpdate
             * @brief Performs post-update logic for the scene.
             */
            void postUpdate() override;

            /**
             * @copydoc IScene::addActor
             * @brief Adds an actor to the scene.
             */
            void addActor( SmartPtr<IGameActor> actor ) override;

            /**
             * @copydoc IScene::removeActor
             * @brief Removes an actor from the scene.
             */
            void removeActor( SmartPtr<IGameActor> actor ) override;

            /**
             * @copydoc IScene::removeAllActors
             * @brief Removes all actors from the scene.
             */
            void removeAllActors() override;

            /**
             * @copydoc IScene::findActorByName
             * @brief Finds an actor in the scene by name.
             */
            SmartPtr<IGameActor> findActorByName( const String &name ) const override;

            /**
             * @copydoc IScene::findActorById
             * @brief Finds an actor in the scene by ID.
             */
            SmartPtr<IGameActor> findActorById( s32 id ) const override;

            /**
             * @copydoc IScene::getActors
             * @brief Gets all actors currently in the scene.
             */
            Array<SmartPtr<IGameActor>> getActors() const override;

            /**
             * @copydoc IScene::setActors
             * @brief Sets the actors in the scene, replacing any existing actors.
             */
            void setActors( const Array<SmartPtr<IGameActor>> &actors ) override;

            /**
             * @copydoc IScene::clear
             * @brief Clears the scene, removing all actors and resetting state.
             */
            void clear( bool clearNow = true ) override;

            /**
             * @copydoc IScene::registerAllUpdates
             * @brief Registers the actor for updates on all tasks.
             */
            void registerAllUpdates( SmartPtr<IGameActor> actor ) override;

            /**
             * @copydoc IScene::registerUpdates
             * @brief Registers an actor for updates on a specific task.
             */
            void registerUpdates( TaskId taskId, SmartPtr<IGameActor> actor ) override;

            /**
             * @copydoc IScene::registerUpdate
             * @brief Registers an actor for updates on a specific task and update type.
             */
            void registerUpdate( TaskId taskId, Thread::UpdateState updateType,
                                 SmartPtr<IGameActor> object ) override;

            /**
             * @copydoc IScene::unregisterUpdate
             * @brief Unregisters the actor from updates on a specific task and update type.
             */
            void unregisterUpdate( TaskId taskId, Thread::UpdateState updateType,
                                   SmartPtr<IGameActor> object ) override;

            /**
             * @copydoc IScene::unregisterAll
             * @brief Unregisters the actor from all tasks.
             */
            void unregisterAll( SmartPtr<IGameActor> object ) override;

            /**
             * @copydoc IScene::getRegisteredObjects
             * @brief Gets the registered objects for a given update state and task.
             */
            ConcurrentArray<SmartPtr<IGameActor>> &getRegisteredObjects( Thread::UpdateState updateState,
                                                                         TaskId task ) override;

            /**
             * @copydoc IScene::getRegisteredObjects
             * @brief Gets the registered objects for a given update state and task.
             */
            const ConcurrentArray<SmartPtr<IGameActor>> &getRegisteredObjects(
                Thread::UpdateState updateState, TaskId task ) const override;

            /**
             * @copydoc IScene::setRegisteredObjects
             * @brief Sets the registered objects for a given update state and task.
             */
            void setRegisteredObjects( Thread::UpdateState updateState, TaskId task,
                                       const ConcurrentArray<SmartPtr<IGameActor>> &objects ) override;

            /**
             * @copydoc IScene::isValid
             * @brief Checks if the scene is valid and initialized.
             */
            bool isValid() const override;

            /**
             * @copydoc IScene::toData
             * @brief Serializes the scene to a shared data object.
             */
            SmartPtr<ISharedObject> toData() const override;

            /**
             * @copydoc IScene::fromData
             * @brief Deserializes the scene from a shared data object.
             */
            void fromData( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IScene::getProperties
             * @brief Gets the scene's properties as a Properties object.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc IScene::setProperties
             * @brief Sets the scene's properties from a Properties object.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Sets the component state of the scene.
             * @param state The new state to set.
             */
            void setState( State state ) override;

            /**
             * @brief Gets the current component state of the scene.
             * @return The current state.
             */
            State getState() const override;

            /**
             * @brief Sets the scene loading state.
             * @param state The new loading state to set.
             */
            void setSceneLoadingState( SceneLoadingState state ) override;

            /**
             * @brief Gets the current scene loading state.
             * @return The current loading state.
             */
            SceneLoadingState getSceneLoadingState() const override;

            /**
             * @brief Sorts the objects in memory for efficient access or rendering.
             */
            void sortObjects();

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
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            /**
             * @brief Destroys the necessary objects when a scene is loaded.
             *
             * This is called internally during scene loading to clean up old objects.
             */
            void destroyOnLoad();

            /**
             * @brief Updates the scene lighting settings.
             */
            void updateLighting();

            /** @copydoc IScene::getLightingDirector */
            SmartPtr<LightingDirector> getLightingDirector() const override;

            /** @copydoc IScene::setLightingDirector */
            void setLightingDirector( SmartPtr<LightingDirector> lightingDirector ) override;

            /**
             * @brief Gets the global string pool.
             */
            StringPool<c8> *getStringPool() const override;

            /**
             * @brief Sets the global string pool.
             */
            void setStringPool( StringPool<c8> *pool ) override;

            /**
             * @brief Sets the spatial partitioning method for the scene.
             * @param method The partitioning method to use.
             */
            void setSpatialPartitioningMethod( SpatialPartitioningMethod method ) override;

            /**
             * @brief Gets the current spatial partitioning method of the scene.
             * @return The current partitioning method.
             */
            SpatialPartitioningMethod getSpatialPartitioningMethod() const override;

            /**
             * @brief Sets the spatial partitioning configuration.
             * @param config The configuration to use.
             */
            void setSpatialPartitioningConfig( const SpatialPartitioningConfig &config ) override;

            /**
             * @brief Gets the current spatial partitioning configuration.
             * @return The current configuration.
             */
            SpatialPartitioningConfig getSpatialPartitioningConfig() const override;

            /**
             * @brief Locks the scene for thread-safe operations.
             */
            void lock() override;

            /**
             * @brief Attempts to lock the scene for thread-safe operations.
             */
            void lock_shared() override;

            /**
             * @brief Attempts to lock the scene for thread-safe operations.
             * @return True if the lock was acquired, false otherwise.
             */
            bool try_lock() override;

            /**
             * @brief Unlocks the scene after thread-safe operations.
             */
            void unlock() override;

            /**
             * @brief Unlocks the scene after shared thread-safe operations.
             */
            void unlock_shared() override;

            /**
             * @brief Registers the class for runtime type information and factory creation.
             *
             * This macro expands to declarations for type info and registration functions used by the
             * engine's RTTI and factory systems.
             */
            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @var m_lightingDirector
             * @brief The lighting director for the scene, managing lighting settings and updates.
             */
            AtomicSmartPtr<LightingDirector> m_lightingDirector;

            ///< String pool for efficient string management
            AtomicRawPtr<StringPool<c8>> m_stringPool;

            /**
             * @var m_state
             * @brief The current state of the scene (edit, play, etc.).
             */
            AtomicValue<State> m_state = State::None;

            /**
             * @var m_sceneLoadingState
             * @brief The current loading state of the scene (loaded, unloaded, etc.).
             */
            AtomicValue<SceneLoadingState> m_sceneLoadingState = SceneLoadingState::Loaded;

            /**
             * @var m_partitioningMethod
             * @brief The current spatial partitioning method.
             */
            AtomicValue<SpatialPartitioningMethod> m_partitioningMethod =
                SpatialPartitioningMethod::None;

            /**
             * @var m_partitioner
             * @brief The current spatial partitioner implementation.
             */
            std::unique_ptr<class SpatialPartitioner> m_partitioner;
            AtomicObject<SpatialPartitioningConfig> m_partitioningConfig;

            /**
             * @var m_label
             * @brief The label or name of the scene.
             */
            AtomicObject<FixedString<128>> m_label;

            /**
             * @var m_playQueue
             * @brief Queue of actors to be set to play mode.
             *
             * This concurrent queue is used to manage actors that need to be transitioned to play mode
             * in a thread-safe manner.
             */
            ConcurrentQueue<SmartPtr<IGameActor>> m_playQueue;

            /**
             * @var m_editQueue
             * @brief Queue of actors to be set to edit mode.
             *
             * This concurrent queue is used to manage actors that need to be transitioned to edit mode
             * in a thread-safe manner.
             */
            ConcurrentQueue<SmartPtr<IGameActor>> m_editQueue;

            /**
             * @var m_actors
             * @brief Shared read-write pointer to the array of actors in the scene.
             *
             * This allows for thread-safe access and modification of the actor list.
             */
            ConcurrentArray<SmartPtr<IGameActor>> m_actors;

            /**
             * @var m_updateObjects
             * @brief Array of arrays of shared pointers to concurrent arrays of actors, used for
             * threaded updates.
             *
             * This structure organizes actors by update state and task for efficient multi-threaded
             * updates.
             */
            ConcurrentArray<ConcurrentArray<ConcurrentArray<SmartPtr<IGameActor>>>> m_updateObjects;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // Scene_h__
