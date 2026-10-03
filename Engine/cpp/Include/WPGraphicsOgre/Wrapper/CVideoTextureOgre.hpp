#ifndef WPOgreVideoTexture_h__
#define WPOgreVideoTexture_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IVideoTexture.hpp>
#include <Workphone/Graphics/ResourceGraphics.hpp>
#include <OgreTexture.h>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Video texture implementation using Ogre.
         *
         * This class wraps an Ogre GPU texture and exposes the IVideoTexture
         * interface used by the engine. It is responsible for creating and
         * maintaining the underlying Ogre::Texture, copying video frames into
         * the GPU texture and providing access to texture handles required by
         * the rendering pipeline.
         *
         * Threading:
         * - Many methods touch GPU resources and should be called from the
         *   rendering thread or synchronized with the renderer.
         *
         * Lifetime:
         * - Call `initialise` to create resources.
         * - Use `load`/`unload`/`reload` to manage GPU resource lifetime.
         */
        class CVideoTextureOgre : public ResourceGraphics<IVideoTexture>
        {
        public:
            /**
             * @brief Construct a new CVideoTextureOgre instance.
             *
             * Does not allocate GPU resources. Call `initialise` / `load` to
             * create the underlying Ogre texture.
             */
            CVideoTextureOgre();

            /**
             * @brief Destructor.
             *
             * Releases held references to GPU resources. Ensure the texture is
             * unloaded on the correct thread if required by the graphics API.
             */
            ~CVideoTextureOgre() override;

            /**
             * @brief Initialise the video texture.
             *
             * Prepares internal state for a texture with the given name and
             * logical size. This does not necessarily allocate the final GPU
             * texture (see `load`).
             *
             * @param name Friendly resource name used by texture manager.
             * @param size Logical pixel dimensions of the video frames.
             */
            void initialise( const String &name, const Vector2I &size ) override;

            /**
             * @brief Update the texture with the latest frame.
             *
             * Performs any per-frame updates required to upload new video
             * data to the GPU texture. Should be called each frame if the
             * video is playing.
             */
            void update() override;

            /**
             * @brief Copy raw frame data into the internal texture.
             *
             * Internal helper that uploads a single frame's pixel data into
             * the GPU texture. The pixel format and stride are expected to
             * match the implementation's requirements.
             *
             * @param frameData Pointer to raw pixel data.
             * @param size Pixel dimensions of the provided frame.
             */
            void _copyFrameData( void *frameData, const Vector2I &size );

            /**
             * @brief Retrieve the underlying native object.
             *
             * Non-const overload: returns the raw underlying object pointer
             * (for example an Ogre texture pointer). The returned pointer is
             * written into `object`.
             *
             * @param object Output pointer to receive the native object.
             */
            void _getObject( void **object );

            /**
             * @brief Retrieve the underlying native object (const).
             *
             * Const overload. Use when you only need a read-only reference to
             * the native texture object.
             *
             * @param ppObject Output pointer to receive the native object.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Get the logical size of the video texture (in pixels).
             *
             * Logical size is the frame resolution requested/expected by the
             * producer of the video frames (may differ from GPU texture size
             * when the hardware requires power-of-two textures).
             *
             * @return Vector2I Logical width/height in pixels.
             */
            Vector2I getSize() const override;

            /**
             * @brief Set the logical video frame size.
             *
             * Changing the logical size may require recreating or resizing the
             * GPU texture. Callers should follow up with `load`/`reload` as
             * needed.
             *
             * @param size New logical size in pixels.
             */
            void setSize( const Vector2I &size ) override;

            /**
             * @brief Get the render target associated with this texture.
             *
             * If the texture is used as a render target (render-to-texture),
             * this returns the render target interface; otherwise returns a
             * null smart pointer.
             *
             * @return SmartPtr<IRenderTarget> Render target or null.
             */
            SmartPtr<IRenderTarget> getRenderTarget() const override;

            /**
             * @brief Associate a render target with this video texture.
             *
             * Setting a render target indicates that this texture should be
             * used as the destination for rendering operations.
             *
             * @param rt Render target to associate (may be null).
             */
            void setRenderTarget( SmartPtr<IRenderTarget> rt ) override;

            /**
             * @brief Copy this video's current contents into another texture.
             *
             * Performs a GPU-side copy where possible. The target texture must
             * be compatible (format/size) or the implementation will perform
             * appropriate conversions or scaling.
             *
             * @param target Destination ITexture to receive the copied pixels.
             */
            void copyToTexture( SmartPtr<ITexture> &target ) override;

            /**
             * @brief Copy raw pixel data into this texture.
             *
             * Public wrapper for uploading a block of pixel data. This is used
             * when frames are supplied by external decoders (e.g. ffmpeg).
             *
             * @param data Pointer to pixel data to copy.
             * @param size Pixel dimensions of the provided data.
             */
            void copyData( void *data, const Vector2I &size ) override;

            /**
             * @brief Get the actual GPU texture size.
             *
             * This returns the real size allocated on the GPU. It may be
             * larger than `getSize()` if the GPU requires texture dimensions
             * to be power-of-two or if padding was added for alignment.
             *
             * @return Vector2I Actual GPU texture width/height in pixels.
             */
            Vector2I getActualSize() const override;

            /**
             * @brief Get access to the underlying GPU texture pointer.
             *
             * Writes the native GPU texture handle/object into `ppTexture`.
             * The concrete type depends on the graphics API (for Ogre this is
             * typically an `Ogre::Texture*` or `Ogre::TexturePtr` raw pointer).
             *
             * @param ppTexture Output pointer to receive the GPU texture object.
             */
            void getTextureGPU( void **ppTexture ) const override;

            /**
             * @brief Get the final texture object used by the renderer.
             *
             * The "final" texture may differ from the internal GPU texture (for
             * example if post-processing or platform-specific wrappers are
             * present). Returns the API-specific final texture object.
             *
             * @param ppTexture Output pointer to receive the final texture object.
             */
            void getTextureFinal( void **ppTexture ) const override;

            /**
             * @copydoc ITexture::getTextureHandle
             *
             * Returns a platform-independent texture handle (size_t) that can
             * be used for lookups or comparisons inside the engine.
             *
             * @return size_t Texture handle/identifier.
             */
            size_t getTextureHandle() const override;

            /**
             * @brief Get usage flags for the texture.
             *
             * Usage flags indicate how the texture is used (static, dynamic,
             * render target, streaming, etc.). The exact flag values are
             * defined elsewhere in the engine.
             *
             * @return u32 Current usage flags bitmask.
             */
            u32 getUsageFlags() const override;

            /**
             * @brief Set usage flags for the texture.
             *
             * Changing usage flags may affect allocation strategy and allowed
             * operations. Callers should ensure flags are set before `load`
             * to have effect on allocation.
             *
             * @param usageFlags Bitmask of usage flags.
             */
            void setUsageFlags( u32 usageFlags ) override;

            /**
             * @brief Allocate or prepare GPU resources for this texture.
             *
             * Creates or uploads the underlying Ogre texture according to the
             * current size and usage flags. Should be called on the rendering
             * thread or with proper synchronization.
             */
            void load();

            /**
             * @brief Reload the GPU texture resource.
             *
             * Releases and re-creates GPU resources. Useful when the graphics
             * device was lost or when texture parameters change.
             */
            void reload();

            /**
             * @brief Unload and free GPU resources associated with this texture.
             *
             * After calling unload the texture is no longer valid for rendering
             * until `load` is called again.
             */
            void unload();

            /**
             * @brief Save the texture resource to persistent storage.
             *
             * Implementation may write texture meta-data or contents to disk.
             * If saving pixel data is not supported by the backend this may be
             * a no-op.
             */
            void save() override;

            /**
             * @brief Get the file system UUID associated with this resource.
             *
             * Returns an identifier used by the engine's resource/file system
             * to track where the texture originates.
             *
             * @return UUID Resource file system id.
             */
            UUID getFileSystemId() const override;

            /**
             * @brief Set the file system UUID for this resource.
             *
             * Used by the resource manager to set or change the origin id.
             *
             * @param id New file system UUID.
             */
            void setFileSystemId( UUID id ) override;

            /**
             * @brief Get custom properties associated with this texture.
             *
             * Properties may store metadata such as encoding, color-space,
             * or producer-specific settings.
             *
             * @return SmartPtr<Properties> Properties object (may be null).
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Set custom properties for this texture.
             *
             * @param properties Properties object owning metadata for this texture.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

        protected:
            /// Ogre texture pointer that backs this video texture.
            Ogre::TexturePtr m_texture;

            /// Logical frame size (width, height) in pixels.
            Vector2I m_size;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // WPOgreVideoTexture_h__
