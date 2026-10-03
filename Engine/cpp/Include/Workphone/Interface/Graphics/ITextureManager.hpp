#ifndef _ITextureManager_H
#define _ITextureManager_H

#include <Workphone/Interface/System/IResourceManager.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    namespace render
    {

        /** Interface for a texture manager. */
        class WPCore_API ITextureManager : public IResourceManager
        {
        public:
            /** Virtual destructor. */
            ~ITextureManager() override;

            /** Creates a manual texture.
             * @return A texture instance.
             */
            virtual SmartPtr<ITexture> createManual( const String &name, const String &group, u8 texType,
                                                     u32 width, u32 height, u32 depth, s32 num_mips,
                                                     u8 format, s32 usage = 0 ) = 0;

            /** Creates a video texture.
             * @param name The name of the texture as a string.
             * @return A video texture instance.
             */
            virtual SmartPtr<IVideoTexture> createVideoTexture( const String &name ) = 0;

            /** Creates a cubemap. */
            virtual SmartPtr<IGraphicsCubemap> addCubemap() = 0;

            /** Creates a render texture.
             *@return A texture instance.
             */
            virtual SmartPtr<ITexture> createRenderTexture() = 0;

            /** Destroys a render texture.
             * @param texture The texture to destroy.
             */
            virtual void destroyRenderTexture( SmartPtr<ITexture> texture ) = 0;

            /** Create a cubemap.
             * @param textures The texture filenames to convert.
             * @return A texture instance.
             */
            virtual SmartPtr<ITexture> createCubeMap( const Array<String> &textures ) = 0;

            /** Create a cubemap.
             * @param textures The texture objects to convert.
             * @return A texture instance.
             */
            virtual SmartPtr<ITexture> createCubeMap( const Array<SmartPtr<ITexture>> &textures ) = 0;

            /** Create a cubemap.
             * @param material The material object to convert.
             * @return A texture instance.
             */
            virtual SmartPtr<ITexture> createCubeMap( SmartPtr<IMaterial> material ) = 0;

            /** Clone a texture. */
            virtual SmartPtr<ITexture> cloneTexture( SmartPtr<ITexture> texture,
                                                     const String &clonedTextureName ) = 0;

            /** Clone a texture. */
            virtual SmartPtr<ITexture> cloneTexture( const String &name,
                                                     const String &clonedTextureName ) = 0;

            /**
             * @brief Gets the internal array of textures.
             * @return Shared pointer to the array of textures.
             */
            virtual Array<SmartPtr<ITexture>> getTextures() const = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif
