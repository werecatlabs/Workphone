#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/Apple/CTextureOgreApple.hpp>
#include <Workphone/WPCore.hpp>
#include <OgreTextureGpuManager.h>
#include <OgreRoot.h>
#include <WPGraphicsOgreNext/Ogre/RenderSystems/Metal/include/OgreMetalPrerequisites.h>
#include <WPGraphicsOgreNext/Ogre/RenderSystems/Metal/include/OgreMetalTextureGpu.h>

namespace workphone
{
    namespace render
    {

        void CTextureOgreApple::getTextureFinal( void **ppTexture ) const
        {
            auto ogreTexture = getTexture();
            if (ogreTexture)
            {
#if defined WP_PLATFORM_APPLE
#    if WP_BUILD_RENDERER_METAL
                auto glTexture = static_cast<Ogre::MetalTextureGpu *>( ogreTexture );
                auto tex = glTexture->getFinalTextureName();
                *static_cast<void **>( ppTexture ) = (void *)CFBridgingRetain( tex );
#    elif WP_BUILD_RENDERER_OPENGL
                auto glTexture = (Ogre::GL3PlusTextureGpu *)ogreTexture;
                auto tex = glTexture->getFinalTextureName();
                *static_cast<void **>( ppTexture ) = (void *)CFBridgingRetain( tex );
#    endif
#endif
            }
        }

    }  // end namespace render
}  // end namespace fb
