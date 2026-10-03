#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IDifferential.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::vehicle
{
    WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IDifferential, IVehicleComponent );

    IDifferential::~IDifferential() = default;

}  // namespace workphone::vehicle
