#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CVehiclePresentation.hpp>
#include <WPProcedural/WPVehiclePresentation.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CVehiclePresentation, IVehiclePresentation );

    VehiclePresentationState CVehiclePresentation::reset( const VehiclePhysicsConfig &physics )
    {
        return WPVehiclePresentation::reset( physics );
    }

    bool CVehiclePresentation::update( const VehiclePresentationConfig &config,
                                       const VehiclePhysicsConfig &physics,
                                       const VehicleDynamicsState &dynamics,
                                       const VehicleDynamicsTelemetry &telemetry,
                                       const VehicleDamageTelemetry &damage,
                                       const VehiclePresentationInput &input, double deltaSeconds,
                                       VehiclePresentationState &state )
    {
        return WPVehiclePresentation::update( config, physics, dynamics, telemetry, damage, input,
                                              deltaSeconds, state );
    }
}  // namespace workphone::procedural
