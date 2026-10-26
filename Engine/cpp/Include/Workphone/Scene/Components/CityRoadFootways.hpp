#pragma once
#include <Workphone/Scene/Components/CityRoadTerrainMesh.hpp>

namespace workphone::scene
{
    /** Derived pedestrian geometry in the canonical road's world-metre frame.
     * Nominal 2.4 m strips have a 0.05 m carriageway gap. Every accepted piece
     * covers retained terrain cell triangles exactly, with its top at terrain
     * +0.15 m and perimeter skirts down to terrain. Unsupported pieces are
     * omitted; malformed inputs, allocation and work limits retain output.
     * All surfaces use Kerb kind and are shared by render/mesh collision.
     */
    WPCore_API bool buildCityRoadFootways( const procedural::CityRoadSplineSource &source,
                                           const procedural::CityRoadSplineBake &bake,
                                           render::TerrainSnapshot terrain,
                                           const Transform3<real_Num> &placement,
                                           CityRoadTerrainMesh &output, String &error );
}  // namespace workphone::scene
