#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IAircraftPropellerUnit.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {
        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IAircraftPropellerUnit, IVehicleComponent );

        IAircraftPropellerUnit::~IAircraftPropellerUnit() = default;

    }  // namespace vehicle
}  // namespace workphone
