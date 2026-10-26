#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include <Workphone/System/SourceBudgetVector.hpp>
#include <functional>

namespace workphone::procedural
{
    enum class CityRoadRoundaboutSurface : u32
    {
        Circulation,
        Approach,
        Island,
        Kerb
    };

    struct CityRoadRoundaboutPort
    {
        SourceBudgetText approachId;  // Stable nonzero UUID, independent of graph node indices.
        Vector3F outward{};           // Horizontal direction away from the junction centre.
        f32 width = 6, length = 18;   // Two-way approach width; starts on the trimmed ring chord.
    };

    struct CityRoadRoundaboutSpec : SourceBudgetObject
    {
        SourceBudgetText junctionId;
        Vector3F center{};
        // Polygonal inner boundary conservatively circumscribes islandRadius;
        // road triangles never intrude into that exact circle.
        f32 outerRadius = 14, islandRadius = 8, islandHeight = .18f, kerbWidth = .25f;
        bool clockwise = true;
        u32 ringSamples = 96;  // Total ring sample cap, including exact port chord endpoints: 32..256.
        f32 gradeX = 0, gradeZ = 0, maxGrade = .08f, maxTerrainDeviation = .35f;
        f32 terrainSampleStep = 2;
        u32 maximumTerrainSamples = 65536;
        SourceBudgetVector<CityRoadRoundaboutPort> ports;  // 3..6 disjoint approaches.
    };

    struct CityRoadRoundaboutVertex
    {
        Vector3F position{}, normal{};
        f32 u = 0, v = 0;
    };

    struct CityRoadRoundaboutConnection
    {
        SourceBudgetText approachId;
        SourceBudgetVector<Vector3F> entry, exit;  // Directed outer-end-to-ring / ring-to-outer-end.
    };

    struct CityRoadRoundaboutRoute
    {
        SourceBudgetText entryApproachId, exitApproachId;
        SourceBudgetVector<Vector3F> points;
    };

    struct CityRoadRoundaboutBake : SourceBudgetObject
    {
        SourceBudgetText junctionId;
        Vector3F center{};
        f32 outerRadius = 0, islandRadius = 0;
        // All outputs share one candidate. Triangle kind keeps drivable and
        // raised island/kerb geometry separate while retaining exact mesh collision.
        SourceBudgetVector<CityRoadRoundaboutVertex> vertices;
        SourceBudgetVector<u32> indices;
        SourceBudgetVector<CityRoadRoundaboutSurface> triangleKinds;
        SourceBudgetVector<CityRoadRoundaboutConnection> connections;
        SourceBudgetVector<CityRoadRoundaboutRoute> routes;
        CityRoadRoundaboutBake() = default;
        CityRoadRoundaboutBake( const CityRoadRoundaboutBake & ) = default;
        CityRoadRoundaboutBake( CityRoadRoundaboutBake && ) noexcept = default;
        CityRoadRoundaboutBake &operator=( CityRoadRoundaboutBake && ) noexcept = default;

        CityRoadRoundaboutBake &operator=( const CityRoadRoundaboutBake &other )
        {
            if( this != &other )
            {
                CityRoadRoundaboutBake copy( other );
                swap( copy );
            }
            return *this;
        }

        WPCore_API void swap( CityRoadRoundaboutBake &other ) noexcept;
        // Exact XZ triangle-union query over drivable kinds; rejects island/kerb.
        WPCore_API bool containsSurface( f32 x, f32 z ) const;
    };

    using CityRoadRoundaboutHeightSampler = std::function<bool( const Vector3F &, f32 & )>;
    // Single circulating lane (3..7m), UUID references, planar road/approach
    // contacts, bounded dense retained-terrain queries, transactional output.
    // Point sampling cannot prove terrain/hole coverage between accepted samples;
    // the runtime terrain adapter must additionally validate covered terrain cells.
    WPCore_API bool bakeCityRoadRoundabout( const CityRoadRoundaboutSpec &,
                                            const CityRoadRoundaboutHeightSampler &,
                                            CityRoadRoundaboutBake &, String &error );
}  // namespace workphone::procedural
