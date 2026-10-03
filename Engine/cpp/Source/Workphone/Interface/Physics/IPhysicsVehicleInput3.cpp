#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsVehicleInput3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsVehicleInput3, ISharedObject );

    IPhysicsVehicleInput3::~IPhysicsVehicleInput3() = default;

}  // namespace workphone::physics
