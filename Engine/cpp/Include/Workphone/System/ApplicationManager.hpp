#ifndef __ApplicationManager_h__
#define __ApplicationManager_h__

#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/IApplication.hpp>
#include <Workphone/Interface/System/IEvent.hpp>
#include <Workphone/Core/FixedArray.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Core/Parameter.hpp>
#include <Workphone/Core/StringPool.hpp>
#include <memory>

namespace workphone
{
    namespace core
    {

        /**
         * @class ApplicationManager
         * @brief Concrete implementation of the IApplicationManager interface.
         *
         * The ApplicationManager class serves as the central coordinator for the entire application,
         * managing the lifecycle, systems, and resources. It implements the facade pattern to provide
         * a simplified interface to complex subsystems and acts as the primary entry point for all
         * application-level operations.
         *
         * Key responsibilities include:
         * - Application state management (running, paused, playing, editor modes)
         * - System manager coordination (graphics, physics, input, audio, etc.)
         * - Resource management and loading progress tracking
         * - Plugin system management
         * - Event handling and propagation
         * - Thread-safe access to subsystems
         * - Configuration and settings management
         *
         * The class uses atomic operations and smart pointers to ensure thread safety across
         * all managed subsystems. It follows the RAII principle for proper resource management
         * and cleanup.
         *
         * @note This class implements the Singleton pattern through its base interface.
         * Access the instance through IApplicationManager::instance().
         *
         * @see IApplicationManager
         * @author Zane Desir
         * @version 1.0
         * @since Engine v1.0
         */
        class WPCore_API ApplicationManager : public IApplicationManager
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes the ApplicationManager with default values for all state variables.
             * All manager pointers are initialized to nullptr and will be set during the
             * application initialization process.
             *
             * @note The constructor does not perform any heavy initialization.
             * Use the load() method for proper initialization.
             */
            ApplicationManager();

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup of all managed resources and subsystems.
             * Automatically calls unload() if the manager hasn't been properly shut down.
             *
             * @note All subsystem managers are automatically released through smart pointers.
             */
            ~ApplicationManager() override;

            /**
             * @brief Loads and initializes the application manager.
             *
             * This method performs the initial setup of the application manager,
             * including initialization of core systems and loading of configuration data.
             *
             * @param data Optional shared object containing initialization data
             *
             * @note This method should be called once during application startup
             * before accessing any subsystems.
             *
             * @see unload()
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads and cleans up the application manager.
             *
             * This method performs cleanup of all managed resources and shuts down
             * all subsystems in the reverse order of their initialization.
             *
             * @param data Optional shared object containing cleanup data
             *
             * @note This method should be called once during application shutdown.
             * After calling this method, the application manager should not be used.
             *
             * @see load()
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IApplicationManager::isEditor */
            bool isEditor() const override;

            /** @copydoc IApplicationManager::setEditor */
            void setEditor( bool editor ) override;

            /** @copydoc IApplicationManager::isEditorCamera */
            bool isEditorCamera() const override;

            /** @copydoc IApplicationManager::setEditorCamera */
            void setEditorCamera( bool editor ) override;

            /** @copydoc IApplicationManager::isPlaying */
            bool isPlaying() const override;

            /** @copydoc IApplicationManager::setPlaying */
            void setPlaying( bool playing ) override;

            /** @copydoc IApplicationManager::isPaused */
            bool isPaused() const override;

            /** @copydoc IApplicationManager::setPaused */
            void setPaused( bool paused ) override;

            /** @copydoc IApplicationManager::isRunning */
            bool isRunning() const override;

            /** @copydoc IApplicationManager::setRunning */
            void setRunning( bool running ) override;

            /** @copydoc IApplicationManager::getQuit */
            bool getQuit() const override;

            /** @copydoc IApplicationManager::setQuit */
            void setQuit( bool quit ) override;

            /** @copydoc IApplicationManager::hasTasks */
            bool hasTasks() const override;

            /** @copydoc IApplicationManager::isPauseMenuActive */
            bool isPauseMenuActive() const override;

            /** @copydoc IApplicationManager::setPauseMenuActive */
            void setPauseMenuActive( bool pauseMenuActive ) override;

            /** @copydoc IApplicationManager::isSceneLoading */
            bool isSceneLoading() const override;

            /** @copydoc IApplicationManager::setSceneLoading */
            void setSceneLoading( bool loading ) override;

            /** @copydoc IApplicationManager::getCachePath */
            String getCachePath() const override;

