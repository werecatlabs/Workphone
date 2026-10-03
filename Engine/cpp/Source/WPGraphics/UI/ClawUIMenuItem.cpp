#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIMenuItem.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone.h>

namespace workphone::ui
{
    ClawUIMenuItem::ClawUIMenuItem()
    {
        setType( "MenuItem" );
    }

    ClawUIMenuItem::~ClawUIMenuItem() = default;

    bool ClawUIMenuItem::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        return isEnabled() && ClawUIElement::handleEvent( event );
    }

    void ClawUIMenuItem::draw( struct wp_context *ctx )
    {
        if( !beginWorkphoneWidget( ctx ) )
        {
            return;
        }

        auto label = getLabel();
        if( label.empty() )
        {
            label = getName();
        }

        if( wp_button_label( ctx, label.c_str() ) )
        {
            onActivate( this );
        }
        drawWorkphoneChildren( ctx );
    }
}  // namespace workphone::ui
