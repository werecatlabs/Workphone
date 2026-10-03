#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIText, IUIElement );

    const String IUIText::textPropertyStr = "text";
    const String IUIText::textSizePropertyStr = "text_size";
    const String IUIText::verticalAlignmentPropertyStr = "vertical_alignment";
    const String IUIText::horizontalAlignmentPropertyStr = "horizontal_alignment";
    const String IUIText::textSizeStr = String( "textSize" );
    const String IUIText::verticalAlignmentStr = String( "verticalAlignment" );
    const String IUIText::horizontalAlignmentStr = String( "horizontalAlignment" );
    const String IUIText::textColourRStr = String( "textColourR" );
    const String IUIText::textColourGStr = String( "textColourG" );
    const String IUIText::textColourBStr = String( "textColourB" );
    const String IUIText::textColourAStr = String( "textColourA" );
    const String IUIText::backgroundColourRStr = String( "backgroundColourR" );
    const String IUIText::backgroundColourGStr = String( "backgroundColourG" );
    const String IUIText::backgroundColourBStr = String( "backgroundColourB" );
    const String IUIText::backgroundColourAStr = String( "backgroundColourA" );
    const String IUIText::textPaddingXStr = String( "textPaddingX" );
    const String IUIText::textPaddingYStr = String( "textPaddingY" );
    const String IUIText::textWrapStr = String( "textWrap" );

    IUIText::IUIText() : IUIElement( IUIText::typeInfo() )
    {
    }

    IUIText::~IUIText() = default;
}  // namespace workphone::ui
