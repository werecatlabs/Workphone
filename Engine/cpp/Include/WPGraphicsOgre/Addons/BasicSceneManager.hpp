#ifndef BasicSceneManager_h__
#define BasicSceneManager_h__

#include <OgreSceneManager.h>
#include <OgreFrameListener.h>
#include <Workphone/Core/Pool.hpp>

namespace workphone
{
    namespace render
    {

        class BasicSceneManager : public Ogre::SceneManager, Ogre::FrameListener
        {
        public:
            BasicSceneManager( const Ogre::String &name );
            ~BasicSceneManager();

            void _updateSceneGraph( Ogre::Camera *camera );
            void _findVisibleObjects( Ogre::Camera *cam, Ogre::VisibleObjectsBoundsInfo *visibleBounds,
                                      bool onlyShadowCasters );

            void addSceneNodes( Ogre::SceneNode *sceneNode, Ogre::Camera *cam, bool onlyShadowCasters,
                                Ogre::VisibleObjectsBoundsInfo *visibleBounds );

            void _addVisibleObjects( Ogre::SceneNode *sceneNode, Ogre::Camera *cam,
                                     bool onlyShadowCasters,
                                     Ogre::VisibleObjectsBoundsInfo *visibleBounds );

            virtual const Ogre::String &getTypeName( void ) const;

            virtual bool frameRenderingQueued( const Ogre::FrameEvent &evt );

            void _setActiveCompositorChain( Ogre::CompositorChain *chain );
            const Ogre::Pass *_setPass( const Ogre::Pass *pass, bool evenIfSuppressed,
                                        bool shadowDerivation );
            Ogre::SceneNode *createSceneNodeImpl( void );
            Ogre::SceneNode *createSceneNodeImpl( const Ogre::String &name );
            void destroySceneNode( const Ogre::String &name );

            Ogre::Entity *createEntity( const Ogre::String &entityName,
                                        Ogre::SceneManager::PrefabType ptype );
            Ogre::Entity *createEntity( Ogre::SceneManager::PrefabType ptype );
            Ogre::Entity *createEntity( const Ogre::String &entityName, const Ogre::String &meshName,
                                        const Ogre::String &groupName =
                                            Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
            Ogre::Entity *createEntity( const Ogre::String &entityName, const Ogre::MeshPtr &pMesh );
            Ogre::Entity *createEntity( const Ogre::String &meshName );
            Ogre::Entity *createEntity( const Ogre::MeshPtr &pMesh );

            void clearScene( void );

            void renderSingleObject( Ogre::Renderable *rend, const Ogre::Pass *pass,
                                     bool lightScissoringClipping, bool doLightIteration,
                                     const Ogre::LightList *manualLightList = 0 );

        protected:
        };

        class BasicSceneManagerFactory : public Ogre::SceneManagerFactory
        {
        protected:
            void initMetaData( void ) const;

        public:
            BasicSceneManagerFactory()
            {
            }
            ~BasicSceneManagerFactory()
            {
            }
            /// Factory type name
            static const Ogre::String FACTORY_TYPE_NAME;
            Ogre::SceneManager *createInstance( const Ogre::String &instanceName );
            void destroyInstance( Ogre::SceneManager *instance );
        };

    }  // namespace render
}  // namespace workphone

#endif  // BasicSceneManager_h__
