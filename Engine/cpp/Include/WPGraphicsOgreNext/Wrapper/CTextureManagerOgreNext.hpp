#ifndef _CTextureManager_H
#define _CTextureManager_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/TextureManager.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Core/Pair.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @brief Texture manager implementation for the Ogre Next backend.
         *
         * This class implements the TextureManager interface using the Ogre Next
         * rendering backend. It is responsible for creating, loading, saving
         * and destroying textures, as well as managing render-target textures
         * and queued operations that need to be executed on the rendering
         * thread (reloads, transitions and delayed destroys).
         */
        class CTextureManagerOgreNext : public TextureManager
        {
        public:
            /**
             * @brief Listener used by textures to forward important events.
             *
             * The TextureListener receives events from texture objects and
             * converts them into actions on the manager (for example tracking
             * loading state changes or scheduling delayed destruction of
             * underlying Ogre resources). It implements IEventListener so
             * it can be registered with the engine event system.
             */
            class TextureListener : public IEventListener
            {
            public:
                /**
                 * @brief Construct a new TextureListener.
                 */
                TextureListener();

                /**
                 * @brief Destroy the TextureListener.
                 */
                ~TextureListener() override;

                /**
                 * @brief Handle an incoming event.
                 *
                 * @param eventType Type of the event being handled.
                 * @param eventValue Numeric value associated with the event.
                 * @param arguments Event parameters as an Array of Parameter.
                 * @param sender The sender of the event (shared object).
                 * @param object Optional object related to the event.
                 * @param event Raw event pointer.
                 * @return Parameter Arbitrary return parameter used by the event system.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Callback when a texture's loading state changes.
                 *
                 * This method is invoked when a tracked texture transitions
                 * between loading states so the manager can react (for
                 * example enqueue reloads or finalize resource creation).
                 *
                 * @param sharedObject The texture object whose state changed.
                 * @param oldState Previous loading state.
                 * @param newState New loading state.
                 */
                void loadingStateChanged( ISharedObject *sharedObject, LoadingState oldState,
                                          LoadingState newState );

                /**
                 * @brief Called when an Ogre resource must be destroyed.
                 *
                 * The manager may defer destruction of GPU resources until it is
                 * safe to do so on the rendering thread. This method is called
                 * to request destruction of a pointer previously associated with
                 * a texture.
                 *
                 * @param ptr Raw pointer to the resource to destroy.
                 * @return true If destruction was handled, false otherwise.
                 */
                bool destroy( void *ptr );

                SmartPtr<CTextureManagerOgreNext> getOwner() const;

                void setOwner( SmartPtr<CTextureManagerOgreNext> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                AtomicWeakPtr<CTextureManagerOgreNext> m_owner; /**< Owner manager of this listener. */
            };

            class StateListener : public IStateListener
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

                void setOwner( SmartPtr<CTextureManagerOgreNext> owner );

                SmartPtr<CTextureManagerOgreNext> getOwner() const;

                WP_CLASS_REGISTER_DECL;

            protected:
                AtomicWeakPtr<CTextureManagerOgreNext> m_owner; /**< Owner manager of this listener. */
            };

            /**
             * @brief Construct a new CTextureManagerOgreNext instance.
             */
            CTextureManagerOgreNext();

            /**
             * @brief Destroy the CTextureManagerOgreNext instance.
             */
            ~CTextureManagerOgreNext() override;

            /**
             * @copydoc ISharedObject::update
             *
             * Performs per-frame updates for the manager. This may process
             * queued operations that are safe to run on the current thread.
             */
            void update() override;

            /**
             * @copydoc ISharedObject::postUpdate
             *
             * Called after update when any pending post-update work should be
             * executed. The Ogre backend may require some operations to run
             * here to stay synchronized with the renderer.
             */
            void postUpdate() override;

            /**
             * @copydoc ISharedObject::load
             *
             * Initialize manager state using the provided shared object
             * (typically configuration or resource registration data).
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ISharedObject::unload
             *
             * Release or clear manager state associated with the provided
             * shared object. This should free any non-GPU resources and
             * schedule GPU resource destruction as needed.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IResourceManager::create
             *
             * Create a texture resource with the given name. The caller
             * receives a smart pointer to the IResource instance.
             */
            SmartPtr<IResource> create( const String &name ) override;

            /**
             * @copydoc IResourceManager::create
             *
             * Create a texture resource with an explicit UUID and name.
             */
            SmartPtr<IResource> create( const String &uuid, const String &name ) override;

            /**
             * @copydoc IResourceManager::load
             *
             * Load a texture resource by name. Returns a smart pointer to the
             * loaded resource or a null pointer if loading failed.
             */
            SmartPtr<IResource> loadResource( const String &name ) override;

            /**
             * @copydoc IResourceManager::createOrRetrieve
             *
             * Create a new resource if it doesn't exist, or retrieve an
             * existing one by UUID and path. The returned Pair contains the
             * resource and a boolean indicating whether it was newly created.
             */
            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &uuid, const String &path,
                                                              const String &type ) override;

            /**
             * @copydoc IResourceManager::createOrRetrieve
             *
             * Create or retrieve a resource by its path. Returns the resource
             * and a flag indicating if a new resource was created.
             */
            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &path ) override;

            /**
             * @copydoc IResourceManager::saveToFile
             *
             * Persist the supplied resource to disk at the specified file
             * path. This is typically used for exporting texture data.
             */
            void saveToFile( const String &filePath, SmartPtr<IResource> resource ) override;

            /**
             * @copydoc IResourceManager::loadFromFile
             *
             * Load a resource from the filesystem and return it as an
             * IResource smart pointer. Returns null on failure.
             */
            SmartPtr<IResource> loadFromFile( const String &filePath ) override;

            /**
             * @copydoc ITextureManager::createManual
             *
             * Create a manually defined texture with explicit dimensions and
             * format. This is used for dynamic or procedurally generated
             * textures that are not loaded from files.
             */
            SmartPtr<ITexture> createManual( const String &name, const String &group, u8 texType,
                                             u32 width, u32 height, u32 depth, s32 num_mips, u8 format,
                                             s32 usage = 0 ) override;

            /**
             * @copydoc ITextureManager::createVideoTexture
             *
             * Create a texture intended to be used as a video stream target.
             */
            SmartPtr<IVideoTexture> createVideoTexture( const String &name ) override;

            /**
             * @copydoc ITextureManager::addCubemap
             *
             * Add a new cubemap resource and return a handle to it.
             */
            SmartPtr<IGraphicsCubemap> addCubemap() override;

            /**
             * @copydoc ITextureManager::createRenderTexture
             *
             * Create a render target texture suitable for use as a framebuffer
             * attachment or render-to-texture operation.
             */
            SmartPtr<ITexture> createRenderTexture() override;

            /**
             * @copydoc ITextureManager::destroyRenderTexture
             *
             * Destroy a previously created render target texture. Destruction
             * may be deferred depending on thread-safety requirements.
             */
            void destroyRenderTexture( SmartPtr<ITexture> texture ) override;

            /**
             * @copydoc ITextureManager::createSkyBoxCubeMap
             *
             * Create a texture suitable for use as a skybox from an
             * array of file paths.
             */
            SmartPtr<ITexture> createCubeMap( const Array<String> &textures ) override;

            /**
             * @copydoc ITextureManager::createSkyBoxCubeMap
             *
             * Create a cubemap from an array of existing texture
             * objects (one per face).
             */
            SmartPtr<ITexture> createCubeMap( const Array<SmartPtr<ITexture>> &textures ) override;

            /**
             * @copydoc ITextureManager::createSkyBoxCubeMap
             *
             * Create a cubemap by extracting textures referenced by a
             * material.
             */
            SmartPtr<ITexture> createCubeMap( SmartPtr<IMaterial> material ) override;

            /**
             * @copydoc ITextureManager::_getObject
             *
             * Return the underlying native graphics object (for example the
             * Ogre resource manager pointer) if applicable. The pointer is
             * written to ppObject.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Schedule a texture to be reloaded on the rendering thread.
             *
             * Some reload operations must be executed on the rendering thread
             * to be safe. This enqueues the texture so the manager can process
             * it at the appropriate time.
             *
             * @param texture Texture to reload.
             */
            void queueReload( SmartPtr<ITexture> texture );

            /**
             * @brief Queue a state transition for a texture.
             *
             * When textures need to change internal state (for example
             * switching residency or layout) this method enqueues the request
             * so the manager can apply it on the correct thread.
             *
             * @param texture Texture to transition.
             * @param state Target state identifier.
             */
            void queueTransition( SmartPtr<ITexture> texture, s32 state );

            bool handleStateChanged( SmartPtr<IState> &state );

            WP_CLASS_REGISTER_DECL;

        protected:
            Array<SmartPtr<ITexture>> m_renderTargets;

            SmartPtr<TextureListener> m_textureListener;
            SmartPtr<StateListener> m_textureStateListener;

            ConcurrentQueue<Pair<SmartPtr<ITexture>, u32>> m_transitionTextures;
            ConcurrentQueue<SmartPtr<ITexture>> m_reloadTextures;
            ConcurrentQueue<Ogre::TextureGpu *> m_destroyTextures;
        };
    }  // end namespace render
}  // namespace workphone

#endif
