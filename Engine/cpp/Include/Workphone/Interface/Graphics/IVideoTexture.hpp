#ifndef IVideoTexture_h__
#define IVideoTexture_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @brief Interface for video textures in the rendering system.
         *
         * This class provides an interface for managing video textures, which are textures
         * that can be updated with video content in real-time. It inherits from ITexture
         * and adds video-specific functionality.
         */
        class WPCore_API IVideoTexture : public render::ITexture
        {
        public:
            IVideoTexture();

            IVideoTexture( u32 poolTypeInfo );

            /**
             * @brief Virtual destructor.
             */
            ~IVideoTexture() override;

            /**
             * @brief Initializes the video texture with the specified parameters.
             *
             * @param name The name identifier for the video texture.
             * @param size The dimensions of the video texture (width and height).
             *
             * @note This method must be called before using the video texture.
             */
            virtual void initialise( const String &name, const Vector2I &size ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // IVideoTexture_h__
