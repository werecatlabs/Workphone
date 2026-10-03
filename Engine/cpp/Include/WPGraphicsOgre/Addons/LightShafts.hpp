#ifndef LightShafts_h__
#define LightShafts_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <OgreRenderTargetListener.h>
#include <Ogre.h>
#include <queue>

namespace workphone
{
    namespace render
    {
        class LightShafts
        {
        public:
            LightShafts( SmartPtr<render::IGraphicsScene> smgr, Ogre::Camera *camera );
            ~LightShafts();

            // Just override the mandatory create scene method
            void createScene( void );

            bool createLightShafts( Ogre::BillboardSet *Billboard, Ogre::Camera *LightCamera,
                                    const int &NumberOfPlanes );

            void createLightCameraRTT();

        protected:
            class LightShaftsListener : public Ogre::FrameListener
            {
            public:
                Ogre::SceneManager *mSceneMgr;
                Ogre::Real mKeyBuffer;
                bool mRotateEnable;
                bool mRotateKnot;

                Ogre::SceneNode *mLightCameraSN;
                Ogre::Camera *mLightCamera;

                LightShaftsListener( Ogre::Camera *cam, Ogre::SceneManager *sm );

                bool frameStarted( const Ogre::FrameEvent &e );

                void updatePosition( const Ogre::FrameEvent &e );

                void updateTextureProjectionMatrix();
            };

            SmartPtr<render::IGraphicsScene> m_smgr;
            Ogre::SceneManager *mSceneMgr;
            Ogre::Camera *mCamera;

            /// Main light billboard set
            Ogre::BillboardSet *mBillboardSet;
            /// Camera used for rendering the shadow map and as light frustum
            Ogre::Camera *mLightCamera;
            /// Light camera scene node
            Ogre::SceneNode *mLightCameraSN;

            SmartPtr<IGraphicsSceneNode> m_lightCameraNode;

            // Knot scene node
            Ogre::SceneNode *mKnotSN;

            // Current cookie selected
            int mCurrentCookie;
        };

    }  // namespace render
}  // namespace workphone

#endif  // LightShafts_h__
