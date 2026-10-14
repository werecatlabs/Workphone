#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CVehiclePhysics.hpp>
#include <WPProcedural/WPVehiclePhysics.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CVehiclePhysics, IVehiclePhysics );

    VehiclePhysicsConfig CVehiclePhysics::generate( VehiclePhysicsPreset preset, std::uint64_t seed )
    {
        return WPVehiclePhysics::generate( preset, seed );
    }

    void CVehiclePhysics::recomputeDerivedProperties( VehiclePhysicsConfig &config )
    {
        WPVehiclePhysics::recomputeDerivedProperties( config );
    }

    VehiclePhysicsValidation CVehiclePhysics::validate( const VehiclePhysicsConfig &config )
    {
        return WPVehiclePhysics::validate( config );
    }

    double CVehiclePhysics::staticWheelLoadN( const VehiclePhysicsConfig &config, WheelCorner corner )
    {
        return WPVehiclePhysics::staticWheelLoadN( config, corner );
    }

    double CVehiclePhysics::aerodynamicDragN( const VehiclePhysicsConfig &config, double speedMps )
    {
        return WPVehiclePhysics::aerodynamicDragN( config, speedMps );
    }

    double CVehiclePhysics::aerodynamicDownforceN( const VehiclePhysicsConfig &config, double speedMps,
                                                   double rideHeightM )
    {
        return WPVehiclePhysics::aerodynamicDownforceN( config, speedMps, rideHeightM );
    }
}  // namespace workphone::procedural
