#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IVehicleManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {

        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IVehicleManager, ISharedObject );

        IVehicleManager::~IVehicleManager() = default;

    }  // namespace vehicle
}  // namespace workphone
