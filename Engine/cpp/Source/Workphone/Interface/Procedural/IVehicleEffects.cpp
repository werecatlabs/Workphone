#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IVehicleEffects.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, IVehicleEffects, ISharedObject );

    IVehicleEffects::~IVehicleEffects() = default;
}  // namespace workphone::procedural
