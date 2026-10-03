#ifndef _ITexture_H
#define _ITexture_H

#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Interface for a texture class.
         *
         * The `ITexture` class is a subtype of `IResource` that defines the interface for a
         * texture object. Texture objects can be used to store and manipulate images and other
         * graphical data in a format that is optimized for rendering in a 3D environment.
         *
         * This class provides the necessary functions to describe the size and properties of a
         * texture object, as well as to manipulate and copy its contents. These functions can
         * be overridden by subclasses to implement specific texture types, such as 2D, 3D, or
         * cube maps.
         *
         * In addition to the functions inherited from `IResource`, this class also defines a
         * number of specific member functions for texture objects, including functions to get
         * and set the render target, copy the texture to another texture, get and set the size
         * of the texture, and get the GPU texture object.
         *
         * @see IResource
         */
        class WPCore_API ITexture : public IResource
        {
        public:
            static const hash_type STATE_MESSAGE_TEXTURE_SIZE;

            ITexture();

            ITexture( u32 poolTypeId );

            /** Virtual destructor. */
            ~ITexture() override;

            /**
             * @brief Gets the render target.
             * @return A smart pointer to the render target associated with this texture.
             */
            virtual SmartPtr<IRenderTarget> getRenderTarget() const = 0;

            /**
             * @brief Sets the render target.
             * @param rt A smart pointer to the render target to associate with this texture.
             */
            virtual void setRenderTarget( SmartPtr<IRenderTarget> rt ) = 0;

            /**
             * @brief Copies (and maybe scales to fit) the contents of this texture to another texture.
             * @param target A smart pointer to the target texture to which to copy the contents.
             */
            virtual void copyToTexture( SmartPtr<ITexture> &target ) = 0;

            /**
             * @brief Copies the data passed.
             * @param data A pointer to the data to copy to the texture.
             * @param size The size of the data.
             */
            virtual void copyData( void *data, const Vector2I &size ) = 0;

            /**
             * @brief Gets the size of the texture.
             * @return The size of the texture as a 2D vector.
             */
            virtual Vector2I getSize() const = 0;

            /**
             * @brief Sets the size of the texture.
             * @param size The new size of the texture as a 2D vector.
             */
            virtual void setSize( const Vector2I &size ) = 0;

            /**
             * @brief Gets the actual size of the texture.
             * @return The actual size of the texture as a 2D vector.
             */
            virtual Vector2I getActualSize() const = 0;

            /**
             * @brief Gets the GPU texture.
             * @param ppTexture A pointer to a pointer to the GPU texture object.
             */
            virtual void getTextureGPU( void **ppTexture ) const = 0;

            /**
             * @brief Gets the final GPU texture.
             * This object depends on the graphics API being used.
             * @param ppTexture A pointer to a pointer to the final GPU texture object.
             */
            virtual void getTextureFinal( void **ppTexture ) const = 0;

            /**
             * @brief Gets the internal handle.
             * @return The internal handle for the texture.
             */
            virtual size_t getTextureHandle() const = 0;

            /**
             * @brief Gets the usage flags of the texture.
             *
             * The usage flags determine how the texture will be used. For example, whether it is
             * used as a render target or a texture, and whether it is read-only or write-only.
             * The flags are a combination of the values in the `TextureUsageFlags` enumeration.
             *
             * @return The usage flags of the texture.
             *
             * @see TextureUsageFlags
             */
            virtual u32 getUsageFlags() const = 0;

            /**
             * @brief Sets the usage flags of the texture.
             *
             * The usage flags determine how the texture will be used. For example, whether it is
             * used as a render target or a texture, and whether it is read-only or write-only.
             * The flags are a combination of the values in the `TextureUsageFlags` enumeration.
             *
             * @param usageFlags The new usage flags to set for the texture.
             *
             * @see TextureUsageFlags
             */
            virtual void setUsageFlags( u32 usageFlags ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif
