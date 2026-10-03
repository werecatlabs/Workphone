#pragma once

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include "OgrePrerequisites.h"
#include "Compositor/Pass/OgreCompositorPassProvider.h"

namespace workphone
{
    namespace render
    {

        /** @ingroup Api_Backend
        @class CompositorPassColibriGuiProvider
            Generates the CompositorPassColibriGui pass needed to render Colibri UI
            Also ensures the compositor scripts recognize the pass type "colibri_gui"
            i.e.
            @code
                pass custom colibri_gui
                {
                }
            @endcode
        */
        class CompositorPassProvider : public Ogre::CompositorPassProvider
        {
        public:
            CompositorPassProvider();

            /** Called from CompositorTargetDef::addPass when adding a Compositor Pass of type 'custom'
            @param passType
            @param customId
                Arbitrary ID in case there is more than one type of custom pass you want to implement.
                Defaults to IdString()
            @param rtIndex
            @param parentNodeDef
            @return
            */
            virtual Ogre::CompositorPassDef *addPassDef( Ogre::CompositorPassType passType,
                                                         Ogre::IdString customId,
                                                         Ogre::CompositorTargetDef *parentTargetDef,
                                                         Ogre::CompositorNodeDef *parentNodeDef );

            /** Creates a CompositorPass from a CompositorPassDef for Compositor Pass of type 'custom'
            @remarks    If you have multiple custom pass types then you will need to use dynamic_cast<>()
                        on the CompositorPassDef to determine what custom pass it is.
            */
            virtual Ogre::CompositorPass *addPass( const Ogre::CompositorPassDef *definition,
                                                   Ogre::Camera *defaultCamera,
                                                   Ogre::CompositorNode *parentNode,
                                                   const Ogre::RenderTargetViewDef *rtvDef,
                                                   Ogre::SceneManager *sceneManager );
        };

    }  // namespace render
}  // namespace workphone
