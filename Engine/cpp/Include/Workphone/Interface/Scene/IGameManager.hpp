#ifndef ___ISceneManager_h__
#define ___ISceneManager_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/Map.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @brief Interface for a scene manager object.
         *
         * The scene manager is responsible for loading, unloading, and
         * managing the lifecycle of scenes, actors, components, and systems.
         *
         * @author	Zane Desir
         * @version 1.0
         */
        class WPCore_API IGameManager : public ISharedObject
        {
        public:
            /// Shared presentation delay for vehicle meshes and follow cameras (seconds).
            static constexpr time_interval smoothMotionDelay = 1.0 / 60.0;

            static const hash_type sceneLoadedHash;  ///< The hash for the scene loaded event.
            static const hash_type sceneUnloadedHash;  ///< The hash for the scene unloaded event.
            static const hash_type scenePlayHash;      ///< The hash for the scene play event.
            static const hash_type sceneEditHash;      ///< The hash for the scene edit event.
            static const hash_type sceneClearHash;     ///< The hash for the scene clear event.

            /**
             * @brief Virtual destructor.
             */
            ~IGameManager() override;

            /**
             * @brief Loads a scene from a file.
             *
             * When loading is complete, the scene loaded event identified by
             * @ref sceneLoadedHash is fired.
             *
             * @param filePath The file path to the scene.
             * @param async Whether to load the scene asynchronously.
             * If true, the scene will be loaded in a background thread and
             * the scene loaded event will be fired when loading is complete.
             * If false, the scene will be loaded synchronously and the scene
             * loaded event will be fired immediately after loading.
             */
            virtual void loadScene( const String &filePath, bool async = true ) = 0;

            /**
             * @brief Loads a scene from a string containing scene data.
             *
             * When loading is complete, the scene loaded event identified by
             * @ref sceneLoadedHash is fired.
             *
             * @param data A string containing the scene data.
             * @param async Whether to load the scene asynchronously.
             * If true, the scene will be loaded in a background thread and
             * the scene loaded event will be fired when loading is complete.
             * If false, the scene will be loaded synchronously and the scene
             * loaded event will be fired immediately after loading.
             */
            virtual void loadSceneDataStr( const String &data, bool async = true ) = 0;

            /**
             * @brief Clears the scene.
             *
             * Removes all actors, components, and systems from the current scene.
             */
            virtual void clear() = 0;

            /**
             * @brief Gets the FSM manager.
             * @return A raw pointer to the FSM manager. Can be null.
             */
            virtual IFSMManager *getFsmManagerPtr() const = 0;

            /**
             * @brief Gets the FSM manager.
             * @return A SmartPtr to the FSM manager. Can be null.
             */
            virtual SmartPtr<IFSMManager> getFsmManager() const = 0;

            /**
             * @brief Sets the FSM manager.
             * @param fsmManager The FSM manager to set.
             */
            virtual void setFsmManager( SmartPtr<IFSMManager> fsmManager ) = 0;

            /**
             * @brief Gets the FSM manager for a specific component type.
             * @param typeId The type ID of the component.
             * @return A raw pointer to the component FSM manager. Can be null.
             */
            virtual IFSMManager *getComponentFsmManagerPtr( u32 typeId ) = 0;

            /**
             * @brief Gets the FSM manager for a specific component type.
             * @param typeId The type ID of the component.
             * @return A SmartPtr to the component FSM manager. Can be null.
             */
            virtual SmartPtr<IFSMManager> getComponentFsmManager( u32 typeId ) = 0;

            /**
             * @brief Sets the FSM manager for a specific component type.
             * @param typeId The type ID of the component.
             * @param fsmManager The component FSM manager to set.
             */
            virtual void setComponentFsmManager( u32 typeId, SmartPtr<IFSMManager> fsmManager ) = 0;

            /**
             * @brief Gets the current scene.
             * @return A raw pointer to the current scene object. Can be null.
             */
            virtual IGameScene *getCurrentScenePtr() const = 0;

            /**
             * @brief Gets the current scene.
             * @return A SmartPtr to the current scene object. Can be null.
             */
            virtual SmartPtr<IGameScene> getCurrentScene() const = 0;

            /**
             * @brief Sets the current scene.
             * @param scene The current scene object to set.
             */
            virtual void setCurrentScene( SmartPtr<IGameScene> scene ) = 0;

            /**
             * @brief Gets all actors in the current scene.
             * @return An array of SmartPtrs to the actors in the scene.
             */
            virtual Array<SmartPtr<IGameActor>> getActors() const = 0;

            /**
             * @brief Creates a new actor and adds it to the current scene.
             * @return A SmartPtr to the newly created actor.
             */
            virtual SmartPtr<IGameActor> createActor() = 0;

            /**
             * @brief Creates a new actor and adds it to the current scene.
             * @return A raw pointer to the newly created actor.
             */
            virtual IGameActor *createActorPtr() = 0;

            /**
             * @brief Destroys an actor.
             * @param actor The actor to destroy.
             * @param cascade Whether to cascade destruction to child actors.
             * If true, all children of the actor are also destroyed.
             */
            virtual void destroyActor( SmartPtr<IGameActor> actor, bool cascade = true ) = 0;

            /**
             * @brief Destroys all actors in the current scene.
             */
            virtual void destroyActors() = 0;

            /**
             * @brief Triggers the play event, transitioning the scene to play mode.
             */
            virtual void play() = 0;

            /**
             * @brief Triggers the edit event, transitioning the scene to edit mode.
             */
            virtual void edit() = 0;

            /**
             * @brief Triggers the stop event, halting scene playback or editing.
             */
            virtual void stop() = 0;

            /**
             * @brief Adds a component to the scene.
             * @param component The component to add.
             * @return The unique identifier assigned to the component.
             */
            virtual u32 addComponent( SmartPtr<IComponent> component ) = 0;

            /**
             * @brief Removes a component from the scene.
             * @param component The component to remove.
             * @return The unique identifier of the removed component.
             */
            virtual u32 removeComponent( SmartPtr<IComponent> component ) = 0;

            /**
             * @brief Gets all components in the current scene.
             * @return An array of SmartPtrs to the components in the scene.
             */
            virtual Array<SmartPtr<IComponent>> getComponents() const = 0;

            /**
             * @brief Adds a system to the scene.
             * @param id The unique identifier for the system.
             * @param system The system to add.
             */
            virtual void addSystem( u32 id, SmartPtr<IComponentSystem> system ) = 0;

            /**
             * @brief Removes a system from the scene.
             * @param id The unique identifier of the system to remove.
             */
            virtual void removeSystem( u32 id ) = 0;

            /**
             * @brief Gets the actor with the specified ID.
             * @param id The ID of the actor to retrieve.
             * @return A SmartPtr to the actor with the specified ID. Can be null.
             */
            virtual SmartPtr<IGameActor> getActor( u32 id ) const = 0;

            /**
             * @brief Gets the first actor with the specified name.
             * @param name The name of the actor to retrieve.
             * @return A SmartPtr to the actor with the specified name. Can be null.
             */
            virtual SmartPtr<IGameActor> getActorByName( const String &name ) const = 0;

            /**
             * @brief Gets all actors with the specified name.
             * @param name The name of the actors to retrieve.
             * @return An array of SmartPtrs to actors with the specified name. Can be empty.
             */
            virtual Array<SmartPtr<IGameActor>> getActorsByName( const String &name ) const = 0;

            /**
             * @brief Gets the actor with the specified file ID.
             * @param id The file ID of the actor to retrieve.
             * @return A SmartPtr to the actor with the specified file ID. Can be null.
             */
            virtual SmartPtr<IGameActor> getActorByFileId( const String &id ) const = 0;

            /**
             * @brief Gets a raw pointer to the FSM for a given ID.
             * @param id The ID of the FSM.
             * @return A raw pointer to the FSM. Can be null.
             */
            virtual IFSM *getFSMPtr( u32 id ) const = 0;

            /**
             * @brief Gets the FSM for a given ID.
             * @param id The ID of the FSM.
             * @return A SmartPtr to the FSM. Can be null.
             */
            virtual SmartPtr<IFSM> getFSM( u32 id ) const = 0;

            /**
             * @brief Creates a new transform component.
             * @return A SmartPtr to the created transform component.
             */
            virtual SmartPtr<ITransform> createTransform() = 0;

            /**
             * @brief Destroys a transform component.
             * @param transform A SmartPtr to the transform component to destroy.
             */
            virtual void destroyTransform( SmartPtr<ITransform> transform ) = 0;

            /**
             * @brief Gets a transform by its ID.
             * @param id The ID of the transform.
             * @return A SmartPtr to the transform. Can be null.
             */
            virtual SmartPtr<Transform> getTransform( u32 id ) const = 0;

            /**
             * @brief Queues properties to be set on an object.
             * @param object The object to set the properties on.
             * @param properties The properties object containing the properties to set.
             */
            virtual void queueProperties( SmartPtr<ISharedObject> object,
                                          SmartPtr<Properties> properties ) = 0;

            /**
             * @brief Returns the task used to update the scene state.
             * @return A TaskId representing the task used to update the scene state.
             */
            virtual TaskId getStateTask() const = 0;

            /**
             * @brief Returns the current task used for the scene.
             * @return A TaskId representing the current task used for the scene.
             */
            virtual TaskId getSceneTask() const = 0;

            /**
             * @brief Returns all components of the specified type attached to actors in the current
             * scene.
             * @param type The type of component to search for.
             * @return An array of SmartPtrs to components of the specified type.
             */
            virtual Array<SmartPtr<IComponent>> getComponents( u32 type ) const = 0;

            /**
             * @brief Gets the list of component types to ignore during factory creation.
             * @return An array of strings representing the types to ignore.
             */
            virtual Array<String> getComponentFactoryIgnoreList() const = 0;

            /**
             * @brief Sets the list of component types to ignore during factory creation.
             * @param ignoreList An array of strings representing the types to ignore.
             */
            virtual void setComponentFactoryIgnoreList( const Array<String> &ignoreList ) = 0;

            /**
             * @brief Gets the component factory map that maps component types to factory names.
             * @return A map from component type strings to factory name strings.
             */
            virtual Map<String, String> getComponentFactoryMap() const = 0;

            /**
             * @brief Sets the component factory map that maps component types to factory names.
             * @param map A map from component type strings to factory name strings.
             */
            virtual void setComponentFactoryMap( const Map<String, String> &map ) = 0;

            /**
             * @brief Gets the factory name for a given component type.
             * @param type The component type string to look up.
             * @return The factory name string associated with the component type.
             */
            virtual String getComponentFactoryType( const String &type ) const = 0;

            /**
             * @brief Adds a transform state for the specified actor at a given time.
             * @param id The ID of the actor.
             * @param time The time interval at which the transform state occurs.
             * @param transform The transform state to add.
             */
            virtual void addTransformState( u32 id, time_interval time,
                                            const Transform3<real_Num> &transform ) = 0;

            /**
             * @brief Adds a transform state with velocity for the specified actor at a given time.
             * @param id The ID of the actor.
             * @param time The time interval at which the transform state occurs.
             * @param transform The transform state to add.
             * @param linearVelocity The linear velocity at this transform state.
             * @param angularVelocity The angular velocity at this transform state.
             */
            virtual void addTransformState( u32 id, time_interval time,
                                            const Transform3<real_Num> &transform,
                                            const Vector3<real_Num> &linearVelocity,
                                            const Vector3<real_Num> &angularVelocity ) = 0;

            /**
             * @brief Gets the transform state for the specified actor at a given time.
             * @param id The ID of the actor.
             * @param t Sample time in the producer clock domain; interpolation uses stored timestamps.
             * Forward prediction is limited to 100 ms beyond the newest sample.
             * @param dt Retained for compatibility; does not affect timestamp-based sampling.
             * @param transform [out] The retrieved transform state.
             * @param task The task ID associated with the retrieval.
             * @return True if the transform state was found, false otherwise.
             */
            virtual bool getTransformState( u32 id, time_interval t, time_interval dt,
                                            Transform3<real_Num> &transform,
                                            TaskId task ) = 0;

            /**
             * @brief Gets the registered components for the specified update state and task.
             * @param state The update state for which to retrieve registered components.
             * @param task The task ID associated with the update.
             * @return A reference to the concurrent array of registered components.
             */
            virtual ConcurrentArray<SmartPtr<IComponent>> &getRegisteredComponents(
                Thread::UpdateState state, TaskId task ) = 0;

            /**
             * @brief Registers a component for a specific update state and task.
             * @param task The task ID associated with the update.
             * @param state The update state to register the component for.
             * @param component The component to register.
             */
            virtual void registerComponentUpdate( TaskId task, Thread::UpdateState state,
                                                  SmartPtr<IComponent> component ) = 0;

            /**
             * @brief Unregisters a component from a specific update state and task.
             * @param task The task ID associated with the update.
             * @param state The update state to unregister the component from.
             * @param component The component to unregister.
             */
            virtual void unregisterComponentUpdate( TaskId task, Thread::UpdateState state,
                                                    SmartPtr<IComponent> component ) = 0;

            /**
             * @brief Unregisters a component from all update states and tasks.
             * @param component The component to unregister.
             */
            virtual void unregisterAllComponent( SmartPtr<IComponent> component ) = 0;

            /**
             * @brief Gets the number of actors in the current scene.
             * @return The number of actors.
             */
            virtual s32 getNumActors() const = 0;

            /**
             * @brief Marks an actor as dirty, indicating it needs to be updated.
             * @param actor Smart pointer to the actor to mark as dirty.
             */
            virtual void addDirty( SmartPtr<IGameActor> actor ) = 0;

            /**
             * @brief Removes an actor from the dirty list.
             * @param actor Smart pointer to the actor to remove from dirty list.
             */
            virtual void removeDirty( SmartPtr<IGameActor> actor ) = 0;

            /**
             * @brief Marks all actor transforms as dirty, indicating they need to be updated.
             */
            virtual void makeActorTransformsDirty() = 0;

            /**
             * @brief Adds an actor to the dirty actor list for transform updates.
             * @param actor A SmartPtr to the actor to add.
             */
            virtual void addDirtyActor( SmartPtr<IGameActor> actor ) = 0;

            /**
             * @brief Marks a transform as dirty.
             * @param transform Smart pointer to the transform to mark as dirty.
             */
            virtual void addDirtyTransform( SmartPtr<ITransform> transform ) = 0;

            /**
             * @brief Marks a component as dirty.
             * @param component Smart pointer to the component to mark as dirty.
             */
            virtual void addDirtyComponent( SmartPtr<IComponent> component ) = 0;

            /**
             * @brief Marks a component's transform as dirty.
             * @param component Smart pointer to the component whose transform is dirty.
             */
            virtual void addDirtyComponentTransform( SmartPtr<IComponent> component ) = 0;

            /**
             * @brief Loads an object via the scene manager.
             * @param object The object to be loaded.
             * @param forceQueue If true, forces the object to be queued for deferred loading.
             */
            virtual void loadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) = 0;

            /**
             * @brief Loads an object with associated data via the scene manager.
             * @param object The object to be loaded.
             * @param data The associated data object for loading.
             * @param forceQueue If true, forces the object to be queued for deferred loading.
             */
            virtual void loadObject( SmartPtr<ISharedObject> object, SmartPtr<ISharedObject> data,
                                     bool forceQueue ) = 0;

            /**
             * @brief Unloads an object via the scene manager.
             * @param object The object to be unloaded.
             * @param forceQueue If true, forces the object to be queued for deferred unloading.
             */
            virtual void unloadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) = 0;

            /**
             * @brief Returns all components of the specified type attached to actors in the current
             * scene.
             *
             * @tparam T The type of component to search for.
             * @param type The type ID of the component to search for.
             * @return An array of SmartPtrs to components of the specified type.
             */
            template <class T>
            Array<SmartPtr<T>> getComponentsByType( u32 type ) const;

            /**
             * @brief Returns the first object of the specified type found in the current scene.
             *
             * Searches through all actors and their children for a component
             * of the specified type.
             *
             * @tparam T The type of object to search for.
             * @return A SmartPtr to the first object of the specified type found,
             *         or a null pointer if none is found.
             */
            template <class T>
            SmartPtr<T> getObjectByType() const;

            WP_CLASS_REGISTER_DECL;
        };

        template <class T>
        Array<SmartPtr<T>> IGameManager::getComponentsByType( u32 type ) const
        {
            auto typeInfo = T::typeInfo();
            auto components = getComponents( typeInfo );
            return Array<SmartPtr<T>>( components.begin(), components.end() );
        }

        template <class T>
        SmartPtr<T> IGameManager::getObjectByType() const
        {
            auto scene = getCurrentScene();
            auto actors = scene->getActors();
            for( auto actor : actors )
            {
                auto component = actor->getComponent<T>();
                if( component )
                {
                    return component;
                }

                auto children = actor->getAllChildren();
                for( auto child : children )
                {
                    component = child->getComponent<T>();
                    if( component )
                    {
                        return component;
                    }
                }
            }

            return nullptr;
        }

    }  // namespace scene
}  // namespace workphone

#endif  // ISceneManager_h__
