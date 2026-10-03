#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IAircraftPropeller.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {
        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IAircraftPropeller, IVehicleComponent );

        IAircraftPropeller::~IAircraftPropeller() = default;

    }  // namespace vehicle
}  // namespace workphone
