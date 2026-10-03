#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUISlider.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUISlider, IUIElement );

    IUISlider::~IUISlider() = default;

}  // namespace workphone::ui
