#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CTerrainOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsSystemOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CTerrainRayResult.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreSharedPtr.h>
#include <OgreShadowCameraSetup.h>
#include <WPGraphicsOgre/Terrain/TerrainComponent/OgreTerrainMaterialGeneratorA.h>
#include <WPGraphicsOgre/Terrain/TerrainComponent/OgreTerrainMaterialGenerator.h>
#include <WPGraphicsOgre/Terrain/TerrainComponent/OgreTerrainQuadTreeNode.h>

// Include PagedGeometry headers that will be needed
#include "WPGraphicsOgre/PagedGeometry/PagedGeometry.h"
#include "WPGraphicsOgre/PagedGeometry/BatchPage.h"
#include "WPGraphicsOgre/PagedGeometry/ImpostorPage.h"
#include "WPGraphicsOgre/PagedGeometry/TreeLoader3D.h"

// Include "LegacyTerrainLoader.h", a header that allows loading Ogre 1.7 style terrain
#include "WPGraphicsOgre/PagedGeometry/LegacyTerrainLoader.h"

// Include "HeightFunction.h", a header that provides some useful functions for quickly and easily
// getting the height of the terrain at a given point.
#include "OgreUtil.hpp"
#include "WPGraphicsOgre/PagedGeometry/HeightFunction.h"
#include "Terrain/TerrainComponent/OgreTerrainAutoUpdateLod.h"

#define TERRAIN_FILE_PREFIX Ogre::String( "testTerrain" )
#define TERRAIN_FILE_SUFFIX Ogre::String( "dat" )
#define TERRAIN_WORLD_SIZE 1000.0f
#define TERRAIN_SIZE 257

#define TERRAIN_PAGE_MIN_X 0
#define TERRAIN_PAGE_MIN_Y 0
#define TERRAIN_PAGE_MAX_X 0
#define TERRAIN_PAGE_MAX_Y 0

