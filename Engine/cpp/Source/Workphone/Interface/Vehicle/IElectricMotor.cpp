#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IElectricMotor.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::vehicle
{
    WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IElectricMotor, IVehicleComponent );

    IElectricMotor::~IElectricMotor() = default;

}  // namespace workphone::vehicle
