#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IAircraftControlSurface.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {
        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IAircraftControlSurface, IVehicleComponent );

        IAircraftControlSurface::~IAircraftControlSurface() = default;

    }  // namespace vehicle
}  // namespace workphone
