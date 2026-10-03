#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IAerodymanicsWind.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::vehicle
{
    WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IAerodymanicsWind, IVehicleComponent );

    IAerodymanicsWind::~IAerodymanicsWind() = default;
}  // namespace workphone::vehicle
