#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <Workphone/Interface/Procedural/IVehicleDynamics.hpp>

namespace workphone::procedural
{
    class WPProcedural_API CVehicleDynamics : public IVehicleDynamics, public SourceBudgetObject
    {
    public:
        using SourceBudgetObject::operator new;
        using SourceBudgetObject::operator delete;

        VehicleDynamicsState reset( const VehiclePhysicsConfig &physics,
                                    const VehiclePhysicsVector3 &position, double yawRad ) override;
        bool stepFixed( const VehiclePhysicsConfig &physics, const VehicleDynamicsTuning &tuning,
                        const VehicleControlInput &input,
                        const std::array<VehicleSurfaceSample, 4> &surfaces, double fixedDeltaSeconds,
                        VehicleDynamicsState &state, VehicleDynamicsTelemetry &telemetry ) override;
        bool stepFixed( const VehiclePhysicsConfig &physics, const VehicleDynamicsTuning &tuning,
                        const VehicleControlInput &input,
                        const std::array<VehicleSurfaceSample, 4> &surfaces,
                        const VehicleDynamicsModifiers &modifiers, double fixedDeltaSeconds,
                        VehicleDynamicsState &state, VehicleDynamicsTelemetry &telemetry ) override;
        VehicleDynamicsValidation validate( const VehiclePhysicsConfig &physics,
                                            const VehicleDynamicsTuning &tuning ) override;
        VehicleDynamicsValidation validateState( const VehiclePhysicsConfig &physics,
                                                 const VehicleDynamicsState &state ) override;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
