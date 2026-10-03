#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CDeferredShadingSystem.hpp>
#include <WPGraphicsOgre/Deferred/DeferredShading.hpp>
#include <WPGraphicsOgre/Deferred/SharedData.hpp>
#include <WPGraphicsOgre/Addons/MaterialListener.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone, CDeferredShadingSystem, IGraphicsDeferredShading );

        CDeferredShadingSystem::CDeferredShadingSystem( SmartPtr<IViewport> viewport ) :
            mSystem( nullptr ),
            m_nextShadowUpdate( 0.0f )
        {
            auto engine = core::IApplicationManager::instance();

            // some hack to test ssao
            auto graphicsSystem = engine->getGraphicsSystem();
            auto window = graphicsSystem->getRenderWindow();
            if( !window )
            {
                return;
            }

            RenderWindow *rwWindow = nullptr;
            window->_getObject( (void **)&rwWindow );

            Viewport *vp = nullptr;
            viewport->_getObject( (void **)&vp );

            Camera *ogreCamera = vp->getCamera();

            SceneManager *ogreSceneManager = ogreCamera->getSceneManager();

            /*GodRaysListener *godRaysListener = new GodRaysListener();
            Ogre::CompositorInstance *godRaysCompositor =
            Ogre::CompositorManager::getSingleton().addCompositor(vp, "GodRays");
            godRaysCompositor->addListener(godRaysListener);
            Ogre::CompositorManager::getSingleton().setCompositorEnabled(vp, "GodRays", true);

            godRaysListener->SunScreenSpacePosition = Ogre::Vector4(0.5,0.5,0,1);*/

            // Ogre::CompositorInstance * pEffectInstance =
            // Ogre::CompositorManager::getSingleton().addCompositor(vp, "VolumetricSunLights", -1);
            // Ogre::CompositorManager::getSingleton().setCompositorEnabled(vp, "VolumetricSunLights",
            // true); m_volumetricListener = new VolumetricSunLightFrameListener(ogreSceneManger,
            // ogreCamera, vp); pEffectInstance->addListener(m_volumetricListener);

            auto materialListener = new MaterialListener();
            MaterialManager::getSingleton().addListener( materialListener );

            // smgr->setSkyBox(false, "");
            // ogreSceneManger->setSkyDome(true, "Ogre/Compositor/VolumetricSunLight/FakeSky", 5, 8,
            // 200.f); ogreSceneManger->setSkyBox(true, "Ogre/Compositor/VolumetricSunLight/FakeSky");

            //	PFXSSAO* ssao = new PFXSSAO(rwWindow, ogreCamera);

            Ogre::Vector3 lightdir( 0.55, -0.3, 0.75 );
            lightdir.normalise();

            // Ogre::Light* l = ogreSceneManager->createLight("Light0");
            // l->setType(Ogre::Light::LT_DIRECTIONAL);
            // l->setDirection(lightdir);
            // l->setDiffuseColour(Ogre::ColourValue::White);
            // l->setSpecularColour(Ogre::ColourValue(0.4, 0.4, 0.4));
            // l->setCastShadows(false);

            // Ogre::Light* l1 = ogreSceneManager->createLight("Light0");
            // l1->setShadowNearClipDistance(0);
            // l1->setShadowFarClipDistance(200);
            // l1->setShadowFarDistance(30);
            // l1->setCastShadows(true);

            // Ogre::Light* l2 = ogreSceneManger->createLight("Light1");
            // l2->setShadowNearClipDistance(0);
            // l2->setShadowFarClipDistance(50);
            // l2->setShadowFarDistance(15);
            // l2->setCastShadows(true);

            // Ogre::Light* l3 = ogreSceneManger->createLight("Light2");
            // l3->setShadowNearClipDistance(0);
            // l3->setShadowFarClipDistance(50);
            // l3->setShadowFarDistance(15);
            // l3->setCastShadows(true);

            // new SharedData();
            mSystem = new DeferredShadingSystem( vp, ogreSceneManager, ogreCamera );
            // SharedData::getSingleton().iSystem = mSystem;

            // safely setup application's (not postfilter!) shared data
            // SharedData::getSingleton().iCamera = ogreCamera;
            ////SharedData::getSingleton().iRoot = Ogre::Root::getSingletonPtr();
            // SharedData::getSingleton().iWindow = rwWindow;
            // SharedData::getSingleton().iActivate = true;
            // SharedData::getSingleton().iGlobalActivate = true;
            // SharedData::getSingleton().iMainLight = ogreSceneManager->getLight("Light0");

            auto attParams = Ogre::Vector4( 4, 1, 0, 7 );
            Ogre::Real lightRadius = 25;

            /*
            Ogre::Light *a = ogreSceneManger->createLight();
            SceneNode *an = ogreSceneManger->createSceneNode();
            an->attachObject(a);
            a->setAttenuation(attParams.x, attParams.y, attParams.z, attParams.w);
            //a->setAttenuation(1.0f, 0.000f, 0.000f);
            an->setPosition(0,0,lightRadius);
            a->setDiffuseColour(1,0,0);

            ogreCamera->getParentNode()->addChild(an);
            */

            // mSystem->setSSAO(true);
        }

        CDeferredShadingSystem::~CDeferredShadingSystem()
        {
            unload( nullptr );
        }

        void CDeferredShadingSystem::load( SmartPtr<ISharedObject> data )
        {
        }

        void CDeferredShadingSystem::unload( SmartPtr<ISharedObject> data )
        {
        }

        u32 CDeferredShadingSystem::getMode() const
        {
            if( mSystem )
            {
                return mSystem->getMode();
            }

            return 0;
        }

        void CDeferredShadingSystem::setMode( u32 mode )
        {
            if( mSystem )
            {
                mSystem->setMode( static_cast<DeferredShadingSystem::DSMode>( mode ) );
            }
        }

        bool CDeferredShadingSystem::getSSAO() const
        {
            if( mSystem )
            {
                return mSystem->getSSAO();
            }

            return false;
        }

        void CDeferredShadingSystem::setSSAO( bool ssao )
        {
            if( mSystem )
            {
                mSystem->setSSAO( ssao );
            }
        }

        bool CDeferredShadingSystem::getActive() const
        {
            // return mSystem->getActive();
            return true;
        }

        void CDeferredShadingSystem::setActive( bool active )
        {
            if( mSystem )
                mSystem->setActive( active );
        }

        bool CDeferredShadingSystem::getShadowsEnabled() const
        {
            // if(mSystem)
            //	return mSystem->getShadowsEnabled();

            return false;
        }

        void CDeferredShadingSystem::setShadowsEnabled( bool enabled )
        {
            // if(mSystem)
            //	mSystem->setShadowsEnabled(enabled);
        }

        DeferredShadingSystem *CDeferredShadingSystem::getDeferredShading() const
        {
            return mSystem;
        }

        void CDeferredShadingSystem::setDeferredShading( DeferredShadingSystem *deferredShading )
        {
            mSystem = deferredShading;
        }

        void CDeferredShadingSystem::update()
        {
            // if(mSystem)
            //{
            //	mSystem->updateShadows();
            //	mSystem->updateShadows(0);
            // }

            // if(m_nextShadowUpdate < t)
            //{
            //	if(mSystem)
            //		mSystem->enableShadowUpdate();

            //	m_nextShadowUpdate = t + 1.0f / 60.0f;
            //}
            // else
            //{
            //	if(mSystem)
            //		mSystem->disableShadowUpdate();
            //}
        }

        String CDeferredShadingSystem::getGBufferCompositorName() const
        {
            return mSystem->getGBufferCompositorName().c_str();
        }

        void CDeferredShadingSystem::setGBufferCompositorName( const String &gBufferCompositorName )
        {
            mSystem->setGBufferCompositorName( gBufferCompositorName.c_str() );
        }

        String CDeferredShadingSystem::getShowLightingCompositorName() const
        {
            return mSystem->getShowLightingCompositorName().c_str();
        }

        void CDeferredShadingSystem::setShowLightingCompositorName( const String &showLightingCompositorName )
        {
            mSystem->setGBufferCompositorName( showLightingCompositorName.c_str() );
        }

        String CDeferredShadingSystem::getShowNormalsCompositorName() const
        {
            return mSystem->getShowNormalsCompositorName().c_str();
        }

        void CDeferredShadingSystem::setShowNormalsCompositorName( const String &showNormalsCompositorName )
        {
            mSystem->setGBufferCompositorName( showNormalsCompositorName.c_str() );
        }

        String CDeferredShadingSystem::getShowDepthSpecularCompositorName() const
        {
            return mSystem->getShowDepthSpecularCompositorName().c_str();
        }

        void CDeferredShadingSystem::setShowDepthSpecularCompositorName( const String &showDepthSpecularCompositorName )
        {
            mSystem->setGBufferCompositorName( showDepthSpecularCompositorName.c_str() );
        }

        String CDeferredShadingSystem::getShowColourCompositorName() const
        {
            return mSystem->getShowColourCompositorName().c_str();
        }

        void CDeferredShadingSystem::setShowColourCompositorName( const String &showColourCompositorName )
        {
            mSystem->setGBufferCompositorName( showColourCompositorName.c_str() );
        }

        void CDeferredShadingSystem::initialise( SmartPtr<IBuildDirector> objectTemplate )
        {
            //SmartPtr<DeferredShadingSystemTemplate> pTemplate;  // = objectTemplate;
            //mSystem->setGBufferCompositorName( pTemplate->getGBufferCompositorName().c_str() );
            //mSystem->setShowLightingCompositorName( pTemplate->getShowLightingCompositorName().c_str() );
            //mSystem->setShowNormalsCompositorName( pTemplate->getShowNormalsCompositorName().c_str() );
            //mSystem->setShowDepthSpecularCompositorName(
            //    pTemplate->getShowDepthSpecularCompositorName().c_str() );
            //mSystem->setShowColourCompositorName( pTemplate->getShowColourCompositorName().c_str() );

            //mSystem->setLightCompositionPassName( pTemplate->getLightCompositionPassName().c_str() );

            //mSystem->setRenderTargetName( pTemplate->getRenderTargetName().c_str() );

            //mSystem->initialize();
        }

        void CDeferredShadingSystem::initialise( SmartPtr<IBuildDirector> objectTemplate,
                                                 SmartPtr<Properties> instanceProperties )
        {
            mSystem->initialize();
        }
    }  // end namespace render
}  // namespace workphone
