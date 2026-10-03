#ifndef Texture3D_h__
#define Texture3D_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>

#include <OgrePixelFormat.h>
#include <OgreCommon.h>
#include <OgreTextureUnitState.h>

namespace workphone
{
    namespace render
    {

        class Texture3D
        {
        public:
            // Constructor
            Texture3D();

            // Create a 3D texture with the specified dimensions and format
            Texture3D( s32 width, s32 height, s32 depth, Ogre::PixelFormat format );

            // Destroy the 3D texture and free any allocated resources
            virtual ~Texture3D();

            // Set the texture filtering and wrapping modes
            void setFilter( Ogre::FilterType minFilter, Ogre::FilterType magFilter,
                            Ogre::FilterType mipFilter );
            void setWrapMode( Ogre::Sampler::UVWAddressingMode u, Ogre::Sampler::UVWAddressingMode v,
                              Ogre::Sampler::UVWAddressingMode w );

            // Update a portion of the 3D texture with the specified data
            void setSubImage( s32 level, s32 xoffset, s32 yoffset, s32 zoffset, s32 width, s32 height,
                              s32 depth, Ogre::PixelFormat format, u32 type, const void *data );

            // Get the width, height, and depth of the 3D texture
            s32 getWidth() const;
            s32 getHeight() const;
            s32 getDepth() const;

            // Get the texture ID of the 3D texture
            u32 getTextureID() const;

        private:
            // Private member variables
            s32 width;
            s32 height;
            s32 depth;
            u32 textureID;
        };

    }  // namespace render
}  // namespace workphone

#endif  // Texture3D_h__
