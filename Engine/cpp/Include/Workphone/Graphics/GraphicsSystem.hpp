#ifndef __WP_GraphicsSystem_h__
#define __WP_GraphicsSystem_h__

#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class GraphicsSystem
         * @brief Concrete implementation of the graphics subsystem manager.
         *
         * The GraphicsSystem coordinates the engine's graphics subsystems including
         * render windows, scene managers, resource/factory managers and various
         * renderer helpers (deferred shading, sprite/instance managers, etc.).
         * It exposes lifecycle methods for loading, unloading and per-frame updates
         * and provides thread-safe access to internal collections using a recursive
         * mutex and concurrent containers.
         *
         * Primary responsibilities:
         * - Maintain and provide snapshots of active render windows.
         * - Create, store and return graphics scene managers and a default scene.
         * - Own and expose factory, resource and manager objects used by renderers.
         * - Queue and process asynchronous load/unload requests for graphics objects.
         *
         * Threading and concurrency:
         * - Uses `ConcurrentArray` / `ConcurrentQueue` for collections and task queues.
         * - Uses `RecursiveMutex m_mutex` to protect sequences of operations that
         *   require exclusive access.
         *
         * @note Inherits from `SharedGraphicsObject<IGraphicsSystem>` and implements
         *       the `IGraphicsSystem` interface.
         */
        class WPCore_API GraphicsSystem : public SharedGraphicsObject<IGraphicsSystem>
        {
        public:
            class WPCore_API StateListener : public IStateListener
            {
            public:
                /**
                 * @brief Default constructor.
                 *
                 * Creates a state listener without an associated material owner.
                 * The owner must be set separately using setOwner().
                 */
                StateListener();

                /**
                 * @brief Virtual destructor.
                 *
                 * Properly cleans up the state listener and removes any
                 * remaining state context associations.
                 */
                ~StateListener() override;

                /**
                 * @brief Handles the unload state change.
                 *
                 * Called when the material or its resources need to be unloaded.
                 * This ensures proper cleanup of graphics resources.
                 *
                 * @param data Optional context data for the unload operation
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handles incoming state messages.
                 *
                 * Processes state messages that may affect the material,
                 * such as resource loading notifications or graphics context changes.
                 *
                 * @param message The state message to handle
                 * @return True if the message was handled, false otherwise
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handles state transitions.
                 *
                 * Called when the material's state changes, allowing the listener
                 * to respond to loading state transitions and resource changes.
                 *
                 * @param state Reference to the new state
                 * @return True if the state change was handled successfully
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Gets a raw pointer to the material that owns this listener.
                 * @return Raw pointer to the owner material
                 */
                GraphicsSystem *getOwnerPtr() const;

                /**
                 * @brief Gets the material that owns this listener.
                 *
                 * Returns a smart pointer to the material associated with this listener.
                 *
                 * @return Smart pointer to the owner material
                 */
                SmartPtr<GraphicsSystem> getOwner() const;

                /**
                 * @brief Sets the material owner for this listener.
                 *
                 * Associates this listener with the specified material.
                 *
                 * @param owner Smart pointer to the material to associate with
                 */
                void setOwner( SmartPtr<GraphicsSystem> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                /** Weak pointer to the owning material to avoid circular references */
                AtomicWeakPtr<GraphicsSystem> m_owner;
            };

            /**
             * @brief Construct a new GraphicsSystem.
             *
             * Initializes internal containers and state. Constructing does not automatically
             * load graphics resources; call load(...) to perform initialization that requires
             * external data or resources.
             */
            GraphicsSystem();

            /**
             * @brief Destroy the GraphicsSystem.
             *
             * Performs any required cleanup. Users should call unload(...) prior to destruction
             * to ensure resources are released in a controlled manner.
             */
            ~GraphicsSystem() override;

            /**
             * @brief Load or enqueue loading of graphics resources.
             *
             * Implements IGraphicsSystem::load. The provided data object can contain
             * configuration or resource descriptors required to initialise parts of the
             * graphics system. Implementations may choose to perform immediate loading
             * or enqueue the request to be processed by the update loop.
             *
             * @param data Smart pointer to an ISharedObject with load parameters or resources.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload or enqueue unloading of graphics resources.
             *
             * Implements IGraphicsSystem::unload. The provided data object identifies
             * resources to remove or deinitialise. The function may perform immediate
             * unload or defer to the internal unload queue.
             *
             * @param data Smart pointer to an ISharedObject identifying resources to unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Update the graphics system.
             *
             * Implements IGraphicsSystem::update. This method is expected to:
             * - Process queued load/unload requests via updateLoadQueue/updateUnloadQueue.
             * - Update managed scenes and window state.
             * - Perform per-frame maintenance tasks required by the graphics backend.
             *
             * This should be called regularly on the main graphics thread or an appropriate
             * update thread depending on the application's architecture.
             */
            void update() override;

            /**
             * @brief Update the rendering of all managed windows and scenes.
             */
            void render() override;

            /**
             * @brief Acquire the internal mutex for exclusive access.
             *
             * Use lock()/unlock() to protect a sequence of operations that must be atomic
             * with respect to the GraphicsSystem internal state.
             */
            void lock() override;

            /**
             * @brief Try to acquire the internal mutex without blocking.
             *
             * Implements IGraphicsSystem::lock semantics for non-blocking acquisition.
             *
             * @return true if the lock was acquired, false otherwise.
             */
            bool try_lock() override;

            /**
             * @brief Release the internal mutex previously acquired by lock()/try_lock().
             */
            void unlock() override;

            /**
             * @brief Get a snapshot of managed windows.
             *
             * Returns a copy (Array) of smart pointers to the currently managed windows.
             * The returned collection is safe to iterate without holding the GraphicsSystem lock,
             * but individual window objects are reference-counted SmartPtr instances and may be
             * shared across threads.
             *
             * @return Array<SmartPtr<IGraphicsWindow>> A copy of the active windows list.
             */
            Array<SmartPtr<IGraphicsWindow>> getWindows() const override;

            /**
             * @brief Get all scene managers (graphics scenes).
             *
             * Returns a copy of the internal scenes collection.
             *
             * @return Array<SmartPtr<IGraphicsScene>> A snapshot of scenes managed by the system.
             */
            Array<SmartPtr<IGraphicsScene>> getSceneManagers() const override;

            /**
             * @brief Retrieve a graphics scene by its name.
             *
             * Performs a lookup by the provided scene name and returns the associated
             * SmartPtr if found; otherwise returns a null SmartPtr.
             *
             * @param name The name of the graphics scene to find.
             * @return SmartPtr<IGraphicsScene> Scene manager with the matching name or null.
             */
            SmartPtr<IGraphicsScene> getGraphicsScene( const String &name ) const override;

            /**
             * @brief Retrieve a graphics scene by its hashed identifier.
             *
             * @param id The hashed ID of the scene.
             * @return SmartPtr<IGraphicsScene> Scene manager matching the id or null.
             */
            SmartPtr<IGraphicsScene> getGraphicsSceneById( hash_type id ) const override;

            /**
             * @brief Get the current default graphics scene.
             *
             * Returns the current default scene manager (if one is set).
             *
             * @return SmartPtr<IGraphicsScene> The default scene manager or null if none set.
             */
            SmartPtr<IGraphicsScene> getGraphicsScene() const override;

            /**
             * @brief Get a raw pointer to the default graphics scene.
             */
            IGraphicsScene *getGraphicsScenePtr() const override;

            /**
             * @brief Set the provided scene as the default scene manager.
             *
             * The provided SmartPtr will become the default scene manager used for operations
             * that rely on a single active scene.
             *
             * @param smgr Smart pointer to the scene to set as default.
             */
            void setGraphicsScene( SmartPtr<IGraphicsScene> smgr );

            /**
             * @brief Get a raw pointer to the factory manager.
             *
             * Returns a raw pointer for use in contexts that do not accept SmartPtr.
             * Caller must not take ownership; the GraphicsSystem retains responsibility
             * for the lifetime of the factory manager object.
             *
             * @return IFactoryManager* Raw pointer to the factory manager or nullptr.
             */
            IFactoryManager *getFactoryManagerPtr() const override;

            /**
             * @brief Get the factory manager as a SmartPtr.
             *
             * @return SmartPtr<IFactoryManager> Smart pointer to the factory manager or null.
             */
            SmartPtr<IFactoryManager> getFactoryManager() const override;

            /**
             * @brief Set the factory manager.
             *
             * Replaces the current factory manager used to create graphics objects.
             *
             * @param factoryManager Smart pointer to the new factory manager.
             */
            void setFactoryManager( SmartPtr<IFactoryManager> factoryManager ) override;

            /**
             * @brief Get the load priority for an object.
             *
             * Implementations may override this to assign priorities for load queue processing.
             * Higher priority objects should be processed before lower priority ones.
             *
             * @param obj Smart pointer to the object for which to determine load priority.
             * @return s32 Integer priority value (higher = processed earlier). Default behaviour
             *             should be provided by the concrete implementation.
             */
            virtual s32 getLoadPriority( SmartPtr<ISharedObject> obj );

            /**
             * @brief Create a new graphics configuration object.
             *
             * Creates and returns a configuration object that can be used to set up
             * graphics system parameters prior to initialization.
             *
             * @return SmartPtr<IBuildDirector> A new graphics settings object, or null if creation
             * fails.
             */
            SmartPtr<IBuildDirector> createConfiguration() override;

            /**
             * @brief Configure the graphics system with the provided settings.
             *
             * Applies the configuration settings to the graphics system. This typically
             * includes render API selection, window parameters, and renderer-specific options.
             *
             * @param config Smart pointer to the graphics settings to apply.
             * @return bool True if configuration was successful, false otherwise.
             */
            bool configure( SmartPtr<IBuildDirector> config ) override;

            /**
             * @brief Process platform-specific message events.
             *
             * Handles OS-level window and input messages. Should be called regularly
             * on the main thread to ensure the application remains responsive.
             */
            void messagePump() override;

            /**
             * @brief Get a raw pointer to the debug interface.
             *
             * Returns a raw pointer to the debug interface used for logging and debug visualization.
             *
             * @return IDebug* Raw pointer to the debug interface, or nullptr if not set.
             */
            IDebug *getDebugPtr() const;

            /**
             * @brief Get the debug interface.
             *
             * Returns the debug interface used for logging, profiling, and runtime debug visualization.
             *
             * @return SmartPtr<IDebug> Smart pointer to the debug interface, or null if not set.
             */
            SmartPtr<IDebug> getDebug() const override;

            /**
             * @brief Set the debug interface.
             *
             * Assigns the debug interface to be used for logging and debug visualization.
             *
             * @param debug Smart pointer to the debug interface to use.
             */
            void setDebug( SmartPtr<IDebug> debug ) override;

            /**
             * @brief Create and register a new graphics scene.
             *
             * Creates a new graphics scene (scene manager) of the specified type and with
             * the given name. The scene is added to the internal collection and initialized.
             * If no default scene exists, the newly created scene becomes the default.
             *
             * @param type Type identifier for the scene (implementation-specific, e.g., "octree",
             * "bsp").
             * @param name Unique name for the scene.
             * @return SmartPtr<IGraphicsScene> The newly created and initialized scene, or null on
             * failure.
             */
            SmartPtr<IGraphicsScene> addGraphicsScene( const String &type, const String &name ) override;

            /**
             * @brief Remove a graphics scene from the system.
             * @param scene Smart pointer to the scene to remove. The scene will be removed from the
             * internal collection and cleaned up.
             */
            void removeGraphicsScene( SmartPtr<IGraphicsScene> scene );

            /**
             * @brief Removes all scene managers from the graphics system.
             */
            void removeAllGraphicsScenes();

            void clearGraphicScenes();

            IOverlayManager *getOverlayManagerPtr() const;

            /**
             * @brief Get the overlay manager.
             *
             * Returns the overlay manager used for 2D screen-space overlays and HUD elements.
             *
             * @return SmartPtr<IOverlayManager> Smart pointer to the overlay manager, or null if not
             * available.
             */
            SmartPtr<IOverlayManager> getOverlayManager() const override;

            /**
             * @brief Get the resource group manager.
             *
             * Returns the resource group manager used for organizing and loading resource groups.
             *
             * @return SmartPtr<IResourceGroupManager> Smart pointer to the resource group manager, or
             * null.
             */
            SmartPtr<IResourceGroupManager> getResourceGroupManager() const override;

            /**
             * @brief Get a raw pointer to the material manager.
             *
             * Returns a raw pointer to the material manager for creating and managing materials.
             *
             * @return IMaterialManager* Raw pointer to the material manager, or nullptr if not
             * available.
             */
            IMaterialManager *getMaterialManagerPtr() const override;

            /**
             * @brief Get the material manager.
             *
             * Returns the material manager used for creating, loading, and managing material resources.
             *
             * @return SmartPtr<IMaterialManager> Smart pointer to the material manager, or null.
             */
            SmartPtr<IMaterialManager> getMaterialManager() const override;

            /**
             * @brief Get the texture manager.
             *
             * Returns the texture manager used for creating, loading, and managing texture resources.
             *
             * @return Pointer to the texture manager, or null.
             */
            ITextureManager *getTextureManagerPtr() const override;

            /**
             * @brief Get the texture manager.
             *
             * Returns the texture manager used for creating, loading, and managing texture resources.
             *
             * @return SmartPtr<ITextureManager> Smart pointer to the texture manager, or null.
             */
            SmartPtr<ITextureManager> getTextureManager() const override;

            /**
             * @brief Get the instance manager.
             *
             * Returns the instance manager used for GPU instancing operations.
             *
             * @return SmartPtr<IInstanceManager> Smart pointer to the instance manager, or null.
             */
            SmartPtr<IInstanceManager> getInstanceManager() const override;

            IRenderer *getRendererPtr() const;

            /**
             * @brief Get the sprite renderer.
             *
             * Returns the sprite renderer used for 2D sprite rendering operations.
             *
             * @return SmartPtr<ISpriteRenderer> Smart pointer to the sprite renderer, or null.
             */
            SmartPtr<IRenderer> getRenderer() const override;

            /**
             * @brief Get the font manager.
             *
             * Returns the font manager used for loading and managing font resources.
             *
             * @return SmartPtr<IFontManager> Smart pointer to the font manager, or null.
             */
            SmartPtr<IFontManager> getFontManager() const override;

            /**
             * @brief Create a new render window.
             *
             * Creates and initializes a new render window with the specified parameters.
             * The window is registered with the graphics system and can be retrieved later by name.
             *
             * @param name Unique name for the window.
             * @param width Width of the window in pixels.
             * @param height Height of the window in pixels.
             * @param fullScreen True to create a fullscreen window, false for windowed mode.
             * @param properties Additional window properties (vsync, MSAA, etc.).
             * @return SmartPtr<IGraphicsWindow> The newly created window, or null on failure.
             */
            SmartPtr<IGraphicsWindow> createRenderWindow(
                const String &name, u32 width, u32 height, bool fullScreen,
                const SmartPtr<Properties> &properties ) override;

            /**
             * @brief Destroys a render window.
             */
            void destroyRenderWindow( SmartPtr<IGraphicsWindow> window ) override;

            /**
             * @brief Retrieve a render window by name.
             *
             * Searches for and returns a window with the specified name from the collection
             * of managed windows.
             *
             * @param name Name of the window to retrieve. If empty, returns the default window.
             * @return SmartPtr<IGraphicsWindow> The window with the matching name, or null if not found.
             */
            SmartPtr<IGraphicsWindow> getRenderWindow(
                const String &name = StringUtil::EmptyString ) const override;

            /**
             * @brief Get the default render window.
             *
             * Returns the default render window, typically the first window created or
             * explicitly set as default.
             *
             * @return SmartPtr<IGraphicsWindow> The default window, or null if none is set.
             */
            SmartPtr<IGraphicsWindow> getDefaultWindow() const override;

            /**
             * @brief Set the default render window.
             *
             * Assigns the specified window as the default window for rendering operations.
             *
             * @param defaultWindow Smart pointer to the window to set as default.
             */
            void setDefaultWindow( SmartPtr<IGraphicsWindow> defaultWindow ) override;

            /**
             * @brief Add a deferred shading system for a viewport.
             *
             * Creates and registers a deferred shading system for the specified viewport,
             * enabling deferred rendering techniques (G-buffer, lighting passes, etc.).
             *
             * @param vp Smart pointer to the viewport to associate with the deferred shading system.
             * @return SmartPtr<IGraphicsDeferredShading> The newly created deferred shading system, or
             * null.
             */
            SmartPtr<IGraphicsDeferredShading> addDeferredShadingSystem(
                SmartPtr<IViewport> vp ) override;

            /**
             * @brief Remove the deferred shading system associated with a viewport.
             *
             * Removes and cleans up the deferred shading system for the specified viewport.
             *
             * @param vp Smart pointer to the viewport whose deferred shading system should be removed.
             */
            void removeDeferredShadingSystem( SmartPtr<IViewport> vp ) override;

            /**
             * @brief Get all deferred shading systems.
             *
             * Returns a collection of all registered deferred shading systems.
             *
             * @return Array<SmartPtr<IGraphicsDeferredShading>> Array of deferred shading systems.
             */
            Array<SmartPtr<IGraphicsDeferredShading>> getDeferredShadingSystems() const override;

            /**
             * @brief Load a graphics object immediately or enqueue for loading.
             *
             * Loads the specified graphics object (mesh, texture, material, etc.). If `forceQueue`
             * is true or if called from a non-main thread, the load request is enqueued for
             * processing during the next update cycle.
             *
             * @param graphicsObject Smart pointer to the graphics object to load.
             * @param forceQueue If true, always enqueue the load request rather than loading
             * immediately.
             */
            void loadObject( SmartPtr<ISharedObject> graphicsObject, bool forceQueue = false ) override;

            /**
             * @brief Reload a graphics object immediately or enqueue for reloading.
             */
            void reloadObject( SmartPtr<ISharedObject> graphicsObject,
                               bool forceQueue = false ) override;

            /**
             * @brief Unload a graphics object immediately or enqueue for unloading.
             *
             * Unloads the specified graphics object, releasing its resources. If `forceQueue`
             * is true or if called from a non-main thread, the unload request is enqueued for
             * processing during the next update cycle.
             *
             * @param graphicsObject Smart pointer to the graphics object to unload.
             * @param forceQueue If true, always enqueue the unload request rather than unloading
             * immediately.
             */
            void unloadObject( SmartPtr<ISharedObject> graphicsObject,
                               bool forceQueue = false ) override;

            /** @copydoc IGraphicsSystem::clearObjectQueues */
            void clearObjectQueues() override;

            /**
             * @brief Configure or teardown renderer state for a scene/window/camera.
             *
             * Establishes renderer-specific state required for rendering into the
             * provided `window` using the supplied `sceneManager` and `camera`.
             * The `workspaceName` identifies a named workspace/context the renderer
             * should use. The `enabled` flag toggles the setup; when false the method
             * may perform teardown or skip enabling renderer features.
             *
             * @param sceneManager Smart pointer to the graphics scene manager to use.
             * @param window Smart pointer to the render window to configure.
             * @param camera Smart pointer to the camera to render from.
             * @param workspaceName Name of the renderer workspace/context to use.
             * @param enabled True to enable/setup the renderer for the given inputs,
             *                false to disable/teardown related renderer state.
             */
            void setupRenderer( SmartPtr<IGraphicsScene> sceneManager, SmartPtr<IGraphicsWindow> window,
                                SmartPtr<IGraphicsCamera> camera, String workspaceName,
                                bool enabled ) override;

            /**
             * @brief Get the task identifier for state update operations.
             *
             * Returns the task ID associated with state update operations, used by the
             * task manager to schedule graphics state updates on the appropriate thread.
             *
             * @return TaskId The task identifier for state updates (typically TaskId::Primary).
             */
            TaskId getStateTask() const override;

            /**
             * @brief Get the task identifier for render operations.
             *
             * Returns the task ID associated with rendering operations, used by the
             * task manager to schedule rendering work on the appropriate thread.
             *
             * @return TaskId The task identifier for rendering (typically TaskId::Render).
             */
            TaskId getRenderTask() const override;

            /**
             * @brief Get the mesh converter.
             *
             * Returns the mesh converter used for importing and converting mesh data
             * from various formats into the engine's internal representation.
             *
             * @return SmartPtr<IMeshConverter> Smart pointer to the mesh converter, or null.
             */
            SmartPtr<IMeshConverter> getMeshConverter() const override;

            /**
             * @brief Set the mesh converter.
             *
             * Assigns the mesh converter to be used for mesh import and conversion operations.
             *
             * @param meshConverter Smart pointer to the mesh converter to use.
             */
            void setMeshConverter( SmartPtr<IMeshConverter> meshConverter ) override;

            /**
             * @brief Get the global string pool.
             *
             * Returns a raw pointer to the string pool used for efficient string storage
             * and management throughout the graphics system.
             *
             * @return StringPool<c8>* Raw pointer to the string pool, or nullptr if not set.
             */
            StringPool<c8> *getStringPool() const;

            /**
             * @brief Set the global string pool.
             *
             * Assigns the string pool to be used for efficient string storage and management
             * throughout the graphics system.
             *
             * @param pool Raw pointer to the string pool to use.
             */
            void setStringPool( StringPool<c8> *pool );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Process pending load requests.
             *
             * Called internally (typically from update()) to dequeue and handle load requests.
             * Concrete implementations will convert queued ISharedObject requests into actual
             * graphics resource initialisation.
             */
            void updateLoadQueue();

            /**
             * @brief Process pending unload requests.
             *
             * Called internally (typically from update()) to dequeue and handle unload requests.
             * Concrete implementations should release or schedule release of resources identified
             * by the queued ISharedObject instances.
             */
            void updateUnloadQueue();

            /**< Currently selected render API. */
            RenderApi m_renderApi = RenderApi::None;

            ///< Sprite renderer for 2D rendering.
            AtomicSmartPtr<IRenderer> m_renderer;

            /**< Debug interface used for logging and runtime debug UI. */
            AtomicSmartPtr<IDebug> m_debug;

            /**< Default graphics scene manager (thread-safe pointer). */
            AtomicSmartPtr<IGraphicsScene> m_sceneManager;

            /**< Overlay manager (thread-safe pointer). */
            AtomicSmartPtr<IOverlayManager> m_overlayMgr;

            /**< Resource group manager wrapper. */
            AtomicSmartPtr<IResourceGroupManager> m_resourceGroupManager;

            /**< Material manager wrapper. */
            AtomicSmartPtr<IMaterialManager> m_materialManager;

            /**< Texture manager wrapper. */
            AtomicSmartPtr<ITextureManager> m_textureManager;

            /**< Instance manager for GPU instancing. */
            AtomicSmartPtr<IInstanceManager> m_instanceManager;

            /**< Mesh converter helper for import/convert operations. */
            AtomicSmartPtr<IMeshConverter> m_meshConverter;

            /**< Mesh resource manager. */
            SmartPtr<IResourceManager> m_meshManager;

            /**< Default render window used by the system. */
            AtomicSmartPtr<IGraphicsWindow> m_defaultWindow;

            /**< Font manager for debug/UI fonts. */
            AtomicSmartPtr<IFontManager> m_fontManager;

            /** Factory manager used to create graphics objects. */
            AtomicSmartPtr<IFactoryManager> m_factoryManager;

            /**< Resource group manager instance (thread-safe pointer). */
            AtomicSmartPtr<IResourceGroupManager> m_resGrpMgr;

            /** Collection of active windows (thread-safe container). */
            ConcurrentArray<SmartPtr<IGraphicsWindow>> m_windows;

            /** Collection of graphics scene managers (thread-safe container). */
            ConcurrentArray<SmartPtr<IGraphicsScene>> m_scenes;

            /**< Collection of deferred shading systems for viewports. */
            Array<SmartPtr<IGraphicsDeferredShading>> m_deferredShadingSystems;

            /**< Queue of graphics objects pending load/unload. */
            ConcurrentArray<SmartPtr<ISharedObject>> m_graphicsObjects;

            /** Queue of requested load operations (thread-safe). */
            ConcurrentQueue<SmartPtr<ISharedObject>> m_loadQueue;

            /** Queue of requested load operations (thread-safe). */
            ConcurrentQueue<SmartPtr<ISharedObject>> m_reloadQueue;

            /** Queue of requested unload operations (thread-safe). */
            ConcurrentQueue<SmartPtr<ISharedObject>> m_unloadQueue;

            ///< String pool for efficient string management
            AtomicRawPtr<StringPool<c8>> m_stringPool;

            /**
             * @brief Recursive mutex protecting operations that require exclusive access.
             *
             * Use lock()/unlock() or try_lock() to manipulate the mutex from client code
             * when performing multi-step operations that must not be interrupted.
             */
            mutable RecursiveSpinMutex m_mutex;
        };

        inline StringPool<c8> *GraphicsSystem::getStringPool() const
        {
            return m_stringPool;
        }

        inline void GraphicsSystem::setStringPool( StringPool<c8> *pool )
        {
            m_stringPool = pool;
        }

        inline IOverlayManager *GraphicsSystem::getOverlayManagerPtr() const
        {
            return m_overlayMgr.get();
        }

    }  // namespace render
}  // namespace workphone

#endif  // CGraphicsSystem_h__
