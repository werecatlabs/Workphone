#ifndef _CGraphicsSystemOgreNext_H
#define _CGraphicsSystemOgreNext_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <WPGraphicsOgreNext/ImguiManagerOgreNext.hpp>
#include <WPGraphicsOgreNext/CompositorManager.hpp>
#include <Workphone/Graphics/GraphicsSystem.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <OgreWindowEventUtilities.h>
#include <OgreFrameListener.h>
#include <OgreLogManager.h>
#include <OgreRenderSystem.h>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Core/HashMap.hpp>
#include <Workphone/Core/Set.hpp>

namespace Ogre
{
    class ParticleEmitterFactory;
}

#if WP_BUILD_RENDERER_VULKAN
namespace Ogre
{
    class VulkanPlugin;
}
#endif

namespace workphone
{
    namespace render
    {

        /**
         * @brief Graphics system implementation that wraps Ogre (OgreNext) functionality.
         *
         * This class implements the engine's GraphicsSystem interface using the Ogre
         * rendering engine (OgreNext). It provides comprehensive management of Ogre::Root,
         * plugins, render systems, windows, and related subsystems. The graphics system
         * handles rendering lifecycle, resource management, window creation/destruction,
         * composition, deferred shading, and integration with the engine's event system.
         *
         * Key responsibilities:
         * - Manages Ogre::Root and render system lifecycle
         * - Creates and manages render windows and viewports
         * - Loads and initializes rendering plugins (GL3+, GLES2, D3D11, Metal, etc.)
         * - Routes Ogre logging into the engine's logging system
         * - Bridges engine events to graphics-specific operations
         * - Manages post-processing via compositor workspaces
         * - Provides access to material, texture, resource, and instance managers
         * - Supports terrain rendering via Ogre::Terra
         * - Facilitates per-frame updates and render scheduling
         *
         * @note This class should be used as a singleton within an application.
         * @note Thread-safety considerations apply to certain operations; see individual method documentation.
         */
        class CGraphicsSystemOgreNext : public GraphicsSystem
        {
        public:
            /**
             * @brief Ogre frame listener that hooks into the rendering loop.
             *
             * This listener receives per-frame callbacks from Ogre's render loop and
             * forwards timing information to the owning CGraphicsSystemOgreNext instance.
             * It allows the graphics system to perform time-dependent updates and maintain
             * frame timing statistics.
             *
             * @see Ogre::FrameListener for detailed frame event semantics.
             */
            class AppFrameListener : public Ogre::FrameListener
            {
            public:
                /**
                 * Constructs the frame listener.
                 *
                 * @param graphicsSystem Pointer to the owning graphics system instance.
                 *                       Must not be null during the lifetime of this listener.
                 */
                AppFrameListener( CGraphicsSystemOgreNext *graphicsSystem );

                /**
                 * Destructor. Cleans up any resources held by the listener.
                 */
                ~AppFrameListener() override;

                /**
                 * Invoked after a frame has finished rendering.
                 *
                 * @param evt Frame event containing timing and state information.
                 * @return true to continue the frame loop; false to halt rendering.
                 *
                 * @see Ogre::FrameListener::frameEnded
                 */
                bool frameEnded( const Ogre::FrameEvent &evt ) override;

                /**
                 * Invoked before a frame begins rendering.
                 *
                 * @param evt Frame event containing timing and state information.
                 * @return true to continue the frame loop; false to halt rendering.
                 *
                 * @see Ogre::FrameListener::frameStarted
                 */
                bool frameStarted( const Ogre::FrameEvent &evt ) override;

                /**
                 * Invoked when a frame has been queued for rendering but not yet processed.
                 *
                 * This is useful for updating renderable objects or other frame-dependent state.
                 *
                 * @param evt Frame event containing timing and state information.
                 * @return true to continue the frame loop; false to halt rendering.
                 *
                 * @see Ogre::FrameListener::frameRenderingQueued
                 */
                bool frameRenderingQueued( const Ogre::FrameEvent &evt ) override;

            protected:
                /** Pointer to the owning graphics system instance. */
                CGraphicsSystemOgreNext *m_graphicsSystem = nullptr;

                /** Accumulated time (in seconds) since the last frame update. */
                float m_time = 0.0f;
            };

