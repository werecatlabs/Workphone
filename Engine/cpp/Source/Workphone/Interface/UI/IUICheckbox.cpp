#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUICheckbox.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUICheckbox, IUIElement );

    IUICheckbox::~IUICheckbox() = default;

}  // namespace workphone::ui
