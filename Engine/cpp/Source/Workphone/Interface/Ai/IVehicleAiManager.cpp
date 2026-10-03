#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Ai/IVehicleAiManager.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IVehicleAiManager, ISharedObject );

    IVehicleAiManager::~IVehicleAiManager() = default;

}  // namespace workphone
