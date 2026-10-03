#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Script/IScriptFunction.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IScriptFunction, ISharedObject );

    IScriptFunction::~IScriptFunction() = default;

}  // namespace workphone
