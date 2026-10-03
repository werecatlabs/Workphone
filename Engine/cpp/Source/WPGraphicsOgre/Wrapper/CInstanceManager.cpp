#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CInstanceManager.hpp>
#include <WPGraphicsOgre/Addons/InstanceObject.hpp>
#include <Workphone/Workphone.hpp>
#include "WPGraphicsOgre/Terrain/TerrainComponent/OgreTerrain.h"
#include "WPGraphicsOgre/Terrain/TerrainComponent/OgreTerrainGroup.h"
#include "WPGraphicsOgre/Terrain/TerrainComponent/OgreTerrainQuadTreeNode.h"
#include "WPGraphicsOgre/Terrain/TerrainComponent/OgreTerrainMaterialGeneratorA.h"
#include "WPGraphicsOgre/Terrain/TerrainComponent/OgreTerrainPaging.h"
#include "WPGraphicsOgre/Jobs/InstancesLODJob.hpp"
#include <Ogre.h>
#include <tinyxml.h>

namespace workphone
{
    namespace render
    {
        class InstanceData
        {
        public:
            Ogre::Vector3 m_position;
            Ogre::Vector3 m_scale;
            Ogre::Vector3 m_rotation;
        };

        CInstanceManager::CInstanceManager() :
            m_lastViewport( nullptr ),
            m_grass( nullptr ),
            m_grassLoader( nullptr ),
            m_nextUpdate( 0 )
        {
        }

        CInstanceManager::~CInstanceManager()
        {
            if( m_grass )
            {
                delete m_grassLoader;
                delete m_grass;
            }

            for( u32 i = 0; i < m_instanceObjects.size(); ++i )
            {
                CInstanceObjectOld *instanceObject = m_instanceObjects[i];
                WP_SAFE_DELETE( instanceObject );
            }
        }

        Ogre::Real CInstanceManager::getTerrainHeight( Ogre::Real x, Ogre::Real z, void *userData )
        {
            auto terrain = static_cast<IGraphicsTerrain *>( userData );
            return terrain->getHeightAtWorldPosition( Vector3F( x, 1000, z ) ) +
                   terrain->getPosition().Y();
        }

        void CInstanceManager::initialise( SmartPtr<IBuildDirector> objectTemplate )
        {
            createGrass();
        }

        void CInstanceManager::load( const String &fileName )
        {
            String ext = Path::getFileExtension( fileName );
            if( ext == ( ".grass" ) )
                createGrass();
            else
                loadVueScene( fileName );
        }

        void CInstanceManager::update()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto timer = applicationManager->getTimer();

            auto task = Thread::getCurrentTask();
            auto t = timer->getTime();
            auto dt = timer->getDeltaTime();

            switch( task )
            {
            case TaskId::Render:
            {
                static time_interval nextUpdate = 0;

                if( m_lastViewport )
                {
                    u32 currentTime = Ogre::Root::getSingletonPtr()->getTimer()->getMilliseconds();
                    if( m_nextUpdate < currentTime )
                    {
                        if( !m_lodJob )
                        {
                            // m_lodJob = SmartPtr<IJob>(new InstancesLODJob(m_instanceObjects,
                            // m_lastViewport), true);

                            // auto engine = core::IApplicationManager::instance();
                            // SmartPtr<IJobQueue> jobQueue = engine->getJobQueue();

                            // m_lodJob->setState(IJob::STATE_READY);
                            // jobQueue->queue(m_lodJob);
                        }
                        else
                        {
                            // if(m_lodJob->getState() == IJob::STATE_FINISHED)
                            //{
                            //	for(u32 i=0; i<m_instanceManagers.size(); ++i)
                            //	{
                            //		Ogre::InstanceManager* instanceManager = m_instanceManagers[i];
                            //		instanceManager->defragmentBatches(false);
                            //	}

                            //	auto engine = core::IApplicationManager::instance();
                            //	SmartPtr<IJobQueue> jobQueue = engine->getJobQueue();

                            //	m_lodJob->setState(IJob::STATE_READY);
                            //	jobQueue->queue(m_lodJob);
                            //}
                            // else
                            //{
                            //	auto engine = core::IApplicationManager::instance();
                            //	SmartPtr<IJobQueue> jobQueue = engine->getJobQueue();
                            //	jobQueue->queue(m_lodJob);
                            //}
                        }

                        m_nextUpdate = currentTime + 1000;
                    }
                }
            }
            break;
            default:
            {
            }
            }
        }

