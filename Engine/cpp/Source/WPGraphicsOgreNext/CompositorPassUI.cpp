#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include "WPGraphicsOgreNext/CompositorPassUI.hpp"
#include "WPGraphicsOgreNext/ImguiManagerOgreNext.hpp"
#include "WPGraphicsOgreNext/CompositorPassUiDef.hpp"
#include <WPGraphicsOgreNext/UIRenderer.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Workphone.hpp>
#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceListener.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreRenderSystem.h"
#include "OgreSceneManager.h"

namespace workphone
{

    CompositorPassUI::CompositorPassUI( const CompositorPassUiDef *definition,
                                        Ogre::SceneManager *sceneManager,
                                        const Ogre::RenderTargetViewDef *rtv,
                                        Ogre::CompositorNode *parentNode ) :
        CompositorPass( definition, parentNode ),
        mSceneManager( sceneManager ),
        mUiDefinition( (CompositorPassUiDef *)definition )
    {
        // initialize() must be called by every CompositorPass-derived class to set up
        // mRenderPassDesc. Omitting it leaves mRenderPassDesc null, causing a crash
        // the first time Ogre needs to bind the render target for this pass.
        initialize( rtv, /*supportsNoRtv=*/true );

        Ogre::TextureGpu *texture =
            mParentNode->getDefinedTexture( rtv->colourAttachments[0].textureName );
        if( texture )
        {
            setResolutionToColibri( texture->getWidth(), texture->getHeight() );
        }
    }

    void CompositorPassUI::execute( const Ogre::Camera *lodCamera )
    {
        //Execute a limited number of times?
        if( mNumPassesLeft != std::numeric_limits<u32>::max() )
        {
            if( !mNumPassesLeft )
            {
                return;
            }

            --mNumPassesLeft;
        }

        profilingBegin();

        notifyPassEarlyPreExecuteListeners();

        //analyzeBarriers();
        //executeResourceTransitions();

        //Fire the listener in case it wants to change anything
        notifyPassPreExecuteListeners();

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        WP_ASSERT( graphicsSystem );

        auto resourceManager = graphicsSystem->getResourceGroupManager();

        if( graphicsSystem->isLoaded() && resourceManager->isLoaded() )
        {
            if( mUiDefinition->mRenderSceneUI )
            {
                if( auto uiRenderer = render::UIRenderer::getSingletonPtr() )
                {
                    uiRenderer->beginFrame();

                    if( auto renderUI = applicationManager->getRenderUI() )
                    {
                        renderUI->render();
                    }

                    uiRenderer->render();
                    uiRenderer->endFrame();
                }
            }

            if( mUiDefinition->mRenderUI )
            {
                if( auto imgui = render::ImguiManagerOgreNext::getSingletonPtr() )
                {
                    imgui->newFrame();

                    if( auto ui = applicationManager->getUI() )
                    {
                        if( auto application = ui->getApplication() )
                        {
                            application->update();
                        }
                    }

                    imgui->render();
                }
            }
        }

        notifyPassPosExecuteListeners();
        profilingEnd();
    }

    void CompositorPassUI::setResolutionToColibri( u32 width, u32 height )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        if( auto ui = applicationManager->getUI() )
        {
            if( auto application = ui->getApplication() )
            {
                application->setWindowSize( Vector2I( width, height ) );
            }
        }
    }

    auto CompositorPassUI::notifyRecreated( const Ogre::TextureGpu *channel ) -> bool
    {
        bool usedByUs = CompositorPass::notifyRecreated( channel );

        if( usedByUs && !Ogre::PixelFormatGpuUtils::isDepth( channel->getPixelFormat() ) &&
            !Ogre::PixelFormatGpuUtils::isStencil( channel->getPixelFormat() ) )
        {
            setResolutionToColibri( channel->getWidth(), channel->getHeight() );
        }

        return usedByUs;
    }

}  // namespace workphone
