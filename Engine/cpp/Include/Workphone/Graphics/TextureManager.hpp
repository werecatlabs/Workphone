#ifndef TextureManager_h__
#define TextureManager_h__

#include <Workphone/Interface/Graphics/ITextureManager.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class TextureManager
         * @brief Manages the creation, loading, retrieval, and destruction of texture resources.
         *
         * This class provides an interface for managing textures, including manual creation,
         * video textures, cubemaps, render textures, and skybox textures. It also supports
         * resource cloning, saving, and retrieval by name or ID.
         */
        class WPCore_API TextureManager : public ITextureManager
        {
        public:
            class WPCore_API StateListener : public IStateListener
            {
            public:
                /**
                 * @brief Default constructor.
                 * Creates a state listener without an associated material owner.
                 * The owner must be set separately using setOwner().
                 */
                StateListener();

                /**
                 * @brief Virtual destructor.
                 * Properly cleans up the state listener and removes any
                 * remaining state context associations.
                 */
                ~StateListener() override;

                /**
                 * @brief Handles the unload state change.
                 * Called when the material or its resources need to be unloaded.
                 * This ensures proper cleanup of graphics resources.
                 *
                 * @param data Optional context data for the unload operation
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handles incoming state messages.
                 * Processes state messages that may affect the material,
                 * such as resource loading notifications or graphics context changes.
                 * @param message The state message to handle
                 * @return True if the message was handled, false otherwise
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handles state transitions.
                 * Called when the material's state changes, allowing the listener
                 * to respond to loading state transitions and resource changes.
                 * @param state Reference to the new state
                 * @return True if the state change was handled successfully
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Gets the material that owns this listener.
                 * Returns a pointer to the material associated with this listener.
                 * @return Pointer to the owner material
                 */
                TextureManager *getOwnerPtr() const;

                /**
                 * @brief Gets the material that owns this listener.
                 * Returns a smart pointer to the material associated with this listener.
                 * @return Smart pointer to the owner material
                 */
                SmartPtr<TextureManager> getOwner() const;

                /**
                 * @brief Sets the material owner for this listener.
                 * Associates this listener with the specified material.
                 * @param owner Smart pointer to the material to associate with
                 */
                void setOwner( SmartPtr<TextureManager> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                /** Weak pointer to the owning material to avoid circular references */
                AtomicWeakPtr<TextureManager> m_owner;
            };

            /**
             * @brief Constructs a TextureManager instance.
             */
            TextureManager();

            /**
             * @brief Destructor for TextureManager.
             */
            ~TextureManager() override;

            /**
             * @copydoc ISharedObject::load
             * @brief Loads texture-related data from a shared object.
             * @param data The shared object containing data to load.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ISharedObject::unload
             * @brief Unloads texture-related data from a shared object.
             * @param data The shared object containing data to unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IResourceManager::create
             * @brief Creates a new resource with the given name.
             * @param name The name of the resource to create.
             * @return A smart pointer to the created resource.
             */
            SmartPtr<IResource> create( const String &name ) override;

            /**
             * @copydoc IResourceManager::create
             * @brief Creates a new resource with the given UUID and name.
             * @param uuid The unique identifier for the resource.
             * @param name The name of the resource to create.
             * @return A smart pointer to the created resource.
             */
            SmartPtr<IResource> create( const String &uuid, const String &name ) override;

            /**
             * @brief Destroys the specified resource.
             * @param resource The resource to destroy.
             */
            void destroyResource( SmartPtr<IResource> resource ) override;

            /**
             * @brief Destroys all managed resources.
             */
            void destroyAll() override;

            /**
             * @copydoc IResourceManager::load
             * @brief Loads a resource by name.
             * @param name The name of the resource to load.
             * @return A smart pointer to the loaded resource.
             */
            SmartPtr<IResource> loadResource( const String &name ) override;

            /**
             * @copydoc IResourceManager::getByName
             * @brief Retrieves a resource by its name.
             * @param name The name of the resource.
             * @return A smart pointer to the resource, or nullptr if not found.
             */
            SmartPtr<IResource> getByName( const String &name ) override;

            /**
             * @copydoc IResourceManager::getById
             * @brief Retrieves a resource by its UUID.
             * @param uuid The unique identifier of the resource.
             * @return A smart pointer to the resource, or nullptr if not found.
             */
            SmartPtr<IResource> getById( const String &uuid ) override;

            /**
             * @brief Clones a resource with a new name.
             * @param resource The resource to clone.
             * @param clonedResourceName The name for the cloned resource.
             * @return A smart pointer to the cloned resource.
             */
            SmartPtr<IResource> cloneResource( SmartPtr<IResource> resource,
                                               const String &clonedResourceName ) override;

            /**
             * @brief Clones a resource by name with a new name.
             * @param name The name of the resource to clone.
             * @param clonedResourceName The name for the cloned resource.
             * @return A smart pointer to the cloned resource.
             */
            SmartPtr<IResource> cloneResource( const String &name,
                                               const String &clonedResourceName ) override;

            /**
             * @copydoc IResourceManager::createOrRetrieve
             * @brief Creates or retrieves a resource by UUID, path, and type.
             * @param uuid The unique identifier for the resource.
             * @param path The path to the resource.
             * @param type The type of the resource.
             * @return A pair containing the resource and a boolean indicating if it was created (true)
             * or retrieved (false).
             */
            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &uuid, const String &path,
                                                              const String &type ) override;

            /**
             * @copydoc IResourceManager::createOrRetrieve
             * @brief Creates or retrieves a resource by path.
             * @param path The path to the resource.
             * @return A pair containing the resource and a boolean indicating if it was created (true)
             * or retrieved (false).
             */
            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &path ) override;

            /**
             * @copydoc IResourceManager::saveToFile
             * @brief Saves a resource to a file.
             * @param filePath The file path to save the resource to.
             * @param resource The resource to save.
             */
            void saveToFile( const String &filePath, SmartPtr<IResource> resource ) override;

            /**
             * @brief Creates a manual texture with the specified parameters.
             * @param name The name of the texture.
             * @param group The resource group.
             * @param texType The type of texture (e.g., 2D, 3D, cube).
             * @param width The width of the texture.
             * @param height The height of the texture.
             * @param depth The depth of the texture.
             * @param num_mips The number of mipmap levels.
             * @param format The pixel format.
             * @param usage The usage flags (default is 0).
             * @return A smart pointer to the created texture.
             */
            SmartPtr<ITexture> createManual( const String &name, const String &group, u8 texType,
                                             u32 width, u32 height, u32 depth, s32 num_mips, u8 format,
                                             s32 usage = 0 ) override;

            /**
             * @brief Creates a video texture with the specified name.
             * @param name The name of the video texture.
             * @return A smart pointer to the created video texture.
             */
            SmartPtr<IVideoTexture> createVideoTexture( const String &name ) override;

            /**
             * @brief Adds a new cubemap texture.
             * @return A smart pointer to the created cubemap.
             */
            SmartPtr<IGraphicsCubemap> addCubemap() override;

            /**
             * @brief Creates a render texture.
             * @return A smart pointer to the created render texture.
             */
            SmartPtr<ITexture> createRenderTexture() override;

            /**
             * @brief Destroys a render texture.
             * @param texture The render texture to destroy.
             */
            void destroyRenderTexture( SmartPtr<ITexture> texture ) override;

            /**
             * @brief Creates a skybox cubemap from a list of texture file paths.
             * @param skyboxTextures An array of file paths for the skybox textures.
             * @return A smart pointer to the created skybox cubemap texture.
             */
            SmartPtr<ITexture> createCubeMap( const Array<String> &skyboxTextures ) override;

            /**
             * @brief Creates a skybox cubemap from a list of texture objects.
             * @param skyboxTextures An array of smart pointers to the skybox textures.
             * @return A smart pointer to the created skybox cubemap texture.
             */
            SmartPtr<ITexture> createCubeMap( const Array<SmartPtr<ITexture>> &skyboxTextures ) override;

            /**
             * @brief Creates a skybox cubemap from a material.
             * @param material A smart pointer to the material to use for the skybox.
             * @return A smart pointer to the created skybox cubemap texture.
             */
            SmartPtr<ITexture> createCubeMap( SmartPtr<IMaterial> material ) override;

            /**
             * @brief Clones a texture with a new name.
             * @param texture The texture to clone.
             * @param clonedTextureName The name for the cloned texture.
             * @return A smart pointer to the cloned texture.
             */
            SmartPtr<ITexture> cloneTexture( SmartPtr<ITexture> texture,
                                             const String &clonedTextureName ) override;

            /**
             * @brief Clones a texture by name with a new name.
             * @param name The name of the texture to clone.
             * @param clonedTextureName The name for the cloned texture.
             * @return A smart pointer to the cloned texture.
             */
            SmartPtr<ITexture> cloneTexture( const String &name,
                                             const String &clonedTextureName ) override;

            /**
             * @brief Gets the internal array of textures.
             * @return Shared pointer to the array of textures.
             */
            Array<SmartPtr<ITexture>> getTextures() const override;

            /**
             * @brief Get the raw pointer to the attached IStateContext.
             * Useful when C-style pointer access is required. The returned pointer is not
             * accompanied by ownership guarantees; prefer getStateContext() for ownership.
             * @return Raw IStateContext pointer or nullptr if none set.
             */
            IStateContext *getStateContextPtr() const override;

            /**
             * @brief Get the SmartPtr to the attached IStateContext.
             * Returns the internal smart pointer that owns the state context.
             * @return SmartPtr<IStateContext> reference (may be nullptr).
             */
            SmartPtr<IStateContext> getStateContext() const override;

            /**
             * @brief Attach a state context to this object.
             * The provided SmartPtr will be stored and later unloaded/removed by
             * destroyStateContext() during object unload.
             * @param stateContext Smart pointer to the state context to attach.
             */
            void setStateContext( SmartPtr<IStateContext> stateContext ) override;

            /**
             * @brief Handles incoming state messages.
             * @param message The state message to handle
             * @return True if the message was handled, false otherwise
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message );

            /**
             * @brief Handles state transitions.
             * Called when the material's state changes, allowing the listener
             * to respond to loading state transitions and resource changes.
             * @param state Reference to the new state
             * @return True if the state change was handled successfully
             */
            bool handleStateChanged( SmartPtr<IState> &state );

            void lock() override;

            bool try_lock() override;

            void unlock() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<IStateContext> m_stateContext;

            /** Internal storage for managed textures. */
            ConcurrentArray<SmartPtr<ITexture>> m_textures;

            /** Internal storage for render target textures. */
            ConcurrentArray<SmartPtr<ITexture>> m_renderTargets;
        };

        inline IStateContext *TextureManager::getStateContextPtr() const
        {
            return m_stateContext.get();
        }

        inline TextureManager *TextureManager::StateListener::getOwnerPtr() const
        {
            return m_owner.get();
        }

    }  // namespace render
}  // namespace workphone

#endif  // TextureManager_h__
