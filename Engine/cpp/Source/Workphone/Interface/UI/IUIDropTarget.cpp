#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIDropTarget.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIDropTarget, IEventListener );

    IUIDropTarget::~IUIDropTarget() = default;

}  // namespace workphone::ui