            /**
             * @brief Listener for native window events originating from Ogre render windows.
             *
             * This listener responds to window-level events such as close requests.
             * It allows the graphics system to perform cleanup or cancellation logic
             * when render windows are being destroyed.
             *
             * @see Ogre::WindowEventListener for the complete event interface.
             */
            class WindowEventListener : public Ogre::WindowEventListener
            {
            public:
                /**
                 * Constructs the window event listener.
                 */
                WindowEventListener();

                /**
                 * Destructor. Cleans up resources.
                 */
                ~WindowEventListener() override;

                /**
                 * Invoked when a render window is about to close.
                 *
                 * @param rw Pointer to the Ogre::RenderWindow being closed.
                 * @return true to allow the window to close; false to prevent it.
                 *
                 * @see Ogre::WindowEventListener::windowClosing
                 */
                bool windowClosing( Ogre::RenderWindow *rw );
            };

            /**
             * @brief Listener that bridges the global event system to the graphics system.
             *
             * This listener responds to engine-level events and forwards them to the
             * associated CGraphicsSystemOgreNext instance for appropriate handling.
             * Uses weak reference semantics to avoid ownership cycles and prevent
             * resource leaks.
             *
             * @see IEventListener for the event handling interface.
             */
            class GraphicsSystemEventListener : public IEventListener
            {
            public:
                /**
                 * Constructs the event listener.
                 */
                GraphicsSystemEventListener();

                /**
                 * Destructor. Cleans up event subscriptions.
                 */
                ~GraphicsSystemEventListener() override;

                /**
                 * Handles incoming events and forwards them to the graphics system.
                 *
                 * @param eventType The type identifier of the event.
                 * @param eventValue A hash value associated with the event.
                 * @param arguments Array of parameters passed with the event.
                 * @param sender Pointer to the object that sent the event.
                 * @param object Pointer to the object associated with the event.
                 * @param event Pointer to the event itself.
                 * @return Parameter value to be returned to the event sender.
                 *
                 * @see IEventListener::handleEvent for detailed semantics.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * Retrieves the owning graphics system instance.
                 *
                 * @return Shared pointer to the graphics system, or null if owner has been deleted.
                 */
                SmartPtr<CGraphicsSystemOgreNext> getOwner() const;

                /**
                 * Sets the owning graphics system instance.
                 *
                 * @param owner Shared pointer to the graphics system to associate with this listener.
                 */
                void setOwner( SmartPtr<CGraphicsSystemOgreNext> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                /** Weak reference to the owning graphics system instance. */
                AtomicWeakPtr<CGraphicsSystemOgreNext> m_owner;
            };

            /**
             * @brief Custom listener that routes Ogre log messages to the engine's logging system.
             *
             * This listener intercepts all messages logged by Ogre and forwards them
             * to the engine's centralized logging and debugging infrastructure,
             * ensuring visibility of Ogre diagnostic output within the application's
             * overall logging context.
             *
             * @see Ogre::LogListener for the logging interface.
             */
            class CustomLogListener : public Ogre::LogListener
            {
            public:
                /**
                 * Constructs the custom log listener.
                 */
                CustomLogListener();

                /**
                 * Destructor. Cleans up logging resources.
                 */
                ~CustomLogListener();

                /**
                 * Called when Ogre logs a message.
                 *
                 * Routes the message to the engine's logging system, potentially applying
                 * filters or transformations based on message level or context.
                 *
                 * @param message The log message text.
                 * @param lml The message log level (debug, info, warning, critical error).
                 * @param maskDebug true if the message is masked as debug-only.
                 * @param logName The name of the log category/file.
                 * @param skipMessage Reference to a boolean; set to true to prevent further processing.
                 *
                 * @see Ogre::LogListener::messageLogged
                 */
                void messageLogged( const Ogre::String &message, Ogre::LogMessageLevel lml,
                                    bool maskDebug, const Ogre::String &logName,
                                    bool &skipMessage ) override;
            };

            /**
             * @brief Listener for render-system-specific events and state changes.
             *
             * This listener responds to events emitted by the underlying Ogre::RenderSystem
             * implementation, allowing the graphics system to react to changes in render
             * state, device state, or other render-system-level events.
             *
             * @see Ogre::RenderSystem::Listener for detailed event semantics.
             */
            class RenderSystemListener : public Ogre::RenderSystem::Listener
            {
            public:
                /**
                 * Constructs the render system listener.
                 */
                RenderSystemListener();

                /**
                 * Destructor. Cleans up resources.
                 */
                ~RenderSystemListener() override;

