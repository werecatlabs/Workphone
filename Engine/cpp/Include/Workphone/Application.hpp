#ifndef __WP_Application_H_
#define __WP_Application_H_

#include <Workphone/Interface/IApplication.hpp>
#include <Workphone/Interface/System/IFSM.hpp>
#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>

namespace workphone
{
    namespace core
    {
        /**
         * @brief Base application class that manages the standard startup and lifecycle of a game
         * application.
         *
         * The Application class provides a complete framework for game development by managing:
         * - Application lifecycle (load, unload, run, iterate, update)
         * - Core systems initialization (graphics, physics, input, audio, etc.)
         * - Scene management and rendering pipeline
         * - Event handling and state management
         * - Resource management and factory systems
         * - Threading and task management
         *
         * This class is designed to be subclassed for specific game requirements while providing
         * sensible defaults for common functionality. It implements the IApplication interface
         * and provides a robust foundation for game development.
         *
         * @note This class is thread-safe and uses recursive mutex for synchronization.
         * @see IApplication
         * @see IFSM
         * @see IEventListener
         */
        class WPCore_API Application : public IApplication
        {
        public:
            /**
             * @brief Event listener for handling application-level events.
             *
             * This nested class handles events specific to the Application class,
             * providing a way to respond to application state changes and system events.
             * It maintains a weak reference to its owner Application to avoid circular references.
             *
             * @see IEventListener
             */
            class ApplicationEventListener : public IEventListener
            {
            public:
                /**
                 * @brief Default constructor.
                 *
                 * Initializes the event listener with a null owner reference.
                 */
                ApplicationEventListener();

                /**
                 * @brief Virtual destructor.
                 *
                 * Ensures proper cleanup of the event listener.
                 */
                ~ApplicationEventListener() override;

                /**
                 * @brief Handles the unloading.
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handles application events.
                 *
                 * Processes events sent to the application and delegates them to the owner
                 * Application instance for further processing.
                 *
                 * @param eventType The type of event that occurred.
                 * @param eventValue The hash value identifying the specific event.
                 * @param arguments Array of parameters associated with the event.
                 * @param sender The object that sent the event.
                 * @param object The target object for the event.
                 * @param event The event object containing additional information.
                 * @return Parameter containing the result of event processing.
                 *
                 * @see EventType
                 * @see Parameter
                 * @see ISharedObject
                 * @see IEvent
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Gets the owner Application instance.
                 *
                 * @return SmartPtr to the owner Application, or null if not set.
                 */
                SmartPtr<Application> getOwner() const;

                /**
                 * @brief Sets the owner Application instance.
                 *
                 * @param owner The Application instance that owns this event listener.
                 */
                void setOwner( SmartPtr<Application> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                /** @brief Weak reference to the owner Application to prevent circular references. */
                AtomicWeakPtr<Application> m_owner;
            };

            /** @brief Default viewport identifier. */
            static const hash_type defaultVP;

            /**
             * @brief Default constructor.
             *
             * Initializes the application with default settings and creates the event listener.
             */
            Application();

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup of all application resources and systems.
             */
            ~Application() override;

            /**
             * @brief Loads the game application and initializes all required systems.
             *
             * This method performs the complete initialization sequence:
             * - Creates core managers (AI, log, factory, state, timer, etc.)
             * - Initializes threading and task management
             * - Sets up graphics, physics, and input systems
             * - Creates the scene and rendering pipeline
             * - Loads plugins and scripts
             *
             * @param data Optional data object containing initialization parameters.
             *
             * @see unload()
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the game application and cleans up all resources.
             *
             * Performs cleanup in reverse order of initialization:
             * - Destroys plugins and scripts
             * - Shuts down graphics, physics, and input systems
             * - Cleans up scene and rendering resources
             * - Destroys managers and releases memory
             *
             * @param data Optional data object containing cleanup parameters.
             *
             * @see load()
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Starts the main game loop.
             *
             * Contains a default implementation that runs the application until exit.
             * This method should be overridden in derived classes to implement
             * specific game logic and rendering loops.
             *
             * @remarks The default implementation provides a basic loop structure.
             *          Override this method to add custom game logic, rendering,
             *          and input handling.
             */
            void run() override;

