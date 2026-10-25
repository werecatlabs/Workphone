#include <Workphone/Interface/Procedural/CityRoadRoundabout.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

using namespace workphone;
using namespace workphone::procedural;
namespace
{
    constexpr float pi = 3.14159265358979323846f, tau = 2 * pi;
    void require( bool value, const char *message ) { if( !value ) throw std::runtime_error( message ); }
    void check( bool value, const String &error ) { if( !value ) throw std::runtime_error( error.c_str() ); }
    bool near( Vector3F a, Vector3F b )
    { return std::abs( a.x - b.x ) < .0001f && std::abs( a.y - b.y ) < .0001f && std::abs( a.z - b.z ) < .0001f; }
    CityRoadRoundaboutSpec source( unsigned count = 4, bool clockwise = true )
    {
        CityRoadRoundaboutSpec spec;
        spec.junctionId = "10000000-0000-4000-8000-000000000001";
        spec.center = { 17, 4, -23 }; spec.clockwise = clockwise;
        for( unsigned i = 0; i < count; ++i )
        {
            CityRoadRoundaboutPort port;
            port.approachId = "20000000-0000-4000-8000-000000000001";
            port.approachId.back() = char( '1' + i );
            const float angle = .17f + tau * i / count;
            port.outward = { std::cos( angle ), 0, std::sin( angle ) };
            port.width = 6; port.length = 18 + i;
            spec.ports.push_back( std::move( port ) );
        }
        return spec;
    }
    float plane( const CityRoadRoundaboutSpec &spec, float x, float z )
    { return spec.center.y + spec.gradeX * ( x - spec.center.x ) + spec.gradeZ * ( z - spec.center.z ); }
    CityRoadRoundaboutHeightSampler terrain( const CityRoadRoundaboutSpec &spec )
    {
        return [&spec]( const Vector3F &p, float &y ) { y = plane( spec, p.x, p.z ); return true; };
    }
    bool drivable( CityRoadRoundaboutSurface kind )
    { return kind == CityRoadRoundaboutSurface::Circulation || kind == CityRoadRoundaboutSurface::Approach; }
    float segmentDistance( float ax, float az, float bx, float bz )
    {
        const float dx = bx - ax, dz = bz - az, length = dx * dx + dz * dz;
        const float t = length > 0 ? std::clamp( -( ax * dx + az * dz ) / length, 0.f, 1.f ) : 0;
        return std::hypot( ax + dx * t, az + dz * t );
    }
    void sameBake( const CityRoadRoundaboutBake &a, const CityRoadRoundaboutBake &b )
    {
        require( a.junctionId == b.junctionId && near( a.center, b.center ) && a.outerRadius == b.outerRadius &&
            a.islandRadius == b.islandRadius && a.vertices.size() == b.vertices.size() && a.indices == b.indices &&
            a.triangleKinds == b.triangleKinds && a.connections.size() == b.connections.size() && a.routes.size() == b.routes.size(),
            "Roundabout failed to preserve deterministic/last-good output" );
        for( std::size_t i = 0; i < a.vertices.size(); ++i )
            require( near( a.vertices[i].position, b.vertices[i].position ) && near( a.vertices[i].normal, b.vertices[i].normal ) &&
                a.vertices[i].u == b.vertices[i].u && a.vertices[i].v == b.vertices[i].v, "Roundabout vertex changed" );
        for( std::size_t i = 0; i < a.connections.size(); ++i )
        {
            require( a.connections[i].approachId == b.connections[i].approachId &&
                a.connections[i].entry.size() == b.connections[i].entry.size() &&
                a.connections[i].exit.size() == b.connections[i].exit.size(), "Roundabout connection reference changed" );
            for( std::size_t j = 0; j < a.connections[i].entry.size(); ++j )
                require( near( a.connections[i].entry[j], b.connections[i].entry[j] ), "Roundabout entry changed" );
            for( std::size_t j = 0; j < a.connections[i].exit.size(); ++j )
                require( near( a.connections[i].exit[j], b.connections[i].exit[j] ), "Roundabout exit changed" );
        }
        for( std::size_t i = 0; i < a.routes.size(); ++i )
        {
            require( a.routes[i].entryApproachId == b.routes[i].entryApproachId &&
                a.routes[i].exitApproachId == b.routes[i].exitApproachId && a.routes[i].points.size() == b.routes[i].points.size(),
                "Roundabout directed route changed" );
            for( std::size_t j = 0; j < a.routes[i].points.size(); ++j )
                require( near( a.routes[i].points[j], b.routes[i].points[j] ), "Roundabout route point changed" );
        }
    }
    void geometry()
    {
        for( unsigned count : { 3u, 4u, 5u, 6u } ) for( bool clockwise : { false, true } )
        {
            auto spec = source( count, clockwise );
            spec.gradeX = .02f; spec.gradeZ = -.03f;
            if( count == 6 ) spec.ringSamples = 256;
            CityRoadRoundaboutBake bake, repeated; String error;
            check( bakeCityRoadRoundabout( spec, terrain( spec ), bake, error ), error );
            check( bakeCityRoadRoundabout( spec, terrain( spec ), repeated, error ), error );
            sameBake( bake, repeated );
            require( bake.vertices.size() <= 256 * 8 + 6 * 34 + 1 && bake.indices.size() == bake.triangleKinds.size() * 3 &&
                bake.connections.size() == count && bake.routes.size() == count * ( count - 1 ), "Roundabout output budget/count changed" );
            require( !bake.containsSurface( spec.center.x, spec.center.z ) &&
                !bake.containsSurface( spec.center.x + spec.islandRadius - .1f, spec.center.z ) &&
                !bake.containsSurface( spec.center.x + 1000, spec.center.z ) &&
                !bake.containsSurface( std::numeric_limits<float>::quiet_NaN(), spec.center.z ),
                "Roundabout island/outside accepted as drivable" );
            for( unsigned i = 0; i < 360; ++i )
            {
                const float angle = tau * i / 360, radius = ( spec.islandRadius + spec.outerRadius ) * .5f;
                require( bake.containsSurface( spec.center.x + std::cos( angle ) * radius,
                    spec.center.z + std::sin( angle ) * radius ), "Annulus contains a surface gap" );
            }
            unsigned circulation = 0, approaches = 0, island = 0, kerb = 0;
            double roadArea = 0;
            for( const auto &v : bake.vertices )
                require( std::isfinite( v.position.x ) && std::isfinite( v.position.y ) && std::isfinite( v.position.z ) &&
                    std::isfinite( v.u ) && std::isfinite( v.v ) && std::abs( v.normal.length() - 1 ) < .0001f,
                    "Roundabout vertex/UV/normal is invalid" );
            for( std::size_t t = 0; t < bake.triangleKinds.size(); ++t )
            {
                const auto kind = bake.triangleKinds[t];
                const auto ia = bake.indices[t * 3], ib = bake.indices[t * 3 + 1], ic = bake.indices[t * 3 + 2];
                require( ia < bake.vertices.size() && ib < bake.vertices.size() && ic < bake.vertices.size(), "Roundabout index is out of bounds" );
                const auto &a = bake.vertices[ia], &b = bake.vertices[ib], &c = bake.vertices[ic];
                const auto normal = ( b.position - a.position ).crossProduct( c.position - a.position );
                require( normal.lengthSquared() > 1.e-12f && normal.dotProduct( a.normal + b.normal + c.normal ) > 0,
                    "Roundabout triangle winding opposes its normals" );
                const float uvArea = ( b.u - a.u ) * ( c.v - a.v ) - ( b.v - a.v ) * ( c.u - a.u );
                require( std::abs( uvArea ) > 1.e-10f, "Roundabout generated degenerate UVs" );
                if( drivable( kind ) )
                {
                    require( normal.y > 0 && a.normal.y > .99f && b.normal.y > .99f && c.normal.y > .99f,
                        "Roundabout road lacks upward normals" );
                    for( const auto p : { a.position, b.position, c.position } )
                        require( std::abs( p.y - plane( spec, p.x, p.z ) ) < .0001f, "Road/approach left its shared grade plane" );
                    const auto p = a.position - spec.center, q = b.position - spec.center, r = c.position - spec.center;
                    require( std::min( { segmentDistance( p.x, p.z, q.x, q.z ), segmentDistance( q.x, q.z, r.x, r.z ),
                        segmentDistance( r.x, r.z, p.x, p.z ) } ) >= spec.islandRadius - .0001f,
                        "Road triangle cuts through central island" );
                    roadArea += normal.y * .5;
                }
                else for( const auto p : { a.position, b.position, c.position } )
                    require( p.y >= plane( spec, p.x, p.z ) - .0001f &&
                        p.y <= plane( spec, p.x, p.z ) + spec.islandHeight + .0001f,
                        "Island/kerb escaped its shared contact heights" );
                circulation += kind == CityRoadRoundaboutSurface::Circulation;
                approaches += kind == CityRoadRoundaboutSurface::Approach;
                island += kind == CityRoadRoundaboutSurface::Island;
                kerb += kind == CityRoadRoundaboutSurface::Kerb;
            }
            const auto splitters = std::count_if( spec.ports.begin(), spec.ports.end(),
                []( const CityRoadRoundaboutPort &port ) { return port.width >= 6 && port.length >= 12; } );
            const unsigned ringCount = circulation / 2;
            require( circulation <= 512 && approaches == count * 2 && island == ringCount + splitters * 4 &&
                kerb == ringCount * 4 + splitters * 12,
                "Roundabout lost separated annulus/approach/raised island/kerb surfaces" );
            double expectedArea = pi * ( spec.outerRadius * spec.outerRadius - spec.islandRadius * spec.islandRadius );
            for( const auto &p : spec.ports )
            {
                const float half = p.width * .5f, chord = std::sqrt( spec.outerRadius * spec.outerRadius - half * half );
                expectedArea -= spec.outerRadius * spec.outerRadius * std::asin( half / spec.outerRadius ) - chord * half;
                expectedArea += p.width * ( spec.outerRadius + p.length - chord );
            }
            require( std::abs( roadArea - expectedArea ) / expectedArea < .012,
                "Road geometry overlaps, fills the island, or loses its trimmed approach area" );
            for( std::size_t i = 0; i < count; ++i )
            {
                const auto &port = spec.ports[i];
                const auto &connection = bake.connections[i];
                require( connection.approachId == port.approachId && connection.entry.size() == 3 && connection.exit.size() == 3,
                    "Stable road references/connection endpoints changed" );
                const Vector3F expected = spec.center + port.outward * ( spec.outerRadius + port.length );
                const auto middle = ( connection.entry.front() + connection.exit.back() ) * .5f;
                require( std::abs( middle.x - expected.x ) < .0001f && std::abs( middle.z - expected.z ) < .0001f,
                    "Approach did not terminate at its explicit external road connection" );
                require( std::abs( ( connection.entry.front() - connection.exit.back() ).length() - port.width * .5f ) < .01f,
                    "Entry and exit lanes share the same centreline" );
                if( port.width >= 6 && port.length >= 12 )
                {
                    const float middle = spec.outerRadius + ( 1.5f + std::min( 8.f, port.length * .5f ) ) * .5f;
                    const auto center = spec.center + port.outward * middle;
                    const Vector3F across{ -port.outward.z, 0, port.outward.x };
                    require( !bake.containsSurface( center.x, center.z ) &&
                        !bake.containsSurface( center.x + across.x * .25f, center.z + across.z * .25f ),
                        "Raised splitter refuge was accepted as drivable pavement" );
                    require( bake.containsSurface( center.x + across.x * .4f, center.z + across.z * .4f ) &&
                        bake.containsSurface( center.x - across.x * .4f, center.z - across.z * .4f ),
                        "Splitter query incorrectly masks the adjacent approach lanes" );
                    const auto before = spec.center + port.outward * ( spec.outerRadius + .5f );
                    const auto after = spec.center + port.outward * ( spec.outerRadius + std::min( 8.f, port.length * .5f ) + 1 );
                    require( bake.containsSurface( before.x, before.z ) && bake.containsSurface( after.x, after.z ),
                        "Vertical splitter faces mask pavement beyond their finite footprint" );
                }
            }
            std::set<std::pair<std::string, std::string>> directed;
            for( const auto &route : bake.routes )
            {
                require( route.entryApproachId != route.exitApproachId && route.points.size() <= 262,
                    "Roundabout route is unbounded or takes an implicit U-turn" );
                require( directed.emplace( std::string( route.entryApproachId.data(), route.entryApproachId.size() ),
                    std::string( route.exitApproachId.data(), route.exitApproachId.size() ) ).second, "Directed route repeated" );
                for( std::size_t i = 1; i < route.points.size(); ++i )
                {
                    const auto a = route.points[i - 1], b = route.points[i];
                    const unsigned samples = std::max( 1u, unsigned( std::ceil( ( b - a ).length() / .5f ) ) );
                    for( unsigned sample = 0; sample <= samples; ++sample )
                    {
                        const auto p = a + ( b - a ) * ( float( sample ) / samples );
                        require( bake.containsSurface( p.x, p.z ) &&
                            std::hypot( p.x - spec.center.x, p.z - spec.center.z ) >= spec.islandRadius,
                            "Directed connection/route leaves road surface or crosses island" );
                        for( const auto &port : spec.ports ) if( port.width >= 6 && port.length >= 12 )
                        {
                            const auto local = p - spec.center;
                            const float along = local.x * port.outward.x + local.z * port.outward.z;
                            const float across = -local.x * port.outward.z + local.z * port.outward.x;
                            const float longitudinal = std::max( { spec.outerRadius + 1.5f - along, 0.f,
                                along - spec.outerRadius - std::min( 8.f, port.length * .5f ) } );
                            const float lateral = std::max( std::abs( across ) - .3f, 0.f );
                            require( std::hypot( longitudinal, lateral ) >= 1.f,
                                "Directed lane route lacks one metre of splitter clearance" );
                        }
                    }
                    // The arc vertices (excluding radial entry/exit connector legs)
                    // must advance in the requested circulation direction.
                    if( i >= 3 && i + 2 < route.points.size() )
                    {
                        const auto p = a - spec.center, q = b - spec.center;
                        const float turn = p.x * q.z - p.z * q.x;
                        require( clockwise ? turn > 0 : turn < 0, "Circulation ignores clockwise selection" );
                    }
                }
            }
        }
    }
    void splitterThresholds()
    {
        for( const auto dimensions : { std::pair<float, float>{ 5.99f, 18 }, { 6, 11.99f }, { 6, 12 }, { 12, 18 } } )
        {
            auto spec = source(); spec.ringSamples = 32;
            for( auto &port : spec.ports ) { port.width = dimensions.first; port.length = dimensions.second; }
            CityRoadRoundaboutBake bake; String error;
            check( bakeCityRoadRoundabout( spec, terrain( spec ), bake, error ), error );
            const bool raised = dimensions.first >= 6 && dimensions.second >= 12;
            for( const auto &port : spec.ports )
            {
                const float along = spec.outerRadius + ( 1.5f + std::min( 8.f, port.length * .5f ) ) * .5f;
                const auto point = spec.center + port.outward * along;
                require( bake.containsSurface( point.x, point.z ) != raised,
                    "Splitter geometry/query qualification differs from width/length thresholds" );
            }
            std::size_t raisedTriangles = 0;
            for( std::size_t triangle = 0; triangle < bake.triangleKinds.size(); ++triangle )
                if( bake.triangleKinds[triangle] == CityRoadRoundaboutSurface::Island )
                {
                    const auto &a = bake.vertices[bake.indices[triangle * 3]].position;
                    const auto &b = bake.vertices[bake.indices[triangle * 3 + 1]].position;
                    const auto &c = bake.vertices[bake.indices[triangle * 3 + 2]].position;
                    const auto center = ( a + b + c ) / 3.f;
                    if( std::hypot( center.x - spec.center.x, center.z - spec.center.z ) > spec.outerRadius )
                    {
                        ++raisedTriangles;
                        for( const auto p : { a, b, c } )
                            require( std::abs( p.y - plane( spec, p.x, p.z ) - std::min( .12f, spec.islandHeight ) ) < .0001f,
                                "Splitter top does not share the raised render/collision contact profile" );
                    }
                }
            require( raisedTriangles == ( raised ? spec.ports.size() * 4 : 0 ),
                "Splitter top triangle count exceeded its bounded derived geometry" );
        }
    }
    void failures()
    {
        const auto spec = source(); String error; CityRoadRoundaboutBake bake;
        check( bakeCityRoadRoundabout( spec, terrain( spec ), bake, error ), error );
        const auto old = bake;
        const auto reject = [&]( const CityRoadRoundaboutSpec &bad, const CityRoadRoundaboutHeightSampler &sample ) {
            require( !bakeCityRoadRoundabout( bad, sample, bake, error ) && !error.empty(), "Invalid roundabout was accepted" );
            sameBake( old, bake );
        };
        auto bad = spec; bad.junctionId = "node-17"; reject( bad, terrain( spec ) );
        bad = spec; bad.junctionId = "00000000-0000-0000-0000-000000000000"; reject( bad, terrain( spec ) );
        bad = spec; bad.ports[1].approachId = bad.ports[0].approachId; reject( bad, terrain( spec ) );
        bad = spec; bad.ports[0].approachId = bad.junctionId; reject( bad, terrain( spec ) );
        bad = spec; bad.ports.resize( 2 ); reject( bad, terrain( spec ) );
        bad = spec; bad.ports.push_back( bad.ports[0] ); bad.ports.push_back( bad.ports[0] ); bad.ports.push_back( bad.ports[0] ); reject( bad, terrain( spec ) );
        bad = spec; bad.outerRadius = bad.islandRadius; reject( bad, terrain( spec ) );
        bad = spec; bad.outerRadius = 30; reject( bad, terrain( spec ) );
        bad = spec; bad.islandRadius = std::numeric_limits<float>::quiet_NaN(); reject( bad, terrain( spec ) );
        bad = spec; bad.ringSamples = 257; reject( bad, terrain( spec ) );
        bad = spec; bad.ringSamples = 16; reject( bad, terrain( spec ) );
        bad = spec; bad.ports[0].outward = {}; reject( bad, terrain( spec ) );
        bad = spec; bad.ports[0].outward.y = 1; reject( bad, terrain( spec ) );
        bad = spec; bad.ports[1].outward = bad.ports[0].outward; reject( bad, terrain( spec ) );
        bad = spec; bad.ports[0].width = 100; reject( bad, terrain( spec ) );
        bad = spec; bad.ports[0].length = 0; reject( bad, terrain( spec ) );
        bad = spec; bad.gradeX = .1f; reject( bad, terrain( spec ) );
        bad = spec; bad.maximumTerrainSamples = 32; reject( bad, terrain( spec ) );
        reject( spec, {} );
        reject( spec, []( const Vector3F &, float & ) { return false; } );
        reject( spec, []( const Vector3F &, float &y ) { y = std::numeric_limits<float>::infinity(); return true; } );
        reject( spec, []( const Vector3F &, float &y ) { y = 7; return true; } );
        reject( spec, [&spec]( const Vector3F &p, float &y ) { y = plane( spec, p.x, p.z ); return std::hypot( p.x - spec.center.x, p.z - spec.center.z ) > 1; } );
        reject( spec, []( const Vector3F &, float & ) -> bool { throw std::runtime_error( "retained terrain unavailable" ); } );
    }
    void budgetDenials()
    {
        const auto scheduler = WorldStreamingScheduler::shared();
        constexpr auto sourceClass = std::size_t( StreamMemoryClass::SourceCpu );
        struct Restore
        {
            std::shared_ptr<WorldStreamingScheduler> scheduler; u64 limit;
            ~Restore() { String error; scheduler->setBudget( StreamMemoryClass::SourceCpu, limit, error ); }
        } restore{ scheduler, scheduler->stats().limits[sourceClass] };
        const auto spec = source(); String error; CityRoadRoundaboutBake bake;
        check( bakeCityRoadRoundabout( spec, terrain( spec ), bake, error ), error );
        const auto old = bake;
        const auto baseline = scheduler->stats().used[sourceClass];
        unsigned failures = 0; bool admitted = false;
        for( u64 additional : { 0ull, 64ull, 256ull, 1024ull, 4096ull, 16384ull, 32768ull, 65536ull, 131072ull, 262144ull, 524288ull } )
        {
            check( scheduler->setBudget( StreamMemoryClass::SourceCpu, baseline + additional, error ), error );
            const bool ok = bakeCityRoadRoundabout( spec, terrain( spec ), bake, error );
            const auto used = scheduler->stats().used[sourceClass];
            check( scheduler->setBudget( StreamMemoryClass::SourceCpu, restore.limit, error ), error );
            require( used == baseline, "Roundabout allocation-denial/success path leaked SourceCpu storage" );
            sameBake( old, bake );
            if( ok ) { admitted = true; break; }
            ++failures;
        }
        require( admitted && failures >= 4, "Roundabout allocation denial test did not exercise staged buffers" );
    }
}
int main()
{
    int result = 0;
    try { geometry(); splitterThresholds(); failures(); budgetDenials(); std::cout << "Roundabout: true annulus, chord joins, raised island/kerb/splitters, grade fit, directed routes and transactional budgets PASS\n"; }
    catch( const std::exception &e ) { std::cerr << "Roundabout: " << e.what() << '\n'; result = 1; }
    WorldStreamingScheduler::shutdownShared(); return result;
}
