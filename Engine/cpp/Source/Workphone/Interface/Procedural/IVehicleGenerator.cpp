#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IVehicleGenerator.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, IVehicleGenerator, ISharedObject );

    IVehicleGenerator::~IVehicleGenerator() = default;
}  // namespace workphone::procedural
