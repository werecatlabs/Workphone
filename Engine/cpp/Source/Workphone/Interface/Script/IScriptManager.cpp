#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Script/IScriptManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IScriptManager, ISharedObject );

    IScriptManager::~IScriptManager() = default;

}  // namespace workphone
