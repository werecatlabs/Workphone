#ifndef CInstanceManager_h__
#define CInstanceManager_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IInstanceManager.hpp>

#include <OgrePrerequisites.h>
#include <OgreRenderTargetListener.h>

namespace workphone
{
    namespace render
    {

        class CInstanceManager : public IInstanceManager, public Ogre::RenderTargetListener
        {
        public:
            CInstanceManager();
            ~CInstanceManager() override;

            void initialise( SmartPtr<IBuildDirector> objectTemplate );

            void createGrass();

            void update() override;

            void load( const String &fileName ) override;

            void loadVueScene( const String &fileName );

            void preViewportUpdate( const Ogre::RenderTargetViewportEvent &evt ) override;

            static Ogre::Real getTerrainHeight( Ogre::Real x, Ogre::Real z, void *userData );

            Ogre::InstanceManager *getInstanceManager() const;
            void setInstanceManager( Ogre::InstanceManager *instanceManager );

            void setNumCustomParams( unsigned char numCustomParams ) override;
            unsigned char getNumCustomParams() const override;

        protected:
            Ogre::Viewport *m_lastViewport;

            Forests::PagedGeometry *m_grass;
            Forests::GrassLoader *m_grassLoader;

            SmartPtr<IGraphicsTerrain> m_terrain;

            SmartPtr<IJob> m_lodJob;

            Ogre::InstanceManager *m_instanceManager;

            Array<Ogre::InstanceManager *> m_instanceManagers;

            Array<CInstanceObjectOld *> m_instanceObjects;
            u32 m_nextUpdate;
        };
    }  // namespace render
}  // namespace workphone

#endif  // CInstanceManager_h__
