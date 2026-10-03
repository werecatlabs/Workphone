#ifndef _VOLUMETRICSUNLIGHTFRAMELISTENER_H
#define _VOLUMETRICSUNLIGHTFRAMELISTENER_H

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include "Ogre.h"

namespace workphone
{

    class VolumetricSunLightFrameListener : public Ogre::CompositorInstance::Listener
    {
    public:
        VolumetricSunLightFrameListener( Ogre::SceneManager *pSceneManager, Ogre::Camera *mCamera,
                                         Ogre::Viewport *pViewport );
        ~VolumetricSunLightFrameListener();

        void notifyMaterialSetup( Ogre::uint32 pass_id, Ogre::MaterialPtr &mat );
        void notifyMaterialRender( Ogre::uint32 pass_id, Ogre::MaterialPtr &mat );

    protected:
        Ogre::GpuProgramParametersSharedPtr fpParams;

        Ogre::SceneManager *m_pSceneManager;
        Ogre::Light *m_pSunLight;
        Ogre::Camera *m_pMainCamera;
        Ogre::Viewport *m_pMainViewport;

        float alphaXZ;
        float alphaZY;
    };

}  // namespace workphone

#endif
