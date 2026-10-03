#pragma once
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Procedural/VehicleAppearanceTypes.hpp>

namespace workphone::procedural
{
    /// Replaceable service; inputs and CPU results belong to Workphone.
    class WPCore_API IVehicleAppearance : public ISharedObject
    {
    public:
        ~IVehicleAppearance() override;
        virtual VehicleAppearanceBundle build( const VehicleAppearanceConfig &config ) const = 0;
        virtual VehicleAppearanceQualityProfile profileFor( VehicleAppearanceQuality quality ) = 0;
        virtual VehicleMaterialSlot slotForGeometryMaterial( VehicleMaterial material ) = 0;
        virtual VehicleAppearanceValidation validate( const VehicleAppearanceConfig &config ) = 0;
        virtual VehicleAppearanceValidation validate( const VehicleAppearanceBundle &bundle ) = 0;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
