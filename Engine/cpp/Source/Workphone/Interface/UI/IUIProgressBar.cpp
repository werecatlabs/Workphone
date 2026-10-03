#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIProgressBar.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIProgressBar, IUIElement );

    IUIProgressBar::~IUIProgressBar() = default;

}  // namespace workphone::ui
