#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include <Workphone/System/SourceBudgetVector.hpp>
#include <Workphone/Interface/Procedural/CityRoadRoundabout.hpp>
#include <functional>
#include <string_view>

namespace workphone::procedural
{
    enum class CityRoadHeightMode : u32 { Drape, Fixed, Bridge };
    struct CityRoadSplineKnot
    {
        SourceBudgetText id;
        Vector3F position{}, inHandle{}, outHandle{}; // Handles are relative vectors.
    };
    struct CityRoadSpline
    {
        SourceBudgetText id;
        f32 width = 12, maxGrade = .08f, heightOffset = 0;
        bool closed = false;
        CityRoadHeightMode heightMode = CityRoadHeightMode::Drape;
        SourceBudgetVector<CityRoadSplineKnot> knots;
    };
    struct CityRoadSplineSource : SourceBudgetObject
    {
        u32 version = 1;
        SourceBudgetText id, terrainResourceUuid;
        u64 revision = 1, terrainRevision = 0;
        SourceBudgetVector<CityRoadSpline> roads;
        SourceBudgetVector<CityRoadRoundaboutSpec> roundabouts; // Version 2, ports reference road GUIDs.
        CityRoadSplineSource() = default;
        CityRoadSplineSource( const CityRoadSplineSource & ) = default;
        CityRoadSplineSource( CityRoadSplineSource && ) noexcept = default;
        CityRoadSplineSource &operator=( CityRoadSplineSource && ) noexcept = default;
        CityRoadSplineSource &operator=( const CityRoadSplineSource &other )
        { if( this != &other ) { CityRoadSplineSource copy( other ); swap( copy ); } return *this; }
        void swap( CityRoadSplineSource &other ) noexcept
        {
            using std::swap; swap( version, other.version ); swap( revision, other.revision );
            swap( terrainRevision, other.terrainRevision ); id.swap( other.id );
            terrainResourceUuid.swap( other.terrainResourceUuid ); roads.swap( other.roads );
            roundabouts.swap( other.roundabouts );
        }
    };
    struct CityRoadSplineSample
    {
        Vector3F position{}, tangent{}, left{}, right{};
        f32 distance = 0;
    };
    struct CityRoadSplineSamples
    {
        SourceBudgetText roadId;
        SourceBudgetVector<CityRoadSplineSample> points;
    };
    struct CityRoadSplineBake : SourceBudgetObject
    {
        u64 sourceRevision = 0;
        SourceBudgetVector<CityRoadSplineSamples> roads;
    };
    struct CityRoadSplineSampling
    {
        // Bounds the authored Bezier before terrain fit; not terrain cell/hole coverage.
        f32 tolerance = .05f, maxStep = 2.f;
        u32 maximumSamples = 65536;
    };
    // Inputs/returned Y share the source coordinate frame. The adapter must retain one
    // immutable terrain snapshot/placement for the whole operation.
    // False means outside/hole/unavailable; no Y=0 fallback is permitted.
    using CityRoadHeightSampler = std::function<bool( const Vector3F &, f32 & )>;
    WPCore_API bool validateCityRoadSplineSource( const CityRoadSplineSource &, String &error );
    WPCore_API bool serializeCityRoadSplineSource( const CityRoadSplineSource &, SourceBudgetText &json, String &error );
    WPCore_API bool deserializeCityRoadSplineSource( std::string_view json, CityRoadSplineSource &, String &error );
    // Transactional outputs; no source edit, terrain carving, mesh/collision or scene publication.
    // Bridge is explicitly unsupported. maxGrade bounds fitted half-crossfalls and road edges.
    // Reversed local offset edges fail; this does not solve global road/junction intersections.
    // Point queries cannot prove continuous terrain/hole coverage between accepted samples.
    WPCore_API bool sampleCityRoadSplines( const CityRoadSplineSource &, const CityRoadSplineSampling &,
                                         const CityRoadHeightSampler &, CityRoadSplineBake &, String &error );
}
