#include <Workphone/Interface/Procedural/OpenCityRoadSource.hpp>
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
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
    void generated( bool natural, int blocks, u32 seed, bool meshCheck )
    {
        const auto city = OpenCityLayout::generate( seed, blocks, 0, natural );
        auto policy = options( seed ); if( !meshCheck ) policy.maximumRoundabouts = 2;
        OpenCityRoadSourceResult converted; String error;
        check( buildOpenCityRoadSource( city, policy, plane, converted, error ), error );
        uniqueIds( converted.source );
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
            const auto saved = reused;
            auto missing = converted.source; missing.roads.clear(); missing.roundabouts.clear();
            need( !fitOpenCityRoadRoute( city, missing, plane, reused, error ), "Missing pinned route silently acquired a fallback" );
            sameRoute( saved, reused );
        }
        std::cout << ( natural ? "natural" : "grid" ) << " seed=" << seed << " blocks=" << blocks <<
            " roads=" << converted.source.roads.size() << " junctions=" << converted.source.roundabouts.size() <<
            " route=" << converted.route.size() << " merged=" << converted.mergedChains << '\n';
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
        failures(); editedRouteContinuity(); std::cout << "Open city editable source and island-avoiding pinned route PASS\n";
    }
    catch( const std::exception &e ) { std::cerr << "Open city road source: " << e.what() << '\n'; result = 1; }
    WorldStreamingScheduler::shutdownShared(); return result;
}