            /**
             * @brief Performs one iteration of the application loop.
             *
             * Called each frame to update the application state, process events,
             * and advance the simulation. This is the core of the game loop.
             *
             * @see update()
             */
            void iterate() override;

            /**
             * @brief Updates the application state for the current frame.
             *
             * Called during each iteration to update game logic, physics,
             * animations, and other time-dependent systems.
             *
             * @see iterate()
             */
            void update() override;

            /**
             * @brief Handles application-level events.
             *
             * Processes events sent to the application and delegates them to
             * the appropriate subsystems or state machine.
             *
             * @param eventType The type of event that occurred.
             * @param eventValue The hash value identifying the specific event.
             * @param arguments Array of parameters associated with the event.
             * @param sender The object that sent the event.
             * @param object The target object for the event.
             * @param event The event object containing additional information.
             * @return Parameter containing the result of event processing.
             *
             * @see EventType
             * @see Parameter
             * @see ISharedObject
             * @see IEvent
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            /**
             * @brief Gets the finite state machine managing application states.
             *
             * @return Pointer to the finite state machine, or null if not initialized.
             *
             * @see setFSM()
             * @see IFSM
             */
            IFSM *getFSMPtr() const override;

            /**
             * @brief Gets the finite state machine managing application states.
             *
             * @return SmartPtr to the finite state machine, or null if not initialized.
             *
             * @see setFSM()
             * @see IFSM
             */
            SmartPtr<IFSM> getFSM() const override;

            /**
             * @brief Sets the finite state machine for managing application states.
             *
             * @param fsm The finite state machine to use for state management.
             *
             * @see getFSM()
             * @see IFSM
             */
            void setFSM( SmartPtr<IFSM> fsm ) override;

            /**
             * @brief Gets the number of active threads in the application.
             *
             * @return The number of threads currently active in the thread pool.
             *
             * @see setActiveThreads()
             */
            u32 getActiveThreads() const override;

            /**
             * @brief Sets the number of active threads for the application.
             *
             * @param activeThreads The number of threads to use in the thread pool.
             *
             * @see getActiveThreads()
             */
            void setActiveThreads( u32 activeThreads ) override;

            /**
             * @brief Creates a scene object of the specified type.
             *
             * @param type The type identifier for the scene object to create.
             * @param director The scene director that will manage the created object.
             * @return SmartPtr to the created scene actor, or null if creation failed.
             *
             * @see scene::IActor
             * @see IBuildDirector
             */
            SmartPtr<scene::IGameActor> createSceneObject( u32 type,
                                                           SmartPtr<IBuildDirector> director ) override;

            /**
             * @brief Creates a default UI panel.
             *
             * @param director The scene director that will manage the panel.
             * @param hint Optional hint text for the panel.
             * @param addToScene Whether to automatically add the panel to the scene.
             * @return SmartPtr to the created panel actor, or null if creation failed.
             *
             * @see scene::IActor
             * @see IBuildDirector
             */
            SmartPtr<scene::IGameActor> createPanel( SmartPtr<IBuildDirector> director,
                                                     const String &hint = "",
                                                     bool addToScene = true ) override;

            /**
             * @brief Creates a default UI button.
             *
             * @param label The text label for the button.
             * @param director The scene director that will manage the button.
             * @param hint Optional hint text for the button.
             * @param addToScene Whether to automatically add the button to the scene.
             * @return SmartPtr to the created button actor, or null if creation failed.
             *
             * @see scene::IActor
             * @see IBuildDirector
             */
            SmartPtr<scene::IGameActor> createButton( const String &label,
                                                      SmartPtr<IBuildDirector> director,
                                                      const String &hint = "",
                                                      bool addToScene = true ) override;

            /**
             * @brief Creates a default UI text element.
             *
             * @param label The text content to display.
             * @param director The scene director that will manage the text element.
             * @param hint Optional hint text for the text element.
             * @param addToScene Whether to automatically add the text element to the scene.
             * @return SmartPtr to the created text actor, or null if creation failed.
             *
             * @see scene::IActor
             * @see IBuildDirector
             */
            SmartPtr<scene::IGameActor> createText( const String &label,
                                                    SmartPtr<IBuildDirector> director,
                                                    const String &hint = "",
                                                    bool addToScene = true ) override;

