#include <Workphone/Scene/Components/CityRoadFootways.hpp>
#include <Workphone/Scene/Components/CityRoadJunctions.hpp>
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <Workphone/Interface/Procedural/OpenCityRoadSource.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
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
    std::shared_ptr<render::TerrainData> terrain( int count = 129, float spacing = 1 )
    {
        auto data = std::make_shared<render::TerrainData>(); data->dimensions = { count, count };
        data->spacing = { spacing, spacing }; data->origin = { -( count - 1 ) * spacing * .5f, -( count - 1 ) * spacing * .5f };
        data->heights.assign( size_t( count ) * count, 0 ); data->revision = 29; return data;
    }
    CityRoadSplineSource source()
    {
        CityRoadSplineSource s; s.version = 2; s.id = "10000000-0000-4000-8000-000000000001";
        s.terrainResourceUuid = "20000000-0000-4000-8000-000000000001"; s.revision = 7; return s;
    }
    void addRoad( CityRoadSplineSource &s, unsigned index, Vector3F a, Vector3F b )
    {
        CityRoadSpline road; road.id = "30000000-0000-4000-8000-000000000001"; road.id.back() = char( '1' + index );
        road.heightOffset = .025f;
        SourceBudgetText first = "40000000-0000-4000-8000-000000000001", second = first;
        first.back() = char( '1' + index * 2 ); second.back() = char( '2' + index * 2 );
        const auto direction = ( b - a ) / 3.f; road.knots = { { first, a, {}, direction }, { second, b, -direction, {} } };
        s.roads.push_back( std::move( road ) );
    }
    CityRoadSplineBake sample( const CityRoadSplineSource &s, render::TerrainSnapshot data, const Transform3<real_Num> &placement = {} )
    {
        CityRoadSplineBake bake; String error;
        check( sampleCityRoadSplines( s, {}, [data, placement]( const Vector3F &p, float &height ) {
            return render::sampleTerrainHeight( *data, placement, p, height );
        }, bake, error ), error ); return bake;
    }
    bool topAt( const CityRoadTerrainMesh &mesh, double x, double z )
    {
        for( size_t i = 0; i < mesh.indices.size(); i += 3 )
        {
            const auto &va = mesh.vertices[mesh.indices[i]], &vb = mesh.vertices[mesh.indices[i + 1]], &vc = mesh.vertices[mesh.indices[i + 2]];
            if( va.normal.y < .5f || vb.normal.y < .5f || vc.normal.y < .5f ) continue;
            const auto a = va.position, b = vb.position, c = vc.position;
            const double denominator = ( b.z - c.z ) * ( a.x - c.x ) + ( c.x - b.x ) * ( a.z - c.z );
            if( std::abs( denominator ) < 1e-10 ) continue;
            const double u = ( ( b.z - c.z ) * ( x - c.x ) + ( c.x - b.x ) * ( z - c.z ) ) / denominator;
            const double v = ( ( c.z - a.z ) * ( x - c.x ) + ( a.x - c.x ) * ( z - c.z ) ) / denominator;
            if( u >= -1e-6 && v >= -1e-6 && u + v <= 1.000001 ) return true;
        }
        return false;
    }
    u64 checksum( const CityRoadTerrainMesh &mesh )
    {
        u64 value = 1469598103934665603ull;
        const auto fold = [&]( u64 input ) { value = ( value ^ input ) * 1099511628211ull; };
        const auto number = [&]( float input ) { u32 bits; std::memcpy( &bits, &input, sizeof( bits ) ); fold( bits ); };
        fold( mesh.sourceRevision ); fold( mesh.terrainRevision );
        for( const auto &v : mesh.vertices )
        { number( v.position.x ); number( v.position.y ); number( v.position.z ); number( v.normal.x ); number( v.normal.y ); number( v.normal.z ); number( v.uv.x ); number( v.uv.y ); }
        for( auto index : mesh.indices ) fold( index ); for( auto kind : mesh.triangleKinds ) fold( u32( kind ) ); return value;
    }
    void agreement( const CityRoadTerrainMesh &mesh, render::TerrainSnapshot data, const Transform3<real_Num> &placement = {} )
    {
        require( !mesh.indices.empty() && mesh.vertices.size() <= 300000 && mesh.indices.size() % 3 == 0 &&
            mesh.triangleKinds.size() == mesh.indices.size() / 3, "Footway topology/budget is invalid" );
        require( mesh.sourceRevision == 7 && mesh.terrainRevision == 29, "Footway revision identity is stale" );
        size_t top = 0, skirts = 0;
        for( const auto &v : mesh.vertices )
        {
            float y = 0; require( render::sampleTerrainHeight( *data, placement, v.position, y ), "Footway extends beyond retained terrain" );
            require( std::isfinite( v.uv.x ) && std::isfinite( v.uv.y ) && std::abs( v.normal.length() - 1 ) < .001f,
                     "Footway normal/UV is nonfinite or unnormalized" );
            if( v.normal.y > .5f ) { ++top; require( std::abs( v.position.y - y - .15f ) < .0002f, "Footway top is not terrain-exact+.15m" ); }
            else { ++skirts; require( std::abs( v.normal.y ) < .0001f &&
                ( std::abs( v.position.y - y ) < .0002f || std::abs( v.position.y - y - .15f ) < .0002f ), "Footway skirt floats or leaves a terrain gap" ); }
        }
        for( size_t i = 0; i < mesh.indices.size(); i += 3 )
        {
            require( mesh.triangleKinds[i / 3] == CityRoadTerrainTriangleKind::Kerb, "Footway has a drivable/island triangle kind" );
            const auto &a = mesh.vertices.at( mesh.indices[i] ), &b = mesh.vertices.at( mesh.indices[i + 1] ), &c = mesh.vertices.at( mesh.indices[i + 2] );
            const auto normal = ( b.position - a.position ).crossProduct( c.position - a.position );
            require( normal.dotProduct( a.normal ) > 1e-9f, "Footway winding is downward, inward or degenerate" );
            if( a.normal.y > .5f )
            {
                const auto center = ( a.position + b.position + c.position ) / 3.f; float y = 0;
                require( render::sampleTerrainHeight( *data, placement, center, y ) && std::abs( center.y - y - .15f ) < .0003f,
                         "Footway triangle cuts across a terrain-cell ridge" );
            }
        }
        require( top && skirts, "Footway omitted its top or terrain skirts" );
    }
    void streetsAndTerrain()
    {
        auto s = source(); addRoad( s, 0, { 0, 0, -24 }, { 0, 0, 24 } ); addRoad( s, 1, { -24, 0, 0 }, { 24, 0, 0 } );
        auto data = terrain(); const auto bake = sample( s, data ); CityRoadTerrainMesh mesh; String error;
        check( buildCityRoadFootways( s, bake, data, {}, mesh, error ), error ); agreement( mesh, data );
        require( topAt( mesh, 7, 12 ) && topAt( mesh, 7, 16 ) && topAt( mesh, 7, 20 ), "Adjacent four-metre footways have alternating gaps" );
        for( double z : { -5., 0., 5. } ) require( !topAt( mesh, 7, z ) && !topAt( mesh, z, 7 ), "Footway blocks an ordinary intersection mouth" );
        require( !topAt( mesh, 0, 0 ) && !topAt( mesh, 5.9, 14 ) && !topAt( mesh, 8.5, 14 ), "Footway width/gap intrudes into road or parcel reservation" );
        const auto original = checksum( mesh ); check( buildCityRoadFootways( s, bake, data, {}, mesh, error ), error );
        require( checksum( mesh ) == original, "Repeated footway generation is nondeterministic" );
        auto holes = terrain(); holes->holes.assign( 128 * 128, 0 ); holes->holes[77 * 128 + 71] = 1;
        check( buildCityRoadFootways( s, bake, holes, {}, mesh, error ), error );
        require( !topAt( mesh, 7.5, 14 ) && topAt( mesh, 7.5, 18 ), "Interior hole did not skip its whole four-metre piece, or removed good neighbours" );
        auto ridge = terrain(); ridge->heights[77 * 129 + 71] = .04f;
        check( buildCityRoadFootways( s, bake, ridge, {}, mesh, error ), error ); agreement( mesh, ridge );
        float highest = 0; for( const auto &v : mesh.vertices ) highest = std::max( highest, v.position.y );
        require( highest > .189f, "Exact sidewalk support missed an interior terrain spike" );
        ridge->heights[77 * 129 + 71] = .8f;
        check( buildCityRoadFootways( s, bake, ridge, {}, mesh, error ), error );
        require( !topAt( mesh, 7.5, 14 ) && topAt( mesh, 7.5, 18 ), "Steep interior terrain grade was flattened or retained" );

        Transform3<real_Num> placement; placement.setPosition( { 5, 3, -4 } ); placement.setScale( { 1.4f, 1.8f, .9f } );
        placement.setOrientation( { std::cos( .155f ), 0, std::sin( .155f ), 0 } );
        auto transformed = terrain(); for( int z = 0; z < 129; ++z ) for( int x = 0; x < 129; ++x )
            transformed->heights[z * 129 + x] = .005f * ( x - 64 ) + .003f * ( z - 64 );
        check( buildCityRoadFootways( s, sample( s, transformed, placement ), transformed, placement, mesh, error ), error );
        agreement( mesh, transformed, placement );
    }
    void roundabout()
    {
        auto s = source(); CityRoadRoundaboutSpec junction; junction.junctionId = "50000000-0000-4000-8000-000000000001";
        junction.center.y = .025f;
        const Vector3F directions[] = { { 1, 0, 0 }, { 0, 0, 1 }, { -1, 0, 0 }, { 0, 0, -1 } };
        for( unsigned i = 0; i < 4; ++i )
        { addRoad( s, i, directions[i] * 32.f, directions[i] * 60.f ); junction.ports.push_back( { s.roads.back().id, directions[i], 12, 18 } ); }
        s.roundabouts.push_back( junction ); auto data = terrain(); const auto bake = sample( s, data );
        CityRoadTerrainMesh road, mesh; String error;
        check( buildCityRoadTerrainMesh( s, bake, data, {}, road, error ), error );
        check( buildCityRoadFootways( s, bake, data, {}, mesh, error ), error ); agreement( mesh, data );
        const double diagonal = 15.5 / std::sqrt( 2. );
        for( int x : { -1, 1 } ) for( int z : { -1, 1 } ) require( topAt( mesh, diagonal * x, diagonal * z ), "Roundabout ring footway is absent between its four mouths" );
        for( const auto &direction : directions ) for( float radius : { 14.2f, 15.5f, 18.f, 25.f, 32.f } )
            require( !topAt( mesh, direction.x * radius, direction.z * radius ), "Annular sidewalk seals a roundabout approach mouth" );
        require( topAt( mesh, 24, 7 ) && topAt( mesh, 7, -24 ), "Roundabout port-side footways are missing" );
        for( const auto &direction : directions ) for( int side : { -1, 1 } )
        {
            const Vector3F across{ -direction.z, 0, direction.x };
            const auto innerJoin = direction * float( std::sqrt( 14.05 * 14.05 - 6.05 * 6.05 ) ) + across * float( side * 6.05 );
            const auto outerJoin = direction * float( std::sqrt( 16.45 * 16.45 - 8.45 * 8.45 ) ) + across * float( side * 8.45 );
            const auto join = ( innerJoin + outerJoin ) * .5f;
            const auto diagonalPoint = ( direction + across * float( side ) ) * float( diagonal );
            const auto straightPoint = direction * 24.f + across * float( side * 7.25 );
            for( const auto &leg : { std::make_pair( diagonalPoint, join ), std::make_pair( join, straightPoint ) } )
                for( unsigned station = 0; station <= 256; ++station )
                {
                    const auto p = leg.first + ( leg.second - leg.first ) * ( station / 256.f );
                    require( topAt( mesh, p.x, p.z ), "A roundabout annulus/approach pedestrian corridor is disconnected" );
                }
            for( size_t i = 0; i < mesh.indices.size(); i += 3 )
            {
                const auto &a = mesh.vertices[mesh.indices[i]], &b = mesh.vertices[mesh.indices[i + 1]], &c = mesh.vertices[mesh.indices[i + 2]];
                if( a.normal.y > .5f ) continue;
                const auto center = ( a.position + b.position + c.position ) / 3.f;
                require( std::hypot( center.x - join.x, center.z - join.z ) > .7f,
                    "An internal skirt blocks a connected roundabout pedestrian join" );
            }
        }
        for( size_t i = 0; i < mesh.indices.size(); i += 3 )
        {
            const auto &a = mesh.vertices[mesh.indices[i]], &b = mesh.vertices[mesh.indices[i + 1]], &c = mesh.vertices[mesh.indices[i + 2]];
            if( a.normal.y < .5f ) continue;
            const auto center = ( a.position + b.position + c.position ) / 3.f;
            require( std::hypot( center.x, center.z ) > 14, "Footway intersects circulation or central island" );
        }
    }
    void tightEditableCurve()
    {
        // Radius8.2 admits the codec's adaptive centre/edge sampling, while the
        // inner sidewalk's outer offset8.45 still crosses the curvature centre.
        auto s = source(); addRoad( s, 0, { 8.2f, 0, 0 }, { 0, 0, 8.2f } );
        auto &road = s.roads[0]; road.knots[0].outHandle = { 0, 0, 4.528735f };
        road.knots[1].inHandle = { 4.528735f, 0, 0 };
        auto data = terrain(); const auto bake = sample( s, data ); CityRoadTerrainMesh roadMesh, footways; String error;
        check( buildCityRoadTerrainMesh( s, bake, data, {}, roadMesh, error ), error );
        check( buildCityRoadFootways( s, bake, data, {}, footways, error ), error ); agreement( footways, data );
        require( topAt( footways, 11, 11 ) && !topAt( footways, 0, 0 ),
            "A tight valid road lost its outer sidewalk or emitted its folded inner sidewalk" );
    }
    void ordinaryJunctionWalks()
    {
        for( unsigned variant = 0; variant < 4; ++variant )
        {
            const bool crossing = ( variant & 1 ) != 0, translated = variant >= 2;
            const Vector3F origin = translated ? Vector3F( -440, 0, -176 ) : Vector3F();
            std::cout << "ordinary footway " << ( crossing ? "X" : "T" ) << " junction translated=" << translated << '\n';
            auto s = source(); addRoad( s, 0, origin + Vector3F( -24, 0, 0 ), origin + Vector3F( 24, 0, 0 ) );
            addRoad( s, 1, origin + ( crossing ? Vector3F( 0, 0, -24 ) : Vector3F() ), origin + Vector3F( 0, 0, 24 ) );
            s.roads[1].width = translated ? 12 : 7;
            auto data = translated ? terrain( 321, 4 ) : terrain(); const auto bake = sample( s, data ); String error;
            SourceBudgetText original; check( serializeCityRoadSplineSource( s, original, error ), error );
            const CityRoadHeightSampler height = [data]( const Vector3F &p, float &h ) { return render::sampleTerrainHeight( *data, {}, p, h ); };
            CityRoadJunctionBake connections;
            check( buildCityRoadJunctions( s, bake, height, {}, connections, error ), error );
            require( connections.junctions.size() == 1, "Footway fixture lost its ordinary junction" );
            CityRoadTerrainMesh roads, walks;
            check( buildCityRoadTerrainMesh( s, bake, data, {}, roads, error ), error );
            check( buildCityRoadFootways( s, bake, data, {}, walks, error ), error ); agreement( walks, data );
            const auto retained = checksum( walks );
            check( buildCityRoadFootways( s, bake, data, {}, walks, error ), error );
            require( checksum( walks ) == retained, "Ordinary junction footways are not deterministic" );
            SourceBudgetVector<Vector3F> joins, exposedEdges;
            unsigned inspected = 0;
            for( const auto &corner : connections.junctions.front().corners )
            {
                require( corner.points.size() >= 2 && corner.points.size() == corner.outward.size(),
                         "Ordinary junction corner has no shared sidewalk boundary" );
                for( size_t i = 0; i + 1 < corner.points.size(); ++i )
                {
                    for( float offset : { .05f, 2.45f } )
                        exposedEdges.push_back( ( corner.points[i] + corner.points[i + 1] ) * .5f +
                            ( corner.outward[i] + corner.outward[i + 1] ) * ( .5f * offset ) );
                    for( float t : { 0.f, .5f, 1.f } )
                    {
                        const auto p = corner.points[i] * ( 1 - t ) + corner.points[i + 1] * t +
                            ( corner.outward[i] * ( 1 - t ) + corner.outward[i + 1] * t ) * 1.25f;
                        require( topAt( walks, p.x, p.z ) && !topAt( roads, p.x, p.z ),
                                 "Rounded junction sidewalk has a gap or intrudes into its carriageway" );
                        joins.push_back( p ); // Cell-diagonal skirts must not block the walking arc either.
                        ++inspected;
                    }
                }
                joins.push_back( corner.points.front() + corner.outward.front() * 1.25f );
                joins.push_back( corner.points.back() + corner.outward.back() * 1.25f );
            }
            require( inspected > 8, "Ordinary junction emitted no useful connected pedestrian corners" );
            for( const auto &road : bake.roads ) for( const auto &p : road.points )
                require( !topAt( walks, p.position.x, p.position.z ), "Ordinary sidewalk seals a T/X approach mouth" );
            for( size_t i = 0; i < walks.indices.size(); i += 3 )
            {
                const auto &a = walks.vertices[walks.indices[i]], &b = walks.vertices[walks.indices[i + 1]],
                           &c = walks.vertices[walks.indices[i + 2]];
                const auto center = ( a.position + b.position + c.position ) / 3.f;
                if( a.normal.y > .5f )
                    require( !topAt( roads, center.x, center.z ), "Junction sidewalk top overlaps asphalt" );
                else for( const auto &join : joins )
                {
                    const auto distance = [&]( Vector3F p, Vector3F q ) {
                        const float dx = q.x - p.x, dz = q.z - p.z, squared = dx * dx + dz * dz;
                        const float t = squared > 1e-10f ? std::clamp( ( ( join.x - p.x ) * dx + ( join.z - p.z ) * dz ) / squared, 0.f, 1.f ) : 0;
                        return std::hypot( join.x - p.x - t * dx, join.z - p.z - t * dz );
                    };
                    if( std::min( { distance( a.position, b.position ), distance( b.position, c.position ),
                                    distance( c.position, a.position ) } ) <= .2f )
                        throw std::runtime_error( "An internal kerb blocks the junction pedestrian join (" +
                            std::to_string( join.x ) + "," + std::to_string( join.z ) + "); triangle=(" +
                            std::to_string( a.position.x ) + "," + std::to_string( a.position.z ) + ")-(" +
                            std::to_string( b.position.x ) + "," + std::to_string( b.position.z ) + ")-(" +
                            std::to_string( c.position.x ) + "," + std::to_string( c.position.z ) + ")" );
                }
            }
            // Removing all skirts would hide the blocking wall while leaving
            // visibly open pavement sides. Both real outlines must remain sealed.
            for( const auto &edge : exposedEdges )
            {
                double nearest = std::numeric_limits<double>::max();
                for( size_t i = 0; i < walks.indices.size(); i += 3 )
                {
                    if( walks.vertices[walks.indices[i]].normal.y > .5f ) continue;
                    for( size_t j = 0; j < 3; ++j )
                    {
                        const auto a = walks.vertices[walks.indices[i + j]].position,
                                   b = walks.vertices[walks.indices[i + ( j + 1 ) % 3]].position;
                        const double dx = double( b.x ) - a.x, dz = double( b.z ) - a.z,
                                     squared = dx * dx + dz * dz;
                        const double t = squared > 1e-10 ? std::clamp( ( ( edge.x - a.x ) * dx +
                            ( edge.z - a.z ) * dz ) / squared, 0., 1. ) : 0;
                        nearest = std::min( nearest, std::hypot( edge.x - a.x - t * dx, edge.z - a.z - t * dz ) );
                    }
                }
                require( nearest < .002, "Rounded pavement lost a real inner or outer kerb" );
            }
            SourceBudgetText after; check( serializeCityRoadSplineSource( s, after, error ), error );
            require( after == original, "Building junction footways changed the editable source" );
        }
    }
    void coalescedStraightAdmission()
    {
        auto data = terrain( 65, 4 ); auto s = source(); addRoad( s, 0, { 0, 0, -120 }, { 0, 0, 120 } );
        CityRoadTerrainMesh mesh, dense; String error;
        check( buildCityRoadFootways( s, sample( s, data ), data, {}, mesh, error ), error ); agreement( mesh, data );
        auto unmerged = source(); CityRoadSplineBake unmergedBake; unmergedBake.sourceRevision = unmerged.revision;
        const auto id = []( u32 value ) {
            SourceBudgetText result = "91000000-0000-4000-8000-000000000000";
            constexpr char hex[] = "0123456789abcdef";
            for( size_t i = 0; i < 8; ++i ) { result[result.size() - 1 - i] = hex[value & 15]; value >>= 4; }
            return result;
        };
        for( int side : { -1, 1 } ) for( unsigned piece = 0; piece < 60; ++piece )
        {
            const auto ordinal = u32( unmerged.roads.size() ); CityRoadSpline road; CityRoadSplineSamples samples;
            road.id = id( ordinal * 3 + 1 ); road.width = 2.4f;
            samples.roadId = road.id; samples.points.resize( 2 ); road.knots.resize( 2 );
            for( unsigned end = 0; end < 2; ++end )
            {
                auto &p = samples.points[end]; const float z = -120.f + ( piece + end ) * 4.f;
                p.position = { side * 7.25f, 0, z }; p.left = { side * 6.05f, 0, z }; p.right = { side * 8.45f, 0, z };
                p.distance = end * 4.f; road.knots[end].position = p.position; road.knots[end].id = id( ordinal * 3 + 2 + end );
            }
            unmerged.roads.push_back( std::move( road ) ); unmergedBake.roads.push_back( std::move( samples ) );
        }
        check( buildCityRoadTerrainMesh( unmerged, unmergedBake, data, {}, dense, error, false ), error );
        size_t topTriangles = 0, skirtTriangles = 0, skirtVertices = 0;
        for( size_t i = 0; i < mesh.indices.size(); i += 3 )
            if( mesh.vertices[mesh.indices[i]].normal.y > .5f ) ++topTriangles; else ++skirtTriangles;
        for( const auto &vertex : mesh.vertices ) if( vertex.normal.y < .5f ) ++skirtVertices;
        std::cout << "Footway straight spans: topTriangles=" << topTriangles << " dense4m=" << dense.triangleKinds.size()
            << " skirtVertices=" << skirtVertices << " boundaryEdges=" << skirtTriangles / 2 << '\n';
        // Two long rectangular walks have eight hard corners. All intermediate
        // coplanar boundary endpoints should be shared, including terrain cuts.
        require( skirtVertices <= skirtTriangles + 32,
            "Straight footway skirts duplicate their shared terrain-cell endpoints" );
        for( float z = -118; z <= 118; z += 4 ) require( topAt( mesh, 7, z ) && topAt( mesh, -7, z ),
            "Straight span coalescing introduced a hole in the original admitted4m pieces" );
    }
    void obliqueBatchBoundary()
    {
        auto data = terrain( 321, 4 ); auto s = source();
        // 400m gives more than128 admitted4m pieces across both footways. The
        // shifted/oblique footprint exercises F32 precision and cell crossings.
        addRoad( s, 0, { 240, 0, 130 }, { 560, 0, 370 } );
        CityRoadTerrainMesh mesh; String error;
        check( buildCityRoadFootways( s, sample( s, data ), data, {}, mesh, error ), error ); agreement( mesh, data );
        size_t skirtTriangles = 0, skirtVertices = 0;
        for( const auto &vertex : mesh.vertices ) if( vertex.normal.y < .5f ) ++skirtVertices;
        for( size_t i = 0; i < mesh.indices.size(); i += 3 )
        {
            const auto &a = mesh.vertices[mesh.indices[i]], &b = mesh.vertices[mesh.indices[i + 1]], &c = mesh.vertices[mesh.indices[i + 2]];
            if( a.normal.y > .5f ) continue; ++skirtTriangles;
            const auto center = ( a.position + b.position + c.position ) / 3.f;
            const double x = double( center.x ) - 400, z = double( center.z ) - 250;
            const double along = x * .8 + z * .6, across = x * .6 - z * .8;
            const double perimeter = std::min( { std::abs( std::abs( across ) - 6.05 ), std::abs( std::abs( across ) - 8.45 ),
                                                std::abs( std::abs( along ) - 200 ) } );
            if( perimeter > .001 ) throw std::runtime_error( "False internal skirt at along=" + std::to_string( along ) +
                " across=" + std::to_string( across ) + " distance=" + std::to_string( perimeter ) );
        }
        require( skirtVertices <= skirtTriangles + 32, "Oblique footway skirts fail to share coplanar boundary endpoints" );
        for( int side : { -1, 1 } ) for( double along = -196; along <= 196; along += 8 )
            require( topAt( mesh, 400 + along * .8 + side * 7.25 * .6, 250 + along * .6 - side * 7.25 * .8 ),
                     "An oblique batch boundary introduced a gap in an admitted sidewalk" );
        std::cout << "Footway oblique batches: vertices=" << mesh.vertices.size() << " skirtVertices=" << skirtVertices
            << " boundaryEdges=" << skirtTriangles / 2 << '\n';
    }
    void generatedProfiles()
    {
        auto data = terrain( 321, 4 ); const CityRoadHeightSampler plane = []( const Vector3F &, float &height ) { height = 0; return true; };
        for( bool natural : { false, true } ) for( u32 seed : { 0u, 7u, 42u, 0xffffffffu } )
        {
            std::cout << "BEGIN generated footways " << ( natural ? "natural" : "grid" ) << " blocks=10 seed=" << seed << '\n';
            const auto city = OpenCityLayout::generate( seed, 10, 0, natural ); OpenCityRoadSourceOptions options;
            options.sourceId = "60000000-0000-4000-8000-000000000001"; options.terrainResourceUuid = "70000000-0000-4000-8000-000000000001";
            options.seed = seed; options.revision = 7; options.maximumRoundabouts = 4;
            OpenCityRoadSourceResult converted; String error; check( buildOpenCityRoadSource( city, options, plane, converted, error ), error );
            const auto bake = sample( converted.source, data ); CityRoadTerrainMesh mesh;
            check( buildCityRoadFootways( converted.source, bake, data, {}, mesh, error ), error ); agreement( mesh, data );
            size_t topVertices = 0, skirtTriangles = 0;
            for( const auto &vertex : mesh.vertices ) if( vertex.normal.y > .5f ) ++topVertices;
            for( size_t i = 0; i < mesh.indices.size(); i += 3 ) if( mesh.vertices[mesh.indices[i]].normal.y < .5f ) ++skirtTriangles;
            std::cout << "Footways " << ( natural ? "natural" : "grid" ) << "10 seed=" << seed << " vertices=" << mesh.vertices.size()
                << " triangles=" << mesh.indices.size() / 3 << " topVertices=" << topVertices
                << " skirtVertices=" << mesh.vertices.size() - topVertices << " boundaryEdges=" << skirtTriangles / 2 << '\n';
        }
    }
    void failuresAndBudget()
    {
        auto s = source(); addRoad( s, 0, { 0, 0, -12 }, { 0, 0, 12 } ); auto data = terrain(); const auto bake = sample( s, data );
        CityRoadTerrainMesh mesh; String error; check( buildCityRoadFootways( s, bake, data, {}, mesh, error ), error ); const auto original = checksum( mesh );
        auto stale = bake; ++stale.sourceRevision;
        require( !buildCityRoadFootways( s, stale, data, {}, mesh, error ) && checksum( mesh ) == original, "Stale bake replaced last-good footways" );
        auto poison = bake; poison.roads[0].points[1].left.z = std::numeric_limits<float>::quiet_NaN();
        require( !buildCityRoadFootways( s, poison, data, {}, mesh, error ) && checksum( mesh ) == original, "Malformed bake replaced last-good footways" );
        require( !buildCityRoadFootways( s, bake, {}, {}, mesh, error ) && checksum( mesh ) == original, "Absent terrain erased last-good footways" );
        {
            auto allHoles = terrain( 1025 ); allHoles->holes.assign( 1024 * 1024, 1 );
            require( !buildCityRoadFootways( s, bake, allHoles, {}, mesh, error ) &&
                error.find( "work budget" ) != String::npos && checksum( mesh ) == original,
                "Recursive all-hole fitting hid unbounded full-terrain validation work or erased retained output" );
        }
        auto scheduler = WorldStreamingScheduler::shared(); const auto stats = scheduler->stats(); const auto index = size_t( StreamMemoryClass::SourceCpu );
        struct Restore { std::shared_ptr<WorldStreamingScheduler> scheduler; u64 limit; ~Restore() { String error; scheduler->setBudget( StreamMemoryClass::SourceCpu, limit, error ); } } restore{ scheduler, stats.limits[index] };
        bool success = false; unsigned failures = 0;
        for( u64 headroom = 0; headroom <= 4 * 1024 * 1024; headroom = headroom ? headroom * 2 : 2048 )
        {
            check( scheduler->setBudget( StreamMemoryClass::SourceCpu, stats.used[index] + headroom, error ), error );
            const bool accepted = buildCityRoadFootways( s, bake, data, {}, mesh, error );
            check( scheduler->setBudget( StreamMemoryClass::SourceCpu, restore.limit, error ), error );
            require( scheduler->stats().used[index] == stats.used[index], "Footway denial leaked SourceCpu storage" );
            require( checksum( mesh ) == original, "Partial footway allocation changed retained output" );
            if( accepted ) { success = true; break; } ++failures;
        }
        require( success && failures >= 3, "Footway staged allocation test did not cover denial through success" );
        auto empty = source(); CityRoadSplineBake emptyBake; emptyBake.sourceRevision = empty.revision;
        check( buildCityRoadFootways( empty, emptyBake, data, {}, mesh, error ), error ); require( mesh.vertices.empty(), "Empty canonical source retained stale footways" );
    }
}
int main()
{
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    int result = 0;
    const auto run = []( const char *name, auto test ) {
        try { std::cout << "BEGIN " << name << '\n'; test(); std::cout << "PASS " << name << '\n'; }
        catch( const std::exception &e ) { throw std::runtime_error( std::string( name ) + ": " + e.what() ); }
    };
    try
    {
        run( "streets and exact terrain", streetsAndTerrain ); run( "four-port roundabout", roundabout );
        run( "tight editable curve", tightEditableCurve );
        run( "ordinary unequal-width T/X pedestrian joins", ordinaryJunctionWalks );
        run( "coalesced straight admission", coalescedStraightAdmission );
        run( "oblique batch boundary", obliqueBatchBoundary );
        run( "generated maximum profiles", generatedProfiles ); run( "transactional denial", failuresAndBudget );
        std::cout << "City road footways: exact support, open mouths, annulus, skirts and budgets PASS\n";
    }
    catch( const std::exception &e ) { std::cerr << "City road footways: " << e.what() << '\n'; result = 1; }
    WorldStreamingScheduler::shutdownShared(); return result;
}
