#ifndef __WP_CSceneManager_h__
#define __WP_CSceneManager_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Core/ConcurrentHashMap.hpp>
#include <Workphone/Core/ConcurrentMap.hpp>
#include <Workphone/Core/Pair.hpp>
#include <Workphone/Core/UnorderedMap.hpp>
#include <Workphone/State/States/TransformStateData.hpp>
#include <Workphone/State/States/PhysicsBodyState.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>
#include <mutex>

namespace workphone
{
    namespace scene
    {

        /**
         * @class GameManager
         * @brief Concrete implementation of IGameManager: manages scenes, actors and components.
         *
         * GameManager is the central coordinator for scene lifecycle and runtime state. It
         * handles loading/unloading scenes, creation and destruction of actors and components,
         * update scheduling, FSM management and propagation of dirty/transform/state changes.
         *
         * Concurrency: many internal collections are concurrent containers allowing multi-threaded
         * producers/consumers. A recursive spin mutex (m_mutex) protects operations that must be
         * executed atomically. Callers may use lock/try_lock/unlock APIs exposed by IGameManager
         * when performing compound operations.
         *
         * Responsibilities include:
         *  - Scene loading/unloading and current scene tracking.
         *  - Actor/component lifecycle and registration with update tasks.
         *  - Transform state buffering and time-sampled queries for interpolation.
         *  - Queuing and processing of load/unload/property change requests.
         *
         * @author Zane Desir
         * @version 1.0
         */
        class WPCore_API GameManager : public IGameManager
        {
        public:
            /**
             * @class EventListener
             * @brief Single application listener that forwards scene events to the GameManager.
             *
             * The EventListener keeps a weak reference to the GameManager to avoid ownership
             * cycles. The manager routes each event to its target component or actor, replacing
             * the per-component application listeners.
             */
            class EventListener : public IEventListener
            {
            public:
                /**
                 * @brief Default constructor.
                 */
                EventListener();

                /**
                 * @brief Destructor.
                 */
                ~EventListener() override;

                /**
                 * @brief Handle an incoming event and forward to the GameManager.
                 * @param eventType The event category/type.
                 * @param eventValue Precomputed hashed value for the event identifier.
                 * @param arguments Event arguments supplied by the sender.
                 * @param sender The object that dispatched the event.
                 * @param object Optional object associated with the event.
                 * @param event Pointer to the event instance, including its optional target.
                 * @return Parameter value produced by event handling (semantics depend on event).
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Return the owner GameManager (may be null if expired).
                 * @return SmartPtr<GameManager> to the owning manager.
                 */
                SmartPtr<GameManager> getOwner() const;

                /**
                 * @brief Assign the owner GameManager for this listener.
                 * @param owner Smart pointer to the GameManager to set as owner (may be null).
                 */
                void setOwner( SmartPtr<GameManager> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                /// Weak pointer to the owner SceneManager.
                AtomicWeakPtr<GameManager> m_owner;
            };

            /**
             * @brief Construct a GameManager.
             *
             * Initializes internal concurrent collections and prepares the manager to
             * accept load requests and create actors/components.
             */
            GameManager();

            /**
             * @brief Destructor.
             *
             * Ensures queued work is processed or abandoned safely and releases owned resources.
             */
            ~GameManager() override;

            /**
             * @brief Load manager configuration and initialize resources.
             * @param data Optional shared object containing initialization properties.
             *
             * Called by the resource system to populate persistent settings and to
             * enqueue any deferred loads required by the manager.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload manager resources and clear runtime state.
             * @param data Optional shared object provided for unload semantics.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Pre-update step executed before the main update tick.
             *
             * Typical duties include processing load/unload queues, applying queued
             * property changes and preparing state for the frame.
             */
            void preUpdate() override;

            /**
             * @brief Main per-frame update for the scene manager.
             *
             * This advances FSMs, updates registered components for each task, and
             * processes dirty lists and transform interpolation as required.
             */
            void update() override;

            /**
             * @brief Post-update step executed after the main update tick.
             *
             * Use this to finalize state changes and flush any per-frame temporary data.
             */
            void postUpdate() override;

            /**
             * @brief Load a scene from disk and make it the current scene.
             * @param filePath Path to the scene file to load.
             *
             * The implementation is responsible for parsing the scene file, creating
             * actors/components and registering them with the manager. This operation
             * may be synchronous or queued depending on implementation.
             */
            void loadScene( const String &filePath, bool async = true ) override;

