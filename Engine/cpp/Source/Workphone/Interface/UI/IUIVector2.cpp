#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIVector2.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIVector2, IUIElement );

    IUIVector2::~IUIVector2() = default;
}  // namespace workphone::ui
