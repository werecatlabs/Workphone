#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{

    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIButton, IUIElement );

    const String IUIButton::referenceWidthStr = String( "referenceWidth" );
    const String IUIButton::referenceHeightStr = String( "referenceHeight" );
    const String IUIButton::textNormalColourStr = String( "textNormalColour" );
    const String IUIButton::textHoverColourStr = String( "textHoverColour" );
    const String IUIButton::textActiveColourStr = String( "textActiveColour" );
    const String IUIButton::textBackgroundColourStr = String( "textBackgroundColour" );
    const String IUIButton::normalColourStr = String( "normalColour" );
    const String IUIButton::hoverColourStr = String( "hoverColour" );
    const String IUIButton::activeColourStr = String( "activeColour" );
    const String IUIButton::borderColourStr = String( "borderColour" );
    const String IUIButton::borderWidthStr = String( "borderWidth" );
    const String IUIButton::roundingStr = String( "rounding" );
    const String IUIButton::paddingStr = String( "padding" );

    IUIButton::IUIButton() : IUIElement( IUIButton::typeInfo() )
    {
    }

    IUIButton::IUIButton( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUIButton::~IUIButton() = default;

}  // namespace workphone::ui
