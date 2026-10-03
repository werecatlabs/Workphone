#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CWater.hpp>
#include <WPGraphicsOgre/Addons/WaterMesh.hpp>
#include <Workphone/Workphone.hpp>

#define COMPLEXITY 32  // watch out - number of polys is 2*ACCURACY*ACCURACY !
#define PLANE_SIZE 1000.0f

namespace workphone
{
    namespace render
    {
        class WaterFrameListener : public Ogre::FrameListener
        {
        public:
            WaterFrameListener( CWater *water ) : m_water( water ), m_time( 0.0f )
            {
            }

            ~WaterFrameListener() override
            {
            }

            bool frameStarted( const Ogre::FrameEvent &evt ) override
            {
                return true;
            }

            bool frameRenderingQueued( const Ogre::FrameEvent &evt ) override
            {
                // m_time += evt.timeSinceLastFrame;

                WaterMesh *waterMesh = m_water->getWaterMesh();
                waterMesh->updateMesh( evt.timeSinceLastFrame );

                // Ogre::Root* root = Ogre::Root::getSingletonPtr();
                // Ogre::SceneManager* smgr = root->getSceneManager("GameSceneManager");
                // Ogre::Camera* camera = smgr->getCamera("DefaultCamera");
                // Ogre::Vector3 position = camera->getParentNode()->getPosition();

                // Vector3F waterPos = m_water->getPosition();
                // if(waterPos.Y() > position.y - 5.0)
                //{
                //	f32 tx = ((position.x - waterPos.X()) / PLANE_SIZE) * ((float)(COMPLEXITY-2) + 1);
                //	f32 ty = ((position.z - waterPos.Z()) / PLANE_SIZE) * ((float)(COMPLEXITY-2) + 1);
                //	waterMesh->push(tx, ty, -0.1, false);
                // }

                return true;
            }

            bool frameEnded( const Ogre::FrameEvent &evt ) override
            {
                return true;
            }

        protected:
            CWater *m_water;
            f32 m_time;
        };

        u32 CWater::m_nameExt = 0;

        CWater::CWater()
        {
        }

        CWater::~CWater()
        {
            Ogre::Root *root = Ogre::Root::getSingletonPtr();
            root->removeFrameListener( m_frameListener );
            WP_SAFE_DELETE( m_frameListener );
        }

        void CWater::initialise( SmartPtr<IBuildDirector> objectTemplate )
        {
            auto meshName = String( "WaterStandard" ) + StringUtil::toString( m_nameExt++ );
            m_waterMesh = new WaterMesh( meshName, PLANE_SIZE, COMPLEXITY );

            m_mesh = m_sceneMgr->addGraphicsObjectByType<IGraphicsMesh>();
            m_mesh->setName( meshName );

            m_sceneNode = m_sceneMgr->getRootSceneNode()->addChildSceneNode();
            m_sceneNode->attachObject( m_mesh );

            Ogre::MaterialPtr material;  // =
            // Ogre::MaterialManager::getSingletonPtr()->load("Water",
            // Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();

            m_mesh->setMaterialName( "Water" );

            auto root = Ogre::Root::getSingletonPtr();
            m_frameListener = new WaterFrameListener( this );
            root->addFrameListener( m_frameListener );
        }

        void CWater::update()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto timer = applicationManager->getTimer();

            auto task = Thread::getCurrentTask();

            switch( task )
            {
            case TaskId::Render:
                // m_waterMesh->updateMesh(dt);
                break;
            default:
            {
            }
            }
        }

        SmartPtr<IGraphicsScene> CWater::getSceneManager() const
        {
            return m_sceneMgr;
        }

        void CWater::setSceneManager( SmartPtr<IGraphicsScene> sceneMgr )
        {
            m_sceneMgr = sceneMgr;
        }

        SmartPtr<IGraphicsCamera> CWater::getCamera() const
        {
            return m_camera;
        }

        void CWater::setCamera( SmartPtr<IGraphicsCamera> camera )
        {
            m_camera = camera;
        }

        SmartPtr<IViewport> CWater::getViewport() const
        {
            return m_viewport;
        }

        void CWater::setViewport( SmartPtr<IViewport> viewport )
        {
            m_viewport = viewport;
        }

        WaterMesh *CWater::getWaterMesh() const
        {
            return m_waterMesh;
        }

        void CWater::setWaterMesh( WaterMesh *waterMesh )
        {
            m_waterMesh = waterMesh;
        }

        Vector3F CWater::getPosition() const
        {
            return m_sceneNode->getPosition();
        }

        void CWater::setPosition( const Vector3F &position )
        {
            m_sceneNode->setPosition( position );
        }
    }  // end namespace render
}  // namespace workphone
