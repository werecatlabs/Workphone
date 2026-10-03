#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {

        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IVehicleComponent, ISharedObject );

        IVehicleComponent::~IVehicleComponent() = default;

    }  // namespace vehicle
}  // namespace workphone
