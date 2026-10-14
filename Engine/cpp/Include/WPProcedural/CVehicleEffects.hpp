#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/IVehicleEffects.hpp>

namespace workphone::procedural
{
    class WPProcedural_API CVehicleEffects : public IVehicleEffects
    {
    public:
        VehicleEffectsState reset( std::uint64_t seed ) override;
        bool emit( const VehicleEffectConfig &config, const VehiclePhysicsConfig &physics,
                   const VehicleDynamicsState &dynamics, const VehicleDynamicsTelemetry &telemetry,
                   const VehiclePresentationState &presentation, const VehicleDamageTelemetry &damage,
                   const VehicleEffectEnvironment &environment, double fixedDeltaSeconds,
                   VehicleEffectsState &state, Array<VehicleEffectEvent> &events ) override;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