            /** @copydoc IApplicationManager::setCachePath */
            void setCachePath( const String &cachePath ) override;

            /** @copydoc IApplicationManager::getProjectPath */
            String getProjectPath() const override;

            /** @copydoc IApplicationManager::setProjectPath */
            void setProjectPath( const String &projectPath ) override;

            /** @copydoc IApplicationManager::getProjectLibraryName */
            String getProjectLibraryName() const override;

            /** @copydoc IApplicationManager::setProjectLibraryName */
            void setProjectLibraryName( const String &projectLibraryName ) override;

            /** @copydoc IApplicationManager::getMediaPath */
            String getMediaPath() const override;

            /** @copydoc IApplicationManager::setMediaPath */
            void setMediaPath( const String &mediaPath ) override;

            /** @copydoc IApplicationManager::getRenderMediaPath */
            String getRenderMediaPath() const override;

            /** @copydoc IApplicationManager::setRenderMediaPath */
            void setRenderMediaPath( const String &renderMediaPath ) override;

            /** @copydoc IApplicationManager::getSettingsPath */
            String getSettingsPath() const override;

            /** @copydoc IApplicationManager::setSettingsPath */
            void setSettingsPath( const String &cachePath ) override;

            /** @copydoc IApplicationManager::getBuildConfig */
            String getBuildConfig() const override;

            /** @copydoc IApplicationManager::getProjectLibraryExtension */
            String getProjectLibraryExtension() const override;

            /** @copydoc IApplicationManager::getProjectLibraryPath */
            String getProjectLibraryPath() const override;

            /** @copydoc IApplicationManager::getStateTask */
            TaskId getStateTask() const override;

            /** @copydoc IApplicationManager::getApplicationTask */
            TaskId getApplicationTask() const override;

            /** @copydoc IApplicationManager::getEnableRenderer */
            bool getEnableRenderer() const override;

            /** @copydoc IApplicationManager::setEnableRenderer */
            void setEnableRenderer( bool enable ) override;

            /** @copydoc IApplicationManager::getFSMPtr */
            IFSM *getFSMPtr() const override;

            /** @copydoc IApplicationManager::getFSM */
            SmartPtr<IFSM> getFSM() const override;

            /** @copydoc IApplicationManager::setFSM */
            void setFSM( SmartPtr<IFSM> fsm ) override;

            /** @copydoc IApplicationManager::getEditorSettings */
            SmartPtr<Properties> getEditorSettings() const override;

            /** @copydoc IApplicationManager::setEditorSettings */
            void setEditorSettings( SmartPtr<Properties> properties ) override;

            /** @copydoc IApplicationManager::getPlayerSettings */
            SmartPtr<Properties> getPlayerSettings() const override;

            /** @copydoc IApplicationManager::setPlayerSettings */
            void setPlayerSettings( SmartPtr<Properties> properties ) override;

            /** @copydoc IApplicationManager::getAiManager */
            SmartPtr<IAiManager> getAiManager() const override;

            /** @copydoc IApplicationManager::setAiManager */
            void setAiManager( SmartPtr<IAiManager> aiManager ) override;

            /** @copydoc IApplicationManager::getLogManager */
            SmartPtr<ILogManager> getLogManager() const override;

            /** @copydoc IApplicationManager::setLogManager */
            void setLogManager( SmartPtr<ILogManager> logManager ) override;

            IFactoryManager *getFactoryManagerPtr() const override;

            /** @copydoc IApplicationManager::getFactoryManager */
            SmartPtr<IFactoryManager> getFactoryManager() const override;

            /** @copydoc IApplicationManager::setFactoryManager */
            void setFactoryManager( SmartPtr<IFactoryManager> factoryManager ) override;

            /** @copydoc IApplicationManager::getProcessManager */
            SmartPtr<IProcessManager> getProcessManager() const override;

            /** @copydoc IApplicationManager::setProcessManager */
            void setProcessManager( SmartPtr<IProcessManager> processManager ) override;

            /** @copydoc IApplicationManager::getApplication */
            SmartPtr<IApplication> getApplication() const override;

            /** @copydoc IApplicationManager::setApplication */
            void setApplication( SmartPtr<IApplication> application ) override;

            /**
             * @brief Gets the editor manager instance as a raw pointer.
             *
             * Provides direct access to the editor manager for performance-critical operations.
             *
             * @return Raw pointer to the editor manager, or nullptr if not set
             *
             * @warning The returned pointer is not reference-counted. Ensure the
             * ApplicationManager remains valid while using this pointer.
             *
             * @see getEditorManager()
             */
            IEditorManager *getEditorManagerPtr() const override;

