#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIWindow, IUIElement );

    IUIWindow::~IUIWindow() = default;

}  // namespace workphone::ui
