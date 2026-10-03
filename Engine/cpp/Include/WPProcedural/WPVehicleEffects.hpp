#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <WPProcedural/WPVehicleDamage.hpp>
#include <WPProcedural/WPVehicleDynamics.hpp>
#include <WPProcedural/WPVehiclePresentation.hpp>

#include <array>
#include <cstdint>
#include <vector>

#include <Workphone/Interface/Procedural/VehicleEffectsTypes.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API WPVehicleEffects
        {
        public:
            static VehicleEffectsState reset( std::uint64_t seed = 0 );
            static bool emit( const VehicleEffectConfig &config, const VehiclePhysicsConfig &physics,
                              const VehicleDynamicsState &dynamics,
                              const VehicleDynamicsTelemetry &telemetry,
                              const VehiclePresentationState &presentation,
                              const VehicleDamageTelemetry &damage,
                              const VehicleEffectEnvironment &environment, double fixedDeltaSeconds,
                              VehicleEffectsState &state, std::vector<VehicleEffectEvent> &events );
        };
    }  // namespace procedural
}  // namespace workphone
