#ifndef CellSceneManager_h__
#define CellSceneManager_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <OgreSceneManager.h>
#include <OgreRenderOperation.h>
#include <OgreSphere.h>
#include <list>
#include <algorithm>
#include <boost/thread/thread.hpp>
#include <boost/pool/object_pool.hpp>
#include "CameraVisibilityState.hpp"

namespace workphone
{
    namespace render
    {

        class CellSceneManager : public Ogre::SceneManager
        {
        public:
            CellSceneManager( const Ogre::String &name );
            ~CellSceneManager();

            void _queueSkiesForRendering( Ogre::Camera *cam );

            Ogre::Camera *createCamera( const Ogre::String &name );

            void _findVisibleObjects( Ogre::Camera *cam, Ogre::VisibleObjectsBoundsInfo *visibleBounds,
                                      bool onlyShadowCasters );

            void renderSingleObject( Ogre::Renderable *rend, const Ogre::Pass *pass,
                                     bool lightScissoringClipping, bool doLightIteration,
                                     const Ogre::LightList *manualLightList = 0 );

            Ogre::RenderQueue *getCurrentRenderQueue();
            Ogre::RenderQueue *getRenderQueue();

            virtual const Ogre::String &getTypeName( void ) const;

            // Ogre::SceneNode* createSceneNodeImpl(void);
            // Ogre::SceneNode* createSceneNodeImpl(const Ogre::String& name);

        protected:
            struct CullThread
            {
                CullThread( CellSceneManager *owner );

                void operator()();

                CellSceneManager *m_owner;
            };

            Pool<GraphicsObjectVisibility> m_graphicsObjectVisibilityPool;

            boost::thread *m_cullThread;

            void addVisibleObjects( Ogre::SceneNode *sceneNode, Ogre::Camera *cam,
                                    Ogre::VisibleObjectsBoundsInfo *visibleBounds,
                                    bool onlyShadowCasters, CameraVisibilityState *visibilityState );
            void _addToRenderQueue( Ogre::SceneNode *sceneNode, Ogre::Camera *cam,
                                    Ogre::RenderQueue *queue, bool onlyShadowCasters,
                                    Ogre::VisibleObjectsBoundsInfo *visibleBounds );
            void renderSingleObjectDerived( Ogre::Renderable *rend, const Ogre::Pass *pass,
                                            bool lightScissoringClipping, bool doLightIteration,
                                            const Ogre::LightList *manualLightList );

            void prepareRenderQueue();
            Ogre::SceneManager::RenderContext *_pauseRendering();
            void _resumeRendering( Ogre::SceneManager::RenderContext *context );

            Ogre::Pass *m_pass;
            Ogre::String m_name;
        };

        /// Factory for OctreeSceneManager
        class CellSceneManagerFactory : public Ogre::SceneManagerFactory
        {
        protected:
            void initMetaData( void ) const;

        public:
            CellSceneManagerFactory();
            ~CellSceneManagerFactory();

            /// Factory type name
            static const Ogre::String FACTORY_TYPE_NAME;
            Ogre::SceneManager *createInstance( const Ogre::String &instanceName );
            void destroyInstance( Ogre::SceneManager *instance );
        };

    }  // namespace render
}  // namespace workphone

#endif  // CellSceneManager_h__
