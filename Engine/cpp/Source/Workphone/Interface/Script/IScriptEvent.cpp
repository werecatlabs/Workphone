#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Script/IScriptEvent.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IScriptEvent, ISharedObject );

    IScriptEvent::~IScriptEvent() = default;

}  // namespace workphone
