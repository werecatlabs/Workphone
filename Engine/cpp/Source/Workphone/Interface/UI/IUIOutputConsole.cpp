#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIOutputConsole.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIOutputConsole, IUIElement );

    IUIOutputConsole::~IUIOutputConsole() = default;

}  // namespace workphone::ui
