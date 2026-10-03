#pragma once
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Procedural/VehiclePhysicsTypes.hpp>

namespace workphone::procedural
{
    /// Replaceable service; inputs and CPU results belong to Workphone.
    class WPCore_API IVehiclePhysics : public ISharedObject
    {
    public:
        ~IVehiclePhysics() override;
        virtual VehiclePhysicsConfig generate( VehiclePhysicsPreset preset, std::uint64_t seed = 0 ) = 0;
        virtual void recomputeDerivedProperties( VehiclePhysicsConfig &config ) = 0;
        virtual VehiclePhysicsValidation validate( const VehiclePhysicsConfig &config ) = 0;
        virtual double staticWheelLoadN( const VehiclePhysicsConfig &config, WheelCorner corner ) = 0;
        virtual double aerodynamicDragN( const VehiclePhysicsConfig &config, double speedMps ) = 0;
        virtual double aerodynamicDownforceN( const VehiclePhysicsConfig &config, double speedMps,
                                              double rideHeightM ) = 0;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
