#ifndef LightDepthMapRttListener_h__
#define LightDepthMapRttListener_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <OgreRenderTargetListener.h>

#include <queue>

namespace workphone
{
    namespace render
    {

        // Just for change all entities materials in depth rtt
        class LightDepthMapRttListener : public Ogre::RenderTargetListener
        {
        public:
            LightDepthMapRttListener( Ogre::SceneManager *sm );

            void preRenderTargetUpdate( const Ogre::RenderTargetEvent &evt );

            void postRenderTargetUpdate( const Ogre::RenderTargetEvent &evt );

            std::queue<String> mMaterials;
            Ogre::SceneManager *mSceneMgr;

            /// Main light billboard set
            Ogre::BillboardSet *mBillboardSet;
            /// Camera used for rendering the shadow map and as light frustum
            Ogre::Camera *mLightCamera;
            /// Light camera scene node
            Ogre::SceneNode *mLightCameraSN;
        };

    }  // namespace render
}  // namespace workphone

#endif  // LightDepthMapRttListener_h__
