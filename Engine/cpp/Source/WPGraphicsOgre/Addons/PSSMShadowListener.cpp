#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include "WPGraphicsOgre/Addons/PSSMShadowListener.hpp"
#include "ShadowCameraSetupStableCSM.hpp"

using namespace Ogre;

namespace workphone
{

    PSSMShadowListener::PSSMShadowListener( Ogre::SceneManager *sm, Ogre::Light *l,
                                            ShadowCameraSetupPtr s, Camera *cam )
    {
        sceneMgr = sm;
        light = l;
        setup = s;
        view_camera = cam;
        split_index = 0;
    }

    PSSMShadowListener::~PSSMShadowListener()
    {
    }

    void PSSMShadowListener::shadowTexturesUpdated( size_t numberOfShadowTextures )
    {
    }

    void PSSMShadowListener::shadowTextureCasterPreViewProj( Light *, Camera *camera, size_t iteration )
    {
        // StableCSMShadowCameraSetup* csmSetup = (StableCSMShadowCameraSetup*)setup.get();
        // StableCSMShadowCameraSetup::SplitPointList splitList = csmSetup->getSplitPoints();

        Array<float> splitList;
        splitList.push_back( 0 );
        splitList.push_back( 0.001 );
        splitList.push_back( 15.0 );
        splitList.push_back( 30.0 );
        splitList.push_back( 60.0 );
        splitList.push_back( 90.0 );

        static bool update = true;
        float old_near = view_camera->getNearClipDistance();
        float old_far = view_camera->getFarClipDistance();
        if( split_index > 0 )
        {
            view_camera->setNearClipDistance( splitList[split_index] );
            view_camera->setFarClipDistance( splitList[split_index + 1] );
        }
        else
        {
            view_camera->setNearClipDistance( splitList[split_index + 1] );
            view_camera->setFarClipDistance( splitList[split_index + 2] );
        }

        // light->setShadowFarDistance(20*(split_index+1));
        // camera->setWindow(0.0, 0.0, iteration * (1.0/3.0), 1.0);

        if( update )
            setup->getShadowCamera( sceneMgr, view_camera, NULL, light, camera, iteration );

        view_camera->setNearClipDistance( old_near );
        view_camera->setFarClipDistance( old_far );

        // camera->setWindow(0.0, 0.0, 1.0, 1.0);

        split_index++;
        if( split_index > 2 )
            split_index = 0;
    }

    void PSSMShadowListener::shadowTextureReceiverPreViewProj( Ogre::Light *light,
                                                               Ogre::Frustum *frustum )
    {
    }

    struct lightsLess
    {
        bool operator()( const Light *l1, const Light *l2 ) const
        {
            if( l1 == l2 )
                return false;
            return l1->tempSquareDist < l2->tempSquareDist;
        }
    };

    bool PSSMShadowListener::sortLightsAffectingFrustum( Ogre::LightList &lightList )
    {
        std::stable_sort( lightList.begin(), lightList.end(), lightsLess() );
        return true;
    }

    void shadowTextureReceiverPreViewProj( Ogre::Light *light, Ogre::Frustum *frustum )
    {
    }

}  // namespace workphone
