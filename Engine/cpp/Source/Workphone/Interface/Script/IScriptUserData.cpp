#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Script/IScriptUserData.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IScriptUserData, ISharedObject );

    IScriptUserData::~IScriptUserData() = default;

}  // namespace workphone