                /**
                 * Called when a named event occurs in the render system.
                 *
                 * @param eventName The name/identifier of the event that occurred.
                 * @param parameters Optional name-value pairs providing additional context about the event.
                 *
                 * @see Ogre::RenderSystem::Listener::eventOccurred
                 */
                virtual void eventOccurred( const String &eventName,
                                            const NameValuePairList *parameters = nullptr );

                /**
                 * Retrieves the owning graphics system instance.
                 *
                 * @return Shared pointer to the graphics system, or null if owner has been deleted.
                 */
                SmartPtr<CGraphicsSystemOgreNext> getOwner() const;

                /**
                 * Sets the owning graphics system instance.
                 *
                 * @param owner Shared pointer to the graphics system to associate with this listener.
                 */
                void setOwner( SmartPtr<CGraphicsSystemOgreNext> owner );

            protected:
                /** Weak reference to the owning graphics system instance. */
                AtomicWeakPtr<CGraphicsSystemOgreNext> m_owner;
            };

            /**
             * Constructs the graphics system.
             *
             * Initializes the graphics system in a non-configured state.
             * Call configure() or load() to fully initialize Ogre and render resources.
             */
            CGraphicsSystemOgreNext();

            /**
             * Copy constructor is explicitly deleted.
             *
             * The graphics system manages unique resources and should not be copied.
             */
            CGraphicsSystemOgreNext( const CGraphicsSystemOgreNext &other ) = delete;

            /**
             * Destructor. Cleans up all Ogre resources, listeners, and managed objects.
             *
             * This invokes unload() to release resources, unregister event listeners,
             * and destroy Ogre objects such as the render root, plugins, and listeners.
             */
            ~CGraphicsSystemOgreNext() override;

            /**
             * Loads graphics system resources and initializes internal state.
             *
             * Performs deferred initialization of the graphics system, including plugin
             * loading, Ogre root creation, and registration of event listeners.
             * Should be called after configure() to fully initialize the system.
             *
             * @param data Optional shared object containing load-time configuration.
             *
             * @see configure(), unload()
             * @copydoc IGraphicsSystem::load
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * Unloads graphics resources and cleans up internal state.
             *
             * Unregisters all event listeners, destroys Ogre resources, closes render
             * windows, and releases allocated memory. Should be called before destroying
             * the graphics system or when shutting down the graphics subsystem.
             *
             * @param data Optional shared object with unload-time parameters.
             *
             * @see load()
             * @copydoc IGraphicsSystem::unload
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * Retrieves the current graphics system properties.
             *
             * @return Shared pointer to a Properties object describing the current state.
             *
             * @copydoc GraphicsSystem::getProperties
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * Sets graphics system properties.
             *
             * @param properties Shared pointer to a Properties object with desired settings.
             *
             * @copydoc GraphicsSystem::setProperties
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * Retrieves child objects owned or managed by the graphics system.
             *
             * Returns a collection of managers, windows, and sub-systems (such as
             * material managers, texture managers, and compositor systems) that
             * should be exposed to higher-level engine systems.
             *
             * @return Array of shared pointers to child objects.
             *
             * @copydoc GraphicsSystem::getChildObjects
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * Configures the graphics system with the provided settings.
             *
             * Applies graphics configuration such as render system selection, resolution,
             * fullscreen mode, and quality parameters. Must be called before load().
             *
             * @param config Shared pointer to graphics settings containing desired configuration.
             * @return true if configuration was successful; false if an error occurred.
             *
             * @see load(), IGraphicsSettings
             * @copydoc IGraphicsSystem::configure
             */
            bool configure( SmartPtr<IBuildDirector> config ) override;

            CompositorManager *getCompositorManagerPtr() const;

            /**
             * Retrieves the compositor manager.
             *
             * The compositor manager handles post-processing effects and compositor workspaces
             * that transform rendered output (e.g., bloom, color grading, tone mapping).
             *
             * @return Shared pointer to the compositor manager, or null if not set.
             *
             * @copydoc IGraphicsSystem::getCompositorManager
             */
            SmartPtr<CompositorManager> getCompositorManager() const;

            /**
             * Sets the compositor manager.
             *
             * @param compositorManager Shared pointer to the compositor manager to use.
             *
             * @see getCompositorManager()
             */
            void setCompositorManager( SmartPtr<CompositorManager> compositorManager );

