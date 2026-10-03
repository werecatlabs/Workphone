#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Script/IScriptVariable.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IScriptVariable, ISharedObject );

    IScriptVariable::~IScriptVariable() = default;

}  // namespace workphone
