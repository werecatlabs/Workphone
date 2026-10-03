#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUISearchBar.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUISearchBar, IUIElement );

    IUISearchBar::~IUISearchBar() = default;

}  // namespace workphone::ui