            /**
             * @brief Load a scene from an in-memory string (for example editor snapshot).
             * @param data Scene content encoded as a string.
             */
            void loadSceneDataStr( const String &data, bool async = true ) override;

            /**
             * @brief Clear the current scene and remove all managed objects.
             *
             * This will attempt to unload and destroy actors, components and systems
             * currently tracked by the manager. Callers should ensure it's safe to
             * perform this operation (no external references are accessing objects).
             */
            void clear() override;

            /**
             * @brief Get a raw pointer to the global FSM manager used by the scene.
             * @return Raw IFSMManager pointer or nullptr if none assigned.
             *
             * Use getFsmManager() to obtain a smart pointer when ownership semantics are
             * required.
             */
            IFSMManager *getFsmManagerPtr() const override;

            /**
             * @brief Get the smart pointer to the global FSM manager.
             * @return SmartPtr<IFSMManager> referencing the manager (may be null).
             */
            SmartPtr<IFSMManager> getFsmManager() const override;

            /**
             * @brief Set or replace the global FSM manager used for actor/component FSMs.
             * @param fsmManager Smart pointer to the FSM manager instance (may be null).
             */
            void setFsmManager( SmartPtr<IFSMManager> fsmManager ) override;

            /**
             * @brief Get a raw pointer to the FSM manager registered for a component type.
             * @param typeId Component type identifier.
             * @return Raw IFSMManager pointer for the type or nullptr if none registered.
             */
            IFSMManager *getComponentFsmManagerPtr( u32 typeId ) override;

            /**
             * @brief Get the FSM manager for a component type as a smart pointer.
             * @param typeId Component type identifier.
             * @return SmartPtr<IFSMManager> for the requested type (may be null).
             */
            SmartPtr<IFSMManager> getComponentFsmManager( u32 typeId ) override;

            /**
             * @brief Register an FSM manager for a specific component type.
             * @param typeId Component type identifier.
             * @param fsmManager Smart pointer to the FSM manager to register (may be null to clear).
             */
            void setComponentFsmManager( u32 typeId, SmartPtr<IFSMManager> fsmManager ) override;

            /**
             * @brief Get the raw pointer to the currently active scene.
             * @return Raw IGameScene pointer or nullptr if no scene is loaded.
             */
            IGameScene *getCurrentScenePtr() const override;

            /**
             * @brief Get the currently active scene as a smart pointer.
             * @return SmartPtr<IGameScene> referencing the active scene (may be null).
             */
            SmartPtr<IGameScene> getCurrentScene() const override;

            /**
             * @brief Assign the current scene for this manager.
             * @param scene Smart pointer to the scene to make active (may be null to clear).
             */
            void setCurrentScene( SmartPtr<IGameScene> scene ) override;

            /**
             * @brief Get a snapshot of all actors currently managed by the scene manager.
             * @return Array of SmartPtr<IGameActor> containing the managed actors.
             */
            Array<SmartPtr<IGameActor>> getActors() const override;

            /**
             * @brief Create a new actor instance and register it with the manager.
             * @return SmartPtr<IGameActor> referencing the newly created actor.
             */
            SmartPtr<IGameActor> createActor() override;

            /**
             * @brief Create a new actor and return a raw (non-owning) pointer.
             * @return Raw IGameActor pointer to the created actor.
             *
             * The caller must not assume ownership; the manager retains lifecycle control.
             */
            IGameActor *createActorPtr() override;

            /**
             * @brief Destroy a managed actor.
             * @param actor Smart pointer to the actor to destroy.
             * @param cascade If true, also destroy child actors recursively.
             */
            void destroyActor( SmartPtr<IGameActor> actor, bool cascade = true ) override;

            /**
             * @brief Destroy all actors currently managed by the scene manager.
             *
             * This will attempt to gracefully tear down actors and release component
             * resources. It is typically called during scene unload or application shutdown.
             */
            void destroyActors() override;

            /**
             * @brief Transition the manager into play mode.
             *
             * Typically used to begin simulation or runtime behavior. May trigger
             * FSM transitions and activate runtime-only systems.
             */
            void play() override;

            /**
             * @brief Transition the manager into edit mode.
             *
             * Typically used by editors to enable editing features and disable runtime-only
             * systems.
             */
            void edit() override;

            /**
             * @brief Stop play mode and related running processes.
             *
             * This returns the manager to an idle/edit-like state and stops simulation.
             */
            void stop() override;

