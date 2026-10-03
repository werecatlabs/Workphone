#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Script/IScriptBreakpoint.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IScriptBreakpoint, ISharedObject );

    IScriptBreakpoint::~IScriptBreakpoint() = default;

}  // namespace workphone
