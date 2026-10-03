#pragma once
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Procedural/VehicleDynamicsTypes.hpp>

namespace workphone::procedural
{
    /// Replaceable service; inputs and CPU results belong to Workphone.
    class WPCore_API IVehicleDynamics : public ISharedObject
    {
    public:
        ~IVehicleDynamics() override;
        virtual VehicleDynamicsState reset( const VehiclePhysicsConfig &physics,
                                            const VehiclePhysicsVector3 &position = {},
                                            double yawRad = 0.0 ) = 0;
        virtual bool stepFixed( const VehiclePhysicsConfig &physics, const VehicleDynamicsTuning &tuning,
                                const VehicleControlInput &input,
                                const std::array<VehicleSurfaceSample, 4> &surfaces,
                                double fixedDeltaSeconds, VehicleDynamicsState &state,
                                VehicleDynamicsTelemetry &telemetry ) = 0;
        virtual bool stepFixed( const VehiclePhysicsConfig &physics, const VehicleDynamicsTuning &tuning,
                                const VehicleControlInput &input,
                                const std::array<VehicleSurfaceSample, 4> &surfaces,
                                const VehicleDynamicsModifiers &modifiers, double fixedDeltaSeconds,
                                VehicleDynamicsState &state, VehicleDynamicsTelemetry &telemetry ) = 0;
        virtual VehicleDynamicsValidation validate( const VehiclePhysicsConfig &physics,
                                                    const VehicleDynamicsTuning &tuning ) = 0;
        virtual VehicleDynamicsValidation validateState( const VehiclePhysicsConfig &physics,
                                                         const VehicleDynamicsState &state ) = 0;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
