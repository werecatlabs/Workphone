#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <WPProcedural/WPVehicleDynamics.hpp>
#include <WPProcedural/WPVehiclePhysics.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include <Workphone/Interface/Procedural/VehicleDamageTypes.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API WPVehicleDamage
        {
        public:
            static VehicleDamageState reset( std::uint64_t seed = 0 );
            static VehicleDamageEvent registerImpact( const VehicleDamageConfig &config,
                                                      const VehiclePhysicsConfig &physics,
                                                      const VehicleDamageImpact &impact,
                                                      VehicleDamageState &state );
            static bool update( const VehicleDamageConfig &config, double fixedDeltaSeconds,
                                VehicleDamageState &state );
            static VehicleDamageTelemetry telemetry( const VehicleDamageConfig &config,
                                                     const VehicleDamageState &state );
            static VehicleDynamicsModifiers dynamicsModifiers( const VehicleDamageTelemetry &telemetry );
            /// Applies the discrete speed loss and yaw kick returned by registerImpact.
            /// This is opt-in so collision solvers can substitute their own impulse.
            static bool applyImpactResponse( const VehicleDamageEvent &event,
                                             VehicleDynamicsState &dynamics );
            static VehicleDamageValidation validate( const VehicleDamageConfig &config );
            static VehicleDamageValidation validateState( const VehicleDamageState &state );
        };
    }  // namespace procedural
}  // namespace workphone
