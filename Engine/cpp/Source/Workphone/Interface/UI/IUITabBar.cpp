#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUITabBar.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUITabBar, IUIElement );

    IUITabBar::~IUITabBar() = default;

}  // namespace workphone::ui
