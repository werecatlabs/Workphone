#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IVehicleAppearance.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, IVehicleAppearance, ISharedObject );

    IVehicleAppearance::~IVehicleAppearance() = default;
}  // namespace workphone::procedural
