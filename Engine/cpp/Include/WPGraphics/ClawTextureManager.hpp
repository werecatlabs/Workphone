#ifndef ClawTextureManager_h__
#define ClawTextureManager_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/TextureManager.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class ClawTextureManager
         * @brief Texture resource manager used by the ClawHammer renderer.
         * 
         * This class handles the lifecycle of texture resources, including loading, 
         * creating, and cloning textures within the ClawHammer rendering pipeline.
         */
        class WPGraphics_API ClawTextureManager : public TextureManager
        {
        public:
            ClawTextureManager();
            ~ClawTextureManager() override;

            /** @brief Loads texture data into the manager. */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @brief Unloads texture data from the manager. */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @brief Creates a new texture resource with the given name. */
            SmartPtr<IResource> create( const String &name ) override;
            /** @brief Creates a new texture resource with the given UUID and name. */
            SmartPtr<IResource> create( const String &uuid, const String &name ) override;
            /** @brief Loads a texture resource by its name. */
            SmartPtr<IResource> loadResource( const String &name ) override;
            /** @brief Loads a texture resource from a specified file path. */
            SmartPtr<IResource> loadFromFile( const String &filePath ) override;
            /** @brief Clones an existing texture resource. */
            SmartPtr<IResource> cloneResource( SmartPtr<IResource> resource,
                                               const String &clonedResourceName ) override;
            /** @brief Clones a texture resource identified by name. */
            SmartPtr<IResource> cloneResource( const String &name,
                                               const String &clonedResourceName ) override;
            /** @brief Retrieves an existing texture or creates a new one based on the provided parameters. */
            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &uuid, const String &path,
                                                              const String &type ) override;
            /** @brief Retrieves an existing texture or creates a new one based on the provided path. */
            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &path ) override;

            /** @brief Creates a texture manually with specified dimensions and format. */
            SmartPtr<ITexture> createManual( const String &name, const String &group, u8 texType,
                                             u32 width, u32 height, u32 depth, s32 numMips, u8 format,
                                             s32 usage = 0 ) override;
            /** @brief Creates a texture specifically for rendering targets. */
            SmartPtr<ITexture> createRenderTexture() override;
            /** Build a filtered cubemap from six generated 2D faces. */
            SmartPtr<ITexture> createCubeMap( const Array<SmartPtr<ITexture>> &textures ) override;
            /** @brief Destroys a previously created render texture. */
            void destroyRenderTexture( SmartPtr<ITexture> texture ) override;

            /** @brief Retrieves the underlying object pointer. */
            void _getObject( void **ppObject ) const override;

            WP_CLASS_REGISTER_DECL;

        private:
            /** @brief Internal helper to create a texture instance. */
            SmartPtr<ITexture> createTexture( const String &uuid, const String &name );
        };
    }  // namespace render
}  // namespace workphone

#endif  // ClawTextureManager_h__
