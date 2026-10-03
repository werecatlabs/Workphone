#ifndef _CTextureOgre_H
#define _CTextureOgre_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/Texture.hpp>
#include <Workphone/Interface/System/ICoroutineData.hpp>
#include <OgreTextureGpuListener.h>

namespace workphone
{
    namespace render
    {
        /**
         * \brief Ogre texture implementation.
         *
         * This class implements the engine's abstract `Texture` interface using
         * Ogre Next GPU textures. It wraps an `Ogre::TextureGpu` and provides
         * lifecycle management (load/reload/unload), size handling and access
         * to the underlying GPU objects used by the rendering backend.
         */
        class CTextureOgreNext : public Texture
        {
        public:
            /**
             * \brief Options used when loading or reloading a texture.
             *
             * Instances of this object may be passed to `load` / `reload` to
             * control finer behaviour such as whether the GPU texture object
             * should be re-created or whether viewports referencing this
             * texture should be unloaded during the operation.
             */
            class TextureLoadingOptions : public ISharedObject
            {
            public:
                /** Default constructor. Initializes options to sensible defaults. */
                TextureLoadingOptions();

                /** Virtual destructor. */
                ~TextureLoadingOptions() override;

                /** If true, the underlying GPU texture object will be reloaded. */
                bool reloadTextureObject = false;

                /** If true, viewports using this texture should be unloaded
                 *  during unload/reload operations. */
                bool unloadViewports = true;
            };

            /**
             * \brief Listener that reacts to Ogre GPU texture events.
             *
             * This listener receives notifications from Ogre when the
             * `Ogre::TextureGpu` changes (for example when the texture is
             * recreated or destroyed). It holds a weak reference to the
             * owning `CTextureOgreNext` instance to forward events safely.
             */
            class TextureGpuListener : public Ogre::TextureGpuListener
            {
            public:
                /** Default constructor. */
                TextureGpuListener();

                /** Virtual destructor. */
                ~TextureGpuListener() override;

                /**
                 * \brief Called by Ogre when the texture changes.
                 *
                 * \param texture The GPU texture that changed.
                 * \param reason  The reason for the change.
                 * \param extraData Optional extra data provided by Ogre.
                 */
                void notifyTextureChanged( Ogre::TextureGpu *texture,
                                           Ogre::TextureGpuListener::Reason reason,
                                           void *extraData ) override;

                /** Returns a strong pointer to the owning texture wrapper. */
                SmartPtr<CTextureOgreNext> getOwner() const;

                /** Sets the owning texture wrapper. */
                void setOwner( SmartPtr<CTextureOgreNext> owner );

            protected:
                /** Weak reference to the owning `CTextureOgreNext`. */
                AtomicWeakPtr<CTextureOgreNext> m_owner;
            };

            /**
             * \brief Default constructor.
             *
             * Creates an empty texture wrapper. The actual GPU texture is not
             * created until `load` or `setTexture` is called.
             */
            CTextureOgreNext();

            /**
             * \brief Construct with an associated resource manager.
             * \param resourceManager Resource manager used for loading assets.
             */
            CTextureOgreNext( IResourceManager *resourceManager );

            /** Destructor. Cleans up any references to the underlying GPU texture. */
            ~CTextureOgreNext() override;

            /** @copydoc Texture::save
             *
             * Implementation saves texture metadata/state required by the
             * engine resource system. */
            void save() override;

            /**
             * \copydoc ISharedObject::load
             *
             * Loads the texture from the provided `data` object which may be
             * of type `TextureLoadingOptions` or another resource descriptor.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * \copydoc ISharedObject::reload
             *
             * Reloads the texture, optionally recreating the GPU texture
             * object depending on the provided `data` options.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * \copydoc ISharedObject::unload
             *
             * Releases the GPU texture resource and performs any engine-side
             * cleanup. `data` may influence whether viewports are unloaded.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** \copydoc Texture::copyData */
            void copyData( void *data, const Vector2I &size ) override;

            /**
             * \copydoc Texture::_getObject
             *
             * Returns the underlying engine object (typically an `Ogre`
             * texture pointer) through the provided pointer-to-pointer.
             */
            void _getObject( void **ppObject ) const override;

            /** \copydoc Texture::getActualSize */
            Vector2I getActualSize() const override;

            /**
             * \brief Retrieve the GPU representation of the texture.
             * \param ppTexture Output pointer that receives the GPU texture
             *                  object (type depends on rendering backend).
             */
            void getTextureGPU( void **ppTexture ) const override;

            /**
             * \brief Retrieve the final texture object used by the graphics API.
             *
             * This may differ from the GPU texture wrapper and is provided to
             * callers that need the backend-specific final handle.
             */
            void getTextureFinal( void **ppTexture ) const override;

            /** \copydoc ITexture::getTextureHandle */
            size_t getTextureHandle() const override;

            /** \copydoc ITexture::getUsageFlags */
            u32 getUsageFlags() const override;

            /** \copydoc ITexture::setUsageFlags */
            void setUsageFlags( u32 usageFlags ) override;

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
             * \brief Access the internal Ogre GPU texture pointer.
             * \return Pointer to the internal `Ogre::TextureGpu` or nullptr.
             */
            Ogre::TextureGpu *getTexture() const;

            /**
             * \brief Set the internal `Ogre::TextureGpu` instance.
             * \param texture Pointer to an existing Ogre GPU texture to adopt.
             */
            void setTexture( Ogre::TextureGpu *texture );

        protected:
            /** \copydoc Texture::createStateObject */
            void createStateObject() override;

            void setSizeCoroutine( ICoroutineData::PullType &coroutine );

            /**
             * Time (timestamp) when the next resize operation should be
             * processed. Used to defer or throttle expensive resize work.
             */
            time_interval m_nextResize = 0.0;

            /** Wrapped pointer to the underlying Ogre GPU texture. */
            AtomicRawPtr<Ogre::TextureGpu> m_texture;
        };
    }  // end namespace render
}  // namespace workphone

#endif
