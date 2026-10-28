#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <Workphone/Interface/Procedural/IVehiclePhysics.hpp>

namespace workphone::procedural
{
    class WPProcedural_API CVehiclePhysics : public IVehiclePhysics, public SourceBudgetObject
    {
    public:
        using SourceBudgetObject::operator new;
        using SourceBudgetObject::operator delete;

        VehiclePhysicsConfig generate( VehiclePhysicsPreset preset, std::uint64_t seed ) override;
        void recomputeDerivedProperties( VehiclePhysicsConfig &config ) override;
        VehiclePhysicsValidation validate( const VehiclePhysicsConfig &config ) override;
        double staticWheelLoadN( const VehiclePhysicsConfig &config, WheelCorner corner ) override;
        double aerodynamicDragN( const VehiclePhysicsConfig &config, double speedMps ) override;
        double aerodynamicDownforceN( const VehiclePhysicsConfig &config, double speedMps,
                                      double rideHeightM ) override;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
