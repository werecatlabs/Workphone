#pragma once
#include <Workphone/Interface/Procedural/CityRoadSplineSource.hpp>
#include <Workphone/Graphics/TerrainData.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone::scene
{
    enum class CityRoadTerrainTriangleKind : u32
    {
        Road,
        Island,
        Kerb,
        Marking
    };

    struct CityRoadTerrainVertex
    {
        Vector3F position{}, normal{};
        Vector2F uv{};
    };

    struct CityRoadTerrainMesh : SourceBudgetObject
    {
        SourceBudgetVector<CityRoadTerrainVertex> vertices;
        SourceBudgetVector<u32> indices;
        SourceBudgetVector<CityRoadTerrainTriangleKind> triangleKinds;
        u64 sourceRevision = 0, terrainRevision = 0;
        WPCore_API void swap( CityRoadTerrainMesh &other ) noexcept;
    };

    /** World-metre source frame. Drape surfaces are clipped to the actual retained
     * terrain cell triangles, including holes. Fixed/roundabout surfaces retain
     * their solved profile only when full-footprint support/deviation checks pass.
     * Transactional, bounded output shared by rendering and mesh collision.
     */
    WPCore_API bool buildCityRoadTerrainMesh( const procedural::CityRoadSplineSource &source,
                                              const procedural::CityRoadSplineBake &bake,
                                              render::TerrainSnapshot terrain,
                                              const Transform3<real_Num> &placement,
                                              CityRoadTerrainMesh &output, String &error );
}  // namespace workphone::scene
