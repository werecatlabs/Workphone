#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IAircraftBody.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {
        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IAircraftBody, IVehicleComponent );

        IAircraftBody::~IAircraftBody() = default;

    }  // namespace vehicle
}  // namespace workphone