            /**
             * Retrieves the resource group manager.
             *
             * The resource group manager organizes and loads graphics resources (meshes, textures, etc.)
             * into named groups for efficient memory management and organization.
             *
             * @return Shared pointer to the resource group manager interface.
             *
             * @copydoc IGraphicsSystem::getResourceGroupManager
             */
            SmartPtr<IResourceGroupManager> getResourceGroupManager() const override;

            /**
             * Retrieves a raw pointer to the material manager.
             *
             * The material manager handles creation and management of materials
             * (surface definitions with textures, shaders, and properties).
             *
             * @return Raw pointer to the material manager, or null if not available.
             *
             * @note Use getMaterialManager() for a shared pointer (exception-safe) unless raw access is required.
             * @copydoc IGraphicsSystem::getMaterialManagerPtr
             */
            IMaterialManager *getMaterialManagerPtr() const override;

            /**
             * Retrieves the material manager.
             *
             * The material manager handles creation and management of materials
             * (surface definitions with textures, shaders, and properties).
             *
             * @return Shared pointer to the material manager interface.
             *
             * @see getMaterialManagerPtr() for raw pointer access.
             * @copydoc IGraphicsSystem::getMaterialManager
             */
            SmartPtr<IMaterialManager> getMaterialManager() const override;

            /**
             * Retrieves the instance manager.
             *
             * The instance manager handles efficient rendering of multiple identical or similar objects
             * using instancing techniques for improved performance.
             *
             * @return Shared pointer to the instance manager interface.
             *
             * @copydoc IGraphicsSystem::getInstanceManager
             */
            SmartPtr<IInstanceManager> getInstanceManager() const override;

            /**
             * Creates a new render window.
             *
             * Creates and registers a render window with the specified properties.
             * The returned window is owned and managed by the graphics system.
             *
             * @param name Unique identifier for the render window.
             * @param width Width of the window in pixels.
             * @param height Height of the window in pixels.
             * @param fullScreen true to create a fullscreen window; false for windowed mode.
             * @param properties Additional properties for window creation (e.g., FSAA, color depth).
             * @return Shared pointer to the created window, or null if creation failed.
             *
             * @see getRenderWindow(), getWindows()
             * @copydoc IGraphicsSystem::createRenderWindow
             */
            SmartPtr<IGraphicsWindow> createRenderWindow(
                const String &name, u32 width, u32 height, bool fullScreen,
                const SmartPtr<Properties> &properties ) override;

            /**
             * Retrieves an existing render window by name.
             *
             * @param name Name of the window to retrieve. If empty, returns the default window.
             * @return Shared pointer to the window, or null if not found.
             *
             * @see createRenderWindow(), getDefaultWindow()
             * @copydoc IGraphicsSystem::getRenderWindow
             */
            SmartPtr<IGraphicsWindow> getRenderWindow(
                const String &name = StringUtil::EmptyString ) const override;

            /**
             * Retrieves the loading priority for a child object.
             *
             * Returns an integer priority used to determine the order in which child objects
             * are loaded. Higher values are loaded first.
             *
             * @param obj Shared pointer to the child object.
             * @return Priority value for loading order.
             */
            s32 getLoadPriority( SmartPtr<ISharedObject> obj ) override;

            /**
             * Updates the graphics system each frame.
             *
             * Performs per-frame processing including:
             * - Updating debug overlays
             * - Processing queued load/unload operations
             * - Dispatching render tasks
             * - Polling windowing events
             *
             * Should be called once per frame by the application or main loop.
             *
             * @see load(), unload(), messagePump()
             * @copydoc IGraphicsSystem::update
             */
            void update() override;

            void render();

            /**
             * Restores graphics configuration from saved settings.
             *
             * Loads previously persisted graphics settings from disk and applies them
             * to the graphics system (e.g., resolution, render API, quality settings).
             *
             * @see saveConfig()
             */
            void restoreConfig();

            /**
             * Persists the current graphics configuration to disk.
             *
             * Saves the current graphics settings (e.g., resolution, render API, quality)
             * to persistent storage for restoration on subsequent application runs.
             *
             * @see restoreConfig()
             */
            void saveConfig();

            /**
             * Retrieves the default/primary render window.
             *
             * @return Shared pointer to the default window, or null if no window is set as default.
             *
             * @see setDefaultWindow(), getWindows()
             */
            SmartPtr<IGraphicsWindow> getDefaultWindow() const override;

            /**
             * Sets the default/primary render window.
             *
             * Designates which render window should be considered the main window for the application.
             *
             * @param window Shared pointer to the window to set as default.
             *
             * @see getDefaultWindow()
             */
            void setDefaultWindow( SmartPtr<IGraphicsWindow> window ) override;

