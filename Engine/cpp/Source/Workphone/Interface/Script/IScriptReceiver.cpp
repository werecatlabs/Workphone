#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Script/IScriptReceiver.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IScriptReceiver, ISharedObject );

    IScriptReceiver::~IScriptReceiver() = default;

}  // namespace workphone
