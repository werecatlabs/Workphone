#ifndef SceneCullJob_h__
#define SceneCullJob_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/System/Job.hpp>
#include <OgreSceneManager.h>
#include <WPGraphicsOgre/Addons/CameraVisibilityState.hpp>  /

namespace workphone
{
    namespace render
    {

        class SceneCullJob : public IJob
        {
        public:
            SceneCullJob();
            ~SceneCullJob();

            void update();

            void findVisible();

            u32 getProgress() const;
            void setProgress( u32 progress );

            s32 getPriority() const;
            void setPriority( s32 priority );

            CameraVisibilityState *getVisibilityState() const;
            void setVisibilityState( CameraVisibilityState *visibilityState );

            Ogre::SceneManager *getSceneManager() const;
            void setSceneManager( Ogre::SceneManager *sceneManager );

            Ogre::Camera *getCamera() const;
            void setCamera( Ogre::Camera *camera );

            Ogre::VisibleObjectsBoundsInfo *getVisibleBounds() const;
            void setVisibleBounds( Ogre::VisibleObjectsBoundsInfo *visibleBounds );

            bool getOnlyShadowCasters() const;
            void setOnlyShadowCasters( bool onlyShadowCasters );

            Ogre::RenderQueue *getRenderQueue() const;
            void setRenderQueue( Ogre::RenderQueue *renderQueue );

        protected:
            void addVisibleObjects( Ogre::SceneNode *sceneNode, Ogre::Camera *cam,
                                    Ogre::VisibleObjectsBoundsInfo *visibleBounds,
                                    bool onlyShadowCasters, CameraVisibilityState *visibilityState );

            void _addToRenderQueue( Ogre::SceneNode *sceneNode, Ogre::Camera *cam,
                                    Ogre::RenderQueue *queue, bool onlyShadowCasters,
                                    Ogre::VisibleObjectsBoundsInfo *visibleBounds );

            CameraVisibilityState *m_visibilityState;
            Ogre::SceneManager *m_sceneManager;
            Ogre::Camera *m_camera;
            Ogre::RenderQueue *m_renderQueue;

            Ogre::VisibleObjectsBoundsInfo *m_visibleBounds;
            bool m_onlyShadowCasters;
        };

    }  // namespace render
}  // namespace workphone

#endif  // SceneCullJob_h__
