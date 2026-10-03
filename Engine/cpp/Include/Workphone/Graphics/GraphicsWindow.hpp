#ifndef __RenderWindow_h__
#define __RenderWindow_h__

#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Graphics/RenderTarget.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Implementation of the IGraphicsWindow interface, representing a renderable window.
         *
         * This class provides a base implementation for window management, including
         * creation, destruction, resizing, event handling, and listener management.
         * It is intended to be subclassed for platform-specific windowing systems.
         */
        class WPCore_API GraphicsWindow : public RenderTarget<IGraphicsWindow>
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes a new Window instance with default parameters.
             */
            GraphicsWindow();

            /**
             * @brief Destructor.
             *
             * Cleans up resources associated with the Window instance.
             */
            ~GraphicsWindow() override;

            /**
             * @brief Loads the window with the specified data.
             * @param data Shared pointer to initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the window and releases associated resources.
             * @param data Shared pointer to data for unloading.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Handles a window event.
             * @param event Shared pointer to the window event to handle.
             */
            void handleEvent( SmartPtr<IGraphicsWindowEvent> event ) override;

            /**
             * @brief Gets the window title.
             * @return The current window title as a String.
             */
            String getTitle() const override;

            /**
             * @brief Sets the window title.
             * @param title The new title for the window.
             */
            void setTitle( const String &title ) override;

            /**
             * @brief Sets the window to fullscreen mode with the specified dimensions.
             * @param fullScreen Whether to enable fullscreen mode.
             * @param width The width of the window in pixels.
             * @param height The height of the window in pixels.
             */
            void setFullscreen( bool fullScreen, u32 width, u32 height ) override;

            /**
             * @brief Destroys the window and releases all resources.
             */
            void destroy() override;

            /**
             * @brief Resizes the window to the specified dimensions.
             * @param width The new width of the window in pixels.
             * @param height The new height of the window in pixels.
             */
            void resize( u32 width, u32 height ) override;

            /**
             * @brief Notifies the window that it has been moved or resized.
             *
             * Should be called after external window movement or resizing.
             */
            void windowMovedOrResized() override;

            /**
             * @brief Repositions the window to the specified coordinates.
             * @param left The new left position of the window.
             * @param top The new top position of the window.
             */
            void reposition( s32 left, s32 top ) override;

            /**
             * @brief Gets the current position of the window.
             * @return The position as a Vector2I (x, y).
             */
            Vector2I getPosition() const override;

            /**
             * @brief Sets the position of the window.
             * @param position The new position as a Vector2I (x, y).
             */
            void setPosition( const Vector2I &position ) override;

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
             * @brief Sets the visibility of the window.
             * @param visible True to make the window visible, false to hide it.
             */
            void setVisible( bool visible ) override;

            /**
             * @brief Checks if the window has been closed.
             * @return True if the window is closed, false otherwise.
             */
            bool isClosed() const override;

            /**
             * @brief Checks if the window is the primary window.
             * @return True if this is the primary window, false otherwise.
             */
            bool isPrimary() const override;

            /**
             * @brief Checks if the window is in fullscreen mode.
             * @return True if the window is fullscreen, false otherwise.
             */
            bool isFullScreen() const override;

            /**
             * @brief Sets the window to fullscreen or windowed mode.
             * @param fullscreen True to enable fullscreen, false for windowed mode.
             */
            void setFullscreen( bool fullscreen ) override;

            /**
             * @brief Checks if the window deactivates on focus change.
             * @return True if the window deactivates on focus change, false otherwise.
             */
            bool isDeactivatedOnFocusChange() const override;

            /**
             * @brief Sets whether the window should deactivate on focus change.
             * @param deactivate True to deactivate on focus change, false otherwise.
             */
            void setDeactivateOnFocusChange( bool deactivate ) override;

            /**
             * @brief Gets a custom attribute of the window.
             * @param name The name of the attribute.
             * @param pData Pointer to the data to receive the attribute value.
             */
            void getCustomAttribute( const String &name, void *pData ) override;

            /**
             * @brief Gets the native window handle.
             * @param pData Pointer to receive the window handle.
             */
            void getWindowHandle( void *pData ) override;

            /**
             * @brief Gets the native device handle associated with the window.
             * @param pData Pointer to receive the device handle.
             */
            void getDeviceHandle( void *pData ) override;

            /**
             * @brief Gets the window handle as a string.
             * @return The window handle as a String.
             */
            String getWindowHandleAsString() const override;

            /**
             * @brief Sets the window handle from a string value.
             * @param handle The window handle as a String.
             */
            void setWindowHandleAsString( const String &handle ) override;

            /**
             * @brief Adds a window listener to receive window events.
             * @param listener Shared pointer to the listener to add.
             */
            void addListener( SmartPtr<IGraphicsWindowListener> listener ) override;

            /**
             * @brief Removes a window listener.
             * @param listener Shared pointer to the listener to remove.
             */
            void removeListener( SmartPtr<IGraphicsWindowListener> listener ) override;

            /**
             * @brief Gets the list of window listeners.
             * @return An array of shared pointers to window listeners.
             */
            Array<SmartPtr<IGraphicsWindowListener>> getListeners() const override;

            SmartPtr<ITexture> getTexture() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Sets up the internal state object for the window.
             *
             * This method is intended to be called during initialization to prepare
             * any platform-specific or internal state required by the window.
             */
            void setupStateObject() override;

            /// List of listeners registered to receive window events.
            ConcurrentArray<SmartPtr<IGraphicsWindowListener>> m_listeners;
        };

    }  // namespace render
}  // namespace workphone

#endif  // CWindow_h__
