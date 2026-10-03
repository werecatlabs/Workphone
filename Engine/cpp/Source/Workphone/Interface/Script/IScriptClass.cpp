#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Script/IScriptClass.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IScriptClass, ISharedObject );

    IScriptClass::~IScriptClass() = default;

}  // namespace workphone
