#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Addons/Texture3D.hpp>

namespace workphone
{
    namespace render
    {

        Texture3D::Texture3D()
        {
        }

        Texture3D::Texture3D( s32 width, s32 height, s32 depth, Ogre::PixelFormat format )
        {
        }

        Texture3D::~Texture3D()
        {
        }

        void Texture3D::setFilter( Ogre::FilterType minFilter, Ogre::FilterType magFilter,
                                   Ogre::FilterType mipFilter )
        {
        }

        void Texture3D::setWrapMode( Ogre::Sampler::UVWAddressingMode u,
                                     Ogre::Sampler::UVWAddressingMode v,
                                     Ogre::Sampler::UVWAddressingMode w )
        {
        }

        void Texture3D::setSubImage( s32 level, s32 xoffset, s32 yoffset, s32 zoffset, s32 width,
                                     s32 height, s32 depth, Ogre::PixelFormat format, u32 type,
                                     const void *data )
        {
        }

        s32 Texture3D::getWidth() const
        {
            return 0;
        }

        s32 Texture3D::getHeight() const
        {
            return 0;
        }

        s32 Texture3D::getDepth() const
        {
            return 0;
        }

        u32 Texture3D::getTextureID() const
        {
            return 0;
        }

    }  // namespace render
}  // namespace workphone
