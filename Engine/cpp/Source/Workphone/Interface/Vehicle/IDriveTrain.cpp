#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IDriveTrain.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::vehicle
{
    WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IDriveTrain, IVehicleComponent );

    IDriveTrain::~IDriveTrain() = default;

}  // namespace workphone::vehicle
