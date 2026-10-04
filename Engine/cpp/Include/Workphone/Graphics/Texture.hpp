#ifndef _CTexture_H
#define _CTexture_H

#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Graphics/ResourceGraphics.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class Texture
         * @brief Representation of a GPU texture resource.
         *
         * The `Texture` class wraps a graphics API texture resource and exposes
         * loading, saving and runtime manipulation operations. It derives from
         * `ResourceGraphics<ITexture>` and implements the engine's texture
         * interface so textures can be managed by the resource system and used
         * by render code.
         */
        class WPCore_API Texture : public ResourceGraphics<ITexture>
        {
        public:
            static const String sizeStr;

            class WPCore_API StateListener : public IStateListener
            {
            public:
                /**
                 * @brief Default constructor.
                 *
                 * Initializes an empty listener which can be later attached to a
                 * `Texture` instance via `setOwner`.
                 */
                StateListener();

                /**
                 * @brief Destructor.
                 */
                ~StateListener() override;

                /**
                 * @brief Handle a state message targeted to the texture.
                 * @param message Smart pointer to the incoming state message.
                 * @return True if the message was handled, false otherwise.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Called when the observed state object changes.
                 * @param state Smart pointer to the new state object.
                 * @return True if the change was handled, false otherwise.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Get the texture that owns this listener.
                 * @return Smart pointer to the owning `Texture`, or null if none.
                 */
                SmartPtr<Texture> getOwner() const;

                /**
                 * @brief Set the texture that owns this listener.
                 * @param owner Smart pointer to the owning `Texture`.
                 */
                void setOwner( SmartPtr<Texture> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                /**
                 * Weak pointer back to the owning texture to avoid reference cycles.
                 */
                AtomicWeakPtr<Texture> m_owner;
            };

            /**
             * @brief Construct an empty Texture object.
             *
             * The actual GPU resource is created/initialized by the concrete
             * graphics implementation when the texture is loaded or used.
             */
            Texture();

            /**
             * @brief Virtual destructor.
             */
            ~Texture() override;

            /**
             * @brief Perform per-frame or deferred updates for the texture.
             *
             * Implementations can upload pending data, regenerate mipmaps or
             * perform other maintenance tasks.
             */
            void update() override;

            /**
             * @brief Persist texture data to its backing store (resource system).
             */
            void save() override;

            /**
             * @brief Save the texture content to a file on disk.
             * @param filePath File system path where the texture will be written.
             */
            void saveToFile( const String &filePath ) override;

            /**
             * @brief Load texture content from a file on disk.
             * @param filePath File system path to read the texture from.
             */
            void loadFromFile( const String &filePath ) override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::reload */
            void reload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Return the render target associated with this texture, if any.
             * @return SmartPtr to `IRenderTarget` or null when not a render target.
             */
            SmartPtr<IRenderTarget> getRenderTarget() const override;

            /**
             * @brief Associate a render target object with this texture.
             * @param rt SmartPtr to the render target implementation.
             */
            void setRenderTarget( SmartPtr<IRenderTarget> rt ) override;

            /**
             * @brief Copy the contents of this texture into another texture.
             * @param target Reference to a SmartPtr that will receive the copied texture.
             */
            void copyToTexture( SmartPtr<ITexture> &target ) override;

            /**
             * @brief Read texture pixel data into a CPU buffer.
             * @param data Destination buffer pointer. Must be large enough for the region.
             * @param size Size (width,height) of the region to copy.
             */
            void copyData( void *data, const Vector2I &size ) override;

            /**
             * @brief Get the logical size (width,height) of the texture resource.
             * @return Texture dimensions in texels.
             */
            Vector2I getSize() const override;

            /**
             * @brief Set the logical size for the texture resource.
             * @param size New texture dimensions in texels.
             */
            void setSize( const Vector2I &size ) override;

            /** @copydoc ResourceGraphics<ITexture>::_getObject */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Get the actual underlying size of the GPU resource.
             *
             * This can differ from `getSize()` due to alignment, padding or
             * API-specific restrictions.
             * @return Actual GPU texture dimensions in texels.
             */
            Vector2I getActualSize() const override;

            /**
             * @brief Get a raw pointer to the GPU texture object.
             * @param ppTexture Out pointer to receive the API-specific texture handle.
             */
            void getTextureGPU( void **ppTexture ) const override;

            /**
             * @brief Get the final GPU texture object used for rendering.
             * @param ppTexture Out pointer to receive the API-specific final texture.
             * @note Concrete type depends on the active graphics backend.
             */
            void getTextureFinal( void **ppTexture ) const override;

            /** @copydoc ITexture::getTextureHandle */
            size_t getTextureHandle() const override;

            /** @copydoc ITexture::getUsageFlags */
            u32 getUsageFlags() const override;

            /** @copydoc ITexture::setUsageFlags */
            void setUsageFlags( u32 usageFlags ) override;

            /** @copydoc ITexture::fromData */
            void fromData( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ITexture::toData */
            SmartPtr<ISharedObject> toData() const override;

            /** @copydoc IResource::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IResource::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc IResource::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Get the texture dimensionality/type.
             * @return Enum value describing whether the texture is 2D, 3D, Cube, etc.
             */
            TextureType getTextureType() const;

            /**
             * @brief Set the texture dimensionality/type.
             * @param textureType Enum value describing the texture type to set.
             */
            void setTextureType( TextureType textureType );

            /**
             * @brief Handle a state message targeted to the texture.
             * @param message Smart pointer to the incoming state message.
             * @return True if the message was handled, false otherwise.
             */
            virtual bool handleStateMessage( const SmartPtr<IStateMessage> &message );

            /**
             * @brief Called when the observed state object changes.
             * @param state Smart pointer to the new state object.
             * @return True if the change was handled, false otherwise.
             */
            virtual bool handleStateChanged( SmartPtr<IState> &state );

            WP_CLASS_REGISTER_DECL;

        protected:
            void createStateObject() override;

            mutable atomic_u64 m_textureHandle;

            mutable u32 m_textureId = 0;

            u32 m_usageFlags = 0;

            TextureType m_textureType = TextureType::TEX_TYPE_2D;

            AtomicSmartPtr<IRenderTarget> m_renderTarget;

            static u32 m_ext;
        };
    }  // end namespace render
}  // namespace workphone

#endif
