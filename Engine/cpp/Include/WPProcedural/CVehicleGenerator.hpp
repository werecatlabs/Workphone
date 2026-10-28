#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <Workphone/Interface/Procedural/IVehicleGenerator.hpp>

namespace workphone::procedural
{
    class WPProcedural_API CVehicleGenerator : public IVehicleGenerator, public SourceBudgetObject
    {
    public:
        using SourceBudgetObject::operator new;
        using SourceBudgetObject::operator delete;

        GeneratedVehicle generate( const VehicleGenerationConfig &config ) override;
        Array<VehicleGenerationIssue> validate( const GeneratedVehicle &vehicle ) override;
        VehicleMaterialSlot materialSlotFor( VehicleMaterial material ) override;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
