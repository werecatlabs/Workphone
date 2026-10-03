#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include "WPGraphicsOgre/Addons/LightShafts.hpp"
#include "WPGraphicsOgre/Core/LightDepthMapRttListener.hpp"
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>

namespace workphone
{
    namespace render
    {

        LightShafts::LightShafts( SmartPtr<render::IGraphicsScene> smgr, Ogre::Camera *camera ) :
            mCamera( camera )
        {
            smgr->_getObject( (void **)&mSceneMgr );
            m_smgr = smgr;
        }

        LightShafts::~LightShafts()
        {
        }

        void LightShafts::createScene( void )
        {
            // Set up light 0
            // Ogre::Light *mLight0 = mSceneMgr->getLight("testLight");

            Ogre::Vector3 lightdir( 0.55, -0.3, 0.75 );
            lightdir.normalise();

            // Set up our light camera
            mLightCamera = mSceneMgr->createCamera( "LightCamera" );
            mLightCamera->setProjectionType( Ogre::PT_PERSPECTIVE );
            // Not forget to set near+far distance in materials
            mLightCamera->setNearClipDistance( 8 );
            mLightCamera->setFarClipDistance( 1000 );
            mLightCamera->setAspectRatio( 1 );
            mLightCamera->setFOVy( Ogre::Degree( 17.5f ) );
            mLightCamera->setDebugDisplayEnabled( true );
            mLightCamera->setVisible( true );

            // m_lightCameraNode = m_smgr->addSceneNode("LightCamera");
            // m_lightCameraNode->_getObject((void**)&mLightCameraSN);
            mLightCameraSN = mSceneMgr->getRootSceneNode()->createChildSceneNode( "LightCamera" );

            Ogre::Vector3 pos = lightdir * -50.0f;
            mLightCameraSN->setPosition( pos );
            mLightCameraSN->attachObject( mLightCamera );
            mLightCameraSN->setDirection( lightdir );

            // Create our billboardset for volumetric rendering
            mBillboardSet = mSceneMgr->createBillboardSet( "LightBillboardSet", 1 );
            mBillboardSet->setMaterialName( "LightShafts" );
            mBillboardSet->setBillboardRotationType( Ogre::BBR_VERTEX );
            mBillboardSet->setCastShadows( false );
            mLightCameraSN->attachObject( mBillboardSet );

            // Create a rush of billboards according to the frustum of the camera(mLightCamera)
            // After it, we can use the lightcamera/billboards scenenode like a light projector
            createLightShafts( mBillboardSet, mLightCamera, 100 );

            // Creating a RRT for depth/shadow map
            createLightCameraRTT();

            // Set a floor plane
            // Ogre::MovablePlane *mPlane = new Ogre::MovablePlane("FloorPlane");
            // mPlane->normal = Ogre::Vector3::UNIT_Y;
            // mPlane->d = 0;
            // Ogre::MeshManager::getSingleton().createPlane("FloorPlaneMesh",
            //	Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, *mPlane,
            //	1500,1500,50,50,true,1,5,5,Ogre::Vector3::UNIT_Z);

            // Ogre::Entity *pPlaneEnt = mSceneMgr->createEntity("Plane", "FloorPlaneMesh");
            // pPlaneEnt->setMaterialName("Examples/OffsetMapping/Specular/Floor");
            // pPlaneEnt->setCastShadows(false);
            // Ogre::SceneNode* pPlaneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
            // pPlaneNode->attachObject(pPlaneEnt);
            // pPlaneNode->setPosition(0,0,0);

            // Set a knot
            /*Ogre::Entity *mKnot = mSceneMgr->createEntity("Knot", "knot.mesh");
            mKnot->setMaterialName("Knot");
            mKnotSN = mSceneMgr->getRootSceneNode()->createChildSceneNode();
            mKnotSN->attachObject(mKnot);
            mKnotSN->setPosition(0, 17.5f, 0);
            mKnotSN->setScale(0.0225, 0.0225, 0.0225);*/

            // Add frame listener
            LightShaftsListener *listener = new LightShaftsListener( mCamera, mSceneMgr );
            listener->mLightCameraSN = mLightCameraSN;
            listener->mLightCamera = mLightCamera;
            Ogre::Root::getSingletonPtr()->addFrameListener( listener );
        }

        bool LightShafts::createLightShafts( Ogre::BillboardSet *Billboard, Ogre::Camera *LightCamera,
                                             const int &NumberOfPlanes )
        {
            // Calculate the distance between planes
            float DistanceBetweenPlanes =
                ( LightCamera->getFarClipDistance() - LightCamera->getNearClipDistance() ) /
                NumberOfPlanes;

            // Get frustum corners to calculate near/far planes dimensions
            const Ogre::Vector3 *FrustumCorners = LightCamera->getWorldSpaceCorners();

            // Calcule near and far planes dimensions
            float NearWidth = ( FrustumCorners[0] - FrustumCorners[1] ).length(),
                  NearHeigth = ( FrustumCorners[1] - FrustumCorners[2] ).length(),
                  FarWidth = ( FrustumCorners[4] - FrustumCorners[5] ).length(),
                  FarHeigth = ( FrustumCorners[5] - FrustumCorners[6] ).length();

            // Now width/heigth setp
            float WidthStep = ( FarWidth - NearWidth ) / NumberOfPlanes,
                  HeigthStep = ( FarHeigth - NearHeigth ) / NumberOfPlanes;

            // Add billboards
            Ogre::Billboard *CurrentBB = 0;
            for( int k = 0; k < NumberOfPlanes; k++ )
            {
                CurrentBB = Billboard->createBillboard(
                    Ogre::Vector3( 0, 0,
                                   -LightCamera->getNearClipDistance() - k * DistanceBetweenPlanes ),
                    Ogre::ColourValue::White );
                CurrentBB->setDimensions( NearWidth + k * WidthStep, NearHeigth + k * HeigthStep );
            }

            return true;
        }

