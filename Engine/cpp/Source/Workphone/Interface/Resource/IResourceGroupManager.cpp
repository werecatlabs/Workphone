#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IResourceGroupManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IResourceGroupManager, ISharedObject );

    IResourceGroupManager::~IResourceGroupManager() = default;

}  // namespace workphone
