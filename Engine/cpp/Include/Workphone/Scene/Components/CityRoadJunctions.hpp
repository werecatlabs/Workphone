#pragma once
#include <Workphone/Interface/Procedural/CityRoadSplineSource.hpp>

namespace workphone::scene
{
    struct CityRoadJunctionOptions
    {
        f32 cornerRadius = 4.f;
        u32 cornerSegments = 6;
        f32 maximumTrim = 24.f;
    };

    struct CityRoadJunctionRun
    {
        u32 roadIndex = 0;
        // Original source arc distances are retained for markings/texture phase.
        SourceBudgetVector<procedural::CityRoadSplineSample> points;
    };

    struct CityRoadJunctionPort
    {
        u32 roadIndex = 0;
        f32 station = 0;
        bool forward = true;
        procedural::CityRoadSplineSample boundary;
    };

    struct CityRoadJunctionCorner
    {
        // Includes both approach-edge endpoints, in boundary order.
        SourceBudgetVector<Vector3F> points;
        // Horizontal offset vectors, including miters at discretized corners.
        // Inner/outer footways use precisely the same joined boundary.
        SourceBudgetVector<Vector3F> outward;
    };

    struct CityRoadJunction
    {
        Vector3F center{};
        f32 heightOffset = 0, maxGrade = .08f;
        SourceBudgetVector<CityRoadJunctionPort> ports;
        SourceBudgetVector<Vector3F> boundary;
        // Triangles reference vertices: center at index 0, then boundary.
        SourceBudgetVector<u32> indices;
        SourceBudgetVector<CityRoadJunctionCorner> corners;
    };

    struct CityRoadJunctionBake : SourceBudgetObject
    {
        u64 sourceRevision = 0;
        SourceBudgetVector<CityRoadJunctionRun> roadRuns;
        SourceBudgetVector<CityRoadJunction> junctions;
        WPCore_API void swap( CityRoadJunctionBake &other ) noexcept;
    };

    // Derived, bounded T/X connectors for supported draped roads. Canonical
    // splines are never edited. Trimmed approaches and rounded unequal-width
    // footprints are shared by asphalt, collision, footways and markings.
    // Full cell/hole/grade coverage remains the terrain mesh builder's job.
    WPCore_API bool buildCityRoadJunctions( const procedural::CityRoadSplineSource &source,
                                            const procedural::CityRoadSplineBake &bake,
                                            const procedural::CityRoadHeightSampler &terrain,
                                            const CityRoadJunctionOptions &options,
                                            CityRoadJunctionBake &output, String &error );
}  // namespace workphone::scene