            /**
             * @brief Creates a default UI toggle/checkbox element.
             *
             * @param label The text label for the toggle.
             * @param director The scene director that will manage the toggle.
             * @param hint Optional hint text for the toggle.
             * @param addToScene Whether to automatically add the toggle to the scene.
             * @return SmartPtr to the created toggle actor, or null if creation failed.
             *
             * @see scene::IActor
             * @see IBuildDirector
             */
            SmartPtr<scene::IGameActor> createToggle( const String &label,
                                                      SmartPtr<IBuildDirector> director,
                                                      const String &hint = "",
                                                      bool addToScene = true ) override;

            /**
             * @brief Creates a default UI slider element.
             *
             * @param label The text label for the slider.
             * @param director The scene director that will manage the slider.
             * @param hint Optional hint text for the slider.
             * @param addToScene Whether to automatically add the slider to the scene.
             * @return SmartPtr to the created slider actor, or null if creation failed.
             *
             * @see scene::IActor
             * @see IBuildDirector
             */
            SmartPtr<scene::IGameActor> createSlider( const String &label,
                                                      SmartPtr<IBuildDirector> director,
                                                      const String &hint = "",
                                                      bool addToScene = true ) override;

            /**
             * @brief Creates a default UI scrollbar element.
             *
             * @param label The text label for the scrollbar.
             * @param director The scene director that will manage the scrollbar.
             * @param hint Optional hint text for the scrollbar.
             * @param addToScene Whether to automatically add the scrollbar to the scene.
             * @return SmartPtr to the created scrollbar actor, or null if creation failed.
             *
             * @see scene::IActor
             * @see IBuildDirector
             */
            SmartPtr<scene::IGameActor> createScrollbar( const String &label,
                                                         SmartPtr<IBuildDirector> director,
                                                         const String &hint = "",
                                                         bool addToScene = true ) override;

            /**
             * @brief Creates a default sky environment.
             *
             * @param addToScene Whether to automatically add the sky to the scene.
             * @return SmartPtr to the created sky actor, or null if creation failed.
             *
             * @see scene::IActor
             */
            SmartPtr<scene::IGameActor> createDefaultSky( bool addToScene = true ) override;

            /**
             * @brief Creates a default camera for the scene.
             *
             * @param addToScene Whether to automatically add the camera to the scene.
             * @return SmartPtr to the created camera actor, or null if creation failed.
             *
             * @see scene::IActor
             */
            SmartPtr<scene::IGameActor> createDefaultCamera( bool addToScene = true ) override;

            /**
             * @brief Creates a default cubemap for environment mapping.
             *
             * @param addToScene Whether to automatically add the cubemap to the scene.
             * @return SmartPtr to the created cubemap actor, or null if creation failed.
             *
             * @see scene::IActor
             */
            SmartPtr<scene::IGameActor> createDefaultCubemap( bool addToScene = true ) override;

            /**
             * @brief Creates a default cube mesh.
             *
             * @param addToScene Whether to automatically add the cube to the scene.
             * @return SmartPtr to the created cube actor, or null if creation failed.
             *
             * @see scene::IActor
             */
            SmartPtr<scene::IGameActor> createDefaultCube( bool addToScene = true ) override;

            /**
             * @brief Creates a default cube mesh with mesh data.
             *
             * @param addToScene Whether to automatically add the cube mesh to the scene.
             * @return SmartPtr to the created cube mesh actor, or null if creation failed.
             *
             * @see scene::IActor
             */
            SmartPtr<scene::IGameActor> createDefaultCubeMesh( bool addToScene = true ) override;

            /**
             * @brief Creates a default ground plane.
             *
             * @param addToScene Whether to automatically add the ground to the scene.
             * @return SmartPtr to the created ground actor, or null if creation failed.
             *
             * @see scene::IActor
             */
            SmartPtr<scene::IGameActor> createDefaultGround( bool addToScene = true ) override;

            /**
             * @brief Creates a default terrain.
             *
             * @param addToScene Whether to automatically add the terrain to the scene.
             * @return SmartPtr to the created terrain actor, or null if creation failed.
             *
             * @see scene::IActor
             */
            SmartPtr<scene::IGameActor> createDefaultTerrain( bool addToScene = true ) override;

