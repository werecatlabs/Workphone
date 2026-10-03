// "Depth of Field" demo for Ogre
// Copyright (C) 2006  Christian Lindequist Larsen
//
// This code is in the public domain. You may do whatever you want with it.
#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include "WPGraphicsOgre/Addons/DepthOfFieldEffect.hpp"
#include <Workphone/WorkphoneTypes.hpp>

using namespace Ogre;

namespace workphone
{
    namespace render
    {

        const int DepthOfFieldEffect::BLUR_DIVISOR = 8;
        static int nameExt = 0;

        DepthOfFieldEffect::DepthOfFieldEffect( Viewport *viewport ) :
            mViewport( viewport ),
            mNearDepth( 10.0 ),
            mFocalDepth( 100.0 ),
            mFarDepth( 1500.0f ),
            mFarBlurCutoff( 1.0 )
        {
            nameExt++;
            createDepthRenderTexture();
        }

        DepthOfFieldEffect::~DepthOfFieldEffect()
        {
            destroyDepthRenderTexture();
        }

        float DepthOfFieldEffect::getNearDepth() const
        {
            return mNearDepth;
        }

        float DepthOfFieldEffect::getFocalDepth() const
        {
            return mFocalDepth;
        }

        float DepthOfFieldEffect::getFarDepth() const
        {
            return mFarDepth;
        }

        void DepthOfFieldEffect::setFocalDepths( float nearDepth, float focalDepth, float farDepth )
        {
            mNearDepth = nearDepth;
            mFocalDepth = focalDepth;
            mFarDepth = farDepth;
        }

        float DepthOfFieldEffect::getFarBlurCutoff() const
        {
            return mFarBlurCutoff;
        }

        void DepthOfFieldEffect::setFarBlurCutoff( float cutoff )
        {
            mFarBlurCutoff = cutoff;
        }

