#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IVehicleGeometry.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, IVehicleGeometry, ISharedObject );

    IVehicleGeometry::~IVehicleGeometry() = default;
}  // namespace workphone::procedural
