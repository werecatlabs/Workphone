#ifndef __CWindowOgre_H
#define __CWindowOgre_H

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <WPGraphicsOgre/Wrapper/CRenderTargetOgre.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Graphics/GraphicsWindow.hpp>
#include <Workphone/Core/Array.hpp>
#include <OgreWindowEventUtilities.h>

namespace workphone
{
    namespace render
    {
        /**
         * @class CWindowOgre
         * @brief Ogre-based implementation of the IGraphicsWindow interface for managing render windows.
         *
         * This class provides a complete wrapper around Ogre::RenderWindow, handling window
         * creation, destruction, resizing, and event management. It serves as the primary
         * interface for rendering output and user interaction with the graphics system.
         *
         * The window supports features such as:
         * - Full-screen and windowed modes
         * - Dynamic resizing and repositioning
         * - Viewport management
         * - Window event handling (move, resize, focus changes)
         * - Custom window attributes and handles
         *
         * @see IGraphicsWindow
         * @see CRenderTargetOgre
         * @see Ogre::RenderWindow
         */
        class CWindowOgre : public CRenderTargetOgre<GraphicsWindow>
        {
        public:
            /**
             * @class WindowStateListener
             * @brief Internal state listener for handling window state changes.
             *
             * This listener responds to state messages and state changes for the window,
             * providing a bridge between the engine's state management system and the
             * window implementation.
             */
            class WindowStateListener : public IStateListener
            {
            public:
                /** @brief Constructs the window state listener. */
                WindowStateListener();

                /** @brief Destroys the window state listener. */
                ~WindowStateListener() override;

                /**
                 * @brief Handles incoming state messages.
                 * @param message The state message to process.
                 * @return True if the message was handled successfully, false otherwise.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handles state changes.
                 * @param state The new state object.
                 * @return True if the state change was handled successfully, false otherwise.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Gets the owning window instance.
                 * @return Pointer to the CWindowOgre that owns this listener.
                 */
                SmartPtr<CWindowOgre> getOwner() const;

                /**
                 * @brief Sets the owning window instance.
                 * @param owner Pointer to the CWindowOgre that owns this listener.
                 */
                void setOwner( SmartPtr<CWindowOgre> owner );

            protected:
                ///< Pointer to the owning window.
                AtomicWeakPtr<CWindowOgre> m_owner;
            };

            /**
             * @class WindowListener
             * @brief Ogre window event listener for handling native window events.
             *
             * This class intercepts Ogre window events (move, resize, close, focus)
             * and forwards them to the CWindowOgre instance for processing and
             * notification to registered listeners.
             */
            class WindowListener : public Ogre::WindowEventListener
            {
            public:
                /** @brief Constructs the window listener. */
                WindowListener();

                /** @brief Destroys the window listener. */
                ~WindowListener() override;

                /**
                 * @brief Called when the window is moved.
                 * @param rw The render window that was moved.
                 */
                void windowMoved( Ogre::RenderWindow *rw ) override;

                /**
                 * @brief Called when the window is resized.
                 * @param rw The render window that was resized.
                 */
                void windowResized( Ogre::RenderWindow *rw ) override;

                /**
                 * @brief Called when the window is about to close.
                 * @param rw The render window that is closing.
                 * @return True to allow the window to close, false to prevent it.
                 */
                bool windowClosing( Ogre::RenderWindow *rw ) override;

                /**
                 * @brief Called after the window has closed.
                 * @param rw The render window that was closed.
                 */
                void windowClosed( Ogre::RenderWindow *rw ) override;

                /**
                 * @brief Called when the window focus changes.
                 * @param rw The render window whose focus changed.
                 */
                void windowFocusChange( Ogre::RenderWindow *rw ) override;

                /**
                 * @brief Gets the owning window instance.
                 * @return Pointer to the CWindowOgre that owns this listener.
                 */
                CWindowOgre *getOwner() const;