            /**
             * @brief Gets the editor manager instance.
             *
             * The editor manager handles all editor-specific functionality including
             * scene editing, property panels, and editor-only rendering.
             *
             * @return Shared pointer to the editor manager, or nullptr if not set
             *
             * @see setEditorManager()
             */
            SmartPtr<IEditorManager> getEditorManager() const override;

            /**
             * @brief Sets the editor manager instance.
             *
             * @param editorManager The new editor manager to set
             *
             * @note This method is thread-safe using atomic operations.
             *
             * @see getEditorManager()
             */
            void setEditorManager( SmartPtr<IEditorManager> editorManager ) override;

            /** @copydoc IApplicationManager::getFileSystemPtr */
            IFileSystem *getFileSystemPtr() const override;

            /** @copydoc IApplicationManager::getFileSystem */
            SmartPtr<IFileSystem> getFileSystem() const override;

            /** @copydoc IApplicationManager::setFileSystem */
            void setFileSystem( SmartPtr<IFileSystem> fileSystem ) override;

            ITimer *getTimerPtr() const override;

            /** @copydoc IApplicationManager::getTimer */
            SmartPtr<ITimer> getTimer() override;

            /** @copydoc IApplicationManager::setTimer */
            void setTimer( SmartPtr<ITimer> timer ) override;

            IFSMManager *getFsmManagerPtr() const override;

            /** @copydoc IApplicationManager::getFsmManager */
            SmartPtr<IFSMManager> getFsmManager() const override;

            /** @copydoc IApplicationManager::setFsmManager */
            void setFsmManager( SmartPtr<IFSMManager> fsmManager ) override;

            /**
             * @brief Gets the FSM manager instance for a specific task.
             *
             * The application can maintain multiple FSM managers for different tasks
             * to allow parallel state management across different subsystems.
             *
             * @param task The task identifier to get the FSM manager for
             * @return Shared pointer to the FSM manager for the specified task
             *
             * @see setFsmManagerByTask()
             */
            SmartPtr<IFSMManager> getFsmManagerByTask( TaskId task ) const override;

            /**
             * @brief Sets the FSM manager instance for a specific task.
             *
             * @param task The task identifier to set the FSM manager for
             * @param fsmManager The FSM manager to associate with the task
             *
             * @note This method is thread-safe and uses atomic operations.
             *
             * @see getFsmManagerByTask()
             */
            void setFsmManagerByTask( TaskId task, SmartPtr<IFSMManager> fsmManager ) override;

            /** @copydoc IApplicationManager::getProceduralManager */
            SmartPtr<procedural::IProceduralManager> getProceduralManager() const override;

            /** @copydoc IApplicationManager::setProceduralManager */
            void setProceduralManager(
                SmartPtr<procedural::IProceduralManager> proceduralManager ) override;

            /** @copydoc IApplicationManager::getPhysicsManager2D */
            SmartPtr<physics::IPhysicsManager2D> getPhysicsManager2D() const override;

            /** @copydoc IApplicationManager::setPhysicsManager2D */
            void setPhysicsManager2D( SmartPtr<physics::IPhysicsManager2D> physicsManager ) override;

            /** @copydoc IApplicationManager::getPhysicsManager */
            SmartPtr<physics::IPhysicsManager> getPhysicsManager() const override;

            /** @copydoc IApplicationManager::getPhysicsManagerPtr */
            physics::IPhysicsManager *getPhysicsManagerPtr() const override;

            /** @copydoc IApplicationManager::setPhysicsManager */
            void setPhysicsManager( SmartPtr<physics::IPhysicsManager> physicsManager ) override;

            render::IGraphicsSystem *getGraphicsSystemPtr() const override;

            SmartPtr<render::IGraphicsSystem> getGraphicsSystem() const override;

            /**
             * @brief Sets the graphics system instance.
             *
             * The graphics system manages all rendering operations including
             * scene rendering, UI rendering, and graphics resource management.
             *
             * @param graphicsSystem The new graphics system to set
             *
             * @note This method is thread-safe using atomic operations.
             *
             * @see getGraphicsSystem(), getGraphicsSystemPtr()
             */
            void setGraphicsSystem( SmartPtr<render::IGraphicsSystem> graphicsSystem ) override;

