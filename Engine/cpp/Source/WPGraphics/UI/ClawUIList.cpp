#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIList.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::ui
{
    void ClawUIList::draw( struct wp_context *ctx )
    {
        if( ctx && isVisible() && isEnabled() )
        {
            drawWorkphoneChildren( ctx );
        }
    }
}  // namespace workphone::ui
