#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/IVehicleGenerator.hpp>

namespace workphone::procedural
{
    class WPProcedural_API CVehicleGenerator : public IVehicleGenerator
    {
    public:
        GeneratedVehicle generate( const VehicleGenerationConfig &config ) override;
        Array<VehicleGenerationIssue> validate( const GeneratedVehicle &vehicle ) override;
        VehicleMaterialSlot materialSlotFor( VehicleMaterial material ) override;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
