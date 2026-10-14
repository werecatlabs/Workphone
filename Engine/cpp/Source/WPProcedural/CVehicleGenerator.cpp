#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CVehicleGenerator.hpp>
#include <WPProcedural/WPVehicleGenerator.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CVehicleGenerator, IVehicleGenerator );

    GeneratedVehicle CVehicleGenerator::generate( const VehicleGenerationConfig &config )
    {
        return WPVehicleGenerator::generate( config );
    }

    Array<VehicleGenerationIssue> CVehicleGenerator::validate( const GeneratedVehicle &vehicle )
    {
        return WPVehicleGenerator::validate( vehicle );
    }

    VehicleMaterialSlot CVehicleGenerator::materialSlotFor( VehicleMaterial material )
    {
        return WPVehicleGenerator::materialSlotFor( material );
    }
}  // namespace workphone::procedural
