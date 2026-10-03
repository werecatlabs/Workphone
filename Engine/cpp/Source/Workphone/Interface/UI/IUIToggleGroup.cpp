#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIToggleGroup.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIToggleGroup, IUIElement );

    IUIToggleGroup::~IUIToggleGroup() = default;
}  // namespace workphone::ui
