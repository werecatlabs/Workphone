#ifndef MaterialListener_h__
#define MaterialListener_h__

#include <OgreCompositorInstance.h>
#include <OgreRenderQueue.h>

namespace workphone
{
    namespace render
    {

        class MaterialListener : public Ogre::MaterialManager::Listener
        {
        public:
            MaterialListener();
            ~MaterialListener();

            Ogre::Technique *handleSchemeNotFound( unsigned short schemeIndex,
                                                   const Ogre::String &schemeName,
                                                   Ogre::Material *originalMaterial,
                                                   unsigned short lodIndex,
                                                   const Ogre::Renderable *rend );

        protected:
            Ogre::MaterialPtr m_sceneWithOutTextures;
            Ogre::MaterialPtr m_depthMaterial;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // MaterialListener_h__
