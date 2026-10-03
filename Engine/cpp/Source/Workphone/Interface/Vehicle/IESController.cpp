#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IESController.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::vehicle
{
    WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IESController, IVehicleComponent );

    IESController::~IESController() = default;

}  // namespace workphone::vehicle
