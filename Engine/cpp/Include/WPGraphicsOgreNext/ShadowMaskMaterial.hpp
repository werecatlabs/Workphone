#ifndef ShadowMaskMaterial_h__
#define ShadowMaskMaterial_h__

#include <Ogre.h>
#include <OgreHlmsManager.h>
#include <OgreHlmsPbs.h>
#include <OgreHlmsPbsDatablock.h>

namespace workphone
{
    namespace render
    {

        class ShadowMaskMaterial
        {
        public:
            static Ogre::HlmsPbsDatablock *create( Ogre::HlmsManager *hlmsManager,
                                                   const Ogre::String &materialName,
                                                   const Ogre::String &albedoTex,
                                                   const Ogre::String &shadowMaskTex );
        };

    }  // namespace render
}  // namespace workphone

#endif  // ShadowMaskMaterial_h__
