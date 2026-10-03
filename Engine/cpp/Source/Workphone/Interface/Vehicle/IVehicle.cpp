#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IVehicle.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::vehicle
{

    WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IVehicle, ISharedObject );

    IVehicle::~IVehicle() = default;

}  // namespace workphone::vehicle
