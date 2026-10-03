#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIScrollingText.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{

    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIScrollingText, IUIElement );

    IUIScrollingText::~IUIScrollingText() = default;

}  // namespace workphone::ui
