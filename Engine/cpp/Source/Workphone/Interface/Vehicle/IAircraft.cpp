#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IAircraft.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::vehicle
{
    WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IAircraft, IVehicle );

    IAircraft::~IAircraft() = default;
}  // namespace workphone::vehicle
