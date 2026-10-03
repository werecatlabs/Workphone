#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IDrone.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::vehicle
{
    WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IDrone, IVehicle );

    IDrone::~IDrone() = default;

}  // namespace workphone::vehicle
