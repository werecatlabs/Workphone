#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include "WPGraphicsOgre/Addons/VolumetricSunLightFrameListener.hpp"

namespace workphone
{

    Ogre::Vector3 saturate( const Ogre::Vector3 &point, const Ogre::Camera &camera )
    {
        // Transform the 3D point into screen space
        Ogre::Vector3 result = camera.getProjectionMatrix() * ( camera.getViewMatrix() * point );

        // Transform from coordinate space [-1, 1] to [0, 1] and update in-value
        result.x = ( result.x / 2 ) + 0.5f;
        result.y = 1 - ( ( result.y / 2 ) + 0.5f );
        return result;
    }

    VolumetricSunLightFrameListener::VolumetricSunLightFrameListener( Ogre::SceneManager *pSceneManager,
                                                                      Ogre::Camera *mCamera,
                                                                      Ogre::Viewport *pViewport )
    {
        m_pSceneManager = pSceneManager;
        m_pSunLight = NULL;
        m_pMainCamera = mCamera;
        m_pMainViewport = pViewport;

        alphaXZ = 0;
        alphaZY = 0;
    }

    VolumetricSunLightFrameListener::~VolumetricSunLightFrameListener()
    {
    }

    void VolumetricSunLightFrameListener::notifyMaterialSetup( Ogre::uint32 pass_id,
                                                               Ogre::MaterialPtr &mat )
    {
        if( pass_id == 1 )
        {
            fpParams = mat->getTechnique( 0 )->getPass( 0 )->getFragmentProgramParameters();
        }
    }

    void VolumetricSunLightFrameListener::notifyMaterialRender( Ogre::uint32 pass_id,
                                                                Ogre::MaterialPtr &mat )
    {
        if( pass_id == 1 )
        {
            if( !m_pSceneManager->hasLight( "Light0" ) )
            {
                m_pSunLight = 0;
                return;
            }

            m_pSunLight = m_pSceneManager->getLight( "Light0" );

            // if( !m_pMainCamera )
            //	m_pMainCamera = m_pSceneManager->getCamera(MAIN_CAMERA_NAME);

            if( m_pSunLight && m_pMainCamera && m_pMainViewport )
            {
                Ogre::Vector3 vCamPos = m_pMainCamera->getRealPosition();
                // Ogre::Vector3 vSunPos = m_pSunLight->getDirection() * -50.0f;
                Ogre::Vector3 vSunPos = Ogre::Vector3::UNIT_Y;  // m_pSunLight->getDirection() * -50.0f;
                Ogre::Vector3 camToLightVector = vSunPos - vCamPos;

                Ogre::Vector3 vCameraDir = m_pMainCamera->getDerivedDirection();
                Ogre::Radian rAngle = vCameraDir.angleBetween( camToLightVector );

                Ogre::Vector3 lightInCamSpace;
                Ogre::Vector3 hcsPosition = m_pMainCamera->getProjectionMatrix() *
                                            m_pMainCamera->getViewMatrix() * camToLightVector;
                lightInCamSpace.x = 0.5 - hcsPosition.x * 0.5f;
                lightInCamSpace.x = 1.0 - lightInCamSpace.x;
                lightInCamSpace.y = hcsPosition.y * 0.5f + 0.5;
                lightInCamSpace.y = 1.0 - lightInCamSpace.y;

                // if( lightInCamSpace.x < -1 )
                //{
                //	lightInCamSpace.x = 1 + lightInCamSpace.x;
                //	lightInCamSpace.y = 1 - lightInCamSpace.y;
                // }
                // lightInCamSpace = saturate( camToLightVector , *m_pMainCamera);

                lightInCamSpace.z = camToLightVector.z;
                fpParams->setNamedConstant( "screenLightPos", lightInCamSpace );

                float fDensity;
                Ogre::Real fAngle = rAngle.valueDegrees();
                if( fAngle < 90 )
                {
                    fDensity = Ogre::Math::Sin( Ogre::Math::HALF_PI +
                                                Ogre::Radian( Ogre::Degree( fAngle ) ).valueRadians() );
                    float delta = fAngle * (float)( 0.1f / 90.0f );
                    fDensity += delta;
                }
                else
                {
                    fDensity = Ogre::Math::Sin( Ogre::Math::HALF_PI +
                                                Ogre::Radian( Ogre::Degree( fAngle ) ).valueRadians() );
                    float delta = fAngle * (float)( 0.1f / 90.0f );
                    fDensity += delta;
                    if( fDensity < 0.1 )
                        fDensity = 0.1;
                }

                fpParams->setNamedConstant( "density", fDensity );
            }
        }
    }

}  // namespace workphone
