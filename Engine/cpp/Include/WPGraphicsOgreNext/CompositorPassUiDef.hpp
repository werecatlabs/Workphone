
#pragma once

#include <WPImGui/WPImGuiPrerequisites.hpp>

#if WP_GRAPHICS_SYSTEM_OGRENEXT
#    include "OgreHeaderPrefix.h"
#    include "Compositor/Pass/OgreCompositorPassDef.h"

namespace workphone
{

    /** @ingroup Api_Backend
    @class CompositorPassUiDef
    */
    class CompositorPassUiDef : public Ogre::CompositorPassDef
    {
    public:
        enum AspectRatioMode
        {
            /// Never change the Canvas resolution
            ArNone,
            /// Change the canvas' height if the aspect ratio of the window changes.
            /// For example if the original canvas size was 1920x1080 and the window
            /// is resized to a resolution with AR = 4:3, now the virtual canvas
            /// will be 1920x1440
            ArKeepWidth,
            /// Change the canvas' width if the aspect ratio of the window changes.
            /// For example if the original canvas size was 1920x1080 and the window
            /// is resized to a resolution with AR = 4:3, now the virtual canvas
            /// will be 1440x1080
            ArKeepHeight,
        };

        bool mRenderUI = false;
        bool mRenderSceneUI = false;
        bool mSetsResolution;
        AspectRatioMode mAspectRatioMode;

        CompositorPassUiDef( Ogre::CompositorTargetDef *parentTargetDef );
    };
}  // namespace workphone

#    include "OgreHeaderSuffix.h"

#endif
