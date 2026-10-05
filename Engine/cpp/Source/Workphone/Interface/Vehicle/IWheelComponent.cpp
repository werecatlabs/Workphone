#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IWheelComponent.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {

        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IWheelComponent, IVehicleComponent );

        IWheelComponent::~IWheelComponent() = default;

        void IWheelComponent::setGrip( physics_Num grip )
        {
            auto properties = getProperties();
            properties->setProperty( "Grip", grip );
            setProperties( properties );
        }

    }  // namespace vehicle
}  // namespace workphone
