#ifndef PSSMShadowListener_h__
#define PSSMShadowListener_h__

#include "OgreSceneManager.h"

namespace workphone
{

    class PSSMShadowListener : public Ogre::SceneManager::Listener
    {
        Ogre::Light *light;
        Ogre::ShadowCameraSetupPtr setup;
        Ogre::Camera *view_camera;  // NOT shadow camera!
        Ogre::SceneManager *sceneMgr;
        mutable int split_index;

    public:
        PSSMShadowListener( Ogre::SceneManager *sm, Ogre::Light *l, Ogre::ShadowCameraSetupPtr s,
                            Ogre::Camera *cam );
        ~PSSMShadowListener();
        void shadowTexturesUpdated( size_t numberOfShadowTextures );
        void shadowTextureCasterPreViewProj( Ogre::Light *light, Ogre::Camera *camera,
                                             size_t iteration );
        void shadowTextureReceiverPreViewProj( Ogre::Light *light, Ogre::Frustum *frustum );
        bool sortLightsAffectingFrustum( Ogre::LightList &lightList );
    };

}  // namespace workphone

#endif  // PSSMShadowListener_h__
