#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IAircraftCallback.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {
        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IAircraftCallback, IVehicleCallback );

        IAircraftCallback::~IAircraftCallback() = default;

    }  // namespace vehicle
}  // namespace workphone
