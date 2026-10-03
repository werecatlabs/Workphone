#ifndef _IRenderTarget_H
#define _IRenderTarget_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Interface for managing a render target, which is a destination for rendering
         * operations.
         *
         * A render target represents a surface where graphics can be rendered to. This can be a window,
         * a texture, or any other surface that can receive rendered content. The interface provides
         * functionality for managing viewports, buffer swapping, and rendering statistics.
         *
         * @note This class inherits from ISharedObject for memory management.
         */
        class WPCore_API IRenderTarget : public ISharedObject
        {
        public:
            /** @brief Flag indicating the render target is in fullscreen mode */
            static const u8 fullscreenFlag;

            /** @brief Flag indicating the render target is currently active */
            static const u8 activeFlag;

            /** @brief Flag indicating the render target is automatically updated */
            static const u8 autoupdatedFlag;

            /**
             * @brief Structure containing statistics about the render target's performance.
             *
             * This structure tracks various performance metrics of the render target,
             * including frame rates, frame times, and rendering statistics.
             */
            struct RenderTargetStats
            {
                f32 lastFPS = 0.0f;   ///< The last measured FPS (frames per second)
                f32 avgFPS = 0.0f;    ///< The average FPS since the last reset
                f32 bestFPS = 0.0f;   ///< The best (highest) FPS since the last reset
                f32 worstFPS = 0.0f;  ///< The worst (lowest) FPS since the last reset
                u32 bestFrameTime =
                    0;  ///< The best (shortest) frame time in milliseconds since the last reset
                u32 worstFrameTime =
                    0;  ///< The worst (longest) frame time in milliseconds since the last reset
                u32 triangleCount = 0;  ///< The total number of triangles rendered since the last reset
                u32 batchCount = 0;     ///< The total number of render batches since the last reset
            };

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup of derived class resources.
             */
            ~IRenderTarget() override;

            /**
             * @brief Swaps the front and back buffers of the render target.
             *
             * This function is typically called at the end of a frame to display
             * the rendered content and prepare for the next frame.
             */
            virtual void swapBuffers() = 0;

            /**
             * @brief Sets the priority of the render target.
             *
             * Higher priority render targets are updated before lower priority ones.
             *
             * @param priority The priority level to set (0-255)
             */
            virtual void setPriority( u8 priority ) = 0;

            /**
             * @brief Gets the current priority of the render target.
             *
             * @return The current priority level (0-255)
             */
            virtual u8 getPriority() const = 0;

            /**
             * @brief Checks if the render target is currently active.
             *
             * @return true if the render target is active, false otherwise
             */
            virtual bool isActive() const = 0;

            /**
             * @brief Sets the active state of the render target.
             *
             * @param state true to activate the render target, false to deactivate it
             */
            virtual void setActive( bool state ) = 0;

            /**
             * @brief Sets whether the render target should be automatically updated.
             *
             * @param autoupdate true to enable automatic updates, false to disable
             */
            virtual void setAutoUpdated( bool autoupdate ) = 0;

            /**
             * @brief Checks if the render target is automatically updated.
             *
             * @return true if automatic updates are enabled, false otherwise
             */
            virtual bool isAutoUpdated() const = 0;

            /**
             * @brief Copies the current contents of the render target to memory.
             *
             * @param buffer Pointer to the destination memory buffer
             * @param size Size of the destination buffer in bytes
             * @param bufferId The frame buffer to copy from (defaults to Auto)
             */
            virtual void copyContentsToMemory( void *buffer, u32 size,
                                               FrameBuffer bufferId = FrameBuffer::Auto ) = 0;

            /**
             * @brief Gets the current size of the render target.
             *
             * @return Vector2I containing the width and height of the render target
             */
            virtual Vector2I getSize() const = 0;

            /**
             * @brief Sets the size of the render target.
             *
             * @param size Vector2I containing the desired width and height
             */
            virtual void setSize( const Vector2I &size ) = 0;

            /**
             * @brief Gets the color depth of the render target.
             *
             * @return The color depth in bits per pixel
             */
            virtual u32 getColourDepth() const = 0;

            /**
             * @brief Sets the color depth of the render target.
             *
             * @param colourDepth The desired color depth in bits per pixel
             */
            virtual void setColourDepth( u32 colourDepth ) = 0;

            /**
             * @brief Adds a new viewport to the render target.
             *
             * @param id Unique identifier for the viewport
             * @param camera Camera to be used by the viewport
             * @param ZOrder Rendering order of the viewport (lower values render first)
             * @param left Left position of the viewport (0.0 to 1.0)
             * @param top Top position of the viewport (0.0 to 1.0)
             * @param width Width of the viewport (0.0 to 1.0)
             * @param height Height of the viewport (0.0 to 1.0)
             * @return SmartPtr to the newly created viewport
             */
            virtual SmartPtr<IViewport> addViewport( hash_type id, SmartPtr<IGraphicsCamera> camera,
                                                     s32 ZOrder = -1, f32 left = 0.0f, f32 top = 0.0f,
                                                     f32 width = 1.0f, f32 height = 1.0f ) = 0;

            /**
             * @brief Gets the number of viewports attached to this render target.
             *
             * @return The number of viewports
             */
            virtual u32 getNumViewports() const = 0;

            /**
             * @brief Gets a viewport by its index.
             *
             * @param index The index of the viewport to retrieve
             * @return SmartPtr to the requested viewport
             */
            virtual SmartPtr<IViewport> getViewport( u32 index ) = 0;

            /**
             * @brief Gets a viewport by its unique identifier.
             *
             * @param id The hash identifier of the viewport to retrieve
             * @return SmartPtr to the requested viewport
             */
            virtual SmartPtr<IViewport> getViewportById( hash_type id ) = 0;

            /**
             * @brief Gets a viewport by its Z-order.
             *
             * @param zorder The Z-order of the viewport to retrieve
             * @return SmartPtr to the requested viewport
             */
            virtual SmartPtr<IViewport> getViewportByZOrder( s32 zorder ) const = 0;

            /**
             * @brief Checks if a viewport exists with the specified Z-order.
             *
             * @param zorder The Z-order to check for
             * @return true if a viewport exists with the given Z-order, false otherwise
             */
            virtual bool hasViewportWithZOrder( s32 zorder ) const = 0;

            /**
             * @brief Gets all viewports attached to this render target.
             *
             * @return Array of SmartPtrs to all viewports
             */
            virtual Array<SmartPtr<IViewport>> getViewports() const = 0;

            /**
             * @brief Removes a specific viewport from the render target.
             *
             * @param vp SmartPtr to the viewport to remove
             */
            virtual void removeViewport( SmartPtr<IViewport> vp ) = 0;

            /**
             * @brief Removes all viewports from the render target.
             */
            virtual void removeAllViewports() = 0;

            /**
             * @brief Gets the current statistics for the render target.
             *
             * @return RenderTargetStats structure containing current performance metrics
             */
            virtual RenderTargetStats getRenderTargetStats() const = 0;

            /**
             * @brief Gets a pointer to the underlying graphics object.
             *
             * @param ppObject Pointer to a void pointer that will be set to the underlying object
             */
            virtual void _getObject( void **ppObject ) const = 0;

            /**
             * @brief Handles state messages for the render target.
             *
             * @param message The state message to handle
             * @return true if the message was handled successfully, false otherwise
             */
            virtual bool handleStateMessage( const SmartPtr<IStateMessage> &message ) = 0;

            /**
             * @brief Handles state changes for the render target.
             *
             * @param state The new state to apply
             * @return true if the state change was handled successfully, false otherwise
             */
            virtual bool handleStateChanged( SmartPtr<IState> &state ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif
