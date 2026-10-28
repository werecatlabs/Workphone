#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <Workphone/Interface/Procedural/IVehicleDamage.hpp>

namespace workphone::procedural
{
    class WPProcedural_API CVehicleDamage : public IVehicleDamage, public SourceBudgetObject
    {
    public:
        using SourceBudgetObject::operator new;
        using SourceBudgetObject::operator delete;

        VehicleDamageState reset( std::uint64_t seed ) override;
        VehicleDamageEvent registerImpact( const VehicleDamageConfig &config,
                                           const VehiclePhysicsConfig &physics,
                                           const VehicleDamageImpact &impact,
                                           VehicleDamageState &state ) override;
        bool update( const VehicleDamageConfig &config, double fixedDeltaSeconds,
                     VehicleDamageState &state ) override;
        VehicleDamageTelemetry telemetry( const VehicleDamageConfig &config,
                                          const VehicleDamageState &state ) override;
        VehicleDynamicsModifiers dynamicsModifiers( const VehicleDamageTelemetry &telemetry ) override;
        bool applyImpactResponse( const VehicleDamageEvent &event,
                                  VehicleDynamicsState &dynamics ) override;
        VehicleDamageValidation validate( const VehicleDamageConfig &config ) override;
        VehicleDamageValidation validateState( const VehicleDamageState &state ) override;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