            /**
             * @brief Register a transform component with the manager and obtain an ID.
             * @param transformComponent Smart pointer to the transform component to add.
             * @return Unique ID assigned to the registered transform.
             */
            u32 addTransformComponent( SmartPtr<ITransform> transformComponent );

            /**
             * @brief Register a component with the manager and obtain an ID.
             * @param component Smart pointer to the component to register.
             * @return Unique ID assigned to the component within manager tables.
             */
            u32 addComponent( SmartPtr<IComponent> component ) override;

            /**
             * @brief Unregister a component from the manager.
             * @param component Smart pointer to the component to remove.
             * @return ID of the component that was removed or an invalid id on failure.
             */
            u32 removeComponent( SmartPtr<IComponent> component ) override;

            /**
             * @brief Register a component system with the manager.
             * @param id Type ID for the system.
             * @param system Smart pointer to the system instance.
             */
            void addSystem( u32 id, SmartPtr<IComponentSystem> system ) override;

            /**
             * @brief Unregister the system with the given type ID.
             * @param id Type ID of the system to remove.
             */
            void removeSystem( u32 id ) override;

            /**
             * @brief Lookup an actor by manager-assigned ID.
             * @param id Unique manager-assigned actor ID.
             * @return SmartPtr<IGameActor> to the actor or null if not found.
             */
            SmartPtr<IGameActor> getActor( u32 id ) const override;

            /**
             * @brief Find the first actor with the given name.
             * @param name Name to search for.
             * @return SmartPtr<IGameActor> to the first matching actor or null if none.
             */
            SmartPtr<IGameActor> getActorByName( const String &name ) const override;

            /**
             * @brief Find all actors with the specified name.
             * @param name Name to search for.
             * @return Array of SmartPtr<IGameActor> containing all matches (may be empty).
             */
            Array<SmartPtr<IGameActor>> getActorsByName( const String &name ) const override;

            /**
             * @brief Lookup an actor by its file-scoped identifier (used during loading).
             * @param id File ID string assigned in scene data.
             * @return SmartPtr<IGameActor> to the actor or null if not found.
             */
            SmartPtr<IGameActor> getActorByFileId( const String &id ) const override;

            /**
             * @brief Get a raw pointer to an FSM by its ID.
             * @param id FSM identifier.
             * @return Raw IFSM pointer or nullptr if no FSM exists with that id.
             */
            IFSM *getFSMPtr( u32 id ) const override;

            /**
             * @brief Get an FSM by its ID as a smart pointer.
             * @param id FSM identifier.
             * @return SmartPtr<IFSM> referencing the FSM or null if not found.
             */
            SmartPtr<IFSM> getFSM( u32 id ) const override;

            /**
             * @brief Create a new transform instance managed by the scene manager.
             * @return SmartPtr<ITransform> referencing the created transform.
             */
            SmartPtr<ITransform> createTransform() override;

            /**
             * @brief Destroy a managed transform instance.
             * @param transform Smart pointer to the transform to destroy.
             */
            void destroyTransform( SmartPtr<ITransform> transform ) override;

            /**
             * @brief Lookup a transform by its manager-assigned ID.
             * @param id Transform identifier.
             * @return SmartPtr<Transform> or null if not found.
             */
            SmartPtr<Transform> getTransform( u32 id ) const override;

            /**
             * @brief Marks an actor as dirty, indicating it needs to be updated.
             * @param actor Smart pointer to the actor to mark as dirty.
             */
            void addDirty( SmartPtr<IGameActor> actor ) override;

            /**
             * @brief Removes an actor from the dirty list.
             * @param actor Smart pointer to the actor to remove from dirty list.
             */
            void removeDirty( SmartPtr<IGameActor> actor ) override;

            /**
             * @copydoc ISceneManager::makeActorTransformsDirty
             * @brief Marks all actor transforms as dirty.
             */
            void makeActorTransformsDirty() override;

            /**
             * @copydoc ISceneManager::addDirtyTransform
             * @brief Marks an actor's transform as dirty.
             * @param actor Smart pointer to the actor whose transform is dirty.
             */
            void addDirtyActor( SmartPtr<IGameActor> actor ) override;

            /**
             * @brief Marks a transform as dirty.
             * @param transform Smart pointer to the transform to mark as dirty.
             */
            void addDirtyTransform( SmartPtr<ITransform> transform ) override;

            /**
             * @brief Marks a component as dirty.
             * @param component Smart pointer to the component to mark as dirty.
             */
            void addDirtyComponent( SmartPtr<IComponent> component ) override;

