#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IAircraftPropWash.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {
        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IAircraftPropWash, IVehicleComponent );

        IAircraftPropWash::~IAircraftPropWash() = default;

    }  // namespace vehicle
}  // namespace workphone
