#ifndef CRenderer_h__
#define CRenderer_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IRenderer.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Wrapper renderer providing a thin interface to the underlying
         *        Ogre-based rendering implementation.
         *
         * CRenderer implements the IRenderer interface used by the engine to
         * perform simple 2D/3D rendering operations (begin/end frame, draw calls,
         * render target and viewport management). This class acts as an adapter
         * between the engine's generic renderer API and the Ogre Next backend.
         */
        class CRenderer : public IRenderer
        {
        public:
            /**
             * @brief Construct a renderer instance.
             *
             * The constructor performs lightweight initialization only. Heavy
             * resource setup should occur in load().
             */
            CRenderer();

            /**
             * @brief Virtual destructor.
             *
             * Calls unload() to release any resources if still loaded.
             */
            ~CRenderer() override;

            /**
             * @brief Load renderer resources.
             *
             * Called by the owning system to create GPU resources, materials or
             * other objects required for rendering. The supplied data pointer
             * may contain initialization parameters (optional).
             *
             * @param data Optional shared object with initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload and free renderer resources.
             *
             * Releases GPU resources and breaks references so that dependent
             * systems can be torn down safely. The method is safe to call
             * multiple times.
             *
             * @param data Optional shared object (unused).
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Begin a rendering frame/sequence.
             *
             * Prepares the renderer to accept draw calls. Typical actions include
             * binding the current render target and clearing per-frame state.
             */
            void beginRender() override;

            /**
             * @brief End the current rendering frame/sequence.
             *
             * Flushes any pending draw commands and performs per-frame finalization
             * such as presenting or marking the render target for swap.
             */
            void endRender() override;

            /**
             * @brief Force submission of any buffered draw calls.
             *
             * Useful when the caller needs to make sure all queued commands are
             * executed immediately (for example before reading back GPU data).
             */
            void flush() override;

            /**
             * @brief Clear the currently bound render target.
             *
             * @param colour Colour to clear the render target with.
             */
            void clear( const ColourF &colour ) override;

            /**
             * @brief Set the active render target.
             *
             * The renderer will direct subsequent draw calls to this target.
             * Passing a nullptr will unset the current render target.
             *
             * @param renderTarget Shared pointer to the new render target.
             */
            void setRenderTarget( SmartPtr<IRenderTarget> renderTarget ) override;

            /**
             * @brief Get the currently active render target.
             *
             * @return Shared pointer to the active render target or nullptr.
             */
            SmartPtr<IRenderTarget> getRenderTarget() const override;

            /**
             * @brief Set the active viewport for rendering.
             *
             * The viewport may affect scissor/viewport transforms used by draws.
             *
             * @param viewport Shared pointer to the viewport to use.
             */
            void setViewport( SmartPtr<IViewport> viewport ) override;

            /**
             * @brief Retrieve the currently active viewport.
             *
             * @return Shared pointer to the active viewport or nullptr.
             */
            SmartPtr<IViewport> getViewport() const override;

            /**
             * @brief Render an object using a texture.
             *
             * This convenience overload issues a draw call using the supplied
             * texture and transform.
             *
             * @param renderData Generic shared object describing the mesh or
             *                   geometry to render.
             * @param texture Texture to bind for the draw.
             * @param transform World transform matrix for the object.
             * @param colour Per-object colour multiplier.
             */
            void render( const SmartPtr<ISharedObject> &renderData, const SmartPtr<ITexture> &texture,
                         const Matrix4F &transform, const ColourF &colour ) override;

            /**
             * @brief Render an object using a material.
             *
             * @param renderData Generic shared object describing the mesh or
             *                   geometry to render.
             * @param material Material to use for the draw.
             * @param transform World transform matrix for the object.
             * @param colour Per-object colour multiplier.
             */
            void render( const SmartPtr<ISharedObject> &renderData, const SmartPtr<IMaterial> &material,
                         const Matrix4F &transform, const ColourF &colour ) override;

            /**
             * @brief Retrieve a raw pointer to the underlying implementation object.
             *
             * This is primarily used by wrapper code that needs direct access to
             * the backend (for example Ogre objects). The returned pointer is
             * stored in the caller-provided void**.
             *
             * @param ppObject Output pointer location to receive the raw object
             *                 pointer.
             */
            void _getObject( void **ppObject ) override;

            /**
             * @name Thread-safety helpers
             * These methods provide a simple mutex-like API for callers that need
             * to serialize access to the renderer from multiple threads.
             */
            //@{
            /**
             * @brief Acquire the internal renderer lock. Blocks until available.
             */
            void lock();

            /**
             * @brief Try to acquire the internal renderer lock without blocking.
             *
             * @return true if the lock was acquired, false otherwise.
             */
            bool try_lock();

            /**
             * @brief Release the internal renderer lock previously acquired by
             *        lock() or try_lock().
             */
            void unlock();

            void setCamera( SmartPtr<IGraphicsCamera> camera ) override;

            SmartPtr<IGraphicsCamera> getCamera() const override;

            //@}

            WP_CLASS_REGISTER_DECL;

        protected:
        };

    }  // namespace render
}  // namespace workphone

#endif  // CRenderer_h__
