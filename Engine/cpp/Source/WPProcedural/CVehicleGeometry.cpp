#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CVehicleGeometry.hpp>
#include <WPProcedural/WPVehicleGeometry.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CVehicleGeometry, IVehicleGeometry );

    VehicleGeometry CVehicleGeometry::generate( const VehicleGeometryConfig &config )
    {
        return WPVehicleGeometry::generate( config );
    }
}  // namespace workphone::procedural
