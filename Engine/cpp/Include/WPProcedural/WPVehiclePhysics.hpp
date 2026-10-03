#ifndef WPProceduralVehiclePhysics_h__
#define WPProceduralVehiclePhysics_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <Workphone/Interface/Procedural/VehiclePhysicsTypes.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API WPVehiclePhysics
        {
        public:
            /// Builds a deterministic, simulation-ready preset. Seed affects small setup
            /// tolerances only; dimensions and homologated mass remain stable.
            static VehiclePhysicsConfig generate( VehiclePhysicsPreset preset, std::uint64_t seed = 0 );

            /// Rebuilds mass, COM, full tensor, weight distribution and driven flags.
            static void recomputeDerivedProperties( VehiclePhysicsConfig &config );

            /// Strict physical and numerical validation suitable for import-time checks.
            static VehiclePhysicsValidation validate( const VehiclePhysicsConfig &config );

            /// Static normal load at a corner before aerodynamic load, in newtons.
            static double staticWheelLoadN( const VehiclePhysicsConfig &config, WheelCorner corner );

            /// Aerodynamic drag/downforce magnitudes at the requested speed, in newtons.
            static double aerodynamicDragN( const VehiclePhysicsConfig &config, double speedMps );
            static double aerodynamicDownforceN( const VehiclePhysicsConfig &config, double speedMps,
                                                 double rideHeightM );
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // WPProceduralVehiclePhysics_h__
