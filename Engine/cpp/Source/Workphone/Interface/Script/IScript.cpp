#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Script/IScript.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IScript, ISharedObject );

    IScript::~IScript() = default;

}  // namespace workphone