            /**
             * @brief Marks a component's transform as dirty.
             * @param component Smart pointer to the component whose transform is dirty.
             */
            void addDirtyComponentTransform( SmartPtr<IComponent> component ) override;

            /**
             * @copydoc ISceneManager::loadObject
             * @brief Loads an object, optionally forcing it to be queued.
             * @param object Smart pointer to the object to load.
             * @param forceQueue Whether to force the object to be queued for loading.
             */
            void loadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) override;

            /**
             * @copydoc ISceneManager::loadObject
             */
            void loadObject( SmartPtr<ISharedObject> object, SmartPtr<ISharedObject> data,
                             bool forceQueue ) override;

            /**
             * @copydoc ISceneManager::unloadObject
             * @brief Unloads an object, optionally forcing it to be queued.
             * @param object Smart pointer to the object to unload.
             * @param forceQueue Whether to force the object to be queued for unloading.
             */
            void unloadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) override;

            /**
             * @brief Queues properties for an object.
             * @param object Smart pointer to the object.
             * @param properties Smart pointer to the properties to queue.
             */
            void queueProperties( SmartPtr<ISharedObject> object,
                                  SmartPtr<Properties> properties ) override;

            /**
             * @brief Gets the task used to update the state.
             * @return The update task.
             */
            TaskId getStateTask() const override;

            /**
             * @brief Gets the current task used for the scene.
             * @return The scene task.
             */
            TaskId getSceneTask() const override;

            /**
             * @copydoc ISceneManager::getComponents
             * @brief Gets all components managed by the scene manager.
             * @return Array of smart pointers to components.
             */
            Array<SmartPtr<IComponent>> getComponents() const override;

            /**
             * @copydoc ISceneManager::getComponents
             * @brief Gets all components of a specific type.
             * @param type The type of components to get.
             * @return Array of smart pointers to components.
             */
            Array<SmartPtr<IComponent>> getComponents( u32 type ) const override;

            /**
             * @brief Gets the array of transforms managed by the scene manager.
             * @return Reference to the array of smart pointers to transforms.
             */
            Array<SmartPtr<ITransform>> getTransforms() const;

            /**
             * @brief Sets the array of transforms managed by the scene manager.
             * @param transforms The array of smart pointers to transforms.
             */
            void setTransforms( Array<SmartPtr<ITransform>> transforms );

            /**
             * @brief Adds a transform to the list of smooth transforms.
             * @param transform Smart pointer to the transform to add.
             */
            void addSmoothTransform( SmartPtr<ITransform> transform );

            /**
             * @brief Removes a transform from the list of smooth transforms.
             * @param transform Smart pointer to the transform to remove.
             */
            void removeSmoothTransform( SmartPtr<ITransform> transform );

            /**
             * @copydoc ISceneManager::getComponentFactoryIgnoreList
             * @brief Gets the list of component factory types to ignore.
             * @return Array of strings representing ignored component factory types.
             */
            Array<String> getComponentFactoryIgnoreList() const override;

            /**
             * @copydoc ISceneManager::setComponentFactoryIgnoreList
             * @brief Sets the list of component factory types to ignore.
             * @param ignoreList Array of strings representing ignored component factory types.
             */
            void setComponentFactoryIgnoreList( const Array<String> &ignoreList ) override;

            /**
             * @copydoc ISceneManager::getComponentFactoryMap
             * @brief Gets the map of component factory types.
             * @return Map of component factory types.
             */
            Map<String, String> getComponentFactoryMap() const override;

            /**
             * @copydoc ISceneManager::setComponentFactoryMap
             * @brief Sets the map of component factory types.
             * @param map Map of component factory types.
             */
            void setComponentFactoryMap( const Map<String, String> &map ) override;

            /**
             * @copydoc ISceneManager::getComponentFactoryType
             * @brief Gets the component factory type for a given type string.
             * @param type The type string.
             * @return The component factory type string.
             */
            String getComponentFactoryType( const String &type ) const override;

            /**
             * @brief Adds a transform state for a given ID and time.
             * @param id The ID of the transform.
             * @param time The time interval.
             * @param transform The transform state data.
             */
            void addTransformState( u32 id, time_interval time,
                                    const Transform3<real_Num> &transform ) override;

            /**
             * @brief Adds a transform state for a given ID and time.
             * @param id The ID of the transform.
             * @param time The time interval.
             * @param transform The transform state data.
             */
            void addTransformState( u32 id, time_interval time, const Transform3<real_Num> &transform,
                                    const Vector3<real_Num> &linearVelocity,
                                    const Vector3<real_Num> &angularVelocity ) override;

