#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IAircraftPowerUnit.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {
        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IAircraftPowerUnit, IVehiclePowerUnit );

        IAircraftPowerUnit::~IAircraftPowerUnit() = default;

    }  // namespace vehicle
}  // namespace workphone
