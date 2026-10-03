#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsManager, ISharedObject );

    IPhysicsManager::~IPhysicsManager() = default;

}  // namespace workphone::physics
