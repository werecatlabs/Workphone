#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIScrollingTextElement.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone.h>

namespace workphone::ui
{
    ClawUIScrollingTextElement::ClawUIScrollingTextElement()
    {
        setType( "ScrollingTextElement" );
    }

    ClawUIScrollingTextElement::~ClawUIScrollingTextElement() = default;

    bool ClawUIScrollingTextElement::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        return isEnabled() && ClawUIElement::handleEvent( event );
    }

    void ClawUIScrollingTextElement::draw( struct wp_context *ctx )
    {
        if( !beginWorkphoneWidget( ctx ) )
        {
            return;
        }

        auto text = getLabel();
        if( text.empty() )
        {
            text = getName();
        }
        wp_label_wrap( ctx, reinterpret_cast<const wp_c8 *>( text.c_str() ) );
        drawWorkphoneChildren( ctx );
    }
}  // namespace workphone::ui
