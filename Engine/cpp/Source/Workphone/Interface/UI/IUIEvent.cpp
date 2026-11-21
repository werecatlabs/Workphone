#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIEvent.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIEvent, IEvent );

    IUIEvent::IUIEvent( u32 poolTypeId ) : IEvent( poolTypeId )
    {
    }

    IUIEvent::IUIEvent() : IEvent( IUIEvent::typeInfo() )
    {
    }

    IUIEvent::~IUIEvent() = default;

}  // namespace workphone::ui
