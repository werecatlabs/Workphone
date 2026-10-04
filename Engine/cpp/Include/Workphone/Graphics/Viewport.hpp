#ifndef __CViewport_h__
#define __CViewport_h__

#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class Viewport
         * @brief Represents a viewport in a window, managing rendering, camera, overlays, and other
         * visual properties.
         * @details The Viewport class provides an interface for controlling a rendering viewport within
         * a window. It allows configuration of camera, overlays, background, z-order, priority,
         * visibility, and other rendering options. It inherits from SharedGraphicsObject<IViewport> and
         * implements the IViewport interface.
         *
         * @note This class is part of the workphone::render namespace.
         */
        class WPCore_API Viewport : public SharedGraphicsObject<IViewport>
        {
        public:
            /**
             * @brief Default constructor.
             */
            Viewport();

            /**
             * @brief Destructor.
             */
            ~Viewport() override;

            /**
             * @brief Loads the viewport with the given data.
             * @param data Shared object containing initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the viewport and releases resources.
             * @param data Shared object containing unload data.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Sets the camera used by this viewport.
             * @param camera Smart pointer to the camera object.
             */
            void setCamera( SmartPtr<IGraphicsCamera> camera ) override;

            /**
             * @brief Gets the camera currently used by this viewport.
             * @return Smart pointer to the camera object.
             */
            SmartPtr<IGraphicsCamera> getCamera() const override;

            /**
             * @brief Gets the unique identifier for this viewport.
             * @return The unique identifier (hash) for this viewport.
             */
            hash_type getViewportId() const;

            /**
             * @brief Sets the unique identifier for this viewport.
             * @param id The unique identifier (hash) to set for this viewport.
             */
            void setViewportId( hash_type id );

            /**
             * @brief Enables or disables overlays for this viewport.
             * @param enabled True to enable overlays, false to disable.
             */
            void setOverlaysEnabled( bool enabled ) override;

            /**
             * @brief Checks if overlays are enabled for this viewport.
             * @return True if overlays are enabled, false otherwise.
             */
            bool getOverlaysEnabled() const override;

            /**
             * @brief Sets the background color of the viewport.
             * @param colour The color to set as background.
             */
            void setBackgroundColour( const ColourF &colour ) override;

            /**
             * @brief Gets the background color of the viewport.
             * @return The current background color.
             */
            ColourF getBackgroundColour() const override;

            /**
             * @brief Sets the Z-order of the viewport (rendering order).
             * @param zorder The Z-order value.
             */
            void setZOrder( s32 zorder ) override;

            /**
             * @brief Gets the Z-order of the viewport.
             * @return The Z-order value.
             */
            s32 getZOrder() const override;

            /**
             * @brief Sets the priority of the viewport.
             * @param priority The priority value (0-255).
             */
            void setPriority( u8 priority );

            /**
             * @brief Gets the priority of the viewport.
             * @return The priority value.
             */
            u8 getPriority() const;

            /**
             * @brief Checks if the viewport is active.
             * @return True if active, false otherwise.
             */
            bool isActive() const override;

            /**
             * @brief Sets the active state of the viewport.
             * @param active True to activate, false to deactivate.
             */
            void setActive( bool active ) override;

            /**
             * @brief Sets whether the viewport is auto-updated every frame.
             * @param autoUpdated True to enable auto-update, false otherwise.
             */
            void setAutoUpdated( bool autoUpdated ) override;

            /**
             * @brief Checks if the viewport is auto-updated every frame.
             * @return True if auto-updated, false otherwise.
             */
            bool isAutoUpdated() const override;

            /**
             * @brief Enables or disables sky rendering in the viewport.
             * @param enabled True to enable skies, false to disable.
             */
            void setSkiesEnabled( bool enabled ) override;

            /**
             * @brief Checks if sky rendering is enabled in the viewport.
             * @return True if skies are enabled, false otherwise.
             */
            bool getSkiesEnabled() const override;

            /**
             * @brief Sets the visibility mask for the viewport.
             * @param mask The visibility mask.
             */
            void setVisibilityMask( u32 mask ) override;

            /**
             * @brief Gets the visibility mask for the viewport.
             * @return The visibility mask.
             */
            u32 getVisibilityMask() const override;

            /**
             * @brief Enables or disables shadow rendering in the viewport.
             * @param enabled True to enable shadows, false to disable.
             */
            void setShadowsEnabled( bool enabled ) override;

            /**
             * @brief Checks if shadow rendering is enabled in the viewport.
             * @return True if shadows are enabled, false otherwise.
             */
            bool getShadowsEnabled() const override;

            /**
             * @brief Gets the position of the viewport (relative coordinates).
             * @return The position as a 2D vector.
             */
            Vector2<real_Num> getPosition() const override;

            /**
             * @brief Gets the actual position of the viewport (in pixels).
             * @return The actual position as a 2D vector.
             */
            Vector2<real_Num> getActualPosition() const override;

            /**
             * @brief Sets the position of the viewport (relative coordinates).
             * @param position The position as a 2D vector.
             */
            void setPosition( const Vector2<real_Num> &position ) override;

            /**
             * @brief Gets the size of the viewport (relative dimensions).
             * @return The size as a 2D vector.
             */
            Vector2<real_Num> getSize() const override;

            /**
             * @brief Gets the actual size of the viewport (in pixels).
             * @return The actual size as a 2D vector.
             */
            Vector2<real_Num> getActualSize() const override;

            /**
             * @brief Sets the size of the viewport (relative dimensions).
             * @param size The size as a 2D vector.
             */
            void setSize( const Vector2<real_Num> &size ) override;

            /**
             * @brief Gets the background texture of the viewport.
             * @return Smart pointer to the background texture.
             */
            SmartPtr<ITexture> getBackgroundTexture() const override;

            /**
             * @brief Sets the background texture of the viewport.
             * @param texture Smart pointer to the texture.
             */
            void setBackgroundTexture( SmartPtr<ITexture> texture ) override;

            /**
             * @brief Enables or disables UI rendering in the viewport.
             * @param enabled True to enable UI, false to disable.
             */
            void setEnableUI( bool enabled ) override;

            /**
             * @brief Checks if UI rendering is enabled in the viewport.
             * @return True if UI is enabled, false otherwise.
             */
            bool getEnableUI() const override;

            /**
             * @brief Enables or disables scene rendering in the viewport.
             * @param enabled True to enable scene rendering, false to disable.
             */
            void setEnableSceneRender( bool enabled ) override;

            /**
             * @brief Checks if scene rendering is enabled in the viewport.
             * @return True if scene rendering is enabled, false otherwise.
             */
            bool getEnableSceneRender() const override;

            /**
             * @brief Sets the material scheme for the viewport.
             * @param schemeName The name of the material scheme.
             */
            void setMaterialScheme( const String &schemeName ) override;

            /**
             * @brief Gets the material scheme used by the viewport.
             * @return The name of the material scheme.
             */
            String getMaterialScheme() const override;

            /**
             * @brief Sets whether to clear the viewport every frame and which buffers to clear.
             * @param clear True to clear every frame, false otherwise.
             * @param buffers Bitmask of buffers to clear.
             */
            void setClearEveryFrame( bool clear, u32 buffers ) override;

            /**
             * @brief Checks if the viewport is cleared every frame.
             * @return True if cleared every frame, false otherwise.
             */
            bool getClearEveryFrame() const override;

            /**
             * @brief Gets the bitmask of buffers cleared every frame.
             * @return The buffer bitmask.
             */
            u32 getClearBuffers() const override;

            /**
             * @brief Gets the window associated with this viewport.
             * @return Smart pointer to the window.
             */
            SmartPtr<IGraphicsWindow> getWindow() const;

            /**
             * @brief Sets the window associated with this viewport.
             * @param window Smart pointer to the window.
             */
            void setWindow( SmartPtr<IGraphicsWindow> window );

            /**
             * @brief Gets the name of the background texture.
             * @return The background texture name.
             */
            String getBackgroundTextureName() const override;

            /**
             * @brief Sets the name of the background texture.
             * @param textureName The background texture name.
             */
            void setBackgroundTextureName( const String &textureName ) override;

            /**
             * @brief Gets the render target associated with this viewport.
             * @return Smart pointer to the render target.
             */
            SmartPtr<IRenderTarget> getRenderTarget() const override;

            /**
             * @brief Sets the render target for this viewport.
             * @param renderTarget Smart pointer to the render target.
             */
            void setRenderTarget( SmartPtr<IRenderTarget> renderTarget ) override;

            /**
             * @brief Handles incoming state messages.
             * @param message The state message to process.
             * @return true if the message was handled successfully, false otherwise.
             * @details Currently returns false as message handling is not implemented.
             *          This method can be extended to handle specific viewport-related messages.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            /**
             * @brief Handles state change notifications.
             * @param state The state object that has changed.
             * @return true if the state change was handled successfully, false otherwise.
             * @details Marks the state as not dirty and returns false. This method manages
             *          the viewport's internal state transitions and can be extended to
             *          perform specific actions when the viewport state changes.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            /**
             * @brief Registers the class for reflection or serialization.
             */
            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Removes the viewport from its associated render target.
             * @details This is a virtual method intended for internal cleanup when detaching the
             * viewport from a render target.
             */
            virtual void removeViewportFromRT();

            /**
             * @brief Weak pointer to the render target associated with this viewport.
             */
            AtomicWeakPtr<IRenderTarget> m_renderTarget;

            /**
             * @brief Weak pointer to the window associated with this viewport.
             */
            AtomicWeakPtr<IGraphicsWindow> m_window;

            atomic_s64 m_viewportId;

            static u32 m_idExt;
        };
    }  // namespace render
}  // namespace workphone

#endif  // CViewport_h__
