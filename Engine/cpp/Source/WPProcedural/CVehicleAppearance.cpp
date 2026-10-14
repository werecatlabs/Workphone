#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CVehicleAppearance.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CVehicleAppearance, IVehicleAppearance );

    VehicleAppearanceBundle CVehicleAppearance::build( const VehicleAppearanceConfig &config ) const
    {
        return mImplementation.build( config );
    }

    VehicleAppearanceQualityProfile CVehicleAppearance::profileFor( VehicleAppearanceQuality quality )
    {
        return WPVehicleAppearance::profileFor( quality );
    }

    VehicleMaterialSlot CVehicleAppearance::slotForGeometryMaterial( VehicleMaterial material )
    {
        return WPVehicleAppearance::slotForGeometryMaterial( material );
    }

    VehicleAppearanceValidation CVehicleAppearance::validate( const VehicleAppearanceConfig &config )
    {
        return WPVehicleAppearance::validate( config );
    }

    VehicleAppearanceValidation CVehicleAppearance::validate( const VehicleAppearanceBundle &bundle )
    {
        return WPVehicleAppearance::validate( bundle );
    }
}  // namespace workphone::procedural