            /**
             * @brief Creates a default physics constraint.
             *
             * @return SmartPtr to the created constraint actor, or null if creation failed.
             *
             * @see scene::IActor
             */
            SmartPtr<scene::IGameActor> createDefaultConstraint() override;

            /**
             * @brief Creates a directional light for the scene.
             *
             * @param addToScene Whether to automatically add the light to the scene.
             * @return SmartPtr to the created directional light actor, or null if creation failed.
             *
             * @see scene::IActor
             */
            SmartPtr<scene::IGameActor> createDirectionalLight( bool addToScene = true ) override;

            /**
             * @brief Creates a point light for the scene.
             *
             * @param addToScene Whether to automatically add the light to the scene.
             * @return SmartPtr to the created point light actor, or null if creation failed.
             *
             * @see scene::IActor
             */
            SmartPtr<scene::IGameActor> createPointLight( bool addToScene = true ) override;

            /**
             * @brief Creates a default plane mesh.
             *
             * @param addToScene Whether to automatically add the plane to the scene.
             * @return SmartPtr to the created plane actor, or null if creation failed.
             *
             * @see scene::IActor
             */
            SmartPtr<scene::IGameActor> createDefaultPlane( bool addToScene = true ) override;

            /**
             * @brief Creates a default vehicle with basic physics.
             *
             * @param addToScene Whether to automatically add the vehicle to the scene.
             * @return SmartPtr to the created vehicle actor, or null if creation failed.
             *
             * @see scene::IActor
             */
            SmartPtr<scene::IGameActor> createDefaultVehicle( bool addToScene = true ) override;

            /**
             * @brief Creates a default car with vehicle physics.
             *
             * @param addToScene Whether to automatically add the car to the scene.
             * @return SmartPtr to the created car actor, or null if creation failed.
             *
             * @see scene::IActor
             */
            SmartPtr<scene::IGameActor> createDefaultCar( bool addToScene = true ) override;

            /**
             * @brief Creates a default truck with vehicle physics.
             *
             * @param addToScene Whether to automatically add the truck to the scene.
             * @return SmartPtr to the created truck actor, or null if creation failed.
             *
             * @see scene::IActor
             */
            SmartPtr<scene::IGameActor> createDefaultTruck( bool addToScene = true ) override;

            /**
             * @brief Creates a default particle system.
             *
             * @param addToScene Whether to automatically add the particle system to the scene.
             * @return SmartPtr to the created particle system actor, or null if creation failed.
             *
             * @see scene::IActor
             */
            SmartPtr<scene::IGameActor> createDefaultParticleSystem( bool addToScene = true ) override;

            /**
             * @brief Creates a default UI material.
             *
             * @return SmartPtr to the created UI material, or null if creation failed.
             *
             * @see render::IMaterial
             */
            SmartPtr<render::IMaterial> createDefaultMaterialUI() override;

            /**
             * @brief Creates a default material for rendering.
             *
             * @return SmartPtr to the created material, or null if creation failed.
             *
             * @see render::IMaterial
             */
            SmartPtr<render::IMaterial> createDefaultMaterial() override;

            /**
             * @brief Creates all default materials used by the application.
             *
             * This method creates a set of commonly used materials including
             * UI materials, basic rendering materials, and specialized materials
             * for different rendering passes.
             *
             * @see render::IMaterial
             */
            void createDefaultMaterials() override;

            /**
             * @brief Creates rigid static mesh physics for an actor.
             *
             * @param actor The actor to add rigid static physics to.
             * @param recursive Whether to apply physics recursively to child actors.
             *
             * @see scene::IActor
             */
            void createRigidStaticMesh( SmartPtr<scene::IGameActor> actor, bool recursive ) override;

            /**
             * @brief Creates rigid dynamic mesh physics for an actor.
             *
             * @param actor The actor to add rigid dynamic physics to.
             * @param recursive Whether to apply physics recursively to child actors.
             *
             * @see scene::IActor
             */
            void createRigidDynamicMesh( SmartPtr<scene::IGameActor> actor, bool recursive ) override;

