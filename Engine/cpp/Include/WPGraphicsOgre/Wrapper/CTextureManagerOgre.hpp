#ifndef _CTextureManager_H
#define _CTextureManager_H

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Graphics/TextureManager.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @brief Ogre implementation of the texture manager.
         *
         * CTextureManagerOgre is responsible for creating, loading and managing
         * texture resources for the Ogre renderer. It implements the
         * generic TextureManager interface and provides additional helpers for
         * render textures, cubemaps and video textures. The manager also
         * registers a TextureListener to respond to texture-related events and
         * loading state changes.
         */
        class CTextureManagerOgre : public TextureManager
        {
        public:
            /**
             * @brief Event listener used by texture objects.
             *
             * TextureListener receives events related to texture lifecycle and
             * loading state changes and translates them to engine-level actions.
             * It also provides a destruction callback used by some resource
             * holders.
             */
            class TextureListener : public IEventListener
            {
            public:
                /** Default constructor */
                TextureListener();
                /** Virtual destructor */
                ~TextureListener() override;

                /**
                 * @brief Handle an event sent to the listener.
                 *
                 * This method is called by the engine's event system for any
                 * event targeted at the texture listener.
                 *
                 * @param eventType Type/category of the event.
                 * @param eventValue Numeric event identifier or hashed value.
                 * @param arguments Additional event parameters.
                 * @param sender Shared object that sent the event (may be null).
                 * @param object Optional related shared object for the event.
                 * @param event The event object containing metadata.
                 * @return A Parameter value returned to the event sender (if used).
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Called when the loading state of a shared object changes.
                 *
                 * Used to observe and react to loading progress for texture assets.
                 *
                 * @param sharedObject Pointer to the object whose state changed.
                 * @param oldState Previous loading state.
                 * @param newState New loading state.
                 */
                void loadingStateChanged( ISharedObject *sharedObject, LoadingState oldState,
                                          LoadingState newState );

                /**
                 * @brief Destruction callback.
                 *
                 * Some systems require a function pointer to a destroy-like
                 * callback; this provides a hook to perform custom cleanup.
                 *
                 * @param ptr Pointer passed by the caller (context-specific).
                 * @return true if the object was destroyed/handled, false otherwise.
                 */
                bool destroy( void *ptr );

                WP_CLASS_REGISTER_DECL;
            };

            /** Default constructor. */
            CTextureManagerOgre();
            /** Virtual destructor. */
            ~CTextureManagerOgre() override;

            /** @copydoc ISharedObject::update
             *
             * @note Runs before the main update step; used to prepare texture state.
             */
            void preUpdate() override;

            /** @copydoc ISharedObject::update */
            void update() override;

            /** @copydoc ISharedObject::postUpdate
             *
             * @note Runs after the main update step; used to finalize texture state.
             */
            void postUpdate() override;

            /**
             * @copydoc ISharedObject::load
             *
             * @param data Optional data used during load (depends on implementation).
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ISharedObject::unload
             *
             * @param data Optional data used during unload (depends on implementation).
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IResourceManager::create
             *
             * @param name Resource name to create.
             * @return Pointer to the created resource.
             */
            SmartPtr<IResource> create( const String &name ) override;

            /**
             * @copydoc IResourceManager::create
             *
             * @param uuid Optional UUID to assign to the resource.
             * @param name Resource name to create.
             * @return Pointer to the created resource.
             */
            SmartPtr<IResource> create( const String &uuid, const String &name ) override;

            /**
             * @copydoc IResourceManager::load
             *
             * @param name Name or identifier of the resource to load.
             * @return Pointer to the loaded resource, or null if loading failed.
             */
            SmartPtr<IResource> loadResource( const String &name ) override;

            /**
             * @copydoc IResourceManager::createOrRetrieve
             *
             * Create a resource if it does not exist, or retrieve an existing
             * one. Returns a pair of (resource, isNew) where isNew is true if
             * the resource was created.
             *
             * @param uuid Unique identifier to use when creating the resource.
             * @param path Resource path.
             * @param type Resource type string.
             * @return Pair containing the resource and a flag indicating creation.
             */
            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &uuid, const String &path,
                                                              const String &type ) override;

            /**
             * @brief Overload of createOrRetrieve that only uses a path.
             *
             * @param path Resource path.
             * @return Pair containing the resource and a flag indicating creation.
             */
            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &path ) override;

            /**
             * @copydoc IResourceManager::loadFromFile
             *
             * Load a resource directly from a file path.
             *
             * @param filePath Absolute or relative file path to load from.
             * @return Pointer to the loaded resource, or null on failure.
             */
            SmartPtr<IResource> loadFromFile( const String &filePath ) override;

            /**
             * @copydoc ITextureManager::createManual
             *
             * Create a texture manually (not loaded from disk). Useful for
             * render-targets, dynamically generated textures, and procedural textures.
             *
             * @param name Texture name.
             * @param group Resource group for the texture.
             * @param texType Texture type (enum/byte defined by engine).
             * @param width Width in pixels.
             * @param height Height in pixels.
             * @param depth Depth (for volume textures), or 1 for 2D.
             * @param num_mips Number of mipmap levels.
             * @param format Pixel format identifier.
             * @param usage Usage flags (optional).
             * @return Pointer to the created texture.
             */
            SmartPtr<ITexture> createManual( const String &name, const String &group, u8 texType,
                                             u32 width, u32 height, u32 depth, s32 num_mips, u8 format,
                                             s32 usage = 0 ) override;

            /**
             * @copydoc ITextureManager::createVideoTexture
             *
             * Create a texture suitable for streaming video frames into.
             *
             * @param name Texture name.
             * @return Pointer to the created video texture object.
             */
            SmartPtr<IVideoTexture> createVideoTexture( const String &name ) override;

            /**
             * @copydoc ITextureManager::addCubemap
             *
             * Create or register a cubemap resource.
             *
             * @return Pointer to the cubemap.
             */
            SmartPtr<IGraphicsCubemap> addCubemap() override;

            /**
             * @copydoc ITextureManager::createRenderTexture
             *
             * Create a texture intended to be used as a render target.
             *
             * @return Pointer to the render texture.
             */
            SmartPtr<ITexture> createRenderTexture() override;

            /**
             * @copydoc ITextureManager::destroyRenderTexture
             *
             * Remove and free a previously created render texture.
             *
             * @param texture The render texture to destroy.
             */
            void destroyRenderTexture( SmartPtr<ITexture> texture ) override;

            /**
             * @copydoc ITextureManager::createCubeMap
             *
             * Create a cubemap used for skyboxes from file paths.
             *
             * @param textures Array of file paths for the 6 faces (order depends on engine).
             * @return Pointer to the created skybox cubemap texture.
             */
            SmartPtr<ITexture> createCubeMap( const Array<String> &textures ) override;

            /**
             * @brief Create a skybox cubemap from existing texture objects.
             *
             * @param textures Array of textures for the 6 faces.
             * @return Pointer to the created skybox cubemap texture.
             */
            SmartPtr<ITexture> createCubeMap(
                const Array<SmartPtr<ITexture>> &textures ) override;

            /**
             * @brief Create a skybox cubemap using a material (material may contain references to textures).
             *
             * @param material Material used to build the skybox cubemap.
             * @return Pointer to the created skybox cubemap texture.
             */
            SmartPtr<ITexture> createCubeMap( SmartPtr<IMaterial> material ) override;

            /**
             * @copydoc ITextureManager::_getObject
             *
             * Provides access to the underlying renderer-specific object (e.g.
             * an Ogre texture manager pointer). The pointer returned is opaque
             * and renderer-dependent.
             *
             * @param ppObject Pointer to receive the underlying object pointer.
             */
            void _getObject( void **ppObject ) const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Register a render texture with the manager.
             *
             * Internal helper used to track and manage the lifetime of render textures.
             *
             * @param texture Render texture to add.
             */
            void addRenderTexture( SmartPtr<ITexture> texture );

            /**
             * @brief Unregister a render texture from the manager.
             *
             * @param texture Render texture to remove.
             */
            void removeRenderTexture( SmartPtr<ITexture> texture );

            /** Create GPU render textures required by the renderer. */
            void createRenderTextures();

            /** Listener used to observe texture events and loading state changes. */
            AtomicSmartPtr<TextureListener> m_textureListener;
        };
    }  // end namespace render
}  // namespace workphone

#endif
