// WPVehicleGenerator.hpp - integrated procedural vehicle asset generation.
#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <WPProcedural/WPVehicleAppearance.hpp>
#include <WPProcedural/WPVehicleGeometry.hpp>
#include <WPProcedural/WPVehiclePhysics.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include <Workphone/Interface/Procedural/VehicleGeneratorTypes.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API WPVehicleGenerator
        {
        public:
            static GeneratedVehicle generate( const VehicleGenerationConfig &config = {} );

            static std::vector<VehicleGenerationIssue> validate( const GeneratedVehicle &vehicle );

            static VehicleMaterialSlot materialSlotFor( VehicleMaterial material );
        };
    }  // namespace procedural
}  // namespace workphone