        void DepthOfFieldEffect::createDOFCompositor()
        {
            using namespace Ogre;

            auto compositorManager = CompositorManager::getSingletonPtr();

            auto nameExtStr = Ogre::StringConverter::toString( nameExt );
            auto name = String( "DoF_Compositor" ) + String( nameExtStr.c_str() );

            // Create compositor
            auto compositor = compositorManager->create(
                name.c_str(), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

            {
                // Create technique
                CompositionTechnique *technique = compositor->createTechnique();
                {
                    // Create texture definition 'scene'
                    CompositionTechnique::TextureDefinition *definition =
                        technique->createTextureDefinition( "scene" );
                    definition->width = 0;   // target width
                    definition->height = 0;  // target height
                    definition->formatList.push_back( PF_X8R8G8B8 );
                }
                {
                    // Create texture definition 'blur'
                    CompositionTechnique::TextureDefinition *definition =
                        technique->createTextureDefinition( "blur" );
                    definition->width = 256;
                    definition->height = 256;
                    definition->formatList.push_back( PF_X8R8G8B8 );
                }
                {
                    // Create target pass 'scene'
                    CompositionTargetPass *target = technique->createTargetPass();
                    target->setOutputName( "scene" );
                    target->setInputMode( CompositionTargetPass::IM_PREVIOUS );
                }
                {
                    // Create target pass 'blur'
                    CompositionTargetPass *target = technique->createTargetPass();
                    target->setOutputName( "blur" );
                    {
                        // Create a composition pass
                        CompositionPass *pass = target->createPass();
                        pass->setIdentifier( DepthOfFieldEffect::BlurPass );
                        pass->setMaterialName( "DoF_Gaussian3x3" );
                        pass->setInput( 0, "scene" );
                    }
                }
                {
                    // Build the output target pass
                    CompositionTargetPass *output = technique->getOutputTargetPass();
                    {
                        // Create a composition pass
                        CompositionPass *pass = output->createPass();
                        pass->setIdentifier( DepthOfFieldEffect::OutputPass );
                        pass->setMaterialName( "DoF_DepthOfField" );
                        pass->setInput( 0, "scene" );
                        pass->setInput( 1, "depth" );  // render texture
                        pass->setInput( 2, "blur" );
                    }
                }
            }
        }

        void DepthOfFieldEffect::createDepthRenderTexture()
        {
            const f32 sizeMulitplier = 1.0f;

            auto name = String( "DoF_Depth" );  // + Ogre::StringConverter::toString(nameExt);

            // Create the depth render texture
            depthTexture = Ogre::TextureManager::getSingleton().createManual(
                name.c_str(), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D,
                mViewport->getActualWidth() * sizeMulitplier,
                mViewport->getActualHeight() * sizeMulitplier, 0, PF_L8, TU_RENDERTARGET );

            // Get its render target and add a viewport to it
            Ogre::RenderTexture *depthTarget = depthTexture->getBuffer()->getRenderTarget();
            depthViewport = depthTarget->addViewport( mViewport->getCamera() );

            // Register 'this' as a render target listener
            depthTarget->addListener( this );

            // Get the technique to use when rendering the depth render texture
            mDepthMaterial = MaterialManager::getSingleton().getByName( "DoF_Depth" );
            mDepthMaterial->load();  // needs to be loaded manually
            mDepthTechnique = mDepthMaterial->getBestTechnique();

            Ogre::MaterialPtr ambientMat = MaterialManager::getSingleton().getByName( "DoF_Ambient" );
            ambientMat->load();
            mAmbientTechnique = ambientMat->getBestTechnique();

            //// Create a custom render queue invocation sequence for the depth render texture
            // invocationSequence = Ogre::Root::getSingleton().createRenderQueueInvocationSequence(name);

            //// Add a render queue invocation to the sequence, and disable shadows for it
            // Ogre::RenderQueueInvocation* invocation =
            // invocationSequence->add(RENDER_QUEUE_WORLD_GEOMETRY_1, "worldGeometry");
            // invocation->setSuppressShadows(true);

            // invocation = invocationSequence->add(RENDER_QUEUE_MAIN, "main");
            // invocation->setSuppressShadows(true);

            // invocation = invocationSequence->add(RENDER_QUEUE_6, "render_queue_6");
            // invocation->setSuppressShadows(true);

            // invocation = invocationSequence->add(RENDER_QUEUE_7, "render_queue_7");
            // invocation->setSuppressShadows(true);

            //// Set the render queue invocation sequence for the depth render texture viewport
            // depthViewport->setRenderQueueInvocationSequenceName(invocationSequence->getName());

            // update debug texture
            // SmartPtr<IMaterial> mat = MaterialManager::getSingleton().getByName("DoF_DepthDebug");
            // mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("DoF_Depth");
        }

        void DepthOfFieldEffect::destroyDepthRenderTexture()
        {
            // Root::getSingleton().destroyRenderQueueInvocationSequence(invocationSequence->getName());

            // Ogre::RenderTarget* rtt = depthTexture->getBuffer()->getRenderTarget();
            // rtt->removeAllViewports();

            // Ogre::MaterialManager::getSingleton().remove(mDepthMaterial->getHandle());
            // Ogre::TextureManager::getSingleton().remove(depthTexture->getHandle());
            // depthTexture.setNull();
        }

        void DepthOfFieldEffect::notifyMaterialSetup( uint32 passId, Ogre::MaterialPtr &material )
        {
            switch( passId )
            {
            case BlurPass:
            {
                // Adjust fragment program parameters
                Ogre::Vector3 pixelSize =
                    Ogre::Vector3( 1.0f / ( mViewport->getActualWidth() / BLUR_DIVISOR ),
                                   1.0f / ( mViewport->getActualHeight() / BLUR_DIVISOR ), 1.0f );
                GpuProgramParametersSharedPtr fragParams =
                    material->getBestTechnique()->getPass( 0 )->getFragmentProgramParameters();

                static const Ogre::String pixelSizeStr( "pixelSize" );

                fragParams->setNamedConstant( pixelSizeStr, pixelSize );
            }
            break;
            case OutputPass:
            {
                float pixelSizeScene[2] = { 1.0f / mViewport->getActualWidth(),
                                            1.0f / mViewport->getActualHeight() };

                float pixelSizeBlur[2] = { 1.0f / ( mViewport->getActualWidth() / BLUR_DIVISOR ),
                                           1.0f / ( mViewport->getActualHeight() / BLUR_DIVISOR ) };

                // Adjust fragment program parameters
                GpuProgramParametersSharedPtr fragParams =
                    material->getBestTechnique()->getPass( 0 )->getFragmentProgramParameters();

                static const Ogre::String pixelSizeSceneStr( "pixelSizeScene" );
                static const Ogre::String pixelSizeBlurStr( "pixelSizeBlur" );

                fragParams->setNamedConstant( pixelSizeSceneStr, pixelSizeScene, 1, 2 );
                fragParams->setNamedConstant( pixelSizeBlurStr, pixelSizeBlur, 1, 2 );
            }
            break;
            }
        }

        void DepthOfFieldEffect::preViewportUpdate( const Ogre::RenderTargetViewportEvent &evt )
        {
            float dofParams[4] = { mNearDepth, mFocalDepth, mFarDepth, mFarBlurCutoff };

            // Adjust fragment program parameters for depth pass
            GpuProgramParametersSharedPtr fragParams =
                mDepthTechnique->getPass( 0 )->getFragmentProgramParameters();

            static const Ogre::String dofParamsStr( "dofParams" );

            fragParams->setNamedConstant( dofParamsStr, dofParams, 1, 4 );

            // Add 'this' as a RenderableListener to replace the technique for all renderables
            Ogre::Camera *camera = evt.source->getCamera();
            if( camera )
            {
                depthViewport->setCamera( camera );

                RenderQueue *queue = evt.source->getCamera()->getSceneManager()->getRenderQueue();
                queue->setRenderableListener( this );
            }
            else
            {
                depthViewport->setCamera( NULL );
            }
        }

        void DepthOfFieldEffect::postViewportUpdate( const Ogre::RenderTargetViewportEvent &evt )
        {
            // Reset the RenderableListener
            Ogre::Camera *camera = evt.source->getCamera();
            if( camera )
            {
                depthViewport->setCamera( camera );

                RenderQueue *queue = evt.source->getCamera()->getSceneManager()->getRenderQueue();
                queue->setRenderableListener( NULL );
            }
            else
            {
                depthViewport->setCamera( NULL );
            }
        }

        bool DepthOfFieldEffect::renderableQueued( Ogre::Renderable *rend, Ogre::uint8 groupID,
                                                   Ogre::ushort priority, Ogre::Technique **ppTech,
                                                   Ogre::RenderQueue * )
        {
#if 0
			// Replace the technique of all renderables
			//hack for transparency issues
			Ogre::MaterialPtr mat = rend->getMaterial();
			Ogre::Technique* tech = mat->getBestTechnique();
			if (tech->getNumPasses() > 0)
			{
				Ogre::Pass* pass = tech->getPass("decal");
				if (pass && pass->isTransparent())
				{
					*ppTech = tech;
					return true;
				}
			}

			*ppTech = mDepthTechnique;
			return true;

#else
            // setup for shmup
            if( groupID == RENDER_QUEUE_WORLD_GEOMETRY_1 || groupID < RENDER_QUEUE_3 )
            {
                *ppTech = mDepthTechnique;
                return true;
            }

            return false;
#endif
        }

    }  // namespace render
}  // namespace workphone
