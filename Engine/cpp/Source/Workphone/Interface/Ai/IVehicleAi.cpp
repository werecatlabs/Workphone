#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Ai/IVehicleAi.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IVehicleAi, ISharedObject );

    IVehicleAi::~IVehicleAi() = default;

}  // namespace workphone
