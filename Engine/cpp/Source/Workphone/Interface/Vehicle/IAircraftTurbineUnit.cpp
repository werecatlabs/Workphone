#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IAircraftTurbineUnit.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {
        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IAircraftTurbineUnit, IVehicleComponent );

        IAircraftTurbineUnit::~IAircraftTurbineUnit() = default;

    }  // namespace vehicle
}  // namespace workphone
