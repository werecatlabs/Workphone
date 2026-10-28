#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <Workphone/Interface/Procedural/IVehiclePresentation.hpp>

namespace workphone::procedural
{
    class WPProcedural_API CVehiclePresentation : public IVehiclePresentation, public SourceBudgetObject
    {
    public:
        using SourceBudgetObject::operator new;
        using SourceBudgetObject::operator delete;

        VehiclePresentationState reset( const VehiclePhysicsConfig &physics ) override;
        bool update( const VehiclePresentationConfig &config, const VehiclePhysicsConfig &physics,
                     const VehicleDynamicsState &dynamics, const VehicleDynamicsTelemetry &telemetry,
                     const VehicleDamageTelemetry &damage, const VehiclePresentationInput &input,
                     double deltaSeconds, VehiclePresentationState &state ) override;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
