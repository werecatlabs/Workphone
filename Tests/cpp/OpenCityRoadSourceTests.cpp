#include <Workphone/Interface/Procedural/OpenCityRoadSource.hpp>
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <Workphone/Interface/Procedural/OpenCityLegacyRoute.hpp>
#include <Workphone/Scene/Components/CityRoadTerrainMesh.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>

using namespace workphone;
using namespace workphone::procedural;
namespace
{
    void need( bool value, const char *message ) { if( !value ) throw std::runtime_error( message ); }
    void check( bool value, const String &error ) { if( !value ) throw std::runtime_error( error.c_str() ); }
    const auto plane = []( const Vector3F &p, float &height ) { height = 2 + .005f * p.x + .003f * p.z; return true; };
    OpenCityRoadSourceOptions options( u32 seed )
    {
        OpenCityRoadSourceOptions result; result.seed = seed; result.revision = 17; result.terrainRevision = 77;
        result.sourceId = "10000000-0000-4000-8000-000000000001";
        result.terrainResourceUuid = "20000000-0000-4000-8000-000000000001"; return result;
    }
    SourceBudgetText encoded( const CityRoadSplineSource &source )
    { SourceBudgetText result; String error; check( serializeCityRoadSplineSource( source, result, error ), error ); return result; }
    void sameRoute( const SourceBudgetVector<Vector3F> &a, const SourceBudgetVector<Vector3F> &b )
    {
        need( a.size() == b.size(), "Identical source changed route sample count" );
        for( size_t i = 0; i < a.size(); ++i ) need( a[i] == b[i], "Identical source changed fitted route coordinates" );
    }
    void uniqueIds( const CityRoadSplineSource &source )
    {
        SourceBudgetSet<SourceBudgetText> ids; need( ids.insert( source.id ).second, "Source GUID is duplicated" );
        for( const auto &road : source.roads )
        {
            need( ids.insert( road.id ).second, "Generated road GUID is duplicated" );
            for( const auto &knot : road.knots )
                need( ids.insert( knot.id ).second, "Generated controls must have unique owning-road GUIDs" );
        }
        for( const auto &junction : source.roundabouts )
            need( ids.insert( junction.junctionId ).second, "Generated junction GUID is duplicated" );
    }
    void routeContracts( const OpenCityRoadSourceResult &result )
    {
        need( result.route.size() >= 4 && result.route.front().x == 0 && result.route.front().z == 0 &&
              std::abs( result.route.front().y - 2.025f ) < .00001f && result.route[1].z < 0 &&
              ( result.route.front() - result.route.back() ).length() < .001f,
              "Adapted race route lost origin, -Z heading, terrain height or closure" );
        for( size_t i = 1; i < result.route.size(); ++i )
        {
            const auto a = result.route[i - 1], b = result.route[i];
            need( std::isfinite( b.y ) && std::hypot( b.x - a.x, b.z - a.z ) <= 1.501f, "Route has a nonfinite height or discontinuous jump" );
            for( const auto &junction : result.source.roundabouts )
            {
                const float dx = b.x - a.x, dz = b.z - a.z, length = dx * dx + dz * dz;
                const float t = length > 0 ? std::clamp( ( ( junction.center.x - a.x ) * dx +
                    ( junction.center.z - a.z ) * dz ) / length, 0.f, 1.f ) : 0;
                need( std::hypot( a.x + t * dx - junction.center.x, a.z + t * dz - junction.center.z ) >= junction.islandRadius - .001f,
                      "A race-route segment cuts across a roundabout island" );
            }
        }
    }
    unsigned approachContracts( const CityRoadSplineSource &source )
    {
        unsigned longApproaches = 0;
        for( const auto &junction : source.roundabouts ) for( const auto &port : junction.ports )
        {
            need( port.length >= 4 && port.length <= 18, "Generated approach escaped its 4..18 m bound" );
            if( port.length >= 12 ) ++longApproaches;
            const auto found = std::find_if( source.roads.begin(), source.roads.end(),
                [&]( const auto &road ) { return road.id == port.approachId; } );
            need( found != source.roads.end(), "Generated approach lost its source-road identity" );
            need( std::abs( port.width - found->width ) < .00001f,
                  "Generated roundabout approach does not match its road's width" );
            const float direction = std::hypot( port.outward.x, port.outward.z );
            const Vector3F expected = junction.center + port.outward * ( ( junction.outerRadius + port.length ) / direction );
            const auto a = found->knots.front().position, b = found->knots.back().position;
            const auto xz = []( Vector3F p, Vector3F q ) { return std::hypot( p.x - q.x, p.z - q.z ); };
            need( std::min( xz( a, expected ), xz( b, expected ) ) <= .001f,
                  "Generated approach and source road use different endpoint trims" );
            need( xz( a, b ) >= 4, "Longer approaches consumed the remaining source road" );
            for( const auto &neighbor : source.roundabouts ) if( neighbor.junctionId != junction.junctionId )
                for( const auto &other : neighbor.ports ) if( other.approachId == port.approachId )
                {
                    const float firstRoom = std::hypot( junction.center.x, junction.center.z ) - junction.outerRadius - 4 - port.width * .5f;
                    const float secondRoom = std::hypot( neighbor.center.x, neighbor.center.z ) - neighbor.outerRadius - 4 - other.width * .5f;
                    if( firstRoom >= 18 && secondRoom >= 18 )
                        need( std::abs( port.length - other.length ) <= .001f,
                              "Neighbouring roundabouts did not share a constrained road symmetrically" );
                }
        }
        return longApproaches;
    }
    void generated( bool natural, int blocks, u32 seed, bool meshCheck )
    {
        const auto city = OpenCityLayout::generate( seed, blocks, 0, natural );
        auto policy = options( seed ); if( !meshCheck ) policy.maximumRoundabouts = 2;
        OpenCityRoadSourceResult converted; String error;
        check( buildOpenCityRoadSource( city, policy, plane, converted, error ), error );
        uniqueIds( converted.source );
        const auto longApproaches = approachContracts( converted.source );
        for( const auto &road : converted.source.roads )
        {
            if( natural )
                need( std::any_of( city.roads.edges.begin(), city.roads.edges.end(),
                    [&]( const auto &edge ) {
                        return road.width == OpenCityRoadNetwork::roadWidth( edge.roadClass );
                    } ), "Natural source road lost its generated hierarchy width" );
            else need( road.width == policy.roadWidth, "Road hierarchy changed the uniform grid width" );
        }
        if( natural ) for( const auto &edge : city.roads.edges )
            need( std::any_of( converted.source.roads.begin(), converted.source.roads.end(),
                [&]( const auto &road ) { return road.width == OpenCityRoadNetwork::roadWidth( edge.roadClass ); } ),
                "Source conversion discarded one class of natural street" );
        need( converted.source.version == 2 && converted.source.revision == 17 && !converted.source.roundabouts.empty() &&
              converted.source.roundabouts.size() <= policy.maximumRoundabouts && converted.source.roads.size() <= 128,
              "Generated source lost a real junction or exceeded the source codec budget" );
        routeContracts( converted );
        SourceBudgetVector<Vector3F> reused;
        check( fitOpenCityRoadRoute( city, converted.source, plane, reused, error ), error ); sameRoute( converted.route, reused );
        if( meshCheck )
        {
            const auto alternate = OpenCityLayout::generate( seed, blocks, 2, natural ); OpenCityRoadSourceResult other;
            check( buildOpenCityRoadSource( alternate, policy, plane, other, error ), error );
            need( encoded( converted.source ) == encoded( other.source ), "Route choice reshuffled generated source GUIDs, roads or junctions" );
            routeContracts( other );
            auto terrain = std::make_shared<render::TerrainData>(); terrain->dimensions = { 321, 321 };
            terrain->origin = { -640, -640 }; terrain->spacing = { 4, 4 }; terrain->revision = 77;
            terrain->heights.resize( 321 * 321 );
            for( int z = 0; z < 321; ++z ) for( int x = 0; x < 321; ++x )
                plane( { -640.f + x * 4, 0, -640.f + z * 4 }, terrain->heights[size_t( z ) * 321 + x] );
            CityRoadSplineBake sampled; check( sampleCityRoadSplines( converted.source, {}, plane, sampled, error ), error );
            scene::CityRoadTerrainMesh mesh;
            check( scene::buildCityRoadTerrainMesh( converted.source, sampled, terrain, {}, mesh, error ), error );
            need( !mesh.indices.empty(), "Generated editable source produced no shared terrain mesh/collision" );
            std::cout << "base mesh " << ( natural ? "natural" : "grid" ) << " blocks=" << blocks <<
                " junctions=" << converted.source.roundabouts.size() << " vertices=" << mesh.vertices.size() <<
                " triangles=" << mesh.triangleKinds.size() << '\n';
            need( longApproaches > 0, "Representative generated roundabouts have no approach long enough for splitter refuges" );
            bool refuge = false;
            for( const auto &junction : converted.source.roundabouts )
            {
                CityRoadRoundaboutBake baked; check( bakeCityRoadRoundabout( junction, plane, baked, error ), error );
                for( size_t triangle = 0; triangle < baked.triangleKinds.size(); ++triangle )
                    if( baked.triangleKinds[triangle] == CityRoadRoundaboutSurface::Island )
                        for( unsigned corner = 0; corner < 3; ++corner )
                        {
                            const auto p = baked.vertices[baked.indices[triangle * 3 + corner]].position;
                            refuge = refuge || std::hypot( p.x - junction.center.x, p.z - junction.center.z ) > junction.outerRadius;
                        }
            }
            need( refuge, "Long approaches did not produce actual raised splitter refuge geometry" );
            if( !natural && blocks == 6 && seed == 42 )
                for( const int diagnosticBlocks : { 6, 10 } )
                {
                    auto diagnosticPolicy = options( seed ); diagnosticPolicy.maximumRoundabouts = 4;
                    OpenCityRoadSourceResult diagnostic;
                    check( buildOpenCityRoadSource( OpenCityLayout::generate( seed, diagnosticBlocks, 0, false ),
                        diagnosticPolicy, plane, diagnostic, error ), error );
                    CityRoadSplineBake diagnosticBake;
                    check( sampleCityRoadSplines( diagnostic.source, {}, plane, diagnosticBake, error ), error );
                    scene::CityRoadTerrainMesh diagnosticMesh;
                    check( scene::buildCityRoadTerrainMesh( diagnostic.source, diagnosticBake, terrain, {}, diagnosticMesh, error ), error );
                    std::cout << "base mesh diagnostic grid blocks=" << diagnosticBlocks << " junctions=4 vertices=" <<
                        diagnosticMesh.vertices.size() << " triangles=" << diagnosticMesh.triangleKinds.size() << '\n';
                    need( diagnosticMesh.vertices.size() < 350000, "Generated city base mesh exceeds its coalesced geometry budget" );
                }
            const auto saved = reused;
            auto missing = converted.source; missing.roads.clear(); missing.roundabouts.clear();
            need( !fitOpenCityRoadRoute( city, missing, plane, reused, error ), "Missing pinned route silently acquired a fallback" );
            sameRoute( saved, reused );
        }
        std::cout << ( natural ? "natural" : "grid" ) << " seed=" << seed << " blocks=" << blocks <<
            " roads=" << converted.source.roads.size() << " junctions=" << converted.source.roundabouts.size() <<
            " longApproaches=" << longApproaches << " route=" << converted.route.size() << " merged=" << converted.mergedChains << '\n';
    }
    void naturalJunctionCorpus()
    {
        // Exercise the four-roundabout runtime policy on less familiar seeds,
        // including every route, without spending this topology test on parcels.
        for( const u32 seed : { 1u, 3u, 19u, 137u, 7919u, 0x9e3779b9u, 0x80000000u, 0xfffffffeu } )
            for( const int blocks : { 6, 8, 10 } )
            {
                SourceBudgetText firstSource;
                for( int route = 0; route < 3; ++route )
                {
                    OpenCityLayout city;
                    city.naturalRoads = true;
                    city.roads = OpenCityRoadNetwork::generate( seed, blocks, route );
                    city.blocks = city.roads.blocks;
                    city.spacing = city.roads.spacing;
                    city.extent = city.roads.extent;
                    for( const auto &point : city.roads.route ) city.route.push_back( { point.x, point.z } );
                    auto policy = options( seed ); policy.maximumRoundabouts = 4;
                    OpenCityRoadSourceResult result; String error;
                    check( buildOpenCityRoadSource( city, policy, plane, result, error ), error );
                    uniqueIds( result.source );
                    approachContracts( result.source );
                    routeContracts( result );
                    need( !result.source.roundabouts.empty() && result.source.roads.size() <= 128,
                          "Natural districts lost roundabouts or exceeded editable source capacity" );
                    const auto serialized = encoded( result.source );
                    if( route == 0 ) firstSource = serialized;
                    else need( firstSource == serialized, "Route selection changed natural editable roads" );
                }
            }
        std::cout << "Natural editable-source corpus: 72 seed/size/route conversions PASS\n";
    }
    void hierarchicalWidths()
    {
        // A main approach crosses the core, while a collector feeds a smaller
        // local street. The collinear class boundary at (60,-80) must remain
        // independently editable even when the width policy clamps both to 4 m.
        using RoadClass = OpenCityRoadNetwork::RoadClass;
        OpenCityLayout city;
        city.naturalRoads = true;
        city.roads.nodes = { { 0, 0 }, { 0, -80 }, { 0, -180 }, { 60, -80 },
                             { 120, -80 }, { -100, -80 }, { 120, 40 }, { 0, 40 } };
        city.roads.edges = { { 0, 1, RoadClass::Arterial }, { 1, 2, RoadClass::Arterial },
                             { 1, 3, RoadClass::Collector }, { 3, 4, RoadClass::Local },
                             { 1, 5, RoadClass::Local }, { 4, 6, RoadClass::Local },
                             { 6, 7, RoadClass::Collector }, { 7, 0, RoadClass::Arterial } };
        city.route = { { 0, 0 }, { 0, -80 }, { 60, -80 }, { 120, -80 },
                       { 120, 40 }, { 0, 40 } };
        for( const float maximumWidth : { 12.f, 6.f, 4.f } )
        {
            auto policy = options( 42 ); policy.maximumRoundabouts = 1; policy.roadWidth = maximumWidth;
            OpenCityRoadSourceResult result; String error;
            check( buildOpenCityRoadSource( city, policy, plane, result, error ), error );
            routeContracts( result ); approachContracts( result.source ); uniqueIds( result.source );
            need( result.source.roundabouts.size() == 1 && result.source.roads.size() == 7 &&
                      result.mergedChains == 1,
                  "Different road classes merged across an editable width boundary" );
            const auto roadWidths = { maximumWidth, std::max( 4.f, maximumWidth * .75f ),
                                      std::max( 4.f, maximumWidth * ( 7.f / 12.f ) ) };
            for( const auto width : roadWidths )
                need( std::any_of( result.source.roads.begin(), result.source.roads.end(),
                    [width]( const auto &road ) { return std::abs( road.width - width ) < .00001f; } ),
                    "Road-class widths were lost or ignored the configured maximum" );
            for( const auto &road : result.source.roads )
                need( std::any_of( roadWidths.begin(), roadWidths.end(),
                    [&]( float width ) { return std::abs( road.width - width ) < .00001f; } ),
                    "A road escaped its width policy" );
            const auto boundary = std::count_if( result.source.roads.begin(), result.source.roads.end(),
                []( const auto &road ) {
                    const auto atBoundary = []( const Vector3F &point ) {
                        return std::abs( point.x - 60 ) < .0001f && std::abs( point.z + 80 ) < .0001f;
                    };
                    return atBoundary( road.knots.front().position ) || atBoundary( road.knots.back().position );
                } );
            need( boundary == 2, "Collector-to-local transition lost its shared editable endpoint" );
            if( maximumWidth == 12 )
            {
                const auto editable = std::find_if( result.source.roads.begin(), result.source.roads.end(),
                    []( const auto &road ) {
                        return std::abs( road.knots.front().position.x - 60 ) < .001f &&
                               std::abs( road.knots.back().position.x - 120 ) < .001f;
                    } );
                need( editable != result.source.roads.end(), "Width-edit fixture lost its local street" );
                auto &edited = *editable;
                edited.width = 8.25f;
                const auto editedId = edited.id;
                ++result.source.revision;
                const auto json = encoded( result.source );
                CityRoadSplineSource reopened;
                check( deserializeCityRoadSplineSource( std::string_view( json.data(), json.size() ), reopened, error ), error );
                need( encoded( reopened ) == json && std::any_of( reopened.roads.begin(), reopened.roads.end(),
                    [&]( const auto &road ) { return road.id == editedId && road.width == 8.25f; } ),
                    "Saving hierarchical roads lost a hand-edited width or identity" );
            }
        }
    }
    void failures()
    {
        auto city = OpenCityLayout::generate( 42, 6, 0, false ); auto policy = options( 42 ); policy.maximumRoundabouts = 2;
        OpenCityRoadSourceResult output; String error; check( buildOpenCityRoadSource( city, policy, plane, output, error ), error );
        const auto source = encoded( output.source ); const auto route = output.route;
        const auto reject = [&]( const OpenCityLayout &candidate, const OpenCityRoadSourceOptions &settings,
                                 const CityRoadHeightSampler &height ) {
            need( !buildOpenCityRoadSource( candidate, settings, height, output, error ) && !error.empty(), "Unsupported conversion was accepted" );
            need( encoded( output.source ) == source, "Failed conversion changed canonical source" ); sameRoute( route, output.route );
        };
        auto invalid = policy; invalid.maximumRoundabouts = 9; reject( city, invalid, plane );
        invalid = policy; invalid.sourceId = "invalid"; reject( city, invalid, plane );
        auto invalidClass = OpenCityLayout::generate( 42, 6, 0, true );
        invalidClass.roads.edges.front().roadClass = static_cast<OpenCityRoadNetwork::RoadClass>( 255 );
        reject( invalidClass, policy, plane );
        reject( city, policy, []( const Vector3F &, float & ) { return false; } );
        auto badRoute = city; badRoute.route[1].x = std::numeric_limits<float>::quiet_NaN(); reject( badRoute, policy, plane );
        const auto scheduler = WorldStreamingScheduler::shared(); const auto stats = scheduler->stats();
        const auto index = size_t( StreamMemoryClass::SourceCpu );
        struct Restore { std::shared_ptr<WorldStreamingScheduler> scheduler; u64 limit;
            ~Restore() { String error; scheduler->setBudget( StreamMemoryClass::SourceCpu, limit, error ); }
        } restore{ scheduler, stats.limits[index] };
        check( scheduler->setBudget( StreamMemoryClass::SourceCpu, stats.used[index], error ), error );
        const bool ok = buildOpenCityRoadSource( city, policy, plane, output, error );
        const auto used = scheduler->stats().used[index];
        check( scheduler->setBudget( StreamMemoryClass::SourceCpu, restore.limit, error ), error );
        need( !ok && used == stats.used[index] && encoded( output.source ) == source, "SourceCpu denial leaked or replaced retained output" );
        sameRoute( route, output.route );
    }
    void sharedEndpointIdentity()
    {
        const auto city = OpenCityLayout::generate( 42, 6, 0, false );
        auto policy = options( 42 ); policy.maximumRoundabouts = 0;
        OpenCityRoadSourceResult result; String error;
        check( buildOpenCityRoadSource( city, policy, plane, result, error ), error ); uniqueIds( result.source );
        unsigned shared = 0, parallel = 0;
        for( size_t i = 0; i < result.source.roads.size(); ++i ) for( size_t j = 0; j < i; ++j )
        {
            const auto &a = result.source.roads[i], &b = result.source.roads[j];
            const auto da = a.knots.back().position - a.knots.front().position;
            const auto db = b.knots.back().position - b.knots.front().position;
            if( da.x == db.x && da.z == db.z ) ++parallel;
            for( const auto &ak : a.knots ) for( const auto &bk : b.knots ) if( ak.position == bk.position )
            { ++shared; need( ak.id != bk.id, "Shared graph endpoints reused a road-owned control identity" ); }
        }
        need( shared >= 4 && parallel > 4, "Identity regression fixture lost shared endpoints or equal-span parallel streets" );
    }
    void legacyNaturalRouteCompatibility()
    {
        // Frozen revision-2 seed 42 / six-block circuit corners and sample
        // offsets: this must not silently track future generator changes.
        const std::array<OpenCityLayout::Point, 8> oldCorners = {
            OpenCityLayout::Point{ 0, 0 }, { 0, -51.31281662f },
            { 41.78147507f, -93.97827911f }, { 117.69773102f, -73.19597626f },
            { 148.42094421f, -9.39504910f }, { 103.36475372f, 69.53980255f },
            { 33.65814590f, 86.60691833f }, { 0, 41.00782394f }
        };
        const std::array<size_t, 8> cornerSamples{ 0, 26, 56, 96, 132, 178, 214, 243 };
        OpenCityLayout legacy;
        legacy.naturalRoads = true; legacy.blocks = 6; legacy.spacing = 96; legacy.extent = 288;
        legacy.route = legacyOpenCityNaturalRoute( 42, 6, 0 );
        need( legacy.route.size() == 264, "Legacy natural route changed its persisted circuit sampling" );
        need( legacyOpenCityNaturalRoute( 42, 6, 1 ).size() == 356 &&
              legacyOpenCityNaturalRoute( 42, 6, 2 ).size() == 446,
              "Legacy natural route choice changed its original circuit" );
        for( size_t i = 0; i < oldCorners.size(); ++i )
        {
            const auto expected = oldCorners[i], actual = legacy.route[cornerSamples[i]];
            need( std::hypot( expected.x - actual.x, expected.z - actual.z ) < .00002f,
                  "Legacy natural route no longer reproduces revision-2 coordinates" );
            legacy.roads.nodes.push_back( { expected.x, expected.z } );
            legacy.roads.edges.push_back( { u32( i ), u32( ( i + 1 ) % oldCorners.size() ),
                                           OpenCityRoadNetwork::RoadClass::Arterial } );
        }
        auto policy = options( 42 ); policy.maximumRoundabouts = 0;
        OpenCityRoadSourceResult saved; String error;
        check( buildOpenCityRoadSource( legacy, policy, plane, saved, error ), error );
        saved.source.roads.front().width = 11.75f; // An authored edit must survive the fallback.
        const auto unchangedSource = encoded( saved.source );
        SourceBudgetVector<Vector3F> route;
        const auto current = OpenCityLayout::generate( 42, 6, 0, true );
        need( !fitOpenCityRoadRoute( current, saved.source, plane, route, error ),
              "Migration fixture no longer distinguishes old saved roads from the new layout" );
        check( fitOpenCityRoadRoute( legacy, saved.source, plane, route, error ), error );
        need( error.empty() && !route.empty() && encoded( saved.source ) == unchangedSource,
              "Legacy route validation changed authored roads or retained a stale failure" );
        const auto retained = route;
        auto broken = saved.source; broken.roads.erase( broken.roads.begin() + 1 );
        need( !fitOpenCityRoadRoute( legacy, broken, plane, route, error ),
              "Legacy compatibility bypassed missing-road coverage validation" );
        sameRoute( retained, route );
        auto malformed = saved.source;
        malformed.roads.front().knots.front().position.x = std::numeric_limits<float>::quiet_NaN();
        need( !fitOpenCityRoadRoute( legacy, malformed, plane, route, error ),
              "Legacy compatibility accepted malformed authored geometry" );
        sameRoute( retained, route );
        need( encoded( saved.source ) == unchangedSource, "Legacy failure modified the saved canonical source" );
    }
    void editedRouteContinuity()
    {
        OpenCityLayout pinned; pinned.route = { { 0, 0 }, { 0, -10 }, { 0, 0 } };
        CityRoadSplineSource source; source.version = 2; source.id = options( 0 ).sourceId;
        source.terrainResourceUuid = options( 0 ).terrainResourceUuid;
        const char *roads[] = { "30000000-0000-4000-8000-000000000001", "30000000-0000-4000-8000-000000000002" };
        const char *knots[][2] = {
            { "40000000-0000-4000-8000-000000000001", "40000000-0000-4000-8000-000000000002" },
            { "40000000-0000-4000-8000-000000000003", "40000000-0000-4000-8000-000000000004" } };
        for( unsigned i = 0; i < 2; ++i )
        {
            const Vector3F a{ 0, 0, i ? -4.5f : 0 }, b{ 0, 0, i ? -10.f : -4.5f };
            CityRoadSpline road; road.id = roads[i]; road.width = 4; road.heightOffset = .025f;
            const auto tangent = ( b - a ) / 3.f;
            road.knots = { { knots[i][0], a, {}, tangent }, { knots[i][1], b, -tangent, {} } };
            source.roads.push_back( std::move( road ) );
        }
        SourceBudgetVector<Vector3F> retained; String error;
        check( fitOpenCityRoadRoute( pinned, source, plane, retained, error ), error ); const auto before = retained;
        auto gap = source; gap.roads[0].knots.back().position.z = -4.4f; gap.roads[1].knots.front().position.z = -4.6f;
        // This 20 cm gap lies between both the 1.5 m route stations and their
        // midpoint probes. Closed triangle-interval coverage must still reject it.
        need( !fitOpenCityRoadRoute( pinned, gap, plane, retained, error ), "A short road-cap gap escaped continuous pinned-route coverage" );
        sameRoute( before, retained );
        auto step = source; step.roads[1].heightOffset = 0;
        need( !fitOpenCityRoadRoute( pinned, step, plane, retained, error ), "A geometric road contact hid a running-surface height step" );
        sameRoute( before, retained );

        const auto scheduler = WorldStreamingScheduler::shared(); const auto stats = scheduler->stats();
        const auto index = size_t( StreamMemoryClass::SourceCpu );
        struct Restore { std::shared_ptr<WorldStreamingScheduler> scheduler; u64 limit;
            ~Restore() { String error; scheduler->setBudget( StreamMemoryClass::SourceCpu, limit, error ); }
        } restore{ scheduler, stats.limits[index] };
        bool admitted = false; unsigned denied = 0;
        for( u64 headroom = 0; headroom <= 65536; headroom += 1024 )
        {
            check( scheduler->setBudget( StreamMemoryClass::SourceCpu, stats.used[index] + headroom, error ), error );
            const bool ok = fitOpenCityRoadRoute( pinned, source, plane, retained, error );
            const auto used = scheduler->stats().used[index];
            check( scheduler->setBudget( StreamMemoryClass::SourceCpu, restore.limit, error ), error );
            sameRoute( before, retained );
            if( ok ) { admitted = true; break; }
            ++denied; need( used == stats.used[index], "Route interval allocation denial leaked SourceCpu storage" );
        }
        need( admitted && denied > 1, "Pinned-route coverage budget sweep did not exercise staged allocation denial" );
    }
}
int main()
{
    int result = 0;
    try
    {
        sharedEndpointIdentity(); generated( false, 6, 42, true ); generated( true, 6, 42, true );
        for( bool natural : { false, true } ) for( int blocks : { 6, 8, 10 } ) for( u32 seed : { 0u, 7u, 42u, 4294967295u } )
            generated( natural, blocks, seed, false );
        naturalJunctionCorpus(); hierarchicalWidths(); legacyNaturalRouteCompatibility(); failures(); editedRouteContinuity();
        std::cout << "Open city editable source and island-avoiding pinned route PASS\n";
    }
    catch( const std::exception &e ) { std::cerr << "Open city road source: " << e.what() << '\n'; result = 1; }
    WorldStreamingScheduler::shutdownShared(); return result;
}