            /** @copydoc IApplicationManager::getVideoManager */
            SmartPtr<render::IVideoManager> getVideoManager() const override;

            /** @copydoc IApplicationManager::setVideoManager */
            void setVideoManager( SmartPtr<render::IVideoManager> videoManager ) override;

            ITaskManager *getTaskManagerPtr() const override;

            /** @copydoc IApplicationManager::getTaskManager */
            SmartPtr<ITaskManager> getTaskManager() const override;

            /** @copydoc IApplicationManager::setTaskManager */
            void setTaskManager( SmartPtr<ITaskManager> taskManager ) override;

            /**
             * @brief Gets the profiler instance as a raw pointer.
             *
             * Provides direct access to the profiler for performance-critical profiling operations.
             *
             * @return Raw pointer to the profiler, or nullptr if not set
             *
             * @warning The returned pointer is not reference-counted.
             *
             * @see getProfiler()
             */
            IProfiler *getProfilerPtr() const override;

            /** @copydoc IApplicationManager::getProfiler */
            SmartPtr<IProfiler> getProfiler() const override;

            /** @copydoc IApplicationManager::setProfiler */
            void setProfiler( SmartPtr<IProfiler> profiler ) override;

            IJobQueue *getJobQueuePtr() const override;

            /** @copydoc IApplicationManager::getJobQueue */
            SmartPtr<IJobQueue> getJobQueue() const override;

            /** @copydoc IApplicationManager::setJobQueue */
            void setJobQueue( SmartPtr<IJobQueue> jobQueue ) override;

            /** @copydoc IApplicationManager::getParticleManager */
            SmartPtr<render::IParticleManager> getParticleManager() const override;

            /** @copydoc IApplicationManager::setParticleManager */
            void setParticleManager( SmartPtr<render::IParticleManager> particleManager ) override;

            IScriptManager *getScriptManagerPtr() const override;

            /** @copydoc IApplicationManager::getScriptManager */
            SmartPtr<IScriptManager> getScriptManager() const override;

            /** @copydoc IApplicationManager::setScriptManager */
            void setScriptManager( SmartPtr<IScriptManager> physicsManager ) override;

            /** @copydoc IApplicationManager::getInput */
            SmartPtr<IInputManager> getInput() const override;

            /** @copydoc IApplicationManager::setInput */
            void setInput( SmartPtr<IInputManager> input ) override;

            /** @copydoc IApplicationManager::getInputDeviceManager */
            SmartPtr<IInputDeviceManager> getInputDeviceManager() const override;

            /** @copydoc IApplicationManager::setInputDeviceManager */
            void setInputDeviceManager( SmartPtr<IInputDeviceManager> inputManager ) override;

            /** @copydoc IApplicationManager::getThreadPoolPtr */
            IThreadPool *getThreadPoolPtr() const override;

            /** @copydoc IApplicationManager::getThreadPool */
            SmartPtr<IThreadPool> getThreadPool() const override;

            /** @copydoc IApplicationManager::setThreadPool */
            void setThreadPool( SmartPtr<IThreadPool> threadPool ) override;

            /** @copydoc IApplicationManager::getConsole */
            SmartPtr<IConsole> getConsole() const override;

            /** @copydoc IApplicationManager::setConsole */
            void setConsole( SmartPtr<IConsole> console ) override;

            /** @copydoc IApplicationManager::getCameraManager */
            SmartPtr<scene::ICameraManager> getCameraManager() const override;

            /** @copydoc IApplicationManager::setCameraManager */
            void setCameraManager( SmartPtr<scene::ICameraManager> cameraManager ) override;

            IStateManager *getStateManagerPtr() const override;

            /** @copydoc IApplicationManager::getStateManager */
            SmartPtr<IStateManager> getStateManager() const override;

            /** @copydoc IApplicationManager::setStateManager */
            void setStateManager( SmartPtr<IStateManager> stateManager ) override;

            /**
             * @brief Gets the command manager instance as a raw pointer.
             *
             * Provides direct access to the command manager for performance-critical operations.
             *
             * @return Raw pointer to the command manager, or nullptr if not set
             *
             * @warning The returned pointer is not reference-counted.
             *
             * @see getCommandManager()
             */
            ICommandManager *getCommandManagerPtr() const override;

            /** @copydoc IApplicationManager::getCommandManager */
            SmartPtr<ICommandManager> getCommandManager() const override;

            /** @copydoc IApplicationManager::setCommandManager */
            void setCommandManager( SmartPtr<ICommandManager> commandManager ) override;

