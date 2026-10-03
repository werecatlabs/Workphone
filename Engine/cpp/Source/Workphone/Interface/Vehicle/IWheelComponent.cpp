#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IWheelComponent.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {

        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IWheelComponent, IVehicleComponent );

        IWheelComponent::~IWheelComponent() = default;

    }  // namespace vehicle
}  // namespace workphone