            /**
             * Retrieves all render windows managed by the graphics system.
             *
             * @return Array of shared pointers to all active render windows.
             *
             * @see createRenderWindow(), getRenderWindow()
             */
            Array<SmartPtr<IGraphicsWindow>> getWindows() const override;

            /**
             * Creates overlay elements used for debug text rendering.
             *
             * Initializes overlay components (text areas and shadow elements) used to
             * display per-frame debug information such as FPS, timing, and performance metrics.
             *
             * @see generateDebugText()
             */
            void createDebugTextOverlay( void );

            /**
             * Generates debug text information for overlay display.
             *
             * Formats current frame statistics and performance information into a string
             * suitable for rendering on debug overlays.
             *
             * @param timeSinceLast Time elapsed (in seconds) since the last frame.
             * @param outText Output string that will contain the formatted debug information.
             *
             * @see createDebugTextOverlay()
             */
            void generateDebugText( f32 timeSinceLast, Ogre::String &outText );

            /**
             * Polls and processes windowing messages.
             *
             * Processes platform-specific window events (e.g., window move, resize, close)
             * that have been queued by the operating system. Must be called regularly
             * to ensure responsive window behavior and event handling.
             *
             * @note This is typically called as part of the main application loop.
             * @copydoc IGraphicsSystem::messagePump
             */
            void messagePump() override;

            /**
             * Configures the renderer for a scene, window, and camera.
             *
             * Sets up rendering state including viewport, camera bindings, and optionally
             * attaches a compositor workspace for post-processing effects.
             *
             * @param scene The graphics scene (scene manager) to render from.
             * @param window The render window to render to.
             * @param camera The camera defining the view.
             * @param workspaceName Name of the compositor workspace to bind (if any).
             * @param enabled true to activate the compositor workspace immediately.
             *
             * @see getCompositorWorkspace(), setCompositorWorkspace()
             * @copydoc IGraphicsSystem::setupRenderer
             */
            void setupRenderer( SmartPtr<IGraphicsScene> scene, SmartPtr<IGraphicsWindow> window,
                                SmartPtr<IGraphicsCamera> camera, String workspaceName,
                                bool enabled ) override;

            /**
             * Retrieves the task ID for graphics state updates.
             *
             * Returns the task scheduler ID used for queuing graphics state update operations.
             * Graphics state updates are performed on a dedicated graphics thread.
             *
             * @return Task ID for state update operations.
             *
             * @see getRenderTask()
             */
            TaskId getStateTask() const override;

            /**
             * Retrieves the task ID for rendering operations.
             *
             * Returns the task scheduler ID used for queuing rendering operations.
             * Rendering is performed on a dedicated graphics thread.
             *
             * @return Task ID for rendering operations.
             *
             * @see getStateTask()
             */
            TaskId getRenderTask() const override;

            /**
             * Loads a graphics object.
             *
             * Loads a graphics object either immediately or by queuing it on the graphics thread,
             * depending on the current execution context and the forceQueue flag.
             *
             * @param graphicsObject The graphics object to load.
             * @param forceQueue If true, always queue the operation on the graphics thread;
             *                   if false, may load immediately if on the graphics thread.
             *
             * @see unloadObject()
             * @copydoc IGraphicsSystem::loadObject
             */
            void loadObject( SmartPtr<ISharedObject> graphicsObject, bool forceQueue = false ) override;

            /**
             * Unloads a graphics object.
             *
             * Unloads a graphics object either immediately or by queuing the unload operation
             * on the graphics thread, depending on the current execution context and the forceQueue flag.
             *
             * @param graphicsObject The graphics object to unload.
             * @param forceQueue If true, always queue the operation on the graphics thread;
             *                   if false, may unload immediately if on the graphics thread.
             *
             * @see loadObject()
             * @copydoc IGraphicsSystem::unloadObject
             */
            void unloadObject( SmartPtr<ISharedObject> graphicsObject,
                               bool forceQueue = false ) override;

            /**
             * Retrieves the current rendering API in use.
             *
             * @return The RenderApi enumeration value indicating the active API (e.g., D3D11, OpenGL, Metal).
             *
             * @see setRenderApi()
             */
            RenderApi getRenderApi() const;

            /**
             * Sets the rendering API to use.
             *
             * @param renderApi The RenderApi enumeration value specifying the desired API.
             *
             * @see getRenderApi()
             */
            void setRenderApi( RenderApi renderApi );

