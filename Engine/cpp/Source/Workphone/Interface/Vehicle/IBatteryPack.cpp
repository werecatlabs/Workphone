#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IBatteryPack.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {
        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IBatteryPack, IVehicleComponent );

        IBatteryPack::~IBatteryPack() = default;

    }  // namespace vehicle
}  // namespace workphone
