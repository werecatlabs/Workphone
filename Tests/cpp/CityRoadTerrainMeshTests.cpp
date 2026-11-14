#include <Workphone/Scene/Components/CityRoadTerrainMesh.hpp>
#include <Workphone/Scene/Components/CityRoadJunctions.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <limits>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

using namespace workphone;
using namespace workphone::procedural;
using namespace workphone::scene;
namespace
{
    void require( bool value, const char *message ) { if( !value ) throw std::runtime_error( message ); }
    void check( bool value, const String &error ) { if( !value ) throw std::runtime_error( error.c_str() ); }
    Vector3F terrainWorld( const Transform3<real_Num> &placement, Vector3F local )
    {
        return placement.getPosition() + placement.getOrientation() * ( local * placement.getScale() );
    }
    u64 checksum( const CityRoadTerrainMesh &mesh )
    {
        u64 value = 1469598103934665603ull;
        const auto fold = [&]( u64 input ) { value = ( value ^ input ) * 1099511628211ull; };
        const auto number = [&]( float input ) { u32 bits; std::memcpy( &bits, &input, sizeof( bits ) ); fold( bits ); };
        fold( mesh.sourceRevision ); fold( mesh.terrainRevision );
        for( const auto &v : mesh.vertices )
        {
            number( v.position.x ); number( v.position.y ); number( v.position.z );
            number( v.normal.x ); number( v.normal.y ); number( v.normal.z ); number( v.uv.x ); number( v.uv.y );
        }
        for( auto index : mesh.indices ) fold( index );
        for( auto kind : mesh.triangleKinds ) fold( u32( kind ) );
        return value;
    }
    std::shared_ptr<render::TerrainData> terrain()
    {
        auto data = std::make_shared<render::TerrainData>();
        data->dimensions = { 65, 65 }; data->spacing = { 1, 1 }; data->origin = { -32, -32 };
        data->heights.assign( 65 * 65, 0 ); data->revision = 29; return data;
    }
    CityRoadSplineSource source( const Transform3<real_Num> &placement = {} )
    {
        CityRoadSplineSource s; s.id = "10000000-0000-4000-8000-000000000001"; s.revision = 7;
        s.terrainResourceUuid = "20000000-0000-4000-8000-000000000001";
        CityRoadSpline road; road.id = "30000000-0000-4000-8000-000000000001";
        road.width = 8; road.maxGrade = .25f; road.heightOffset = .025f;
        road.knots.push_back( { "40000000-0000-4000-8000-000000000001", terrainWorld( placement, { 0, 0, 20 } ), {}, {} } );
        road.knots.push_back( { "40000000-0000-4000-8000-000000000002", terrainWorld( placement, { 0, 0, -20 } ), {}, {} } );
        s.roads.push_back( std::move( road ) ); return s;
    }
    CityRoadSplineBake sample( const CityRoadSplineSource &s, render::TerrainSnapshot data,
                               const Transform3<real_Num> &placement = {} )
    {
        CityRoadSplineBake bake; String error; CityRoadSplineSampling options; options.maxStep = 10;
        check( sampleCityRoadSplines( s, options, [data, placement]( const Vector3F &p, float &y ) {
            return render::sampleTerrainHeight( *data, placement, p, y );
        }, bake, error ), error ); return bake;
    }
    void agreement( const CityRoadTerrainMesh &mesh, render::TerrainSnapshot data,
                     const Transform3<real_Num> &placement, float offset )
    {
        require( !mesh.indices.empty() && mesh.indices.size() % 3 == 0 &&
                 mesh.triangleKinds.size() == mesh.indices.size() / 3, "Mesh topology is incomplete" );
        require( mesh.sourceRevision == 7 && mesh.terrainRevision == 29, "Mesh revision identity is wrong" );
        for( const auto &vertex : mesh.vertices )
        {
            float height = 0;
            require( render::sampleTerrainHeight( *data, placement, vertex.position, height ) &&
                     std::abs( vertex.position.y - height - offset ) < .00015f,
                     "A clipped road vertex differs from retained terrain" );
            require( std::isfinite( vertex.uv.x ) && std::isfinite( vertex.uv.y ) &&
                     vertex.normal.y > .9f && std::abs( vertex.normal.length() - 1 ) < .001f,
                     "Road normal/UV is invalid or downward" );
        }
        for( size_t i = 0; i < mesh.indices.size(); i += 3 )
        {
            const auto a = mesh.vertices[mesh.indices[i]].position;
            const auto b = mesh.vertices[mesh.indices[i + 1]].position;
            const auto c = mesh.vertices[mesh.indices[i + 2]].position;
            require( ( b - a ).crossProduct( c - a ).y > 0, "Road winding faces downward" );
            const auto center = ( a + b + c ) / 3.f; float height = 0;
            require( render::sampleTerrainHeight( *data, placement, center, height ) &&
                     std::abs( center.y - height - offset ) < .00015f,
                     "Road triangle bridges a terrain ridge/diagonal" );
        }
    }
    void conforming()
    {
        auto data = terrain(); data->heights[35 * 65 + 34] = .12f;
        auto s = source(); auto bake = sample( s, data ); CityRoadTerrainMesh mesh; String error;
        check( buildCityRoadTerrainMesh( s, bake, data, {}, mesh, error ), error );
        agreement( mesh, data, {}, .025f );
        float highest = 0;
        for( const auto &vertex : mesh.vertices ) highest = std::max( highest, vertex.position.y );
        require( highest >= .144f, "Exact cell clipping missed a ridge between coarse edge samples" );

        Transform3<real_Num> placement;
        placement.setPosition( { 100, 4, 200 } ); placement.setScale( { 1.4f, 1.2f, .8f } );
        const float yaw = .31f; placement.setOrientation( { std::cos( yaw * .5f ), 0, std::sin( yaw * .5f ), 0 } );
        s = source( placement ); bake = sample( s, data, placement );
        check( buildCityRoadTerrainMesh( s, bake, data, placement, mesh, error ), error );
        agreement( mesh, data, placement, .025f );
    }
    void collinearRuns()
    {
        auto data = terrain(); data->heights[35 * 65 + 34] = .12f;
        const auto s = source(); const auto coarse = sample( s, data );
        CityRoadSplineSampling options; options.maxStep = .125f;
        CityRoadSplineBake dense; String error;
        check( sampleCityRoadSplines( s, options, [data]( const Vector3F &p, float &height ) {
            return render::sampleTerrainHeight( *data, {}, p, height );
        }, dense, error ), error );
        require( dense.roads[0].points.size() > 256 && dense.roads[0].points.size() > coarse.roads[0].points.size() * 8,
            "Collinear regression did not exercise redundant Bezier subdivision" );
        CityRoadTerrainMesh sparseMesh, denseMesh;
        check( buildCityRoadTerrainMesh( s, coarse, data, {}, sparseMesh, error ), error );
        check( buildCityRoadTerrainMesh( s, dense, data, {}, denseMesh, error ), error );
        require( denseMesh.vertices.size() == sparseMesh.vertices.size() && denseMesh.indices.size() == sparseMesh.indices.size(),
            "Straight road terrain geometry still grows with authored sampling density" );
        agreement( denseMesh, data, {}, .025f );
        float highest = 0; for( const auto &v : denseMesh.vertices ) highest = std::max( highest, v.position.y );
        require( highest > .144f, "Collinear reduction flattened an interior terrain-cell ridge" );
        const auto retained = checksum( denseMesh );
        auto holes = terrain(); holes->holes.assign( 64 * 64, 0 ); holes->holes[32 * 64 + 34] = 1;
        require( !buildCityRoadTerrainMesh( s, dense, holes, {}, denseMesh, error ) && checksum( denseMesh ) == retained,
            "Collinear reduction skipped a covered hole or replaced last-good geometry" );

        auto fixed = source(); auto &road = fixed.roads[0];
        road.heightMode = CityRoadHeightMode::Fixed; road.heightOffset = 0;
        road.knots[1].position = { 0, .02f, 0 };
        road.knots.push_back( { "40000000-0000-4000-8000-000000000003", { 0, 0, -20 }, {}, {} } );
        const auto flat = terrain(); const auto fixedBake = sample( fixed, flat );
        check( buildCityRoadTerrainMesh( fixed, fixedBake, flat, {}, denseMesh, error ), error );
        highest = 0; for( const auto &v : denseMesh.vertices ) highest = std::max( highest, v.position.y );
        require( highest > .0199f, "Collinear reduction erased a Fixed road elevation break" );
        auto poisoned = dense; poisoned.roads[0].points[poisoned.roads[0].points.size() / 2].left.y =
            std::numeric_limits<float>::quiet_NaN();
        const auto fixedChecksum = checksum( denseMesh );
        require( !buildCityRoadTerrainMesh( s, poisoned, data, {}, denseMesh, error ) && checksum( denseMesh ) == fixedChecksum,
            "Collinear reduction skipped a malformed interior bake sample" );
    }
    void failures()
    {
        auto data = terrain(); auto s = source(); auto bake = sample( s, data );
        CityRoadTerrainMesh mesh; String error;
        check( buildCityRoadTerrainMesh( s, bake, data, {}, mesh, error ), error );
        const auto originalVertices = mesh.vertices.size(), originalIndices = mesh.indices.size();
        const auto originalChecksum = checksum( mesh );
        const auto first = mesh.vertices.front().position;
        const auto reject = [&]( const CityRoadSplineSource &candidate, const CityRoadSplineBake &candidateBake,
                                 render::TerrainSnapshot candidateTerrain ) {
            require( !buildCityRoadTerrainMesh( candidate, candidateBake, candidateTerrain, {}, mesh, error ) && !error.empty(),
                     "Unsupported road/terrain was accepted" );
            require( mesh.vertices.size() == originalVertices && mesh.indices.size() == originalIndices &&
                     mesh.vertices.front().position == first && mesh.sourceRevision == 7 && checksum( mesh ) == originalChecksum,
                     "Failed preparation replaced last-good road mesh" );
        };
        auto holes = terrain(); holes->holes.assign( 64 * 64, 0 ); holes->holes[32 * 64 + 34] = 1;
        // This hole lies between centre and edge samples, so the generic source
        // sampler accepts it. The complete swept-footprint helper must reject it.
        auto holeBake = sample( s, holes ); reject( s, holeBake, holes );
        auto ridge = terrain(); ridge->heights[35 * 65 + 34] = 1.2f;
        auto ridgeBake = sample( s, ridge ); reject( s, ridgeBake, ridge );
        auto stale = bake; ++stale.sourceRevision; reject( s, stale, data );
        auto poisoned = bake; poisoned.roads[0].points.back().distance = std::numeric_limits<float>::quiet_NaN();
        reject( s, poisoned, data );
        poisoned = bake; poisoned.roads[0].points[1].distance = 0; reject( s, poisoned, data );
        reject( s, bake, {} );
        auto outside = bake; for( auto &point : outside.roads[0].points ) point.left.x = -33;
        reject( s, outside, data );
        auto raised = s; raised.roads[0].heightOffset = 5;
        reject( raised, sample( raised, data ), data );
        auto fixed = s; fixed.roads[0].heightMode = CityRoadHeightMode::Fixed;
        fixed.roads[0].heightOffset = 0; for( auto &knot : fixed.roads[0].knots ) knot.position.y = .04f;
        reject( fixed, sample( fixed, data ), data );

        auto empty = s; empty.version = 2; empty.roads.clear(); CityRoadSplineBake emptyBake;
        check( sampleCityRoadSplines( empty, {}, {}, emptyBake, error ), error );
        check( buildCityRoadTerrainMesh( empty, emptyBake, {}, {}, mesh, error ), error );
        require( mesh.vertices.empty() && mesh.indices.empty(), "Canonical empty source did not clear its mesh" );
    }
    void budgetDenial()
    {
        auto data = terrain(); auto s = source(); auto bake = sample( s, data );
        CityRoadTerrainMesh mesh; String error;
        check( buildCityRoadTerrainMesh( s, bake, data, {}, mesh, error ), error );
        const auto scheduler = WorldStreamingScheduler::shared();
        const auto stats = scheduler->stats(); const auto index = size_t( StreamMemoryClass::SourceCpu );
        struct Restore { std::shared_ptr<WorldStreamingScheduler> scheduler; u64 limit;
            ~Restore() { String error; scheduler->setBudget( StreamMemoryClass::SourceCpu, limit, error ); }
        } restore{ scheduler, stats.limits[index] };
        const auto original = checksum( mesh ); unsigned failures = 0; bool admitted = false;
        u64 lastDeniedHeadroom = 0;
        for( u64 headroom = 0; headroom <= 1024 * 1024; headroom += 4096 )
        {
            check( scheduler->setBudget( StreamMemoryClass::SourceCpu, stats.used[index] + headroom, error ), error );
            const bool ok = buildCityRoadTerrainMesh( s, bake, data, {}, mesh, error );
            const auto used = scheduler->stats().used[index];
            check( scheduler->setBudget( StreamMemoryClass::SourceCpu, restore.limit, error ), error );
            require( checksum( mesh ) == original, "Staged SourceCpu denial changed resident geometry" );
            require( used == stats.used[index], "Clipping allocation failure leaked SourceCpu storage" );
            if( ok ) { admitted = true; break; }
            ++failures; lastDeniedHeadroom = headroom;
        }
        require( admitted && failures > 4 && lastDeniedHeadroom > 32768,
                 "Budget sweep did not exercise failure after staged mesh buffers were allocated" );
    }
    void roundaboutSupport()
    {
        auto data = terrain(); data->dimensions = { 129, 129 }; data->origin = { -64, -64 };
        data->heights.assign( 129 * 129, 0 );
        auto s = source(); s.version = 2; s.roads.clear();
        CityRoadRoundaboutSpec junction; junction.junctionId = "50000000-0000-4000-8000-000000000001";
        junction.center.y = .025f;
        const Vector3F directions[] = { { 1, 0, 0 }, { 0, 0, 1 }, { -1, 0, 0 } };
        const char *roadIds[] = { "30000000-0000-4000-8000-000000000001",
            "30000000-0000-4000-8000-000000000002", "30000000-0000-4000-8000-000000000003" };
        const char *knotIds[][2] = {
            { "40000000-0000-4000-8000-000000000001", "40000000-0000-4000-8000-000000000002" },
            { "40000000-0000-4000-8000-000000000003", "40000000-0000-4000-8000-000000000004" },
            { "40000000-0000-4000-8000-000000000005", "40000000-0000-4000-8000-000000000006" } };
        for( size_t i = 0; i < 3; ++i )
        {
            CityRoadSpline road; road.id = roadIds[i]; road.width = 6; road.heightOffset = .025f;
            road.knots.push_back( { knotIds[i][0], directions[i] * 32.f, {}, {} } );
            road.knots.push_back( { knotIds[i][1], directions[i] * 48.f, {}, {} } );
            s.roads.push_back( std::move( road ) );
            CityRoadRoundaboutPort port; port.approachId = roadIds[i]; port.outward = directions[i];
            junction.ports.push_back( std::move( port ) );
        }
        s.roundabouts.push_back( junction ); auto bake = sample( s, data );
        CityRoadTerrainMesh mesh; String error;
        check( buildCityRoadTerrainMesh( s, bake, data, {}, mesh, error ), error );
        size_t islands = 0, kerbs = 0;
        for( size_t i = 0; i < mesh.triangleKinds.size(); ++i )
        {
            const auto kind = mesh.triangleKinds[i];
            if( kind == CityRoadTerrainTriangleKind::Island ) ++islands;
            if( kind == CityRoadTerrainTriangleKind::Kerb ) ++kerbs;
            if( kind == CityRoadTerrainTriangleKind::Road )
                for( size_t v = 0; v < 3; ++v )
                {
                    const auto p = mesh.vertices[mesh.indices[i * 3 + v]].position;
                    require( std::hypot( p.x, p.z ) >= junction.islandRadius - .0001f,
                             "Terrain clipping introduced a driveable island shortcut" );
                }
        }
        require( islands && kerbs, "Roundabout island/kerb surfaces were lost" );
        const auto triangles = mesh.triangleKinds.size();
        const auto original = checksum( mesh );
        auto disconnected = s; disconnected.roads[0].knots.front().position.x += .02f;
        require( !buildCityRoadTerrainMesh( disconnected, sample( disconnected, data ), data, {}, mesh, error ) &&
                 checksum( mesh ) == original, "A disconnected approach road endpoint was accepted" );
        auto wrongWidth = s; wrongWidth.roads[0].width += .02f;
        require( !buildCityRoadTerrainMesh( wrongWidth, sample( wrongWidth, data ), data, {}, mesh, error ) &&
                 checksum( mesh ) == original, "A mismatched approach road width was accepted" );
        auto step = s; step.roundabouts[0].center.y += .002f;
        require( !buildCityRoadTerrainMesh( step, bake, data, {}, mesh, error ) && checksum( mesh ) == original,
                 "A 2 mm approach height seam was accepted" );
        auto badTangent = bake; badTangent.roads[0].points.front().tangent = { 0, 0, 1 };
        require( !buildCityRoadTerrainMesh( s, badTangent, data, {}, mesh, error ) && checksum( mesh ) == original,
                 "A misaligned approach tangent was accepted" );
        auto hole = std::make_shared<render::TerrainData>( *data ); hole->holes.assign( 128 * 128, 0 );
        hole->holes[64 * 128 + 64] = 1;
        require( !buildCityRoadTerrainMesh( s, bake, hole, {}, mesh, error ) &&
                 mesh.triangleKinds.size() == triangles, "A junction island hid a terrain hole or replaced last-good mesh" );
        auto unsupported = std::make_shared<render::TerrainData>( *data );
        for( auto &height : unsupported->heights ) height = .12f;
        require( !buildCityRoadTerrainMesh( s, bake, unsupported, {}, mesh, error ),
                 "Roundabout accepted a floating/cut profile beyond its tighter native support policy" );

        auto graded = std::make_shared<render::TerrainData>( *data );
        for( int z = 0; z < 129; ++z ) for( int x = 0; x < 129; ++x )
            graded->heights[z * 129 + x] = .01f * ( x - 64 ) + .015f * ( z - 64 );
        auto gradedSource = s; gradedSource.roundabouts[0].gradeX = .01f; gradedSource.roundabouts[0].gradeZ = .015f;
        check( buildCityRoadTerrainMesh( gradedSource, sample( gradedSource, graded ), graded, {}, mesh, error ), error );
    }
    CityRoadSplineSource ordinarySource( bool crossing, bool oblique = false, bool curved = false )
    {
        auto s = source(); s.version = 2; s.roads.clear();
        const auto append = [&]( unsigned i, Vector3F a, Vector3F b, float width ) {
            CityRoadSpline road; road.id = "30000000-0000-4000-8000-000000000001";
            road.id.back() = char( '1' + i ); road.width = width; road.heightOffset = .025f;
            SourceBudgetText first = "40000000-0000-4000-8000-000000000001", second = first;
            first.back() = char( '1' + 2 * i ); second.back() = char( '2' + 2 * i );
            const auto handle = ( b - a ) / 3.f;
            road.knots = { { first, a, {}, handle }, { second, b, -handle, {} } };
            s.roads.push_back( std::move( road ) );
        };
        append( 0, { -24, 0, 0 }, { 24, 0, 0 }, 12 );
        const Vector3F end = oblique ? Vector3F( 12, 0, 20 ) : Vector3F( 0, 0, 24 );
        append( 1, crossing ? -end : Vector3F(), end, 7 );
        if( curved )
        {
            s.roads[1].knots.front().outHandle = { 0, 0, 8 };
            s.roads[1].knots.back().inHandle = { 3, 0, -8 };
        }
        return s;
    }
    unsigned surfaceCoverage( const CityRoadTerrainMesh &mesh, double x, double z, bool strict = false )
    {
        unsigned hits = 0;
        for( size_t i = 0; i < mesh.indices.size(); i += 3 )
        {
            if( mesh.triangleKinds[i / 3] != CityRoadTerrainTriangleKind::Road ) continue;
            const auto a = mesh.vertices[mesh.indices[i]].position, b = mesh.vertices[mesh.indices[i + 1]].position,
                       c = mesh.vertices[mesh.indices[i + 2]].position;
            const double d = ( b.z - c.z ) * ( a.x - c.x ) + ( c.x - b.x ) * ( a.z - c.z );
            if( std::abs( d ) < 1e-10 ) continue;
            const double u = ( ( b.z - c.z ) * ( x - c.x ) + ( c.x - b.x ) * ( z - c.z ) ) / d;
            const double v = ( ( c.z - a.z ) * ( x - c.x ) + ( a.x - c.x ) * ( z - c.z ) ) / d;
            if( strict ? u > 1e-6 && v > 1e-6 && u + v < .999999 :
                         u >= -1e-6 && v >= -1e-6 && u + v <= 1.000001 ) ++hits;
        }
        return hits;
    }
    u64 junctionChecksum( const CityRoadJunctionBake &bake )
    {
        u64 hash = 1469598103934665603ull;
        const auto fold = [&]( u64 value ) { hash = ( hash ^ value ) * 1099511628211ull; };
        const auto number = [&]( float value ) { u32 bits; std::memcpy( &bits, &value, sizeof( bits ) ); fold( bits ); };
        const auto point = [&]( Vector3F p ) { number( p.x ); number( p.y ); number( p.z ); };
        fold( bake.sourceRevision ); fold( bake.roadRuns.size() ); fold( bake.junctions.size() );
        for( const auto &run : bake.roadRuns )
        { fold( run.roadIndex ); for( const auto &p : run.points ) { point( p.position ); point( p.left ); point( p.right ); number( p.distance ); } }
        for( const auto &junction : bake.junctions )
        {
            point( junction.center ); number( junction.heightOffset ); number( junction.maxGrade );
            for( const auto &p : junction.boundary ) point( p );
            for( auto index : junction.indices ) fold( index );
            for( const auto &port : junction.ports ) { fold( port.roadIndex ); number( port.station ); fold( port.forward ); point( port.boundary.position ); }
            for( const auto &corner : junction.corners ) { for( auto p : corner.points ) point( p ); for( auto p : corner.outward ) point( p ); }
        }
        return hash;
    }
    void ordinaryJunctions()
    {
        for( unsigned variant = 0; variant < 4; ++variant )
        {
            const bool crossing = variant == 1 || variant == 2, oblique = variant == 2, curved = variant == 3;
            std::cout << "ordinary junction variant=" << variant << " crossing=" << crossing <<
                " oblique=" << oblique << " curved=" << curved << '\n';
            auto data = terrain();
            for( int z = 0; z < 65; ++z ) for( int x = 0; x < 65; ++x )
                data->heights[z * 65 + x] = .007f * ( x - 32 ) + .011f * ( z - 32 );
            const auto s = ordinarySource( crossing, oblique, curved ); const auto bake = sample( s, data );
            SourceBudgetText original; String error; check( serializeCityRoadSplineSource( s, original, error ), error );
            const CityRoadHeightSampler height = [data]( const Vector3F &p, float &h ) { return render::sampleTerrainHeight( *data, {}, p, h ); };
            CityRoadJunctionBake connections;
            check( buildCityRoadJunctions( s, bake, height, {}, connections, error ), error );
            require( connections.junctions.size() == 1 && connections.junctions.front().ports.size() == ( crossing ? 4u : 3u ),
                     "Ordinary connection lost a merged through-road arm or its local approach" );
            require( connections.sourceRevision == s.revision && connections.roadRuns.size() == ( crossing ? 4u : 3u ),
                     "Ordinary connection did not retain every trimmed road run" );
            const auto connectionHash = junctionChecksum( connections );
            check( buildCityRoadJunctions( s, bake, height, {}, connections, error ), error );
            require( junctionChecksum( connections ) == connectionHash, "Repeated connection solving changed derived geometry" );
            if( variant == 0 )
            {
                auto malformed = bake;
                malformed.roads[1].points[1].position = malformed.roads[1].points.front().position;
                require( !buildCityRoadJunctions( s, malformed, height, {}, connections, error ) &&
                         junctionChecksum( connections ) == connectionHash,
                         "Zero-span junction sample escaped validation or replaced retained connectors" );
                malformed = bake;
                malformed.roads[0].points[malformed.roads[0].points.size() / 2].left.x =
                    std::numeric_limits<float>::quiet_NaN();
                require( !buildCityRoadJunctions( s, malformed, height, {}, connections, error ) &&
                         junctionChecksum( connections ) == connectionHash,
                         "Junction trimming hid a malformed interior road edge" );
            }
            CityRoadTerrainMesh mesh;
            check( buildCityRoadTerrainMesh( s, bake, data, {}, mesh, error ), error );
            agreement( mesh, data, {}, .025f );
            for( const auto &road : bake.roads ) for( const auto &p : road.points )
                require( surfaceCoverage( mesh, p.position.x, p.position.z ) > 0,
                         "Trimmed T/X connector left a gap along an authored carriageway" );
            for( double x = -13.173; x < 14; x += .83 ) for( double z = -13.271; z < 14; z += .91 )
                require( surfaceCoverage( mesh, x, z, true ) <= 1,
                         "Ordinary junction retains duplicate overlapping asphalt/collision triangles" );
            if( variant == 0 )
            {
                require( surfaceCoverage( mesh, 4.1, 6.1 ) > 0, "Unequal-width T junction still has an unrounded inner corner" );
                const auto retained = checksum( mesh );
                auto holes = std::make_shared<render::TerrainData>( *data ); holes->holes.assign( 64 * 64, 0 );
                holes->holes[38 * 64 + 36] = 1; // Inside the new corner, outside both original ribbons.
                require( !buildCityRoadTerrainMesh( s, bake, holes, {}, mesh, error ) && checksum( mesh ) == retained,
                         "Rounded junction covered a terrain hole or replaced retained geometry" );
                auto mismatch = s; mismatch.roads[1].heightOffset = .02f;
                require( !buildCityRoadTerrainMesh( mismatch, sample( mismatch, data ), data, {}, mesh, error ) &&
                         checksum( mesh ) == retained, "Unequal-level draped roads acquired a false connected surface" );
                auto obstructed = s;
                auto narrow = s.roads[1]; narrow.id.back() = '3'; narrow.width = .5f;
                narrow.knots.front().id.back() = '5'; narrow.knots.back().id.back() = '6';
                narrow.knots.front().position = { 4, 0, 6.5f }; narrow.knots.back().position = { 5, 0, 6.5f };
                narrow.knots.front().outHandle = { 1.f / 3, 0, 0 }; narrow.knots.back().inHandle = { -1.f / 3, 0, 0 };
                obstructed.roads.push_back( std::move( narrow ) );
                require( !buildCityRoadTerrainMesh( obstructed, sample( obstructed, data ), data, {}, mesh, error ) &&
                         checksum( mesh ) == retained,
                         "Rounded junction overlaps an unrelated road that cleared the original ribbons" );
            }
            SourceBudgetText after; check( serializeCityRoadSplineSource( s, after, error ), error );
            require( after == original, "Derived junction solving changed saved spline controls or identities" );
        }
    }
    void junctionBudgetDenial()
    {
        auto data = terrain(); const auto s = ordinarySource( true ); const auto bake = sample( s, data );
        const CityRoadHeightSampler height = [data]( const Vector3F &p, float &h ) { return render::sampleTerrainHeight( *data, {}, p, h ); };
        CityRoadJunctionBake result; String error;
        check( buildCityRoadJunctions( s, bake, height, {}, result, error ), error ); const auto retained = junctionChecksum( result );
        const auto scheduler = WorldStreamingScheduler::shared(); const auto stats = scheduler->stats();
        const auto index = size_t( StreamMemoryClass::SourceCpu );
        struct Restore { std::shared_ptr<WorldStreamingScheduler> scheduler; u64 limit;
            ~Restore() { String error; scheduler->setBudget( StreamMemoryClass::SourceCpu, limit, error ); }
        } restore{ scheduler, stats.limits[index] };
        unsigned denied = 0; bool admitted = false;
        for( u64 room = 0; room <= 2 * 1024 * 1024; room = room ? room * 2 : 1024 )
        {
            check( scheduler->setBudget( StreamMemoryClass::SourceCpu, stats.used[index] + room, error ), error );
            const bool ok = buildCityRoadJunctions( s, bake, height, {}, result, error );
            const auto used = scheduler->stats().used[index];
            check( scheduler->setBudget( StreamMemoryClass::SourceCpu, restore.limit, error ), error );
            require( used == stats.used[index] && junctionChecksum( result ) == retained,
                     "Junction allocation denial leaked source storage or replaced retained geometry" );
            if( ok ) { admitted = true; break; } ++denied;
        }
        require( admitted && denied >= 2, "Junction admission sweep missed staged allocation failures" );
    }
    void roundaboutJunctionConflicts()
    {
        auto data = terrain(); data->dimensions = { 129, 129 }; data->origin = { -64, -64 };
        data->heights.assign( 129 * 129, 0 );
        const CityRoadHeightSampler height = [data]( const Vector3F &p, float &h ) { return render::sampleTerrainHeight( *data, {}, p, h ); };
        const auto baseline = ordinarySource( false ); CityRoadJunctionBake result; String error;
        check( buildCityRoadJunctions( baseline, sample( baseline, data ), height, {}, result, error ), error );
        const auto retained = junctionChecksum( result );
        for( bool approach : { false, true } )
        {
            auto s = ordinarySource( false );
            if( approach )
            {
                // This connector stays outside the circular footprint but lies
                // across the east approach, before its retained source road.
                s.roads[0].width = 7;
                s.roads[0].knots.front().position = { 24, 0, -12 };
                s.roads[0].knots.back().position = { 24, 0, 12 };
                s.roads[1].knots.front().position = { 24, 0, 0 };
                s.roads[1].knots.back().position = { 12, 0, 0 };
                for( auto &road : s.roads )
                {
                    const auto tangent = ( road.knots.back().position - road.knots.front().position ) / 3.f;
                    road.knots.front().outHandle = tangent; road.knots.back().inHandle = -tangent;
                }
            }
            CityRoadRoundaboutSpec junction; junction.junctionId = "50000000-0000-4000-8000-000000000001";
            junction.center.y = .025f;
            const Vector3F directions[] = { { 1, 0, 0 }, { 0, 0, 1 }, { -1, 0, 0 } };
            constexpr char digits[] = "0123456789abcdef";
            for( unsigned i = 0; i < 3; ++i )
            {
                const unsigned index = i + 2;
                CityRoadSpline road; road.id = "30000000-0000-4000-8000-000000000001";
                road.id.back() = digits[index + 1]; road.width = 6; road.heightOffset = .025f;
                SourceBudgetText first = "40000000-0000-4000-8000-000000000001", second = first;
                first.back() = digits[index * 2 + 1]; second.back() = digits[index * 2 + 2];
                const auto a = directions[i] * 32.f, b = directions[i] * 48.f, tangent = ( b - a ) / 3.f;
                road.knots = { { first, a, {}, tangent }, { second, b, -tangent, {} } };
                junction.ports.push_back( { road.id, directions[i], 6, 18 } );
                s.roads.push_back( std::move( road ) );
            }
            s.roundabouts.push_back( std::move( junction ) );
            require( !buildCityRoadJunctions( s, sample( s, data ), height, {}, result, error ) &&
                     junctionChecksum( result ) == retained,
                     approach ? "Ordinary connector overlaps an explicit roundabout approach" :
                                "Ordinary connector puts drivable asphalt over a roundabout island" );
        }
    }
}
int main()
{
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    int result = 0;
    const auto run = []( const char *stage, auto test ) {
        try { std::cout << "BEGIN " << stage << '\n'; test(); std::cout << "PASS " << stage << '\n'; }
        catch( const std::exception &e ) { throw std::runtime_error( std::string( stage ) + ": " + e.what() ); }
    };
    try
    {
        run( "conforming and transformed terrain", conforming ); run( "bounded collinear runs", collinearRuns );
        run( "transactional invalid terrain/bake", failures );
        run( "roundabout footprint and approach seams", roundaboutSupport ); run( "staged SourceCpu denial", budgetDenial );
        run( "ordinary unequal-width T/X connectors", ordinaryJunctions );
        run( "junction SourceCpu denial", junctionBudgetDenial );
        run( "roundabout and ordinary junction conflicts", roundaboutJunctionConflicts );
        std::cout << "City road exact terrain mesh: support, ridges, holes, transforms, roundabouts, revisions and denial PASS\n";
    }
    catch( const std::exception &e ) { std::cerr << "City road terrain mesh: " << e.what() << '\n'; result = 1; }
    WorldStreamingScheduler::shutdownShared(); return result;
}