        void CInstanceManager::preViewportUpdate( const Ogre::RenderTargetViewportEvent &evt )
        {
            m_lastViewport = evt.source;
        }

        void CInstanceManager::loadVueScene( const String &fileName )
        {
            using namespace Ogre;

            auto engine = core::IApplicationManager::instance();

            using InstanceDataSorted = std::map<String, Array<InstanceData>>;
            InstanceDataSorted instanceDataSorted;

            SmartPtr<IFileSystem> fileSystem = engine->getFileSystem();
            SmartPtr<IStream> stream = fileSystem->open( fileName );
            if( !stream )
                return;

            auto dataStr = stream->getAsString();

            // sort data
            TiXmlDocument doc;
            doc.Parse( dataStr.c_str() );
            if( !doc.Error() )
            {
                TiXmlElement *root = doc.RootElement();
                TiXmlElement *ecoSystemElem = root->FirstChildElement( "eco_system" );
                while( ecoSystemElem )
                {
                    Ogre::Vector3 position;
                    Ogre::Vector3 rotation;
                    Ogre::Vector3 scale;
                    String fileName;

                    TiXmlElement *instanceElem = ecoSystemElem->FirstChildElement( "instance" );
                    while( instanceElem )
                    {
                        TiXmlElement *positionElem = instanceElem->FirstChildElement( "position" );
                        if( positionElem )
                        {
                            position.x = StringUtil::parseFloat( positionElem->Attribute( "x" ) );
                            position.z = StringUtil::parseFloat( positionElem->Attribute( "y" ) );
                            position.y = StringUtil::parseFloat( positionElem->Attribute( "z" ) );

                            position.y += 42.0f;  // hack

                            position.z = -position.z;
                        }

                        TiXmlElement *rotationElem = instanceElem->FirstChildElement( "rotation" );
                        if( rotationElem )
                        {
                            rotation.x = StringUtil::parseFloat( rotationElem->Attribute( "x" ) );
                            rotation.z = StringUtil::parseFloat( rotationElem->Attribute( "y" ) );
                            rotation.y = StringUtil::parseFloat( rotationElem->Attribute( "z" ) );
                        }

                        TiXmlElement *scaleElem = instanceElem->FirstChildElement( "scale" );
                        if( scaleElem )
                        {
                            scale.x = StringUtil::parseFloat( scaleElem->Attribute( "x" ) );
                            scale.z = StringUtil::parseFloat( scaleElem->Attribute( "y" ) );
                            scale.y = std::max( scale.x, scale.z );

                            scale *= 5.0f;

                            // scale = Ogre::Vector3::UNIT_SCALE *
                            // StringUtil::parseFloat(scaleElem->Attribute("z"));

                            scale = Ogre::Vector3::UNIT_SCALE;
                        }

                        TiXmlElement *fileNameElem = instanceElem->FirstChildElement( "fileName" );
                        if( fileNameElem )
                        {
                            fileName = fileNameElem->Attribute( "path" );
                        }

                        if( fileName == "gloriosum.veg" )
                        {
                            scale = Ogre::Vector3::UNIT_SCALE * 0.2f * MathF::RangedRandom( 0.9f, 1.1f );
                        }
                        else if( fileName == "coffee.veg" )
                        {
                            scale = Ogre::Vector3::UNIT_SCALE * 0.2f * MathF::RangedRandom( 0.9f, 1.1f );
                        }
                        else if( fileName == "lush_small_palm.veg" )
                        {
                            scale = Ogre::Vector3::UNIT_SCALE * 0.2f * MathF::RangedRandom( 0.9f, 1.1f );
                        }
                        else if( fileName == "croton_green.veg" )
                        {
                            scale = Ogre::Vector3::UNIT_SCALE * 0.2f * MathF::RangedRandom( 0.9f, 1.1f );
                        }
                        else
                        {
                            scale = Ogre::Vector3::UNIT_SCALE * MathF::RangedRandom( 0.8f, 1.2f );
                        }

                        InstanceData data;
                        data.m_position = position;
                        data.m_position.y *= 0.25f;

                        data.m_rotation = rotation;
                        data.m_scale = scale;
                        instanceDataSorted[fileName].push_back( data );

                        instanceElem = instanceElem->NextSiblingElement();
                    }

                    ecoSystemElem = ecoSystemElem->NextSiblingElement();
                }

                doc.Clear();
            }

            auto it = instanceDataSorted.begin();
            for( ; it != instanceDataSorted.end(); ++it )
            {
                String meshFileNameWithoutExt = Path::getFileNameWithoutExtension( it->first.c_str() );
                Array<InstanceData> &instanceDataVector = it->second;

                String meshFileNameNgp = meshFileNameWithoutExt + ".ngp";

                auto graphicsSystem = engine->getGraphicsSystem();
                SmartPtr<IGraphicsScene> smgr = graphicsSystem->getGraphicsScene( "Default" );
                if( !smgr )
                    return;

                SmartPtr<IGraphicsTerrain> terrain = nullptr;
                //smgr->getTerrain( 0 );
                if( !terrain )
                    return;

                m_terrain = terrain;

                SceneManager *sceneMgr;
                smgr->_getObject( (void **)&sceneMgr );

                Camera *camera = sceneMgr->getCamera( "DefaultCamera" );

                int mInstancingTechnique = 3;
                int mCurrentFlags = 0;
                InstanceManager::InstancingTechnique technique = InstanceManager::ShaderBased;
                technique = InstanceManager::HWInstancingBasic;

                uint16 flags = IM_USEALL;
                flags |= mCurrentFlags;

                if( mInstancingTechnique == 4 )
                {
                    flags |= IM_VTFBONEMATRIXLOOKUP;
                }
                // Only one weight is recommended for the VTF technique, but force the use of more for
                // the demo
                if( mInstancingTechnique == 1 && ( flags & IM_USEBONEDUALQUATERNIONS ) )
                {
                    flags &= ~IM_USEONEWEIGHT;
                }

                Ogre::String meshFileName = "";//
                //OgreNGPlantLoader::loadMesh( meshFileNameNgp ).c_str();

                Ogre::MeshPtr mesh = Ogre::MeshManager::getSingletonPtr()->load(
                    meshFileName, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );

                // Create one instance mgr per submesh, managerNames contains unique random names
                for( int i = 0; i < mesh->getNumSubMeshes(); ++i )
                {
                    InstanceManager *mgr = sceneMgr->createInstanceManager(
                        "InstanceMgr" + StringConverter::toString( i ) + meshFileName, meshFileName,
                        ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, technique, 80, IM_USE16BIT,
                        i );

                    m_instanceManagers.push_back( mgr );

                    mgr->setBatchesAsStaticAndUpdate( true );
                }

                AxisAlignedBox box = mesh->getBounds();
                Ogre::Vector3 halfExtent = box.getHalfSize();

                Ogre::MeshManager::getSingleton().createPlane(
                    "InstanceMgr" + meshFileName + "_BilboardPlane",
                    ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Plane( Ogre::Vector3::UNIT_Z, 0 ),
                    halfExtent.x, halfExtent.y, 1, 1, true, 1, 1.0f, 1.0f, Ogre::Vector3::UNIT_Y );

                // create one for the billboard
                InstanceManager *mgr = sceneMgr->createInstanceManager(
                    "InstanceMgr" + meshFileName + "_BilboardPlane",
                    "InstanceMgr" + meshFileName + "_BilboardPlane",
                    ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, technique, 1500, IM_USE16BIT );

                m_instanceManagers.push_back( mgr );

                for( size_t instanceDataIdx = 0; instanceDataIdx < instanceDataVector.size();
                     ++instanceDataIdx )
                {
                    // hack
                    if( instanceDataIdx % 2 == 0 )
                        continue;

                    const InstanceData &data = instanceDataVector[instanceDataIdx];

                    auto instanceObject = new CInstanceObjectOld;

                    for( size_t subMeshIdx = 0; subMeshIdx < mesh->getNumSubMeshes(); ++subMeshIdx )
                    {
                        Ogre::SubMesh *subMesh = mesh->getSubMesh( subMeshIdx );

                        Ogre::String instanceManagerName =
                            "InstanceMgr" + StringConverter::toString( subMeshIdx ) + meshFileName;

                        Ogre::String materialName = subMesh->getMaterialName();

                        // Create the instanced entity
                        InstancedEntity *ent =
                            sceneMgr->createInstancedEntity( materialName, instanceManagerName );

                        ent->setPosition( data.m_position, false );

                        Ogre::Matrix3 mat;
                        mat.FromEulerAnglesXYZ( Degree( data.m_rotation.x ), Degree( data.m_rotation.y ),
                                                Degree( data.m_rotation.z ) );

                        Ogre::Quaternion orientation( mat );
                        ent->setOrientation( orientation, false );

                        ent->setScale( data.m_scale, false );
                        ent->updateTransforms();

                        ent->setVisibilityFlags( IGraphicsScene::VIEWPORT_MASK_OCCLUDER |
                                                 IGraphicsScene::VIEWPORT_MASK_SHADOW );

                        instanceObject->add( ent );
                    }

                    // create billboard instance
                    /*
                    String billboardMaterialName = meshFileNameWithoutExt + "_Billboard";

                    Ogre::InstancedEntity *billboardEntity = sceneMgr->createInstancedEntity(
                        billboardMaterialName.c_str(), "InstanceMgr" + meshFileName + "_BilboardPlane");

                    billboardEntity->setCastShadows(false);

                    billboardEntity->setPosition(data.m_position + Ogre::Vector3::UNIT_Y * halfExtent.y *
                    0.45f, false); billboardEntity->setScale(data.m_scale, false);
                    billboardEntity->updateTransforms();
                    instanceObject->setBillboard(billboardEntity);
                    */

                    instanceObject->m_position = data.m_position;
                    m_instanceObjects.push_back( instanceObject );
                }

                for( size_t subMeshIdx = 0; subMeshIdx < mesh->getNumSubMeshes(); ++subMeshIdx )
                {
                    Ogre::SubMesh *subMesh = mesh->getSubMesh( subMeshIdx );

                    Ogre::String instanceManagerName =
                        "InstanceMgr" + StringConverter::toString( subMeshIdx ) + meshFileName;
                    InstanceManager *instanceMgr = sceneMgr->getInstanceManager( instanceManagerName );
                    instanceMgr->defragmentBatches( true );

                    // Ogre::InstanceManager::InstanceBatchIterator instanceBatchIterator =
                    // instanceMgr->getInstanceBatchIterator( subMesh->getMaterialName() );
                    // while(instanceBatchIterator.hasMoreElements())
                    //{
                    //	Ogre::InstanceBatch* instanceBatch = instanceBatchIterator.getNext();
                    //	instanceBatch->setVisibilityFlags(IGraphicsSceneManager::VIEWPORT_MASK_USER |
                    // IGraphicsSceneManager::VIEWPORT_MASK_SHADOW);
                    // }
                }

                // Ogre::String instanceManagerName = "InstanceMgr" + meshFileName + "_BilboardPlane";
                // Ogre::InstanceManager* instanceMgr =
                // sceneMgr->getInstanceManager(instanceManagerName); instanceMgr->defragmentBatches(
                // true );

                // String billboardMaterialName = meshFileNameWithoutExt + "_Billboard";
                // Ogre::InstanceManager::InstanceBatchIterator instanceBatchIterator =
                // instanceMgr->getInstanceBatchIterator( billboardMaterialName.c_str() );
                // while(instanceBatchIterator.hasMoreElements())
                //{
                //	Ogre::InstanceBatch* instanceBatch = instanceBatchIterator.getNext();
                //	instanceBatch->setVisibilityFlags(IGraphicsSceneManager::VIEWPORT_MASK_USER);
                // }
            }
        }

