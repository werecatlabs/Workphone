#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IPackageManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IPackageManager, ISharedObject );

    IPackageManager::~IPackageManager() = default;

}  // namespace workphone