            /** @copydoc IApplicationManager::getResourceManager */
            SmartPtr<IResourceManager> getResourceManager() const override;

            /** @copydoc IApplicationManager::setResourceManager */
            void setResourceManager( SmartPtr<IResourceManager> resourceManager ) override;

            /** @copydoc IApplicationManager::getPrefabManager */
            SmartPtr<scene::IGamePrefabManager> getPrefabManager() const override;

            /** @copydoc IApplicationManager::setPrefabManager */
            void setPrefabManager( SmartPtr<scene::IGamePrefabManager> prefabManager ) override;

            /** @copydoc IApplicationManager::getMeshLoader */
            SmartPtr<IMeshLoader> getMeshLoader() const override;

            /** @copydoc IApplicationManager::setMeshLoader */
            void setMeshLoader( SmartPtr<IMeshLoader> meshLoader ) override;

            IResourceDatabase *getResourceDatabasePtr() const override;

            /** @copydoc IApplicationManager::getResourceDatabase */
            SmartPtr<IResourceDatabase> getResourceDatabase() const override;

            /** @copydoc IApplicationManager::setResourceDatabase */
            void setResourceDatabase( SmartPtr<IResourceDatabase> resourceDatabase ) override;

            scene::IGameManager *getGameManagerPtr() const override;

            /** @copydoc IApplicationManager::getSceneManager */
            SmartPtr<scene::IGameManager> getGameManager() const override;

            /** @copydoc IApplicationManager::setSceneManager */
            void setGameManager( SmartPtr<scene::IGameManager> gameManager ) override;

            /**
             * @brief Gets the selection manager instance as a raw pointer.
             * Provides direct access to the selection manager for performance-critical operations.
             *
             * @return Raw pointer to the selection manager, or nullptr if not set
             *
             * @warning The returned pointer is not reference-counted.
             *
             * @see getSelectionManager()
             */
            ISelectionManager *getSelectionManagerPtr() const override;

            /** @copydoc IApplicationManager::getSelectionManager */
            SmartPtr<ISelectionManager> getSelectionManager() const override;

            /** @copydoc IApplicationManager::setSelectionManager */
            void setSelectionManager( SmartPtr<ISelectionManager> selectionManager ) override;

            /** @copydoc IApplicationManager::getSoundManagerPtr */
            ISoundManager *getSoundManagerPtr() const override;

            /** @copydoc IApplicationManager::getSoundManager */
            SmartPtr<ISoundManager> getSoundManager() const override;

            /** @copydoc IApplicationManager::setSoundManager */
            void setSoundManager( SmartPtr<ISoundManager> soundManager ) override;

            ui::IUIManager *getUIPtr() const override;

            /** @copydoc IApplicationManager::getUI */
            SmartPtr<ui::IUIManager> getUI() const override;

            /** @copydoc IApplicationManager::setUI */
            void setUI( SmartPtr<ui::IUIManager> ui ) override;

            ui::IUIManager *getRenderUIPtr() const override;

            /** @copydoc IApplicationManager::getRenderUI */
            SmartPtr<ui::IUIManager> getRenderUI() const override;

            /** @copydoc IApplicationManager::setRenderUI */
            void setRenderUI( SmartPtr<ui::IUIManager> renderUI ) override;

            /** @copydoc IApplicationManager::getDatabase */
            SmartPtr<IDatabaseManager> getDatabase() const override;

            /** @copydoc IApplicationManager::setDatabase */
            void setDatabase( SmartPtr<IDatabaseManager> database ) override;

            /**
             * @brief Gets the application properties.
             *
             * Properties contain configuration settings and runtime parameters
             * that control various aspects of the application behavior.
             *
             * @return Shared pointer to the properties object
             *
             * @see setProperties()
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Sets the application properties.
             *
             * @param properties The new properties object to set
             *
             * @note This method is thread-safe using atomic operations.
             *
             * @see getProperties()
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc IApplicationManager::getVehicleManager */
            SmartPtr<vehicle::IVehicleManager> getVehicleManager() const override;

            /** @copydoc IApplicationManager::setVehicleManager */
            void setVehicleManager( SmartPtr<vehicle::IVehicleManager> vehicleManager ) override;

            /** @copydoc IApplicationManager::getMeshManager */
            SmartPtr<IResourceManager> getMeshManager() const override;

            /** @copydoc IApplicationManager::setMeshManager */
            void setMeshManager( SmartPtr<IResourceManager> meshManager ) override;

