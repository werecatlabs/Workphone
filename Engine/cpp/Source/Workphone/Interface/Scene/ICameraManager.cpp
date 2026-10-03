#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, ICameraManager, ISharedObject );

    ICameraManager::~ICameraManager() = default;

}  // namespace workphone::scene
