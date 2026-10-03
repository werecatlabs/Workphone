#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <WPProcedural/WPVehicleDamage.hpp>
#include <WPProcedural/WPVehicleDynamics.hpp>

#include <array>

#include <Workphone/Interface/Procedural/VehiclePresentationTypes.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API WPVehiclePresentation
        {
        public:
            static VehiclePresentationState reset( const VehiclePhysicsConfig &physics );
            static bool update( const VehiclePresentationConfig &config,
                                const VehiclePhysicsConfig &physics,
                                const VehicleDynamicsState &dynamics,
                                const VehicleDynamicsTelemetry &telemetry,
                                const VehicleDamageTelemetry &damage,
                                const VehiclePresentationInput &input, double deltaSeconds,
                                VehiclePresentationState &state );
        };
    }  // namespace procedural
}  // namespace workphone