                /**
                 * @brief Sets the owning window instance.
                 * @param owner Pointer to the CWindowOgre that owns this listener.
                 */
                void setOwner( CWindowOgre *owner );

            protected:
                CWindowOgre *m_owner = nullptr;  ///< Pointer to the owning window.
            };

            /** @brief Constructs a new CWindowOgre instance. */
            CWindowOgre();

            /** @brief Destroys the CWindowOgre instance and releases resources. */
            ~CWindowOgre() override;

            /**
             * @brief Loads the window with the provided data.
             * @param data Optional shared object data for initialization.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the window and releases associated resources.
             * @param data Optional shared object data for cleanup.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Handles a window event.
             * @param event The window event to handle.
             */
            void handleEvent( SmartPtr<IGraphicsWindowEvent> event ) override;

            /**
             * @brief Updates the window state.
             *
             * Called once per frame to process pending window operations and events.
             */
            void update() override;

            /**
             * @brief Initializes the window with an existing Ogre render window.
             * @param window Pointer to the Ogre::RenderWindow to wrap.
             */
            void initialise( Ogre::RenderWindow *window );

            /**
             * @brief Sets the window to fullscreen or windowed mode with specified dimensions.
             * @param fullScreen True for fullscreen mode, false for windowed mode.
             * @param width The width of the window/screen in pixels.
             * @param height The height of the window/screen in pixels.
             */
            void setFullscreen( bool fullScreen, unsigned int width, unsigned int height ) override;

            /**
             * @brief Destroys the window and releases all associated resources.
             */
            void destroy() override;

            /**
             * @brief Resizes the window to the specified dimensions.
             * @param width The new width in pixels.
             * @param height The new height in pixels.
             */
            void resize( u32 width, u32 height ) override;

            /**
             * @brief Notifies the window that it has been moved or resized.
             *
             * This should be called after external window manipulation to ensure
             * internal state consistency.
             */
            void windowMovedOrResized() override;

            /**
             * @brief Repositions the window to the specified screen coordinates.
             * @param left The x-coordinate of the window's top-left corner.
             * @param top The y-coordinate of the window's top-left corner.
             */
            void reposition( s32 left, s32 top ) override;

            /**
             * @brief Maximizes the window to fill the screen.
             */
            void maximize() override;

            /**
             * @brief Checks if the window is currently visible.
             * @return True if the window is visible, false otherwise.
             */
            bool isVisible() const override;

            /**
             * @brief Sets the window visibility.
             * @param visible True to show the window, false to hide it.
             */
            void setVisible( bool visible ) override;

            /**
             * @brief Checks if the window has been closed.
             * @return True if the window is closed, false otherwise.
             */
            bool isClosed() const override;

            /**
             * @brief Checks if this is the primary render window.
             * @return True if this is the primary window, false otherwise.
             */
            bool isPrimary() const override;

            /**
             * @brief Checks if the window is in fullscreen mode.
             * @return True if the window is fullscreen, false if windowed.
             */
            bool isFullScreen() const override;

            /**
             * @brief Sets the fullscreen state of the window.
             * @param fullscreen True to enable fullscreen mode, false for windowed mode.
             */
            void setFullscreen( bool fullscreen ) override;

            /**
             * @brief Gets the window metrics including dimensions and position.
             * @param[out] width The width of the window in pixels.
             * @param[out] height The height of the window in pixels.
             * @param[out] colourDepth The color depth in bits per pixel.
             * @param[out] left The x-coordinate of the window's top-left corner.
             * @param[out] top The y-coordinate of the window's top-left corner.
             */
            void getMetrics( u32 &width, u32 &height, u32 &colourDepth, s32 &left, s32 &top );

            /**
             * @brief Suggests the optimal pixel format for the window.
             * @return The suggested pixel format as a byte value.
             */
            u8 suggestPixelFormat() const;

            /**
             * @brief Checks if the window should be deactivated on focus change.
             * @return True if deactivation on focus change is enabled, false otherwise.
             */
            bool isDeactivatedOnFocusChange() const override;

