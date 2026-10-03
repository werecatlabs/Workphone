#pragma once
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Procedural/VehiclePresentationTypes.hpp>

namespace workphone::procedural
{
    /// Replaceable service; inputs and CPU results belong to Workphone.
    class WPCore_API IVehiclePresentation : public ISharedObject
    {
    public:
        ~IVehiclePresentation() override;
        virtual VehiclePresentationState reset( const VehiclePhysicsConfig &physics ) = 0;
        virtual bool update( const VehiclePresentationConfig &config,
                             const VehiclePhysicsConfig &physics, const VehicleDynamicsState &dynamics,
                             const VehicleDynamicsTelemetry &telemetry,
                             const VehicleDamageTelemetry &damage, const VehiclePresentationInput &input,
                             double deltaSeconds, VehiclePresentationState &state ) = 0;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
