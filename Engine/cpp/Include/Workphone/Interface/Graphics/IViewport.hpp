#ifndef _IViewport_H
#define _IViewport_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/ColourUtil.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * Interface for a viewport that represents a region on a render target. A viewport is
         * essentially a camera view into the 3D world, and a region on the screen where the rendered
         * scene is displayed. This class is an extension of the `ISharedObject` interface.
         */
        class WPCore_API IViewport : public ISharedObject
        {
        public:
            static const u32 overlaysEnabledFlag;
            static const u32 skiesEnabledFlag;
            static const u32 activeFlag;
            static const u32 shadowsEnabledFlag;
            static const u32 autoUpdatedFlag;
            static const u32 enableUIFlag;
            static const u32 clearFlag;
            static const u32 enableSceneRenderFlag;

            /** Virtual destructor. */
            ~IViewport() override;

            /**
             * Sets the camera to use for rendering to this viewport.
             * @param camera The camera to use for rendering to this viewport.
             */
            virtual void setCamera( SmartPtr<IGraphicsCamera> camera ) = 0;

            /**
             * Retrieves a pointer to the camera for this viewport.
             * @return A pointer to the camera for this viewport.
             */
            virtual SmartPtr<IGraphicsCamera> getCamera() const = 0;

            virtual hash_type getViewportId() const = 0;
            virtual void setViewportId( hash_type id ) = 0;

            /**
             * Gets the Z-Order of this viewport, which is used to determine the order in which
             * multiple viewports are rendered.
             * @return The Z-Order of this viewport.
             */
            virtual s32 getZOrder() const = 0;

            /**
             * Sets the Z-Order of this viewport, which is used to determine the order in which
             * multiple viewports are rendered.
             * @param zorder The Z-Order to set.
             */
            virtual void setZOrder( s32 zorder ) = 0;

            /**
             * Gets the position of the viewport, which is expressed as a value between 0.0 and 1.0.
             * @return The position of the viewport.
             */
            virtual Vector2<real_Num> getPosition() const = 0;

            /** Gets the actual position of the viewport */
            virtual Vector2<real_Num> getActualPosition() const = 0;

            /**
             * Sets the position of the viewport, which is expressed as a value between 0.0 and 1.0.
             * @param position The position to set.
             */
            virtual void setPosition( const Vector2<real_Num> &position ) = 0;

            /**
             * Gets the size of the viewport, which is expressed as a value between 0.0 and 1.0.
             * @return The size of the viewport.
             */
            virtual Vector2<real_Num> getSize() const = 0;

            /** Gets the actual size of the viewport */
            virtual Vector2<real_Num> getActualSize() const = 0;

            /**
             * Sets the size of the viewport, which is expressed as a value between 0.0 and 1.0.
             * @param size The size to set.
             */
            virtual void setSize( const Vector2<real_Num> &size ) = 0;

            /**
             * Sets the initial background colour of the viewport (before rendering).
             *
             * @param colour The new background colour of the viewport.
             */
            virtual void setBackgroundColour( const ColourF &colour ) = 0;

            /**
             * Gets the background colour of the viewport.
             *
             * @return The background colour of the viewport.
             */
            virtual ColourF getBackgroundColour() const = 0;

            /**
             * Determines whether to clear the viewport before rendering.
             *
             * @param clear Whether to clear the viewport.
             * @param buffers Which buffers to clear (colour, depth, stencil).
             */
            virtual void setClearEveryFrame( bool clear, u32 buffers = IGraphicsScene::FBT_COLOUR |
                                                                       IGraphicsScene::FBT_DEPTH ) = 0;

            /**
             * Determines if the viewport is cleared before every frame.
             *
             * @return True if the viewport is cleared before every frame; false otherwise.
             */
            virtual bool getClearEveryFrame() const = 0;

            /**
             * Gets which buffers are to be cleared each frame.
             *
             * @return Which buffers are to be cleared each frame.
             */
            virtual u32 getClearBuffers() const = 0;

            /** Set the material scheme which the viewport should use.
             *  @param schemeName The name of the material scheme to set.
             */
            virtual void setMaterialScheme( const String &schemeName ) = 0;

            /** Get the material scheme which the viewport should use.
             *  @return The name of the material scheme currently in use.
             */
            virtual String getMaterialScheme() const = 0;

            /** Sets if the scene should be rendered. */
            virtual void setEnableSceneRender( bool enabled ) = 0;

            /** Gets if the scene should be rendered. */
            virtual bool getEnableSceneRender() const = 0;

            /** To know if overlay objects should be rendered. */
            virtual void setOverlaysEnabled( bool enabled ) = 0;

            /** Gets if overlay objects should be rendered. */
            virtual bool getOverlaysEnabled() const = 0;

            /** Sets if ui objects should be rendered. */
            virtual void setEnableUI( bool enabled ) = 0;

            /** Gets if ui objects should be rendered. */
            virtual bool getEnableUI() const = 0;

            /** Tells this viewport whether it should display skies. */
            virtual void setSkiesEnabled( bool enabled ) = 0;

            /** Returns whether or not skies (created in the SceneManager) are displayed in this
            viewport. */
            virtual bool getSkiesEnabled() const = 0;

            /** Tells this viewport whether it should display shadows. */
            virtual void setShadowsEnabled( bool enabled ) = 0;

            /** Returns whether or not shadows (defined in the SceneManager) are displayed in this
            viewport. */
            virtual bool getShadowsEnabled() const = 0;

            /** Sets a per-viewport visibility mask. */
            virtual void setVisibilityMask( u32 mask ) = 0;

            /** Gets a per-viewport visibility mask. */
            virtual u32 getVisibilityMask() const = 0;

            /** Sets whether this viewport should be automatically updated
                if Ogre's rendering loop or RenderTarget::update is being used.
            */
            virtual void setAutoUpdated( bool autoupdate ) = 0;

            /** Gets whether this viewport is automatically updated if
                Ogre's rendering loop or RenderTarget::update is being used.
            */
            virtual bool isAutoUpdated() const = 0;

            /** Gets a pointer to the underlying object.
             */
            virtual void _getObject( void **ppObject ) const = 0;

            /** Gets the background texture.
             */
            virtual SmartPtr<ITexture> getBackgroundTexture() const = 0;

            /** Sets the background texture.
             */
            virtual void setBackgroundTexture( SmartPtr<ITexture> texture ) = 0;

            /** Gets the background texture name.
             */
            virtual String getBackgroundTextureName() const = 0;

            /** Sets the background texture name.
             */
            virtual void setBackgroundTextureName( const String &textureName ) = 0;

            /** Gets the render target.
             */
            virtual SmartPtr<IRenderTarget> getRenderTarget() const = 0;

            /** Sets the render target.
             */
            virtual void setRenderTarget( SmartPtr<IRenderTarget> renderTarget ) = 0;

            /** Gets if the viewport is active.
             */
            virtual bool isActive() const = 0;

            /** Sets if the viewport is active.
             */
            virtual void setActive( bool active ) = 0;

            /**
             * @brief Handles incoming state messages.
             * @param message The state message to process.
             * @return true if the message was handled successfully, false otherwise.
             * @details Currently returns false as message handling is not implemented.
             *          This method can be extended to handle specific viewport-related messages.
             */
            virtual bool handleStateMessage( const SmartPtr<IStateMessage> &message ) = 0;

            /**
             * @brief Handles state change notifications.
             * @param state The state object that has changed.
             * @return true if the state change was handled successfully, false otherwise.
             * @details Marks the state as not dirty and returns false. This method manages
             *          the viewport's internal state transitions and can be extended to
             *          perform specific actions when the viewport state changes.
             */
            virtual bool handleStateChanged( SmartPtr<IState> &state ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif
