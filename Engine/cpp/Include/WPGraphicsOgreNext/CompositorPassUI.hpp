#pragma once

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include "OgrePrerequisites.h"
#include "Compositor/Pass/OgreCompositorPass.h"
#include "OgreHeaderPrefix.h"

namespace workphone
{
    class CompositorPassUiDef;

    class CompositorPassUI : public Ogre::CompositorPass
    {
    public:
        CompositorPassUI( const CompositorPassUiDef *definition, Ogre::SceneManager *sceneManager,
                          const Ogre::RenderTargetViewDef *rtv, Ogre::CompositorNode *parentNode );

        virtual void execute( const Ogre::Camera *lodCamera );

        virtual bool notifyRecreated( const Ogre::TextureGpu *channel );

        void setResolutionToColibri( u32 width, u32 height );

        Ogre::SceneManager *mSceneManager;

        /// Typed pointer to the UI pass definition. This is the same object that the base class
        /// holds as `const CompositorPassDef *mDefinition`. Keeping a separate typed pointer
        /// avoids repeated static_casts, but it must NOT shadow the base member name.
        CompositorPassUiDef *mUiDefinition;
    };
}  // namespace workphone

#include "OgreHeaderSuffix.h"
