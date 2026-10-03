#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/ImguiOgre/ImGuiRenderTargetListener.hpp>
#include <Workphone/Workphone.hpp>

#include <WPGraphicsOgre/ImguiOgre/ImGuiOverlayOgre.hpp>
#include <OgreOverlayManager.h>
#include <Ogre.h>

using namespace workphone;

ImGuiRenderTargetListener::ImGuiRenderTargetListener()
{
}

ImGuiRenderTargetListener::~ImGuiRenderTargetListener()
{
}

void ImGuiRenderTargetListener::preViewportUpdate( const Ogre::RenderTargetViewportEvent &evt )
{
    using namespace Ogre;

    if( ImGuiOverlayOgre::NewFrame() )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        if( auto ui = applicationManager->getUI() )
        {
            if( auto application = ui->getApplication() )
            {
                application->update();
            }
        }
    }
}
