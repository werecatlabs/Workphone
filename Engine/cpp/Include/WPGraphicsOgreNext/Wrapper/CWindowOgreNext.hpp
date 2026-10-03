#ifndef __CWindowOgreNext_H
#define __CWindowOgreNext_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <WPGraphicsOgreNext/Wrapper/CRenderTargetOgreNext.hpp>
#include <Workphone/Graphics/Texture.hpp>
#include <Workphone/Graphics/GraphicsWindow.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <OgreWindowEventUtilities.h>

namespace workphone
{
    namespace render
    {

        /**
         * @brief OgreNext-specific implementation of a render window.
         *
         * CWindowOgreNext provides a concrete implementation of the Window interface
         * using the OgreNext rendering engine. It handles window creation, event
         * management, viewport management, and platform-specific window operations.
         *
         * This class integrates with OgreNext's window system and provides thread-safe
         * event handling through a concurrent queue mechanism. It supports cross-platform
         * window operations including Windows (Win32) and macOS platforms.
         *
         * @par Features:
         * - OgreNext window creation and management
         * - Thread-safe window event handling
         * - Platform-specific window handle management
         * - Viewport creation and management
         * - Window state synchronization
         * - Event listener management
         *
         * @par Usage Example:
         * @code
         * auto window = factoryManager->make_ptr<CWindowOgreNext>();
         * window->load(data);
         * window->setTitle("My Application");
         * window->resize(1920, 1080);
         * @endcode
         *
         * @see CRenderTargetOgreNext
         * @see Window
         * @see IGraphicsWindow
         */
        class CWindowOgreNext : public CRenderTargetOgreNext<GraphicsWindow>
        {
        public:
            /**
             * @brief Internal window event listener for OgreNext window events.
             *
             * This class implements the OgreNext WindowEventListener interface to
             * handle window-specific events such as window closing. It maintains
             * a weak reference to its owning CWindowOgreNext instance to avoid
             * circular dependencies.
             */
            class WindowListener : public Ogre::WindowEventListener
            {
            public:
                /**
                 * @brief Default constructor.
                 */
                WindowListener();

                /**
                 * @brief Virtual destructor.
                 */
                ~WindowListener() override;

                /**
                 * @brief Called when the window is about to close.
                 *
                 * This method is called by OgreNext when the window receives a close
                 * event. It triggers application shutdown by default.
                 *
                 * @param rw Pointer to the OgreNext render window being closed
                 * @return True to allow window closing, false to prevent it
                 */
                bool windowClosing( Ogre::Window *rw ) override;

                /**
                 * @brief Gets the owning CWindowOgreNext instance.
                 * @return Smart pointer to the owner, or null if owner has been destroyed
                 */
                SmartPtr<CWindowOgreNext> getOwner() const;

                /**
                 * @brief Sets the owning CWindowOgreNext instance.
                 * @param owner Smart pointer to the CWindowOgreNext that owns this listener
                 */
                void setOwner( SmartPtr<CWindowOgreNext> owner );

            protected:
                /// Weak reference to the owning window to avoid circular dependencies
                AtomicWeakPtr<CWindowOgreNext> m_owner;
            };

            class WindowTexture : public Texture
            {
            public:
                WindowTexture();
                ~WindowTexture() override;

                void load( SmartPtr<ISharedObject> data ) override;
                void unload( SmartPtr<ISharedObject> data ) override;

                Ogre::TextureGpu *getRenderTexture() const;

                void setRenderTexture( Ogre::TextureGpu *texture );

                void _getObject( void **ppObject ) const override;

                WP_CLASS_REGISTER_DECL;

            private:
                Ogre::TextureGpu *m_texture = nullptr;
            };

            /**
             * @brief Constructor.
             *
             * Initializes the window with default state and sets up the state management
             * infrastructure including state context, listeners, and render target data.
             */
            CWindowOgreNext();

            /**
             * @brief Destructor.
             *
             * Automatically calls unload() to ensure proper cleanup of resources.
             */
            ~CWindowOgreNext() override;

            /**
             * @brief Loads and initializes the window.
             *
             * Creates the OgreNext window with the appropriate parameters and sets up
             * platform-specific window handling. This method handles external window
             * handles or creates new platform-specific windows as needed.
             *
             * @param data Optional initialization data (currently unused)
             *
             * @par Platform Support:
             * - Windows: Creates Win32 window if no external handle provided
             * - macOS: Creates native macOS window if no external handle provided
             * - Linux: Uses X11 window handles
             *
             * @throw std::exception If window creation fails
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads and destroys the window.
             *
             * Cleans up all resources including the OgreNext window, event listeners,
             * platform-specific windows, and state management objects.
             *
             * @param data Optional unload data (currently unused)
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Updates the window state and processes queued events.
             *
             * This method should be called regularly (typically once per frame) to:
             * - Update window metrics (position, size, flags)
             * - Process queued window events
             * - Synchronize state with listeners
             */
            void update() override;

            /**
             * @brief Adds a viewport to the window.
             *
             * Creates a new viewport associated with the specified camera and adds it
             * to this window's viewport collection.
             *
             * @param id Unique identifier for the viewport
             * @param camera Camera to associate with the viewport
             * @param ZOrder Z-order for viewport rendering (higher values render on top)
             * @param left Left coordinate of viewport in normalized coordinates [0,1]
             * @param top Top coordinate of viewport in normalized coordinates [0,1]
             * @param width Width of viewport in normalized coordinates [0,1]
             * @param height Height of viewport in normalized coordinates [0,1]
             * @return Smart pointer to the created viewport, or null on failure
             */
            SmartPtr<IViewport> addViewport( hash_type id, SmartPtr<IGraphicsCamera> camera,
                                             s32 ZOrder = 0, f32 left = 0.0f, f32 top = 0.0f,
                                             f32 width = 1.0f, f32 height = 1.0f ) override;

