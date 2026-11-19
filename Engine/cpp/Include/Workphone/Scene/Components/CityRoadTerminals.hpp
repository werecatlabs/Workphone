#pragma once

#include <Workphone/Scene/Components/CityRoadJunctions.hpp>
#include <Workphone/Scene/Components/CityRoadTerrainMesh.hpp>

namespace workphone::scene
{
    struct CityRoadTerminal
    {
        u32 roadIndex = 0;
        bool atStart = false;
        Vector3F center{};
        SourceBudgetVector<Vector3F> edge, outward;
    };

    // Resolve only free tips whose complete cap fits the existing terrain. The
    // asphalt, paint and pedestrian return share the resulting boundary.
    SourceBudgetVector<CityRoadTerminal> buildCityRoadTerminals(
        const procedural::CityRoadSplineSource &source, const procedural::CityRoadSplineBake &bake,
        const CityRoadJunctionBake &connections, const render::TerrainData &terrain,
        const Transform3<real_Num> &placement, CityRoadTerrainMesh *asphalt = nullptr );
}  // namespace workphone::scene
