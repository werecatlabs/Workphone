#ifndef CGraphicsScene_h__
#define CGraphicsScene_h__

#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Core/ConcurrentHashMap.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class GraphicsScene
         * @brief High-level manager for a renderable scene.
         *
         * The GraphicsScene encapsulates all data and operations needed to build and
         * render a 3D scene: cameras, lights, scene nodes, meshes, particle systems,
         * skyboxes, terrains and scene-wide render states such as ambient lighting,
         * fog and environment mapping. The class exposes thread-safe operations and
         * supports animation management, instancing helpers, and ray casting.
         *
         * Responsibilities:
         * - Create, register and remove graphics objects and scene nodes.
         * - Maintain lists of scene sub-systems (skies, terrains, particle systems).
         * - Provide scene-wide properties (ambient light, hemisphere lighting, fog).
         * - Offer helpers for instancing and mesh splitting.
         *
         * Thread-safety: many internal containers use atomic/lock-free wrappers; callers
         * should still use the provided lock()/unlock() when performing multi-step
         * operations that must be atomic with respect to the scene state.
         */
        class WPCore_API GraphicsScene : public SharedGraphicsObject<IGraphicsScene>
        {
        public:
            /**
             * @brief Listener used to react to state changes for scene-wide objects.
             *
             * The StateListener forwards relevant state change notifications to the
             * owning GraphicsScene. It holds a weak reference to the owner to avoid
             * circular references between state contexts and scene objects.
             */
            class WPCore_API StateListener : public IStateListener
            {
            public:
                /**
                 * @brief Construct an uninitialized StateListener.
                 *
                 * The listener will not have an owner until setOwner() is called.
                 */
                StateListener();

                /**
                 * @brief Virtual destructor, releases any state associations.
                 */
                ~StateListener() override;

                /**
                 * @brief Unload callback invoked when a state context requests unloading.
                 *
                 * Implementations should perform any necessary cleanup of graphics
                 * resources associated with the owner scene.
                 *
                 * @param data Optional context object provided by the caller.
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handle an incoming state message.
                 *
                 * This method is called for individual state messages. The listener
                 * may inspect the message and decide whether it affects the owner.
                 *
                 * @param message State message to process.
                 * @return True if the message was handled and no further processing is needed.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handle a full state transition.
                 *
                 * Called when the bound state changes (e.g. loading completed). The
                 * listener may update cached resources or trigger reloads on the owner.
                 *
                 * @param state New state object referenced by the listener.
                 * @return True when the transition was processed successfully.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Return raw pointer to the owning GraphicsScene, if still valid.
                 *
                 * @return Raw pointer to the owner or nullptr if it has expired.
                 */
                GraphicsScene *getOwnerPtr() const;

                /**
                 * @brief Return a shared pointer to the owning GraphicsScene.
                 *
                 * @return SmartPtr<GraphicsScene> to the owner, or nullptr if expired.
                 */
                SmartPtr<GraphicsScene> getOwner() const;

                /**
                 * @brief Associate this listener with a GraphicsScene owner.
                 *
                 * The listener stores the owner as a weak pointer; callers should
                 * ensure the owner remains alive for as long as necessary.
                 *
                 * @param owner Smart pointer to the GraphicsScene to associate.
                 */
                void setOwner( SmartPtr<GraphicsScene> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                /**
                 * Weak pointer to the owning GraphicsScene to avoid circular references.
                 */
                AtomicWeakPtr<GraphicsScene> m_owner;
            };

            /**
             * @brief Constructs a new GraphicsScene object.
             *
             * Initializes internal containers, creates the root scene node, and sets up
             * default scene state. The scene is ready for object registration after construction.
             */
            GraphicsScene();

            /**
             * @brief Destroys the GraphicsScene object and releases resources.
             *
             * Clears all scene objects, unregisters listeners, and releases graphics resources.
             * All scene nodes, graphics objects, and animations are destroyed during cleanup.
             */
            ~GraphicsScene() override;

            /**
             * @brief Loads scene data from the given shared object.
             *
             * Deserializes scene configuration and content from the provided data object.
             * This may include scene graph structure, object properties, and render settings.
             *
             * @param data Shared object containing serialized scene data to load.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads scene data and releases associated resources.
             *
             * Removes objects and state associated with the provided data context.
             * Called during scene cleanup or when transitioning between scene configurations.
             *
             * @param data Shared object identifying which scene data to unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Updates the scene state for the current frame.
             *
             * Advances all active animations, updates transforms, and performs other
             * per-frame logic required before rendering. Should be called once per frame
             * from the main update loop.
             */
            void update() override;

            /**
             * @brief Get the type identifier of the scene.
             *
             * The type string can be used to distinguish different scene categories
             * or configurations within the application.
             *
             * @return String containing the scene type identifier.
             */
            String getType() const override;

            /**
             * @brief Set the type identifier of the scene.
             *
             * Assigns a type string that can be used for scene categorization or
             * identification purposes.
             *
             * @param type The type identifier to assign to this scene.
             */
            void setType( const String &type ) override;

            /**
             * @brief Gets the color of the upper hemisphere for ambient lighting.
             * @return The upper hemisphere color.
             */
            ColourF getUpperHemisphere() const override;

            /**
             * @brief Sets the color of the upper hemisphere for ambient lighting.
             * @param upperHemisphere The color to set.
             */
            void setUpperHemisphere( const ColourF &upperHemisphere ) override;

            /**
             * @brief Gets the color of the lower hemisphere for ambient lighting.
             * @return The lower hemisphere color.
             */
            ColourF getLowerHemisphere() const override;

            /**
             * @brief Sets the color of the lower hemisphere for ambient lighting.
             * @param lowerHemisphere The color to set.
             */
            void setLowerHemisphere( const ColourF &lowerHemisphere ) override;

            /**
             * @brief Gets the direction vector for the hemisphere lighting.
             * @return The hemisphere direction vector.
             */
            Vector3<real_Num> getHemisphereDir() const override;

            /**
             * @brief Sets the direction vector for the hemisphere lighting.
             * @param hemisphereDir The direction vector to set.
             */
            void setHemisphereDir( const Vector3<real_Num> &hemisphereDir ) override;

            /**
             * @brief Gets the environment map scale factor.
             * @return The environment map scale.
             */
            f32 getEnvmapScale() const override;

            /**
             * @brief Sets the environment map scale factor.
             * @param envmapScale The scale to set.
             */
            void setEnvmapScale( f32 envmapScale ) override;

            /**
             * @brief Locks the scene for thread-safe operations.
             *
             * Blocks the calling thread until exclusive access to the scene is acquired.
             * Use this when performing multi-step operations that must be atomic.
             * Always pair with unlock() to prevent deadlocks.
             */
            void lock() override;

            /**
             * @brief Attempts to lock the scene without blocking.
             *
             * Non-blocking variant of lock(). Returns immediately with lock status.
             * Useful when the calling code can defer work if the scene is busy.
             *
             * @return True if the lock was acquired, false if already locked.
             */
            bool try_lock() override;

            /**
             * @brief Releases the scene lock.
             *
             * Must be called after lock() or a successful try_lock() to allow other
             * threads to access the scene. Failure to unlock will cause deadlocks.
             */
            void unlock() override;

            /**
             * @brief Clears all objects and resets the scene to empty state.
             *
             * Removes and destroys all scene nodes, graphics objects, animations,
             * particle systems, skies, and terrains. The root node is preserved.
             * This is a heavy operation and should be used when completely resetting
             * or shutting down the scene.
             */
            void clear() override;

            /**
             * @brief Checks if the scene has an animation with the given name.
             * @param animationName The name of the animation.
             * @return True if the animation exists, false otherwise.
             */
            bool hasAnimation( const String &animationName ) override;

            /**
             * @brief Destroys the animation with the given name.
             * @param animationName The name of the animation to destroy.
             * @return True if the animation was destroyed, false otherwise.
             */
            bool destroyAnimation( const String &animationName ) override;

            /**
             * @brief Adds a named graphics object of a specified type to the scene.
             *
             * Creates a new graphics object (mesh, light, camera, etc.) with the given
             * name and type string. The object is registered with the scene and returned
             * to the caller for further configuration.
             *
             * @param name Unique name identifier for the graphics object.
             * @param type Type string identifying the kind of object to create (e.g., "Mesh", "Light").
             * @return Smart pointer to the newly created graphics object, or nullptr on failure.
             */
            SmartPtr<ISharedObject> addGraphicsObject( const String &name, const String &type ) override;

            /**
             * @brief Adds an auto-named graphics object of a specified type to the scene.
             *
             * Creates a graphics object with an automatically generated unique name.
             * Convenient when manual naming is not required.
             *
             * @param type Type string identifying the kind of object to create.
             * @return Smart pointer to the newly created graphics object, or nullptr on failure.
             */
            SmartPtr<ISharedObject> addGraphicsObject( const String &type ) override;

            /**
             * @brief Adds a graphics object using a pre-computed type hash ID.
             *
             * More efficient than string-based lookup when the type ID is known at compile
             * time or cached. Useful in performance-critical code paths.
             *
             * @param id Hash identifier of the object type to create.
             * @return Smart pointer to the newly created graphics object, or nullptr on failure.
             */
            SmartPtr<ISharedObject> addGraphicsObjectByTypeId( hash_type id ) override;

            /**
             * @brief Removes a graphics object from the scene.
             * @param graphicsObject The object to remove.
             * @return True if the object was removed, false otherwise.
             */
            bool removeGraphicsObject( SmartPtr<ISharedObject> graphicsObject ) override;

            /**
             * @brief Gets all graphics objects in the scene.
             * @return An array of smart pointers to graphics objects.
             */
            Array<SmartPtr<IGraphicsObject>> getGraphicsObjects() const override;

            /**
             * @brief Gets the terrain objects currently owned by the scene.
             * @return A thread-safe snapshot of the scene's terrains.
             */
            Array<SmartPtr<IGraphicsTerrain>> getTerrains() const;

            /**
             * @brief Sets the ambient light color for the scene.
             * @param colour The ambient light color.
             */
            void setAmbientLight( const ColourF &colour ) override;

            /**
             * @brief Gets the ambient light color for the scene.
             * @return The ambient light color.
             */
            ColourF getAmbientLight() const override;

            /**
             * @brief Gets the currently active camera.
             * @return A smart pointer to the active camera.
             */
            SmartPtr<IGraphicsCamera> getActiveCamera() const override;

            /**
             * @brief Sets the active camera for the scene.
             * @param camera The camera to set as active.
             */
            void setActiveCamera( SmartPtr<IGraphicsCamera> camera ) override;

            /**
             * @brief Gets the default camera for the scene.
             *
             * The default camera is used when no active camera is explicitly set,
             * or as a fallback during initialization.
             *
             * @return Smart pointer to the default camera, or nullptr if not set.
             */
            SmartPtr<IGraphicsCamera> getDefaultCamera() const;

            /**
             * @brief Set the default camera for the scene.
             *
             * Assigns the camera that will be used as the fallback when no active
             * camera is specified. Does not automatically make it the active camera.
             *
             * @param camera Smart pointer to the camera to use as default.
             */
            void setDefaultCamera( SmartPtr<IGraphicsCamera> camera );

            /**
             * @brief Gets a scene node by name.
             * @param name The name of the scene node.
             * @return A smart pointer to the scene node, or nullptr if not found.
             */
            SmartPtr<IGraphicsSceneNode> getSceneNode( const String &name ) const override;

            /**
             * @brief Gets a scene node by its unique ID.
             * @param id The unique hash ID of the scene node.
             * @return A smart pointer to the scene node, or nullptr if not found.
             */
            SmartPtr<IGraphicsSceneNode> getSceneNodeById( hash_type id ) const override;

            /**
             * @brief Gets the root scene node.
             * @return A smart pointer to the root scene node.
             */
            SmartPtr<IGraphicsSceneNode> getRootSceneNode() const override;

            /**
             * @brief Adds a new scene node to the scene.
             * @return A smart pointer to the created scene node.
             */
            SmartPtr<IGraphicsSceneNode> addSceneNode() override;

            /**
             * @brief Adds a scene node with the given name.
             * @param name The name of the scene node.
             * @return A smart pointer to the created scene node.
             */
            SmartPtr<IGraphicsSceneNode> addSceneNode( const String &name ) override;

            /**
             * @brief Removes a scene node from the scene.
             * @param sceneNode The scene node to remove.
             * @return True if the node was removed, false otherwise.
             */
            bool removeSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode ) override;

            /**
             * @brief Enables or disables the skybox for the scene.
             *
             * The skybox provides a distant background for the scene, typically used for
             * sky, space, or environment visuals. Drawing first (before other geometry)
             * can improve depth buffer efficiency.
             *
             * @param enable True to enable the skybox, false to disable it.
             * @param material Material defining the skybox appearance (cube map or similar).
             * @param distance Distance at which the skybox geometry is positioned (default 5000).
             * @param drawFirst True to render skybox before other geometry (default true).
             */
            void setSkyBox( bool enable, SmartPtr<IMaterial> material, f32 distance = 5000,
                            bool drawFirst = true ) override;

            void setSkyBox( bool enable, SmartPtr<ITexture> texture, f32 distance = 5000,
                            bool drawFirst = true ) override;

            /**
             * @brief Sets the fog parameters for the scene.
             *
             * Configures distance-based fog to simulate atmospheric effects. Supports
             * multiple fog modes: none, linear (distance-based), and exponential (density-based).
             *
             * @param fogMode Fog calculation mode (0=none, 1=exponential, 2=exponential squared,
             * 3=linear).
             * @param colour Fog color that geometry blends toward at distance (default white).
             * @param expDensity Density coefficient for exponential fog modes (default 0.001).
             * @param linearStart Distance where linear fog starts to apply (default 0.0).
             * @param linearEnd Distance where linear fog reaches full opacity (default 1.0).
             */
            void setFog( u32 fogMode, const ColourF &colour = ColourF::White, f32 expDensity = 0.001f,
                         f32 linearStart = 0.0f, f32 linearEnd = 1.0f ) override;

            /**
             * @brief Checks if the skybox is enabled.
             * @return True if the skybox is enabled, false otherwise.
             */
            bool getEnableSkybox() const override;

            /**
             * @brief Checks if shadows are enabled in the scene.
             * @return True if shadows are enabled, false otherwise.
             */
            bool getEnableShadows() const override;

            /**
             * @brief Enables or disables shadows in the scene.
             * @param enableShadows Whether to enable shadows.
             * @param depthShadows Whether to use depth shadows (default true).
             */
            void setEnableShadows( bool enableShadows, bool depthShadows = true ) override;

            /**
             * @brief Casts a ray into the scene and returns the first intersection point.
             *
             * Performs ray-scene intersection testing against all collidable scene geometry.
             * Returns the closest hit point along the ray direction. Useful for picking,
             * line-of-sight tests, and physics queries.
             *
             * @param ray Ray definition with origin and direction for intersection testing.
             * @param result Output parameter receiving the world-space intersection point if hit occurs.
             * @return True if the ray intersected any scene object, false if no hit.
             */
            bool castRay( const Ray3<real_Num> &ray, Vector3<real_Num> &result ) override;

            /**
             * @brief Splits a mesh into multiple sub-meshes based on splitting criteria.
             *
             * Divides a single mesh into separate mesh objects according to properties such as
             * material boundaries, vertex groups, or geometric criteria. Useful for optimizing
             * rendering or creating separate collision meshes.
             *
             * @param mesh Source mesh to split into parts.
             * @param properties Configuration properties defining split criteria and behavior.
             * @return Array of smart pointers to the resulting split meshes.
             */
            virtual Array<SmartPtr<IGraphicsMesh>> splitMesh( SmartPtr<IGraphicsMesh> mesh,
                                                              const SmartPtr<Properties> &properties );

            /**
             * @brief Creates an instance manager for efficient instanced rendering.
             *
             * Instance managers handle batching of multiple identical mesh instances for
             * high-performance rendering. They group instances to minimize draw calls and
             * state changes.
             *
             * @param customName Unique identifier for this instance manager.
             * @param meshName Name of the mesh resource to instance.
             * @param groupName Resource group containing the mesh.
             * @param technique Instancing technique ID (hardware/shader instancing method).
             * @param numInstancesPerBatch Maximum instances to render in a single batch.
             * @param flags Optional behavior flags for the instance manager (default 0).
             * @param subMeshIdx Index of the sub-mesh to instance if mesh has multiple parts (default
             * 0).
             * @return Smart pointer to the created instance manager, or nullptr on failure.
             */
            virtual SmartPtr<IInstanceManager> createInstanceManager(
                const String &customName, const String &meshName, const String &groupName, u32 technique,
                u32 numInstancesPerBatch, u16 flags = 0, u16 subMeshIdx = 0 );

            /**
             * @brief Creates an instanced object within an existing instance manager.
             *
             * Allocates a single instance slot from the specified manager. Each instanced
             * object represents one occurrence of the instanced mesh with its own transform
             * and material variant.
             *
             * @param materialName Material to apply to this instance.
             * @param managerName Name of the instance manager to allocate from.
             * @return Smart pointer to the created instanced object, or nullptr on failure.
             */
            virtual SmartPtr<IInstancedObject> createInstancedObject( const String &materialName,
                                                                      const String &managerName );

            /**
             * @brief Destroys an instanced object and frees its instance slot.
             *
             * Removes the instance from the rendering batch and releases its resources.
             * The slot becomes available for reuse by the instance manager.
             *
             * @param instancedObject The instanced object to destroy and deallocate.
             */
            virtual void destroyInstancedObject( SmartPtr<IInstancedObject> instancedObject );

            /**
             * @brief Gets raw pointer to the factory manager.
             *
             * Returns a non-owning pointer for performance-critical access.
             *
             * @return Raw pointer to the factory manager, or nullptr if not set.
             */
            IFactoryManager *getFactoryManagerPtr() const;

            /**
             * @brief Gets smart pointer to the factory manager.
             *
             * The factory manager creates graphics objects by type name or ID.
             *
             * @return Smart pointer to the factory manager, or nullptr if not set.
             */
            SmartPtr<IFactoryManager> getFactoryManager() const;

            /**
             * @brief Sets the factory manager for creating graphics objects.
             *
             * The factory manager must be set before adding graphics objects to the scene.
             *
             * @param factoryManager Smart pointer to the factory manager to use.
             */
            void setFactoryManager( SmartPtr<IFactoryManager> factoryManager );

            /**
             * @brief Handles incoming state messages for the scene.
             *
             * Processes state system messages that affect scene-wide state transitions.
             *
             * @param message State message to process.
             * @return True if the message was handled, false otherwise.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message );

            /**
             * @brief Handles scene state transitions.
             *
             * Called when the scene's loading or resource state changes.
             *
             * @param state New state object for the scene.
             * @return True if the state change was processed successfully.
             */
            bool handleStateChanged( SmartPtr<IState> &state );

            /**
             * @brief Gets the string pool used for efficient string management.
             *
             * The string pool reduces memory allocations by reusing string instances.
             *
             * @return Raw pointer to the string pool, or nullptr if not set.
             */
            StringPool<c8> *getStringPool() const;

            /**
             * @brief Sets the string pool for efficient string management.
             *
             * Assigns a string pool that the scene will use for interning strings.
             * The pool must remain valid for the lifetime of the scene.
             *
             * @param pool Raw pointer to the string pool to use.
             */
            void setStringPool( StringPool<c8> *pool );

            /**
             * @brief Gets the underlying implementation object pointer.
             *
             * Provides access to the backend-specific scene object for interoperability
             * with rendering APIs or low-level systems. Handle with care.
             *
             * @param ppObject Output parameter receiving pointer to the underlying object.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Gets raw pointer to the scene node state context.
             * @return Raw pointer to scene node state context, or nullptr if not set.
             */
            IStateContext *getSceneNodeContextPtr() const;

            /**
             * @brief Gets smart pointer to the scene node state context.
             * @return Smart pointer to scene node state context.
             */
            SmartPtr<IStateContext> getSceneNodeContext() const;

            /**
             * @brief Sets the state context for managing scene node state transitions.
             * @param context Smart pointer to the state context for scene nodes.
             */
            void setSceneNodeContext( SmartPtr<IStateContext> context );

            /**
             * @brief Gets raw pointer to the graphics object state context.
             * @return Raw pointer to graphics object state context, or nullptr if not set.
             */
            IStateContext *getGraphicsObjectContextPtr( u32 typeId ) const;

            /**
             * @brief Gets smart pointer to the graphics object state context.
             * @return Smart pointer to graphics object state context.
             */
            SmartPtr<IStateContext> getGraphicsObjectContext( u32 typeId ) const;

            /**
             * @brief Sets the state context for managing graphics object state transitions.
             * @param context Smart pointer to the state context for graphics objects.
             */
            void setGraphicsObjectContext( u32 typeId, SmartPtr<IStateContext> context );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Marks the scene state dirty after a dependency collection changes. */
            void invalidateSceneState();

            /** Weak pointer to state context for scene node lifecycle management */
            AtomicSmartPtr<IStateContext> m_sceneNodeContext;

            /** Root node of the scene graph hierarchy; all scene nodes descend from this */
            AtomicSmartPtr<IGraphicsSceneNode> m_rootSceneNode;

            /** Default camera used when no active camera is explicitly set */
            AtomicSmartPtr<IGraphicsCamera> m_defaultCamera;

            /** Factory manager responsible for creating graphics objects by type */
            AtomicSmartPtr<IFactoryManager> m_factoryManager;

            /** Thread-safe container of all active animations in the scene */
            ConcurrentArray<SmartPtr<IAnimation>> m_animations;

            /** Thread-safe container of registered scene nodes for fast lookup and iteration */
            ConcurrentArray<SmartPtr<IGraphicsSceneNode>> m_registeredSceneNodes;

            /** Thread-safe container of raw pointers to registered graphics objects */
            ConcurrentArray<IGraphicsObject *> m_registeredGfxObjects;

            /** Thread-safe container of scene nodes created and managed by this scene */
            ConcurrentArray<SmartPtr<IGraphicsSceneNode>> m_sceneNodes;

            /** Thread-safe container of camera objects in the scene */
            ConcurrentArray<SmartPtr<IGraphicsObject>> m_cameras;

            /** Thread-safe container of all graphics objects (meshes, lights, cameras) in the scene */
            ConcurrentArray<SmartPtr<IGraphicsObject>> m_graphicsObjects;

            /** Thread-safe container of active particle systems */
            ConcurrentArray<SmartPtr<IParticleSystem>> m_particleSystems;

            /** Thread-safe container of sky/skybox objects */
            ConcurrentArray<SmartPtr<ISky>> m_skies;

            /** Thread-safe container of terrain objects */
            ConcurrentArray<SmartPtr<IGraphicsTerrain>> m_terrains;

            /** Pointer to string pool for efficient string storage and lookup */
            AtomicRawPtr<StringPool<c8>> m_stringPool;

            /** Flag indicating clearScene() is in progress to prevent recursive operations */
            atomic_bool m_isClearing = false;

            /** Type identifier string for this scene instance */
            AtomicObject<FixedString<32>> m_type;

            /** Weak pointer to state context for graphics object lifecycle management */
            ConcurrentHashMap<u32, WeakPtr<IStateContext>> m_graphicsObjectContexts;

            /** Static counter for generating unique node names across all scene instances */
            static u32 nodeCounter;
        };

        /**
         * @brief Inline implementation of StateListener::getOwnerPtr().
         * @return Raw pointer to owning GraphicsScene.
         */
        inline GraphicsScene *GraphicsScene::StateListener::getOwnerPtr() const
        {
            return m_owner.get();
        }

        /**
         * @brief Inline implementation of getSceneNodeContextPtr().
         * @return Raw pointer to scene node state context.
         */
        inline IStateContext *GraphicsScene::getSceneNodeContextPtr() const
        {
            return m_sceneNodeContext.get();
        }

        /**
         * @brief Inline implementation of getGraphicsObjectContextPtr().
         * @return Raw pointer to graphics object state context.
         */
        inline IStateContext *GraphicsScene::getGraphicsObjectContextPtr( u32 typeId ) const
        {
            auto it = m_graphicsObjectContexts.find( typeId );
            if( it != m_graphicsObjectContexts.end() )
            {
                return it->second.get();
            }

            return nullptr;
        }

        /**
         * @brief Inline implementation of getFactoryManagerPtr().
         * @return Raw pointer to factory manager.
         */
        inline IFactoryManager *GraphicsScene::getFactoryManagerPtr() const
        {
            return m_factoryManager.get();
        }

    }  // namespace render
}  // namespace workphone

#endif  // CGraphicsScene_h__
