#pragma once
#include <Workphone/Scene/Components/CityRoadTerrainMesh.hpp>

namespace workphone::scene
{
    struct CityRoadTerrainFitOptions
    {
        // World metres. Full cell support and a footway apron are added automatically.
        f32 shoulderWidth = 16.f, smoothingRadius = 24.f, maximumEarthwork = 12.f;
        u32 maximumTerrainSamples = 262144;
        procedural::CityRoadSplineSampling sampling;
        bool connectRoads = true;
    };

    struct CityRoadTerrainFitReport
    {
        u32 changedSamples = 0, attempts = 0;
        f32 maxCut = 0, maxFill = 0, maxRoadGrade = 0, roadHeightSpan = 0;
        // Bounded preparation diagnostics survive a later successful candidate.
        SourceBudgetText rejectedAttempts;
    };

    struct CityRoadTerrainFitResult : SourceBudgetObject
    {
        procedural::CityRoadSplineSource source;
        render::TerrainSnapshot terrain;
        procedural::CityRoadSplineBake bake;
        CityRoadTerrainFitReport report;
        WPCore_API void swap( CityRoadTerrainFitResult &other ) noexcept;
    };

    /** Prepare a bounded cut/fill candidate without publishing scene or terrain edits.
     * Editable XZ controls, IDs, materials, holes and samples outside the feathered
     * corridor are preserved. Drape roads receive smoothed longitudinal profiles;
     * Fixed roads retain their authored profile. Roundabouts receive local grade
     * planes with cell-wide support, including approach caps. Every successful
     * result passes the ordinary sampler and complete terrain-road mesh builder.
     * Revisions are retained; the authoring transaction assigns publication tokens.
     * Failure (including SourceCpu denial) leaves output and both inputs unchanged.
     */
    WPCore_API bool prepareCityRoadTerrainFit(
        const procedural::CityRoadSplineSource &source, render::TerrainSnapshot terrain,
        const Transform3<real_Num> &placement, const CityRoadTerrainFitOptions &options,
        CityRoadTerrainFitResult &output, String &error );
}