            /**
             * @brief Handles a window event asynchronously.
             *
             * Queues the event for processing during the next update() call.
             * This ensures thread-safe event handling.
             *
             * @param event Window event to process
             */
            void handleEvent( SmartPtr<IGraphicsWindowEvent> event ) override;

            /**
             * @brief Destroys the window.
             *
             * Immediately destroys the OgreNext window. This is more immediate than
             * unload() and should be used when the window needs to be destroyed
             * without going through the normal unload process.
             */
            void destroy() override;

            /**
             * @brief Resizes the window to the specified dimensions.
             *
             * Requests a window resize operation. If called from the render thread,
             * the resize is performed immediately. Otherwise, it's queued as a
             * state message for thread-safe execution.
             *
             * @param width New width in pixels
             * @param height New height in pixels
             */
            void resize( u32 width, u32 height ) override;

            /**
             * @brief Notifies that the window has been moved or resized externally.
             *
             * Updates internal state and triggers appropriate events. This should be
             * called when the window is moved or resized by external means (e.g.,
             * user dragging the window).
             */
            void windowMovedOrResized() override;

            /**
             * @brief Repositions the window to the specified coordinates.
             *
             * Moves the window to the given screen coordinates. If called from the
             * render thread, the operation is performed immediately. Otherwise, it's
             * queued as a state message.
             *
             * @param left New left position in screen coordinates
             * @param top New top position in screen coordinates
             */
            void reposition( s32 left, s32 top ) override;

            /**
             * @brief Maximizes the window.
             *
             * On Windows platforms, this uses the native ShowWindow API with
             * SW_SHOWMAXIMIZED. Updates internal state after maximization.
             */
            void maximize() override;

            /**
             * @brief Gets a custom attribute from the underlying OgreNext window.
             *
             * Delegates to the OgreNext window's getCustomAttribute method.
             *
             * @param name Name of the attribute to retrieve
             * @param pData Pointer to receive the attribute value
             */
            void getCustomAttribute( const String &name, void *pData ) override;

            /**
             * @brief Gets the underlying OgreNext window object.
             *
             * Internal method for retrieving the raw OgreNext window pointer.
             *
             * @param ppObject Pointer to receive the window object pointer
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Gets the OgreNext window instance.
             * @return Pointer to the underlying Ogre::Window, or null if not created
             */
            Ogre::Window *getWindow() const;

            /**
             * @brief Sets the OgreNext window instance.
             *
             * This method is typically used internally or when adopting an
             * existing OgreNext window.
             *
             * @param window Pointer to the Ogre::Window to adopt
             */
            void setWindow( Ogre::Window *window );

            /**
             * @brief Sets up the window with an existing OgreNext window.
             *
             * Configures this instance to use an existing OgreNext window,
             * including setting up event listeners and updating the loading state.
             *
             * @param window Pointer to the existing Ogre::Window to adopt
             */
            void setupWindow( Ogre::Window *window );

            /**
             * @brief Gets the native window handle.
             *
             * Retrieves the platform-specific window handle. The exact type
             * depends on the platform:
             * - Windows: HWND
             * - macOS: NSWindow or Metal device
             * - Linux: X11 Window
             *
             * @param pData Pointer to receive the window handle
             */
            void getWindowHandle( void *pData ) override;

            /**
             * @brief Gets the native device handle.
             *
             * Currently not implemented - reserved for future use.
             *
             * @param pData Pointer to receive the device handle
             */
            void getDeviceHandle( void *pData ) override;

            /**
             * @brief Handles state messages for thread-safe operations.
             *
             * Processes state messages such as resize and reposition requests
             * that were queued from other threads.
             *
             * @param message State message to process
             * @return True if the message was handled, false otherwise
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message );

            /**
             * @brief Handles state changes for window properties.
             *
             * Updates the window based on changes to window state data such as
             * visibility, size, and position.
             *
             * @param state State object containing the changed data
             * @return True if the state change was handled, false otherwise
             */
            bool handleStateChanged( SmartPtr<IState> &state );

            SmartPtr<ITexture> getTexture() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Processes a queued window event.
             *
             * Handles platform-specific window events such as WM_CLOSE, WM_SIZE,
             * WM_MOVE, etc. on Windows platforms. Dispatches events to registered
             * listeners.
             *
             * @param event Window event to process
             */
            void handleQueuedEvent( SmartPtr<IGraphicsWindowEvent> event );

            /// Thread-safe queue for window events
            ConcurrentQueue<SmartPtr<IGraphicsWindowEvent>> m_eventQueue;

            /// OgreNext window event listener for this window
            WindowListener *m_windowEventListener = nullptr;

            /// Pointer to the underlying OgreNext window
            Ogre::Window *m_window = nullptr;

#if defined WP_PLATFORM_WIN32
            /// Platform-specific Windows window implementation (Windows only)
            WindowWin32Alt *m_windowWin32 = nullptr;
#elif defined WP_PLATFORM_APPLE
            /// Platform-specific macOS window implementation (macOS only)
            WindowMacOS *m_osWindow = nullptr;
#endif
        };
    }  // end namespace render
}  // namespace workphone

#endif