            /**
             * Retrieves the mesh converter.
             *
             * The mesh converter handles conversion of mesh formats and optimization
             * for rendering-specific requirements.
             *
             * @return Shared pointer to the mesh converter, or null if not available.
             *
             * @copydoc IGraphicsSystem::getMeshConverter
             */
            SmartPtr<IMeshConverter> getMeshConverter() const override;

            /**
             * Sets the mesh converter.
             *
             * @param meshConverter Shared pointer to the mesh converter to use.
             *
             * @see getMeshConverter()
             * @copydoc IGraphicsSystem::setMeshConverter
             */
            void setMeshConverter( SmartPtr<IMeshConverter> meshConverter ) override;

            /**
             * Checks if the underlying Ogre system is valid and initialized.
             *
             * @return true if Ogre::Root and the render system are valid; false otherwise.
             *
             * @copydoc IGraphicsSystem::isValid
             */
            bool isValid() const override;

            /**
             * Checks if the Real-Time Shader System (RTSS) is enabled.
             *
             * RTSS provides automatic shader generation for fixed-function-like rendering.
             *
             * @return true if RTSS is enabled; false otherwise.
             *
             * @see setUseRTSS()
             */
            bool getUseRTSS() const;

            /**
             * Enables or disables the Real-Time Shader System (RTSS).
             *
             * @param useRTSS true to enable RTSS; false to disable.
             *
             * @see getUseRTSS()
             */
            void setUseRTSS( bool useRTSS );

            /**
             * Retrieves the current compositor workspace.
             *
             * The compositor workspace coordinates post-processing effects and
             * composition of render targets.
             *
             * @return Raw pointer to the Ogre::CompositorWorkspace, or nullptr if not set.
             *
             * @see setCompositorWorkspace()
             */
            Ogre::CompositorWorkspace *getCompositorWorkspace() const;

            /**
             * Sets the compositor workspace.
             *
             * @param workspace Raw pointer to the Ogre::CompositorWorkspace to use.
             *
             * @see getCompositorWorkspace()
             */
            void setCompositorWorkspace( Ogre::CompositorWorkspace *workspace );

            /**
             * Retrieves the terrain system instance.
             *
             * Ogre::Terra provides terrain rendering capabilities.
             *
             * @return Raw pointer to the Ogre::Terra instance, or nullptr if not available.
             *
             * @see setTerra()
             */
            Ogre::Terra *getTerra() const;

            /**
             * Sets the terrain system instance.
             *
             * @param terra Raw pointer to the Ogre::Terra instance to use.
             *
             * @see getTerra()
             */
            void setTerra( Ogre::Terra *terra );

            /**
             * Retrieves the Ogre v1 overlay system.
             *
             * The overlay system manages 2D overlay elements in the legacy Ogre v1 format.
             *
             * @return Raw pointer to the Ogre::v1::OverlaySystem, or nullptr if not available.
             *
             * @see setOverlaySystem()
             */
            Ogre::v1::OverlaySystem *getOverlaySystem() const;

            /**
             * Sets the Ogre v1 overlay system.
             *
             * @param overlaySystem Raw pointer to the Ogre::v1::OverlaySystem to use.
             *
             * @see getOverlaySystem()
             */
            void setOverlaySystem( Ogre::v1::OverlaySystem *overlaySystem );

            /**
             * Retrieves the font manager.
             *
             * The font manager handles loading and management of fonts used for
             * text rendering in overlays and UI elements.
             *
             * @return Shared pointer to the font manager, or null if not available.
             *
             * @copydoc IGraphicsSystem::getFontManager
             */
            SmartPtr<IFontManager> getFontManager() const override;

            /**
             * Sets the font manager.
             *
             * @param fontManager Shared pointer to the font manager to use.
             *
             * @see getFontManager()
             */
            void setFontManager( SmartPtr<IFontManager> fontManager );

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
            /**
             * Windows message procedure for native window events.
             *
             * This is the internal window procedure used by Ogre render windows
             * when creating Win32 windows. It handles Windows native events and
             * forwards them to the appropriate Ogre and engine handlers.
             *
             * @param hWnd Handle to the window.
             * @param uMsg Message identifier.
             * @param wParam First message parameter.
             * @param lParam Second message parameter.
             * @return Result of message processing.
             */
            static LRESULT CALLBACK _WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE && !defined __OBJC__ && !defined( __LP64__ )
            /**
             * OS X Carbon event handler for native window events.
             *
             * This is the internal event handler used by Ogre render windows
             * when creating OS X Carbon windows. It handles native Carbon events
             * and forwards them to the appropriate Ogre and engine handlers.
             *
             * @param nextHandler Reference to the next handler in the chain.
             * @param event The Carbon event to handle.
             * @param wnd Pointer to the window structure.
             * @return Status code for event handling.
             */
            static OSStatus _CarbonWindowHandler( EventHandlerCallRef nextHandler, EventRef event,
                                                  void *wnd );
#endif

