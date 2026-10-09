#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IVehicleVisualEffects.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
namespace workphone::advanced
{
WP_CLASS_REGISTER_DERIVED( workphone::advanced, IVehicleVisualEffects, ISharedObject );
IVehicleVisualEffects::~IVehicleVisualEffects() = default;
}  // namespace workphone::advanced