            /**
             * @brief Gets the transform state for a given ID and time.
             * @param id The ID of the transform.
             * @param t The time interval.
             * @param transform The transform state data to fill.
             * @return True if the state was found, false otherwise.
             */
            bool getTransformState( u32 id, time_interval t, time_interval dt,
                                    Transform3<real_Num> &transform, TaskId task ) override;

            /**
             * @brief Gets the registered components for a given update state and task.
             * @param state The update state.
             * @param task The task.
             * @return Concurrent array of smart pointers to components.
             */
            ConcurrentArray<SmartPtr<IComponent>> &getRegisteredComponents( Thread::UpdateState state,
                                                                            TaskId task ) override;

            /**
             * @copydoc ISceneManager::registerComponentUpdate
             * @brief Registers a component for updates for a given task and state.
             * @param task The task.
             * @param state The update state.
             * @param component Smart pointer to the component to register.
             */
            void registerComponentUpdate( TaskId task, Thread::UpdateState state,
                                          SmartPtr<IComponent> component ) override;

            /**
             * @copydoc ISceneManager::unregisterComponentUpdate
             * @brief Unregisters a component from updates for a given task and state.
             * @param task The task.
             * @param state The update state.
             * @param component Smart pointer to the component to unregister.
             */
            void unregisterComponentUpdate( TaskId task, Thread::UpdateState state,
                                            SmartPtr<IComponent> component ) override;

            /**
             * @copydoc ISceneManager::unregisterAllComponent
             * @brief Unregisters all updates for a given component.
             * @param component Smart pointer to the component to unregister.
             */
            void unregisterAllComponent( SmartPtr<IComponent> component ) override;

            /**
             * @copydoc ISceneManager::getNumActors
             * @brief Gets the number of actors managed by the scene manager.
             * @return The number of actors.
             */
            s32 getNumActors() const override;

            /**
             * @brief Gets the global string pool.
             */
            StringPool<c8> *getStringPool() const;

            /**
             * @brief Sets the global string pool.
             */
            void setStringPool( StringPool<c8> *pool );

            /**
             * @brief Handles an event for the scene manager.
             * @param eventType The type of the event.
             * @param eventValue The hashed value of the event.
             * @param arguments The arguments associated with the event.
             * @param sender The sender of the event.
             * @param object The object associated with the event.
             * @param event The event object. Its optional target controls direct component or
             * actor dispatch; an event without a target is broadcast.
             * @return A parameter result from handling the event.
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event );

            /**
             * @copydoc ISceneManager::lock
             * @brief Locks the scene manager for thread-safe operations.
             */
            void lock() override;

            /**
             * @copydoc ISceneManager::lock_shared
             * @brief Locks the scene manager for shared access.
             */
            void lock_shared() override;

            /**
             * @copydoc ISceneManager::try_lock
             * @brief Attempts to lock the scene manager for thread-safe operations.
             * @return True if the lock was acquired, false otherwise.
             */
            bool try_lock() override;

            /**
             * @copydoc ISceneManager::unlock
             * @brief Unlocks the scene manager.
             */
            void unlock() override;

            /**
             * @copydoc ISceneManager::unlock_shared
             * @brief Unlocks the scene manager from shared access.
             */
            void unlock_shared() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Gets the load priority for a shared object.
             * @param obj The shared object.
             * @return The load priority value.
             */
            s32 getLoadPriority( ISharedObject *obj );

            /// Sample an actor or compose its local pose with a sampled ancestor.
            bool getActorTransformState( IGameActor *actor, time_interval time, time_interval dt,
                                         Transform3<real_Num> &worldTransform,
                                         UnorderedMap<u32, Transform3<real_Num>> &sampledTransforms );

            struct TransformSample
            {
                time_interval time = 0;
                Transform3<real_Num> transform;
                Vector3<real_Num> linearVelocity = Vector3<real_Num>::zero();
                Vector3<real_Num> angularVelocity = Vector3<real_Num>::zero();
                bool hasVelocity = false;
            };

            void storeTransformSample( u32 id, const TransformSample &sample );

            // Each producer task owns a timestamped history per actor. The mutex
            // protects publication and sampling independently of scene operations.
            FixedArray<UnorderedMap<u32, Array<TransformSample>>, (u32)TaskId::Count> m_transformHistory;
            mutable std::mutex m_transformHistoryMutex;

            /// Update flags for objects.
            Array<Array<bool>> m_updateObjects;

