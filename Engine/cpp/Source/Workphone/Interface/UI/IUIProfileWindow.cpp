#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIProfileWindow.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIProfileWindow, IUIElement );

    IUIProfileWindow::~IUIProfileWindow() = default;

}  // namespace workphone::ui
