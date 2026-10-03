#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IAircraftWing.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {
        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IAircraftWing, IVehicleComponent );

        IAircraftWing::~IAircraftWing() = default;

    }  // namespace vehicle
}  // namespace workphone
