#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/CityRoadRoundabout.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace workphone::procedural
{
    namespace
    {
        constexpr float pi = 3.14159265358979323846f, tau = 2 * pi;
        bool finite( Vector3F p ) { return std::isfinite( p.x ) && std::isfinite( p.y ) && std::isfinite( p.z ); }
        bool uuid( const SourceBudgetText &id )
        {
            if( id.size() != 36 ) return false;
            bool nonzero = false;
            for( std::size_t i = 0; i < id.size(); ++i )
            {
                if( i == 8 || i == 13 || i == 18 || i == 23 ) { if( id[i] != '-' ) return false; continue; }
                const char c = id[i];
                if( !( ( c >= '0' && c <= '9' ) || ( c >= 'a' && c <= 'f' ) || ( c >= 'A' && c <= 'F' ) ) ) return false;
                nonzero |= c != '0';
            }
            return nonzero;
        }
        bool sameUuid( const SourceBudgetText &a, const SourceBudgetText &b )
        {
            if( a.size() != b.size() ) return false;
            for( std::size_t i = 0; i < a.size(); ++i )
            {
                const auto lower = []( char c ) { return c >= 'A' && c <= 'F' ? char( c + 'a' - 'A' ) : c; };
                if( lower( a[i] ) != lower( b[i] ) ) return false;
            }
            return true;
        }
        void need( bool valid, const char *message ) { if( !valid ) throw std::runtime_error( message ); }
        float wrap( float angle ) { angle = std::fmod( angle, tau ); return angle < 0 ? angle + tau : angle; }
        float difference( float a, float b )
        { float d = wrap( a - b ); return d > pi ? d - tau : d; }
        float cross2( float ax, float az, float bx, float bz ) { return ax * bz - az * bx; }
        bool triangleContains( float x, float z, Vector3F a, Vector3F b, Vector3F c )
        {
            const float aa = cross2( b.x - a.x, b.z - a.z, x - a.x, z - a.z );
            const float bb = cross2( c.x - b.x, c.z - b.z, x - b.x, z - b.z );
            const float cc = cross2( a.x - c.x, a.z - c.z, x - c.x, z - c.z );
            return ( aa >= -.0001f && bb >= -.0001f && cc >= -.0001f ) ||
                   ( aa <= .0001f && bb <= .0001f && cc <= .0001f );
        }
    }

    void CityRoadRoundaboutBake::swap( CityRoadRoundaboutBake &other ) noexcept
    {
        using std::swap;
        junctionId.swap( other.junctionId ); swap( center, other.center );
        swap( outerRadius, other.outerRadius ); swap( islandRadius, other.islandRadius );
        vertices.swap( other.vertices ); indices.swap( other.indices ); triangleKinds.swap( other.triangleKinds );
        connections.swap( other.connections ); routes.swap( other.routes );
    }

    bool CityRoadRoundaboutBake::containsSurface( f32 x, f32 z ) const
    {
        if( !std::isfinite( x ) || !std::isfinite( z ) || indices.size() != triangleKinds.size() * 3 ) return false;
        // Raised refuges intentionally share the underlying approach pavement.
        // Their actual top footprints take precedence over that road union.
        // Vertical kerb faces have zero projected area and cannot mask whole lines.
        for( std::size_t i = 0; i < triangleKinds.size(); ++i )
        {
            if( triangleKinds[i] != CityRoadRoundaboutSurface::Island &&
                triangleKinds[i] != CityRoadRoundaboutSurface::Kerb ) continue;
            const auto a = indices[i * 3], b = indices[i * 3 + 1], c = indices[i * 3 + 2];
            if( a >= vertices.size() || b >= vertices.size() || c >= vertices.size() ) return false;
            const auto aa = vertices[a].position, bb = vertices[b].position, cc = vertices[c].position;
            if( std::abs( cross2( bb.x - aa.x, bb.z - aa.z, cc.x - aa.x, cc.z - aa.z ) ) <= 1.e-8f ) continue;
            if( triangleContains( x, z, aa, bb, cc ) ) return false;
        }
        for( std::size_t i = 0; i < triangleKinds.size(); ++i )
        {
            if( triangleKinds[i] != CityRoadRoundaboutSurface::Circulation &&
                triangleKinds[i] != CityRoadRoundaboutSurface::Approach ) continue;
            const auto a = indices[i * 3], b = indices[i * 3 + 1], c = indices[i * 3 + 2];
            if( a >= vertices.size() || b >= vertices.size() || c >= vertices.size() ) return false;
            if( triangleContains( x, z, vertices[a].position, vertices[b].position, vertices[c].position ) ) return true;
        }
        return false;
    }

    bool bakeCityRoadRoundabout( const CityRoadRoundaboutSpec &spec,
        const CityRoadRoundaboutHeightSampler &height, CityRoadRoundaboutBake &output, String &error )
    {
        try
        {
            need( uuid( spec.junctionId ), "Roundabout junction requires a stable nonzero UUID" );
            need( finite( spec.center ) && std::isfinite( spec.outerRadius ) && std::isfinite( spec.islandRadius ) &&
                spec.outerRadius >= 6 && spec.outerRadius <= 128 && spec.islandRadius >= 1 &&
                spec.outerRadius - spec.islandRadius >= 3 && spec.outerRadius - spec.islandRadius <= 7,
                "Roundabout requires explicit island/outer radii and one 3..7m circulating lane" );
            need( std::isfinite( spec.islandHeight ) && spec.islandHeight >= .05f && spec.islandHeight <= .4f &&
                std::isfinite( spec.kerbWidth ) && spec.kerbWidth >= .1f && spec.kerbWidth <= .5f &&
                spec.kerbWidth < spec.islandRadius * .5f, "Roundabout raised island/kerb dimensions are invalid" );
            need( spec.ports.size() >= 3 && spec.ports.size() <= 6 && spec.ringSamples >= 32 && spec.ringSamples <= 256,
                "Roundabout supports 3..6 ports and 32..256 ring samples" );
            need( std::isfinite( spec.gradeX ) && std::isfinite( spec.gradeZ ) && std::isfinite( spec.maxGrade ) &&
                spec.maxGrade > 0 && spec.maxGrade <= .15f && std::hypot( spec.gradeX, spec.gradeZ ) <= spec.maxGrade &&
                std::isfinite( spec.maxTerrainDeviation ) && spec.maxTerrainDeviation >= 0 && spec.maxTerrainDeviation <= 2 &&
                std::isfinite( spec.terrainSampleStep ) && spec.terrainSampleStep >= .25f && spec.terrainSampleStep <= 4 &&
                spec.maximumTerrainSamples >= 32 && spec.maximumTerrainSamples <= 65536 && bool( height ),
                "Roundabout requires a supported grade plane and retained terrain sampler" );

            struct Port { float x, z, angle, half, chord, length, width; };
            std::array<Port, 6> ports{};
            float minimumChord = spec.outerRadius;
            for( std::size_t i = 0; i < spec.ports.size(); ++i )
            {
                const auto &p = spec.ports[i];
                need( uuid( p.approachId ) && !sameUuid( p.approachId, spec.junctionId ), "Roundabout approach requires a distinct road UUID" );
                for( std::size_t j = 0; j < i; ++j ) need( !sameUuid( p.approachId, spec.ports[j].approachId ), "Roundabout approach UUID repeated" );
                need( finite( p.outward ) && std::abs( p.outward.y ) < .0001f &&
                    std::hypot( p.outward.x, p.outward.z ) > .001f &&
                    std::isfinite( p.width ) && p.width >= 4 && p.width <= 12 && p.width < spec.outerRadius &&
                    std::isfinite( p.length ) && p.length >= 4 && p.length <= 256,
                    "Roundabout approach direction/width/length is invalid" );
                const float scale = 1 / std::hypot( p.outward.x, p.outward.z );
                const float half = std::asin( p.width * .5f / spec.outerRadius );
                const float chord = std::sqrt( spec.outerRadius * spec.outerRadius - p.width * p.width * .25f );
                need( chord - spec.islandRadius >= 3, "Roundabout entry chord pinches the circulating lane below 3m" );
                ports[i] = { p.outward.x * scale, p.outward.z * scale,
                    wrap( std::atan2( p.outward.z, p.outward.x ) ), half, chord, p.length, p.width };
                minimumChord = std::min( minimumChord, chord );
                for( std::size_t j = 0; j < i; ++j )
                    need( std::abs( difference( ports[i].angle, ports[j].angle ) ) >= half + ports[j].half + .12f,
                        "Roundabout approach mouths overlap or have insufficient separating arc" );
            }
            CityRoadRoundaboutBake candidate;
            candidate.junctionId = spec.junctionId; candidate.center = spec.center;
            candidate.outerRadius = spec.outerRadius; candidate.islandRadius = spec.islandRadius;
            const auto plane = [&]( float x, float z ) {
                return spec.center.y + spec.gradeX * ( x - spec.center.x ) + spec.gradeZ * ( z - spec.center.z );
            };
            const float normScale = 1 / std::sqrt( 1 + spec.gradeX * spec.gradeX + spec.gradeZ * spec.gradeZ );
            const Vector3F up{ -spec.gradeX * normScale, normScale, -spec.gradeZ * normScale };
            const auto vertex = [&]( float x, float z, float elevation, Vector3F normal ) {
                const auto index = u32( candidate.vertices.size() );
                candidate.vertices.push_back( { { x, plane( x, z ) + elevation, z }, normal,
                    ( x - spec.center.x ) * .25f, ( z - spec.center.z ) * .25f } );
                return index;
            };
            const auto triangle = [&]( u32 a, u32 b, u32 c, CityRoadRoundaboutSurface kind, Vector3F normal ) {
                const auto &aa = candidate.vertices[a].position, &bb = candidate.vertices[b].position,
                    &cc = candidate.vertices[c].position;
                const float ux = bb.x - aa.x, uy = bb.y - aa.y, uz = bb.z - aa.z;
                const float vx = cc.x - aa.x, vy = cc.y - aa.y, vz = cc.z - aa.z;
                const float nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
                need( nx * nx + ny * ny + nz * nz > 1.e-12f, "Roundabout produced a degenerate triangle" );
                if( nx * normal.x + ny * normal.y + nz * normal.z < 0 ) std::swap( b, c );
                candidate.indices.push_back( a ); candidate.indices.push_back( b ); candidate.indices.push_back( c );
                candidate.triangleKinds.push_back( kind );
            };
            SourceBudgetVector<float> angles;
            angles.reserve( spec.ringSamples );
            const unsigned regular = spec.ringSamples - unsigned( spec.ports.size() * 2 );
            for( unsigned i = 0; i < regular; ++i ) angles.push_back( tau * i / regular );
            for( std::size_t i = 0; i < spec.ports.size(); ++i )
            { angles.push_back( wrap( ports[i].angle - ports[i].half ) ); angles.push_back( wrap( ports[i].angle + ports[i].half ) ); }
            std::sort( angles.begin(), angles.end() );
            angles.erase( std::unique( angles.begin(), angles.end(), []( float a, float b ) { return std::abs( a - b ) < 1.e-6f; } ), angles.end() );
            if( tau - angles.back() + angles.front() < 1.e-6f ) angles.pop_back();
            const std::size_t count = angles.size();
            candidate.vertices.reserve( count * 8 + spec.ports.size() * 34 + 1 );
            candidate.indices.reserve( count * 21 + spec.ports.size() * 54 );
            candidate.triangleKinds.reserve( count * 7 + spec.ports.size() * 18 );
            SourceBudgetVector<float> innerRadii;
            innerRadii.reserve( count );
            for( std::size_t i = 0; i < count; ++i )
            {
                const float before = wrap( angles[i] - angles[( i + count - 1 ) % count] );
                const float after = wrap( angles[( i + 1 ) % count] - angles[i] );
                // Circumscribe the island: every straight mesh edge clears the
                // exact island circle, rather than cutting through it as a chord.
                const float inner = spec.islandRadius / std::cos( std::max( before, after ) * .5f );
                float outer = spec.outerRadius;
                for( std::size_t p = 0; p < spec.ports.size(); ++p )
                {
                    const float delta = difference( angles[i], ports[p].angle );
                    if( std::abs( delta ) <= ports[p].half + 1.e-6f ) outer = std::min( outer, ports[p].chord / std::cos( delta ) );
                }
                need( outer - inner >= 2.9f, "Roundabout sampling pinches the circulating lane" );
                innerRadii.push_back( inner );
                const float c = std::cos( angles[i] ), s = std::sin( angles[i] );
                vertex( spec.center.x + c * inner, spec.center.z + s * inner, 0, up );
                vertex( spec.center.x + c * outer, spec.center.z + s * outer, 0, up );
            }
            for( u32 i = 0; i < count; ++i )
            {
                const u32 j = ( i + 1 ) % u32( count );
                triangle( i * 2, j * 2 + 1, i * 2 + 1, CityRoadRoundaboutSurface::Circulation, up );
                triangle( i * 2, j * 2, j * 2 + 1, CityRoadRoundaboutSurface::Circulation, up );
            }
            const u32 islandCentre = vertex( spec.center.x, spec.center.z, spec.islandHeight, up );
            const u32 islandBegin = u32( candidate.vertices.size() );
            for( std::size_t i = 0; i < count; ++i )
            {
                const float c = std::cos( angles[i] ), s = std::sin( angles[i] );
                vertex( spec.center.x + c * ( spec.islandRadius - spec.kerbWidth ),
                    spec.center.z + s * ( spec.islandRadius - spec.kerbWidth ), spec.islandHeight, up );
                vertex( spec.center.x + c * innerRadii[i], spec.center.z + s * innerRadii[i], spec.islandHeight, up );
            }
            for( u32 i = 0; i < count; ++i )
            {
                const u32 j = ( i + 1 ) % u32( count );
                triangle( islandCentre, islandBegin + j * 2, islandBegin + i * 2, CityRoadRoundaboutSurface::Island, up );
                triangle( islandBegin + i * 2, islandBegin + j * 2, islandBegin + j * 2 + 1, CityRoadRoundaboutSurface::Kerb, up );
                triangle( islandBegin + i * 2, islandBegin + j * 2 + 1, islandBegin + i * 2 + 1, CityRoadRoundaboutSurface::Kerb, up );
                const float ai = angles[i], aj = angles[j];
                const Vector3F ni{ std::cos( ai ), 0, std::sin( ai ) }, nj{ std::cos( aj ), 0, std::sin( aj ) };
                const u32 v0 = vertex( spec.center.x + ni.x * innerRadii[i], spec.center.z + ni.z * innerRadii[i], 0, ni );
                const u32 v1 = vertex( spec.center.x + nj.x * innerRadii[j], spec.center.z + nj.z * innerRadii[j], 0, nj );
                const u32 v2 = vertex( spec.center.x + nj.x * innerRadii[j], spec.center.z + nj.z * innerRadii[j], spec.islandHeight, nj );
                const u32 v3 = vertex( spec.center.x + ni.x * innerRadii[i], spec.center.z + ni.z * innerRadii[i], spec.islandHeight, ni );
                const float u0 = ai * spec.islandRadius * .25f;
                const float u1 = ( j ? aj : aj + tau ) * spec.islandRadius * .25f;
                candidate.vertices[v0].u = candidate.vertices[v3].u = u0;
                candidate.vertices[v1].u = candidate.vertices[v2].u = u1;
                candidate.vertices[v0].v = candidate.vertices[v1].v = 0;
                candidate.vertices[v2].v = candidate.vertices[v3].v = spec.islandHeight * .25f;
                triangle( v0, v1, v2, CityRoadRoundaboutSurface::Kerb, ni );
                triangle( v0, v2, v3, CityRoadRoundaboutSurface::Kerb, ni );
            }
            const float direction = spec.clockwise ? 1.f : -1.f;
            const float routeRadius = ( spec.islandRadius + minimumChord ) * .5f;
            const auto point = [&]( const Port &p, float along, float across ) {
                const float x = spec.center.x + p.x * along - p.z * across;
                const float z = spec.center.z + p.z * along + p.x * across;
                return Vector3F{ x, plane( x, z ), z };
            };
            candidate.connections.reserve( spec.ports.size() );
            for( std::size_t i = 0; i < spec.ports.size(); ++i )
            {
                const auto &p = ports[i];
                const float half = p.width * .5f;
                const auto a = point( p, p.chord, -half ), b = point( p, p.chord, half );
                const auto c = point( p, spec.outerRadius + p.length, half ), d = point( p, spec.outerRadius + p.length, -half );
                const u32 first = vertex( a.x, a.z, 0, up ); vertex( b.x, b.z, 0, up ); vertex( c.x, c.z, 0, up ); vertex( d.x, d.z, 0, up );
                triangle( first, first + 1, first + 2, CityRoadRoundaboutSurface::Approach, up );
                triangle( first, first + 2, first + 3, CityRoadRoundaboutSurface::Approach, up );
                if( p.width >= 6 && p.length >= 12 )
                {
                    const float nearEnd = spec.outerRadius + 1.5f;
                    const float farEnd = spec.outerRadius + std::min( 8.f, p.length * .5f );
                    const float halfWidth = .3f, taper = .75f;
                    const float height = std::min( .12f, spec.islandHeight );
                    const std::array<Vector3F, 6> refuge = { point( p, nearEnd, 0 ),
                        point( p, nearEnd + taper, halfWidth ), point( p, farEnd - taper, halfWidth ),
                        point( p, farEnd, 0 ), point( p, farEnd - taper, -halfWidth ),
                        point( p, nearEnd + taper, -halfWidth ) };
                    const u32 top = u32( candidate.vertices.size() );
                    for( const auto &v : refuge ) vertex( v.x, v.z, height, up );
                    for( u32 corner = 1; corner + 1 < refuge.size(); ++corner )
                        triangle( top, top + corner, top + corner + 1, CityRoadRoundaboutSurface::Island, up );
                    float perimeter = 0;
                    for( std::size_t side = 0; side < refuge.size(); ++side )
                    {
                        const auto &start = refuge[side], &end = refuge[( side + 1 ) % refuge.size()];
                        const float dx = end.x - start.x, dz = end.z - start.z;
                        const float length = std::hypot( dx, dz );
                        const Vector3F normal{ -dz / length, 0, dx / length };
                        const u32 base = vertex( start.x, start.z, 0, normal );
                        vertex( end.x, end.z, 0, normal ); vertex( end.x, end.z, height, normal );
                        vertex( start.x, start.z, height, normal );
                        candidate.vertices[base].u = candidate.vertices[base + 3].u = perimeter * .25f;
                        candidate.vertices[base + 1].u = candidate.vertices[base + 2].u = ( perimeter + length ) * .25f;
                        candidate.vertices[base].v = candidate.vertices[base + 1].v = 0;
                        candidate.vertices[base + 2].v = candidate.vertices[base + 3].v = height * .25f;
                        triangle( base, base + 1, base + 2, CityRoadRoundaboutSurface::Kerb, normal );
                        triangle( base, base + 2, base + 3, CityRoadRoundaboutSurface::Kerb, normal );
                        perimeter += length;
                    }
                }
                CityRoadRoundaboutConnection connection;
                connection.approachId = spec.ports[i].approachId;
                const float offset = p.width * .25f, ringAlong = std::sqrt( routeRadius * routeRadius - offset * offset );
                connection.entry = { point( p, spec.outerRadius + p.length, -direction * offset ),
                    point( p, p.chord, -direction * offset ), point( p, ringAlong, -direction * offset ) };
                connection.exit = { point( p, ringAlong, direction * offset ),
                    point( p, p.chord, direction * offset ), point( p, spec.outerRadius + p.length, direction * offset ) };
                candidate.connections.push_back( std::move( connection ) );
            }
            candidate.routes.reserve( spec.ports.size() * ( spec.ports.size() - 1 ) );
            for( std::size_t entry = 0; entry < spec.ports.size(); ++entry )
                for( std::size_t exit = 0; exit < spec.ports.size(); ++exit )
                {
                    if( entry == exit ) continue;
                    CityRoadRoundaboutRoute route;
                    route.entryApproachId = spec.ports[entry].approachId; route.exitApproachId = spec.ports[exit].approachId;
                    const float start = ports[entry].angle - direction * std::asin( ports[entry].width * .25f / routeRadius );
                    const float finish = ports[exit].angle + direction * std::asin( ports[exit].width * .25f / routeRadius );
                    const float turn = wrap( direction * ( finish - start ) );
                    const unsigned steps = std::max( 1u, unsigned( std::ceil( turn * regular / tau ) ) );
                    route.points.reserve( steps + 5 );
                    for( const auto p : candidate.connections[entry].entry ) route.points.push_back( p );
                    for( unsigned i = 1; i <= steps; ++i )
                    {
                        const float angle = start + direction * turn * i / steps;
                        const float x = spec.center.x + std::cos( angle ) * routeRadius;
                        const float z = spec.center.z + std::sin( angle ) * routeRadius;
                        route.points.push_back( { x, plane( x, z ), z } );
                    }
                    route.points.push_back( candidate.connections[exit].exit[1] );
                    route.points.push_back( candidate.connections[exit].exit[2] );
                    candidate.routes.push_back( std::move( route ) );
                }

            u32 queries = 0;
            const auto terrain = [&]( float x, float z ) {
                need( ++queries <= spec.maximumTerrainSamples, "Roundabout terrain sample budget exhausted" );
                float y = 0;
                const Vector3F position{ x, plane( x, z ), z };
                need( height( position, y ) && std::isfinite( y ), "Roundabout terrain is missing, holed or nonfinite" );
                need( std::abs( y - position.y ) <= spec.maxTerrainDeviation,
                    "Roundabout cannot fit the retained terrain within its grade-plane tolerance" );
            };
            for( const auto &v : candidate.vertices ) terrain( v.position.x, v.position.z );
            const int across = int( std::ceil( spec.outerRadius * 2 / spec.terrainSampleStep ) );
            for( int x = 0; x <= across; ++x ) for( int z = 0; z <= across; ++z )
            {
                const float px = -spec.outerRadius + spec.outerRadius * 2 * x / across;
                const float pz = -spec.outerRadius + spec.outerRadius * 2 * z / across;
                if( px * px + pz * pz <= spec.outerRadius * spec.outerRadius ) terrain( spec.center.x + px, spec.center.z + pz );
            }
            for( std::size_t i = 0; i < spec.ports.size(); ++i )
            {
                const auto &p = ports[i];
                const unsigned lengthSteps = unsigned( std::ceil( ( spec.outerRadius + p.length - p.chord ) / spec.terrainSampleStep ) );
                const unsigned widthSteps = unsigned( std::ceil( p.width / spec.terrainSampleStep ) );
                for( unsigned along = 0; along <= lengthSteps; ++along ) for( unsigned side = 0; side <= widthSteps; ++side )
                {
                    const auto sample = point( p, p.chord + ( spec.outerRadius + p.length - p.chord ) * along / lengthSteps,
                        -p.width * .5f + p.width * side / widthSteps );
                    terrain( sample.x, sample.z );
                }
            }
            output.swap( candidate );
            error.clear();
            return true;
        }
        catch( const std::exception &exception ) { error = exception.what(); return false; }
        catch( ... ) { error = "Roundabout terrain callback failed"; return false; }
    }
}
