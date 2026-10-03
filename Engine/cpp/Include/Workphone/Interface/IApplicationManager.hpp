/*
zlib License

Copyright (c) 2026 ${Your Name}

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented.
2. Altered source versions must be plainly marked as such.
3. This notice may not be removed or altered from any source distribution.
*/

/**
 * @file IApplicationManager.hpp
 * @brief Public interface for the central application manager.
 *
 * This file declares the `IApplicationManager` interface which provides a
 * unified facade to control application lifecycle, core subsystems, and
 * globally shared resources. The interface is intended to be implemented by a
 * concrete application manager that coordinates systems such as rendering,
 * physics, scripting, resources, and various managers used across the engine.
 */

#ifndef IApplicationManager_h__
#define IApplicationManager_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/System/IEvent.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Parameter.hpp>
#include <Workphone/Core/StringPool.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    namespace core
    {
        /**
         * @class IApplicationManager
         * @brief Core application manager interface.
         *
         * The `IApplicationManager` is the primary entry point for interacting with
         * and querying the global state of the application. Implementations are
         * responsible for:
         * - Tracking application lifecycle (running, paused, playing, quitting).
         * - Owning or referencing core subsystem managers (graphics, input, audio,
         *   physics, scripting, resources, etc.).
         * - Providing global paths and configuration (project, media, cache).
         * - Facilitating cross-system events and task scheduling.
         * - Managing globally shared string pools and resource databases.
         *
         * The interface exposes convenience accessors that return either a
         * `SmartPtr<T>` or a raw pointer to managers when lower overhead is
         * required. A single global instance is expected (singleton-style) and
         * can be accessed via `IApplicationManager::instance()` or
         * `IApplicationManager::instancePtr()`.
         *
         * Implementations should ensure thread-safety for operations that may be
         * accessed from multiple threads. Lifetime of provided raw pointers is
         * tied to the owning application manager instance.
         */
        class WPCore_API IApplicationManager : public ISharedObject
        {
        public:
            IApplicationManager();

            /**
             * @brief Virtual destructor.
             * Ensures proper cleanup of resources when the application manager is destroyed.
             */
            ~IApplicationManager() override;

            /**
             * @brief Gets the renderer enabled state.
             * @return True if the renderer is enabled, false otherwise.
             */
            virtual bool getEnableRenderer() const = 0;

            /**
             * @brief Sets the renderer enabled state.
             * @param enable True to enable the renderer, false to disable it.
             */
            virtual void setEnableRenderer( bool enable ) = 0;

            /**
             * @brief Checks if the pause menu is currently active.
             * @return True if the pause menu is active, false otherwise.
             */
            virtual bool isPauseMenuActive() const = 0;

            /**
             * @brief Sets the pause menu active state.
             * @param pauseMenuActive True to activate the pause menu, false to deactivate it.
             */
            virtual void setPauseMenuActive( bool pauseMenuActive ) = 0;

            /**
             * @brief Checks if a scene is currently loading.
             * @return True if a scene is loading, false otherwise.
             */
            virtual bool isSceneLoading() const = 0;

            /**
             * @brief Sets the scene loading state.
             * @param loading True if a scene is loading, false otherwise.
             */
            virtual void setSceneLoading( bool loading ) = 0;

            /**
             * @brief Checks if the application is running in editor mode.
             * @return True if running in editor mode, false otherwise.
             */
            virtual bool isEditor() const = 0;

            /**
             * @brief Sets the editor mode state.
             * @param editor True to run in editor mode, false to run in game mode.
             */
            virtual void setEditor( bool editor ) = 0;

            /**
             * @brief Checks if the application is using editor camera mode.
             * @return True if using editor camera mode, false otherwise.
             */
            virtual bool isEditorCamera() const = 0;

            /**
             * @brief Sets the editor camera mode state.
             * @param editor True to use editor camera mode, false to use game camera mode.
             */
            virtual void setEditorCamera( bool editor ) = 0;

            /**
             * @brief Checks if the application is currently playing.
             * @return True if the application is playing, false otherwise.
             */
            virtual bool isPlaying() const = 0;

            /**
             * @brief Sets the playing state of the application.
             * @param playing True to start playing, false to stop playing.
             */
            virtual void setPlaying( bool playing ) = 0;

            /**
             * @brief Checks if the application is currently paused.
             * @return True if the application is paused, false otherwise.
             */
            virtual bool isPaused() const = 0;

            /**
             * @brief Sets the paused state of the application.
             * @param paused True to pause the application, false to resume it.
             */
            virtual void setPaused( bool paused ) = 0;

            /**
             * @brief Checks if the application is currently running.
             * @return True if the application is running, false otherwise.
             */
            virtual bool isRunning() const = 0;

            /**
             * @brief Sets the running state of the application.
             * @param running True to run the application, false to stop it.
             */
            virtual void setRunning( bool running ) = 0;

            /**
             * @brief Checks if the application is in the process of quitting.
             * @return True if the application is quitting, false otherwise.
             */
            virtual bool getQuit() const = 0;

            /**
             * @brief Sets the quit state of the application.
             * @param quit True to initiate application quit, false to keep running.
             */
            virtual void setQuit( bool quit ) = 0;

            /**
             * @brief Checks if there are any active tasks running.
             * @return True if there are tasks running, false otherwise.
             */
            virtual bool hasTasks() const = 0;

            /**
             * @brief Gets the path to the cache directory.
             * @return The absolute path to the cache directory.
             */
            virtual String getCachePath() const = 0;

            /**
             * @brief Sets the path to the cache directory.
             * @param cachePath The absolute path to set as the cache directory.
             */
            virtual void setCachePath( const String &cachePath ) = 0;

            /**
             * @brief Gets the path to the settings directory.
             * @return The absolute path to the settings directory.
             */
            virtual String getSettingsPath() const = 0;

            /**
             * @brief Sets the path to the settings directory.
             * @param cachePath The absolute path to set as the settings directory.
             */
            virtual void setSettingsPath( const String &cachePath ) = 0;

            /**
             * @brief Gets the path to the project directory.
             * @return The absolute path to the project directory.
             */
            virtual String getProjectPath() const = 0;

            /**
             * @brief Sets the path to the project directory.
             * @param projectPath The absolute path to set as the project directory.
             */
            virtual void setProjectPath( const String &projectPath ) = 0;

            /**
             * @brief Gets the name of the project library.
             * @return The name of the project library.
             */
            virtual String getProjectLibraryName() const = 0;

            /**
             * @brief Sets the name of the project library.
             * @param projectLibraryName The name to set for the project library.
             */
            virtual void setProjectLibraryName( const String &projectLibraryName ) = 0;

            /**
             * @brief Gets the path to the media directory.
             * @return The absolute path to the media directory.
             */
            virtual String getMediaPath() const = 0;

            /**
             * @brief Sets the path to the media directory.
             * @param mediaPath The absolute path to set as the media directory.
             */
            virtual void setMediaPath( const String &mediaPath ) = 0;

            /**
             * @brief Gets the path to the render media directory.
             * @return The absolute path to the render media directory.
             */
            virtual String getRenderMediaPath() const = 0;

            /**
             * @brief Sets the path to the render media directory.
             * @param renderMediaPath The absolute path to set as the render media directory.
             */
            virtual void setRenderMediaPath( const String &renderMediaPath ) = 0;

            /**
             * @brief Gets the current build configuration.
             * @return The build configuration string (e.g., "Debug", "Release").
             */
            virtual String getBuildConfig() const = 0;

            /**
             * @brief Gets the file extension used for project libraries.
             * @return The file extension string (e.g., ".dll", ".so").
             */
            virtual String getProjectLibraryExtension() const = 0;

            /**
             * @brief Gets the full path to the project library.
             * @return The absolute path to the project library file.
             */
            virtual String getProjectLibraryPath() const = 0;

            /**
             * @brief Gets the task ID used for state updates.
             * @return The task ID for state updates.
             */
            virtual TaskId getStateTask() const = 0;

            /**
             * @brief Gets the current application task ID.
             * @return The current application task ID.
             */
            virtual TaskId getApplicationTask() const = 0;

            /**
             * @brief Gets the Finite State Machine (FSM) instance.
             * @return A shared pointer to the FSM instance.
             */
            virtual IFSM *getFSMPtr() const = 0;

            /**
             * @brief Gets the Finite State Machine (FSM) instance.
             * @return A shared pointer to the FSM instance.
             */
            virtual SmartPtr<IFSM> getFSM() const = 0;

            /**
             * @brief Sets the Finite State Machine (FSM) instance.
             * @param fsm The new FSM instance to set.
             */
            virtual void setFSM( SmartPtr<IFSM> fsm ) = 0;

            /**
             * @brief Gets the editor settings.
             * @return A shared pointer to the editor settings properties.
             */
            virtual SmartPtr<Properties> getEditorSettings() const = 0;

            /**
             * @brief Sets the editor settings.
             * @param properties The new editor settings to set.
             */
            virtual void setEditorSettings( SmartPtr<Properties> properties ) = 0;

            /**
             * @brief Gets the player settings.
             * @return A shared pointer to the player settings properties.
             */
            virtual SmartPtr<Properties> getPlayerSettings() const = 0;

            /**
             * @brief Sets the player settings.
             * @param properties The new player settings to set.
             */
            virtual void setPlayerSettings( SmartPtr<Properties> properties ) = 0;

            /**
             * @brief Gets the AI manager instance.
             * @return A shared pointer to the AI manager.
             */
            virtual SmartPtr<IAiManager> getAiManager() const = 0;

            /**
             * @brief Sets the AI manager instance.
             * @param aiManager The new AI manager to set.
             */
            virtual void setAiManager( SmartPtr<IAiManager> aiManager ) = 0;

            /**
             * @brief Gets the log manager instance.
             * @return A shared pointer to the log manager.
             */
            virtual SmartPtr<ILogManager> getLogManager() const = 0;

            /**
             * @brief Sets the log manager instance.
             * @param logManager The new log manager to set.
             */
            virtual void setLogManager( SmartPtr<ILogManager> logManager ) = 0;

            /**
             * @brief Gets the factory manager instance.
             * @return A shared pointer to the factory manager.
             */
            virtual SmartPtr<IFactoryManager> getFactoryManager() const = 0;

            /**
             * @brief Gets the factory manager instance as a raw pointer.
             * @return A raw pointer to the factory manager.
             */
            virtual IFactoryManager *getFactoryManagerPtr() const = 0;

            /**
             * @brief Sets the factory manager instance.
             * @param factoryManager The new factory manager to set.
             */
            virtual void setFactoryManager( SmartPtr<IFactoryManager> factoryManager ) = 0;

            /**
             * @brief Gets the process manager instance.
             * @return A shared pointer to the process manager.
             */
            virtual SmartPtr<IProcessManager> getProcessManager() const = 0;

            /**
             * @brief Sets the process manager instance.
             * @param processManager The new process manager to set.
             */
            virtual void setProcessManager( SmartPtr<IProcessManager> processManager ) = 0;

            /**
             * @brief Gets the particle manager instance.
             * @return A shared pointer to the particle manager.
             */
            virtual SmartPtr<render::IParticleManager> getParticleManager() const = 0;

            /**
             * @brief Sets the particle manager instance.
             * @param particleManager The new particle manager to set.
             */
            virtual void setParticleManager( SmartPtr<render::IParticleManager> particleManager ) = 0;

            /**
             * @brief Gets the profiler instance.
             * @return A shared pointer to the profiler.
             */
            virtual IProfiler *getProfilerPtr() const = 0;

            /**
             * @brief Gets the profiler instance.
             * @return A shared pointer to the profiler.
             */
            virtual SmartPtr<IProfiler> getProfiler() const = 0;

            /**
             * @brief Sets the profiler instance.
             * @param profiler The new profiler to set.
             */
            virtual void setProfiler( SmartPtr<IProfiler> profiler ) = 0;

            /**
             * @brief Gets the application instance.
             * @return A shared pointer to the application.
             */
            virtual SmartPtr<IApplication> getApplication() const = 0;

            /**
             * @brief Sets the application instance.
             * @param application The new application to set.
             */
            virtual void setApplication( SmartPtr<IApplication> application ) = 0;

            /**
             * @brief Gets the job queue instance.
             * @return A shared pointer to the job queue.
             */
            virtual SmartPtr<IJobQueue> getJobQueue() const = 0;

            /**
             * @brief Gets the job queue instance as a raw pointer.
             * @return A raw pointer to the job queue.
             */
            virtual IJobQueue *getJobQueuePtr() const = 0;

            /**
             * @brief Sets the job queue instance.
             * @param jobQueue The new job queue to set.
             */
            virtual void setJobQueue( SmartPtr<IJobQueue> jobQueue ) = 0;

            /**
             * @brief Gets the graphics system instance.
             * @return A shared pointer to the graphics system.
             */
            virtual SmartPtr<render::IGraphicsSystem> getGraphicsSystem() const = 0;

            /**
             * @brief Gets the graphics system instance as a raw pointer.
             * @return A raw pointer to the graphics system.
             */
            virtual render::IGraphicsSystem *getGraphicsSystemPtr() const = 0;

            /**
             * @brief Sets the graphics system instance.
             * @param graphicsSystem The new graphics system to set.
             */
            virtual void setGraphicsSystem( SmartPtr<render::IGraphicsSystem> graphicsSystem ) = 0;

            /**
             * @brief Gets the video manager instance.
             * @return A shared pointer to the video manager.
             */
            virtual SmartPtr<render::IVideoManager> getVideoManager() const = 0;

            /**
             * @brief Sets the video manager instance.
             * @param videoManager The new video manager to set.
             */
            virtual void setVideoManager( SmartPtr<render::IVideoManager> videoManager ) = 0;

            /**
             * @brief Gets the task manager instance as a raw pointer.
             * @return A raw pointer to the task manager.
             */
            virtual ITaskManager *getTaskManagerPtr() const = 0;

            /**
             * @brief Gets the task manager instance.
             * @return A shared pointer to the task manager.
             */
            virtual SmartPtr<ITaskManager> getTaskManager() const = 0;

            /**
             * @brief Sets the task manager instance.
             * @param taskManager The new task manager to set.
             */
            virtual void setTaskManager( SmartPtr<ITaskManager> taskManager ) = 0;

            /**
             * @brief Gets the editor manager instance as a raw pointer.
             * @return A raw pointer to the editor manager.
             */
            virtual IEditorManager *getEditorManagerPtr() const = 0;

            /**
             * @brief Gets the editor manager instance.
             * @return A shared pointer to the editor manager.
             */
            virtual SmartPtr<IEditorManager> getEditorManager() const = 0;

            /**
             * @brief Sets the editor manager instance.
             * @param editorManager The new editor manager to set.
             */
            virtual void setEditorManager( SmartPtr<IEditorManager> editorManager ) = 0;

            /**
             * @brief Gets the file system instance as a raw pointer.
             * @return A raw pointer to the file system.
             */
            virtual IFileSystem *getFileSystemPtr() const = 0;

            /**
             * @brief Gets the file system instance.
             * @return A shared pointer to the file system.
             */
            virtual SmartPtr<IFileSystem> getFileSystem() const = 0;

            /**
             * @brief Sets the file system instance.
             * @param fileSystem The new file system to set.
             */
            virtual void setFileSystem( SmartPtr<IFileSystem> fileSystem ) = 0;

            /**
             * @brief Gets the timer instance.
             * @return A shared pointer to the timer.
             */
            virtual SmartPtr<ITimer> getTimer() = 0;

            /**
             * @brief Gets the timer instance as a raw pointer.
             * @return A raw pointer to the timer.
             */
            virtual ITimer *getTimerPtr() const = 0;

            /**
             * @brief Sets the timer instance.
             * @param timer The new timer to set.
             */
            virtual void setTimer( SmartPtr<ITimer> timer ) = 0;

            /**
             * @brief Gets the FSM manager instance.
             * @return A pointer to the FSM manager.
             */
            virtual IFSMManager *getFsmManagerPtr() const = 0;

            /**
             * @brief Gets the FSM manager instance.
             * @return A shared pointer to the FSM manager.
             */
            virtual SmartPtr<IFSMManager> getFsmManager() const = 0;

            /**
             * @brief Sets the FSM manager instance.
             * @param fsmManager The new FSM manager to set.
             */
            virtual void setFsmManager( SmartPtr<IFSMManager> fsmManager ) = 0;

            /**
             * @brief Gets the FSM manager instance for a specific task.
             * @param task The task to get the FSM manager for.
             * @return A shared pointer to the FSM manager for the specified task.
             */
            virtual SmartPtr<IFSMManager> getFsmManagerByTask( TaskId task ) const = 0;

            /**
             * @brief Sets the FSM manager instance for a specific task.
             * @param task The task to set the FSM manager for.
             * @param fsmManager The new FSM manager to set.
             */
            virtual void setFsmManagerByTask( TaskId task, SmartPtr<IFSMManager> fsmManager ) = 0;

            /**
             * @brief Gets the procedural manager instance.
             * @return A shared pointer to the procedural manager.
             */
            virtual SmartPtr<procedural::IProceduralManager> getProceduralManager() const = 0;

            /**
             * @brief Sets the procedural manager instance.
             * @param proceduralManager The new procedural manager to set.
             */
            virtual void setProceduralManager(
                SmartPtr<procedural::IProceduralManager> proceduralManager ) = 0;

            /**
             * @brief Gets the 2D physics manager instance.
             * @return A shared pointer to the 2D physics manager.
             */
            virtual SmartPtr<physics::IPhysicsManager2D> getPhysicsManager2D() const = 0;

            /**
             * @brief Sets the 2D physics manager instance.
             * @param physicsManager The new 2D physics manager to set.
             */
            virtual void setPhysicsManager2D( SmartPtr<physics::IPhysicsManager2D> physicsManager ) = 0;

            /**
             * @brief Gets the physics manager instance.
             * @return A shared pointer to the physics manager.
             */
            virtual SmartPtr<physics::IPhysicsManager> getPhysicsManager() const = 0;

            /**
             * @brief Gets the physics manager instance as a raw pointer.
             * @return A raw pointer to the physics manager.
             */
            virtual physics::IPhysicsManager *getPhysicsManagerPtr() const = 0;

            /**
             * @brief Sets the physics manager instance.
             * @param physicsManager The new physics manager to set.
             */
            virtual void setPhysicsManager( SmartPtr<physics::IPhysicsManager> physicsManager ) = 0;

            /**
             * @brief Gets the script manager instance.
             * @return A pointer to the script manager.
             */
            virtual IScriptManager *getScriptManagerPtr() const = 0;

            /**
             * @brief Gets the script manager instance.
             * @return A shared pointer to the script manager.
             */
            virtual SmartPtr<IScriptManager> getScriptManager() const = 0;

            /**
             * @brief Sets the script manager instance.
             * @param scriptManager The new script manager to set.
             */
            virtual void setScriptManager( SmartPtr<IScriptManager> scriptManager ) = 0;

            /**
             * @brief Gets the input manager instance.
             * @return A shared pointer to the input manager.
             */
            virtual SmartPtr<IInputManager> getInput() const = 0;

            /**
             * @brief Sets the input manager instance.
             * @param input The new input manager to set.
             */
            virtual void setInput( SmartPtr<IInputManager> input ) = 0;

            /**
             * @brief Gets the input device manager instance.
             * @return A shared pointer to the input device manager.
             */
            virtual SmartPtr<IInputDeviceManager> getInputDeviceManager() const = 0;

            /**
             * @brief Sets the input device manager instance.
             * @param inputManager The new input device manager to set.
             */
            virtual void setInputDeviceManager( SmartPtr<IInputDeviceManager> inputManager ) = 0;

            /**
             * @brief Gets the thread pool instance as a raw pointer.
             * @return A raw pointer to the thread pool.
             */
            virtual IThreadPool *getThreadPoolPtr() const = 0;

            /**
             * @brief Gets the thread pool instance.
             * @return A shared pointer to the thread pool.
             */
            virtual SmartPtr<IThreadPool> getThreadPool() const = 0;

            /**
             * @brief Sets the thread pool instance.
             * @param threadPool The new thread pool to set.
             */
            virtual void setThreadPool( SmartPtr<IThreadPool> threadPool ) = 0;

            /**
             * @brief Gets the console instance.
             * @return A shared pointer to the console.
             */
            virtual SmartPtr<IConsole> getConsole() const = 0;

            /**
             * @brief Sets the console instance.
             * @param console The new console to set.
             */
            virtual void setConsole( SmartPtr<IConsole> console ) = 0;

            /**
             * @brief Gets the camera manager instance.
             * @return A shared pointer to the camera manager.
             */
            virtual SmartPtr<scene::ICameraManager> getCameraManager() const = 0;

            /**
             * @brief Sets the camera manager instance.
             * @param cameraManager The new camera manager to set.
             */
            virtual void setCameraManager( SmartPtr<scene::ICameraManager> cameraManager ) = 0;

            /**
             * @brief Gets the state manager instance as a raw pointer.
             * @return A raw pointer to the state manager.
             */
            virtual IStateManager *getStateManagerPtr() const = 0;

            /**
             * @brief Gets the state manager instance.
             * @return A shared pointer to the state manager.
             */
            virtual SmartPtr<IStateManager> getStateManager() const = 0;

            /**
             * @brief Sets the state manager instance.
             * @param stateManager The new state manager to set.
             */
            virtual void setStateManager( SmartPtr<IStateManager> stateManager ) = 0;

            /**
             * @brief Gets the command manager instance as a raw pointer.
             * @return A raw pointer to the command manager.
             */
            virtual ICommandManager *getCommandManagerPtr() const = 0;

            /**
             * @brief Gets the command manager instance.
             * @return A shared pointer to the command manager.
             */
            virtual SmartPtr<ICommandManager> getCommandManager() const = 0;

            /**
             * @brief Sets the command manager instance.
             * @param commandManager The new command manager to set.
             */
            virtual void setCommandManager( SmartPtr<ICommandManager> commandManager ) = 0;

            /**
             * @brief Gets the resource manager instance.
             * @return A shared pointer to the resource manager.
             */
            virtual SmartPtr<IResourceManager> getResourceManager() const = 0;

            /**
             * @brief Sets the resource manager instance.
             * @param resourceManager The new resource manager to set.
             */
            virtual void setResourceManager( SmartPtr<IResourceManager> resourceManager ) = 0;

            /**
             * @brief Gets the prefab manager instance.
             * @return A shared pointer to the prefab manager.
             */
            virtual SmartPtr<scene::IGamePrefabManager> getPrefabManager() const = 0;

            /**
             * @brief Sets the prefab manager instance.
             * @param prefabManager The new prefab manager to set.
             */
            virtual void setPrefabManager( SmartPtr<scene::IGamePrefabManager> prefabManager ) = 0;

            /**
             * @brief Gets the mesh loader instance.
             * @return A shared pointer to the mesh loader.
             */
            virtual SmartPtr<IMeshLoader> getMeshLoader() const = 0;

            /**
             * @brief Sets the mesh loader instance.
             * @param meshLoader The new mesh loader to set.
             */
            virtual void setMeshLoader( SmartPtr<IMeshLoader> meshLoader ) = 0;

            /**
             * @brief Gets the resource database instance as a raw pointer.
             * @return A raw pointer to the resource database.
             */
            virtual IResourceDatabase *getResourceDatabasePtr() const = 0;

            /**
             * @brief Gets the resource database instance.
             * @return A shared pointer to the resource database.
             */
            virtual SmartPtr<IResourceDatabase> getResourceDatabase() const = 0;

            /**
             * @brief Sets the resource database instance.
             * @param resourceDatabase The new resource database to set.
             */
            virtual void setResourceDatabase( SmartPtr<IResourceDatabase> resourceDatabase ) = 0;

            /**
             * @brief Gets the scene manager instance as a raw pointer.
             * @return A raw pointer to the scene manager.
             */
            virtual scene::IGameManager *getGameManagerPtr() const = 0;

            /**
             * @brief Gets the scene manager instance.
             * @return A shared pointer to the scene manager.
             */
            virtual SmartPtr<scene::IGameManager> getGameManager() const = 0;

            /**
             * @brief Sets the scene manager instance.
             * @param gameManager The new game manager to set.
             */
            virtual void setGameManager( SmartPtr<scene::IGameManager> gameManager ) = 0;

            /**
             * @brief Gets the selection manager instance as a raw pointer.
             * @return A raw pointer to the selection manager.
             */
            virtual ISelectionManager *getSelectionManagerPtr() const = 0;

            /**
             * @brief Gets the selection manager instance.
             * @return A shared pointer to the selection manager.
             */
            virtual SmartPtr<ISelectionManager> getSelectionManager() const = 0;

            /**
             * @brief Sets the selection manager instance.
             * @param selectionManager The new selection manager to set.
             */
            virtual void setSelectionManager( SmartPtr<ISelectionManager> selectionManager ) = 0;

            /**
             * @brief Gets the sound manager instance.
             * @return A pointer to the sound manager.
             */
            virtual ISoundManager *getSoundManagerPtr() const = 0;

            /**
             * @brief Gets the sound manager instance.
             * @return A shared pointer to the sound manager.
             */
            virtual SmartPtr<ISoundManager> getSoundManager() const = 0;

            /**
             * @brief Sets the sound manager instance.
             * @param soundManager The new sound manager to set.
             */
            virtual void setSoundManager( SmartPtr<ISoundManager> soundManager ) = 0;

            /**
             * @brief Gets the UI manager instance as a raw pointer.
             * @return A raw pointer to the UI manager.
             */
            virtual ui::IUIManager *getUIPtr() const = 0;

            /**
             * @brief Gets the UI manager instance.
             * @return A shared pointer to the UI manager.
             */
            virtual SmartPtr<ui::IUIManager> getUI() const = 0;

            /**
             * @brief Sets the UI manager instance.
             * @param ui The new UI manager to set.
             */
            virtual void setUI( SmartPtr<ui::IUIManager> ui ) = 0;

            /**
             * @brief Gets the render UI manager instance.
             * @return A pointer to the render UI manager.
             */
            virtual ui::IUIManager *getRenderUIPtr() const = 0;

            /**
             * @brief Gets the render UI manager instance.
             * @return A shared pointer to the render UI manager.
             */
            virtual SmartPtr<ui::IUIManager> getRenderUI() const = 0;

            /**
             * @brief Sets the render UI manager instance.
             * @param renderUI The new render UI manager to set.
             */
            virtual void setRenderUI( SmartPtr<ui::IUIManager> renderUI ) = 0;

            /**
             * @brief Gets the database manager instance.
             * @return A shared pointer to the database manager.
             */
            virtual SmartPtr<IDatabaseManager> getDatabase() const = 0;

            /**
             * @brief Sets the database manager instance.
             * @param database The new database manager to set.
             */
            virtual void setDatabase( SmartPtr<IDatabaseManager> database ) = 0;

            /**
             * @brief Gets the mesh manager instance.
             * @return A shared pointer to the mesh manager.
             */
            virtual SmartPtr<IResourceManager> getMeshManager() const = 0;

            /**
             * @brief Sets the mesh manager instance.
             * @param meshManager The new mesh manager to set.
             */
            virtual void setMeshManager( SmartPtr<IResourceManager> meshManager ) = 0;

            /**
             * @brief Gets the vehicle manager instance.
             * @return A shared pointer to the vehicle manager.
             */
            virtual SmartPtr<vehicle::IVehicleManager> getVehicleManager() const = 0;

            /**
             * @brief Sets the vehicle manager instance.
             * @param vehicleManager The new vehicle manager to set.
             */
            virtual void setVehicleManager( SmartPtr<vehicle::IVehicleManager> vehicleManager ) = 0;

            /**
             * @brief Gets the current loading progress.
             * @return The loading progress value.
             */
            virtual s32 getLoadProgress() const = 0;

            /**
             * @brief Sets the loading progress.
             * @param loadProgress The new loading progress value.
             */
            virtual void setLoadProgress( s32 loadProgress ) = 0;

            /**
             * @brief Adds to the current loading progress.
             * @param loadProgress The amount to add to the loading progress.
             */
            virtual void addLoadProgress( s32 loadProgress ) = 0;

            /**
             * @brief Gets the array of loaded actors.
             * @return An array of shared pointers to actors.
             */
            virtual Array<SmartPtr<scene::IGameActor>> getActors() const = 0;

            /**
             * @brief Gets the plugin manager instance.
             * @return A shared pointer to the plugin manager.
             */
            virtual SmartPtr<IPluginManager> getPluginManager() const = 0;

            /**
             * @brief Sets the plugin manager instance.
             * @param pluginManager The new plugin manager to set.
             */
            virtual void setPluginManager( SmartPtr<IPluginManager> pluginManager ) = 0;

            /**
             * @brief Adds a plugin to the application.
             * @param plugin The plugin to add.
             */
            virtual void addPlugin( SmartPtr<ISharedObject> plugin ) = 0;

            /**
             * @brief Removes a plugin from the application.
             * @param plugin The plugin to remove.
             */
            virtual void removePlugin( SmartPtr<ISharedObject> plugin ) = 0;

            /**
             * @brief Gets the main window instance as a raw pointer.
             * @return A raw pointer to the main window.
             */
            virtual render::IGraphicsWindow *getWindowPtr() const = 0;

            /**
             * @brief Gets the main window instance.
             * @return A shared pointer to the main window.
             */
            virtual SmartPtr<render::IGraphicsWindow> getWindow() const = 0;

            /**
             * @brief Sets the main window instance.
             * @param window The new main window to set.
             */
            virtual void setWindow( SmartPtr<render::IGraphicsWindow> window ) = 0;

            /**
             * @brief Gets the scene render window instance as a raw pointer.
             * @return A raw pointer to the scene render window.
             */
            virtual ui::IUIWindow *getSceneRenderWindowPtr() const = 0;

            /**
             * @brief Gets the scene render window instance.
             * @return A shared pointer to the scene render window.
             */
            virtual SmartPtr<ui::IUIWindow> getSceneRenderWindow() const = 0;

            /**
             * @brief Sets the scene render window instance.
             * @param sceneRenderWindow The new scene render window to set.
             */
            virtual void setSceneRenderWindow( SmartPtr<ui::IUIWindow> sceneRenderWindow ) = 0;

            /**
             * @brief Gets a component by its type ID.
             * @param typeId The type ID of the component to get.
             * @return A shared pointer to the component, or nullptr if not found.
             */
            virtual SmartPtr<scene::IComponent> getComponentByType( u32 typeId ) const = 0;

            /**
             * @brief Gets a component by its type ID as a raw pointer.
             * @param typeId The type ID of the component to get.
             * @return A raw pointer to the component, or nullptr if not found.
             */
            virtual scene::IComponent *getComponentPtrByType( u32 typeId ) const = 0;

            /**
             * @brief Triggers an event in the application.
             * @param eventType The type of event to trigger.
             * @param eventValue The value associated with the event.
             * @param arguments The arguments for the event.
             * @param sender The object that triggered the event.
             * @param object The object associated with the event.
             * @param event The event data. Set its target for direct delivery; a null event or
             * null event target retains broadcast behavior.
             * @param sendNow Whether to send the event immediately.
             * @param taskFlags A bitmask of Thread::*_Flag values for queued delivery. TaskId enum
             * values are not task flags.
             * @return The result of the event trigger.
             */
            virtual Parameter triggerEvent( EventType eventType, hash_type eventValue,
                                            const Array<Parameter> &arguments,
                                            SmartPtr<ISharedObject> sender,
                                            SmartPtr<ISharedObject> object, SmartPtr<IEvent> event,
                                            bool sendNow = false,
                                            u32 taskFlags = std::numeric_limits<u32>::max() ) = 0;

            /**
             * @brief Clears event jobs created by triggerEvent().
             *
             * Releases references held by pending EventJob instances so shutdown can continue
             * without queued events retaining sender, target or event payload objects.
             */
            virtual void clearAllEvents() = 0;

            /**
             * @brief Gets the network manager instance.
             * @return A shared pointer to the network manager.
             */
            virtual SmartPtr<INetworkManager> getNetworkManager() const = 0;

            /**
             * @brief Sets the network manager instance.
             * @param networkManager The new network manager to set.
             */
            virtual void setNetworkManager( SmartPtr<INetworkManager> networkManager ) = 0;

            /**
             * @brief Gets the package manager instance.
             * @return A shared pointer to the package manager.
             */
            virtual SmartPtr<IPackageManager> getPackageManager() const = 0;

            /**
             * @brief Sets the package manager instance.
             * @param packageManager The new package manager to set.
             */
            virtual void setPackageManager( SmartPtr<IPackageManager> packageManager ) = 0;

            /**
             * @brief Gets the global string pool.
             */
            virtual StringPool<c8> *getStringPool() const = 0;

            /**
             * @brief Sets the global string pool.
             */
            virtual void setStringPool( StringPool<c8> *pool ) = 0;

            /**
             * @brief Gets the global string pool.
             */
            virtual StringPool<wchar_t> *getStringPoolW() const = 0;

            /**
             * @brief Sets the global string pool.
             */
            virtual void setStringPoolW( StringPool<wchar_t> *pool ) = 0;
            /**
             * @brief Gets the global string pool.
             */
            virtual StringPool<c8> *getPropertyNamePool() const = 0;

            /**
             * @brief Sets the global string pool.
             */
            virtual void setPropertyNamePool( StringPool<c8> *pool ) = 0;

            /**
             * @brief Gets the global string pool.
             */
            virtual StringPool<c8> *getPropertyValuePool() const = 0;

            /**
             * @brief Sets the global string pool.
             */
            virtual void setPropertyValuePool( StringPool<c8> *pool ) = 0;

            /**
             * @brief Gets a component by its template type.
             * @tparam T The type of component to get.
             * @return A shared pointer to the component, or nullptr if not found.
             */
            template <class T>
            SmartPtr<T> getComponent() const;

            /**
             * @brief Gets a component by its template type as a raw pointer.
             * @tparam T The type of component to get.
             * @return A raw pointer to the component, or nullptr if not found.
             */
            template <class T>
            T *getComponentPtr() const;

            /**
             * @brief Gets the singleton instance of the application manager.
             * @return A shared pointer to the application manager instance.
             */
            static SmartPtr<IApplicationManager> instance();

            /**
             * @brief Gets the singleton instance of the application manager as a raw pointer.
             * @return A raw pointer to the application manager instance.
             */
            static IApplicationManager *instancePtr();

            /**
             * @brief Sets the singleton instance of the application manager.
             * @param instance The new application manager instance to set.
             */
            static void setInstance( SmartPtr<IApplicationManager> instance );

            /**
             * @brief Sets the singleton instance of the application manager as a raw pointer.
             * @param instance The new application manager instance to set.
             */
            static void setInstancePtr( IApplicationManager *instance );

            WP_CLASS_REGISTER_DECL;

        protected:
            ///< The singleton instance of the application manager.
            static std::atomic<IApplicationManager *> m_instance;
        };

        template <class T>
        SmartPtr<T> IApplicationManager::getComponent() const
        {
            auto typeInfo = T::typeInfo();
            auto component = getComponentByType( typeInfo );
            return workphone::static_pointer_cast<T>( component );
        }

        template <class T>
        T *IApplicationManager::getComponentPtr() const
        {
            auto typeInfo = T::typeInfo();
            return (T *)getComponentPtrByType( typeInfo );
        }

    }  // namespace core
}  // namespace workphone

#endif  // IApplicationManager_h__
