#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/IVehicleAppearance.hpp>
#include <WPProcedural/WPVehicleAppearance.hpp>

namespace workphone::procedural
{
    class WPProcedural_API CVehicleAppearance : public IVehicleAppearance
    {
    public:
        VehicleAppearanceBundle build( const VehicleAppearanceConfig &config ) const override;
        VehicleAppearanceQualityProfile profileFor( VehicleAppearanceQuality quality ) override;
        VehicleMaterialSlot slotForGeometryMaterial( VehicleMaterial material ) override;
        VehicleAppearanceValidation validate( const VehicleAppearanceConfig &config ) override;
        VehicleAppearanceValidation validate( const VehicleAppearanceBundle &bundle ) override;
        WP_CLASS_REGISTER_DECL;

    private:
        WPVehicleAppearance mImplementation;
    };
}  // namespace workphone::procedural