            /** @copydoc IApplicationManager::getLoadProgress */
            s32 getLoadProgress() const override;

            /** @copydoc IApplicationManager::setLoadProgress */
            void setLoadProgress( s32 loadProgress ) override;

            /** @copydoc IApplicationManager::addLoadProgress */
            void addLoadProgress( s32 loadProgress ) override;

            /** @copydoc IApplicationManager::getActors */
            Array<SmartPtr<scene::IGameActor>> getActors() const override;

            /** @copydoc IApplicationManager::getPluginManager */
            SmartPtr<IPluginManager> getPluginManager() const override;

            /** @copydoc IApplicationManager::setPluginManager */
            void setPluginManager( SmartPtr<IPluginManager> pluginManager ) override;

            /** @copydoc IApplicationManager::addPlugin */
            void addPlugin( SmartPtr<ISharedObject> plugin ) override;

            /** @copydoc IApplicationManager::removePlugin */
            void removePlugin( SmartPtr<ISharedObject> plugin ) override;

            /**
             * @brief Gets the main window instance as a raw pointer.
             *
             * Provides direct access to the main application window for performance-critical operations.
             *
             * @return Raw pointer to the main window, or nullptr if not set
             *
             * @warning The returned pointer is not reference-counted.
             *
             * @see getWindow()
             */
            render::IGraphicsWindow *getWindowPtr() const override;

            /** @copydoc IApplicationManager::getWindow */
            SmartPtr<render::IGraphicsWindow> getWindow() const override;

            /** @copydoc IApplicationManager::setWindow */
            void setWindow( SmartPtr<render::IGraphicsWindow> window ) override;

            /** @copydoc IApplicationManager::getSceneRenderWindowPtr */
            ui::IUIWindow *getSceneRenderWindowPtr() const override;

            /** @copydoc IApplicationManager::getSceneRenderWindow */
            SmartPtr<ui::IUIWindow> getSceneRenderWindow() const override;

            /** @copydoc IApplicationManager::setSceneRenderWindow */
            void setSceneRenderWindow( SmartPtr<ui::IUIWindow> sceneRenderWindow ) override;

            /** @copydoc IApplicationManager::getComponentByType */
            SmartPtr<scene::IComponent> getComponentByType( u32 typeId ) const override;

            /**
             * @brief Gets a component by its type ID as a raw pointer.
             *
             * Provides direct access to components for performance-critical operations.
             *
             * @param typeId The type ID of the component to retrieve
             * @return Raw pointer to the component, or nullptr if not found
             *
             * @warning The returned pointer is not reference-counted.
             *
             * @see getComponentByType()
             */
            scene::IComponent *getComponentPtrByType( u32 typeId ) const override;

            /** @copydoc IApplicationManager::triggerEvent */
            Parameter triggerEvent( EventType eventType, hash_type eventValue,
                                    const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                    SmartPtr<ISharedObject> object, SmartPtr<IEvent> event,
                                    bool sendNow = false,
                                    u32 taskFlags = std::numeric_limits<u32>::max() ) override;

            /** @copydoc IApplicationManager::clearAllEvents */
            void clearAllEvents() override;

            /** @copydoc IApplicationManager::getNetworkManager */
            SmartPtr<INetworkManager> getNetworkManager() const override;

            /** @copydoc IApplicationManager::setNetworkManager */
            void setNetworkManager( SmartPtr<INetworkManager> networkManager ) override;

            /** @copydoc IApplicationManager::getPackageManager */
            SmartPtr<IPackageManager> getPackageManager() const override;

            /** @copydoc IApplicationManager::setPackageManager */
            void setPackageManager( SmartPtr<IPackageManager> packageManager ) override;

            /**
             * @brief Gets the global string pool.
             */
            StringPool<c8> *getStringPool() const override;

            /**
             * @brief Sets the global string pool.
             */
            void setStringPool( StringPool<c8> *pool ) override;

            /**
             * @brief Gets the global string pool.
             */
            StringPool<wchar_t> *getStringPoolW() const override;

            /**
             * @brief Sets the global string pool.
             */
            void setStringPoolW( StringPool<wchar_t> *pool ) override;
            /**
             * @brief Gets the global string pool.
             */
            StringPool<c8> *getPropertyNamePool() const override;

            /**
             * @brief Sets the global string pool.
             */
            void setPropertyNamePool( StringPool<c8> *pool ) override;

