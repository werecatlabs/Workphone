#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIApplication.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIApplication, ISharedObject );

    IUIApplication::~IUIApplication() = default;

}  // namespace workphone::ui
