#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include "WPGraphicsOgreNext/CompositorPassUiDef.hpp"

namespace workphone
{

    CompositorPassUiDef::CompositorPassUiDef( Ogre::CompositorTargetDef *parentTargetDef ) :
        CompositorPassDef( Ogre::PASS_CUSTOM, parentTargetDef ),
        mSetsResolution( true ),
        mAspectRatioMode( ArNone )
    {
        mProfilingId = "Colibri Gui";
    }

}  // namespace workphone
