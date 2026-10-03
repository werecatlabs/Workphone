#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIProfilerWindow.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIProfilerWindow, IUIElement );

    IUIProfilerWindow::~IUIProfilerWindow() = default;

}  // namespace workphone::ui