            /**
             * @brief Sets whether the window should be deactivated on focus change.
             * @param deactivate True to enable deactivation on focus change, false to disable.
             */
            void setDeactivateOnFocusChange( bool deactivate ) override;

            /**
             * @brief Retrieves a custom platform-specific attribute.
             * @param name The name of the attribute to retrieve.
             * @param pData Pointer to store the attribute data.
             */
            void getCustomAttribute( const String &name, void *pData ) override;

            /**
             * @brief Gets the underlying Ogre object.
             * @param[out] ppObject Pointer to receive the Ogre::RenderWindow pointer.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Gets all viewports attached to this window.
             * @return Array of smart pointers to IViewport objects.
             */
            Array<SmartPtr<IViewport>> getViewports() const override;

            /**
             * @brief Gets the underlying Ogre render window.
             * @return Pointer to the Ogre::RenderWindow instance.
             */
            Ogre::RenderWindow *getWindow() const;

            /**
             * @brief Sets the underlying Ogre render window.
             * @param window Pointer to the Ogre::RenderWindow to use.
             */
            void setWindow( Ogre::RenderWindow *window );

            /**
             * @brief Gets the window size as a 2D vector.
             * @return The window size (width, height) in pixels.
             */
            Vector2I getSize() const override;

            /**
             * @brief Sets the window size.
             * @param size The new size (width, height) in pixels.
             */
            void setSize( const Vector2I &size ) override;

            /**
             * @brief Sets the color depth of the window.
             * @param colourDepth The color depth to set in bits per pixel (e.g., 16, 24, 32).
             */
            void setColourDepth( u32 colourDepth ) override;

            /**
             * @brief Gets the native window handle as a string.
             *
             * The format of this string is platform-dependent.
             * @return String representation of the window handle.
             */
            String getWindowHandleAsString() const override;

            /**
             * @brief Sets the window handle from a string representation.
             * @param handle The string containing the platform-specific window handle.
             */
            void setWindowHandleAsString( const String &handle ) override;

            /**
             * @brief Gets the native window handle as a void pointer.
             * @param[out] pData Pointer to receive the window handle (HWND on Windows, etc.).
             */
            void getWindowHandle( void *pData ) override;

            /**
             * @brief Gets the native device handle.
             * @param[out] pData Pointer to receive the device handle (HDC on Windows, etc.).
             */
            void getDeviceHandle( void *pData ) override;

            /**
             * @brief Adds a window event listener.
             * @param listener The listener to add for window events.
             */
            void addListener( SmartPtr<IGraphicsWindowListener> listener ) override;

            /**
             * @brief Removes a window event listener.
             * @param listener The listener to remove.
             */
            void removeListener( SmartPtr<IGraphicsWindowListener> listener ) override;

            /**
             * @brief Gets all registered window listeners.
             * @return Array of smart pointers to IGraphicsWindowListener objects.
             */
            Array<SmartPtr<IGraphicsWindowListener>> getListeners() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            ///< Pointer to the underlying Ogre render window.
            Ogre::RenderWindow *m_window = nullptr;

            ///< Ogre window event listener instance.
            Ogre::WindowEventListener *m_windowListener = nullptr;

            ///< Platform-specific Windows window wrapper.
            WindowWin32 *m_windowWin32 = nullptr;

            ///< Platform-specific macOS window wrapper.
            WindowMacOS *m_osWindow = nullptr;

            ///< Color depth in bits per pixel.
            u32 m_colourDepth = 0;

            ///< Flag indicating whether the window is in fullscreen mode.
            bool m_isFullscreen = false;

            ///< String representation of the native window handle.
            String m_windowHandle;

            ///< Collection of registered window event listeners.
            ConcurrentArray<SmartPtr<IGraphicsWindowListener>> m_listeners;

            ///< Collection of viewports attached to this window.
            ConcurrentArray<SmartPtr<IViewport>> m_viewports;
        };
    }  // end namespace render
}  // namespace workphone

#endif
