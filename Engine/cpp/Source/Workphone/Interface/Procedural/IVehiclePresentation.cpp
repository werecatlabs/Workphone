#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IVehiclePresentation.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, IVehiclePresentation, ISharedObject );

    IVehiclePresentation::~IVehiclePresentation() = default;
}  // namespace workphone::procedural