            /// Registered components for update.
            Array<Array<ConcurrentArray<SmartPtr<IComponent>>>> m_updateComponents;

            /// FSM manager for the scene.
            SmartPtr<IFSMManager> m_fsmManager;

            /// FSM managers for specific component types.
            Map<u32, SmartPtr<IFSMManager>> m_componentFsmManagers;

            /// Single application listener used to dispatch events to scene objects.
            SmartPtr<IEventListener> m_eventListener;

            /// The current scene.
            AtomicSmartPtr<IGameScene> m_scene;

            /// Queue for property updates.
            ConcurrentQueue<Pair<ISharedObject *, SmartPtr<Properties>>> m_queueProperties;

            /// Queues for loading actors and components.
            ConcurrentQueue<Pair<SmartPtr<IGameActor>, SmartPtr<Properties>>> m_loadingActors;

            // Queues for unloading actors and components.
            ConcurrentQueue<Pair<SmartPtr<IComponent>, SmartPtr<Properties>>> m_loadingComponents;

            /// Queues for unloading actors and components.
            ConcurrentQueue<SmartPtr<IGameActor>> m_unloadingActors;

            /** Queue for unloading components.
             */
            ConcurrentQueue<SmartPtr<IComponent>> m_unloadingComponents;

            /// Queues for dirty actors and components.
            ConcurrentQueue<Pair<SmartPtr<IGameActor>, Pair<u32, u32>>> m_dirtyActors;
            ConcurrentQueue<SmartPtr<IComponent>> m_dirtyComponents;
            ConcurrentQueue<SmartPtr<IComponent>> m_dirtyComponentTransforms;
            ConcurrentQueue<SmartPtr<IGameActor>> m_dirtyTransforms;
            ConcurrentQueue<SmartPtr<ITransform>> m_dirtyActorTransforms;

            /// Map of components by type ID.
            ConcurrentHashMap<u32, Array<SmartPtr<IComponent>>> m_components;

            /// Map of systems by type ID.
            ConcurrentHashMap<u32, SmartPtr<IComponentSystem>> m_systems;

            /// Array of transforms managed by the scene manager.
            ConcurrentArray<SmartPtr<ITransform>> m_transforms;

            /// Array of smooth transforms.
            ConcurrentArray<SmartPtr<ITransform>> m_smoothTransforms;

            /// Array of actors managed by the scene manager.
            ConcurrentArray<SmartPtr<IGameActor>> m_actors;

            /// Array of FSMs.
            ConcurrentArray<SmartPtr<IFSM>> m_fsms;

            /// Array of FSM listeners.
            ConcurrentArray<SmartPtr<IFSMListener>> m_fsmListeners;

            /// Array of game FSMs.
            ConcurrentArray<SmartPtr<IFSM>> m_gameFSMs;

            /// Array of game FSM listeners.
            ConcurrentArray<SmartPtr<IFSMListener>> m_gameFsmListeners;

            /// Array of scene pointers.
            ConcurrentArray<SmartPtr<IGameScene>> m_scenes;

            /// Loading states for actors.
            ConcurrentArray<LoadingState> m_actorLoadingStates;

            /// Next time to resize transform arrays.
            atomic_f64 m_nextTransformSizeTime;

            /// Number of actors managed by the scene manager.
            atomic_s32 m_numActors = 0;

            /// String pool for efficient string management.
            AtomicRawPtr<StringPool<c8>> m_stringPool;

            /// Array of actor load jobs.
            ConcurrentArray<SmartPtr<ActorLoadJob>> m_actorLoadJobs;

            /// List of component factory types to ignore.
            ConcurrentArray<String> m_componentFactoryIgnoreList;

            /// Map of component factory types.
            ConcurrentMap<String, String> m_componentFactoryMap;

            /// Mutex for thread-safe operations.
            mutable RecursiveSpinMutex m_mutex;
        };

        inline IGameScene *GameManager::getCurrentScenePtr() const
        {
            return m_scene.get();
        }

        inline IFSMManager *GameManager::getFsmManagerPtr() const
        {
            return m_fsmManager.get();
        }

        inline SmartPtr<IFSMManager> GameManager::getFsmManager() const
        {
            return m_fsmManager;
        }

        inline StringPool<c8> *GameManager::getStringPool() const
        {
            return m_stringPool;
        }

        inline void GameManager::setStringPool( StringPool<c8> *pool )
        {
            m_stringPool = pool;
        }

    }  // namespace scene
}  // namespace workphone

#endif  // CSceneManager_h__
