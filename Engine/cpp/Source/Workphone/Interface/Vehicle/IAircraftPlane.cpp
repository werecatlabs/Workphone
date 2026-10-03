#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IAircraftPlane.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {
        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IAircraftPlane, IAircraft );

        IAircraftPlane::~IAircraftPlane() = default;

    }  // namespace vehicle
}  // namespace workphone
