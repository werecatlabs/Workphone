#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsVehicleWheel3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsVehicleWheel3, ISharedObject );

    IPhysicsVehicleWheel3::~IPhysicsVehicleWheel3() = default;

}  // namespace workphone::physics
