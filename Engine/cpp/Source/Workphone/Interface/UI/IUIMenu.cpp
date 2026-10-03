#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIMenu.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIMenu, IUIElement );

    IUIMenu::~IUIMenu() = default;

}  // namespace workphone::ui
