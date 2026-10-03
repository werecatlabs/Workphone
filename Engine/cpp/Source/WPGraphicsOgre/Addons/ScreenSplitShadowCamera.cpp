#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>

#include <Ogre.h>
#include "WPGraphicsOgre/Addons/ScreenSplitShadowCamera.hpp"
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{

    /// Default constructor
    ScreenSplitShadowCamera::ScreenSplitShadowCamera()
    {
    }

    /// Destructor
    ScreenSplitShadowCamera::~ScreenSplitShadowCamera()
    {
    }

    /// Default shadow camera setup implementation
    void ScreenSplitShadowCamera::getShadowCamera( const Ogre::SceneManager *sm, const Ogre::Camera *cam,
                                                   const Ogre::Viewport *vp, const Ogre::Light *light,
                                                   Ogre::Camera *texCam, size_t iteration ) const
    {
        using namespace Ogre;

        Ogre::Vector3 pos, dir;

        // reset custom view / projection matrix in case already set
        texCam->setCustomViewMatrix( false );
        texCam->setCustomProjectionMatrix( false );
        texCam->setNearClipDistance( light->_deriveShadowNearClipDistance( cam ) );
        texCam->setFarClipDistance( light->_deriveShadowFarClipDistance( cam ) );

        Array<Vector2I> indexs;
        indexs.resize( 9 );
        indexs[0] = Vector2I::ZERO;
        indexs[1] = Vector2I( 1, 0 );
        indexs[2] = Vector2I( 1, 1 );
        indexs[3] = Vector2I( 0, 1 );
        indexs[4] = Vector2I( -1, 1 );
        indexs[5] = Vector2I( -1, 0 );
        indexs[6] = Vector2I( -1, -1 );
        indexs[7] = Vector2I( 0, -1 );
        indexs[8] = Vector2I( 1, -1 );

        // get the shadow frustum's far distance
        f32 shadowDist = light->getShadowFarDistance();
        if( !shadowDist )
        {
            // need a shadow distance, make one up
            shadowDist = cam->getNearClipDistance() * 300;
        }

        Real shadowOffset = shadowDist * ( sm->getShadowDirLightTextureOffset() );

        // Directional lights
        if( light->getType() == Light::LT_DIRECTIONAL )
        {
            // double splitSize = 1.0/(double)sm->getShadowTextureCount();
            double splitSize = 1.0;

            // set up the shadow texture
            // Set ortho projection
            texCam->setProjectionType( PT_ORTHOGRAPHIC );

            Vector2I offset = indexs[iteration];

            texCam->setOrthoWindow( shadowDist * 2.0f, shadowDist * 2.0f );

            Ogre::Vector3 offsetUp = Ogre::Vector3::ZERO;
            offsetUp += shadowDist * 2 * offset.Y() * -texCam->getDerivedUp();
            offsetUp += shadowDist * 2 * offset.X() * texCam->getDerivedRight();

            Ogre::Vector3 target =
                ( cam->getDerivedPosition() + offsetUp ) + ( cam->getDerivedDirection() * shadowOffset );

            // Calculate direction, which same as directional light direction
            dir = -light->getDerivedDirection();  // backwards since point down -z
            dir.normalise();

            // Calculate position
            // We want to be in the -ve direction of the light direction
            // far enough to project for the dir light extrusion distance
            pos = target + dir * sm->getShadowDirectionalLightExtrusionDistance();

            // Round local x/y position based on a world-space texel; this helps to reduce
            // jittering caused by the projection moving with the camera
            // Viewport is 2 * near clip distance across (90 degree fov)
            //~ Real worldTexelSize = (texCam->getNearClipDistance() * 20) / vp->getActualWidth();
            //~ pos.x -= fmod(pos.x, worldTexelSize);
            //~ pos.y -= fmod(pos.y, worldTexelSize);
            //~ pos.z -= fmod(pos.z, worldTexelSize);
            Real worldTexelSize = ( shadowDist * 2 ) / texCam->getViewport()->getActualWidth();

            // get texCam orientation

            Ogre::Vector3 up = Ogre::Vector3::UNIT_Y;
            // Check it's not coincident with dir
            if( Ogre::Math::Abs( up.dotProduct( dir ) ) >= 1.0f )
            {
                // Use camera up
                up = Ogre::Vector3::UNIT_Z;
            }
            // cross twice to rederive, only direction is unaltered
            Ogre::Vector3 left = dir.crossProduct( up );
            left.normalise();
            up = dir.crossProduct( left );
            up.normalise();
            // Derive quaternion from axes
            Ogre::Quaternion q;
            q.FromAxes( left, up, dir );

            // convert world space camera position into light space
            Ogre::Vector3 lightSpacePos = q.Inverse() * pos;

            // snap to nearest texel
            lightSpacePos.x -= fmod( lightSpacePos.x, worldTexelSize );
            lightSpacePos.y -= fmod( lightSpacePos.y, worldTexelSize );

            // convert back to world space
            pos = q * lightSpacePos;

            //// Finally set position
            // texCam->setPosition(pos);

            // Ogre::Matrix4 viewMatrix = cam->getProjectionMatrix();
            // Ogre::Vector3 viewPos = viewMatrix * pos;
            // viewPos = viewPos;

            // texCam->setFrustumExtents(iteration * (1.0/3.0), 0.0, (iteration+1) * (1.0/3.0), 1.0);
            // texCam->setFrustumOffset(iteration * (1.0/3.0), 0.0);
        }
        // Spotlight
        else if( light->getType() == Light::LT_SPOTLIGHT )
        {
            // Set perspective projection
            texCam->setProjectionType( PT_PERSPECTIVE );
            // set FOV slightly larger than the spotlight range to ensure coverage
            Radian fovy = light->getSpotlightOuterAngle() * 1.2;
            // limit angle
            if( fovy.valueDegrees() > 175 )
                fovy = Degree( 175 );
            texCam->setFOVy( fovy );

            // Calculate position, which same as spotlight position
            pos = light->getDerivedPosition();

            // Calculate direction, which same as spotlight direction
            dir = -light->getDerivedDirection();  // backwards since point down -z
            dir.normalise();

            //// Finally set position
            // texCam->setPosition(pos);
        }
        // Point light
        else
        {
            // Set perspective projection
            texCam->setProjectionType( PT_PERSPECTIVE );
            // Use 120 degree FOV for point light to ensure coverage more area
            texCam->setFOVy( Degree( 120 ) );

            // Calculate look at position
            // We want to look at a spot shadowOffset away from near plane
            // 0.5 is a litle too close for angles
            Ogre::Vector3 target =
                cam->getDerivedPosition() + ( cam->getDerivedDirection() * shadowOffset );

            // Calculate position, which same as point light position
            pos = light->getDerivedPosition();

            dir = ( pos - target );  // backwards since point down -z
            dir.normalise();

            //// Finally set position
            // texCam->setPosition(pos);
        }

        // Calculate orientation based on direction calculated above
        /*
        // Next section (camera oriented shadow map) abandoned
        // Always point in the same direction, if we don't do this then
        // we get 'shadow swimming' as camera rotates
        // As it is, we get swimming on moving but this is less noticeable

        // calculate up vector, we want it aligned with cam direction
        Vector3 up = cam->getDerivedDirection();
        // Check it's not coincident with dir
        if (up.dotProduct(dir) >= 1.0f)
        {
        // Use camera up
        up = cam->getUp();
        }
        */
        Ogre::Vector3 up = Ogre::Vector3::UNIT_Y;
        // Check it's not coincident with dir
        if( Ogre::Math::Abs( up.dotProduct( dir ) ) >= 1.0f )
        {
            // Use camera up
            up = Ogre::Vector3::UNIT_Z;
        }
        // cross twice to rederive, only direction is unaltered
        Ogre::Vector3 left = dir.crossProduct( up );
        left.normalise();
        up = dir.crossProduct( left );
        up.normalise();
        // Derive quaternion from axes
        Ogre::Quaternion q;
        q.FromAxes( left, up, dir );
        // texCam->setOrientation(q);
    }

}  // namespace workphone