        void LightShafts::createLightCameraRTT()
        {
            // Creat a texture for use as rtt
            Ogre::TexturePtr LightCameraRTT = Ogre::TextureManager::getSingleton().createManual(
                "LightDepthMap", "General", Ogre::TEX_TYPE_2D,
                // 256*256 must be sufficient
                256, 256, 0, Ogre::PF_FLOAT16_R, Ogre::TU_RENDERTARGET );

            Ogre::RenderTarget *RT_Texture = LightCameraRTT->getBuffer()->getRenderTarget();

            Ogre::Viewport *RT_Texture_Viewport = RT_Texture->addViewport( mLightCamera );
            RT_Texture_Viewport->setClearEveryFrame( true );
            RT_Texture_Viewport->setBackgroundColour( Ogre::ColourValue::White );
            RT_Texture_Viewport->setOverlaysEnabled( false );
            RT_Texture_Viewport->setSkiesEnabled( false );

            // Add our depth listener
            LightDepthMapRttListener *depthListener = new LightDepthMapRttListener( mSceneMgr );
            depthListener->mLightCamera = mLightCamera;
            depthListener->mLightCameraSN = mLightCameraSN;
            depthListener->mBillboardSet = mBillboardSet;
            RT_Texture->addListener( depthListener );

            // Fill the texture in our material
            static_cast<Ogre::MaterialPtr>(
                Ogre::MaterialManager::getSingleton().getByName( "LightShafts" ) )
                ->getTechnique( 0 )
                ->getPass( 0 )
                ->getTextureUnitState( 0 )
                ->setTextureName( "LightDepthMap" );
        }

        LightShafts::LightShaftsListener::LightShaftsListener( Ogre::Camera *cam,
                                                               Ogre::SceneManager *sm ) :
            mSceneMgr( sm ),
            mKeyBuffer( -1 ),
            mRotateEnable( true ),
            mRotateKnot( false )
        {
        }

        bool LightShafts::LightShaftsListener::frameStarted( const Ogre::FrameEvent &e )
        {
            static int m_nextUpdate = 0;
            // Update light position
            if( m_nextUpdate > 5 )
            {
                updatePosition( e );
                m_nextUpdate = 0;
            }

            ++m_nextUpdate;

            return true;
        }

        void LightShafts::LightShaftsListener::updatePosition( const Ogre::FrameEvent &e )
        {
            // Just a simple circular trajectory
            const Ogre::Real &SimulationTime = Ogre::Root::getSingleton().getTimer()->getMilliseconds();
            Ogre::Real Radius = 8;

            if( !mRotateEnable )
            {
                Radius = 0;
            }

            Ogre::Camera *camera = mSceneMgr->getCamera( "DefaultCamera" );
            mLightCameraSN->lookAt( camera->getRealPosition(), Ogre::Node::TS_WORLD );

            // mLightCameraSN->setPosition(Ogre::Math::Sin(SimulationTime/1000)*Radius,
            //	mLightCameraSN->getPosition().y,
            //	Ogre::Math::Cos(SimulationTime/1000)*Radius);

            // Set the the scene node direction to 0,KnotHeight,0 point
            // Ogre::Vector3 NormalisedDirection =
            // (Ogre::Vector3(0,mKnotSN->getPosition().y,0)-mLightCameraSN->getPosition()).normalisedCopy();
            // mLightCameraSN->setDirection(NormalisedDirection, Ogre::Node::TS_WORLD);

            // Upload current position to light shafts materials
            static_cast<Ogre::MaterialPtr>(
                Ogre::MaterialManager::getSingleton().getByName( "LightShafts" ) )
                ->getTechnique( 0 )
                ->getPass( 0 )
                ->getFragmentProgramParameters()
                ->setNamedConstant( "uLightPosition", mLightCameraSN->getPosition() );
            static_cast<Ogre::MaterialPtr>(
                Ogre::MaterialManager::getSingleton().getByName( "LightShaftsDepth" ) )
                ->getTechnique( 0 )
                ->getPass( 0 )
                ->getFragmentProgramParameters()
                ->setNamedConstant( "uLightPosition", mLightCameraSN->getPosition() );

            // We've to update the texture projection matrix
            updateTextureProjectionMatrix();
        }

        void LightShafts::LightShaftsListener::updateTextureProjectionMatrix()
        {
            const Ogre::Matrix4 PROJECTIONCLIPSPACE2DTOIMAGESPACE_PERSPECTIVE(
                0.5, 0, 0, 0.5, 0, -0.5, 0, 0.5, 0, 0, 1, 0, 0, 0, 0, 1 );

            Ogre::Matrix4 TexViewProj = PROJECTIONCLIPSPACE2DTOIMAGESPACE_PERSPECTIVE *
                                        mLightCamera->getProjectionMatrixWithRSDepth() *
                                        mLightCamera->getViewMatrix();

            static_cast<Ogre::MaterialPtr>(
                Ogre::MaterialManager::getSingleton().getByName( "LightShafts" ) )
                ->getTechnique( 0 )
                ->getPass( 0 )
                ->getVertexProgramParameters()
                ->setNamedConstant( "uTexViewProj", TexViewProj );
        }

    }  // end namespace render
}  // namespace workphone