            /**
             * @brief Imports a scene from a file.
             *
             * @param filePath The path to the scene file to import.
             * @return SmartPtr to the imported scene properties, or null if import failed.
             *
             * @see Properties
             */
            SmartPtr<Properties> importScene( const String &filePath ) override;

            /**
             * @brief Creates rigid static mesh physics for the current scene.
             *
             * Applies rigid static physics to all applicable actors in the scene.
             */
            void createRigidStaticMesh() override;

            /**
             * @brief Creates rigid dynamic mesh physics for the current scene.
             *
             * Applies rigid dynamic physics to all applicable actors in the scene.
             */
            void createRigidDynamicMesh() override;

            /**
             * @brief Gets whether frame statistics are being created.
             *
             * @return True if frame statistics are enabled, false otherwise.
             *
             * @see setCreateFrameStatistics()
             */
            bool getCreateFrameStatistics() const override;

            /**
             * @brief Sets whether frame statistics should be created.
             *
             * @param bCreateFrameStatistics True to enable frame statistics, false to disable.
             *
             * @see getCreateFrameStatistics()
             */
            void setCreateFrameStatistics( bool bCreateFrameStatistics ) override;

            /**
             * @brief Gets the plugins configuration file path.
             *
             * @return The path to the plugins configuration file.
             *
             * @see setPluginsConfigFilePath()
             */
            String getPluginsConfigFilePath() const override;

            /**
             * @brief Sets the plugins configuration file path.
             *
             * @param pluginsConfigFilePath The path to the plugins configuration file.
             *
             * @see getPluginsConfigFilePath()
             */
            void setPluginsConfigFilePath( const String &pluginsConfigFilePath ) override;

            /**
             * @brief Gets whether the application is in debug mode.
             *
             * @return True if debug mode is enabled, false otherwise.
             *
             * @see setDebugMode()
             */
            bool isDebugMode() const override;

            /**
             * @brief Sets whether the application should run in debug mode.
             *
             * @param debugMode True to enable debug mode, false to disable.
             *
             * @see isDebugMode()
             */
            void setDebugMode( bool debugMode ) override;

            /**
             * @brief Gets the render UI hint string.
             *
             * @return The render UI hint used for UI system selection.
             *
             * @see setRenderUIHint()
             */
            String getRenderUIHint() const;

            /**
             * @brief Sets the render UI hint string.
             *
             * @param renderUIHint The hint string for selecting the UI system.
             *
             * @see getRenderUIHint()
             */
            void setRenderUIHint( const String &renderUIHint );

            /**
             * @brief Gets the media path for the application.
             *
             * @return The path to the media directory.
             */
            String getMediaPath() const override;

            u8 getApplicationFlags() const;

            void setApplicationFlags( u8 applicationFlags );

            SmartPtr<render::IViewport> getViewport() const;

            void setViewport( SmartPtr<render::IViewport> viewport );

            SmartPtr<render::IGraphicsCamera> getCamera() const;

            void setCamera( SmartPtr<render::IGraphicsCamera> camera );

            /**
             * @brief Locks the application mutex.
             *
             * Acquires the recursive mutex to ensure thread-safe access to application data.
             *
             * @see try_lock()
             * @see unlock()
             */
            void lock() override;

            /**
             * @brief Attempts to lock the application mutex.
             *
             * @return True if the lock was acquired, false if the mutex was already locked.
             *
             * @see lock()
             * @see unlock()
             */
            bool try_lock() override;

            /**
             * @brief Unlocks the application mutex.
             *
             * Releases the recursive mutex to allow other threads to access application data.
             *
             * @see lock()
             * @see try_lock()
             */
            void unlock() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Creates the AI manager for the application.
             *
             * Initializes the artificial intelligence system that handles
             * NPC behavior, pathfinding, and decision making.
             */
            virtual void createAiManager();

            /**
             * @brief Creates the log manager for the application.
             *
             * Initializes the logging system that handles debug output,
             * error reporting, and performance monitoring.
             */
            virtual void createLogManager();

            /**
             * @brief Creates the factory manager for the application.
             *
             * Initializes the factory system that handles object creation
             * and resource management.
             */
            virtual void createFactoryManager();

