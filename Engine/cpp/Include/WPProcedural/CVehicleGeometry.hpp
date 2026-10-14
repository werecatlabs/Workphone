#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/IVehicleGeometry.hpp>

namespace workphone::procedural
{
    class WPProcedural_API CVehicleGeometry : public IVehicleGeometry
    {
    public:
        VehicleGeometry generate( const VehicleGeometryConfig &config ) override;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
