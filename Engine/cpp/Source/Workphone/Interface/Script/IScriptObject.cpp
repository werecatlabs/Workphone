#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Script/IScriptObject.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IScriptObject, ISharedObject );

    IScriptObject::~IScriptObject() = default;

}  // namespace workphone
