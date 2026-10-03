#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIElement, core::IPrototype );

    const hash_type IUIElement::STATE_MESSAGE_ADD_CHILD = StringUtil::getHash( "addChild" );
    const hash_type IUIElement::STATE_MESSAGE_REMOVE_CHILD = StringUtil::getHash( "removeChild" );

    const u32 IUIElement::sameLineFlag = 1 << 1;
    const u32 IUIElement::renderChildrenFlag = 1 << 2;
    const u32 IUIElement::enabledFlag = 1 << 3;
    const u32 IUIElement::visibleFlag = 1 << 4;
    const u32 IUIElement::selectedFlag = 1 << 5;
    const u32 IUIElement::hoveredFlag = 1 << 6;
    const u32 IUIElement::focusedFlag = 1 << 7;
    const u32 IUIElement::highlightedFlag = 1 << 8;
    const u32 IUIElement::dragDropSourceFlag = 1 << 9;
    const u32 IUIElement::handleInputEventsFlag = 1 << 10;
    const u32 IUIElement::elementVisibleFlag = 1 << 11;

    const String IUIElement::widgetPosStr = "widget_pos";
    const String IUIElement::widgetSizeStr = "widget_size";
    const String IUIElement::sizeStr = "size";
    const String IUIElement::positionStr = "position";
    const String IUIElement::enabledStr = "enabled";
    const String IUIElement::visibleStr = "visible";
    const String IUIElement::colourStr = "colour";
    const String IUIElement::sameLineStr = "same_line";
    const String IUIElement::orderStr = "order";
    const String IUIElement::makeDirtyStr = "make dirty";

    IUIElement::IUIElement() = default;
    IUIElement::IUIElement( u32 poolTypeId ) : core::IPrototype( poolTypeId )
    {
    }

    IUIElement::~IUIElement() = default;

}  // namespace workphone::ui
