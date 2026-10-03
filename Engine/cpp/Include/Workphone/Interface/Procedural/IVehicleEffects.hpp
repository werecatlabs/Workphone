#pragma once
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Procedural/VehicleEffectsTypes.hpp>

namespace workphone::procedural
{
    /// Replaceable service; inputs and CPU results belong to Workphone.
    class WPCore_API IVehicleEffects : public ISharedObject
    {
    public:
        ~IVehicleEffects() override;
        virtual VehicleEffectsState reset( std::uint64_t seed = 0 ) = 0;
        virtual bool emit( const VehicleEffectConfig &config, const VehiclePhysicsConfig &physics,
                           const VehicleDynamicsState &dynamics,
                           const VehicleDynamicsTelemetry &telemetry,
                           const VehiclePresentationState &presentation,
                           const VehicleDamageTelemetry &damage,
                           const VehicleEffectEnvironment &environment, double fixedDeltaSeconds,
                           VehicleEffectsState &state, std::vector<VehicleEffectEvent> &events ) = 0;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
