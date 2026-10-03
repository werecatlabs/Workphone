#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIVector3.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIVector3, IUIElement );

    IUIVector3::~IUIVector3() = default;
}  // namespace workphone::ui
