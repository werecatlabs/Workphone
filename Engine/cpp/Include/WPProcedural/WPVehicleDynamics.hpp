#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <WPProcedural/WPVehiclePhysics.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <Workphone/Interface/Procedural/VehicleDynamicsTypes.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API WPVehicleDynamics
        {
        public:
            static VehicleDynamicsState reset( const VehiclePhysicsConfig &physics,
                                               const VehiclePhysicsVector3 &position = {},
                                               double yawRad = 0.0 );

            static bool stepFixed( const VehiclePhysicsConfig &physics,
                                   const VehicleDynamicsTuning &tuning, const VehicleControlInput &input,
                                   const std::array<VehicleSurfaceSample, 4> &surfaces,
                                   double fixedDeltaSeconds, VehicleDynamicsState &state,
                                   VehicleDynamicsTelemetry &telemetry );

            static bool stepFixed( const VehiclePhysicsConfig &physics,
                                   const VehicleDynamicsTuning &tuning, const VehicleControlInput &input,
                                   const std::array<VehicleSurfaceSample, 4> &surfaces,
                                   const VehicleDynamicsModifiers &modifiers, double fixedDeltaSeconds,
                                   VehicleDynamicsState &state, VehicleDynamicsTelemetry &telemetry );

            static VehicleDynamicsValidation validate( const VehiclePhysicsConfig &physics,
                                                       const VehicleDynamicsTuning &tuning );
            static VehicleDynamicsValidation validateState( const VehiclePhysicsConfig &physics,
                                                            const VehicleDynamicsState &state );
        };
    }  // namespace procedural
}  // namespace workphone
