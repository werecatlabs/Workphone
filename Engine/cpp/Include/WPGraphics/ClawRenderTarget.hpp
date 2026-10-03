#ifndef ClawRenderTarget_h__
#define ClawRenderTarget_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IRenderTarget.hpp>

struct wp_renderer;
struct wp_render_texture_dx11;

namespace workphone
{
    namespace render
    {
        /**
         * @class ClawRenderTarget
         * @brief Production-ready render target implementation for the Claw graphics backend.
         *
         * ClawRenderTarget implements the full @c IRenderTarget interface with defensive
         * null-checking, error logging via @c WP_LOG_* macros, and data-driven property
         * exposure through @c getProperties / @c setProperties for the game editor.
         *
         * Viewports are managed through @c ClawViewport instances. Operations that require
         * a native backend (such as @c swapBuffers and @c copyContentsToMemory) are
         * stubbed with warning logs and can be extended when the C renderer API is wired.
         *
         * @see IRenderTarget
         * @see ClawViewport
         */
        class WPGraphics_API ClawRenderTarget : public IRenderTarget
        {
        public:
            /** @brief Property name for the render target size. */
            static const String sizeStr;
            /** @brief Property name for the colour depth. */
            static const String colourDepthStr;
            /** @brief Property name for the priority. */
            static const String priorityStr;
            /** @brief Property name for the active state. */
            static const String activeStr;
            /** @brief Property name for the auto-update state. */
            static const String autoUpdatedStr;

            /**
             * @brief Default constructor.
             *
             * Initializes the render target with a zero size, default colour depth,
             * and active/auto-update enabled.
             */
            ClawRenderTarget();

            /**
             * @brief Destructor.
             *
             * Removes all viewports and logs any remaining state.
             */
            ~ClawRenderTarget() override;

            ClawRenderTarget( const ClawRenderTarget & ) = delete;
            ClawRenderTarget &operator=( const ClawRenderTarget & ) = delete;

            /** @brief Swaps front and back buffers. Logs a warning (not yet wired to C API). */
            void swapBuffers() override;

            /** @brief Sets the priority (0–255). */
            void setPriority( u8 priority ) override;
            /** @brief Gets the current priority. */
            u8 getPriority() const override;

            /** @brief Returns whether the render target is active. */
            bool isActive() const override;
            /** @brief Sets the active state. */
            void setActive( bool state ) override;

            /** @brief Sets whether the target should auto-update. */
            void setAutoUpdated( bool autoupdate ) override;
            /** @brief Returns whether the target auto-updates. */
            bool isAutoUpdated() const override;

            /** @brief Copies the current contents to memory. Logs a warning (stub). */
            void copyContentsToMemory( void *buffer, u32 size,
                                       FrameBuffer bufferId = FrameBuffer::Auto ) override;

            /** @brief Gets the current size. */
            Vector2I getSize() const override;
            /** @brief Sets the size. */
            void setSize( const Vector2I &size ) override;

            /** @brief Gets the colour depth in bits per pixel. */
            u32 getColourDepth() const override;
            /** @brief Sets the colour depth in bits per pixel. */
            void setColourDepth( u32 colourDepth ) override;

            /** @brief Adds a viewport to the render target. */
            SmartPtr<IViewport> addViewport( hash_type id, SmartPtr<IGraphicsCamera> camera,
                                             s32 ZOrder = -1, f32 left = 0.0f, f32 top = 0.0f,
                                             f32 width = 1.0f, f32 height = 1.0f ) override;

            /** @brief Returns the number of viewports. */
            u32 getNumViewports() const override;
            /** @brief Gets a viewport by index. */
            SmartPtr<IViewport> getViewport( u32 index ) override;
            /** @brief Gets a viewport by its unique id. */
            SmartPtr<IViewport> getViewportById( hash_type id ) override;
            /** @brief Gets a viewport by Z-order. */
            SmartPtr<IViewport> getViewportByZOrder( s32 zorder ) const override;
            /** @brief Checks if a viewport exists with the given Z-order. */
            bool hasViewportWithZOrder( s32 zorder ) const override;
            /** @brief Gets all viewports. */
            Array<SmartPtr<IViewport>> getViewports() const override;
            /** @brief Removes a specific viewport. */
            void removeViewport( SmartPtr<IViewport> vp ) override;
            /** @brief Removes all viewports. */
            void removeAllViewports() override;

            /** @brief Gets current rendering statistics. */
            RenderTargetStats getRenderTargetStats() const override;

            /** @brief Retrieves the underlying raw object pointer. */
            void _getObject( void **ppObject ) const override;

            wp_render_texture_dx11 *getNativeRenderTexture( wp_renderer *renderer );
            void *getNativeTextureResource() const;
            void *getNativeTextureView() const;

            /** @brief Handles state messages. */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
            /** @brief Handles state changes. */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            /**
             * @brief Retrieves render target properties for the game editor.
             *
             * Calls the base class @c getProperties, then augments with
             * size, colour depth, priority, active, and auto-update state.
             *
             * @return Smart pointer to a Properties object, or null on failure.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Applies editor-supplied properties to the render target.
             *
             * @param properties Smart pointer to a Properties object.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            void releaseNativeRenderTexture();

            Vector2I m_size;                         ///< Dimensions of the render target.
            u32 m_colourDepth = 32;                  ///< Colour depth in bits per pixel.
            u8 m_priority = 0;                       ///< Priority (0–255).
            bool m_active = true;                    ///< Whether the target is active.
            bool m_autoUpdated = true;               ///< Whether the target auto-updates.
            RenderTargetStats m_stats;               ///< Cached rendering statistics.
            Array<SmartPtr<IViewport>> m_viewports;  ///< Attached viewports.
            wp_render_texture_dx11 *m_nativeRenderTexture = nullptr;
            void *m_nativeDevice = nullptr;
            Vector2I m_nativeSize;
        };
    }  // namespace render
}  // namespace workphone

#endif  // ClawRenderTarget_h__
