#ifndef __IRenderer_h__
#define __IRenderer_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Graphics/IRenderTarget.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/Matrix4.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * Base interface for rendering.
         *
         * Provides frame lifecycle management, render target and viewport control,
         * buffer clearing, and sprite rendering via texture or material.
         */
        class WPCore_API IRenderer : public ISharedObject
        {
        public:
            /** Virtual destructor. */
            ~IRenderer() override;

            /**
             * Begins a new render frame.
             *
             * Must be called before any render() calls within a frame.
             */
            virtual void beginRender() = 0;

            /**
             * Ends the current render frame and submits all queued draw calls.
             *
             * Must be called after all render() calls within a frame.
             */
            virtual void endRender() = 0;

            /**
             * Flushes all pending draw calls immediately.
             */
            virtual void flush() = 0;

            /**
             * Clears the current render target with the specified colour.
             *
             * @param colour    The colour to fill the render target with.
             */
            virtual void clear( const ColourF &colour ) = 0;

            /**
             * Sets the active render target.
             *
             * @param renderTarget  The render target to render into.
             */
            virtual void setRenderTarget( SmartPtr<IRenderTarget> renderTarget ) = 0;

            /**
             * Gets the active render target.
             *
             * @return The current render target, or null if none is set.
             */
            virtual SmartPtr<IRenderTarget> getRenderTarget() const = 0;

            /**
             * Sets the camera used for 3D rendering.
             *
             * @param camera    The camera to use when rendering the scene.
             */
            virtual void setCamera( SmartPtr<IGraphicsCamera> camera ) = 0;

            /**
             * Gets the camera used for 3D rendering.
             *
             * @return The current camera, or null if none is set.
             */
            virtual SmartPtr<IGraphicsCamera> getCamera() const = 0;

            /**
             * Sets the active viewport.
             *
             * @param viewport  The viewport to use for rendering.
             */
            virtual void setViewport( SmartPtr<IViewport> viewport ) = 0;

            /**
             * Gets the active viewport.
             *
             * @return The current viewport, or null if none is set.
             */
            virtual SmartPtr<IViewport> getViewport() const = 0;

            /**
             * Renders a texture as a sprite.
             *
             * @param renderData    A shared object containing additional rendering data.
             * @param texture       The texture to use for rendering.
             * @param transform     The transformation matrix for the sprite.
             * @param colour        The colour to apply to the sprite.
             */
            virtual void render( const SmartPtr<ISharedObject> &renderData,
                                 const SmartPtr<ITexture> &texture, const Matrix4F &transform,
                                 const ColourF &colour ) = 0;

            /**
             * Renders a material as a sprite.
             *
             * @param renderData    A shared object containing additional rendering data.
             * @param material      The material to use for rendering.
             * @param transform     The transformation matrix for the sprite.
             * @param colour        The colour to apply to the sprite.
             */
            virtual void render( const SmartPtr<ISharedObject> &renderData,
                                 const SmartPtr<IMaterial> &material, const Matrix4F &transform,
                                 const ColourF &colour ) = 0;

            /**
             * Gets the internal object used for rendering.
             *
             * @param ppObject  A pointer to the internal object.
             */
            virtual void _getObject( void **ppObject ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // __IRenderer_h__