            /**
             * @brief Allocates pool memory for the application.
             *
             * Sets up memory pools for efficient allocation and deallocation
             * of frequently used objects.
             */
            virtual void allocatePoolMemory();

            /**
             * @brief Creates the state manager for the application.
             *
             * Initializes the state management system that handles
             * application state transitions and persistence.
             */
            virtual void createStateManager();

            /**
             * @brief Creates the timer system for the application.
             *
             * Initializes the timing system that provides high-resolution
             * timing for game loops and performance measurement.
             */
            virtual void createTimer();

            /**
             * @brief Creates the prefab manager for the application.
             *
             * Initializes the prefab system that handles reusable object
             * templates and instantiation.
             */
            virtual void createPrefabManager();

            /**
             * @brief Creates the profiler for the application.
             *
             * Initializes the profiling system that monitors performance
             * and provides detailed timing information.
             */
            virtual void createProfiler();

            /**
             * @brief Creates the thread pool for the application.
             *
             * Initializes the threading system that manages concurrent
             * execution of tasks and jobs.
             */
            virtual void createThreadPool();

            /**
             * @brief Creates the task manager for the application.
             *
             * Initializes the task management system that handles
             * asynchronous task execution and scheduling.
             */
            virtual void createTaskManager();

            /**
             * @brief Creates the default tasks for the application.
             *
             * Sets up common tasks that run during the application lifecycle,
             * such as update tasks, render tasks, and cleanup tasks.
             */
            virtual void createTasks();

            /**
             * @brief Creates the job queue for the application.
             *
             * Initializes the job queue system that manages background
             * processing and parallel execution.
             */
            virtual void createJobQueue();

            /**
             * @brief Creates the finite state machine manager.
             *
             * Initializes the FSM system that manages application state
             * transitions and state-specific behavior.
             */
            virtual void createFsmManager();

            /**
             * @brief Creates the default finite state machine.
             *
             * Sets up the FSM with default states and transitions.
             * This method is designed to be overridden to create custom
             * state machines for specific applications.
             */
            virtual void createFsm();

            /**
             * @brief Creates the components container.
             *
             * Initializes the component system that manages object
             * composition and behavior.
             */
            virtual void createComponentsContainer();

            /**
             * @brief Creates the platform manager.
             *
             * Initializes the platform abstraction layer that handles
             * platform-specific functionality and APIs.
             */
            virtual void createPlatformManager();

            /**
             * @brief Creates the file system.
             *
             * Initializes the file system abstraction that handles
             * file I/O operations and resource loading.
             */
            virtual void createFileSystem();

            /**
             * @brief Creates the user interface system.
             *
             * Initializes the UI framework and creates the main
             * user interface for the application.
             */
            virtual void createUI();

            /**
             * @brief Creates the render UI system.
             *
             * Initializes the rendering UI system that handles
             * UI rendering and display.
             */
            virtual void createRenderUI();

            /**
             * @brief Creates the scene manager.
             *
             * Initializes the scene management system that handles
             * scene organization, hierarchy, and spatial queries.
             */
            virtual void createSceneManager();

            /**
             * @brief Creates the main scene.
             *
             * Sets up the primary scene that contains all game objects,
             * actors, and visual elements.
             */
            virtual void createScene();

            /**
             * @brief Creates the graphics system.
             *
             * Initializes the rendering engine and graphics pipeline.
             *
             * @return True if graphics system creation was successful, false otherwise.
             */
            virtual bool createGraphicsSystem();

            /**
             * @brief Creates the default font.
             *
             * Loads and initializes the default font used for text rendering
             * throughout the application.
             */
            virtual void createDefaultFont();

            /**
             * @brief Creates the render window.
             *
             * Creates the main application window and sets up the
             * rendering surface.
             */
            virtual void createRenderWindow();

            /**
             * @brief Creates the graphics scene manager.
             *
             * Initializes the graphics scene system that manages
             * rendering objects and visual representation.
             */
            virtual void createGraphicsScene();

            /**
             * @brief Loads graphics resources.
             *
             * Loads textures, shaders, and other graphics resources
             * required for rendering.
             */
            virtual void loadGraphicsResources();

            /**
             * @brief Sets up the render pipeline.
             *
             * Configures the rendering pipeline with appropriate
             * passes, effects, and post-processing.
             */
            virtual void setupRenderpipeline();

