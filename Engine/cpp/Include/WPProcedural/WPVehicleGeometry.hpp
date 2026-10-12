// WPVehicleGeometry.hpp - deterministic production vehicle mesh generation.
#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Interface/Procedural/VehicleGeometryTypes.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API WPVehicleGeometry
        {
        public:
            static VehicleGeometry generate( const VehicleGeometryConfig &config = {} );
        };
    }  // namespace procedural
}  // namespace workphone
