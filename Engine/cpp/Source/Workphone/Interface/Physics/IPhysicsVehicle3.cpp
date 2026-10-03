#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsVehicle3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{

    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsVehicle3, ISharedObject );

    IPhysicsVehicle3::~IPhysicsVehicle3() = default;

}  // namespace workphone::physics
