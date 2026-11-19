#pragma once
#include <Workphone/Scene/Components/CityRoadTerrainMesh.hpp>
#include <string_view>

namespace workphone::scene
{
    struct CityRoadTerrainGradeData;
    using CityRoadTerrainGrading = std::shared_ptr<const CityRoadTerrainGradeData>;

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
     * corridor are preserved. Drape roads receive smoothed longitudinal profiles
     * and a bounded terrain-supported crown which fades at edges and open ends;
     * Fixed roads retain their authored profile. Roundabouts receive local grade
     * planes with cell-wide support, including approach caps. Every successful
     * result passes the ordinary sampler and complete terrain-road mesh builder.
     * Revisions are retained; the authoring transaction assigns publication tokens.
     * Failure (including SourceCpu denial) leaves output and both inputs unchanged.
     */
    WPCore_API bool prepareCityRoadTerrainFit( const procedural::CityRoadSplineSource &source,
                                               render::TerrainSnapshot terrain,
                                               const Transform3<real_Num> &placement,
                                               const CityRoadTerrainFitOptions &options,
                                               CityRoadTerrainFitResult &output, String &error );

    // Grade an edit only where the draft changes road geometry or a junction.
    // New or moved road footprints and width changes refresh the terrain-supported
    // crown even when an uncrowned mesh would already be supported. Compatible
    // edits such as a grade-limit change can retain the exact terrain snapshot.
    // Unchanged roundabouts and unrelated carriageways retain their support.
    WPCore_API bool prepareCityRoadTerrainEdit( const procedural::CityRoadSplineSource &before,
                                                const procedural::CityRoadSplineSource &draft,
                                                render::TerrainSnapshot terrain,
                                                const Transform3<real_Num> &placement,
                                                const CityRoadTerrainFitOptions &options,
                                                CityRoadTerrainFitResult &output, String &error );

    // Restore recorded samples that are no longer covered by the draft and
    // still match the last road-applied height. External edits take priority.
    WPCore_API bool prepareCityRoadTerrainRestoration( const procedural::CityRoadSplineSource &draft,
                                                       render::TerrainSnapshot active,
                                                       const Transform3<real_Num> &placement,
                                                       const CityRoadTerrainGrading &grading,
                                                       render::TerrainSnapshot &output, String &error );

    // Build the next sparse record set from the exact active and prepared grids.
    // Staging is read-only and leaves all output unchanged if admission fails.
    WPCore_API bool retainCityRoadTerrainGrading( std::string_view terrainUuid,
                                                  render::TerrainSnapshot active,
                                                  render::TerrainSnapshot prepared,
                                                  const Transform3<real_Num> &placement,
                                                  const CityRoadTerrainGrading &previous,
                                                  CityRoadTerrainGrading &output, String &error );

    WPCore_API bool serializeCityRoadTerrainGrading( const CityRoadTerrainGrading &grading,
                                                     SourceBudgetText &output, String &error );
    WPCore_API bool deserializeCityRoadTerrainGrading( std::string_view value,
                                                       std::string_view terrainUuid,
                                                       CityRoadTerrainGrading &output, String &error );
}  // namespace workphone::scene
