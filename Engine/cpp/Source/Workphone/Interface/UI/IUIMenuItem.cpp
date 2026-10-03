#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIMenuItem.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIMenuItem, IUIElement );

    IUIMenuItem::~IUIMenuItem() = default;

}  // namespace workphone::ui
