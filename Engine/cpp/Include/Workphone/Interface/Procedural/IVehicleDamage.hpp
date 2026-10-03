#pragma once
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Procedural/VehicleDamageTypes.hpp>

namespace workphone::procedural
{
    /// Replaceable service; inputs and CPU results belong to Workphone.
    class WPCore_API IVehicleDamage : public ISharedObject
    {
    public:
        ~IVehicleDamage() override;
        virtual VehicleDamageState reset( std::uint64_t seed = 0 ) = 0;
        virtual VehicleDamageEvent registerImpact( const VehicleDamageConfig &config,
                                                   const VehiclePhysicsConfig &physics,
                                                   const VehicleDamageImpact &impact,
                                                   VehicleDamageState &state ) = 0;
        virtual bool update( const VehicleDamageConfig &config, double fixedDeltaSeconds,
                             VehicleDamageState &state ) = 0;
        virtual VehicleDamageTelemetry telemetry( const VehicleDamageConfig &config,
                                                  const VehicleDamageState &state ) = 0;
        virtual VehicleDynamicsModifiers dynamicsModifiers(
            const VehicleDamageTelemetry &telemetry ) = 0;
        virtual bool applyImpactResponse( const VehicleDamageEvent &event,
                                          VehicleDynamicsState &dynamics ) = 0;
        virtual VehicleDamageValidation validate( const VehicleDamageConfig &config ) = 0;
        virtual VehicleDamageValidation validateState( const VehicleDamageState &state ) = 0;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
