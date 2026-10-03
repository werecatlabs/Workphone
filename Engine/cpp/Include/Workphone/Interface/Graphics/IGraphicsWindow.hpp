#ifndef _IGraphicsWindow_H
#define _IGraphicsWindow_H

#include <Workphone/Interface/Graphics/IRenderTarget.hpp>

namespace workphone
{
    namespace render
    {

        /** Manages a render window. */
        class WPCore_API IGraphicsWindow : public IRenderTarget
        {
        public:
            /** Hash identifier for resize events. */
            static const hash_type RESIZE_HASH;

            /** Hash identifier for reposition events. */
            static const hash_type REPOSITION_HASH;

            /** Hash identifier for moved or resized events. */
            static const hash_type MOVED_OR_RESIZED_HASH;

            static const u32 WINDOW_FLAG_FULLSCREEN;
            static const u32 WINDOW_FLAG_VISIBLE;
            static const u32 WINDOW_FLAG_BORDERLESS;
            static const u32 WINDOW_FLAG_RESIZABLE;
            static const u32 WINDOW_FLAG_HIDDEN;
            static const u32 WINDOW_FLAG_MAXIMIZED;
            static const u32 WINDOW_FLAG_MINIMIZED;
            static const u32 WINDOW_FLAG_NO_TASKBAR;
            static const u32 WINDOW_FLAG_ALWAYS_ON_TOP;
            static const u32 WINDOW_FLAG_DEACTIVATE_ON_FOCUS_CHANGE;
            static const u32 WINDOW_FLAG_IS_PRIMARY;
            static const u32 WINDOW_FLAG_IS_CLOSED;

            /** Virtual destructor. */
            ~IGraphicsWindow() override;

            /**
             * Handles the window event.
             *
             * @param event The window event.
             */
            virtual void handleEvent( SmartPtr<IGraphicsWindowEvent> event ) = 0;

            /** Gets the window title.
             *  @return The title of the window as a string.
             */
            virtual String getTitle() const = 0;

            /** Sets the window title.
             *  @param title The new title for the window.
             */
            virtual void setTitle( const String &title ) = 0;

            /** Alter fullscreen mode options.
             *  @note Nothing will happen unless the settings here are different from the current
             * settings.
             *  @param fullScreen Whether to use fullscreen mode or not.
             *  @param width The new width to use.
             *  @param height The new height to use.
             */
            virtual void setFullscreen( bool fullScreen, u32 width, u32 height ) = 0;

            /** Destroys the window. */
            virtual void destroy() = 0;

            /** Alter the size of the window.
             *  @param width The new width to use.
             *  @param height The new height to use.
             */
            virtual void resize( u32 width, u32 height ) = 0;

            /** Notify that the window has been resized.
             *  @remarks You don't need to call this unless you created the window externally.
             */
            virtual void windowMovedOrResized() = 0;

            /** Reposition the window.
             *  @param left The new x-coordinate of the window's top-left corner.
             *  @param top The new y-coordinate of the window's top-left corner.
             */
            virtual void reposition( s32 left, s32 top ) = 0;

            /** Gets the position of the window. */
            virtual Vector2I getPosition() const = 0;

            /** Sets the position of the window. */
            virtual void setPosition( const Vector2I &position ) = 0;

            /** Maximizes the window. */
            virtual void maximize() = 0;

            /** Indicates whether the window is visible (not minimized or obscured).
             *  @return True if the window is visible, false otherwise.
             */
            virtual bool isVisible() const = 0;

            /** Set the visibility state.
             *  @param visible True to make the window visible, false to hide it.
             */
            virtual void setVisible( bool visible ) = 0;

            /** Indicates whether the window has been closed by the user.
             *  @return True if the window is closed, false otherwise.
             */
            virtual bool isClosed() const = 0;

            /** Indicates whether the window is the primary window.
             *  @remarks The primary window is special in that it is destroyed when Ogre is shut down,
             * and cannot be destroyed directly. This is the case because it holds the context for
             * vertex, index buffers and textures.
             *  @return True if the window is the primary window, false otherwise.
             */
            virtual bool isPrimary() const = 0;

            /** Returns true if window is running in fullscreen mode.
             *  @return True if the window is running in fullscreen mode, false otherwise.
             */
            virtual bool isFullScreen() const = 0;

            /** Alter fullscreen mode options.
             *  @note Nothing will happen unless the settings here are different from the current
             * settings.
             *  @param fullscreen Whether to use fullscreen mode or not.
             */
            virtual void setFullscreen( bool fullscreen ) = 0;

            /**
             * Returns whether the window will automatically deactivate itself when losing focus.
             * @return A boolean value indicating whether the window is deactivated on focus change.
             */
            virtual bool isDeactivatedOnFocusChange() const = 0;

            /**
             * Sets whether the window will automatically deactivate itself when losing focus.
             * @param deactivate A boolean value indicating whether to deactivate the window on focus
             * change.
             */
            virtual void setDeactivateOnFocusChange( bool deactivate ) = 0;

            /**
             * Gets a custom (maybe platform-specific) attribute.
             * @param name The name of the attribute.
             * @param pData A pointer to the attribute data.
             */
            virtual void getCustomAttribute( const String &name, void *pData ) = 0;

            /**
             * Gets the window handle. This is normally platform specific.
             * @param pData A pointer to the window handle.
             */
            virtual void getWindowHandle( void *pData ) = 0;

            /**
             * Gets the device handle. This is normally platform specific.
             * @param pData A pointer to the device handle.
             */
            virtual void getDeviceHandle( void *pData ) = 0;

            /**
             * Gets the window handle as a string. This is normally platform specific.
             * @return A string representation of the window handle.
             */
            virtual String getWindowHandleAsString() const = 0;

            /**
             * Sets the window handle as a string. This is normally platform specific.
             * @param handle A string representation of the window handle.
             */
            virtual void setWindowHandleAsString( const String &handle ) = 0;

            /**
             * Adds a window listener.
             * @param listener A pointer to the window listener to be added.
             */
            virtual void addListener( SmartPtr<IGraphicsWindowListener> listener ) = 0;

            /**
             * Removes a window listener.
             * @param listener A pointer to the window listener to be removed.
             */
            virtual void removeListener( SmartPtr<IGraphicsWindowListener> listener ) = 0;

            /**
             * Gets an array of all window listeners.
             * @return An array of all window listeners.
             */
            virtual Array<SmartPtr<IGraphicsWindowListener>> getListeners() const = 0;

            virtual SmartPtr<ITexture> getTexture() const = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif
