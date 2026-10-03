#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IFourWheelVehicle.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::vehicle
{
    WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IFourWheelVehicle, IVehicle );

    IFourWheelVehicle::~IFourWheelVehicle() = default;

}  // namespace workphone::vehicle
