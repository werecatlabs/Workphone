#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CVehicleEffects.hpp>
#include <WPProcedural/WPVehicleEffects.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CVehicleEffects, IVehicleEffects );

    VehicleEffectsState CVehicleEffects::reset( std::uint64_t seed )
    {
        return WPVehicleEffects::reset( seed );
    }

    bool CVehicleEffects::emit( const VehicleEffectConfig &config, const VehiclePhysicsConfig &physics,
                                const VehicleDynamicsState &dynamics,
                                const VehicleDynamicsTelemetry &telemetry,
                                const VehiclePresentationState &presentation,
                                const VehicleDamageTelemetry &damage,
                                const VehicleEffectEnvironment &environment, double fixedDeltaSeconds,
                                VehicleEffectsState &state, Array<VehicleEffectEvent> &events )
    {
        return WPVehicleEffects::emit( config, physics, dynamics, telemetry, presentation, damage,
                                       environment, fixedDeltaSeconds, state, events );
    }
}  // namespace workphone::procedural
