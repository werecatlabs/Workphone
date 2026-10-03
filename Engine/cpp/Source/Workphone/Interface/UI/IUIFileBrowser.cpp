#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIFileBrowser.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIFileBrowser, IUIElement );

    IUIFileBrowser::~IUIFileBrowser() = default;

}  // namespace workphone::ui
