#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IPluginManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IPluginManager, ISharedObject );

    IPluginManager::~IPluginManager() = default;
}  // namespace workphone
