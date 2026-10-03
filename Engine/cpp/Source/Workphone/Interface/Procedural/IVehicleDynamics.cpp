#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IVehicleDynamics.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, IVehicleDynamics, ISharedObject );

    IVehicleDynamics::~IVehicleDynamics() = default;
}  // namespace workphone::procedural
