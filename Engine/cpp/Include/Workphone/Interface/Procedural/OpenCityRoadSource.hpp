#pragma once
#include <Workphone/Interface/Procedural/CityRoadSplineSource.hpp>

namespace workphone::procedural
{
    struct OpenCityLayout;

    struct OpenCityRoadSourceOptions
    {
        SourceBudgetText sourceId, terrainResourceUuid;
        u32 seed = 0;
        u64 revision = 1, terrainRevision = 0;
        // Uniform grid width; maximum (arterial) width for natural-road profiles.
        f32 roadWidth = 12, maxGrade = .08f, heightOffset = .025f;
        u32 maximumRoundabouts = 8;
        bool clockwise = true;
        // Natural-road cubic fitting follows terrain inside the existing graph
        // corridors. Disable to reproduce the earlier straight source geometry.
        bool terrainCurves = true;
    };

    struct OpenCityRoadSourceResult : SourceBudgetObject
    {
        CityRoadSplineSource source;
        SourceBudgetVector<Vector3F> route;
        u32 inputNodes = 0, inputEdges = 0, mergedChains = 0;
        WPCore_API void swap( OpenCityRoadSourceResult &other ) noexcept;
    };

    // Converts the existing graph without dropping branches. Unique opposite
    // collinear arms coalesce through ordinary crossings to respect v2's 128-road
    // limit. Roundabouts retain real port/route topology; ordinary crossings are
    // geometric contacts pending the general authored-junction solver.
    // Stable custom UUIDs use namespace/seed/signed millimetre geometry, with
    // collision validation. Terrain callback/world coordinates are world metres.
    // Transactional output; unsupported topology, fit or admission fails explicitly.
    WPCore_API bool buildOpenCityRoadSource( const OpenCityLayout &city,
                                             const OpenCityRoadSourceOptions &options,
                                             const CityRoadHeightSampler &terrain,
                                             OpenCityRoadSourceResult &output, String &error );
    // Follows monotonic edited/generated curves on their pinned chord stations,
    // then proves continuous running-surface coverage. Junction endpoints remain
    // pinned; missing roads, folded/ambiguous mappings and gaps are rejected.
    WPCore_API bool fitOpenCityRoadRoute( const OpenCityLayout &original,
                                          const CityRoadSplineSource &current,
                                          const CityRoadHeightSampler &terrain,
                                          SourceBudgetVector<Vector3F> &output, String &error );
}  // namespace workphone::procedural
