#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/ITruckVehicle.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {

        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, ITruckVehicle, IVehicle );

        ITruckVehicle::~ITruckVehicle() = default;

    }  // namespace vehicle
}  // namespace workphone