// PagedGeometry's classes and functions are under the "Forests" namespace
using namespace Forests;

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, CTerrainOgre, IGraphicsTerrain );
        u32 CTerrainOgre::m_ext = 0;

        CTerrainOgre::CTerrainOgre() :
            mTerrainGroup( nullptr ),
            mTerrainPaging( nullptr ),
            mPageManager( nullptr ),

            mTerrainPos( 0, 0, 0 )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto stateManager = applicationManager->getStateManager();
            auto factoryManager = applicationManager->getFactoryManager();

            auto stateContext = stateManager->addStateContext();
            setStateContext( stateContext );

            //auto state = factoryManager->make_ptr<TerrainStateData>();
            //stateContext->addState( state );

            //auto stateListener = factoryManager->make_ptr<TerrainStateListener>();
            //stateListener->setOwner( this );
            //stateContext->addStateListener( stateListener );
            //setStateListener( stateListener );

            stateContext->setTaskId( TaskId::Render );
        }

        CTerrainOgre::~CTerrainOgre()
        {
            unload( nullptr );
        }

        void CTerrainOgre::update()
        {
            try
            {
                switch( auto task = Thread::getCurrentTask() )
                {
                case TaskId::Render:
                {
                    if( auto terrainGroup = getTerrainGroup() )
                    {
                        terrainGroup->update( true );

                        auto terrain = terrainGroup->getTerrain( 0, 0 );
                        if( terrain )
                        {
                            auto node = terrain->_getRootSceneNode();
                            if( node )
                            {
                                node->_updateBounds();
                            }
                        }
                    }

                    if( auto trees = getTrees() )
                    {
                        setTreeMask( trees->getSceneNode() );

                        auto applicationManager = core::IApplicationManager::instance();
                        auto graphicsSystem = applicationManager->getGraphicsSystem();
                        auto window = graphicsSystem->getDefaultWindow();
                        auto viewports = window->getViewports();
                        for( auto viewport : viewports )
                        {
                            if( viewport )
                            {
                                auto camera = viewport->getCamera();
                                if( camera )
                                {
                                    Ogre::Camera *ogreCamera = nullptr;
                                    camera->_getObject( (void **)&ogreCamera );

                                    auto ogreSceneManager = getOgreSceneManager();

                                    if( ogreCamera->getSceneManager() == ogreSceneManager )
                                    {
                                        trees->setCamera( ogreCamera );
                                    }
                                }
                            }
                        }

                        trees->update();
                    }
                }
                break;
                default:
                {
                }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CTerrainOgre::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto graphicsSystem = applicationManager->getGraphicsSystem();
                auto factoryManager = applicationManager->getFactoryManager();

                auto renderTask = graphicsSystem->getRenderTask();
                auto task = Thread::getCurrentTask();

                WP_ASSERT( task == renderTask );

                setLoadingState( LoadingState::Loading );

                auto ogreSceneManager = getOgreSceneManager();

                auto cameraName = String( "TerrainCamera_" ) + StringUtil::toString( m_ext++ );
                auto terrainCamera = ogreSceneManager->createCamera( cameraName.c_str() );
                auto terrainCameraSceneNode =
                    ogreSceneManager->getRootSceneNode()->createChildSceneNode();
                terrainCameraSceneNode->attachObject( terrainCamera );

                setTerrainCamera( terrainCamera );
                setTerrainCameraSceneNode( terrainCameraSceneNode );

                loadTerrain();
                loadTrees();

                auto message = workphone::make_ptr<StateMessageUIntValue>();
                message->setType( StringUtil::getHash( "loadComplete" ) );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->addMessage( renderTask, message );
                }

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CTerrainOgre::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                if( isLoaded() )
                {
                    WP_ASSERT( Thread::getTaskFlag( Thread::Render_Flag ) );
                    setLoadingState( LoadingState::Unloading );

                    auto applicationManager = core::IApplicationManager::instance();
                    WP_ASSERT( applicationManager );

                    auto graphicsSystem = applicationManager->getGraphicsSystem();
                    auto stateManager = applicationManager->getStateManager();

                    // m_terrainTemplate.setNull();
                    // m_stateContext.setNull();
                    // m_stateListener.setNull();

                    m_blendLayers.clear();

                    WP_SAFE_DELETE( mTerrainGroup );

                    if( m_trees )
                    {
                        delete m_trees;
                        m_trees = nullptr;
                    }

                    if( auto stateContext = getStateContext() )
                    {
                        if( auto stateListener = getStateListener() )
                        {
                            stateContext->removeStateListener( stateListener );

                            stateListener->unload( nullptr );
                            setStateListener( nullptr );
                        }

                        if( stateManager )
                        {
                            stateManager->removeStateContext( stateContext );
                        }

                        setStateContext( nullptr );
                    }

                    m_sceneManager = nullptr;

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CTerrainOgre::loadTerrain()
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto graphicsSystem = applicationManager->getGraphicsSystem();
                bool deferredEnabled = false;  //graphicsSystem->isDeferred();

                bool blankTerrain = false;
                blankTerrain = true;  // initial terrain

                auto ogreSceneManager = getOgreSceneManager();

                // mSceneMgr->setFog(Ogre::FOG_EXP, Ogre::ColourValue::Red);

                Ogre::LogManager::getSingleton().setLogDetail( Ogre::LL_BOREME );

                Ogre::Vector3 lightdir( 0.55, -0.3, 0.75 );
                lightdir.normalise();

                auto lightName = String( "Light0" ) + StringUtil::toString( m_ext++ );
                Ogre::Light *l = ogreSceneManager->createLight( lightName.c_str() );
                l->setType( Ogre::Light::LT_DIRECTIONAL );
                l->setDirection( lightdir );
                l->setDiffuseColour( Ogre::ColourValue::White );
                l->setSpecularColour( Ogre::ColourValue( 0.4, 0.4, 0.4 ) );

                //mSceneMgr->setAmbientLight( Ogre::ColourValue( 0.2, 0.2, 0.2 ) );

                auto terrainGlobals = Ogre::TerrainGlobalOptions::getSingletonPtr();

                // mTerrainGlobals->setCompositeMapDistance(100.0f);

                if( !deferredEnabled )
                {
                    auto generator = terrainGlobals->getDefaultMaterialGenerator();
                    auto activeProfile = generator->getActiveProfile();

                    auto matProfile =
                        static_cast<Ogre::TerrainMaterialGeneratorA::SM2Profile *>( activeProfile );
                    matProfile->setLayerNormalMappingEnabled( false );
                    matProfile->setCompositeMapEnabled( false );
                    matProfile->setGlobalColourMapEnabled( true );
                }
                else
                {
                    /*
                    Ogre::TerrainMaterialGeneratorDeferredPtr terrainMaterialGenerator(
                        new Ogre::TerrainMaterialGeneratorDeferred );
                    mTerrainGlobals->setDefaultMaterialGenerator( terrainMaterialGenerator );
                    Ogre::TerrainGlobalOptions::getSingleton().setCastsDynamicShadows( true );
                    auto matProfile = static_cast<Ogre::TerrainMaterialGeneratorDeferred::SM2Profile *>(
                        terrainMaterialGenerator->getActiveProfile() );
                    matProfile->setCompositeMapEnabled( false );
                    matProfile->setLightmapEnabled( false );
                    matProfile->setGlobalColourMapEnabled( false );
                     */
                }

                //terrainGlobals->setVisibilityFlags(std::numeric_limits<u32>::max());

                //mTerrainGroup = OGRE_NEW Ogre::TerrainGroup( m_ogreSceneManager );

                //mTerrainGroup->loadLegacyTerrain("terrain.cfg");

                mTerrainGroup = OGRE_NEW Ogre::TerrainGroup( ogreSceneManager, Ogre::Terrain::ALIGN_X_Z,
                                                             129, 512.0f );

                //mTerrainGroup = loadLegacyTerrain( "terrain.cfg", ogreSceneManager );
                // mTerrainGroup->setFilenameConvention(TERRAIN_FILE_PREFIX, TERRAIN_FILE_SUFFIX);
                mTerrainGroup->setAutoUpdateLod(
                    Ogre::TerrainAutoUpdateLodFactory::getAutoUpdateLod( Ogre::BY_DISTANCE ) );

                mTerrainGroup->setOrigin( Ogre::Vector3::ZERO );
                mTerrainGroup->setResourceGroup(
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

                // Vector3F position = Vector3F::zero();
                // if (m_terrainTemplate)
                //{
                //	position = m_terrainTemplate->getPosition();
                // }

                // mTerrainPos = Ogre::Vector3(position.X(), position.Y(), position.Z());
                // mTerrainGroup->setOrigin(mTerrainPos);

                configureTerrainDefaults( l );

                // for (long x = TERRAIN_PAGE_MIN_X; x <= TERRAIN_PAGE_MAX_X; ++x)
                //	for (long y = TERRAIN_PAGE_MIN_Y; y <= TERRAIN_PAGE_MAX_Y; ++y)
                defineTerrain( 0, 0, blankTerrain );

                // sync load since we want everything in place when we start
                mTerrainGroup->loadAllTerrains( true );

                ////if (mTerrainsImported)
                //{
                // Ogre::TerrainGroup::TerrainIterator ti = mTerrainGroup->getTerrainIterator();
                // while (ti.hasMoreElements())
                //{
                //	Ogre::Terrain* t = ti.getNext()->instance;
                //	initBlendMaps(t);
                //}
                //}

                mTerrainGroup->updateGeometry();
                mTerrainGroup->updateDerivedData( true );
                mTerrainGroup->update( true );

                mTerrainGroup->freeTemporaryResources();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CTerrainOgre::configureTerrainDefaults( Ogre::Light *l )
        {
            // Configure global
            //mTerrainGlobals->setMaxPixelError( 1 );
            //// testing composite map
            // mTerrainGlobals->setCompositeMapDistance(3000);
            ////mTerrainGlobals->setUseRayBoxDistanceCalculation(true);
            ////mTerrainGlobals->getDefaultMaterialGenerator()->setDebugLevel(1);
            ////mTerrainGlobals->setLightMapSize(256);
            //
            //// Important to set these so that the terrain knows what to use for derived (non-realtime)
            /// data
            // mTerrainGlobals->setLightMapDirection(l->getDerivedDirection());
            ////mTerrainGlobals->setCompositeMapAmbient(mSceneMgr->getAmbientLight());
            // mTerrainGlobals->setCompositeMapAmbient(Ogre::ColourValue::Red);
            // mTerrainGlobals->setCompositeMapDiffuse(l->getDiffuseColour());

            // mTerrainGlobals->setLayerBlendMapSize(4096);

            // Configure default import settings for if we use imported image
            Ogre::Terrain::ImportData &defaultimp = mTerrainGroup->getDefaultImportSettings();
            defaultimp = Ogre::Terrain::ImportData();

            defaultimp.terrainSize = 129;

            //defaultimp.terrainSize = TERRAIN_SIZE;
            //defaultimp.worldSize = TERRAIN_WORLD_SIZE;

            /*
            if( m_terrainTemplate )
            {
                defaultimp.inputScale = m_terrainTemplate->getHeightScale();
            }
            else
            {
                defaultimp.inputScale = 600;
            }
             */

            //defaultimp.minBatchSize = 33;
            //defaultimp.maxBatchSize = 65;

            // textures
            /*
            if( m_terrainTemplate )
            {
                Array<SmartPtr<TerrainLayerTemplate>> textureLayers =
                    m_terrainTemplate->getTerrainLayers();
                defaultimp.layerList.resize( textureLayers.size() );

                for( u32 i = 0; i < textureLayers.size(); ++i )
                {
                    SmartPtr<TerrainLayerTemplate> textureLayer = textureLayers[i];

                    defaultimp.layerList[i].worldSize = textureLayer->getWorldSize();
                    defaultimp.layerList[i].textureNames.push_back(
                        textureLayer->getDiffuseTexture().c_str() );
                    defaultimp.layerList[i].textureNames.push_back(
                        textureLayer->getNormalTexture().c_str() );
                }
            }
            else
            {
                // defaultimp.layerList.resize(3);
                // defaultimp.layerList[0].worldSize = 10;
                // defaultimp.layerList[0].textureNames.push_back("dirt_grayrocky_diffusespecular.dds");
                // defaultimp.layerList[0].textureNames.push_back("dirt_grayrocky_normalheight.dds");
                // defaultimp.layerList[1].worldSize = 10;
                // defaultimp.layerList[1].textureNames.push_back("growth_weirdfungus-03_diffusespecular.dds");
                // defaultimp.layerList[1].textureNames.push_back("growth_weirdfungus-03_normalheight.dds");
                // defaultimp.layerList[2].worldSize = 20;
                // defaultimp.layerList[2].textureNames.push_back("growth_weirdfungus-03_diffusespecular.dds");
                // defaultimp.layerList[2].textureNames.push_back("growth_weirdfungus-03_normalheight.dds");
            }
             */
        }

        void CTerrainOgre::defineTerrain( long x, long y, bool flat )
        {
            // if a file is available, use it
            // if not, generate file from import

            // Usually in a real project you'll know whether the compact terrain data is
            // available or not; I'm doing it this way to save distribution size

            if( flat )
            {
                Ogre::Terrain::ImportData imp;

                //imp.pos = Ogre::Vector3::ZERO;
                //imp.inputImage = 0;
                //imp.inputFloat = 0;
                //imp.constantHeight = 0.0f;
                imp.terrainSize = 129;
                //imp.worldSize = 512;
                //imp.inputScale = 1;
                //imp.minBatchSize = 17;
                //imp.maxBatchSize = 129;

                imp.layerList.resize( 1 );
                imp.layerList[0].worldSize = 10;
                imp.layerList[0].textureNames.push_back( "checker.png" );
                //imp.layerList[0].textureNames.push_back( "checker.png" );

                mTerrainGroup->defineTerrain( x, y, &imp );

                mTerrainGroup->loadTerrain( x, y, true );

                //mTerrainGroup->defineTerrain( x, y, 0.0f );
            }
            else
            {
                Ogre::String filename = mTerrainGroup->generateFilename( x, y );
                if( Ogre::ResourceGroupManager::getSingleton().resourceExists(
                        mTerrainGroup->getResourceGroup(), filename ) )
                {
                    mTerrainGroup->defineTerrain( x, y );
                }
                else
                {
#if 0
					String filePath = m_terrainTemplate->getHeightMapPath();
					String fileNameExt = FileSystem::getFileExtension(filePath);

					Ogre::DataStreamPtr layerStream = Ogre::ResourceGroupManager::getSingletonPtr()->openResource(
						filePath.c_str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

					Ogre::Image img;
					img.load(layerStream, fileNameExt.subString(1, fileNameExt.length() - 1).c_str());
					img.flipAroundY();

					Ogre::PixelBox box = img.getPixelBox();


					int imageSize = std::max(img.getWidth(), img.getHeight());

					f32* pData = new float[mTerrainGroup->getTerrainSize() * mTerrainGroup->getTerrainSize()];
					f32* pPtr = pData;

					for (Ogre::uint16 y = 0; y < mTerrainGroup->getTerrainSize(); ++y)
					{
						for (Ogre::uint16 x = 0; x < mTerrainGroup->getTerrainSize(); ++x)
						{
							float xPos = (float)x / (float)mTerrainGroup->getTerrainSize();
							//float yPos = 1.0f - ((float)y / (float)mTerrainGroup->getTerrainSize());
							float yPos = (float)y / (float)mTerrainGroup->getTerrainSize();

							Ogre::ColourValue colourVal = box.getColourAt(xPos * (float)imageSize, yPos * (float)imageSize, 0);
							float val = *pPtr++ = colourVal.r;
							if (val < 0.0f || val > 1.0f)
								val = val;
						}
					}

					mTerrainGroup->defineTerrain(x, y, pData);

					delete[] pData;

					mTerrainsImported = true;
#else
                    Ogre::Image img;
                    // getTerrainImage(x % 2 != 0, y % 2 != 0, img);
                    if( getTerrainImage( false, false, img ) )
                    {
                        mTerrainGroup->defineTerrain( x, y, &img );
                    }
#endif
                }
            }
        }

        bool CTerrainOgre::getTerrainImage( bool flipX, bool flipY, Ogre::Image &img )
        {
            /*
            if( m_terrainTemplate )
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto fileSystem = applicationManager->getFileSystem();

                auto filePath = m_terrainTemplate->getHeightMap();
                if( fileSystem->isExistingFile( filePath ) )
                {
                    img.load( filePath.c_str(),
                              Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

                    if( flipX )
                        img.flipAroundY();

                    if( flipY )
                        img.flipAroundX();

                    return true;
                }
            }
             */

            return false;
        }

        void CTerrainOgre::initBlendMaps( Ogre::Terrain *terrain )
        {
            /*
            if( m_terrainTemplate )
            {
                Array<SmartPtr<TerrainLayerTemplate>> layers = m_terrainTemplate->getTerrainLayers();
                for( u32 i = 1; i < layers.size(); ++i )
                {
                    SmartPtr<TerrainLayerTemplate> layerTemplate = layers[i];

                    String fileName = layerTemplate->getBlendTexture();
                    String fileNameExt = Path::getFileExtension( fileName );

                    Ogre::TerrainLayerBlendMap *blendMap = terrain->getLayerBlendMap( i );

                    Ogre::DataStreamPtr layerStream =
                        Ogre::ResourceGroupManager::getSingletonPtr()->openResource(
                            fileName.c_str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

                    Ogre::Image img;
                    img.load( layerStream, fileNameExt.substr( 1, fileNameExt.length() - 1 ).c_str() );

                    Ogre::PixelBox box = img.getPixelBox();

                    float width = img.getWidth();
                    float height = img.getHeight();

                    // blendMap->loadImage(img);

                    float size = terrain->getLayerBlendMapSize();

                    float *pBlend0 = blendMap->getBlendPointer();
                    for( Ogre::uint16 y = 0; y < terrain->getLayerBlendMapSize(); ++y )
                    {
                        for( Ogre::uint16 x = 0; x < terrain->getLayerBlendMapSize(); ++x )
                        {
                            Ogre::ColourValue colourVal =
                                box.getColourAt( ( x / size ) * width, ( y / size ) * height, 0 );
                            // Ogre::ColourValue colourVal = box.getColourAt((x / size) * width, (1.0 -
                            // (y / size)) * height, 0);
                            float val = *pBlend0++ = colourVal.r;
                            if( val < 0.0f || val > 1.0f )
                                val = val;
                        }
                    }

                    blendMap->dirty();
                    blendMap->update();

                    // SmartPtr<CTerrainBlendMap> blendMap0_(new CTerrainBlendMap);
                    // blendMap0_->initialise(this, i);
                    // m_blendLayers.push_back(blendMap0_);
                }

                // for(u32 i=1; i<layers.size(); ++i)
                //{
                //	Ogre::TerrainLayerBlendMap* blendMap = terrain->getLayerBlendMap(i);
                //	blendMap->dirty();
                //	blendMap->update();
                // }
            }
             */
        }

        Array<f32> CTerrainOgre::getHeightData() const
        {
            Array<f32> data;

            Ogre::TerrainGroup::TerrainIterator ti = mTerrainGroup->getTerrainIterator();
            while( ti.hasMoreElements() )
            {
                Ogre::Terrain *t = ti.getNext()->instance;
                f32 *pData = t->getHeightData();
                u32 size = t->getSize() * t->getSize();
                data.reserve( size + data.size() );

                for( u32 i = 0; i < size; ++i )
                {
                    data.push_back( pData[i] );
                }
            }

            return data;
        }

        String CTerrainOgre::getMaterialName() const
        {
            Ogre::TerrainGroup::TerrainIterator ti = mTerrainGroup->getTerrainIterator();
            while( ti.hasMoreElements() )
            {
                Ogre::Terrain *t = ti.getNext()->instance;
                Ogre::String materialName = t->getMaterialName();
                return String( materialName.c_str() );
            }

            return StringUtil::EmptyString;
        }

        void CTerrainOgre::setMaterialName( const String &materialName )
        {
        }

        SmartPtr<ITerrainBlendMap> CTerrainOgre::getBlendMap( u32 index )
        {
            if( index >= m_blendLayers.size() )
            {
                return nullptr;
            }

            return m_blendLayers[index];
        }

        SmartPtr<ITerrainRayResult> CTerrainOgre::intersects( const Ray3F &ray ) const
        {
            Ogre::Ray orgeRay;
            orgeRay.setOrigin(
                Ogre::Vector3( ray.getOrigin().X(), ray.getOrigin().Y(), ray.getOrigin().Z() ) );
            orgeRay.setDirection( Ogre::Vector3( ray.getDirection().X(), ray.getDirection().Y(),
                                                 ray.getDirection().Z() ) );

            Ogre::TerrainGroup::RayResult rayResult = mTerrainGroup->rayIntersects( orgeRay );
            if( rayResult.hit )
            {
                SmartPtr<ITerrainRayResult> terrainRayResult( new CTerrainRayResult );
                terrainRayResult->setPosition(
                    Vector3F( rayResult.position.x, rayResult.position.y, rayResult.position.z ) );
                terrainRayResult->setIntersected( true );
                terrainRayResult->setTerrain( nullptr );

                return terrainRayResult;
            }

            return nullptr;
        }

        u16 CTerrainOgre::getLayerBlendMapSize() const
        {
            Ogre::TerrainGroup::TerrainIterator ti = mTerrainGroup->getTerrainIterator();
            while( ti.hasMoreElements() )
            {
                Ogre::Terrain *t = ti.getNext()->instance;
                return t->getLayerBlendMapSize();
            }

            return 0;
        }

        SmartPtr<IMesh> CTerrainOgre::getMesh() const
        {
            SmartPtr<IMesh> mesh( new Mesh );
            SmartPtr<ISubMesh> subMesh( new SubMesh );
            mesh->addSubMesh( subMesh );

            // create sub mesh data
            SmartPtr<IVertexDeclaration> vertexDeclaration( new VertexDeclaration );
            /*vertexDeclaration->addElement(sizeof(Vector3F), VertexDeclaration::VES_POSITION,
            VertexDeclaration::VET_FLOAT3); vertexDeclaration->addElement(sizeof(Vector3F),
            VertexDeclaration::VES_NORMAL, VertexDeclaration::VET_FLOAT3);
            vertexDeclaration->addElement(sizeof(Vector2F), VertexDeclaration::VES_TEXTURE_COORDINATES,
            VertexDeclaration::VET_FLOAT2, 0); vertexDeclaration->addElement(sizeof(Vector2F),
            VertexDeclaration::VES_TEXTURE_COORDINATES, VertexDeclaration::VET_FLOAT2, 1);
            */
            SmartPtr<IVertexBuffer> vertexBuffer( new VertexBuffer );
            // vertexBuffer->setVertexDeclaration(vertexDeclaration);

            u32 tileSize = 512;
            u32 numVerticies = tileSize * tileSize;

            vertexBuffer->setNumVertices( numVerticies );
            auto vertexData = static_cast<f32 *>( vertexBuffer->createVertexData() );
            f32 *vertexDataPtr = vertexData;

            return mesh;
        }

        Ogre::TerrainGroup *CTerrainOgre::getTerrainGroup() const
        {
            return mTerrainGroup;
        }

        void CTerrainOgre::setTerrainGroup( Ogre::TerrainGroup *terrainGroup )
        {
            mTerrainGroup = terrainGroup;
        }

        f32 CTerrainOgre::getHeightScale() const
        {
            return 0.0f;
        }

        void CTerrainOgre::setHeightScale( f32 heightScale )
        {
            if( mTerrainGroup )
            {
                Ogre::Terrain::ImportData &defaultimp = mTerrainGroup->getDefaultImportSettings();
                defaultimp.inputScale = heightScale;

                Ogre::TerrainGroup::TerrainIterator ti = mTerrainGroup->getTerrainIterator();
                while( ti.hasMoreElements() )
                {
                    Ogre::Terrain *t = ti.getNext()->instance;
                    t->update();
                }
            }
        }

        void CTerrainOgre::_getObject( void **ppObject ) const
        {
            *ppObject = mTerrainGroup;
        }

        SmartPtr<IGraphicsScene> CTerrainOgre::getSceneManager() const
        {
            return m_sceneManager;
        }

        void CTerrainOgre::setSceneManager( SmartPtr<IGraphicsScene> sceneManager )
        {
            m_sceneManager = sceneManager;
        }

        Ogre::SceneManager *CTerrainOgre::getOgreSceneManager() const
        {
            if( auto scene = getSceneManager() )
            {
                Ogre::SceneManager *smgr = nullptr;
                scene->_getObject( (void **)&smgr );
                return smgr;
            }

            return nullptr;
        }

        Ogre::Camera *CTerrainOgre::getTerrainCamera() const
        {
            return m_terrainCamera;
        }

        void CTerrainOgre::setTerrainCamera( Ogre::Camera *terrainCamera )
        {
            m_terrainCamera = terrainCamera;
        }

        Ogre::SceneNode *CTerrainOgre::getTerrainCameraSceneNode() const
        {
            return m_terrainCameraSceneNode;
        }

        void CTerrainOgre::setTerrainCameraSceneNode( Ogre::SceneNode *terrainCameraSceneNode )
        {
            m_terrainCameraSceneNode = terrainCameraSceneNode;
        }

        PagedGeometry *CTerrainOgre::getTrees() const
        {
            return m_trees;
        }

        void CTerrainOgre::setTrees( PagedGeometry *trees )
        {
            m_trees = trees;
        }

        u16 CTerrainOgre::getTerrainSize() const
        {
            return m_terrainSize;
        }

        void CTerrainOgre::setTerrainSize( u16 terrainSize )
        {
            m_terrainSize = terrainSize;
        }

        f32 CTerrainOgre::getTerrainWorldSize() const
        {
            return m_terrainWorldSize;
        }

        void CTerrainOgre::setTerrainWorldSize( f32 terrainWorldSize )
        {
            m_terrainWorldSize = terrainWorldSize;
        }

        void CTerrainOgre::setTextureLayer( s32 layer, const String &textureName )
        {
            auto terrain = mTerrainGroup->getTerrain( 0, 0 );
            terrain->setLayerTextureName( layer, 0, textureName.c_str() );
        }

        void CTerrainOgre::loadTrees()
        {
            try
            {
                using namespace Ogre;

                Camera *ogreCamera = getTerrainCamera();

                auto applicationManager = core::IApplicationManager::instance();
                auto graphicsSystem = applicationManager->getGraphicsSystem();
                auto window = graphicsSystem->getDefaultWindow();
                //auto viewports = window->getViewports();
                //for( auto viewport : viewports )
                //{
                //    if( viewport )
                //    {
                //        auto camera = viewport->getCamera();
                //        if( camera )
                //        {
                //            camera->_getObject( (void **)&ogreCamera );
                //        }
                //    }
                //}

                //if( !ogreCamera )
                //{
                //    WP_LOG_ERROR( "CTerrain::loadTrees No camera found" );
                //    return;
                //}

                //-------------------------------------- LOAD TREES
                //--------------------------------------
                // Create and configure a new PagedGeometry instance
                m_trees = new PagedGeometry();
                m_trees->setCamera(
                    ogreCamera );  // Set the camera so PagedGeometry knows how to calculate LODs
                m_trees->setPageSize( 80 );  // Set the size of each page of geometry
                m_trees->setInfinite();      // Use infinite paging mode
                m_trees->addDetailLevel<BatchPage>(
                    150, 50 );  // Use batches up to 150 units away, and fade for 30 more units
                //m_trees->addDetailLevel<ImpostorPage>(
                //    500, 50 );  // Use impostors up to 400 units, and for for 50 more units

                auto terrainWorldSize = getTerrainWorldSize();

                // Create a new TreeLoader3D object
                auto bounds =
                    TBounds( -terrainWorldSize, -terrainWorldSize, terrainWorldSize, terrainWorldSize );
                auto treeLoader = new TreeLoader3D( m_trees, bounds );

                // Assign the "treeLoader" to be used to load geometry
                m_trees->setPageLoader( treeLoader );
                // for the PagedGeometry instance

                auto sceneMgr = getOgreSceneManager();

                // Load a tree entity
                auto treeName = String( "Tree" ) + StringUtil::toString( m_ext++ );
                auto myEntity = sceneMgr->createEntity( treeName.c_str(), "fir05_30.mesh" );

                // Setup the height function (so the Y values of trees can be calculated when they are
                // placed on the terrain)
                HeightFunction::initialize( mTerrainGroup );

                // Randomly place 20,000 copies of the tree on the terrain
                for( int i = 0; i < 1000; i++ )
                {
                    auto yaw = Degree( Ogre::Math::RangeRandom( 0, 360 ) );

                    Ogre::Vector3 position;
                    position.x = Ogre::Math::RangeRandom( -terrainWorldSize, terrainWorldSize );
                    position.z = Ogre::Math::RangeRandom( -terrainWorldSize, terrainWorldSize );
                    position.y = HeightFunction::getTerrainHeight( position.x, position.z );

                    auto scale = Ogre::Math::RangeRandom( 0.2f, 0.6f );

                    treeLoader->addTree( myEntity, position, yaw, scale );
                }

                setTreeMask( m_trees->getSceneNode() );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CTerrainOgre::setTreeMask( Ogre::SceneNode *node )
        {
            auto mask = static_cast<u32>( 0 );
            mask = BitUtil::setFlagValue( mask, IGraphicsObject::SceneFlag, true );

            auto attachedObjects = node->getAttachedObjects();
            for( auto obj : attachedObjects )
            {
                obj->setVisibilityFlags( mask );
            }

            auto children = node->getChildren();
            for( auto child : children )
            {
                setTreeMask( static_cast<Ogre::SceneNode *>( child ) );
            }
        }

        CTerrainOgre::TerrainStateListener::TerrainStateListener() = default;

        CTerrainOgre::TerrainStateListener::~TerrainStateListener() = default;

        void CTerrainOgre::TerrainStateListener::unload( SmartPtr<ISharedObject> data )
        {
            m_owner = nullptr;
        }

        bool CTerrainOgre::TerrainStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            if( auto owner = getOwner() )
            {
                if( message->isDerived<StateMessageLoad>() )
                {
                    auto loadMsg = workphone::static_pointer_cast<StateMessageLoad>( message );
                    auto type = loadMsg->getType();

                    if( type == StateMessageLoad::LOAD_HASH )
                    {
                        owner->load( nullptr );
                    }
                    else if( type == StateMessageLoad::RELOAD_HASH )
                    {
                        owner->load( nullptr );
                    }
                }
            }

            return false;
        }

        bool CTerrainOgre::TerrainStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            if( auto owner = getOwner() )
            {
                auto stateData = state->getData();
                if( stateData )
                {
                    if( stateData->isDerived<TransformStateData>() )
                    {
                    }
                    else if( stateData->isDerived<TerrainStateData>() )
                    {
                        auto terrainState = SafeReadPtr<TerrainStateData>( state->getData() );
                        auto visible = true;
                        //BitUtil::getFlagValue( terrainState->flags, IGraphicsObject::visibleFlag );

                        auto terrainGroup = owner->getTerrainGroup();
                        if( terrainGroup )
                        {
                            auto terrain = terrainGroup->getTerrain( 0, 0 );
                            auto node = terrain->getQuadTree();

                            node->setVisible( visible );

                            terrainGroup->update();

                            return true;
                        }
                    }
                }
            }

            return false;
        }

        SmartPtr<CTerrainOgre> CTerrainOgre::TerrainStateListener::getOwner() const
        {
            auto p = m_owner.load();
            return p.lock();
        }

        void CTerrainOgre::TerrainStateListener::setOwner( SmartPtr<CTerrainOgre> owner )
        {
            m_owner = owner;
        }

        void CTerrainOgre::ShadowRenderTargetListener::preRenderTargetUpdate(
            const Ogre::RenderTargetEvent &evt )
        {
            if( m_nextUpdate > 5 )
            {
                m_tex->getBuffer()->blitFromMemory( *box );
                m_nextUpdate = 0;
            }

            ++m_nextUpdate;
        }

        CTerrainOgre::ShadowRenderTargetListener::ShadowRenderTargetListener( Ogre::TexturePtr tex ) :
            m_tex( tex ),
            m_nextUpdate( 0 )
        {
            float floatMax = 1000.0f * 1000.0f;
            int maxValue = *reinterpret_cast<int *>( &floatMax );

            int size = m_tex->getWidth() * m_tex->getHeight() * sizeof( float ) * 2;
            data = new float[size];
            // memset((void*)data,maxValue,m_tex->getWidth()*m_tex->getHeight()*sizeof(float)*2);
            for( int i = 0; i < size; i += 2 )
            {
                data[i] = 1000.0f;
                data[i + 1] = 1000.0f * 1000.0f;
            }

            box = new Ogre::PixelBox( m_tex->getWidth(), m_tex->getHeight(), m_tex->getDepth(),
                                      m_tex->getFormat(), data );
        }

        CTerrainOgre::ShadowRenderTargetListener::~ShadowRenderTargetListener()
        {
            delete[] data;
        }

        void CTerrainOgre::ShadowRenderTargetListener::preViewportUpdate(
            const Ogre::RenderTargetViewportEvent &evt )
        {
        }
    }  // namespace render
}  // namespace workphone
