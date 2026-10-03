#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IVehicleCallback.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::vehicle
{

    WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IVehicleCallback, ISharedObject );

    IVehicleCallback::~IVehicleCallback() = default;

}  // namespace workphone::vehicle
