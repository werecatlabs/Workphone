#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUISpinner.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUISpinner, IUIElement );

    IUISpinner::~IUISpinner() = default;

}  // namespace workphone::ui