            SmartPtr<UIRenderer> getUIRenderer() const;

            void setUIRenderer( SmartPtr<UIRenderer> uiRenderer );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * Checks if the graphics system is currently updating.
             *
             * @return true if an update is in progress; false otherwise.
             *
             * @see setUpdating()
             */
            bool isUpdating() const;

            /**
             * Sets the updating state of the graphics system.
             *
             * @param updating true to mark the system as updating; false otherwise.
             *
             * @see isUpdating()
             */
            void setUpdating( bool updating );

            /**
             * Chooses and initializes the appropriate render system.
             *
             * Selects the best available render system based on configuration
             * and system capabilities.
             *
             * @return true if a render system was successfully chosen; false otherwise.
             */
            bool chooseRenderSystem();

            /**
             * Sets default properties on the render system.
             *
             * Configures render system defaults such as viewport orientation,
             * texture filtering, and other baseline settings.
             *
             * @param renderSystem Pointer to the Ogre::RenderSystem to configure.
             */
            void setRenderSystemDefaults( Ogre::RenderSystem *renderSystem );

            /**
             * Initializes the graphics system and optionally creates the primary window.
             *
             * Performs system-level initialization including Ogre root creation,
             * render system setup, and optionally window creation.
             *
             * @param autoCreateWindow true to automatically create the primary window.
             * @param windowTitle Title for the primary window (if auto-created).
             * @param customCapabilitiesConfig Path to custom capabilities configuration file.
             * @return Shared pointer to the created or initialized window, or null on error.
             *
             * @see load(), configure()
             */
            SmartPtr<IGraphicsWindow> initialise( bool autoCreateWindow, const String &windowTitle,
                                                  const String &customCapabilitiesConfig );

            /**
             * Loads and initializes Ogre plugins.
             *
             * Loads rendering plugins (e.g., GL3+, GLES2, D3D11, Metal) based on
             * build configuration and user selection.
             *
             * @param root Pointer to the Ogre::Root instance to install plugins into.
             *
             * @see load()
             */
            void installPlugins( Ogre::Root *root );

            void setupStateObject();

            /**
             * Root Ogre object managing rendering subsystems and lifecycle.
             *
             * Owned by this graphics system. Initialized during load() and destroyed
             * during unload().
             */
            Ogre::Root *m_root = nullptr;

            SmartPtr<UIRenderer> m_uiRenderer;

            /**
             * ImGui manager for OgreNext integration.
             *
             * Owned by this graphics system. Created during load() once the render
             * system is available and destroyed during unload().
             */
            ImguiManagerOgreNext *m_imguiManager = nullptr;

            /**
             * Event listener that routes engine events to this graphics system.
             *
             * Subscribes to engine-level events and forwards relevant ones to
             * graphics-specific handlers.
             */
            SmartPtr<GraphicsSystemEventListener> m_eventListener;

            /**
             * Compositor manager for post-processing effects and workspaces.
             *
             * Thread-safe shared pointer allowing atomic access from multiple threads.
             */
            AtomicSmartPtr<CompositorManager> m_compositorManager;

            /**
             * Text overlay element for rendering debug information on-screen.
             *
             * Displays per-frame statistics and performance metrics.
             */
            Ogre::v1::TextAreaOverlayElement *m_debugText = nullptr;

            /**
             * Shadow overlay element rendered behind debug text for improved readability.
             *
             * Provides a background for debug text to ensure visibility on all backgrounds.
             */
            Ogre::v1::TextAreaOverlayElement *m_debugTextShadow = nullptr;

            /**
             * Frame listener receiving per-frame callbacks from Ogre.
             *
             * Forwards frame timing and state information to the graphics system.
             */
            AppFrameListener *m_frameListener = nullptr;

            /**
             * Listener for resource loading events and notifications from Ogre.
             *
             * Allows tracking and management of resource lifecycle.
             */
            ResourceLoadingListener *m_resourceLoadingListener = nullptr;

