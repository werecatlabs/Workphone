#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CVehicleDynamics.hpp>
#include <WPProcedural/WPVehicleDynamics.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CVehicleDynamics, IVehicleDynamics );

    VehicleDynamicsState CVehicleDynamics::reset( const VehiclePhysicsConfig &physics,
                                                  const VehiclePhysicsVector3 &position, double yawRad )
    {
        return WPVehicleDynamics::reset( physics, position, yawRad );
    }

    bool CVehicleDynamics::stepFixed( const VehiclePhysicsConfig &physics,
                                      const VehicleDynamicsTuning &tuning,
                                      const VehicleControlInput &input,
                                      const std::array<VehicleSurfaceSample, 4> &surfaces,
                                      double fixedDeltaSeconds, VehicleDynamicsState &state,
                                      VehicleDynamicsTelemetry &telemetry )
    {
        return WPVehicleDynamics::stepFixed( physics, tuning, input, surfaces, fixedDeltaSeconds, state,
                                             telemetry );
    }

    bool CVehicleDynamics::stepFixed( const VehiclePhysicsConfig &physics,
                                      const VehicleDynamicsTuning &tuning,
                                      const VehicleControlInput &input,
                                      const std::array<VehicleSurfaceSample, 4> &surfaces,
                                      const VehicleDynamicsModifiers &modifiers,
                                      double fixedDeltaSeconds, VehicleDynamicsState &state,
                                      VehicleDynamicsTelemetry &telemetry )
    {
        return WPVehicleDynamics::stepFixed( physics, tuning, input, surfaces, modifiers,
                                             fixedDeltaSeconds, state, telemetry );
    }

    VehicleDynamicsValidation CVehicleDynamics::validate( const VehiclePhysicsConfig &physics,
                                                          const VehicleDynamicsTuning &tuning )
    {
        return WPVehicleDynamics::validate( physics, tuning );
    }

    VehicleDynamicsValidation CVehicleDynamics::validateState( const VehiclePhysicsConfig &physics,
                                                               const VehicleDynamicsState &state )
    {
        return WPVehicleDynamics::validateState( physics, state );
    }
}  // namespace workphone::procedural