            /**
             * @brief Gets the global string pool.
             */
            StringPool<c8> *getPropertyValuePool() const override;

            /**
             * @brief Sets the global string pool.
             */
            void setPropertyValuePool( StringPool<c8> *pool ) override;

            /** @copydoc IApplicationManager::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /** @copydoc IApplicationManager::lock */
            void lock() override;

            /** @copydoc IApplicationManager::try_lock */
            bool try_lock() override;

            /** @copydoc IApplicationManager::unlock */
            void unlock() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Gets a component from a specific actor by type ID.
             *
             * Helper method for retrieving components from actors in the scene.
             *
             * @param actor The actor to search for the component
             * @param typeId The type ID of the component to find
             * @return Shared pointer to the component, or nullptr if not found
             *
             * @see getActorComponentPtr()
             */
            SmartPtr<scene::IComponent> getActorComponent( SmartPtr<scene::IGameActor> actor,
                                                           u32 typeId ) const;

            /**
             * @brief Gets a component from a specific actor by type ID as a raw pointer.
             *
             * Helper method for retrieving components from actors for performance-critical operations.
             *
             * @param actor The actor to search for the component
             * @param typeId The type ID of the component to find
             * @return Raw pointer to the component, or nullptr if not found
             *
             * @warning The returned pointer is not reference-counted.
             *
             * @see getActorComponent()
             */
            scene::IComponent *getActorComponentPtr( scene::IGameActor *actor, u32 typeId ) const;

            AtomicSmartPtr<IStateManager> m_stateManager;              ///< The state manager instance.
            AtomicSmartPtr<render::IGraphicsSystem> m_graphicsSystem;  ///< The graphics system instance.
            AtomicSmartPtr<ITaskManager> m_taskManager;                ///< The task manager instance.
            AtomicSmartPtr<IScriptManager> m_scriptManager;            ///< The script manager instance.

            AtomicSmartPtr<IFSMManager> m_fsmManager;           ///< Default finite state machine manager
            AtomicSmartPtr<scene::IGameManager> m_gameManager;  ///< Game scene and object management
            AtomicSmartPtr<IJobQueue> m_jobQueue;               ///< The job queue instance.
            AtomicSmartPtr<ITimer> m_timer;                     ///< The timer instance.

            AtomicSmartPtr<ui::IUIManager> m_ui;        ///< User interface management
            AtomicSmartPtr<ui::IUIManager> m_renderUI;  ///< Render-specific UI management

            ///< Database for resource metadata and indexing
            AtomicSmartPtr<IResourceDatabase> m_resourceDatabase;

            ///< String pool for efficient string management
            AtomicRawPtr<StringPool<c8>> m_stringPool;

            ///< String pool for efficient string management
            AtomicRawPtr<StringPool<wchar_t>> m_stringPoolW;

            AtomicRawPtr<StringPool<c8>> m_propertyNamePool;
            AtomicRawPtr<StringPool<c8>> m_propertyValuePool;

            std::unique_ptr<StringPool<c8>> m_ownedStringPool;
            std::unique_ptr<StringPool<c8>> m_ownedPropertyNamePool;
            std::unique_ptr<StringPool<c8>> m_ownedPropertyValuePool;

            // System Managers - Thread-safe atomic smart pointers
            AtomicSmartPtr<IAiManager>
                m_aiManager;  ///< AI system manager for artificial intelligence features
            AtomicSmartPtr<Properties>
                m_editorSettings;  ///< Configuration settings specific to editor mode
            AtomicSmartPtr<Properties>
                m_playerSettings;  ///< Configuration settings specific to player/game mode
            AtomicSmartPtr<IEditorManager>
                m_editorManager;  ///< Manager for editor-specific functionality

            AtomicSmartPtr<ILogManager> m_logManager;          ///< Logging system manager
            AtomicSmartPtr<IProcessManager> m_processManager;  ///< Process and task scheduling manager
            AtomicSmartPtr<IResourceManager> m_meshManager;    ///< Mesh resource management
            AtomicSmartPtr<Properties> m_properties;  ///< Application-wide configuration properties
            AtomicSmartPtr<vehicle::IVehicleManager> m_vehicleManager;  ///< Vehicle simulation manager

            AtomicSmartPtr<ISoundManager> m_soundManager;  ///< Audio system manager
            AtomicSmartPtr<ISelectionManager>
                m_selectionManager;  ///< Manager for object selection in editor

