#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIVector4.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIVector4, IUIElement );

    IUIVector4::~IUIVector4() = default;

}  // namespace workphone::ui
