#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IVehiclePowerUnit.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {

        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IVehiclePowerUnit, IVehicleComponent );

        IVehiclePowerUnit::~IVehiclePowerUnit() = default;

    }  // namespace vehicle
}  // namespace workphone
