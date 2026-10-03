#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IInputEvent, ISharedObject );

    IInputEvent::~IInputEvent() = default;

}  // namespace workphone
