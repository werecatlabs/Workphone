#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IVehiclePhysics.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, IVehiclePhysics, ISharedObject );

    IVehiclePhysics::~IVehiclePhysics() = default;
}  // namespace workphone::procedural
