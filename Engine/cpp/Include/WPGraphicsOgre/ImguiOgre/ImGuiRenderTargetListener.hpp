#ifndef ImGuiRenderTargetListener_h__
#define ImGuiRenderTargetListener_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>

#include <OgreRenderTargetListener.h>

class ImGuiRenderTargetListener : public Ogre::RenderTargetListener
{
public:
    ImGuiRenderTargetListener();
    ~ImGuiRenderTargetListener();

    void preViewportUpdate( const Ogre::RenderTargetViewportEvent &evt );
};

#endif  // ImGuiRenderTargetListener_h__