            /**
             * Listener responding to material and shader-related events.
             *
             * Handles shader compilation, material loading, and related graphics state changes.
             */
            MaterialListener *m_materialListener = nullptr;

            /**
             * Optional particle plugin instance if loaded.
             *
             * Provides particle system rendering capabilities when available.
             */
            Ogre::Plugin *m_particlePlugin = nullptr;

            /**
             * Built-in point emitter factory used by code-created particle systems.
             *
             * Ogre Next keeps emitter implementations in an optional ParticleFX plugin,
             * but this renderer is built without that plugin. The factory is registered
             * during configure() and kept alive until after Ogre::Root is destroyed.
             */
            Ogre::ParticleEmitterFactory *m_pointEmitterFactory = nullptr;

            /**
             * Static plugin loader for Ogre when built with static linking.
             *
             * Used in static build configurations to load plugins without dynamic linking.
             */
            Ogre::StaticPluginLoader *m_staticPluginLoader = nullptr;

            /**
             * Ogre v1 overlay system for managing legacy-format overlay elements.
             *
             * Supports rendering of v1-style 2D overlays and UI elements.
             */
            Ogre::v1::OverlaySystem *m_overlaySystem = nullptr;

#ifdef OGRE_STATIC_LIB
            /**
             * Dummy value reserved for static linking initialization.
             *
             * Used to ensure proper static library linking of Ogre plugins.
             */
            unsigned int mDummy = 0;

#    if WP_BUILD_RENDERER_GL3PLUS
            /**
             * OpenGL 3+ renderer plugin (static build).
             *
             * Provides hardware-accelerated rendering using OpenGL 3.0 and later.
             */
            Ogre::GL3PlusPlugin *mGL3PlusPlugin = nullptr;
#    endif

#    if WP_BUILD_RENDERER_GLES2
            /**
             * OpenGL ES 2.0 renderer plugin (static build).
             *
             * Provides hardware-accelerated rendering for mobile and embedded systems.
             */
            Ogre::GLES2Plugin *mGLES2Plugin = nullptr;
#    endif

#    if WP_BUILD_RENDERER_DX11
            /**
             * Direct3D 11 renderer plugin (static build).
             *
             * Provides hardware-accelerated rendering using Microsoft's Direct3D 11 API.
             */
            Ogre::D3D11Plugin *mD3D11PlusPlugin = nullptr;
#    endif

#    if WP_BUILD_RENDERER_VULKAN
            /**
             * Vulkan renderer plugin (static build).
             */
            Ogre::VulkanPlugin *mVulkanPlugin = nullptr;
#    endif

#    if WP_BUILD_RENDERER_METAL
            /**
             * Metal renderer plugin (static build).
             *
             * Provides hardware-accelerated rendering on macOS and iOS using Apple's Metal API.
             */
            Ogre::MetalPlugin *mMetalPlugin = nullptr;
#    endif
#endif

            /**
             * Terrain system plugin instance.
             *
             * Provides terrain rendering capabilities through Ogre::Terra.
             * May be null if terrain support is not enabled.
             */
            Ogre::Terra *m_terra = nullptr;

            /**
             * Log listener that routes Ogre log messages to the engine's logging system.
             *
             * Ensures that all Ogre diagnostic output is captured and integrated into
             * the application's centralized logging infrastructure.
             */
            Ogre::LogListener *m_logListener = nullptr;

            /**
             * Scene manager factory for creating cell-based scene managers.
             *
             * Provides custom scene manager implementations optimized for
             * cell-based spatial organization.
             */
            CellSceneManagerFactory *m_cellSceneManagerFactory = nullptr;

            /**
             * Atomic flag indicating whether the graphics system is currently updating.
             *
             * Used for thread-safe detection of update operations in progress.
             */
            atomic_bool m_isUpdating = false;

            /**
             * Internal list of Ogre::Window pointers created by the graphics system.
             *
             * Mirrors the managed IGraphicsWindow objects to track underlying Ogre window instances.
             */
            Array<Ogre::Window *> _msWindows;

            /**
             * Path to the Ogre resource folder.
             *
             * Used by Ogre to locate media files, shaders, and other rendering resources.
             */
            Ogre::String m_resourcePath;
        };

        inline CompositorManager *CGraphicsSystemOgreNext::getCompositorManagerPtr() const
        {
            return m_compositorManager.get();
        }

    }  // end namespace render
}  // namespace workphone

#endif