        void CInstanceManager::createGrass()
        {
            // PagedGeometry's classes and functions are under the "Forests" namespace
            using namespace Forests;
            using namespace Ogre;

            auto engine = core::IApplicationManager::instance();
            auto graphicsSystem = engine->getGraphicsSystem();
            SmartPtr<IGraphicsScene> smgr = graphicsSystem->getGraphicsScene( "Default" );
            if( !smgr )
            {
                WP_EXCEPTION( "" );
            }

            SmartPtr<IGraphicsTerrain> terrain = nullptr;
            //smgr->getTerrain( 0 );
            if( !terrain )
            {
                WP_EXCEPTION( "" );
            }

            m_terrain = terrain;

            SceneManager *sceneMgr;
            smgr->_getObject( (void **)&sceneMgr );

            Camera *camera = sceneMgr->getCamera( "DefaultCamera" );

            /*

            //-------------------------------------- LOAD TREES --------------------------------------
            //Create and configure a new PagedGeometry instance
            trees = new PagedGeometry();
            trees->setCamera(camera);	//Set the camera so PagedGeometry knows how to calculate LODs
            trees->setPageSize(80);	//Set the size of each page of geometry
            trees->setInfinite();		//Use infinite paging mode
            trees->addDetailLevel<BatchPage>(100, 50);		//Use batches up to 150 units away, and fade
            for 30 more units trees->addDetailLevel<ImpostorPage>(200, 50);	//Use impostors up to 400
            units, and for for 50 more units

            //Create a new TreeLoader3D object
            TreeLoader3D *treeLoader = new TreeLoader3D(trees, TBounds(-500, -500, 500, 500));
            trees->setPageLoader(treeLoader);	//Assign the "treeLoader" to be used to load geometry for
            the PagedGeometry instance

            //Load a tree entity
            SmartPtr<IGraphicsMesh> ent = smgr->addMesh("palm.ngp");
            Entity *myEntity = 0;
            ent->_getObject((void**)&myEntity);



            //Randomly place 20,000 copies of the tree on the terrain
            Ogre::Vector3 position;
            Radian yaw;
            Real scale;
            for (int i = 0; i < 0; i++){
            yaw = Degree(Ogre::Math::RangeRandom(0, 360));

            position.x = fb::MathF::RangedRandom(-500, 500) - 0;
            position.z = fb::MathF::RangedRandom(-500, 500) - 0;
            position.y = 0.0f;
            position.y = terrain->getHeightAtWorldPosition(fb::Vector3F(position.x, position.y,
            position.z)); position.y = position.y; scale = Ogre::Math::RangeRandom(0.9f, 1.2f);

            treeLoader->addTree(myEntity, position, yaw, scale);
            }

            */

            camera->getViewport()->getTarget()->addListener( this );
        }

        Ogre::InstanceManager *CInstanceManager::getInstanceManager() const
        {
            return m_instanceManager;
        }

        void CInstanceManager::setInstanceManager( Ogre::InstanceManager *instanceManager )
        {
            m_instanceManager = instanceManager;
        }

        void CInstanceManager::setNumCustomParams( unsigned char numCustomParams )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_instanceManager->setNumCustomParams( numCustomParams );
        }

        unsigned char CInstanceManager::getNumCustomParams() const
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            return m_instanceManager->getNumCustomParams();
        }
    }  // end namespace render
}  // namespace workphone
