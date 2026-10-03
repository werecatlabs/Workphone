#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/ProceduralServices.hpp>
namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CRoadSystem, IRoadSystem );
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CSkyAtmosphere, ISkyAtmosphere );
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CTextureForge, ITextureForge );
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CVehicleAppearance, IVehicleAppearance );
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CVehicleDamage, IVehicleDamage );
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CVehicleDynamics, IVehicleDynamics );
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CVehicleEffects, IVehicleEffects );
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CVehicleGenerator, IVehicleGenerator );
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CVehicleGeometry, IVehicleGeometry );
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CVehiclePhysics, IVehiclePhysics );
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CVehiclePresentation, IVehiclePresentation );
}  // namespace workphone::procedural
