#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/UIElementStateData.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, UIElementStateData, StateData );

    UIElementStateData::UIElementStateData() : StateData( UIElementStateData::typeInfo() )
    {
        flags = ui::IUIElement::enabledFlag | ui::IUIElement::visibleFlag |
                ui::IUIElement::renderChildrenFlag | ui::IUIElement::handleInputEventsFlag;
    }

    UIElementStateData::~UIElementStateData() = default;

}  // namespace workphone
