// ============================================================================
// WPVehicleAppearance.hpp - deterministic procedural vehicle surface authoring
// ============================================================================

#ifndef WPVehicleAppearance_h__
#define WPVehicleAppearance_h__

#include "WPProcedural/WPProceduralPrerequisites.hpp"
#include "WPProcedural/WPProceduralTextureData.hpp"
#include "WPProcedural/WPVehicleGeometry.hpp"
#include <array>
#include <string>
#include <vector>

#include <Workphone/Interface/Procedural/VehicleAppearanceTypes.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API WPVehicleAppearance
        {
        public:
            explicit WPVehicleAppearance( u32 seed = 0x7f4a7c15u );

            VehicleAppearanceBundle build( const VehicleAppearanceConfig &config ) const;

            static VehicleAppearanceQualityProfile profileFor( VehicleAppearanceQuality quality );
            /// Resolves each explicit geometry material to its renderer-facing appearance slot.
            static VehicleMaterialSlot slotForGeometryMaterial( VehicleMaterial material );
            static VehicleAppearanceValidation validate( const VehicleAppearanceConfig &config );
            static VehicleAppearanceValidation validate( const VehicleAppearanceBundle &bundle );

        private:
            u32 mSeed;
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // WPVehicleAppearance_h__
