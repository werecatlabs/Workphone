#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IVehicleBody.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::vehicle
{

    WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IVehicleBody, IVehicleComponent );

    IVehicleBody::~IVehicleBody() = default;

}  // namespace workphone::vehicle