            AtomicSmartPtr<scene::IGamePrefabManager>
                m_prefabManager;                                 ///< Prefab template system manager
            AtomicSmartPtr<IMeshLoader> m_meshLoader;            ///< Mesh loading and processing
            AtomicSmartPtr<IResourceManager> m_resourceManager;  ///< General resource management system
            AtomicSmartPtr<ICommandManager>
                m_commandManager;                   ///< Command pattern implementation for undo/redo
            AtomicSmartPtr<IProfiler> m_profiler;   ///< Performance profiling and monitoring
            AtomicSmartPtr<IInputManager> m_input;  ///< Input device handling (keyboard, mouse, etc.)
            AtomicSmartPtr<IInputDeviceManager> m_inputManager;  ///< Low-level input device management
            AtomicSmartPtr<IConsole> m_console;                  ///< Debug console interface

            AtomicSmartPtr<IThreadPool> m_threadPool;  ///< Thread pool for parallel processing
            AtomicSmartPtr<render::IParticleManager> m_particleManager;  ///< Particle system management
            AtomicSmartPtr<scene::ICameraManager> m_cameraManager;       ///< Camera system management
            AtomicSmartPtr<render::IVideoManager>
                m_videoManager;                         ///< Video rendering and display management
            AtomicSmartPtr<IFileSystem> m_fileSystem;   ///< File system abstraction layer
            AtomicWeakPtr<IApplication> m_application;  ///< Main application instance
            AtomicSmartPtr<IPackageManager> m_packageManager;  ///< Package and module management
            AtomicSmartPtr<INetworkManager> m_networkManager;  ///< Network communication management
            AtomicSmartPtr<procedural::IProceduralManager>
                m_proceduralManager;  ///< Procedural content generation
            AtomicSmartPtr<physics::IPhysicsManager2D> m_physicsManager2;  ///< 2D physics simulation
            AtomicSmartPtr<physics::IPhysicsManager> m_physicsManager3;    ///< 3D physics simulation
            AtomicSmartPtr<IFSM> m_fsm;  ///< Main application finite state machine
            mutable AtomicSmartPtr<IDatabaseManager>
                m_database;  ///< Database management system (mutable for lazy initialization)
            AtomicSmartPtr<render::IGraphicsWindow> m_window;   ///< Main application window
            AtomicSmartPtr<ui::IUIWindow> m_sceneRenderWindow;  ///< Scene rendering viewport window
            AtomicSmartPtr<IPluginManager> m_pluginManager;     ///< Plugin system management

            // Path Configuration
            String m_settingsCachePath;   ///< Path to settings cache directory
            String m_cachePath;           ///< Path to general cache directory
            String m_projectPath;         ///< Path to current project directory
            String m_projectLibraryName;  ///< Name of the project library file
            String m_mediaPath;           ///< Path to media assets directory
            String m_renderMediaPath;     ///< Path to render-specific media directory

            // Application State - Atomic variables for thread-safe access
            atomic_s32 m_loadProgress = 0;            ///< Current loading progress (0-100)
            atomic_bool m_enableRenderer = false;     ///< Whether rendering is enabled
            atomic_bool m_isPauseMenuActive = false;  ///< Whether the pause menu is currently displayed
            atomic_bool m_isRunning = true;           ///< Whether the application is running
            atomic_bool m_isPlaying = false;          ///< Whether the application is in play mode
            atomic_bool m_isPaused = false;           ///< Whether the application is paused
            atomic_bool m_isEditor = false;           ///< Whether running in editor mode
            atomic_bool m_isEditorCamera = false;     ///< Whether using editor camera controls
            atomic_bool m_sceneLoading = false;

            /**
             * @brief Indicates if the application is in the process of shutting down.
             *
             * This flag is used to coordinate graceful shutdown across all subsystems
             * and prevent new operations from starting during cleanup.
             */
            atomic_bool m_quit = false;

            ///< The factory manager instances.
            FixedArray<AtomicSmartPtr<IFactoryManager>, (u32)TaskId::Count> m_factoryManagers;

            ///< Array of FSM managers for different tasks
            FixedArray<SmartPtr<IFSMManager>, (u32)TaskId::Count> m_fsmManagers;

            ///< Collection of loaded plugins
            ConcurrentArray<SmartPtr<ISharedObject>> m_plugins;

            ///< Mutex for thread-safe operations
            mutable RecursiveSpinMutex m_mutex;
        };

    }  // namespace core
}  // namespace workphone

#endif  // CApplicationManager_h__