            /**
             * @brief Creates the mesh loader.
             *
             * Initializes the mesh loading system that handles
             * 3D model loading and processing.
             */
            virtual void createMeshLoader();

            /**
             * @brief Creates the physics system.
             *
             * Initializes the physics engine that handles collision
             * detection, rigid body simulation, and constraints.
             */
            virtual void createPhysics();

            /**
             * @brief Creates the input system.
             *
             * Initializes the input handling system that processes
             * keyboard, mouse, and gamepad input.
             */
            virtual void createInputSystem();

            /**
             * @brief Creates the graphics system camera.
             *
             * Sets up the main camera for the graphics system
             * and configures its properties.
             */
            virtual void createCamera();

            /**
             * @brief Creates the viewports.
             *
             * Sets up rendering viewports that define how the
             * scene is displayed on screen.
             */
            virtual void createViewports();

            /**
             * @brief Creates the script manager.
             *
             * Initializes the scripting system that handles
             * Lua, Python, or other script execution.
             *
             * @return True if script manager creation was successful, false otherwise.
             */
            virtual bool createScriptManager();

            /**
             * @brief Creates the sound manager.
             *
             * Initializes the audio system that handles sound
             * playback, music, and audio effects.
             *
             * @return True if sound manager creation was successful, false otherwise.
             */
            virtual bool createSoundManager();

            /**
             * @brief Creates the plugin manager.
             *
             * Initializes the plugin system that handles
             * dynamic loading and management of plugins.
             */
            virtual void createPluginManager();

            /**
             * @brief Creates the plugins.
             *
             * Loads and initializes all registered plugins
             * for the application.
             */
            virtual void createPlugins();

            /**
             * @brief Creates the process manager.
             *
             * Initializes the process management system that handles
             * background processes and system integration.
             */
            virtual void createProcessManager();

            /**
             * @brief Destroys the scene and cleans up scene resources.
             *
             * This method is optional to override and provides a hook
             * for custom scene cleanup logic.
             */
            virtual void destroyScene();

            /**
             * @brief Loads scripts for the application.
             *
             * Loads and initializes all scripts that define
             * game behavior and logic.
             */
            void loadScripts();

            /**
             * @brief Handles events for the finite state machine.
             *
             * @param state The current state of the FSM.
             * @param eventType The type of event that occurred.
             * @return The return type indicating the result of event processing.
             *
             * @see FSMReturnType
             * @see FSMEvent
             */
            virtual FSMReturnType handleEvent( u32 state, FSMEvent eventType );

            /** @brief The application manager that coordinates all subsystems. */
            SmartPtr<IApplicationManager> m_applicationManager;

            /** @brief The finite state machine managing application states. */
            SmartPtr<IFSM> m_fsm;

            /** @brief The main application window. */
            SmartPtr<render::IGraphicsWindow> m_window;

            /** @brief The graphics scene manager. */
            SmartPtr<render::IGraphicsScene> m_sceneMgr;

            /** @brief The main application camera. */
            SmartPtr<render::IGraphicsCamera> m_camera;

            /** @brief The scene node containing the camera. */
            SmartPtr<render::IGraphicsSceneNode> m_cameraSceneNode;

            /** @brief The main viewport for rendering. */
            SmartPtr<render::IViewport> m_viewport;

            /** @brief Frame statistics for performance monitoring. */
            SmartPtr<IFrameStatistics> m_frameStatistics;

            /** @brief The application event listener. */
            SmartPtr<ApplicationEventListener> m_applicationEventListener;

            /** @brief Number of active threads in the thread pool (default: 4). */
            atomic_u32 m_activeThreads = 4;

            u8 m_applicationFlags = 0;

            /** @brief Path to the plugins configuration file (default: "wp_plugins.cfg"). */
            String m_pluginsConfigFilePath;

            /** @brief Render UI hint for system selection (default: "RenderUI"). */
            String m_renderUIHint;

            /** @brief Recursive mutex for thread-safe access to application data. */
            mutable RecursiveSpinMutex m_applicationMutex;
        };
    }  // namespace core
}  // namespace workphone

#endif  // __WP_Application_H_
