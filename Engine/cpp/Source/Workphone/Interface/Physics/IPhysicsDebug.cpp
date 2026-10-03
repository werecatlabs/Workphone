#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsDebug.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsDebug, ISharedObject );

    IPhysicsDebug::~IPhysicsDebug() = default;

}  // namespace workphone::physics
